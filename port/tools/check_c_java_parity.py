#!/usr/bin/env python3
"""Check that the C and Java patchers produce the same ROM from the same input.

The two builds implement the same pipeline in two languages, from one generated
plan. The failure this guards against is not a crash but a quiet divergence:
one build rounding a .5 boundary differently, or clamping a field the other
does not, and each producing a plausible-looking ROM of its own. Comparing the
two outputs byte for byte is the only check that catches that.

Requires a JDK on PATH (or JAVA_HOME) and a compiled pit_patch. Usage:

    python tools/check_c_java_parity.py --rom path/to/rom.nds
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# Only these four are Android-independent. MainActivity and PatcherView pull in
# android.*, so they cannot run on a desktop JVM and are excluded by design.
PORTABLE_SOURCES = ["PatchData.java", "Patcher.java", "NitroFs.java", "Sha1.java",
                    "ModProfile.java"]


def sha1_file(path: Path) -> str:
    h = hashlib.sha1()
    with path.open("rb") as fp:
        for block in iter(lambda: fp.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def run(cmd: list[str], cwd: Path) -> None:
    result = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(f"command failed ({result.returncode}): {' '.join(cmd)}")


def compile_java(port: Path, out_dir: Path) -> None:
    javac = shutil.which("javac")
    java_home = os.environ.get("JAVA_HOME")
    if not javac and java_home:
        candidate = Path(java_home) / "bin" / "javac"
        javac = str(candidate) if candidate.is_file() else None
    if not javac:
        raise SystemExit("no javac found: set JAVA_HOME or put javac on PATH")

    package_dir = port / "android/app/src/main/java/com/partnersintime/patcher"
    sources = [str(package_dir / name) for name in PORTABLE_SOURCES]
    sources.append(str(port / "tools/java/com/partnersintime/patcher/PitSelfTest.java"))
    missing = [s for s in sources if not Path(s).is_file()]
    if missing:
        raise SystemExit("missing source(s): " + ", ".join(missing))

    run([javac, "-Xlint:all", "-d", str(out_dir)] + sources, port)


def compare_pair(java: str, c: str, java_cmd: list[str], c_cmd: list[str],
                 port: Path, label: str) -> bool:
    """Run one build of each patcher and require byte-identical output."""
    run(java_cmd, port)
    run(c_cmd, port)

    java_hash = sha1_file(java)
    c_hash = sha1_file(c)

    print(f"java  {java_hash}")
    print(f"c     {c_hash}")
    if java_hash != c_hash:
        print(f"MISMATCH ({label}): the two patchers disagree")
        return False
    print(f"EXACT MATCH ({label}): both patchers produced identical ROMs")
    return True


# Fixtures for the profile loader. Both readers must accept, truncate, and refuse
# exactly the same profiles: the loader decides what a profile does to the ROM,
# so a disagreement here would mean one launcher patching differently from the
# other with no error anywhere. Each entry is (directory, transforms JSON, note).
#
# The two readers phrase their diagnostics differently, so the assertion is on
# which profiles load and what each one's transforms are, not on message text.
LOADER_FIXTURES = {
    # A disabled transform is retained for display and skipped when patching.
    "disabled": ('[{"field":"max_hp","scale":3,"enabled":false},'
                 '{"field":"power","scale":2}]'),
    # Over-long cosmetic strings truncate; the shipped hard_mode description is
    # longer than the buffer, so refusing it would break the bundled profile.
    "long_text": '[{"field":"power","scale":2}]',
    # Both must refuse: no transform is enabled.
    "all_disabled": '[{"field":"power","scale":2,"enabled":false}]',
    # Both must refuse: one field named twice would scale it twice, making the
    # written bytes depend on array order.
    "duplicate_field": ('[{"field":"power","scale":2},'
                        '{"field":"power","scale":3}]'),
    # Both must refuse: the id is identity, and truncating it would let two
    # profiles become indistinguishable.
    "long_id": '[{"field":"power","scale":2}]',
    # Exactly at each limit. These must load, and the summary line carries the
    # stored text, so this also catches a reader truncating one character early.
    "id_at_limit": '[{"field":"power","scale":2}]',
    "field_at_limit": None,
}


def write_profile(mods_dir: Path, name: str, transforms: str, *,
                  profile_id: str | None = None,
                  profile_name: str | None = None,
                  description: str | None = None) -> None:
    folder = mods_dir / name
    folder.mkdir(parents=True, exist_ok=True)
    body = {
        "schema": "pit-mod-profile-v1",
        "id": profile_id if profile_id is not None else name,
        "name": profile_name if profile_name is not None else name,
        "version": "1.0.0",
        "transforms": json.loads(transforms),
    }
    if description is not None:
        body["description"] = description
    (folder / "profile.json").write_text(json.dumps(body), encoding="utf-8")


def list_mods(cmd: list[str], port: Path) -> list[str]:
    """Run a --list-mods invocation and return its loaded-profile lines.

    Only lines naming a loaded profile are kept, so the result is comparable
    between readers that word their skip diagnostics differently.
    """
    result = subprocess.run(cmd, cwd=str(port), capture_output=True, text=True)
    return [line.rstrip() for line in result.stdout.splitlines()
            if line.rstrip().endswith("transforms)")]


def check_loader_parity(c_cmd: list[str], java_cmd: list[str], port: Path,
                        work: Path) -> bool:
    """Require both readers to load and refuse the same profile fixtures."""
    ok = True
    for name, transforms in LOADER_FIXTURES.items():
        mods_dir = work / f"mods-{name}"
        if name == "long_id":
            write_profile(mods_dir, name, transforms,
                          profile_id="i" * (32 + 1), profile_name=name)
        elif name == "id_at_limit":
            # Exactly at the limit: both must accept, since the limits mean
            # character counts rather than buffer sizes.
            write_profile(mods_dir, name, transforms,
                          profile_id="i" * 32, profile_name=name)
        elif name == "long_text":
            write_profile(mods_dir, name, transforms, profile_name="n" * 64,
                          description="d" * 400)
        elif name == "field_at_limit":
            write_profile(mods_dir, name, '[{"field":"' + "f" * 24
                                         + '","scale":2}]')
        else:
            write_profile(mods_dir, name, transforms)

        c_out = list_mods(c_cmd + ["--mods-dir", str(mods_dir)], port)
        java_out = list_mods(java_cmd + ["--mods-dir", str(mods_dir)], port)

        # --list-mods is a report, not a validation gate: both drivers exit 0
        # even when every profile was refused. The signal is which profiles are
        # listed, so compare exactly the summary lines naming a loaded profile.
        # That carries the stored name, id, version and transform count, so it
        # also catches a reader truncating a string one character early, while
        # staying independent of each reader's skip-reason wording.
        if c_out != java_out:
            ok = False
            print(f"MISMATCH (profile {name}): the readers listed it differently")
            print(f"                       c    {c_out}")
            print(f"                       java {java_out}")
        elif not c_out:
            print(f"EXACT MATCH (profile {name}): both refused")
        else:
            print(f"EXACT MATCH (profile {name}): both loaded ({c_out[0]})")
    return ok


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rom", type=Path, required=True,
                    help="the supported EUR ROM, kept outside the repository")
    ap.add_argument("--port", type=Path, default=Path("."))
    ap.add_argument("--pit-patch", type=Path, default=Path("build/pit_patch.exe"),
                    help="the compiled C driver")
    args = ap.parse_args()

    port = args.port.resolve()
    if not args.rom.is_file():
        raise SystemExit(f"ROM not found: {args.rom}")
    if not (port / args.pit_patch).is_file():
        raise SystemExit(f"C driver not found: {port / args.pit_patch} "
                         "(build it first)")

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        classes = work / "classes"
        classes.mkdir()
        java_out = work / "java.nds"
        c_out = work / "c.nds"
        java_prep = work / "java_prepared.nds"
        c_prep = work / "c_prepared.nds"

        print("compiling the Java patcher...")
        compile_java(port, classes)

        java = shutil.which("java")
        java_home = os.environ.get("JAVA_HOME")
        if not java and java_home:
            candidate = Path(java_home) / "bin" / "java"
            java = str(candidate) if candidate.is_file() else None
        if not java:
            raise SystemExit("no java found: set JAVA_HOME or put java on PATH")

        print("running the Java patcher (plan)...")
        java_base = [java, "-cp", str(classes),
                     "com.partnersintime.patcher.PitSelfTest"]
        c_base = [str(port / args.pit_patch)]
        ok = compare_pair(
            java_out, c_out,
            java_base + [str(args.rom), str(java_out)],
            c_base + [str(args.rom), str(c_out)],
            port, "plan")
        if not ok:
            return 1

        print()
        print("running the Java patcher (--no-mods)...")
        ok = compare_pair(
            java_prep, c_prep,
            java_base + ["--no-mods", str(args.rom), str(java_prep)],
            c_base + ["--no-mods", str(args.rom), str(c_prep)],
            port, "no-mods")
        if not ok:
            return 1

        print()
        print("checking that both readers load profiles the same way...")
        if not check_loader_parity(c_base + ["--list-mods"],
                                   java_base + ["--list-mods"],
                                   port, work):
            return 1

        return 0


if __name__ == "__main__":
    sys.exit(main())
