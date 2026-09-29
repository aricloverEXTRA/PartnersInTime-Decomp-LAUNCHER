#!/usr/bin/env python3
"""
Compile every linked translation unit with clang instead of Metrowerks.

This is the "second build target" proposed in docs/PC_PORT.md: a cheap,
additive check that keeps the tree honest about how portable the reconstructed
sources actually are. The Metrowerks build optimises for byte identity, which is
the wrong objective for a port, so portability is otherwise only discovered when
someone tries to move an architecture and inherits all of the surprise at once.

This tool never touches the matching build. It reads the authoritative list of
translation units from config/<version>/arm9/linked_sources.txt, compiles each
one to an object file under build/portability/, and reports what failed and why.
It does not link, so a missing platform symbol is not reported as an error.

Every Metrowerks flag used by tools/configure.py is mapped to its clang
equivalent so the result reflects the real build rather than an easier one.
"""

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

# Metrowerks flag -> clang flag, from CC_FLAGS in tools/configure.py. Each entry
# is (metrowerks, clang, note) and the note records anything that is only an
# approximation, so the report can state the limits instead of implying a
# faithful ABI match.
FLAG_MAP = [
    ("-O4,p", "-Oz", "aggressive size/speed; clang has no direct equivalent"),
    ("-enum int", "-fshort-enums", "exact"),
    ("-char signed", "-fsigned-char", "exact"),
    ("-str noreuse", "-fno-merge-constants", "exact"),
    ("-proc arm946e", "-mcpu=arm946e-s", "exact"),
    ("-gccext,on", "-std=gnu89", "GNU extensions on by default in gnu dialects"),
    ("-fp soft", "-mfloat-abi=soft", "exact"),
    ("-inline noauto", "-fno-inline", "approximate: clang may still inline marked functions"),
    ("-Cpp_exceptions off", "-fno-exceptions", "exact"),
    ("-RTTI off", "-fno-rtti", "exact"),
    ("-w off", "-w", "exact"),
    ("-sym on", "-g", "exact"),
    ("-nolink", "-c", "exact"),
]

# Extra flags that are not Metrowerks flags but are required to parse the
# sources at all, because the tree is C89 in practice while clang defaults to a
# later dialect.
COMPAT_FLAGS = [
    # Many files call NitroSDK entry points that no header declares and that
    # they do not declare locally either. Metrowerks defaults to C89, where an
    # implicit function declaration is legal, so the sources rely on that.
    ("-Wno-implicit-function-declaration", "C89 implicit declarations"),
    ("-Wno-int-conversion", "Metrowerks is laxer about narrowing"),
    ("-ffp-contract=off", "no FMA fusion; Metrowerks soft-float emits none"),
]

# Divergences measured on this target. These are not defects in the
# reconstruction: the size asserts below encode the layout the ROM actually
# uses, and MWCC is the compiler that produced it. They are listed here so a
# failure is recognisable instead of looking like a new problem.
KNOWN_DIVERGENCES = [
    {
        "id": "s64-alignment",
        "units": 175,
        "detail": (
            "MWCC aligns 64-bit integers to 4 bytes on ARM946E with -fp soft; "
            "clang follows AAPCS and aligns them to 8. Any struct containing "
            "s64/u64 therefore gains padding before that member and its own "
            "alignment, so its total size grows."
        ),
        "evidence": [
            "include/game/field_entity.h:753 s64 collision_policy",
            "FieldRuntimeEntity 0x520 (MWCC) vs 0x528 (clang), align 8",
            "include/nitro/os_alarm.h:15,18,19 u64 fire, period, start",
            "OsAlarm 44 (MWCC) vs 48 (clang)",
        ],
        "note": (
            "Do not 'fix' the headers. The asserts are correct for the "
            "matching build. A port that needs clang's natural layout must "
            "state that it diverges from the ROM, because the ROM stores "
            "these structs at MWCC offsets."
        ),
    },
    {
        "id": "metrowerks-inline-asm",
        "units": 24,
        "detail": (
            "These units contain MWCC 'asm { ... }' blocks. Clang has no "
            "equivalent MS-style ARM inline assembly, so the block cannot be "
            "parsed at all."
        ),
        "note": (
            "Some are deliberate byte-matching fragments, for example "
            "src/battle/battle_capture_transform.c:29. A port must supply "
            "these as intrinsics or plain C, not reinterpret them."
        ),
    },
    {
        "id": "lvalue-cast",
        "units": 1,
        "detail": "Assignment to a cast expression, an MWCC extension clang rejects.",
    },
]

