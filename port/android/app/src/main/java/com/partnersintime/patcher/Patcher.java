package com.partnersintime.patcher;

/**
 * The patch pipeline, matching src/core/pit_patcher.c step for step.
 *
 * <p>Scales are applied as exact integer rationals and rounded half-up, so this
 * writes the same bytes as the C build and as the Python profile pipeline for
 * every input. Nothing here reads or embeds retail data: the record layout comes
 * from the decompiled struct and every stat value is read from the user's ROM.
 */
public final class Patcher {

    public static final int STEPS = 9;

    public static final int OK = 0;
    public static final int ERR_ARGS = 1;
    public static final int ERR_READ = 2;
    public static final int ERR_SIZE = 3;
    public static final int ERR_SHA1 = 4;
    public static final int ERR_HEADER = 5;
    public static final int ERR_CRC = 6;
    public static final int ERR_TARGET = 7;
    public static final int ERR_TARGET_SIZE = 8;
    public static final int ERR_WRITE = 9;

    private static final int HEADER_CRC_OFFSET = 0x15E;
    private static final int HEADER_CRC_SPAN = 0x15E;

    /** Progress callback, invoked on the worker thread. */
    public interface Listener {
        void onStep(int step, String message, double fraction);
    }

    public static final class Result {
        public int code = OK;
        public int targetOffset;
        public int targetSize;
        public int targetId = -1;
        public int recordsPatched;
        public int fieldsWritten;
        public int headerCrc;
        public String inputSha1 = "";
    }

    public static String resultText(int code) {
        switch (code) {
            case OK:                return "Patched ROM written.";
            case ERR_ARGS:          return "No source ROM was selected.";
            case ERR_READ:          return "The source ROM could not be read.";
            case ERR_SIZE:          return "That file is not a 64 MiB NDS ROM.";
            case ERR_SHA1:          return "Unsupported ROM: SHA-1 does not match the EUR release.";
            case ERR_HEADER:        return "Unsupported ROM: title or game code mismatch.";
            case ERR_CRC:           return "Cartridge header CRC-16 does not verify.";
            case ERR_TARGET:        return "Target file is missing from this ROM's NitroFS.";
            case ERR_TARGET_SIZE:   return "Target file is not the expected record table.";
            case ERR_WRITE:         return "The patched ROM could not be written.";
            default:                return "Unknown error.";
        }
    }

    /**
     * CRC-16/MODBUS over the first 0x15E header bytes.
     *
     * <p>The seed is 0xFFFF rather than the 0 the DS documentation describes:
     * the EUR release stores 0xD0BC, which only the 0xFFFF seed reproduces.
     */
    public static int headerCrc16(byte[] data, int offset, int length) {
        int crc = 0xFFFF;
        int span = Math.min(length, HEADER_CRC_SPAN);

        for (int i = 0; i < span; i++) {
            crc ^= (data[offset + i] & 0xFF);
            for (int bit = 0; bit < 8; bit++) {
                if ((crc & 1) != 0) {
                    crc = (crc >>> 1) ^ 0xA001;
                } else {
                    crc >>>= 1;
                }
            }
        }
        return crc & 0xFFFF;
    }

    /** Half-up scaling on exact integer rationals, matching scale_value() in C. */
    static int scaleValue(int value, int num, int den, int minValue, int maxValue) {
        long scaled = ((long) value * num + (den / 2)) / den;

        if (scaled < minValue) {
            return minValue;
        }
        if (scaled > maxValue) {
            return maxValue;
        }
        return (int) scaled;
    }

    private static void readU16(byte[] data, int offset, int[] out) {
        out[0] = (data[offset] & 0xFF) | ((data[offset + 1] & 0xFF) << 8);
    }

    private static void writeU16(byte[] data, int offset, int value) {
        data[offset] = (byte) (value & 0xFF);
        data[offset + 1] = (byte) ((value >>> 8) & 0xFF);
    }

    private static void step(Listener listener, int step, String message, double fraction) {
        if (listener != null) {
            listener.onStep(step, message, fraction);
        }
    }

