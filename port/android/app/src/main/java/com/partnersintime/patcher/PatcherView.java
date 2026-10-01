package com.partnersintime.patcher;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

/**
 * The patcher screen, drawn with the same 8-bit look as the Windows build.
 *
 * <p>Everything is painted by hand: a fixed 480x320 design grid is scaled to fit
 * the screen, and text is blitted from {@link PatchData#FONT8X8} as rectangles
 * rather than drawn with a system typeface. That keeps the two builds visually
 * identical and avoids shipping a font file, and it means the UI needs no
 * resources beyond its own string literals.
 *
 * <p>The layout, palette and drawing order mirror
 * {@code port/src/platform/sdl2/pit_patcher_ui.c} one to one. Every colour is a
 * value the C renderer also uses, taken from the real EUR ROM's 15-bit BGR555
 * palettes, so a frame here and a frame there are the same picture. When one
 * side's layout changes, the other has to change with it; the constants below are
 * named after the C macros deliberately.
 *
 * <p>The view is otherwise inert. It reports taps through {@link Callback} and
 * lets the activity own all state, so the same layout can be exercised in a
 * test without an Android runtime.
 *
 * <p>The screen is split into tabs, matching the C build: ROM (the core
 * flow), MODS (optional data mods, off by default) and ABOUT (what this tool is
 * for). Both touch and the D-pad / Tab keyboard work: Tab or the D-pad move the
 * focus around the visible tab, Enter/Space activate the focused control, and
 * Left/Right switch tabs when nothing is being edited.
 */
public final class PatcherView extends View {

    /** Design-space size, matching UI_W/UI_H in the C renderer. */
    public static final int UI_W = 480;
    public static final int UI_H = 320;

    private static final int FONT_W = 8;
    private static final int FONT_H = 8;
    private static final int LINE_H = 10;

    /* Layout, matching the macros of the same name in the C renderer. */
    private static final int MARGIN = 12;
    private static final int HEADER_H = 48;

    private static final int TAB_X0 = MARGIN;
    private static final int TAB_Y = 50;
    private static final int TAB_H = 16;
    public static final int TAB_COUNT = 3;
    private static final int TAB_GAP = 8;
    private static final int TAB_W =
            (UI_W - 2 * MARGIN - (TAB_COUNT - 1) * TAB_GAP) / TAB_COUNT;
    private static final int CONTENT_Y = 76;

    private static final int FIELD_X = 76;
    private static final int FIELD_W = UI_W - FIELD_X - 12 - 78;
    private static final int FIELD_H = 16;
    private static final int FIELD_Y0 = 88;
    private static final int FIELD_DY = 28;
    private static final int BROWSE_X = UI_W - MARGIN - 76;
    private static final int BROWSE_W = 76;
    private static final int BROWSE_H = 20;

    private static final int MODS_Y = 146;

    private static final int PATCH_X = MARGIN;
    private static final int PATCH_Y = 156;
    private static final int PATCH_W = 160;
    private static final int PATCH_H = 26;
    private static final int STATUS_X = PATCH_X + PATCH_W + 14;
    private static final int BAR_X = MARGIN;
    private static final int BAR_Y = 190;
    private static final int BAR_H = 12;
    private static final int BAR_W = UI_W - 2 * MARGIN;

    private static final int FOOTER_Y = 308;

    private static final int LOG_X = MARGIN;
    private static final int LOG_Y = 212;
    private static final int LOG_W = UI_W - 2 * MARGIN;
    private static final int LOG_H = FOOTER_Y - 8 - LOG_Y;

    private static final int MODS_CARD_X = MARGIN;
    private static final int MODS_CARD_Y = 84;
    private static final int MODS_CARD_W = UI_W - 2 * MARGIN;
    private static final int MODS_CARD_H = 160;
    private static final int MODS_HINT_Y = MODS_CARD_Y + MODS_CARD_H + 10;
    private static final int TOGGLE_X = MODS_CARD_X + 16;
    private static final int TOGGLE_Y = 112;
    private static final int TOGGLE_W = 32;
    private static final int TOGGLE_H = 14;
    private static final int TOGGLE_KNOB_W = 12;
    private static final int MODS_NAME_X = TOGGLE_X + TOGGLE_W + 8;
    private static final int MODS_NAME_Y = TOGGLE_Y - 4;
    private static final int MODS_DESC_Y = 130;
    private static final int MODS_DIV_Y = 152;

    private static final int CHIP_H = 24;
    private static final int CHIP_GAP = 12;
    private static final int CHIP_COUNT = 3;
    private static final int CHIP_W =
            (MODS_CARD_W - 24 - (CHIP_COUNT - 1) * CHIP_GAP) / CHIP_COUNT;
    private static final int CHIP_X0 = MODS_CARD_X + 12;
    private static final int CHIP_Y0 = MODS_DIV_Y + 8;
    private static final int CHIP_DY = CHIP_H + 4;

    private static final int ABOUT_CARD_X = MARGIN;
    private static final int ABOUT_CARD_Y = 84;
    private static final int ABOUT_CARD_W = UI_W - 2 * MARGIN;
    private static final int ABOUT_CARD_H = FOOTER_Y - 8 - ABOUT_CARD_Y;