# -lang=c++ applies to both languages in the Metrowerks build, so the standard
# is selected per file extension here.
STD_FOR_SUFFIX = {
    ".c": "-std=gnu89",
    ".cpp": "-std=gnu++98",
    ".cc": "-std=gnu++98",
}

ERROR_RE = re.compile(r"^(?P<path>.*?):(?P<line>\d+):(?P<col>\d+): error: (?P<msg>.*)$")


def find_clang(explicit):
    """Locate a clang binary, preferring an explicit path then PATH."""
    if explicit:
        if not Path(explicit).is_file():
            raise SystemExit(f"error: no clang at {explicit}")
        return explicit

    env = os.environ.get("PIT_CLANG")
    if env:
        if not Path(env).is_file():
            raise SystemExit(f"error: PIT_CLANG does not point at a file: {env}")
        return env

    found = shutil.which("clang")
    if found:
        return found

    for candidate in (
        Path(r"C:\Program Files\LLVM\bin\clang.exe"),
        Path(r"C:\Program Files (x86)\LLVM\bin\clang.exe"),
    ):
        if candidate.is_file():
            return str(candidate)

    raise SystemExit(
        "error: clang not found.\n"
        "       Pass --clang <path> or set PIT_CLANG to a clang binary."
    )


def include_dirs(root):
    """Mirror the include list built by tools/configure.py.

    That is the repository include directory plus every libs/*/include.
    """
    dirs = [root / "include"]
    libs = root / "libs"
    if libs.is_dir():
        for entry in sorted(libs.iterdir()):
            candidate = entry / "include"
            if candidate.is_dir():
                dirs.append(candidate)
    return [str(d) for d in dirs if d.is_dir()]


def read_units(root, version):
    """Return the linked translation units for a version.

    This is the same manifest the Metrowerks build uses, so the check covers
    exactly the code that ships in the verified ROM and nothing else.
    """
    path = root / "config" / version / "arm9" / "linked_sources.txt"
    if not path.is_file():
        raise SystemExit(f"error: missing {path}; run tools/configure.py first")

    units = []
    for line in path.read_text(encoding="utf-8").splitlines():
        entry = line.strip()
        if not entry or entry.startswith("#"):
            continue
        if not (root / entry).is_file():
            raise SystemExit(f"error: {entry} is listed but not present")
        units.append(entry)
    return units


def base_flags(root, clang):
    """Build the flag list shared by every translation unit."""
    flags = [
        f"--target=arm-none-eabi",
        f"-mcpu=arm946e-s",
        "-mfloat-abi=soft",
        # Metrowerks wchar_t is two bytes wide on this target.
        "-fshort-wchar",
        # The build has no C or C++ standard library of its own, and the
        # NitroSDK headers provide everything the sources need. Letting clang
        # add its own headers would hide missing includes instead of reporting
        # them.
        "-nostdinc",
    ]
    for _, clang_flag, _ in FLAG_MAP:
        flags.append(clang_flag)
    for flag, _ in COMPAT_FLAGS:
        flags.append(flag)
    for directory in include_dirs(root):
        flags.append(f"-I{directory}")
    return flags


def first_error(text):
    """Return the first clang diagnostic, as (message, path, line)."""
    for line in text.splitlines():
        match = ERROR_RE.match(line.strip())
        if match:
            return match.group("msg"), match.group("path"), int(match.group("line"))
    return None, None, None


def compile_unit(clang, root, out_root, shared, unit, syntax_only):
    """Compile one unit. Returns a result record instead of raising."""
    source = root / unit
    record = {"unit": unit, "ok": False, "error": None, "error_line": None}

    language = STD_FOR_SUFFIX.get(source.suffix)
    if language is None:
        record["error"] = f"unsupported source suffix {source.suffix}"
        return record

    command = [clang, *shared, language, "-c" if not syntax_only else "-fsyntax-only"]
    if not syntax_only:
        target = out_root / (unit + ".o")
        target.parent.mkdir(parents=True, exist_ok=True)
        command += ["-o", str(target)]
    command.append(str(source))

    try:
        done = subprocess.run(
            command, cwd=root, capture_output=True, text=True, errors="replace"
        )
    except OSError as exc:
        record["error"] = f"could not run clang: {exc}"
        return record

    if done.returncode == 0:
        record["ok"] = True
        return record

    message, path, line = first_error(done.stderr or "")
    record["error"] = message or (done.stderr or "").strip().splitlines()[-1:] or [
        "unknown failure"
    ][0]
    if isinstance(record["error"], list):
        record["error"] = record["error"][0]
    record["error_path"] = path
    record["error_line"] = line
    return record


