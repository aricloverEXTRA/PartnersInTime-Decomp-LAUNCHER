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

    /* Log levels, not colours: PatcherView maps them to the ROM palette so the
     * colour of a line cannot drift away from the Windows build. */
    private static final int LOG_INFO = PatcherView.LOG_INFO;
    private static final int LOG_OK = PatcherView.LOG_OK;
    private static final int LOG_WARN = PatcherView.LOG_WARN;
    private static final int LOG_ERROR = PatcherView.LOG_ERROR;

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
        view.addLog(LOG_INFO, "HARD MODE is optional: MODS tab, off by default.");
        view.addLog(LOG_INFO, "TIP: Tab / D-pad moves focus; Enter activates.");
        for (String line : CompatInfo.report(this)) {
            view.addLog(LOG_INFO, line);
        }
    }

    @Override
    public void onTab(int tab) {
        view.setTab(tab);
        view.addLog(LOG_INFO, tab == PatcherView.TAB_MODS ? "MODS tab."
                : (tab == PatcherView.TAB_ABOUT ? "ABOUT tab." : "ROM tab."));
    }

    @Override
    public void onToggleMods() {
        boolean on = view.state().hardMode;
        view.addLog(LOG_INFO, on
                ? "HARD MODE ON: the patch will raise enemy stats."
                : "HARD MODE OFF: the copy is prepared without mods.");
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
        /*
         * All files. A provider reports .nds as application/octet-stream or as
         * its own made-up type, and never as application/x-nintendo-ds-rom;
         * restricting the picker to that extra hides the cartridge entirely on
         * some devices. Letting everything through is what makes it findable.
         */
        intent.setType("*/*");
        startActivityForResult(intent, REQ_OPEN_ROM);
    }

    @Override
    public void onBrowseOutput() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE,
                view.state().hardMode ? "PiT_hard_mode.nds" : "PiT_prepared.nds");
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
            /* Empty means "not chosen yet"; the view supplies its own hint. */
            view.setPaths(name, destination == null ? "" : displayName(destination));
            view.addLog(LOG_INFO, "Source: " + name);
        } else if (request == REQ_CREATE_ROM) {
            destination = uri;
            view.setPaths(source == null ? "" : displayName(source), name);
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

    private void log(final int level, final String message) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (!isFinishing()) {
                    view.addLog(level, message);
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
                    view.addLog(LOG_INFO, message);
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
                    view.setFailed(true);
                    view.addLog(LOG_ERROR, message);
                }
            }
        });
    }

    private void runPatch() {
        cancelled = false;
        view.clearLog();
        view.setBusy(true);

        final boolean applyPlan = view.state().hardMode;

        new Thread(new Runnable() {
            @Override
            public void run() {
                Patcher.Result result = new Patcher.Result();
                byte[] rom = null;
                byte[] out = null;
                int romLen = 0;

                try {
                    rom = readAll(source, PatchData.ROM_SIZE + 1);
                    if (rom == null) {
                        result.code = Patcher.ERR_READ;
                    } else {
                        romLen = rom.length;
                        out = new byte[rom.length];
                        Patcher.Listener listener = new Patcher.Listener() {
                            @Override
                            public void onStep(int step, String message, double fraction) {
                                progress(step, message, fraction);
                            }
                        };
                        result = Patcher.run(rom, out, applyPlan, listener);
                        if (result.code != Patcher.OK) {
                            out = null;
                        }
                    }

                    if (result.code == Patcher.OK) {
                        log(LOG_INFO, "Writing output...");
                        if (!writeAll(destination, out)) {
                            result.code = Patcher.ERR_WRITE;
                            out = null;
                        }
                    }

                    if (result.code == Patcher.OK) {
                        log(LOG_INFO, "Probing your ROM for the decomp...");
                        Patcher.Probe probe = Patcher.probe(rom);
                        if (probe.bannerTitleOk) {
                            log(LOG_INFO, String.format(
                                    "Banner: v%d, CRC-16 %04X, title \"%s\".",
                                    probe.bannerVersion, probe.bannerCrc16,
                                    probe.bannerTitle));
                        } else {
                            log(LOG_INFO, "Banner: none present in this ROM.");
                        }
                        log(LOG_INFO, "NitroFS: " + probe.archiveFiles + " archive files.");
                        if (probe.sdatPresent) {
                            log(LOG_INFO, String.format(
                                    "sound_data.sdat: SDAT v%d.%d, %d sections.",
                                    probe.sdatVersionMajor, probe.sdatVersionMinor,
                                    probe.sdatSections));
                        } else {
                            log(LOG_INFO, "sound_data.sdat: not found in this ROM.");
                        }
                    }
                } catch (OutOfMemoryError e) {
                    /*
                     * A 128 MiB working set needs a large heap. Saying so is far
                     * more useful than a crash dialog, and the device is
                     * otherwise fine.
                     */
                    result.code = Patcher.ERR_READ;
                    log(LOG_WARN, "Not enough memory: 128 MiB is required.");
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
                    if (applyPlan) {
                        log(LOG_OK, String.format(
                                "Patched %d records, %d fields. Header CRC-16 %04X.",
                                result.recordsPatched, result.fieldsWritten, result.headerCrc));
                    } else {
                        log(LOG_OK, String.format(
                                "Prepared %d-byte copy; header CRC-16 %04X.",
                                romLen, result.headerCrc));
                    }
                }
                finishWith(result.code, Patcher.resultText(result.code));
            }
        }, "pit-patch").start();
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
