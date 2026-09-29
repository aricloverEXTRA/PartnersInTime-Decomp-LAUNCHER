#!/usr/bin/env python3
"""Check that the Android layout mirrors the Windows layout.

Both patchers draw the same 480x320 screen by hand, and both declare their
geometry and palette as a block of integer constants. Nothing makes those two
blocks agree: editing the C renderer's macros and forgetting the Java view
would produce two launchers that look similar but are laid out differently, and
only a human comparing screenshots on two platforms would notice.

This reads both files, pairs each C macro with the Java constant of the same
name, and fails if any paired value differs. The C side writes its colours as
``PIT_ARGB(255, 0xDE, 0x94, 0x29)`` and its geometry as parenthesised
arithmetic over earlier macros; both are resolved here rather than compared as
text, so a reformat on either side is not a failure but a changed number is.

The two identifier namespaces are also checked against each other: a layout or
palette constant that exists on only one side is a silent difference, because
the missing side either hardcodes a literal or has lost a feature.

Exit status is 0 when the two blocks agree, 1 otherwise.
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
C_UI = ROOT / "src" / "platform" / "sdl2" / "pit_patcher_ui.c"
JAVA_UI = (
    ROOT
    / "android"
    / "app"
    / "src"
    / "main"
    / "java"
    / "com"
    / "partnersintime"
    / "patcher"
    / "PatcherView.java"
)

C_INT = re.compile(r"^#define\s+([A-Z][A-Z0-9_]*)\s+(.+?)\s*$")
JAVA_INT = re.compile(
    r"^\s*(?:public|private)?\s*static\s+final\s+int\s+([A-Z][A-Z0-9_]*)\s*=\s*(.+?);\s*$"
)

# Names that are not part of the shared contract.
#
# LOG_LEVEL_* are the C enum values for log severity; Android reuses the same
# numbers but exposes them as a separate public enum rather than macros, and
# they are not a layout or colour.
SKIP = {
    "LOG_LEVEL_INFO",
    "LOG_LEVEL_OK",
    "LOG_LEVEL_WARN",
    "LOG_LEVEL_ERROR",
}

# The C log levels are an enum rather than macros, so the enum body is collected
# as ordinary integer constants. The Android view mirrors the same four values.
C_ENUM_OPEN = re.compile(r"^typedef\s+enum\s*\{")
C_ENUM_CLOSE = re.compile(r"^\}\s*\w+\s*;")

# Names that may exist on only one side. Each entry needs a reason: they are
# identity enumerations or drawing parameters, not a shared measurement.
ONE_SIDED = {
    # Button identities. The C side uses an enum and the Java side private ints
    # for its hit-test slots; neither is a measurement.
    "BTN_BROWSE_IN",
    "BTN_BROWSE_OUT",
    "BTN_PATCH",
    "BTN_BROWSE_SOURCE",
    "BTN_BROWSE_OUTPUT",
}


def strip_comments(expr: str) -> str:
    expr = re.sub(r"/\*.*?\*/", " ", expr)
    return re.sub(r"//.*$", "", expr).strip().rstrip(";").strip()


def arithexpr(expr: str, env: dict[str, int]) -> int:
    """Evaluate an integer expression built from literals, names and + - * / ( ).

    Names are looked up in env, which is filled in dependency order below. Only
    these tokens are accepted: anything else is a source mistake rather than a
    value to compare, so it is reported instead of guessed.
    """

    if not re.fullmatch(r"[0-9A-Za-z_+\-*/() ]+", expr):
        raise ValueError(f"unsupported expression: {expr!r}")

    # Integer division truncating toward zero, like C and Java.
    def trunc_div(a: int, b: int) -> int:
        q = abs(a) // abs(b)
        return -q if (a < 0) != (b < 0) else q

    def walk(text: str) -> str:
        # Replace every bare name with its value, longest first so a name that
        # is a prefix of another cannot shadow it.
        def sub(m: re.Match) -> str:
            name = m.group(0)
            if name not in env:
                raise ValueError(f"unknown name {name!r} in {text!r}")
            return str(env[name])

        return re.sub(r"\b[A-Za-z_][A-Za-z0-9_]*\b", sub, text)

    resolved = walk(expr)
    if not re.fullmatch(r"[0-9+\-*/() ]+", resolved):
        raise ValueError(f"unresolved tokens in {expr!r}")
    value = eval(  # noqa: S307 - the regexes above restrict this to arithmetic
        compile(resolved, "<layout>", "eval"),
        {"__builtins__": {}},
        {"__builtins__": {}},
    )
    return int(value)


def parse_argb(expr: str) -> int:
    """Resolve ``PIT_ARGB(a, r, g, b)`` to the 0xAARRGGBB integer it produces.

    The C macro is ``(a << 24) | (r << 16) | (g << 8) | b``. The alpha is
    written as a decimal and the colour channels as hex, matching the palette
    block's use of ROM channel values, so both spellings are accepted.
    """

    m = re.fullmatch(r"PIT_ARGB\(([^,]+),([^,]+),([^,]+),([^,]+)\)", expr)
    if not m:
        raise ValueError(f"unsupported expression: {expr!r}")
    channels = []
    for part in m.groups():
        part = part.strip()
        if re.fullmatch(r"0[xX][0-9a-fA-F]+", part):
            channels.append(int(part, 16))
        elif re.fullmatch(r"[0-9]+", part):
            channels.append(int(part, 10))
        else:
            raise ValueError(f"unsupported PIT_ARGB argument: {part!r}")
    a, r, g, b = channels
    return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF)


def evaluate(expr: str, env: dict[str, int]) -> int:
    expr = strip_comments(expr)
    if expr.startswith("PIT_ARGB("):
        return parse_argb(expr)
    if re.fullmatch(r"0[xX][0-9a-fA-F]+", expr):
        return int(expr, 16)
    return arithexpr(expr, env)


def collect(path: pathlib.Path, pattern: re.Pattern) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        m = pattern.match(line)
        if m and m.group(1) not in SKIP:
            values[m.group(1)] = m.group(2)
    return values


def collect_c_enums(path: pathlib.Path) -> dict[str, str]:
    """Collect plain integer enum members, which the C side uses for log levels.

    Only bodies whose members are all bare integers or simple `= <int>` forms
    are taken, so an enum of pointers or struct members is skipped instead of
    being misread as a shared constant.
    """

    values: dict[str, str] = {}
    inside = False
    next_value = 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if not inside:
            if C_ENUM_OPEN.match(line):
                inside = True
            continue
        if C_ENUM_CLOSE.match(line):
            inside = False
            continue
        body = line.strip().rstrip(",")
        if not body or body.startswith(("/*", "*", "//")):
            continue
        name, sep, expr = body.partition("=")
        name = name.strip()
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", name):
            values.clear()
            break
        if not sep:
            # An omitted initialiser means "one past the previous member".
            expr = str(next_value)
        elif not re.fullmatch(r"[0-9+\-*/() ]+", expr.strip()):
            values.clear()
            break
        else:
            expr = expr.strip()
            next_value = arithexpr(expr, values) + 1
            values[name] = expr
            continue
        values[name] = expr
        next_value += 1
    return values


def resolve_all(raw: dict[str, str]) -> tuple[dict[str, int], list[str]]:
    """Resolve a constant block, honouring references to earlier constants.

    Both files define their layout as a forward-referencing block, so the values
    are folded repeatedly until nothing new resolves. A name that never resolves
    is a genuine cycle or typo and is reported by name.
    """

    env: dict[str, int] = {}
    pending = dict(raw)
    while pending:
        progressed = False
        for name in sorted(pending):
            try:
                env[name] = evaluate(pending[name], env)
            except ValueError:
                continue
            del pending[name]
            progressed = True
        if not progressed:
            break
    return env, sorted(pending)


def main() -> int:
    if not C_UI.is_file() or not JAVA_UI.is_file():
        print("check_ui_parity: cannot find both layout files", file=sys.stderr)
        return 1

    c_raw = collect(C_UI, C_INT)
    # The log levels are an enum on the C side; they are part of the same
    # contract as the palette, because both files map them to colours.
    c_raw.update(collect_c_enums(C_UI))
    java_raw = collect(JAVA_UI, JAVA_INT)
    c_vals, c_stuck = resolve_all(c_raw)
    java_vals, java_stuck = resolve_all(java_raw)

    problems: list[str] = []

    if c_stuck:
        problems.append(f"unresolvable C constants: {', '.join(c_stuck)}")
    if java_stuck:
        problems.append(
            f"unresolvable Android constants: {', '.join(java_stuck)}"
        )

    shared = (set(c_vals) & set(java_vals)) - ONE_SIDED
    for name in sorted(shared):
        if c_vals[name] != java_vals[name]:
            problems.append(
                f"{name}: C has {c_vals[name]:#x} ({c_raw[name].strip()}), "
                f"Android has {java_vals[name]:#x} ({java_raw[name].strip()})"
            )

    only_c = sorted((set(c_vals) - set(java_vals)) - ONE_SIDED)
    only_java = sorted((set(java_vals) - set(c_vals)) - ONE_SIDED)
    for name in only_c:
        problems.append(f"{name}: in the C layout but not the Android view")
    for name in only_java:
        problems.append(f"{name}: in the Android view but not the C layout")

    if problems:
        print("ui parity check FAILED:")
        for p in problems:
            print(f"  {p}")
        print(f"  ({len(shared)} shared constants compared)")
        return 1

    if not shared:
        print("ui parity check FAILED: no shared constants found")
        return 1

    print(
        f"ui parity passed: {len(shared)} layout/palette constants match "
        "between pit_patcher_ui.c and PatcherView.java"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
