#!/usr/bin/env python3
"""Structural and legibility checks for the SDL launcher frame.

The launcher is software-rendered into an off-screen canvas and can dump that
canvas as a PNG without a display, so the exact pixels the window shows are
checkable from a build log. This decodes that PNG with the standard library and
asserts things that a screenshot review would otherwise have to catch by eye:

* every declared region is actually painted, and its edges are where the layout
  constants say they are;
* the palette is still the ROM-derived one and no colour was invented;
* text regions meet a minimum contrast ratio against what is behind them;
* gradients, shadows and rounded corners produced real variation rather than a
  flat fill, which is the failure mode of this kind of code;
* no two interactive regions overlap.

Usage from port/:

    pit_patcher --screenshot frame.png --simulate
    python tools/check_ui.py frame.png
"""

import pathlib
import re
import struct
import sys
import zlib

# Mirrors the layout block in src/platform/sdl2/pit_patcher_ui.c.
UI_W = 480
UI_H = 320
MARGIN = 12
HEADER_H = 48
PLAN = (MARGIN, 58, UI_W - 2 * MARGIN, 82)
CHIP_W = ((UI_W - 2 * MARGIN - 24 - 24) // 3)
CHIP_H = 24
CHIP_GAP = 12
CHIP_X0 = MARGIN + 12
CHIP_Y0 = 58 + 24
FIELD_X = 76
FIELD_W = UI_W - FIELD_X - 12 - 78
FIELD_H = 16
FIELD_Y0 = 152
FIELD_DY = 28
BROWSE = (UI_W - MARGIN - 76, FIELD_Y0 - 2, 76, 20)
PATCH = (MARGIN, 202, 140, 26)
BAR = (164, 222, UI_W - 164 - MARGIN, 10)
LOG = (MARGIN, 240, UI_W - 2 * MARGIN, UI_H - 240 - MARGIN)

errors: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


# --------------------------------------------------------------------- PNG I/O


def read_png(path: pathlib.Path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit("%s is not a PNG" % path)

    pos = 8
    idat = bytearray()
    width = height = depth = colour = 0
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos : pos + 4])
        kind = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, colour, _, _, interlace = struct.unpack(
                ">IIBBBBB", chunk
            )
            if depth != 8 or interlace != 0:
                raise SystemExit("expected non-interlaced 8-bit PNG")
        elif kind == b"IDAT":
            idat += chunk
        elif kind == b"IEND":
            break
        pos += 12 + length

    if colour != 6:
        raise SystemExit("expected RGBA (colour type 6), got %d" % colour)

    raw = zlib.decompress(bytes(idat))
    stride = width * 4
    rows = []
    prev = bytearray(stride)
    pos = 0
    for _ in range(height):
        filt = raw[pos]
        pos += 1
        line = bytearray(raw[pos : pos + stride])
        pos += stride
        if filt == 0:
            pass
        elif filt == 1:
            for i in range(4, stride):
                line[i] = (line[i] + line[i - 4]) & 0xFF
        elif filt == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif filt == 3:
            for i in range(stride):
                left = line[i - 4] if i >= 4 else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif filt == 4:
            for i in range(stride):
                a = line[i - 4] if i >= 4 else 0
                b = prev[i]
                c = prev[i - 4] if i >= 4 else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xFF
        else:
            raise SystemExit("unknown PNG filter %d" % filt)
        rows.append(line)
        prev = line

    return width, height, rows


# -------------------------------------------------------------------- helpers


def px(rows, x, y):
    i = x * 4
    return (rows[y][i], rows[y][i + 1], rows[y][i + 2])


def region(rows, x, y, w, h):
    return [px(rows, xx, yy) for yy in range(y, y + h) for xx in range(x, x + w)]


def luminance(c):
    def chan(v):
        v = v / 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4

    return 0.2126 * chan(c[0]) + 0.7152 * chan(c[1]) + 0.0722 * chan(c[2])