    private static boolean matches(byte[] data, int offset, String ascii) {
        for (int i = 0; i < ascii.length(); i++) {
            if ((data[offset + i] & 0xFF) != ascii.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Runs the full pipeline over the user's ROM bytes and returns the patched
     * image. The caller writes {@code out} so this stays off the main thread
     * and works with any storage the platform hands back.
     */
    public static Result run(byte[] rom, byte[] out, Listener listener) {
        Result result = new Result();

        if (rom == null || out == null || out.length < rom.length) {
            result.code = ERR_ARGS;
            return result;
        }

        step(listener, 1, "Reading source ROM...", 0.02);

        if (rom.length != PatchData.ROM_SIZE) {
            step(listener, 2, "Expected " + PatchData.ROM_SIZE + " bytes, found "
                    + rom.length + ".", 0.08);
            result.code = ERR_SIZE;
            return result;
        }
        step(listener, 2, "Cartridge size is 64 MiB.", 0.12);

        step(listener, 3, "Computing SHA-1...", 0.18);
        String actual = Sha1.hex(Sha1.digest(rom));
        result.inputSha1 = actual;
        if (!actual.equals(PatchData.ROM_SHA1)) {
            step(listener, 3, "Expected " + PatchData.ROM_SHA1 + ", got " + actual + ".",
                    0.18);
            result.code = ERR_SHA1;
            return result;
        }
        step(listener, 3, "SHA-1 matches the supported EUR release.", 0.26);

        step(listener, 4, "Reading cartridge header...", 0.32);
        if (!matches(rom, 0, PatchData.ROM_TITLE) || !matches(rom, 12, PatchData.ROM_GAME_CODE)) {
            step(listener, 4, "Cartridge title or game code mismatch.", 0.32);
            result.code = ERR_HEADER;
            return result;
        }
        step(listener, 4, "Title " + PatchData.ROM_TITLE + " (" + PatchData.ROM_GAME_CODE
                + ") confirmed.", 0.38);

        step(listener, 5, "Verifying header CRC-16...", 0.44);
        int[] stored = new int[1];
        readU16(rom, HEADER_CRC_OFFSET, stored);
        int computed = headerCrc16(rom, 0, rom.length);
        if (stored[0] != computed) {
            step(listener, 5, String.format("Header CRC mismatch: stored %04X, computed %04X.",
                    stored[0], computed), 0.44);
            result.code = ERR_CRC;
            return result;
        }
        step(listener, 5, String.format("Header CRC-16 %04X verified.", stored[0]), 0.48);

        step(listener, 6, "Scanning NitroFS for the stat table...", 0.52);
        NitroFs.Entry target = NitroFs.find(rom, PatchData.TARGET_PATH);
        if (target == null) {
            step(listener, 6, PatchData.TARGET_PATH + " is not present in this ROM.", 0.52);
            result.code = ERR_TARGET;
            return result;
        }
        int targetOffset = target.offset;
        int targetSize = target.size;
        int expectedSize = PatchData.RECORD_COUNT * PatchData.RECORD_SIZE;
        if (targetSize != expectedSize) {
            step(listener, 6, PatchData.TARGET_PATH + " is " + targetSize
                    + " bytes, expected " + expectedSize + ".", 0.52);
            result.code = ERR_TARGET_SIZE;
            return result;
        }
        result.targetOffset = targetOffset;
        result.targetSize = targetSize;
        result.targetId = target.id;
        step(listener, 6, String.format("Found %s at 0x%08X (%d records x %d bytes).",
                PatchData.TARGET_PATH, targetOffset, PatchData.RECORD_COUNT,
                PatchData.RECORD_SIZE), 0.58);

        step(listener, 7, "Applying plan to a copy...", 0.64);
        System.arraycopy(rom, 0, out, 0, rom.length);

        int written = 0;
        int[] value = new int[1];
        for (int record = 0; record < PatchData.RECORD_COUNT; record++) {
            int base = targetOffset + record * PatchData.RECORD_SIZE;

            for (int t = 0; t < PatchData.TRANSFORM_NUM.length; t++) {
                int field = PatchData.TRANSFORM_FIELD[t];

                readU16(out, base + PatchData.FIELD_OFFSETS[field], value);
                int scaled = scaleValue(value[0],
                        PatchData.TRANSFORM_NUM[t], PatchData.TRANSFORM_DEN[t],
                        PatchData.TRANSFORM_MIN[t], PatchData.TRANSFORM_MAX[t]);
                writeU16(out, base + PatchData.FIELD_OFFSETS[field], scaled);
                written++;
            }

            if (record % 8 == 0 || record + 1 == PatchData.RECORD_COUNT) {
                step(listener, 7, "Patched record " + (record + 1) + "/"
                        + PatchData.RECORD_COUNT + "...", 0.64 + 0.24
                        * (double) (record + 1) / PatchData.RECORD_COUNT);
            }
        }
        result.recordsPatched = PatchData.RECORD_COUNT;
        result.fieldsWritten = written;
        step(listener, 7, "All " + PatchData.RECORD_COUNT + " records patched.", 0.88);

        step(listener, 8, "Recomputing header CRC-16...", 0.92);
        int crc = headerCrc16(out, 0, out.length);
        writeU16(out, HEADER_CRC_OFFSET, crc);
        result.headerCrc = crc;
        step(listener, 8, String.format("Header CRC-16 set to %04X.", crc), 0.94);

        return result;
    }
}
