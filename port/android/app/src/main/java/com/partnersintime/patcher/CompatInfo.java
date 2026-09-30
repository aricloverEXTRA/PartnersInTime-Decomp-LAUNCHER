package com.partnersintime.patcher;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;

/**
 * Reads what the device can do and turns it into log lines.
 *
 * <p>The patcher itself has no real requirement: it reads one 64 MiB ROM,
 * writes one output document, and draws a 480 x 320 screen, so any device
 * that can run this APK can patch. Whether the patched game is playable is a
 * different question: the result is a Nintendo DS game, so it needs real DS
 * hardware or an NDS emulator. Emulators are expensive in CPU and RAM, which
 * is what this report measures, and the answer is a rough tier rather than a
 * promise.
 */
public final class CompatInfo {

    private CompatInfo() {
    }

    /** A line of the on-screen report, safe to call from the main thread. */
    public static String[] report(Context context) {
        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "unknown";
        int cores = Runtime.getRuntime().availableProcessors();
        long ramBytes = totalRamBytes(context);
        String ramText = ramBytes > 0
                ? String.format(java.util.Locale.US, "%.1f GiB", ramBytes / (1024.0 * 1024 * 1024))
                : "unknown";
        long heapMiB = Runtime.getRuntime().maxMemory() / (1024 * 1024);

        return new String[] {
                "Device: " + Build.MANUFACTURER + " " + Build.MODEL,
                "OS: Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")",
                "CPU: " + cores + " cores, " + abi,
                "RAM: " + ramText + ", heap " + heapMiB + " MiB",
                "Screen: " + screenDp(context) + " dp",
                "Patcher: any device that can run this APK.",
                "NDS emulation tier: " + emulationTier(cores, ramBytes, abi),
        };
    }

    private static long totalRamBytes(Context context) {
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        if (am == null) {
            return 0;
        }
        ActivityManager.MemoryInfo info = new ActivityManager.MemoryInfo();
        am.getMemoryInfo(info);
        return info.totalMem;
    }

    private static String screenDp(Context context) {
        return context.getResources().getConfiguration().screenWidthDp
                + "x" + context.getResources().getConfiguration().screenHeightDp;
    }

    /**
     * The rough position on the emulator spectrum. 64-bit CPUs and more than a
     * gigabyte of RAM are the two things an emulator actually spends; the tier
     * is deliberately coarse.
     */
    private static String emulationTier(int cores, long ramBytes, String abi) {
        long ramGiB = ramBytes >> 30;
        boolean fast = abi.startsWith("arm64") || abi.startsWith("aarch64")
                || abi.startsWith("x86_64");
        if (fast && Build.VERSION.SDK_INT >= 24 && cores >= 4 && ramGiB >= 2) {
            return "A - comfortable for melonDS at DS resolution";
        }
        if (fast && ramGiB >= 1) {
            return "B - emulation works, lighter games are safer";
        }
        return "C - the patcher is fine, emulation will struggle";
    }
}
