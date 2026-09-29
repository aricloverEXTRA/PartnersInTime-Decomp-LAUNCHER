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
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# Only these four are Android-independent. MainActivity and PatcherView pull in
# android.*, so they cannot run on a desktop JVM and are excluded by design.
PORTABLE_SOURCES = ["PatchData.java", "Patcher.java", "NitroFs.java", "Sha1.java"]


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

        print("compiling the Java patcher...")
        compile_java(port, classes)

        java = shutil.which("java")
        java_home = os.environ.get("JAVA_HOME")
        if not java and java_home:
            candidate = Path(java_home) / "bin" / "java"
            java = str(candidate) if candidate.is_file() else None
        if not java:
            raise SystemExit("no java found: set JAVA_HOME or put java on PATH")

        print("running the Java patcher...")
        run([java, "-cp", str(classes),
             "com.partnersintime.patcher.PitSelfTest", str(args.rom), str(java_out)], port)

        print("running the C patcher...")
        run([str(port / args.pit_patch), str(args.rom), str(c_out)], port)

        java_hash = sha1_file(java_out)
        c_hash = sha1_file(c_out)

        print()
        print(f"java  {java_hash}")
        print(f"c     {c_hash}")
        if java_hash != c_hash:
            print("MISMATCH: the two patchers disagree")
            return 1
        print("EXACT MATCH: both patchers produced identical ROMs")
        return 0


if __name__ == "__main__":
    sys.exit(main())
