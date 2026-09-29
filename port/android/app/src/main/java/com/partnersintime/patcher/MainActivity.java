package com.partnersintime.patcher;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

/**
 * Hosts the patcher screen and owns all of its state.
 *
 * <p>The 64 MiB ROM is loaded, patched and written on a worker thread; only log
 * lines and progress hop back to the main thread. Files are chosen and written
 * through the Storage Access Framework, so the app needs no storage permission
 * and never sees a path outside the document the user picked.
 *
 * <p>No dependency on SDL, the NDK or a build system: this is plain Java against
 * the platform APIs, so the APK builds with javac and d8 alone.
 */
public final class MainActivity extends Activity implements PatcherView.Callback {

    private static final int REQ_OPEN_ROM = 1;
    private static final int REQ_CREATE_ROM = 2;

    /* Log colours, matching the C renderer. From the real EUR ROM's
     * BGR555 palettes; see pit_patcher_ui.c for the ROM offsets. */
    private static final int LOG_INFO = 0xFF738494;
    private static final int LOG_OK = 0xFF52C55A;
    private static final int LOG_ERROR = 0xFFEF5A63;
    private static final int LOG_WORK = 0xFFFF9421;

    private PatcherView view;
    private final Handler ui = new Handler(Looper.getMainLooper());

    private Uri source;
    private Uri destination;
    private volatile boolean cancelled;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        view = new PatcherView(this);
        view.setCallback(this);
        setContentView(view);
        view.addLog(LOG_INFO, "Select your EUR ROM, then choose an output name.");
    }

    @Override
    protected void onDestroy() {
        cancelled = true;
        super.onDestroy();
    }

    @Override
    public void onBrowseSource() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {"application/x-nintendo-ds-rom"});
        startActivityForResult(intent, REQ_OPEN_ROM);
    }

    @Override
    public void onBrowseOutput() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE, "PiT_hard_mode.nds");
        startActivityForResult(intent, REQ_CREATE_ROM);
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (result != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri uri = data.getData();
        String name = displayName(uri);

        if (request == REQ_OPEN_ROM) {
            source = uri;
            view.setPaths(name, destination == null ? "(none)" : displayName(destination));
            view.addLog(LOG_INFO, "Source: " + name);
        } else if (request == REQ_CREATE_ROM) {
            destination = uri;
            view.setPaths(source == null ? "(none)" : displayName(source), name);
            view.addLog(LOG_INFO, "Output: " + name);
        }
    }

    /** Last path segment, which is the only name SAF guarantees us. */
    private String displayName(Uri uri) {
        String name = uri.getLastPathSegment();
        int slash = name == null ? -1 : name.lastIndexOf('/');
        return slash < 0 ? name : name.substring(slash + 1);
    }

    @Override
    public void onPatch() {
        if (source == null) {
            view.addLog(LOG_ERROR, "Choose a source ROM first.");
            return;
        }
        if (destination == null) {
            view.addLog(LOG_ERROR, "Choose an output file first.");
            return;
        }
        if (destination.equals(source)) {
            view.addLog(LOG_ERROR, "The output would overwrite the source ROM.");
            return;
        }
        runPatch();
    }

    private void log(final int color, final String message) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (!isFinishing()) {
                    view.addLog(color, message);
                }
            }
        });
    }

    private void progress(final int step, final String message, final double fraction) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (!isFinishing()) {
                    view.setProgress(step, fraction);
                    view.addLog(LOG_WORK, message);
                }
            }
        });
    }

    private void finishWith(final int code, final String message) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (isFinishing()) {
                    return;
                }
                if (code == Patcher.OK) {
                    view.setPatched(true);
                    view.addLog(LOG_OK, message);
                } else {
                    view.setError(code);
                    view.addLog(LOG_ERROR, message);
                }
            }
        });
    }

    private void runPatch() {
        cancelled = false;
        view.clearLog();
        view.setBusy(true);
        setButtonsEnabled(false);

        new Thread(new Runnable() {
            @Override
            public void run() {
                Patcher.Result result = new Patcher.Result();
                byte[] rom = null;
                byte[] out = null;

                try {
                    rom = readAll(source, PatchData.ROM_SIZE + 1);
                    if (rom == null) {
                        result.code = Patcher.ERR_READ;
                    } else {
                        out = new byte[rom.length];
                        Patcher.Listener listener = new Patcher.Listener() {
                            @Override
                            public void onStep(int step, String message, double fraction) {
                                progress(step, message, fraction);
                            }
                        };
                        result = Patcher.run(rom, out, listener);
                        if (result.code != Patcher.OK) {
                            out = null;
                        }
                    }

                    if (result.code == Patcher.OK) {
                        log(LOG_WORK, "Writing output...");
                        if (!writeAll(destination, out)) {
                            result.code = Patcher.ERR_WRITE;
                            out = null;
                        }
                    }
                } catch (OutOfMemoryError e) {
                    /*
                     * A 128 MiB working set needs a large heap. Saying so is far
                     * more useful than a crash dialog, and the device is
                     * otherwise fine.
                     */
                    result.code = Patcher.ERR_READ;
                    log(LOG_ERROR, "Not enough memory: 128 MiB is required.");
                    out = null;
                } catch (Exception e) {
                    result.code = Patcher.ERR_READ;
                    log(LOG_ERROR, e.getClass().getSimpleName() + ": " + e.getMessage());
                    out = null;
                } finally {
                    rom = null;
                }

                if (cancelled) {
                    return;
                }
                if (result.code == Patcher.OK) {
                    out = null;
                    log(LOG_OK, String.format(
                            "Patched %d records, %d fields. Header CRC-16 %04X.",
                            result.recordsPatched, result.fieldsWritten, result.headerCrc));
                }
                finishWith(result.code, Patcher.resultText(result.code));
                ui.post(new Runnable() {
                    @Override
                    public void run() {
                        setButtonsEnabled(true);
                    }
                });
            }
        }, "pit-patch").start();
    }

    private void setButtonsEnabled(final boolean enabled) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (!isFinishing()) {
                    view.setBusy(!enabled);
                }
            }
        });
    }

    /** Reads the document, refusing anything larger than the expected cartridge. */
    private byte[] readAll(Uri uri, int limit) throws Exception {
        java.io.InputStream in = getContentResolver().openInputStream(uri);
        if (in == null) {
            return null;
        }
        try {
            java.io.ByteArrayOutputStream buffer =
                    new java.io.ByteArrayOutputStream(PatchData.ROM_SIZE);
            byte[] chunk = new byte[65536];
            int total = 0;
            int n;
            while ((n = in.read(chunk)) > 0) {
                total += n;
                if (total > limit) {
                    return null;
                }
                buffer.write(chunk, 0, n);
            }
            return buffer.toByteArray();
        } finally {
            in.close();
        }
    }

    private boolean writeAll(Uri uri, byte[] data) throws Exception {
        if (data == null) {
            return false;
        }
        java.io.OutputStream out = getContentResolver().openOutputStream(uri, "wt");
        if (out == null) {
            return false;
        }
        try {
            out.write(data);
            out.flush();
            return true;
        } finally {
            out.close();
        }
    }
}
