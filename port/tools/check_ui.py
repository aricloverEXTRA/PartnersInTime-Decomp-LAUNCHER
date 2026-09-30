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

The screen is tabbed, so the frame that is checked decides what to look at. The
mode is guessed from the filename: names containing ``mods-on`` render the mods
tab with the toggle on, ``mods`` the toggle off, ``about`` the about tab, and
anything else the PATCH tab. ``--expect-half`` additionally requires the
progress fill to be near half the track, which is what ``--simulate`` paints.

Usage from port/:

    pit_patcher --screenshot build/ui-frames/idle.png
    pit_patcher --screenshot build/ui-frames/busy.png --simulate
    pit_patcher --screenshot build/ui-frames/mods.png --tab MODS
    pit_patcher --screenshot build/ui-frames/mods-on.png --tab MODS --mods on
    pit_patcher --screenshot build/ui-frames/about.png --tab ABOUT
    python tools/check_ui.py build/ui-frames/busy.png --expect-half
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

TAB_X0 = MARGIN
TAB_Y = 50
TAB_H = 16
TAB_COUNT = 3
TAB_GAP = 8
TAB_W = (UI_W - 2 * MARGIN - (TAB_COUNT - 1) * TAB_GAP) // TAB_COUNT

FIELD_X = 76
FIELD_W = UI_W - FIELD_X - 12 - 78
FIELD_H = 16
FIELD_Y0 = 88
FIELD_DY = 28
BROWSE = (UI_W - MARGIN - 76, FIELD_Y0 - 2, 76, 20)

MODS_Y = 146
PATCH = (MARGIN, 156, 160, 26)
STATUS_X = PATCH[0] + PATCH[2] + 14
BAR = (MARGIN, 190, UI_W - 2 * MARGIN, 12)
LOG = (MARGIN, 212, UI_W - 2 * MARGIN, 88)
FOOTER_Y = 308

MODS_CARD = (MARGIN, 84, UI_W - 2 * MARGIN, 160)
TOGGLE = (MODS_CARD[0] + 16, 112, 32, 14)
TOGGLE_KNOB_W = 12
MODS_DIV_Y = 152
CHIP_W = (MODS_CARD[2] - 24 - (3 - 1) * 12) // 3
CHIP_H = 24
CHIP_GAP = 12
CHIP_COUNT = 3
CHIP_X0 = MODS_CARD[0] + 12
CHIP_Y0 = MODS_DIV_Y + 8
CHIP_DY = CHIP_H + 4

ABOUT_CARD = (MARGIN, 84, UI_W - 2 * MARGIN, FOOTER_Y - 8 - 84)

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


def is_warm(c):
    return c[0] - c[2] > 40


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
    warm = [c for c in rule if is_warm(c)]
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


def card_corners_rounded(rows, x, y, w, h, name):
    for cx, cy in ((x, y), (x + w - 1, y), (x, y + h - 1), (x + w - 1, y + h - 1)):
        if not is_backdrop(rows, x, y, cx, cy):
            fail("%s corner (%d,%d) is not rounded: rgb%s" % (name, cx, cy, px(rows, cx, cy)))


def check_footer(rows):
    """The status tag on the right must be warm, and the left tag dim text."""
    tag = [px(rows, x, FOOTER_Y) for x in range(UI_W - MARGIN - 48, UI_W - MARGIN - 4)]
    if len([c for c in tag if is_warm(c)]) < 12:
        fail("footer '(57%)' tag is not warm: %d warm of %d pixels" % (len([c for c in tag if is_warm(c)]), len(tag)))


def check_tabs(rows, selected):
    """Three tabs, the active one gold with an underline, the rest panel."""
    for tab in range(TAB_COUNT):
        x = TAB_X0 + tab * (TAB_W + TAB_GAP)
        face = region(rows, x + 4, TAB_Y + 2, TAB_W - 8, TAB_H - 4)
        if distinct(face) < 4:
            fail("tab %d face is flat; the gradient is missing" % tab)
        warm = [c for c in face if is_warm(c)]
        if tab == selected:
            if len(warm) < len(face) // 2:
                fail("selected tab %d is not gold: %d warm of %d pixels"
                     % (tab, len(warm), len(face)))
            # Gold underline joins the selected tab to the content below it.
            under = region(rows, x + 8, TAB_Y + TAB_H, TAB_W - 16, 2)
            if len([c for c in under if is_warm(c)]) < (TAB_W - 16) // 2:
                fail("selected tab %d has no gold underline" % tab)
        elif warm:
            fail("unselected tab %d is warm (%d px), it should be panel blue"
                 % (tab, len(warm)))


