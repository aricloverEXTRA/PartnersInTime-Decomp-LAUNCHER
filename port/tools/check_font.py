#!/usr/bin/env python3
"""Validate the patcher font and prove the generated tables match it.

Three things are checked:

1. Structural invariants on the source font. A 5x7 face in an 8x8 cell has ink
   only in columns 0-4, only in rows 0-6, and must not leave an empty row inside
   a glyph unless the glyph genuinely has no ink there. These catch the class of
   bug where a glyph is truncated or shifted, which reads as scrambled text
   rather than as an error.

2. The generated C table in pit_patch_data.h is byte-for-byte the source font.

3. The generated Java table in PatchData.java is byte-for-byte the same.

Exits non-zero on any failure. Run from the port/ directory:

    python tools/check_font.py
"""

import pathlib
import re
import sys

FONT_FIRST = 0x20
FONT_LAST = 0x7E
GLYPH_W = 8
GLYPH_H = 8
INK_COLS = 5
INK_ROWS = 7

# Glyphs whose whole 7-row body is blank by design.
BLANK = {" "}

# Punctuation that is legitimately only one or two rows of ink. Anything else
# with less than three rows of ink is a stub.
SHORT_MARKS = set("\"',-._`:;")

# Lowercase letter classes. The cell is seven rows of ink, so every letter rests
# on the last row; what differs is where it starts:
#   ascender   row 0    (b d f h k l t)
#   x-height   row 2    (a c e m n o r s u v w x z)
#   descender  row 1    (g j p q y) - they give up a row at the top to fit the
#                       tail, since the cell has no room below the baseline
LOWER_ASCENDER = set("bdfhkl")
LOWER_XHEIGHT = set("acemnorsuvwxz")
LOWER_DESCENDER = set("gjpqy")


def parse_source(path):
    glyphs = {}
    for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        code = int(parts[0], 10)
        rows = parts[1:]
        if len(rows) != GLYPH_H:
            raise SystemExit(
                "%s:%d: expected %d rows, got %d" % (path, lineno, GLYPH_H, len(rows))
            )
        padded = []
        for r, row in enumerate(rows):
            if set(row) - {".", "#"}:
                raise SystemExit("%s:%d: bad cells in %r" % (path, lineno, row))
            if len(row) > GLYPH_W:
                raise SystemExit(
                    "%s:%d: row %r is %d cells, max %d"
                    % (path, lineno, row, len(row), GLYPH_W)
                )
            padded.append(row.ljust(GLYPH_W, "."))
        glyphs[code] = padded
    return glyphs


def row_bits(row):
    """Pack a row with bit 7 as the leftmost pixel, matching the generator."""
    value = 0
    for col, cell in enumerate(row):
        if cell == "#":
            value |= 1 << (GLYPH_W - 1 - col)
    return value


def bits_to_row(bits):
    return "".join("#" if bits & (1 << (7 - c)) else "." for c in range(GLYPH_W))


