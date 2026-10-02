package com.partnersintime.patcher;

/**
 * Command-line harness that runs the Android patch pipeline outside Android.
 *
 * <p>The patcher itself has no Android dependencies, so it can be exercised on a
 * desktop JVM against a real ROM. That is what makes the Java implementation
 * testable at all: a class-path test of {@link Patcher}, {@link NitroFs} and
 * {@link Patcher#headerCrc16} against the same cartridge and the same expected
 * output as the C build, which is how a silent drift between the two would be
 * caught.
 *
 * <p>Usage: {@code PitSelfTest [--no-mods] [--in-place] [--mods-dir <dir>]
 * [--mod <id>] <rom.nds> <out.nds>}
 */
public final class PitSelfTest {

    private static final String USAGE =
            "usage: PitSelfTest [--no-mods] [--in-place] [--mods-dir <dir>] [--mod <id>] "
                    + "<rom.nds> <out.nds>";

    private PitSelfTest() {
    }

    public static void main(String[] args) throws Exception {
        boolean applyPlan = true;
        String modsDir = "mods";
        String modId = null;
        boolean listRequested = false;
        boolean inPlace = false;
        while (args.length > 0 && args[0].startsWith("--")) {
            if (args[0].equals("--no-mods")) {
                applyPlan = false;
            } else if (args[0].equals("--in-place")) {
                inPlace = true;
            } else if (args[0].equals("--list-mods")) {
                /*
                 * Recorded rather than acted on, so "--mods-dir" is still
                 * honoured when it follows this flag, as in the C launcher.
                 */
                listRequested = true;
            } else if (args[0].equals("--mods-dir") && args.length > 1) {
                modsDir = args[1];
                args = drop(args, 2);
                continue;
            } else if (args[0].equals("--mod") && args.length > 1) {
                modId = args[1];
                args = drop(args, 2);
                continue;
            } else {
                System.err.println(USAGE);
                System.exit(2);
            }
            args = drop(args, 1);
        }
        if (listRequested) {
            System.exit(listMods(new java.io.File(modsDir)));
        }
        if (args.length != 2) {
            System.err.println(USAGE);
            System.exit(2);
        }

        ModProfile profile = null;
        if (modId != null) {
            ModProfile.Catalog catalog = ModProfile.Catalog.scan(new java.io.File(modsDir));

            profile = catalog.find(modId);
            if (profile == null) {
                System.err.println("no usable mod with id '" + modId + "' under " + modsDir);
                if (catalog.count() > 0) {
                    StringBuilder available = new StringBuilder("available:");
                    for (int i = 0; i < catalog.count(); i++) {
                        available.append(' ').append(catalog.at(i).id);
                    }
                    System.err.println(available);
                }
                System.exit(2);
            }
            System.out.println("mod profile : " + profile + ", " + profile.transformCount
                    + " transforms");
        }

        byte[] rom = readAll(args[0]);
        /*
         * The Android app hands Patcher.run the same array twice, because a
         * 64 MiB cartridge cannot afford a second buffer on a phone. That is
         * only sound because Patcher.run verifies size, SHA-1, header and the
         * NitroFS tables before it copies, and afterwards works solely on the
         * destination. --in-place exercises that path here so the assumption is
         * tested rather than assumed.
         */
        byte[] out = inPlace ? rom : new byte[rom.length];

        Patcher.Result result = Patcher.run(rom, out, applyPlan, profile,
                new Patcher.Listener() {
            @Override
            public void onStep(int step, String message, double fraction) {
                System.out.printf("  [%d/%d] %5.1f%%  %s%n",
                        step, Patcher.STEPS, fraction * 100.0, message);
            }
        });

        System.out.println();
        System.out.println("result      : " + Patcher.resultText(result.code)
                + " (code " + result.code + ")");
        System.out.println("sha1        : " + result.inputSha1);
        System.out.println("plan applied: " + applyPlan);
        System.out.println("mod profile : "
                + (profile == null ? "built-in " + PatchData.PLAN_ID : profile.id));
        if (result.targetOffset != 0 || result.targetSize != 0) {
            System.out.printf("target      : id %d at 0x%08X, %d bytes%n",
                    result.targetId, result.targetOffset, result.targetSize);
        }
        System.out.println("records     : " + result.recordsPatched);
        System.out.println("fields      : " + result.fieldsWritten);
        System.out.printf("header crc  : %04X%n", result.headerCrc);

        if (result.code != Patcher.OK) {
            System.exit(1);
        }
        writeAll(args[1], out);
        System.out.println("wrote       : " + args[1] + " (" + out.length + " bytes)");
    }

    private static String[] drop(String[] args, int count) {
        String[] rest = new String[args.length - count];
        System.arraycopy(args, count, rest, 0, rest.length);
        return rest;
    }

    private static int listMods(java.io.File root) {
        ModProfile.Catalog catalog = ModProfile.Catalog.scan(root);

        for (int i = 0; i < catalog.count(); i++) {
            ModProfile profile = catalog.at(i);

            System.out.println(profile.name + "  (" + profile.id
                    + (profile.version.isEmpty() ? "" : ", v" + profile.version)
                    + ", " + profile.transformCount + " transforms)");
            for (int j = 0; j < profile.transformCount; j++) {
                ModProfile.Transform t = profile.transforms[j];
                System.out.printf("    %-12s %s [%d..%d]%n", t.field, t.scaleText(),
                        t.minValue, t.maxValue);
            }
        }
        if (catalog.skipped > 0) {
            System.out.println(catalog.skipped + " profile(s) skipped; last reason: "
                    + catalog.error);
        }
        return 0;
    }

    private static byte[] readAll(String path) throws Exception {
        java.io.File file = new java.io.File(path);
        if (!file.isFile()) {
            throw new java.io.FileNotFoundException(path);
        }
        byte[] data = new byte[(int) file.length()];
        java.io.DataInputStream in =
                new java.io.DataInputStream(new java.io.BufferedInputStream(
                        new java.io.FileInputStream(file)));
        try {
            in.readFully(data);
        } finally {
            in.close();
        }
        return data;
    }

    private static void writeAll(String path, byte[] data) throws Exception {
        java.io.OutputStream out = new java.io.BufferedOutputStream(
                new java.io.FileOutputStream(path));
        try {
            out.write(data);
        } finally {
            out.close();
        }
    }
}