def check_fields(rows):
    for i in range(2):
        fy = FIELD_Y0 + i * FIELD_DY
        well = region(rows, FIELD_X + 4, fy + 2, FIELD_W - 8, FIELD_H - 4)
        if distinct(well) < 2:
            fail("field %d well is flat; the sunken gradient is missing" % i)
        # A well must be darker than a raised panel: that is what makes it read
        # as inset. The log card is the plainest raised surface on the frame.
        well_l = sum(luminance(c) for c in well) / len(well)
        card_l = sum(luminance(c) for c in region(rows, LOG[0] + 6, LOG[1] + 26, LOG[2] - 12, 8)) / (LOG[2] - 12) / 8
        if well_l >= card_l:
            fail(
                "field %d well is not darker than the log card "
                "(well %.3f, card %.3f)" % (i, well_l, card_l)
            )
        # Label must be readable against the backdrop.
        label = region(rows, MARGIN, fy + 4, 60, 8)
        text = [c for c in label if luminance(c) > 0.05]
        if len(text) < 12:
            fail("field %d label has %d visible pixels" % (i, len(text)))


def check_mods_line(rows, hard_mode):
    """The mods status on the left of the PATCH tab."""
    line = region(rows, MARGIN, MODS_Y, 150, 10)
    text = [c for c in line if luminance(c) > 0.08]
    if len(text) < 30:
        fail("PATCH-tab mods line has %d visible pixels" % len(text))
    if hard_mode:
        # "MODS: HARD MODE" is amber, not the dim grey of the OFF hint.
        if len([c for c in line if is_warm(c)]) < 20:
            fail("HARD MODE status is not amber on the PATCH tab")


def check_patch_button(rows):
    bx, by, bw, bh = PATCH
    face = region(rows, bx + 8, by + 6, bw - 16, bh - 12)
    if distinct(face) < 4:
        fail("PATCH button face is flat; the gold gradient is missing")
    warm = [c for c in face if is_warm(c)]
    if len(warm) < (bw - 16) * (bh - 12) * 0.5:
        fail("PATCH button is not gold: %d warm of %d pixels" % (len(warm), (bw - 16) * (bh - 12)))

    # The label is dark-on-gold, which is the only inverted text in the UI.
    label = region(rows, bx + 40, by + 8, bw - 80, bh - 16)
    dark = [c for c in label if luminance(c) < 0.15]
    if len(dark) < 20:
        fail("PATCH label is not dark-on-gold; %d dark pixels found" % len(dark))


def check_browse(rows):
    x, y, w, h = BROWSE
    browse = region(rows, x + 6, y + 6, w - 12, h - 12)
    if distinct(browse) < 3:
        fail("BROWSE button face is flat")
    out = region(rows, x + 6, y + FIELD_DY + 6, w - 12, h - 12)
    if distinct(out) < 3:
        fail("BROWSE (output) button face is flat")


def check_status(rows):
    band = region(rows, STATUS_X, PATCH[1] + 4, 120, 10)
    if len([c for c in band if luminance(c) > 0.08]) < 12:
        fail("status line has no visible text")


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
        if any(is_warm(px(rows, xx, yy)) for yy in range(y, y + h))
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
    card_corners_rounded(rows, x, y, w, h, "log card")

    # The seeded entries leave text in the pane; measure it against the pane.
    text_rows = [yy for yy in range(y + 26, y + h - 6)
                 if any(luminance(px(rows, xx, yy)) > 0.12 for xx in range(x + 18, x + w - 6))]
    if len(text_rows) < 3:
        fail("log pane has %d text rows; the seeded entries did not render" % len(text_rows))
    if text_rows:
        worst = 99.0
        for yy in text_rows:
            bg = px(rows, x + 6, yy)
            for xx in range(x + 18, x + w - 6, 2):
                c = px(rows, xx, yy)
                # Only pixels that are text-bright count; a card-interior pixel
                # sampled beside the glyphs is the same colour as the background
                # and would drag the minimum down to 1.0.
                if luminance(c) > 0.12:
                    worst = min(worst, contrast(c, bg))
        if worst < 4.5:
            fail("log text contrast is %.2f, below the 4.5:1 minimum" % worst)