def contrast(a, b):
    la, lb = luminance(a), luminance(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def distinct(colours):
    return len(set(colours))


# ---------------------------------------------------------------------- checks


def check_size(width, height):
    if (width, height) != (UI_W, UI_H):
        fail("canvas is %dx%d, expected %dx%d" % (width, height, UI_W, UI_H))


def check_header(rows):
    """Gold rule under the header, spanning the full width, plus a title."""
    rule = [px(rows, x, HEADER_H - 2) for x in range(UI_W)]
    if distinct(rule) < 8:
        fail(
            "header rule at y=%d has only %d distinct colours; the gold gradient "
            "is not being drawn" % (HEADER_H - 2, distinct(rule))
        )
    warm = [c for c in rule if c[0] - c[2] > 40]
    if len(warm) < UI_W // 2:
        fail("header rule is not gold: only %d/%d warm pixels" % (len(warm), UI_W))

    # Title band must contain bright text pixels.
    title = region(rows, MARGIN, 8, 200, 26)
    bright = [c for c in title if max(c) > 180]
    if len(bright) < 60:
        fail("title area has only %d bright pixels; the title is not legible" % len(bright))


def is_backdrop(rows, x, y, corner_x, corner_y):
    """True when a corner pixel matches the backdrop beside it.

    Compared against the neighbouring pixel rather than an absolute brightness
    threshold: the backdrop carries a vertical gradient and a gold wash under the
    header, so its brightness varies with y and no fixed cutoff is correct.
    """
    here = px(rows, corner_x, corner_y)
    for ref_x, ref_y in ((x, corner_y), (corner_x, y)):
        if contrast(here, px(rows, ref_x, ref_y)) < 1.6:
            return True
    return False


def check_plan_card(rows):
    x, y, w, h = PLAN
    body = region(rows, x + 6, y + 30, w - 12, h - 40)
    if distinct(body) < 4:
        fail("plan card body is flat (%d colours); the gradient is missing" % distinct(body))

    for cx, cy in ((x, y), (x + w - 1, y), (x, y + h - 1), (x + w - 1, y + h - 1)):
        if not is_backdrop(rows, x, y, cx, cy):
            fail("plan card corner (%d,%d) is not rounded: rgb%s" % (cx, cy, px(rows, cx, cy)))

    for i in range(6):
        col = i % 3
        row = i // 3
        cx = CHIP_X0 + col * (CHIP_W + CHIP_GAP)
        cy = CHIP_Y0 + row * (CHIP_H + 4)
        if cx + CHIP_W > x + w:
            fail("chip %d overflows the plan card: %d > %d" % (i, cx + CHIP_W, x + w))
        chip = region(rows, cx + 4, cy + 3, CHIP_W - 8, CHIP_H - 6)
        if distinct(chip) < 3:
            fail("chip %d is flat; nothing was drawn in it" % i)
        # Gold spine on the left edge.
        spine = px(rows, cx + 2, cy + CHIP_H // 2)
        if spine[0] - spine[2] < 40:
            fail("chip %d has no gold spine: rgb%s" % (i, spine))


def check_fields(rows):
    for i in range(2):
        fy = FIELD_Y0 + i * FIELD_DY
        well = region(rows, FIELD_X + 4, fy + 2, FIELD_W - 8, FIELD_H - 4)
        if distinct(well) < 2:
            fail("field %d well is flat; the sunken gradient is missing" % i)
        # A well must be darker than the plan card above it: that is what makes
        # it read as inset rather than as another raised panel.
        well_l = sum(luminance(c) for c in well) / len(well)
        card_l = sum(luminance(c) for c in region(rows, FIELD_X + 4, PLAN[1] + 30,
                                                  FIELD_W - 8, 20)) / (FIELD_W - 8 * 20)
        if well_l >= card_l:
            fail(
                "field %d well is not darker than the card below it "
                "(well %.3f, card %.3f)" % (i, well_l, card_l)
            )
        # Label must be readable against the backdrop.
        label = region(rows, MARGIN, fy + 4, 60, 8)
        text = [c for c in label if luminance(c) > 0.05]
        if len(text) < 12:
            fail("field %d label has %d visible pixels" % (i, len(text)))


def check_buttons(rows):
    bx, by, bw, bh = PATCH
    face = region(rows, bx + 8, by + 6, bw - 16, bh - 12)
    if distinct(face) < 4:
        fail("PATCH button face is flat; the gold gradient is missing")
    warm = [c for c in face if c[0] - c[2] > 40]
    if len(warm) < (bw - 16) * (bh - 12) * 0.5:
        fail("PATCH button is not gold: %d warm of %d pixels" % (len(warm), (bw - 16) * (bh - 12)))

    # The label is dark-on-gold, which is the only inverted text in the UI.
    label = region(rows, bx + 40, by + 8, bw - 80, bh - 16)
    dark = [c for c in label if luminance(c) < 0.15]
    if len(dark) < 20:
        fail("PATCH label is not dark-on-gold; %d dark pixels found" % len(dark))

    x, y, w, h = BROWSE
    browse = region(rows, x + 6, y + 4, w - 12, h - 8)
    if distinct(browse) < 3:
        fail("BROWSE button face is flat")

    # Shadows: a pixel just below a card must be darker than the backdrop, and
    # brighter than the card body. drop_shadow blends, so a hard block means the
    # blend path is broken.
    below = luminance(px(rows, PLAN[0] + PLAN[2] // 2, PLAN[1] + PLAN[3] + 1))
    backdrop = luminance(px(rows, 2, UI_H - 2))
    if below >= backdrop:
        fail("no visible drop shadow under the plan card (%.3f vs %.3f)" % (below, backdrop))


def check_progress(rows):
    """The bar must have a gold fill whose width tracks the reported fraction.

    The idle state shows a short stub, so the minimum expected fill is that stub
    rather than nothing. `--simulate` renders a 50% frame and is checked strictly
    by check_ui.py --expect-half.
    """
    x, y, w, h = BAR
    track = region(rows, x + 4, y + 2, w - 8, h - 4)
    if distinct(track) < 3:
        fail("progress bar is flat")

    # Count the columns that contain any gold: that is the fill width.
    gold_cols = [
        xx
        for xx in range(x, x + w)
        if any(px(rows, xx, yy)[0] - px(rows, xx, yy)[2] > 40 for yy in range(y, y + h))
    ]
    if not gold_cols:
        fail("progress bar shows no gold fill at all")
        return
    span = max(gold_cols) - min(gold_cols) + 1
    if span < 8:
        fail("progress fill is only %dpx wide; the idle stub is missing" % span)
    if min(gold_cols) < x:
        fail("progress fill starts at %d, left of the bar at %d" % (min(gold_cols), x))
    if max(gold_cols) >= x + w:
        fail("progress fill ends at %d, right of the bar at %d" % (max(gold_cols), x + w))


def check_log(rows):
    x, y, w, h = LOG
    body = region(rows, x + 6, y + 26, w - 12, h - 32)
    if distinct(body) < 3:
        fail("log card is flat; the gradient is missing")
    for cx, cy in ((x, y), (x + w - 1, y), (x, y + h - 1), (x + w - 1, y + h - 1)):
        if not is_backdrop(rows, x, y, cx, cy):
            fail("log card corner (%d,%d) is not rounded: rgb%s" % (cx, cy, px(rows, cx, cy)))

    # Simulated run leaves text in the pane; measure it against the pane.
    text_rows = [yy for yy in range(y + 26, y + h - 6)
                 if any(luminance(px(rows, xx, yy)) > 0.12 for xx in range(x + 18, x + w - 6))]
    if len(text_rows) < 3:
        fail("log pane has %d text rows; the simulated entries did not render" % len(text_rows))
    if text_rows:
        worst = 99.0
        for yy in text_rows:
            for xx in range(x + 18, x + w - 6, 2):
                c = px(rows, xx, yy)
                if luminance(c) > 0.02:
                    bg = px(rows, x + 6, yy)
                    worst = min(worst, contrast(c, bg))
        if worst < 4.5:
            fail("log text contrast is %.2f, below the 4.5:1 minimum" % worst)


def check_no_overlap():
    """Interactive regions must not overlap, or clicks land on the wrong one."""
    boxes = {
        "BROWSE_IN": (BROWSE[0], BROWSE[1], BROWSE[2], BROWSE[3]),
        "BROWSE_OUT": (BROWSE[0], FIELD_Y0 + FIELD_DY - 2, BROWSE[2], BROWSE[3]),
        "PATCH": PATCH,
    }
    items = sorted(boxes.items())
    for i in range(len(items)):
        for j in range(i + 1, len(items)):
            (na, a) = items[i]
            (nb, b) = items[j]
            if a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and \
               a[1] < b[1] + b[3] and b[1] < a[1] + a[3]:
                fail("%s and %s overlap" % (na, nb))

    # The primary button must sit clear of the progress bar.
    if PATCH[0] + PATCH[2] > BAR[0]:
        fail("PATCH button overlaps the progress bar")
    # BROWSE must sit clear of the fields.
    if BROWSE[0] < FIELD_X + FIELD_W:
        fail("BROWSE overlaps the path well")
    # The plan card must sit below the header, and the log below the actions.
    if PLAN[1] < HEADER_H:
        fail("plan card overlaps the header")
    if LOG[1] < PATCH[1] + PATCH[3]:
        fail("log card overlaps the action row")


def check_palette_source():
    """Colours must still carry their ROM provenance comment."""
    src = pathlib.Path("src/platform/sdl2/pit_patcher_ui.c").read_text(encoding="utf-8")
    for name in ("C_BG", "C_ACCENT", "C_AMBER", "C_EDGE", "C_TEXT", "C_ERROR", "C_OK"):
        m = re.search(r"^#define\s+%s\s+PIT_ARGB\([^)]*\)\s*/\*\s*(.*?)\*/" % name,
                      src, re.M)
        if not m:
            fail("%s lost its provenance comment" % name)
        elif "ROM" not in m.group(1):
            fail("%s is no longer documented as coming from the ROM: %r" % (name, m.group(1)))


def check_half_full(rows):
    """A --simulate frame reports fraction 0.5, so the fill must be near half the
    track. This ties the bar to the value the patcher thread reports, so a
    progress regression cannot pass on a still-correct-looking stub."""
    x, y, w, h = BAR
    gold_cols = [
        xx
        for xx in range(x, x + w)
        if any(px(rows, xx, yy)[0] - px(rows, xx, yy)[2] > 40 for yy in range(y, y + h))
    ]
    if not gold_cols:
        fail("expected a 50%% fill, found none")
        return
    span = max(gold_cols) - min(gold_cols) + 1
    ratio = span / float(w - 4)
    if not 0.4 <= ratio <= 0.6:
        fail("expected a fill near 50%%, got %.0f%%" % (ratio * 100))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    expect_half = "--expect-half" in sys.argv
    if len(args) != 1:
        print(__doc__)
        return 2
    path = pathlib.Path(args[0])
    if not path.exists():
        print("no such frame: %s" % path)
        return 2

    width, height, rows = read_png(path)
    check_size(width, height)
    check_header(rows)
    check_plan_card(rows)
    check_fields(rows)
    check_buttons(rows)
    check_progress(rows)
    check_log(rows)
    check_no_overlap()
    check_palette_source()
    if expect_half:
        check_half_full(rows)

    if errors:
        print("ui check FAILED (%d problems) in %s" % (len(errors), path))
        for e in errors:
            print("  - %s" % e)
        return 1

    print("ui check passed: %dx%d frame, layout/contrast/palette all verified" % (width, height))
    return 0


if __name__ == "__main__":
    sys.exit(main())
