#!/usr/bin/env bash
#
# Builds the Android patcher APK with the platform tools directly.
#
# This is the POSIX counterpart of android/build-apk.ps1 and does the same
# steps in the same order, because the APK format requires that order and
# because a script that continues past a failed native command would report a
# success that never happened. `set -e` plus the explicit step function is what
# makes a failure stop the build.
#
# There is no Gradle, no Android Gradle Plugin and no NDK: the app is plain Java
# against the platform APIs, so aapt2, javac, d8, zipalign and apksigner are
# enough. It compiles the same generated PatchData the Windows build uses, so
# the two patchers cannot drift apart.
#
# Usage:
#   ./build-apk.sh [--unsigned]
#
# Environment:
#   ANDROID_HOME / ANDROID_SDK_ROOT   Android SDK location
#   JAVA_HOME                         JDK 17 or newer
#   ANDROID_KEYSTORE                  base64 keystore for a signed build
#   ANDROID_KEYSTORE_PASSWORD         keystore and key password
#   ANDROID_KEY_ALIAS                 key alias inside the keystore
set -euo pipefail

unsigned=0
for arg in "$@"; do
  case "$arg" in
    --unsigned) unsigned=1 ;;
    *) echo "unknown argument: $arg" >&2; exit 2 ;;
  esac
done

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
app_root="$here/app"
manifest_dir="$app_root/src/main"
java_root="$manifest_dir/java"
res_root="$manifest_dir/res"
# The build directory is wiped up front so a stale gen/ or classes/ from an
# earlier run cannot be picked up: aapt2's generated R.java and a deleted source
# would otherwise both leak into the APK.
out_dir="${OUT_DIR:-$app_root/build}"
rm -rf "$out_dir"
mkdir -p "$out_dir"

# --- locate the toolchain ---------------------------------------------------

sdk="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
if [ -z "$sdk" ] || [ ! -d "$sdk" ]; then
  echo "No Android SDK: set ANDROID_HOME." >&2
  exit 1
fi

# Highest version present, so a runner that ships several works without edits.
build_tools="$(ls -1d "$sdk"/build-tools/*/ 2>/dev/null | sort -V | tail -1)"
platform="$(ls -1d "$sdk"/platforms/*/ 2>/dev/null | sort -V | tail -1)"
if [ -z "$build_tools" ] || [ -z "$platform" ]; then
  echo "No build-tools or platforms under $sdk" >&2
  exit 1
fi
build_tools="${build_tools%/}"
platform="${platform%/}"

android_jar="$platform/android.jar"
for tool in javac keytool; do
  if [ -n "${JAVA_HOME:-}" ]; then
    hash -r
    export PATH="$JAVA_HOME/bin:$PATH"
  fi
  command -v "$tool" >/dev/null 2>&1 || { echo "No $tool on PATH; set JAVA_HOME." >&2; exit 1; }
done

step() {
  local name="$1"; shift
  echo "==> $name"
  "$@"
}

# 1. Resources and manifest. aapt2 compiles the res tree to flat files, then link
#    turns those plus the manifest into the packaged resources of the APK. The
#    link target is an .apk rather than a .ap_ because aapt2 only puts the
#    compiled AndroidManifest.xml at the archive root of an .apk, and apksigner
#    rejects an APK with no manifest.
step 'aapt2 compile' "$build_tools/aapt2" compile --dir "$res_root" -o "$out_dir/res.zip"

step 'aapt2 link' "$build_tools/aapt2" link \
  -I "$android_jar" \
  --manifest "$manifest_dir/AndroidManifest.xml" \
  -o "$out_dir/resources.apk" \
  --java "$out_dir/gen" \
  "$out_dir/res.zip"

# 2. Java to class files, then classes to dex. -source/-target pin the bytecode
#    level so d8 sees something it can convert on any toolchain version.
classes="$out_dir/classes"
dex_dir="$out_dir/dex"
mkdir -p "$classes" "$dex_dir"

step 'javac' javac -source 8 -target 8 -nowarn \
  -encoding UTF-8 -classpath "$android_jar" -d "$classes" \
  $(find "$java_root" "$out_dir/gen" -name '*.java' 2>/dev/null)