    /* Palette, matching the C renderer. Every value comes from the real EUR
     * ROM's 15-bit BGR555 palettes, expanded with c * 255 / 31. See
     * port/src/platform/sdl2/pit_patcher_ui.c for the ROM offsets. */
    private static final int C_BG = 0xFF001029;
    private static final int C_BG_TOP = 0xFF000814;
    private static final int C_PANEL = 0xFF00106B;
    private static final int C_PANEL_HI = 0xFF001C94;
    private static final int C_PANEL_LO = 0xFF000B4A;
    private static final int C_WELL = 0xFF000933;
    private static final int C_EDGE = 0xFF52637B;
    private static final int C_EDGE_SOFT = 0xFF1B2A94;
    private static final int C_EDGE_HOT = 0xFFFFA521;
    private static final int C_TEXT = 0xFFC5DEF7;
    private static final int C_TEXT_HI = 0xFFFFFFFF;
    private static final int C_DIM = 0xFF738494;
    private static final int C_ACCENT = 0xFFDE9429;
    private static final int C_ACCENT_LO = 0xFF9A6310;
    private static final int C_AMBER = 0xFFFF9421;
    private static final int C_AMBER_HI = 0xFFFFD27A;
    private static final int C_ERROR = 0xFFEF5A63;
    private static final int C_OK = 0xFF52C55A;
    private static final int C_SHADOW = 0xA0000008;
    private static final int C_SHADOW_SOFT = 0x60000004;
    private static final int C_TRACK = 0xFF00061F;
    private static final int C_PILL_TEXT = 0xFF2A1802;

    /** Level colours, matching level_color() in the C renderer. */
    public static final int LOG_INFO = 0;
    public static final int LOG_OK = 1;
    public static final int LOG_WARN = 2;
    public static final int LOG_ERROR = 3;

    /* Tab identities, matching the tab_id enum in the C renderer. */
    public static final int TAB_PATCH = 0;
    public static final int TAB_MODS = 1;
    public static final int TAB_ABOUT = 2;

    /* Button identities, matching the button_id enum values in the C renderer. */
    private static final int BTN_NONE = 0;
    private static final int BTN_TAB_PATCH = 1;
    private static final int BTN_TAB_MODS = 2;
    private static final int BTN_TAB_ABOUT = 3;
    private static final int BTN_TOGGLE = 4;
    private static final int BTN_BROWSE_IN = 5;
    private static final int BTN_BROWSE_OUT = 6;
    private static final int BTN_PATCH = 7;

    /* Keyboard focus slots, matching the FOCUS_* constants in the C renderer. */
    public static final int FOCUS_NONE = 0;
    public static final int FOCUS_SRC = 1;
    public static final int FOCUS_BROWSE_IN = 2;
    public static final int FOCUS_OUT = 3;
    public static final int FOCUS_BROWSE_OUT = 4;
    public static final int FOCUS_PATCH = 5;
    public static final int FOCUS_TOGGLE = 6;

    /**
     * The idle stub the C renderer paints so a fresh window is not a dead
     * rectangle. fractionForRender() applies the same fallback, so the two
     * builds agree on what an untouched screen looks like.
     */
    private static final double IDLE_STUB = 0.04;

    private static final int LOG_MAX = 64;

    /** One log line: a palette colour and its message. */
    public static final class LogLine {
        public final int color;
        public final String message;

        LogLine(int color, String message) {
            this.color = color;
            this.message = message;
        }
    }

    /** Visual state the view renders; owned by the activity. */
    public static final class State {
        public String sourceName = "";
        public String outputName = "";
        public final List<LogLine> log = new ArrayList<LogLine>();
        public int step;
        public double fraction;
        public boolean busy;
        public boolean patched;
        public boolean failed;
        public int totalSteps = Patcher.STEPS;
        public int tab = TAB_PATCH;
        public boolean hardMode;
        public int focus = FOCUS_NONE;
    }

    public interface Callback {
        void onBrowseSource();

        void onBrowseOutput();

        void onPatch();

        void onTab(int tab);

        void onToggleMods();
    }

    private final Paint fill = new Paint();
    private final Paint stroke = new Paint();
    private final RectF scratch = new RectF();

    private final State state = new State();
    private Callback callback;

    private float scale = 1f;
    private float offsetX;
    private float offsetY;
    private int hovered = BTN_NONE;

    public PatcherView(Context context) {
        this(context, null);
    }

