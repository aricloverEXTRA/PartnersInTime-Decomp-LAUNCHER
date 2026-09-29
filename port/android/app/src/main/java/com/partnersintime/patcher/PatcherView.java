package com.partnersintime.patcher;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

/**
 * The patcher screen, drawn with the same 8-bit look as the Windows build.
 *
 * <p>Everything is painted by hand: a fixed 400x240 design grid is scaled to fit
 * the screen, and text is blitted from {@link PatchData#FONT8X8} as rectangles
 * rather than drawn with a system typeface. That keeps the two builds visually
 * identical and avoids shipping a font file, and it means the UI needs no
 * resources beyond the string table in {@link Strings}.
 *
 * <p>The view is otherwise inert. It reports taps through {@link Callback} and
 * lets the activity own all state, so the same layout can be exercised in a
 * test without an Android runtime.
 */
public final class PatcherView extends View {

    /** Design-space size, matching UI_W/UI_H in the C renderer. */
    public static final int UI_W = 400;
    public static final int UI_H = 240;

    private static final int FONT_W = 8;
    private static final int FONT_H = 8;
    private static final int TITLE_SCALE = 2;
    private static final int LINE_H = 10;
    private static final int MAX_LOG_ROWS = 6;

    /* Palette, matching the C renderer. Every value comes from the real EUR
     * ROM's 15-bit BGR555 palettes, expanded with c * 255 / 31. See
     * port/src/platform/sdl2/pit_patcher_ui.c for the ROM offsets. */
    private static final int C_BG = 0xFF001029;
    private static final int C_PANEL = 0xFF00106B;
    private static final int C_PANEL_ALT = 0xFF001094;
    private static final int C_EDGE = 0xFF52637B;
    private static final int C_EDGE_HOT = 0xFFFFA521;
    private static final int C_ACCENT = 0xFFDE9429;
    private static final int C_AMBER = 0xFFFF9421;
    private static final int C_TEXT = 0xFFC5DEF7;
    private static final int C_DIM = 0xFF738494;
    private static final int C_ERROR = 0xFFEF5A63;
    private static final int C_OK = 0xFF52C55A;