step 'd8' "$build_tools/d8" --lib "$android_jar" --min-api 21 --output "$dex_dir" \
  $(find "$classes" -name '*.class')

# 3. Assemble. The manifest and resources are carried across from the linked APK
#    with their own entry names, then the dex is appended. python3 does the zip
#    because it is on every runner and lets every entry be written uncompressed
#    with a fixed timestamp, so two builds of identical sources are byte
#    identical.
step 'assemble' python3 - "$out_dir/resources.apk" "$dex_dir/classes.dex" "$out_dir/pit-patcher-unsigned.apk" <<'PY'
import sys
import zipfile

resources, dex, out = sys.argv[1:4]

with zipfile.ZipFile(out, "w") as target:
    with zipfile.ZipFile(resources) as source:
        for info in source.infolist():
            if info.is_dir():
                continue
            # resources.arsc and the manifest must be stored uncompressed; the
            # rest of the linked archive is already stored by aapt2.
            data = source.read(info.filename)
            item = zipfile.ZipInfo(info.filename, date_time=(1980, 1, 1, 0, 0, 0))
            item.compress_type = zipfile.ZIP_STORED
            item.external_attr = info.external_attr
            target.writestr(item, data)

    item = zipfile.ZipInfo("classes.dex", date_time=(1980, 1, 1, 0, 0, 0))
    item.compress_type = zipfile.ZIP_STORED
    with open(dex, "rb") as handle:
        target.writestr(item, handle.read())
PY

# 4. Align, then sign. Alignment must precede signing because signing rewrites
#    the central directory, which would undo it.
aligned="$out_dir/pit-patcher-aligned.apk"
step 'zipalign' "$build_tools/zipalign" -f -p 4 "$out_dir/pit-patcher-unsigned.apk" "$aligned"

if [ "$unsigned" -eq 1 ]; then
  apk="$out_dir/pit-patcher-unsigned.apk"
  cp "$aligned" "$apk"
  echo "Unsigned build; it will not install until it is signed."
else
  [ -n "${ANDROID_KEYSTORE:-}" ] || {
    echo "No ANDROID_KEYSTORE. Pass --unsigned for an unsigned build." >&2
    exit 1
  }
  keystore="$out_dir/signing.keystore"
  # Decoded from base64 so the key is a repository secret rather than a file in
  # the tree. It is written under the build directory, which is not committed.
  printf '%s' "$ANDROID_KEYSTORE" | base64 --decode > "$keystore"

  password="${ANDROID_KEYSTORE_PASSWORD:?set ANDROID_KEYSTORE_PASSWORD}"
  alias_name="${ANDROID_KEY_ALIAS:-aricloverEXTRA}"
  apk="$out_dir/pit-patcher-release.apk"

  # --v3-signing-enabled false keeps the signature scheme v1/v2 only, so the APK
  # verifies all the way down to the declared minSdk 21. A v3 block carries a
  # SHA-256 signature that older platforms reject outright.
  step 'apksigner' "$build_tools/apksigner" sign \
    --ks "$keystore" --ks-pass "pass:$password" --key-pass "pass:$password" \
    --ks-key-alias "$alias_name" --min-sdk-version 21 \
    --v3-signing-enabled false --out "$apk" "$aligned"

  step 'apksigner verify' "$build_tools/apksigner" verify --min-sdk-version 21 "$apk"
  rm -f "$keystore"
fi

echo
# The intermediates are removed so that a caller's `pit-patcher-*.apk` glob
# matches exactly one file. Otherwise the aligned copy stays behind on the
# unsigned path and the caller has to guess which of the two is the result.
rm -f "$aligned"
[ "$apk" = "$out_dir/pit-patcher-unsigned.apk" ] || rm -f "$out_dir/pit-patcher-unsigned.apk"

echo "APK: $apk"
echo "Size: $(du -h "$apk" | cut -f1)"
echo
echo "This APK contains no ROM data. It needs the user's own EUR cartridge ROM."