    public PatcherView(Context context, AttributeSet attrs) {
        super(context, attrs);
        fill.setStyle(Paint.Style.FILL);
        fill.setAntiAlias(false);
        stroke.setStyle(Paint.Style.STROKE);
        stroke.setAntiAlias(false);
        setClickable(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    public void setCallback(Callback callback) {
        this.callback = callback;
    }

    public State state() {
        return state;
    }

    public void addLog(int level, String message) {
        int color = levelColor(level);
        if (state.log.size() >= LOG_MAX) {
            state.log.remove(0);
        }
        state.log.add(new LogLine(color, message));
        invalidate();
    }

    public void clearLog() {
        state.log.clear();
        state.step = 0;
        state.fraction = 0;
        state.busy = false;
        state.patched = false;
        state.failed = false;
        invalidate();
    }

    public void setProgress(int step, double fraction) {
        state.step = step;
        state.fraction = fraction;
        invalidate();
    }

    public void setBusy(boolean busy) {
        state.busy = busy;
        invalidate();
    }

    public void setPatched(boolean patched) {
        state.patched = patched;
        state.failed = false;
        state.busy = false;
        state.fraction = 1.0;
        invalidate();
    }

    public void setFailed(boolean failed) {
        state.failed = failed;
        state.busy = false;
        invalidate();
    }

    public void setPaths(String source, String output) {
        state.sourceName = source;
        state.outputName = output;
        invalidate();
    }

    public void setTab(int tab) {
        state.tab = tab;
        state.focus = FOCUS_NONE;
        invalidate();
    }

    public void setHardMode(boolean hardMode) {
        state.hardMode = hardMode;
        invalidate();
    }

    /** Log colour for a level, matching level_color() in the C renderer. */
    public static int levelColor(int level) {
        switch (level) {
            case LOG_OK:
                return C_OK;
            case LOG_WARN:
                return C_AMBER;
            case LOG_ERROR:
                return C_ERROR;
            default:
                return C_TEXT;
        }
    }

    /* ------------------------------------------------------------- drawing */

    private void rect(Canvas canvas, int x, int y, int w, int h, int color) {
        fill.setColor(color);
        fill.setShader(null);
        canvas.drawRect(x, y, x + w, y + h, fill);
    }

    private void roundRect(Canvas canvas, int x, int y, int w, int h, int radius, int color) {
        fill.setColor(color);
        fill.setShader(null);
        scratch.set(x, y, x + w, y + h);
        canvas.drawRoundRect(scratch, radius, radius, fill);
    }

    /** Vertical gradient inside a rounded rect, matching fill_round_gradient. */
    private void roundGradient(Canvas canvas, int x, int y, int w, int h, int radius,
            int top, int bottom) {
        fill.setShader(new LinearGradient(x, y, x, y + h, top, bottom, Shader.TileMode.CLAMP));
        scratch.set(x, y, x + w, y + h);
        canvas.drawRoundRect(scratch, radius, radius, fill);
        fill.setShader(null);
    }

    private void roundOutline(Canvas canvas, int x, int y, int w, int h, int radius, int color) {
        stroke.setColor(color);
        stroke.setShader(null);
        scratch.set(x + 0.5f, y + 0.5f, x + w - 0.5f, y + h - 0.5f);
        canvas.drawRoundRect(scratch, radius, radius, stroke);
    }

    private static int lerp(int a, int b, int t) {
        int ar = (a >> 16) & 0xFF;
        int ag = (a >> 8) & 0xFF;
        int ab = a & 0xFF;
        int br = (b >> 16) & 0xFF;
        int bg = (b >> 8) & 0xFF;
        int bb = b & 0xFF;
        int r = ar + (br - ar) * t / 255;
        int g = ag + (bg - ag) * t / 255;
        int bl = ab + (bb - ab) * t / 255;
        return 0xFF000000 | (r << 16) | (g << 8) | bl;
    }

    private static int blendOnto(int dst, int src) {
        int sa = (src >>> 24) & 0xFF;
        if (sa == 0) {
            return dst;
        }
        if (sa == 255) {
            return src;
        }
        int sr = (src >> 16) & 0xFF;
        int sg = (src >> 8) & 0xFF;
        int sb = src & 0xFF;
        int dr = (dst >> 16) & 0xFF;
        int dg = (dst >> 8) & 0xFF;
        int db = dst & 0xFF;
        int r = sr * sa / 255 + dr * (255 - sa) / 255;
        int g = sg * sa / 255 + dg * (255 - sa) / 255;
        int b = sb * sa / 255 + db * (255 - sa) / 255;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    private void hline(Canvas canvas, int x, int y, int w, int color) {
        rect(canvas, x, y, w, 1, color);
    }

    /** Draws one 8x8 glyph; unknown characters fall back to a blank. */
    private void glyph(Canvas canvas, char ch, int x, int y, int color) {
        int index = ch - PatchData.FONT_FIRST;
        if (index < 0 || index >= PatchData.FONT_GLYPHS) {
            return;
        }
        fill.setColor(color);
        fill.setShader(null);
        int[] rows = PatchData.FONT8X8[index];

        for (int row = 0; row < FONT_H; row++) {
            int bits = rows[row];
            for (int col = 0; col < FONT_W; col++) {
                if ((bits & (0x80 >> col)) != 0) {
                    canvas.drawRect(x + col, y + row, x + col + 1, y + row + 1, fill);
                }
            }
        }
    }

    private int textWidth(String s) {
        return s.length() * FONT_W;
    }

    private void text(Canvas canvas, int x, int y, String s, int color) {
        for (int i = 0; i < s.length(); i++) {
            glyph(canvas, s.charAt(i), x + i * FONT_W, y, color);
        }
    }

    /** Text scaled by an integer factor, matching draw_text()'s scale argument. */
    private void textScaled(Canvas canvas, int x, int y, String s, int color, int factor) {
        for (int i = 0; i < s.length(); i++) {
            int index = s.charAt(i) - PatchData.FONT_FIRST;
            if (index < 0 || index >= PatchData.FONT_GLYPHS) {
                continue;
            }
            fill.setColor(color);
            fill.setShader(null);
            int[] rows = PatchData.FONT8X8[index];
            for (int row = 0; row < FONT_H; row++) {
                int bits = rows[row];
                for (int col = 0; col < FONT_W; col++) {
                    if ((bits & (0x80 >> col)) != 0) {
                        int gx = x + i * FONT_W * factor + col * factor;
                        int gy = y + row * factor;
                        canvas.drawRect(gx, gy, gx + factor, gy + factor, fill);
                    }
                }
            }
        }
    }

    private void textClipped(Canvas canvas, int x, int y, int limit, String s, int color) {
        int max = Math.max(0, (limit - x) / FONT_W);

        if (s.length() > max) {
            s = s.substring(0, Math.max(0, max - 1)) + ".";
        }
        text(canvas, x, y, s, color);
    }

    private void dropShadow(Canvas canvas, int x, int y, int w, int h, int radius) {
        roundRect(canvas, x + 2, y + 3, w, h, radius, C_SHADOW_SOFT);
        roundRect(canvas, x + 1, y + 2, w, h, radius, C_SHADOW);
    }

    private void drawCard(Canvas canvas, int x, int y, int w, int h, int radius) {
        dropShadow(canvas, x, y, w, h, radius);
        roundGradient(canvas, x, y, w, h, radius, C_PANEL, C_PANEL_LO);
        roundOutline(canvas, x, y, w, h, radius, C_EDGE_SOFT);
    }

    private void drawWell(Canvas canvas, int x, int y, int w, int h, boolean active) {
        roundRect(canvas, x, y, w, h, 2, C_WELL);
        hline(canvas, x, y, w, C_EDGE_SOFT);
        hline(canvas, x, y + h - 1, w, 0xFF000000);
        canvas.drawLine(x, y + h, x + w, y + h, stroke(0xFF00060F));
        canvas.drawLine(x + w, y, x + w, y + h, stroke(0xFF00060F));
        rect(canvas, x, y, 1, h, C_EDGE_SOFT);
        if (active) {
            roundOutline(canvas, x, y, w, h, 2, C_EDGE_HOT);
        }
    }

    private Paint stroke(int color) {
        stroke.setColor(color);
        stroke.setShader(null);
        return stroke;
    }

    /* ------------------------------------------------------------- chrome */

    private void drawBackdrop(Canvas canvas) {
        fill.setShader(new LinearGradient(0, 0, 0, UI_H, C_BG_TOP, C_BG, Shader.TileMode.CLAMP));
        canvas.drawRect(0, 0, UI_W, UI_H, fill);
        fill.setShader(null);

        /* Gold wash over the header, fading out downward. */
        for (int y = 0; y < HEADER_H + 20; y++) {
            int fall = HEADER_H + 20 - y;
            int alpha = Math.max(0, Math.min(26, 26 - fall)) + 8;
            int c = blendOnto(cornerColor(y), alpha << 24 | 0xDE9429);

            for (int x = 0; x < UI_W; x += 64) {
                rect(canvas, x, y, Math.min(64, UI_W - x), 1, c);
            }
        }
    }

    /** Backdrop colour at row y, matching the vertical lerp in draw_backdrop. */
    private int cornerColor(int y) {
        return lerp(C_BG_TOP, C_BG, (y * 255) / (UI_H - 1));
    }

    private void drawHeader(Canvas canvas) {
        /* Gold rule under the header, brightest in the middle. */
        for (int x = 0; x < UI_W; x++) {
            int t = (x * 255) / (UI_W - 1);

            putPixel(canvas, x, HEADER_H - 2, lerp(C_ACCENT_LO, C_AMBER_HI, t));
        }
        hline(canvas, 0, HEADER_H - 1, UI_W, C_EDGE_SOFT);

        textScaled(canvas, MARGIN + 1, 11, "PiT LAUNCHER", C_SHADOW, 3);
        textScaled(canvas, MARGIN, 10, "PiT LAUNCHER", C_TEXT_HI, 3);
        text(canvas, MARGIN + 2, 30, "DECOMP ROM WORKSHOP", C_ACCENT);

        /* Version chip, right aligned. */
        String version = PatchData.PLAN_VERSION;
        int chipW = textWidth(version) + 14;
        int chipX = UI_W - MARGIN - chipW;

        roundGradient(canvas, chipX, 12, chipW, 16, 3, C_PANEL_HI, C_PANEL_LO);
        roundOutline(canvas, chipX, 12, chipW, 16, 3, C_EDGE_SOFT);
        text(canvas, chipX + 7, 16, version, C_AMBER);

        /* Region badge: a gold pill that says the ROM has to be EUR. */
        String badge = PatchData.ROM_BADGE;
        int pillW = textWidth(badge) + 16;
        int pillX = UI_W - MARGIN - pillW;
        int pillY = 12;

        if (pillX - textWidth(version) - 20 > MARGIN) {
            pillX -= textWidth(version) + 34;
            roundGradient(canvas, pillX, pillY, pillW, 16, 3, C_AMBER, C_ACCENT);
            rect(canvas, pillX + 3, pillY + 1, pillW - 6, 1, C_AMBER_HI);
            roundOutline(canvas, pillX, pillY, pillW, 16, 3, C_ACCENT_LO);
            text(canvas, pillX + 8, pillY + 4, badge, C_PILL_TEXT);
        }
    }

    private void putPixel(Canvas canvas, int x, int y, int color) {
        rect(canvas, x, y, 1, 1, color);
    }

    /* ---------------------------------------------------------------- tabs */

    private void drawTab(Canvas canvas, int tab, String label) {
        int x = TAB_X0 + tab * (TAB_W + TAB_GAP);
        boolean selected = state.tab == tab;
        boolean hot = hovered == BTN_TAB_PATCH + tab
                || (tab == TAB_MODS && state.focus == FOCUS_TOGGLE)
                || (tab == TAB_PATCH && (state.focus == FOCUS_BROWSE_IN
                        || state.focus == FOCUS_BROWSE_OUT || state.focus == FOCUS_PATCH));

        if (selected) {
            roundGradient(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_AMBER, C_ACCENT);
            rect(canvas, x + 2, TAB_Y + 1, TAB_W - 4, 1, C_AMBER_HI);
            roundOutline(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_AMBER_HI);
            text(canvas, x + (TAB_W - textWidth(label)) / 2, TAB_Y + (TAB_H - FONT_H) / 2,
                    label, C_PILL_TEXT);
            rect(canvas, x + 8, TAB_Y + TAB_H, TAB_W - 16, 2, C_ACCENT);
            rect(canvas, x + TAB_W / 2 - 1, TAB_Y + TAB_H + 2, 2, 1, C_AMBER);
        } else if (hot) {
            roundGradient(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_PANEL_HI, C_PANEL);
            roundOutline(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_EDGE_HOT);
            text(canvas, x + (TAB_W - textWidth(label)) / 2, TAB_Y + (TAB_H - FONT_H) / 2,
                    label, C_AMBER);
        } else {
            roundGradient(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_PANEL_HI, C_PANEL);
            roundOutline(canvas, x, TAB_Y, TAB_W, TAB_H, 3, C_EDGE_SOFT);
            text(canvas, x + (TAB_W - textWidth(label)) / 2, TAB_Y + (TAB_H - FONT_H) / 2,
                    label, C_DIM);
        }
    }

    private void drawToggle(Canvas canvas, boolean on, boolean hot) {
        if (on) {
            roundGradient(canvas, TOGGLE_X, TOGGLE_Y, TOGGLE_W, TOGGLE_H, TOGGLE_H / 2,
                    hot ? C_AMBER_HI : C_AMBER, C_ACCENT);
            roundOutline(canvas, TOGGLE_X, TOGGLE_Y, TOGGLE_W, TOGGLE_H, TOGGLE_H / 2,
                    C_AMBER_HI);
        } else {
            roundGradient(canvas, TOGGLE_X, TOGGLE_Y, TOGGLE_W, TOGGLE_H, TOGGLE_H / 2,
                    C_PANEL_LO, C_WELL);
            roundOutline(canvas, TOGGLE_X, TOGGLE_Y, TOGGLE_W, TOGGLE_H, TOGGLE_H / 2,
                    hot ? C_AMBER_HI : C_EDGE_SOFT);
        }
        int knob = TOGGLE_KNOB_W;
        int kx = on ? TOGGLE_X + TOGGLE_W - knob - 2 : TOGGLE_X + 2;
        int ky = TOGGLE_Y + (TOGGLE_H - knob) / 2;

        roundGradient(canvas, kx, ky, knob, knob, 3, C_TEXT_HI, C_TEXT);
        roundOutline(canvas, kx, ky, knob, knob, 3, C_EDGE);
    }

    private void drawFooter(Canvas canvas) {
        hline(canvas, 0, FOOTER_Y - 2, UI_W, C_EDGE_SOFT);
        text(canvas, MARGIN, FOOTER_Y, "FOR THE PARTNERS IN TIME DECOMPILATION", C_DIM);
        text(canvas, UI_W - MARGIN - textWidth("YOUR ROM"), FOOTER_Y, "YOUR ROM", C_AMBER);
    }

    /* ---------------------------------------------------------------- mods */

    private void drawChip(Canvas canvas, int x, int y, int w, int h, String label, String value,
            boolean hot) {
        dropShadow(canvas, x, y, w, h, 3);
        roundGradient(canvas, x, y, w, h, 3, hot ? C_PANEL_HI : C_PANEL, C_PANEL_LO);
        rect(canvas, x + 4, y + 1, w - 8, 1, C_EDGE);
        roundOutline(canvas, x, y, w, h, 3, C_EDGE_SOFT);

        /* Gold spine on the left edge marks it as a tuned stat. */
        rect(canvas, x + 1, y + 4, 2, h - 8, hot ? C_AMBER : C_ACCENT);
        rect(canvas, x + 1, y + 3, 2, 1, 0xA0FFFFFF);

        text(canvas, x + 8, y + 3, label, C_DIM);
        text(canvas, x + 8, y + 12, value, hot ? C_AMBER_HI : C_AMBER);
    }

    private void drawModsTab(Canvas canvas) {
        boolean on = state.hardMode;
        boolean toggleHot = hovered == BTN_TOGGLE || state.focus == FOCUS_TOGGLE;

        drawCard(canvas, MODS_CARD_X, MODS_CARD_Y, MODS_CARD_W, MODS_CARD_H, 4);
        text(canvas, MODS_CARD_X + 12, MODS_CARD_Y + 9, "MODS", C_ACCENT);
        text(canvas, MODS_CARD_X + MODS_CARD_W - 12 - textWidth("OPTIONAL"),
                MODS_CARD_Y + 9, "OPTIONAL", C_DIM);
        hline(canvas, MODS_CARD_X + 12, MODS_CARD_Y + 20, MODS_CARD_W - 24, C_EDGE_SOFT);

        drawToggle(canvas, on, toggleHot);
        text(canvas, MODS_NAME_X, MODS_NAME_Y, "HARD MODE", C_TEXT_HI);
        {
            String stateText = on ? "ON" : "OFF";
            int sw = textWidth(stateText);

            text(canvas, MODS_CARD_X + MODS_CARD_W - 12 - sw, TOGGLE_Y + TOGGLE_H / 2 - 4,
                    stateText, on ? C_AMBER : C_DIM);
        }

        text(canvas, MODS_CARD_X + 12, MODS_DESC_Y,
                "A DATA MOD THAT RAISES ENEMY STATS: HP, POW, DEF", C_DIM);
        text(canvas, MODS_CARD_X + 12, MODS_DESC_Y + LINE_H,
                "AND SPD UP, PLUS 75% MORE EXPERIENCE AND COINS.", C_DIM);
        hline(canvas, MODS_CARD_X + 12, MODS_DIV_Y, MODS_CARD_W - 24, C_EDGE_SOFT);

        if (on) {
            for (int i = 0; i < PatchData.TRANSFORM_LABEL.length; i++) {
                int col = i % CHIP_COUNT;
                int row = i / CHIP_COUNT;
                int cx = CHIP_X0 + col * (CHIP_W + CHIP_GAP);
                int cy = CHIP_Y0 + row * CHIP_DY;

                drawChip(canvas, cx, cy, CHIP_W, CHIP_H, PatchData.TRANSFORM_LABEL[i],
                        PatchData.TRANSFORM_SCALE[i], true);
            }
        } else {
text(canvas, MODS_CARD_X + 12, CHIP_Y0,
                "OFF: THE LAUNCHER VERIFIES YOUR EUR ROM AND", C_DIM);
        text(canvas, MODS_CARD_X + 12, CHIP_Y0 + LINE_H,
                "PASSES IT TO THE DECOMP - NO MOD IS APPLIED.", C_DIM);
        text(canvas, MODS_CARD_X + 12, CHIP_Y0 + 2 * LINE_H,
                "TAP THE TOGGLE FOR HARD MODE, THEN INGEST.", C_DIM);
        }

        text(canvas, MARGIN, MODS_HINT_Y,
                "MODS ARE OPTIONAL DATA EDITS TO YOUR OWN ROM COPY.", C_DIM);
    }

    /* --------------------------------------------------------------- about */

    private void drawAboutTab(Canvas canvas) {
        String[] lines = {
            "THIS LAUNCHER WORKS WITH YOUR OWN EUR COPY OF",
            "MARIO & LUIGI: PARTNERS IN TIME ('ARMP'). IT",
            "VERIFIES THE ROM BY SHA-1 AND HEADER CRC-16,",
            "THEN PASSES YOUR CARTRIDGE TO THE DECOMP.",
            "",
            "THE GAME, ITS ASSETS AND ITS AUDIO STAY YOURS.",
            "",
            "PARTNERS IN TIME IS A DECOMPILATION, NOT A",
            "REBUILD: THE SOURCE RE-CREATES THE GAME FROM",
            "YOUR ROM'S OWN DATA. NO NINTENDO CONTENT IS",
            "SHIPPED, EXTRACTED OR COMMITTED HERE.",
            "",
            "HARD MODE IS AN OPTIONAL DATA MOD (MODS TAB)",
            "THAT RAISES ENEMY STATS. IT IS OFF BY DEFAULT.",
        };

        drawCard(canvas, ABOUT_CARD_X, ABOUT_CARD_Y, ABOUT_CARD_W, ABOUT_CARD_H, 4);
        text(canvas, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 9, "ABOUT", C_ACCENT);
        text(canvas, ABOUT_CARD_X + ABOUT_CARD_W - 12 - textWidth("PIT LAUNCHER"),
                ABOUT_CARD_Y + 9, "PIT LAUNCHER", C_DIM);
        hline(canvas, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 20, ABOUT_CARD_W - 24, C_EDGE_SOFT);

        text(canvas, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 32,
                "PARTNERS IN TIME - DECOMPILATION EDITION", C_TEXT_HI);
        hline(canvas, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 42, ABOUT_CARD_W - 24, C_EDGE_SOFT);

        for (int i = 0; i < lines.length; i++) {
            int ly = ABOUT_CARD_Y + 54 + i * LINE_H;

            text(canvas, ABOUT_CARD_X + 12, ly, lines[i], C_TEXT);
        }
        text(canvas, ABOUT_CARD_X + 12, ABOUT_CARD_Y + ABOUT_CARD_H - 14,
                PatchData.PLAN_ID + " v" + PatchData.PLAN_VERSION + " | "
                        + PatchData.RECORD_COUNT + " x " + PatchData.RECORD_SIZE
                        + "-BYTE RECORDS",
                C_DIM);
    }

    /* -------------------------------------------------------------- fields */

    private void drawFields(Canvas canvas) {
        String[] values = {state.sourceName, state.outputName};

        for (int i = 0; i < 2; i++) {
            String label = i == 0 ? "SOURCE" : "OUTPUT";
            String value = values[i].isEmpty()
                    ? (i == 0 ? "CHOOSE EUR ROM" : "PREPARED COPY") : values[i];
            int fy = FIELD_Y0 + i * FIELD_DY;

            text(canvas, MARGIN, fy + 4, label, C_DIM);
            drawWell(canvas, FIELD_X, fy, FIELD_W, FIELD_H, false);
            textClipped(canvas, FIELD_X + 4, fy + 4, FIELD_X + FIELD_W - 4, value, C_TEXT);
        }
    }

    /* -------------------------------------------------------------- actions */

    private void drawButton(Canvas canvas, int x, int y, int w, int h, String label,
            boolean hot, boolean primary, boolean enabled) {
        if (primary) {
            if (enabled) {
                dropShadow(canvas, x, y, w, h, 3);
                roundGradient(canvas, x, y, w, h, 3, hot ? C_AMBER_HI : C_AMBER, C_ACCENT_LO);
                roundOutline(canvas, x, y, w, h, 3, hot ? C_AMBER_HI : C_ACCENT);
                hline(canvas, x + 4, y + 1, w - 8, C_AMBER_HI);
                text(canvas, x + (w - textWidth(label)) / 2, y + (h - FONT_H) / 2 + 1, label,
                        C_PILL_TEXT);
            } else {
                roundGradient(canvas, x, y, w, h, 3, C_PANEL_HI, C_PANEL_LO);
                roundOutline(canvas, x, y, w, h, 3, C_EDGE_SOFT);
                text(canvas, x + (w - textWidth(label)) / 2, y + (h - FONT_H) / 2 + 1, label,
                        C_DIM);
            }
            return;
        }

        if (enabled) {
            dropShadow(canvas, x, y, w, h, 2);
            roundGradient(canvas, x, y, w, h, 2, hot ? C_PANEL_HI : C_PANEL, C_PANEL_LO);
            roundOutline(canvas, x, y, w, h, 2, hot ? C_EDGE_HOT : C_EDGE_SOFT);
        } else {
            roundGradient(canvas, x, y, w, h, 2, C_PANEL, C_PANEL_LO);
            roundOutline(canvas, x, y, w, h, 2, C_EDGE_SOFT);
        }
        int color = !enabled ? C_DIM : (hot ? C_TEXT_HI : C_TEXT);
        text(canvas, x + (w - textWidth(label)) / 2, y + (h - FONT_H) / 2, label, color);
    }

    private boolean patchEnabled() {
        return !state.busy && !state.patched && !state.failed;
    }

    private void drawActions(Canvas canvas) {
        boolean hotIn = hovered == BTN_BROWSE_IN || state.focus == FOCUS_BROWSE_IN;
        boolean hotOut = hovered == BTN_BROWSE_OUT || state.focus == FOCUS_BROWSE_OUT;
        boolean hotPatch = hovered == BTN_PATCH || state.focus == FOCUS_PATCH;

        drawButton(canvas, BROWSE_X, FIELD_Y0 - 2, BROWSE_W, BROWSE_H, "BROWSE",
                hotIn, false, true);
        drawButton(canvas, BROWSE_X, FIELD_Y0 + FIELD_DY - 2, BROWSE_W, BROWSE_H, "BROWSE",
                hotOut, false, true);
        drawButton(canvas, PATCH_X, PATCH_Y, PATCH_W, PATCH_H, "INGEST",
                hotPatch, true, patchEnabled());

        String caption = "STATUS";
        String value;
        int color;

        if (state.busy) {
            value = "WORKING " + state.step + "/" + state.totalSteps;
            color = C_AMBER;
        } else if (state.failed) {
            value = "FAILED";
            color = C_ERROR;
        } else if (state.patched) {
            value = "DONE";
            color = C_OK;
        } else {
            value = "READY";
            color = C_DIM;
        }
        text(canvas, STATUS_X, PATCH_Y + 4, caption, C_DIM);
        text(canvas, STATUS_X + textWidth(caption) + 8, PATCH_Y + 4, value, color);

        drawProgress(canvas, BAR_X, BAR_Y, BAR_W, BAR_H, fractionForRender());
    }

    /** fraction as painted, including the idle stub the C renderer also draws. */
    double fractionForRender() {
        if (state.fraction > 0) {
            return state.fraction;
        }
        return state.busy || state.patched || state.failed ? 1.0 : IDLE_STUB;
    }

    private void drawProgress(Canvas canvas, int x, int y, int w, int h, double fraction) {
        roundRect(canvas, x, y, w, h, 2, C_TRACK);
        roundOutline(canvas, x, y, w, h, 2, C_EDGE_SOFT);

        int inner = w - 4;
        int fillW = (int) (inner * fraction);

        if (fillW <= 0) {
            return;
        }
        int top = state.failed ? C_ERROR : (state.patched ? C_OK : C_AMBER);
        int bottom = state.failed ? lerp(C_ERROR, C_ACCENT_LO, 200) : C_ACCENT;
        int barH = h - 4;

        if (fillW >= inner) {
            roundGradient(canvas, x + 2, y + 2, inner, barH, 1, top, bottom);
        } else {
            /* The C renderer squares off the leading end of a partial fill. */
            fill.setShader(new LinearGradient(x, y, x, y + h, top, bottom, Shader.TileMode.CLAMP));
            canvas.drawRect(x + 2, y + 2, x + 2 + fillW, y + 2 + barH, fill);
            fill.setShader(null);
            rect(canvas, x + 2, y + 2, Math.min(1, fillW), barH, C_AMBER_HI);
        }
    }

    /* ------------------------------------------------------------------ log */

    private void drawLog(Canvas canvas) {
        drawCard(canvas, LOG_X, LOG_Y, LOG_W, LOG_H, 4);
        text(canvas, LOG_X + 12, LOG_Y + 9, "ACTIVITY", C_ACCENT);
        hline(canvas, LOG_X + 12, LOG_Y + 20, LOG_W - 24, C_EDGE_SOFT);

        int rows = (LOG_H - 28) / LINE_H;
        int first = Math.max(0, state.log.size() - rows);

        for (int row = first; row < state.log.size(); row++) {
            int ly = LOG_Y + 26 + (row - first) * LINE_H;
            LogLine line = state.log.get(row);

            /* Level dot, so severity reads without relying on the prefix. */
            if (line.color != C_TEXT) {
                roundRect(canvas, LOG_X + 12, ly + 2, 3, 3, 1, line.color);
            }
            textClipped(canvas, LOG_X + 20, ly, LOG_X + LOG_W - 12, line.message, line.color);
        }
    }

    private void drawPatchTab(Canvas canvas) {
        drawFields(canvas);

        /* Plan status on the left, keyboard hints on the right. */
        text(canvas, MARGIN, MODS_Y, state.hardMode ? "MODS: HARD MODE" : "MODS: NONE",
                state.hardMode ? C_AMBER : C_DIM);
        text(canvas, UI_W - MARGIN - textWidth("TAB FOCUS  ENTER INGEST  ESC QUIT"),
                MODS_Y, "TAB FOCUS  ENTER INGEST  ESC QUIT", C_DIM);

        drawActions(canvas);
        drawLog(canvas);
    }

    private void render(Canvas canvas) {
        drawBackdrop(canvas);
        drawHeader(canvas);

        drawTab(canvas, TAB_PATCH, "ROM");
        drawTab(canvas, TAB_MODS, "MODS");
        drawTab(canvas, TAB_ABOUT, "ABOUT");

        if (state.tab == TAB_MODS) {
            drawModsTab(canvas);
        } else if (state.tab == TAB_ABOUT) {
            drawAboutTab(canvas);
        } else {
            drawPatchTab(canvas);
        }

        drawFooter(canvas);
    }

    /* --------------------------------------------------------------- input */

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        scale = Math.min(w / (float) UI_W, h / (float) UI_H);
        offsetX = (w - UI_W * scale) / 2f;
        offsetY = (h - UI_H * scale) / 2f;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.save();
        canvas.translate(offsetX, offsetY);
        canvas.scale(scale, scale);
        render(canvas);
        canvas.restore();
    }

    private boolean isFocused(int buttonNow) {
        return hovered == buttonNow && state.busy
                ? false : state.focus == focusOf(buttonNow);
    }

    private static int focusOf(int buttonNow) {
        switch (buttonNow) {
            case BTN_BROWSE_IN:
                return FOCUS_BROWSE_IN;
            case BTN_BROWSE_OUT:
                return FOCUS_BROWSE_OUT;
            case BTN_PATCH:
                return FOCUS_PATCH;
            case BTN_TOGGLE:
                return FOCUS_TOGGLE;
            default:
                return FOCUS_NONE;
        }
    }

    /** Geometry is fixed, so hit tests and painting cannot disagree. */
    private int buttonAt(float x, float y) {
        for (int tab = 0; tab < TAB_COUNT; tab++) {
            int tx = TAB_X0 + tab * (TAB_W + TAB_GAP);

            if (inButton(x, y, tx, TAB_Y, TAB_W, TAB_H)) {
                return BTN_TAB_PATCH + tab;
            }
        }
        if (state.tab == TAB_MODS
                && inButton(x, y, TOGGLE_X, TOGGLE_Y, TOGGLE_W, TOGGLE_H)) {
            return BTN_TOGGLE;
        }
        if (state.tab == TAB_PATCH) {
            if (inButton(x, y, BROWSE_X, FIELD_Y0 - 2, BROWSE_W, BROWSE_H)) {
                return BTN_BROWSE_IN;
            }
            if (inButton(x, y, BROWSE_X, FIELD_Y0 + FIELD_DY - 2, BROWSE_W, BROWSE_H)) {
                return BTN_BROWSE_OUT;
            }
            if (inButton(x, y, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)) {
                return BTN_PATCH;
            }
        }
        return BTN_NONE;
    }

    private static boolean inButton(float x, float y, int bx, int by, int bw, int bh) {
        return x >= bx && x < bx + bw && y >= by && y < by + bh;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = (event.getX() - offsetX) / scale;
        float y = (event.getY() - offsetY) / scale;

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                hovered = buttonAt(x, y);
                invalidate();
                return true;
            case MotionEvent.ACTION_UP: {
                int hit = buttonAt(x, y);
                if (hit == hovered) {
                    activate(hit);
                }
                hovered = BTN_NONE;
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                hovered = BTN_NONE;
                invalidate();
                return true;
            default:
                return true;
        }
    }

    /* ------------------------------------------------------------- keyboard */

    private static final int[] PATCH_CYCLE = {
        FOCUS_BROWSE_IN, FOCUS_BROWSE_OUT, FOCUS_PATCH
    };

    private void focusCycle(boolean backward) {
        if (state.tab == TAB_MODS) {
            state.focus = FOCUS_TOGGLE;
        } else if (state.tab == TAB_ABOUT) {
            state.focus = FOCUS_NONE;
        } else {
            int index = -1;

            for (int i = 0; i < PATCH_CYCLE.length; i++) {
                if (PATCH_CYCLE[i] == state.focus) {
                    index = i;
                    break;
                }
            }
            if (index < 0) {
                index = backward ? PATCH_CYCLE.length - 1 : 0;
            } else if (backward) {
                index = (index + PATCH_CYCLE.length - 1) % PATCH_CYCLE.length;
            } else {
                index = (index + 1) % PATCH_CYCLE.length;
            }
            state.focus = PATCH_CYCLE[index];
        }
        invalidate();
    }

    private void focusMove(int delta) {
        int index = -1;

        for (int i = 0; i < PATCH_CYCLE.length; i++) {
            if (PATCH_CYCLE[i] == state.focus) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            index = delta > 0 ? 0 : PATCH_CYCLE.length - 1;
        } else {
            index = (index + delta + PATCH_CYCLE.length) % PATCH_CYCLE.length;
        }
        state.focus = PATCH_CYCLE[index];
        invalidate();
    }

    private void switchTab(int delta) {
        int next = (state.tab + delta + TAB_COUNT) % TAB_COUNT;

        setTab(next);
        if (callback != null) {
            callback.onTab(next);
        }
    }

    private void activate(int id) {
        switch (id) {
            case BTN_TAB_PATCH:
            case BTN_TAB_MODS:
            case BTN_TAB_ABOUT:
                switchTab(id - BTN_TAB_PATCH - state.tab);
                break;
            case BTN_TOGGLE:
                if (state.tab == TAB_MODS) {
                    state.hardMode = !state.hardMode;
                    state.focus = FOCUS_TOGGLE;
                    if (callback != null) {
                        callback.onToggleMods();
                    }
                    invalidate();
                }
                break;
            case BTN_BROWSE_IN:
                if (state.tab == TAB_PATCH && callback != null) {
                    callback.onBrowseSource();
                }
                break;
            case BTN_BROWSE_OUT:
                if (state.tab == TAB_PATCH && callback != null) {
                    callback.onBrowseOutput();
                }
                break;
            case BTN_PATCH:
                if (state.tab == TAB_PATCH && callback != null) {
                    callback.onPatch();
                }
                break;
            default:
                break;
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_TAB:
                focusCycle(event.hasModifiers(KeyEvent.META_SHIFT_ON));
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                switchTab(1);
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                switchTab(-1);
                return true;
            case KeyEvent.KEYCODE_DPAD_UP:
                focusMove(-1);
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                focusMove(1);
                return true;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_SPACE:
                activate(focusOf(state.focus));
                return true;
            default:
                return super.onKeyDown(keyCode, event);
        }
    }
}