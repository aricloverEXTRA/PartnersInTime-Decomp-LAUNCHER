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
 * <p>Usage: {@code PitSelfTest <rom.nds> <out.nds>}
 */
public final class PitSelfTest {

    private PitSelfTest() {
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.err.println("usage: PitSelfTest <rom.nds> <out.nds>");
            System.exit(2);
        }

        byte[] rom = readAll(args[0]);
        byte[] out = new byte[rom.length];

        Patcher.Result result = Patcher.run(rom, out, new Patcher.Listener() {
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
