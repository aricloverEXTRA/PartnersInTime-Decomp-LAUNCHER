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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

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

    /*
     * The shipped mod, bundled as an asset at mods/<id>/profile.json. It is
     * copied into app-private storage on first run so the same mods/<id>
     * directory layout works on Android and on the desktop launcher, and so a
     * future import path only has to write another folder next to it.
     */
    private static final String[] BUNDLED_MODS = { "hard_mode" };

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
        loadMods();
        view.addLog(LOG_INFO, "Select your EUR ROM, then choose an output name.");
        view.addLog(LOG_INFO, "MODS are optional: pick one on the MODS tab.");
        view.addLog(LOG_INFO, "TIP: Tab / D-pad moves focus; Enter activates.");
        for (String line : CompatInfo.report(this)) {
            view.addLog(LOG_INFO, line);
        }
    }

    /**
     * Installs the bundled profiles, then scans the mods directory.
     *
     * <p>Each profile is rewritten every launch so a repaired or updated profile
     * replaces an older copy, and a failed install is reported instead of being
     * hidden: the built-in Hard Mode plan stays available either way.
     */
    private void loadMods() {
        File root = new File(getFilesDir(), "mods");
        int installed = 0;

        for (String id : BUNDLED_MODS) {
            try {
                installBundledMod(root, id);
                installed++;
            } catch (IOException e) {
                view.addLog(LOG_WARN, "Bundled mod " + id + " was not installed: "
                        + e.getMessage());
            }
        }

        ModProfile.Catalog mods = ModProfile.Catalog.scan(root);

        view.setMods(mods);

        if (mods.skipped > 0) {
            view.addLog(LOG_WARN, "Skipped " + mods.skipped + " mod folder"
                    + (mods.skipped == 1 ? "" : "s") + (mods.error == null ? "."
                            : ": " + mods.error));
        }
        view.addLog(LOG_INFO, installed + " bundled mod"
                + (installed == 1 ? "" : "s") + ", " + mods.count()
                + " profile" + (mods.count() == 1 ? "" : "s") + " found.");
    }

    private void installBundledMod(File root, String id) throws IOException {
        File dir = new File(root, id);
        File out = new File(dir, "profile.json");
        InputStream in = getAssets().open("mods/" + id + "/profile.json");

        try {
            if (!dir.isDirectory() && !dir.mkdirs()) {
                throw new IOException("cannot create the " + id + " folder");
            }
            /*
             * Write to a temporary name and move it into place, so a killed
             * process cannot leave a half-written profile that the scan would
             * then reject. The old copy is deleted first because
             * File.renameTo only replaces an existing file on some platforms.
             * A crash in that window leaves no profile rather than a partial
             * one, and the next launch reinstalls it.
             */
            File temp = new File(dir, "profile.json.part");
            OutputStream stream = new java.io.FileOutputStream(temp);

            try {
                byte[] chunk = new byte[8192];
                int got;

                while ((got = in.read(chunk)) > 0) {
                    stream.write(chunk, 0, got);
                }
                stream.flush();
            } finally {
                stream.close();
            }
            if (out.exists() && !out.delete()) {
                throw new IOException("cannot replace the existing " + id + " profile");
            }
            if (!temp.renameTo(out)) {
                throw new IOException("cannot install the " + id + " profile");
            }
        } finally {
            in.close();
        }
    }

    @Override
    public void onTab(int tab) {
        view.setTab(tab);
        view.addLog(LOG_INFO, tab == PatcherView.TAB_MODS ? "MODS tab."
                : (tab == PatcherView.TAB_ABOUT ? "ABOUT tab." : "ROM tab."));
    }

    private String modId(int index) {
        return index == 0 ? "" : (index == 1 ? PatchData.PLAN_ID
                : view.state().mods.at(index - 2).id);
    }

    /**
     * Suggested output name for the selected mod.
     *
     * <p>A profile id is free-form text, so it is reduced to filename-safe
     * characters before it reaches the system file picker; anything unusable
     * falls back to a generic name rather than producing a name the user cannot
     * type. The character set matches set_default_output in the C launcher, so
     * the same profile suggests the same name on both platforms.
     */
    private String suggestedName() {
        String id = modId(view.state().modIndex);
        StringBuilder safe = new StringBuilder(id.length());

        for (int i = 0; i < id.length() && safe.length() < 24; i++) {
            char ch = id.charAt(i);

            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                    || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
                safe.append(ch);
            }
        }
        return safe.length() == 0 ? "PiT_prepared.nds" : "PiT_" + safe + ".nds";
    }

    @Override
    protected void onDestroy() {
        cancelled = true;
        super.onDestroy();
    }

    /*
     * Android can kill a backgrounded process at any time, which would otherwise
     * silently discard the two picked documents and the selected mod. A desktop
     * window is not destroyed behind the user's back, so this has no counterpart
     * to mirror on Windows; saving the small amount of state that describes the
     * session is the platform-native answer.
     *
     * A URI is only restored if the provider still grants access, so a document
     * whose grant did not persist is reported rather than failing later with a
     * SecurityException mid-patch.
     */
    @Override
    protected void onSaveInstanceState(Bundle out) {
        super.onSaveInstanceState(out);

        out.putInt("tab", view.state().tab);
        out.putInt("mod", view.state().modIndex);
        out.putString("source", source == null ? null : source.toString());
        out.putString("destination", destination == null ? null
                : destination.toString());
    }

    @Override
    protected void onRestoreInstanceState(Bundle saved) {
        super.onRestoreInstanceState(saved);

        view.setTab(saved.getInt("tab", PatcherView.TAB_PATCH));
        view.setModIndex(saved.getInt("mod", 0));

        Uri restoredSource = restoreUri(saved.getString("source"));
        Uri restoredOutput = restoreUri(saved.getString("destination"));
        source = restoredSource;
        destination = restoredOutput;
        view.setPaths(restoredSource == null ? "" : displayName(restoredSource),
                restoredOutput == null ? "" : displayName(restoredOutput));

        if (restoredSource != null) {
            view.addLog(LOG_INFO, "Restored source: " + displayName(restoredSource));
        }
        if (restoredOutput != null) {
            view.addLog(LOG_INFO, "Restored output: " + displayName(restoredOutput));
        }
        if (source == null || destination == null) {
            view.addLog(LOG_WARN, "Android reclaimed one of the picked files. "
                    + "Choose it again to patch.");
        }
    }

    /**
     * Rebuilds a URI and confirms the app may still read it.
     *
     * <p>{@code takePersistableUriPermission} is requested when a document is
     * first chosen so the grant outlives the process. Providers are not required
     * to support that, so a provider that refuses simply leaves the URI
     * unrestored and the user picks the file again.
     */
    private Uri restoreUri(String text) {
        if (text == null) {
            return null;
        }
        Uri uri = Uri.parse(text);

        try {
            getContentResolver().openInputStream(uri).close();
        } catch (Exception e) {
            view.addLog(LOG_WARN, "Cannot reopen " + displayName(uri)
                    + "; choose it again.");
            return null;
        }
        return uri;
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
        intent.putExtra(Intent.EXTRA_TITLE, suggestedName());
        startActivityForResult(intent, REQ_CREATE_ROM);
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (result != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri uri = data.getData();
        String name = displayName(uri);

        /*
         * Hold the grant across a process death. Without this, a document
         * chosen now becomes unreadable if Android reclaims the activity while
         * the app is in the background, and the user has to pick it again.
         * Providers are not obliged to support it, so failure here is not
         * fatal; onSaveInstanceState simply will not restore that document.
         */
        try {
            int flags = data.getFlags()
                    & (Intent.FLAG_GRANT_READ_URI_PERMISSION
                       | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            if (flags != 0) {
                getContentResolver().takePersistableUriPermission(uri, flags);
            }
        } catch (Exception e) {
            /* Not every provider offers a persistable grant; carry on. */
        }

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

        /*
         * Row 0 is no mod, row 1 the compiled-in plan (a null profile selects
         * exactly that) and row 2+ the profile the user picked.
         */
        final int modIndex = view.state().modIndex;
        final boolean applyPlan = modIndex != 0;
        final ModProfile profile = modIndex >= 2 ? view.state().mods.at(modIndex - 2)
                : null;

        new Thread(new Runnable() {
            @Override
            public void run() {
                Patcher.Result result = new Patcher.Result();
                byte[] rom = null;
                int romLen = 0;
                Patcher.Listener listener = new Patcher.Listener() {
                    @Override
                    public void onStep(int step, String message, double fraction) {
                        progress(step, message, fraction);
                    }
                };

                try {
                    rom = readRom(source);
                    if (rom == null) {
                        result.code = Patcher.ERR_READ;
                    } else {
                        romLen = rom.length;

                        /*
                         * The probe describes the cartridge that was handed in,
                         * so it runs before the patch. The patch rewrites the
                         * header CRC-16, which the probe reports, and reading it
                         * afterwards would describe the output instead.
                         */
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

                        /*
                         * Patched in place. Patcher.run verifies the size,
                         * SHA-1, header and NitroFS tables before it copies, and
                         * then works only on the destination buffer, so handing
                         * it the same array twice is a self-copy and halves the
                         * memory a 64 MiB cartridge needs.
                         */
                        result = Patcher.run(rom, rom, applyPlan, profile, listener);
                        if (result.code != Patcher.OK) {
                            rom = null;
                        }
                    }

                    if (result.code == Patcher.OK) {
                        log(LOG_INFO, "Writing output...");
                        if (!writeAll(destination, rom)) {
                            result.code = Patcher.ERR_WRITE;
                            rom = null;
                        }
                    }
                } catch (OutOfMemoryError e) {
                    /*
                     * One 64 MiB buffer is all a patch needs. Saying so is far
                     * more useful than a crash dialog, and the device is
                     * otherwise fine.
                     */
                    result.code = Patcher.ERR_READ;
                    log(LOG_WARN, "Not enough memory: 64 MiB is required.");
                    rom = null;
                } catch (Patcher.WrongSize e) {
                    /*
                     * A single buffer means the wrong size is detected before
                     * Patcher.run ever sees the bytes, so the count it would
                     * normally report has to be reported here.
                     */
                    result.code = Patcher.ERR_SIZE;
                    log(LOG_WARN, "Expected " + PatchData.ROM_SIZE
                            + " bytes, found " + e.found + ".");
                    rom = null;
                } catch (Exception e) {
                    result.code = Patcher.ERR_READ;
                    log(LOG_ERROR, e.getClass().getSimpleName() + ": " + e.getMessage());
                    rom = null;
                } finally {
                    rom = null;
                }

                if (cancelled) {
                    return;
                }
                if (result.code == Patcher.OK) {
                    rom = null;
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

    /**
     * Opens the picked document and reads it into a single buffer.
     *
     * <p>The sizing rule lives in {@link Patcher#readCartridge} so it can be
     * tested on a desktop JVM; this method only deals with Storage Access
     * Framework plumbing and leaves the stream's lifetime to the caller.
     *
     * @return the cartridge bytes, or null if the document could not be opened
     * @throws Patcher.WrongSize if the document is not exactly one
     *         supported-size cartridge, carrying the size actually found
     */
    private byte[] readRom(Uri uri) throws Exception {
        java.io.InputStream in = getContentResolver().openInputStream(uri);
        if (in == null) {
            return null;
        }
        try {
            return Patcher.readCartridge(in);
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