    private static final int BTN_BROWSE_SOURCE = 0;
    private static final int BTN_BROWSE_OUTPUT = 1;
    private static final int BTN_PATCH = 2;

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
        public String sourceName = "(none)";
        public String outputName = "(none)";
        public final List<LogLine> log = new ArrayList<LogLine>();
        public int step;
        public double fraction;
        public boolean busy;
        public boolean patched;
        public int error = -1;
    }

    public interface Callback {
        void onBrowseSource();

        void onBrowseOutput();

        void onPatch();
    }

    private final Paint fill = new Paint();

    private final State state = new State();
    private Callback callback;

    private int buttonsY;
    private int progressY;

    public PatcherView(Context context) {
        this(context, null);
    }

    public PatcherView(Context context, AttributeSet attrs) {
        super(context, attrs);
        fill.setStyle(Paint.Style.FILL);
        setClickable(true);
    }

    public void setCallback(Callback callback) {
        this.callback = callback;
    }

    public State state() {
        return state;
    }

    public void addLog(int color, String message) {
        if (state.log.size() >= MAX_LOG_ROWS) {
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
        state.error = -1;
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
        state.busy = false;
        state.fraction = 1.0;
        invalidate();
    }

    public void setError(int code) {
        state.error = code;
        state.busy = false;
        invalidate();
    }

    public void setPaths(String source, String output) {
        state.sourceName = source;
        state.outputName = output;
        invalidate();
    }

    private void rect(Canvas canvas, int x, int y, int w, int h, int color) {
        fill.setColor(color);
        canvas.drawRect(x, y, x + w, y + h, fill);
    }

    /** Draws one 8x8 glyph; unknown characters fall back to a blank. */
    private void glyph(Canvas canvas, char ch, int x, int y, int color) {
        int index = ch - PatchData.FONT_FIRST;
        if (index < 0 || index >= PatchData.FONT_GLYPHS) {
            return;
        }
        fill.setColor(color);
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

    private void text(Canvas canvas, int x, int y, String s, int color) {
        for (int i = 0; i < s.length(); i++) {
            glyph(canvas, s.charAt(i), x + i * FONT_W, y, color);
        }
    }

    private void textClipped(Canvas canvas, int x, int y, int limit, String s, int color) {
        int max = Math.max(0, (limit - x) / FONT_W);

        if (s.length() > max) {
            s = s.substring(0, Math.max(0, max - 1)) + ".";
        }
        text(canvas, x, y, s, color);
    }

    /** Title text, drawn at TITLE_SCALE so the header reads at a glance. */
    private void title(Canvas canvas, int x, int y, String s, int color) {
        for (int i = 0; i < s.length(); i++) {
            int index = s.charAt(i) - PatchData.FONT_FIRST;
            if (index < 0 || index >= PatchData.FONT_GLYPHS) {
                continue;
            }
            fill.setColor(color);
            int[] rows = PatchData.FONT8X8[index];

            for (int row = 0; row < FONT_H; row++) {
                int bits = rows[row];
                for (int col = 0; col < FONT_W; col++) {
                    if ((bits & (0x80 >> col)) != 0) {
                        canvas.drawRect(x + i * FONT_W * TITLE_SCALE + col * TITLE_SCALE,
                                y + row * TITLE_SCALE,
                                x + i * FONT_W * TITLE_SCALE + col * TITLE_SCALE + TITLE_SCALE,
                                y + row * TITLE_SCALE + TITLE_SCALE, fill);
                    }
                }
            }
        }
    }

    private void hline(Canvas canvas, int x, int y, int w, int color) {
        rect(canvas, x, y, w, 1, color);
    }

    private void frame(Canvas canvas, int x, int y, int w, int h, int color) {
        hline(canvas, x, y, w, color);
        hline(canvas, x, y + h - 1, w, color);
        rect(canvas, x, y, 1, h, color);
        rect(canvas, x + w - 1, y, 1, h, color);
    }

    private void button(Canvas canvas, int x, int y, int w, int h, String label,
            boolean hot, boolean enabled) {
        int body = !enabled ? C_PANEL : (hot ? C_EDGE_HOT : C_PANEL_ALT);
        rect(canvas, x + 1, y + 2, w, h, C_BG);
        rect(canvas, x, y, w, h, body);
        frame(canvas, x, y, w, h, hot && enabled ? C_ACCENT : C_EDGE);

        int tw = label.length() * FONT_W;
        text(canvas, x + (w - tw) / 2, y + (h - FONT_H) / 2, label,
                enabled ? C_TEXT : C_DIM);
    }

    /** Redraws the whole screen; the C build renders the same layout in one pass. */
    private void render(Canvas canvas) {
        rect(canvas, 0, 0, UI_W, UI_H, C_BG);

        title(canvas, 8, 8, "PiT PATCHER", C_TEXT);
        text(canvas, UI_W - 8 - 8 * PatchData.ROM_BADGE.length(), 12, PatchData.ROM_BADGE,
                C_AMBER);
        hline(canvas, 8, 34, UI_W - 16, C_EDGE);

        /* Plan panel: the six transforms, two columns, with exact rationals. */
        int panelY = 40;
        int panelH = 34;
        rect(canvas, 8, panelY, UI_W - 16, panelH, C_PANEL);
        frame(canvas, 8, panelY, UI_W - 16, panelH, C_EDGE);
        text(canvas, 14, panelY + 5, PatchData.PLAN_NAME + " " + PatchData.PLAN_VERSION,
                C_ACCENT);

        int half = (UI_W - 16) / 2;
        for (int i = 0; i < PatchData.TRANSFORM_LABEL.length; i++) {
            int col = (i % 2) * half;
            int row = i / 2;
            int tx = 14 + col;
            int ty = panelY + 15 + row * 9;
            text(canvas, tx, ty, PatchData.TRANSFORM_LABEL[i], C_TEXT);
            text(canvas, tx + 88, ty, PatchData.TRANSFORM_SCALE[i], C_AMBER);
        }

        /* Source and output fields. */
        int fieldY = 80;
        int fieldH = 14;
        int labelW = 8 * 6 + 4;
        int fieldW = UI_W - 16 - labelW - 66;

        rect(canvas, 8, fieldY, UI_W - 16, fieldH, C_PANEL);
        frame(canvas, 8, fieldY, UI_W - 16, fieldH, C_EDGE);
        text(canvas, 12, fieldY + 3, "SOURCE", C_DIM);
        textClipped(canvas, labelW, fieldY + 3, labelW + fieldW - 4, state.sourceName,
                state.sourceName.startsWith("(") ? C_DIM : C_TEXT);

        fieldY += fieldH + 4;
        rect(canvas, 8, fieldY, UI_W - 16, fieldH, C_PANEL);
        frame(canvas, 8, fieldY, UI_W - 16, fieldH, C_EDGE);
        text(canvas, 12, fieldY + 3, "OUTPUT", C_DIM);
        textClipped(canvas, labelW, fieldY + 3, labelW + fieldW - 4, state.outputName,
                state.outputName.startsWith("(") ? C_DIM : C_TEXT);

        /* Buttons, placed once so hit testing and drawing cannot disagree. */
        buttonsY = fieldY + fieldH + 4;
        int btnW = 62;
        int btnH = 14;
        button(canvas, 8, buttonsY, btnW, btnH, "BROWSE", hovered == BTN_BROWSE_SOURCE,
                true);
        button(canvas, 8 + btnW + 4, buttonsY, btnW, btnH, "BROWSE",
                hovered == BTN_BROWSE_OUTPUT, true);
        button(canvas, UI_W - 8 - btnW, buttonsY, btnW, btnH, "PATCH ROM",
                hovered == BTN_PATCH, !state.busy && !state.patched);

        /* Progress. */
        progressY = buttonsY + btnH + 6;
        text(canvas, 8, progressY, "STEPS", C_DIM);
        int barX = 8 + labelW;
        int barW = UI_W - 8 - barX;
        rect(canvas, barX, progressY, barW, 8, C_PANEL);
        frame(canvas, barX, progressY, barW, 8, C_EDGE);
        int fillW = (int) (barW * state.fraction);
        if (fillW > 0) {
            int color = state.error >= 0 ? C_ERROR : (state.patched ? C_ACCENT : C_EDGE_HOT);
            rect(canvas, barX + 1, progressY + 1, fillW, 6, color);
        }

        /* Log pane. */
        int logY = progressY + 14;
        int logH = UI_H - 8 - logY;
        rect(canvas, 8, logY, UI_W - 16, logH, C_PANEL);
        frame(canvas, 8, logY, UI_W - 16, logH, C_EDGE);

        int rows = Math.min(MAX_LOG_ROWS, (logH - 6) / LINE_H);
        int first = state.log.size() - rows;
        for (int i = 0; i < rows; i++) {
            int slot = first + i;
            if (slot < 0 || slot >= state.log.size()) {
                continue;
            }
            LogLine line = state.log.get(slot);
            textClipped(canvas, 12, logY + 4 + i * LINE_H, UI_W - 12, line.message,
                    line.color);
        }
    }

    private float scale = 1f;
    private float offsetX;
    private float offsetY;
    private int hovered = -1;

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

    private int buttonAt(float x, float y) {
        int btnW = 62;
        int btnH = 14;
        if (y < buttonsY || y > buttonsY + btnH) {
            return -1;
        }
        if (x >= 8 && x < 8 + btnW) {
            return BTN_BROWSE_SOURCE;
        }
        if (x >= 8 + btnW + 4 && x < 8 + btnW + 4 + btnW) {
            return BTN_BROWSE_OUTPUT;
        }
        if (x >= UI_W - 8 - btnW && x < UI_W - 8) {
            return BTN_PATCH;
        }
        return -1;
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
                    if (hit == BTN_BROWSE_SOURCE && callback != null) {
                        callback.onBrowseSource();
                    } else if (hit == BTN_BROWSE_OUTPUT && callback != null) {
                        callback.onBrowseOutput();
                    } else if (hit == BTN_PATCH && callback != null) {
                        callback.onPatch();
                    }
                }
                hovered = -1;
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                hovered = -1;
                invalidate();
                return true;
            default:
                return true;
        }
    }
}
