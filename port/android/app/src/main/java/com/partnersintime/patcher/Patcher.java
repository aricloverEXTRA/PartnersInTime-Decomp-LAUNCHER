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
    public static final int ERR_PLAN = 10;

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
            case OK:                return "ROM verified; copy written.";
            case ERR_ARGS:          return "No source ROM was selected.";
            case ERR_READ:          return "The source ROM could not be read.";
            case ERR_SIZE:          return "That file is not a 64 MiB NDS ROM.";
            case ERR_SHA1:          return "Unsupported ROM: SHA-1 does not match the EUR release.";
            case ERR_HEADER:        return "Unsupported ROM: title or game code mismatch.";
            case ERR_CRC:           return "Cartridge header CRC-16 does not verify.";
            case ERR_TARGET:        return "Target file is missing from this ROM's NitroFS.";
            case ERR_TARGET_SIZE:   return "Target file is not the expected record table.";
            case ERR_WRITE:         return "The copy could not be written.";
            case ERR_PLAN:          return "That mod profile changes no field this launcher knows.";
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

    private static void readU32(byte[] data, int offset, int[] out) {
        out[0] = (data[offset] & 0xFF) | ((data[offset + 1] & 0xFF) << 8)
                | ((data[offset + 2] & 0xFF) << 16)
                | ((data[offset + 3] & 0xFF) << 24);
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
     * and works with any storage the platform hands back. When {@code applyPlan}
     * is false the ROM is verified and copied with only the header CRC repaired,
     * matching the C {@code --no-mods} path, so the data mod stays optional.
     */
    public static Result run(byte[] rom, byte[] out, boolean applyPlan, Listener listener) {
        return run(rom, out, applyPlan, null, listener);
    }

    /**
     * Runs the pipeline applying {@code profile} instead of the built-in plan.
     * A null profile selects the built-in Hard Mode table, so the shipped path is
     * unchanged. Transform names are resolved against PatchData.FIELD_NAMES at
     * apply time: a profile that names a field the launcher does not own simply
     * skips that transform, and a profile with no usable transform is refused
     * rather than writing a copy that only looks patched.
     */
    public static Result run(byte[] rom, byte[] out, boolean applyPlan,
                             ModProfile profile, Listener listener) {
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

        /*
         * Sized for the largest profile the reader accepts rather than for the
         * built-in table, so resolving one cannot run out of room and silently
         * drop the tail. In practice at most six can resolve: the reader refuses
         * a repeated field and only six fields are known.
         */
        int capacity = profile == null ? PatchData.TRANSFORM_NUM.length
                : ModProfile.MAX_TRANSFORMS;
        int[] fieldOf = new int[capacity];
        long[] numOf = new long[capacity];
        long[] denOf = new long[capacity];
        int[] minOf = new int[capacity];
        int[] maxOf = new int[capacity];
        int planCount = PatchData.TRANSFORM_NUM.length;

        if (applyPlan) {
            if (profile == null) {
                for (int i = 0; i < planCount; i++) {
                    fieldOf[i] = PatchData.TRANSFORM_FIELD[i];
                    numOf[i] = PatchData.TRANSFORM_NUM[i];
                    denOf[i] = PatchData.TRANSFORM_DEN[i];
                    minOf[i] = PatchData.TRANSFORM_MIN[i];
                    maxOf[i] = PatchData.TRANSFORM_MAX[i];
                }
            } else {
                planCount = 0;
                for (int i = 0; i < profile.transformCount; i++) {
                    ModProfile.Transform t = profile.transforms[i];
                    int field = ModProfile.fieldIndex(t.field);

                    if (!t.enabled || field < 0) {
                        continue;
                    }
                    fieldOf[planCount] = field;
                    numOf[planCount] = t.num;
                    denOf[planCount] = t.den;
                    minOf[planCount] = t.minValue;
                    maxOf[planCount] = t.maxValue;
                    planCount++;
                }
                if (planCount == 0) {
                    step(listener, 7, "No transform in this profile matches a known field...", 0.64);
                    result.code = ERR_PLAN;
                    return result;
                }
            }
        }

        step(listener, 7, applyPlan
                ? "Applying " + (profile == null ? PatchData.PLAN_NAME : profile.name)
                        + " to a copy..."
                : "Copying ROM; plan is OFF...", 0.64);
        System.arraycopy(rom, 0, out, 0, rom.length);

        int written = 0;
        if (applyPlan) {
            int[] value = new int[1];
            for (int record = 0; record < PatchData.RECORD_COUNT; record++) {
                int base = targetOffset + record * PatchData.RECORD_SIZE;

                for (int t = 0; t < planCount; t++) {
                    readU16(out, base + PatchData.FIELD_OFFSETS[fieldOf[t]], value);
                    int scaled = scaleValue(value[0], (int) numOf[t], (int) denOf[t],
                            minOf[t], maxOf[t]);
                    writeU16(out, base + PatchData.FIELD_OFFSETS[fieldOf[t]], scaled);
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
        } else {
            result.recordsPatched = 0;
            result.fieldsWritten = 0;
            step(listener, 7, "Plan skipped; the copy is unchanged.", 0.88);
        }

        step(listener, 8, "Recomputing header CRC-16...", 0.92);
        int crc = headerCrc16(out, 0, out.length);
        writeU16(out, HEADER_CRC_OFFSET, crc);
        result.headerCrc = crc;
        step(listener, 8, String.format("Header CRC-16 set to %04X.", crc), 0.94);

        return result;
    }

    /** Read-only snapshot of the user's cartridge, derived from its own bytes. */
    public static final class Probe {
        public boolean bannerPresent;
        public int bannerOffset;
        public int bannerVersion;
        public int bannerCrc16;
        public String bannerTitle = "";
        public boolean bannerTitleOk;
        public int archiveFiles;
        public boolean sdatPresent;
        public int sdatVersionMajor;
        public int sdatVersionMinor;
        public int sdatSections;
    }

    /**
     * Decodes the cartridge fields the decomp reads, mirroring the ingest banner
     * and the C SDAT probe. The banner header is version-first: {@code u16} version
     * at +0 then the stored {@code u16} crc16 at +2, matching pit_nds_banner.c.
     * The banner title is UTF-16LE with line breaks folded to spaces, capped at
     * 32 code units; a control character or a non-ASCII unit makes the title
     * unreadable. Sound is looked up under its real path
     * {@code Sound/sound_data.sdat}, with the SDAT version and section count read
     * little-endian like the C probe.
     */
    public static Probe probe(byte[] rom) {
        Probe p = new Probe();

        if (rom == null || rom.length < 0x140) {
            return p;
        }

        int[] u32 = new int[1];
        int[] u16 = new int[1];
        readU32(rom, 0x68, u32);
        int bannerOffset = u32[0];
        if (bannerOffset != 0 && bannerOffset + 2 <= rom.length) {
            readU16(rom, bannerOffset, u16);
            int version = u16[0];

            if (version >= 1 && version <= 3) {
                p.bannerPresent = true;
                p.bannerOffset = bannerOffset;
                p.bannerVersion = version;
                readU16(rom, bannerOffset + 2, u16);
                p.bannerCrc16 = u16[0];

                int titleAt = bannerOffset + (version == 1 ? 0x340 : 0x400);
                StringBuilder sb = new StringBuilder(32);
                boolean ok = true;

                for (int i = 0; i < 32 && titleAt + (long) i * 2 + 2 <= rom.length; i++) {
                    char lo = (char) (rom[titleAt + i * 2] & 0xFF);
                    char hi = (char) (rom[titleAt + i * 2 + 1] & 0xFF);

                    if (hi != 0 || lo == 0) {
                        break;
                    }
                    if (lo == '\r' || lo == '\n' || lo == '\t') {
                        sb.append(' ');
                        continue;
                    }
                    if (lo < 0x20 || lo >= 0x7F) {
                        ok = false;
                        break;
                    }
                    sb.append(lo);
                }
                p.bannerTitle = sb.toString();
                p.bannerTitleOk = ok && p.bannerTitle.length() > 0;
            }
        }

        readU32(rom, 0x40, u32);
        int fntOffset = u32[0];
        readU32(rom, 0x4C, u32);
        int fatSize = u32[0];
        int fatRecords = fatSize >= 8 ? fatSize / 8 : 0;

        if (fntOffset != 0 && fntOffset + 6 <= rom.length && fatRecords > 0) {
            readU16(rom, fntOffset + 4, u16);
            p.archiveFiles = Math.max(0, fatRecords - u16[0]);
        } else {
            p.archiveFiles = 0;
        }

        NitroFs.Entry sdat = NitroFs.find(rom, "Sound/sound_data.sdat");
        if (sdat != null) {
            p.sdatPresent = true;
            if ((long) sdat.offset + 0x10 <= rom.length) {
                readU16(rom, sdat.offset + 6, u16);
                int version = u16[0];

                p.sdatVersionMajor = (version >> 8) & 0xFF;
                p.sdatVersionMinor = version & 0xFF;
                readU16(rom, sdat.offset + 0x0E, u16);
                p.sdatSections = u16[0];
            }
        }
        return p;
    }

    /**
     * Raised when a stream is not exactly one cartridge long.
     *
     * <p>Reading into a single exactly-sized buffer detects a wrong length
     * before the bytes reach {@link #run}, so the count has to travel with the
     * failure instead of being reported by the patcher.
     */
    public static final class WrongSize extends Exception {
        private static final long serialVersionUID = 1L;
        /** Bytes actually present; for a larger stream this is a lower bound. */
        public final int found;

        WrongSize(int found) {
            this.found = found;
        }
    }

    /**
     * Reads a cartridge into one exactly-sized buffer.
     *
     * <p>Deliberately not built through a growable stream: filling one 64 MiB
     * cartridge that way needs a 64 MiB staging buffer plus a 64 MiB copy out of
     * it, which on a phone is often the difference between working and an
     * {@link OutOfMemoryError}. The buffer is allocated once, at its final
     * size, and filled in place.
     *
     * @return the cartridge bytes
     * @throws WrongSize if the stream is not exactly {@link PatchData#ROM_SIZE}
     *         bytes, carrying the size actually found
     * @throws java.io.IOException if the stream fails
     */
    public static byte[] readCartridge(java.io.InputStream in) throws java.io.IOException,
            WrongSize {
        byte[] rom = new byte[PatchData.ROM_SIZE];
        int total = 0;

        while (total < rom.length) {
            int n = in.read(rom, total, rom.length - total);

            if (n < 0) {
                break;
            }
            total += n;
        }
        /* Short means truncated; a byte still available means too large. */
        if (total != rom.length) {
            throw new WrongSize(total);
        }
        if (in.read() >= 0) {
            throw new WrongSize(rom.length + 1);
        }
        return rom;
    }
}