def main():
    parser = argparse.ArgumentParser(
        description="Compile the linked sources with clang to measure portability."
    )
    parser.add_argument("--clang", help="path to a clang binary")
    parser.add_argument(
        "--version", default="eur", help="game version directory under config/"
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=os.cpu_count() or 4,
        help="parallel clang invocations",
    )
    parser.add_argument(
        "--syntax-only",
        action="store_true",
        help="stop after semantic analysis instead of writing object files",
    )
    parser.add_argument(
        "--out", type=Path, default=None, help="object output directory"
    )
    parser.add_argument(
        "--report", type=Path, default=None, help="write a JSON report here"
    )
    parser.add_argument(
        "--top",
        type=int,
        default=25,
        help="how many distinct failure kinds to summarise",
    )
    parser.add_argument(
        "--allow-failures",
        action="store_true",
        help="report success even if some units fail",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    clang = find_clang(args.clang)
    units = read_units(root, args.version)
    shared = base_flags(root, clang)

    out_root = args.out or (root / "build" / "portability")
    if not args.syntax_only:
        out_root.mkdir(parents=True, exist_ok=True)

    print(f"clang      : {clang}")
    print(f"version    : {args.version}")
    print(f"units      : {len(units)}")
    print(f"jobs       : {args.jobs}")
    print(f"mode       : {'syntax only' if args.syntax_only else 'full compile'}")
    print()

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [
            pool.submit(compile_unit, clang, root, out_root, shared, unit, args.syntax_only)
            for unit in units
        ]
        for done_index, future in enumerate(concurrent.futures.as_completed(futures), 1):
            results.append(future.result())
            if done_index % 100 == 0 or done_index == len(units):
                print(f"  {done_index}/{len(units)}")

    results.sort(key=lambda r: r["unit"])
    passed = [r for r in results if r["ok"]]
    failed = [r for r in results if not r["ok"]]

    by_language = Counter()
    for unit in units:
        by_language[Path(unit).suffix] += 1

    print()
    print(f"passed     : {len(passed)}/{len(units)}")
    for suffix, total in sorted(by_language.items()):
        good = sum(1 for r in passed if Path(r["unit"]).suffix == suffix)
        print(f"  {suffix:<6} {good}/{total}")
    print(f"failed     : {len(failed)}")

    if failed:
        print()
        print("failure kinds:")
        for message, count in Counter(r["error"] for r in failed).most_common(args.top):
            print(f"  {count:>5}  {message}")
        print()
        print("first 20 failing units:")
        for record in failed[:20]:
            location = record.get("error_line")
            where = f":{location}" if location else ""
            print(f"  {record['unit']}{where}  {record['error']}")

        print()
        print("known divergences on this target:")
        for entry in KNOWN_DIVERGENCES:
            print(f"  [{entry['id']}] ~{entry['units']} units")
            print(f"    {entry['detail']}")
            if entry.get("note"):
                print(f"    note: {entry['note']}")

    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(
            json.dumps(
                {
                    "clang": clang,
                    "version": args.version,
                    "units": len(units),
                    "passed": len(passed),
                    "failed": len(failed),
                    "flag_map": [
                        {"metrowerks": m, "clang": c, "note": n}
                        for m, c, n in FLAG_MAP
                    ],
                    "compat_flags": [
                        {"flag": f, "reason": r} for f, r in COMPAT_FLAGS
                    ],
                    "known_divergences": KNOWN_DIVERGENCES,
                    "failures": [
                        {
                            "unit": r["unit"],
                            "error": r["error"],
                            "line": r.get("error_line"),
                        }
                        for r in failed
                    ],
                },
                indent=2,
            ),
            encoding="utf-8",
        )
        print()
        print(f"report     : {args.report}")

    if failed and not args.allow_failures:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