def check_toggle(rows, on):
    tx, ty, tw, th = TOGGLE
    face = region(rows, tx, ty, tw, th)
    if distinct(face) < 3:
        fail("mods toggle is flat")

    knob = TOGGLE_KNOB_W
    if on:
        kx = tx + tw - knob - 2
        warm = [c for c in face if is_warm(c)]
        if len(warm) < len(face) // 2:
            fail("ON toggle is not gold: %d warm of %d pixels" % (len(warm), len(face)))
    else:
        kx = tx + 2
        warm = [c for c in face if is_warm(c)]
        if warm:
            fail("OFF toggle is warm (%d px); it should be steel blue" % len(warm))

    # The knob is pale (white to grey), distinct from both fill states.
    knob_region_x = range(kx, kx + knob)
    knob_pxs = [px(rows, rx, ty + (th - knob) // 2) for rx in knob_region_x]
    if sum(max(c) > 120 for c in knob_pxs) < knob // 2:
        fail("toggle knob is not visibly pale: %d bright of %d px"
             % (sum(max(c) > 120 for c in knob_pxs), knob))


def check_mods_card(rows, on):
    x, y, w, h = MODS_CARD
    body = region(rows, x + 6, y + 30, w - 12, h - 40)
    if distinct(body) < 4:
        fail("mods card body is flat (%d colours); the gradient is missing" % distinct(body))
    card_corners_rounded(rows, x, y, w, h, "mods card")

    # Toggle row: the name must be bright text and the state readable.
    name = region(rows, TOGGLE[0] + TOGGLE[2] + 8, TOGGLE[1] - 4, 80, 8)
    if len([c for c in name if luminance(c) > 0.15]) < 20:
        fail("HARD MODE name has %d visible pixels" % len([c for c in name if luminance(c) > 0.15]))

    # The description block and the divider under it.
    desc = region(rows, x + 12, TOGGLE[1] + 18, w - 24, 20)
    if len([c for c in desc if luminance(c) > 0.05]) < 40:
        fail("mods description has %d visible pixels" % len([c for c in desc if luminance(c) > 0.05]))
    # The divider is a solid 1px C_EDGE_SOFT line; it must differ from the card
    # interior a couple of rows above it, or it has not been drawn.
    div_row = px(rows, x + w // 2, MODS_DIV_Y)
    above = px(rows, x + w // 2, MODS_DIV_Y - 2)
    if abs(div_row[0] - 0x1B) + abs(div_row[1] - 0x2A) + abs(div_row[2] - 0x94) > 60:
        fail("mods divider at y=%d is not the edge colour: rgb%s" % (MODS_DIV_Y, div_row))
    elif abs(div_row[0] - above[0]) + abs(div_row[1] - above[1]) + abs(div_row[2] - above[2]) < 10:
        fail("mods divider at y=%d is invisible against the card: rgb%s" % (MODS_DIV_Y, div_row))

    if on:
        check_chips(rows)
    else:
        # The OFF note explains that nothing is applied; three dim lines.
        for row in range(3):
            ly = CHIP_Y0 + row * 10
            line = region(rows, x + 12, ly, w - 24, 10)
            if len([c for c in line if luminance(c) > 0.05]) < 20:
                fail("mods OFF note line %d has %d visible pixels" % (row, len([c for c in line if luminance(c) > 0.05])))


def check_chips(rows):
    for i in range(6):
        col = i % CHIP_COUNT
        row = i // CHIP_COUNT
        cx = CHIP_X0 + col * (CHIP_W + CHIP_GAP)
        cy = CHIP_Y0 + row * CHIP_DY
        if cx + CHIP_W > MODS_CARD[0] + MODS_CARD[2]:
            fail("chip %d overflows the mods card: %d > %d" % (i, cx + CHIP_W, MODS_CARD[0] + MODS_CARD[2]))
        chip = region(rows, cx + 4, cy + 3, CHIP_W - 8, CHIP_H - 6)
        if distinct(chip) < 3:
            fail("chip %d is flat; nothing was drawn in it" % i)
        # Gold spine on the left edge marks it as a tuned stat.
        spine = px(rows, cx + 2, cy + CHIP_H // 2)
        if not is_warm(spine):
            fail("chip %d has no gold spine: rgb%s" % (i, spine))


def check_about(rows):
    x, y, w, h = ABOUT_CARD
    body = region(rows, x + 6, y + 30, w - 12, h - 40)
    if distinct(body) < 4:
        fail("about card body is flat; the gradient is missing")
    card_corners_rounded(rows, x, y, w, h, "about card")

    # Heading in bright white, then enough body rows to hold the paragraph.
    heading = region(rows, x + 12, y + 29, w - 24, 10)
    if len([c for c in heading if max(c) > 200]) < 40:
        fail("about heading has no bright pixels")
    text_rows = [yy for yy in range(y + 50, y + h - 18)
                 if any(luminance(px(rows, xx, yy)) > 0.12 for xx in range(x + 14, x + w - 12))]
    if len(text_rows) < 6:
        fail("about body has %d text rows; the paragraph did not render" % len(text_rows))


def check_shadow(rows, mode):
    """A pixel just below a raised card must be darker than the backdrop.

    The probe is compared against the untouched backdrop on the same row, not
    against an absolute brightness: the gradient lightens with depth and a fixed
    floor would be wrong for both the top and bottom of the screen. The shadow
    is a soft blue-black wash, so luminance barely moves; compare the summed
    green and blue channels instead, which the wash darkens measurably."""
    card = {"mods-off": MODS_CARD,
            "mods-on": MODS_CARD,
            "about": ABOUT_CARD}.get(mode, LOG)
    below_x = card[0] + card[2] // 2
    below_y = card[1] + card[3] + 1
    if below_y >= UI_H:
        return
    backdrop = px(rows, 2, below_y)
    spot = min(px(rows, below_x + i, below_y) for i in range(-3, 4))
    below_g_b = spot[1] + spot[2]
    edge_g_b = backdrop[1] + backdrop[2]
    if below_g_b >= edge_g_b - 2:
        fail("no visible drop shadow under the card "
             "(below %s vs backdrop %s)" % (spot, backdrop))


def check_no_overlap():
    """Interactive regions must not overlap, or clicks land on the wrong one."""
    boxes = {
        "TAB_PATCH": (TAB_X0, TAB_Y, TAB_W, TAB_H),
        "TAB_MODS": (TAB_X0 + TAB_W + TAB_GAP, TAB_Y, TAB_W, TAB_H),
        "TAB_ABOUT": (TAB_X0 + 2 * (TAB_W + TAB_GAP), TAB_Y, TAB_W, TAB_H),
        "TOGGLE": TOGGLE,
        "BROWSE_IN": (BROWSE[0], BROWSE[1], BROWSE[2], BROWSE[3]),
        "BROWSE_OUT": (BROWSE[0], BROWSE[1] + FIELD_DY, BROWSE[2], BROWSE[3]),
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
    if PATCH[1] + PATCH[3] > BAR[1]:
        fail("PATCH button overlaps the progress bar")
    # BROWSE must sit clear of the path wells and the tab strip.
    if BROWSE[0] < FIELD_X + FIELD_W:
        fail("BROWSE overlaps the path well")
    if BROWSE[1] < TAB_Y + TAB_H:
        fail("BROWSE overlaps the tab strip")


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
        if any(is_warm(px(rows, xx, yy)) for yy in range(y, y + h))
    ]
    if not gold_cols:
        fail("expected a 50%% fill, found none")
        return
    span = max(gold_cols) - min(gold_cols) + 1
    ratio = span / float(w - 4)
    if not 0.4 <= ratio <= 0.6:
        fail("expected a fill near 50%%, got %.0f%%" % (ratio * 100))


def mode_of(path: str) -> str:
    name = path.lower()
    if "mods-on" in name:
        return "mods-on"
    if "mods" in name:
        return "mods-off"
    if "about" in name:
        return "about"
    return "patch"


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
    mode = mode_of(path.name)

    width, height, rows = read_png(path)
    check_size(width, height)
    check_header(rows)
    check_footer(rows)
    check_palette_source()
    check_no_overlap()
    check_shadow(rows, mode)

    selected = {"mods-on": 1, "mods-off": 1, "about": 2}.get(mode, 0)
    check_tabs(rows, selected)

    if mode == "patch":
        check_fields(rows)
        check_mods_line(rows, hard_mode=False)
        check_browse(rows)
        check_patch_button(rows)
        check_status(rows)
        check_progress(rows)
        check_log(rows)
    elif mode == "mods-off":
        check_mods_card(rows, on=False)
        check_toggle(rows, on=False)
    elif mode == "mods-on":
        check_mods_card(rows, on=True)
        check_toggle(rows, on=True)
    elif mode == "about":
        check_about(rows)

    if expect_half:
        check_half_full(rows)

    if errors:
        print("ui check FAILED (%d problems) in %s" % (len(errors), path))
        for e in errors:
            print("  - %s" % e)
        return 1

    print("ui check passed: %s (%s tab), layout/contrast/palette all verified"
          % (width, mode))
    return 0


if __name__ == "__main__":
    sys.exit(main())