def check_structure(glyphs, errors):
    expected = list(range(FONT_FIRST, FONT_LAST + 1))
    for code in expected:
        if code not in glyphs:
            errors.append("missing glyph for 0x%02X %r" % (code, chr(code)))
    for code in sorted(set(glyphs) - set(expected)):
        errors.append("unexpected glyph 0x%02X %r" % (code, chr(code)))
    if errors:
        return

    for code in expected:
        rows = glyphs[code]
        ch = chr(code)
        label = "%r 0x%02X" % (ch, code)

        for r, row in enumerate(rows):
            if row[INK_COLS:].strip("."):
                errors.append(
                    "%s: row %d has ink in the inter-glyph gap: %r" % (label, r, row)
                )
        if rows[INK_ROWS].strip("."):
            errors.append("%s: baseline row %d is not blank: %r" % (label, INK_ROWS, rows[INK_ROWS]))

        ink_rows = [r for r in range(INK_ROWS) if rows[r].strip(".")]
        if ch in BLANK:
            if ink_rows:
                errors.append("%s: should be blank but has ink" % label)
            continue
        if not ink_rows:
            errors.append("%s: glyph is entirely blank" % label)
            continue

        if ch in SHORT_MARKS:
            continue

        # A glyph that uses fewer than three rows is almost certainly a stub.
        if ink_rows[-1] - ink_rows[0] < 2:
            errors.append(
                "%s: only %d rows of ink (%d..%d), glyph looks like a stub"
                % (label, ink_rows[-1] - ink_rows[0] + 1, ink_rows[0], ink_rows[-1])
            )

        if ch.isupper() or ch.isdigit():
            if ink_rows[0] != 0 or ink_rows[-1] != INK_ROWS - 1:
                errors.append(
                    "%s: uppercase/digit should span rows 0..%d, got %d..%d"
                    % (label, INK_ROWS - 1, ink_rows[0], ink_rows[-1])
                )
        elif ch in LOWER_ASCENDER:
            if ink_rows[0] != 0 or ink_rows[-1] != INK_ROWS - 1:
                errors.append(
                    "%s: ascender should span rows 0..%d, got %d..%d"
                    % (label, INK_ROWS - 1, ink_rows[0], ink_rows[-1])
                )
        elif ch in LOWER_XHEIGHT:
            if ink_rows[0] != 2 or ink_rows[-1] != INK_ROWS - 1:
                errors.append(
                    "%s: x-height letter should span rows 2..%d, got %d..%d"
                    % (label, INK_ROWS - 1, ink_rows[0], ink_rows[-1])
                )
        elif ch in LOWER_DESCENDER:
            if ink_rows[0] > 1 or ink_rows[-1] != INK_ROWS - 1:
                errors.append(
                    "%s: descender should end on row %d and start by row 1, got %d..%d"
                    % (label, INK_ROWS - 1, ink_rows[0], ink_rows[-1])
                )


def parse_header(path):
    text = path.read_text(encoding="utf-8")
    m = re.search(r"PIT_FONT8X8\[PIT_FONT_GLYPH_COUNT\]\[8\] = \{(.*?)\n\};", text, re.S)
    if not m:
        raise SystemExit("PIT_FONT8X8 table not found in %s" % path)
    table = []
    for entry in re.findall(r"\{([^{}]*?)\}", m.group(1), re.S):
        vals = [int(x, 0) for x in entry.split(",") if x.strip()]
        if len(vals) != GLYPH_H:
            raise SystemExit("header glyph has %d rows: %r" % (len(vals), entry))
        table.append(vals)
    return table


def parse_java(path):
    text = path.read_text(encoding="utf-8")
    m = re.search(r"FONT8X8 = \{(.*?)\n    \};", text, re.S)
    if not m:
        raise SystemExit("FONT8X8 table not found in %s" % path)
    rows = []
    for entry in re.findall(r"\{([^{}]*?)\}", m.group(1), re.S):
        vals = [int(x, 0) for x in entry.split(",") if x.strip()]
        if len(vals) != GLYPH_H:
            raise SystemExit("java glyph has %d rows: %r" % (len(vals), entry))
        rows.append(vals)
    return rows


def compare(label, glyphs, table, errors):
    if len(table) != len(glyphs):
        errors.append(
            "%s: %d glyphs, source has %d" % (label, len(table), len(glyphs))
        )
        return
    for code in sorted(glyphs):
        want = glyphs[code]
        got = [bits_to_row(b) for b in table[code - FONT_FIRST]]
        if want != got:
            errors.append(
                "%s: glyph %r 0x%02X differs from source\n  want %s\n  got  %s"
                % (label, chr(code), code,
                   " ".join(want[:INK_ROWS]),
                   " ".join(got[:INK_ROWS]))
            )


def main():
    errors: list[str] = []
    src = parse_source(pathlib.Path("tools/patch_sources/font8x8.txt"))

    check_structure(src, errors)

    compare("C header", src, parse_header(pathlib.Path("src/core/pit_patch_data.h")), errors)
    compare(
        "Java PatchData",
        src,
        parse_java(
            pathlib.Path(
                "android/app/src/main/java/com/partnersintime/patcher/PatchData.java"
            )
        ),
        errors,
    )

    if errors:
        print("font check FAILED (%d problems)" % len(errors))
        for e in errors:
            print("  - %s" % e)
        return 1

    print("font check passed: %d glyphs, structure valid, C and Java tables match"
          % len(src))

    # Print a specimen so a regression is visible in the log, not just a failure.
    for label, text in (("title", "PiT PATCHER"), ("lowercase", "hard mode 1.10x")):
        print("  %s:" % label)
        for r in range(INK_ROWS):
            print("    " + "".join(src[ord(c)][r] for c in text))
    return 0


if __name__ == "__main__":
    sys.exit(main())
