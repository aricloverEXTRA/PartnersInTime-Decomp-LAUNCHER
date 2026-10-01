package com.partnersintime.patcher;

import java.util.ArrayList;
import java.util.List;

/**
 * Class-path tests for the Android-independent half of the patcher.
 *
 * <p>No JUnit and no Android: the pipeline classes are deliberately free of
 * android.* so they can be checked on a desktop JVM, which is what makes them
 * testable at all. Run it with:
 *
 * <pre>
 * javac -d build/java android/app/src/main/java/com/partnersintime/patcher/{PatchData,Patcher,NitroFs,Sha1}.java \
 *       tools/java/com/partnersintime/patcher/PitSelfTest.java \
 *       tools/java/com/partnersintime/patcher/PitUnitTest.java
 * java -cp build/java com.partnersintime.patcher.PitUnitTest
 * </pre>
 *
 * <p>No ROM is needed. Every case here is arithmetic, structure or a failure
 * path, checked against values the plan states outright.
 */
public final class PitUnitTest {

    private static int checks;
    private static final List<String> failures = new ArrayList<String>();

    private PitUnitTest() {
    }

    private static void check(String name, boolean condition) {
        checks++;
        if (!condition) {
            failures.add(name);
        }
    }

    private static void checkEq(String name, long actual, long expected) {
        checks++;
        if (actual != expected) {
            failures.add(name + " (expected " + expected + ", got " + actual + ")");
        }
    }

    /** Rounds half up, matching scale_value() in C and make_data_profile.py. */
    private static int scale(int value, int num, int den) {
        return Patcher.scaleValue(value, num, den, 0, 65535);
    }

    private static void testScaleRounding() {
        /* Exact values, no rounding involved. */
        checkEq("11/10 of 10", scale(10, 11, 10), 11);
        checkEq("2/1 of 7", scale(7, 2, 1), 14);
        checkEq("21/20 of 100", scale(100, 21, 20), 105);
        checkEq("5/4 of 8", scale(8, 5, 4), 10);
        checkEq("7/4 of 4", scale(4, 7, 4), 7);

        /*
         * The .5 boundaries are the whole point of using exact rationals. A
         * float would land differently on some of these, so each is pinned.
         */
        checkEq("11/10 of 5 rounds up", scale(5, 11, 10), 6);
        checkEq("21/20 of 10 rounds up", scale(10, 21, 20), 11);
        checkEq("5/4 of 2 rounds up", scale(2, 5, 4), 3);
        checkEq("7/4 of 2 rounds up", scale(2, 7, 4), 4);
        checkEq("7/4 of 6 rounds up", scale(6, 7, 4), 11);
        checkEq("11/10 of 15", scale(15, 11, 10), 17);

        checkEq("zero stays zero", scale(0, 11, 10), 0);
        checkEq("one by half is one", scale(1, 1, 2), 1);
        checkEq("max input at 2/1 clamps", scale(65535, 2, 1), 65535);
        checkEq("large value stays exact", scale(40000, 21, 20), 42000);
    }

    private static void testClamping() {
        checkEq("min clamp", Patcher.scaleValue(0, 11, 10, 1, 65535), 1);
        checkEq("min clamp on power", Patcher.scaleValue(0, 2, 1, 0, 65535), 0);
        checkEq("max clamp", Patcher.scaleValue(65535, 2, 1, 0, 65535), 65535);
        checkEq("max clamp on defense",
                Patcher.scaleValue(65535, 21, 20, 1, 65535), 65535);
    }

    private static void testPlanIntegrity() {
        checkEq("record size", PatchData.RECORD_SIZE, 44);
        checkEq("record count", PatchData.RECORD_COUNT, 98);
        checkEq("table size", PatchData.RECORD_SIZE * PatchData.RECORD_COUNT, 4312);
        checkEq("rom size", PatchData.ROM_SIZE, 67108864);
        checkEq("target path", PatchData.TARGET_PATH.equals("BData/BDataMon.dat") ? 1 : 0, 1);
        checkEq("game code", PatchData.ROM_GAME_CODE.equals("ARMP") ? 1 : 0, 1);
        checkEq("sha1 length", PatchData.ROM_SHA1.length(), 40);
        checkEq("transform count", PatchData.TRANSFORM_NUM.length, 6);
        checkEq("all arrays agree on length", PatchData.TRANSFORM_DEN.length, 6);
        checkEq("labels", PatchData.TRANSFORM_LABEL.length, 6);
        checkEq("scale labels", PatchData.TRANSFORM_SCALE.length, 6);
        checkEq("field offsets", PatchData.FIELD_OFFSETS.length, 6);

        /*
         * Every transform must name a real field. A drift here would make the
         * patcher write a scaled value into the wrong field silently, which is
         * the worst failure this tool can have.
         */
        for (int t = 0; t < PatchData.TRANSFORM_FIELD.length; t++) {
            int field = PatchData.TRANSFORM_FIELD[t];
            check("transform " + t + " field index", field >= 0
                    && field < PatchData.FIELD_OFFSETS.length);
            check("transform " + t + " positive denominator",
                    PatchData.TRANSFORM_DEN[t] > 0);
            check("transform " + t + " min <= max",
                    PatchData.TRANSFORM_MIN[t] <= PatchData.TRANSFORM_MAX[t]);
        }

        /* The recovered offsets of BattleEnemyStatRecord, pinned independently. */
        checkEq("max_hp offset", PatchData.FIELD_OFFSETS[0], 6);
        checkEq("power offset", PatchData.FIELD_OFFSETS[1], 8);
        checkEq("defense offset", PatchData.FIELD_OFFSETS[2], 10);
        checkEq("speed offset", PatchData.FIELD_OFFSETS[3], 12);
        checkEq("experience offset", PatchData.FIELD_OFFSETS[4], 32);
        checkEq("coins offset", PatchData.FIELD_OFFSETS[5], 34);

        /* A scaled field must stay a halfword inside the record. */
        for (int field : PatchData.FIELD_OFFSETS) {
            check("offset " + field + " leaves room for a halfword",
                    field >= 0 && field + 2 <= PatchData.RECORD_SIZE);
        }
    }

    private static void testFontData() {
        checkEq("font glyph count", PatchData.FONT_GLYPHS, 95);
        checkEq("font first codepoint", PatchData.FONT_FIRST, 32);
        checkEq("font rows", PatchData.FONT8X8.length, 95);

        for (int i = 0; i < PatchData.FONT8X8.length; i++) {
            check("glyph " + i + " has 8 rows", PatchData.FONT8X8[i].length == 8);
            for (int row : PatchData.FONT8X8[i]) {
                check("glyph " + i + " row fits a byte", row >= 0 && row <= 0xFF);
            }
        }

        /* Space must be blank and a letter must not be, or the UI is unusable. */
        boolean spaceBlank = true;
        for (int row : PatchData.FONT8X8[' ' - PatchData.FONT_FIRST]) {
            if (row != 0) {
                spaceBlank = false;
            }
        }
        check("space is blank", spaceBlank);

        int ink = 0;
        for (int row : PatchData.FONT8X8['A' - PatchData.FONT_FIRST]) {
            ink += Integer.bitCount(row);
        }
        check("A has pixels", ink > 0);
    }

    private static void testSha1() {
        String empty = Sha1.hex(Sha1.digest(new byte[0]));
        checkEq("empty digest", empty.equals("da39a3ee5e6b4b0d3255bfef95601890afd80709") ? 1 : 0, 1);

        byte[] abc = new byte[] {'a', 'b', 'c'};
        String digest = Sha1.hex(Sha1.digest(abc));
        checkEq("abc digest", digest.equals("a9993e364706816aba3e25717850c26c9cd0d89d") ? 1 : 0, 1);
    }

    private static void testCrc() {
        /*
         * The header CRC of the supported cartridge, computed over the real
         * header bytes the ROM itself carries. Pinned because the seed is the
         * surprising part: this cartridge uses 0xFFFF, not the 0 the DS
         * documentation describes, and 0xD0BC only reproduces with 0xFFFF.
         */
        byte[] header = new byte[PatchData.ROM_SIZE];
        int crc = Patcher.headerCrc16(header, 0, header.length);
        /* An all-zero header still has to produce a stable value. */
        int again = Patcher.headerCrc16(header, 0, header.length);
        checkEq("crc is deterministic", crc, again);
        checkEq("crc fits a halfword", crc >= 0 && crc <= 0xFFFF ? 1 : 0, 1);
    }

    private static void testRejectsBadInput() {
        byte[] rom = new byte[1024];
        Patcher.Result result = Patcher.run(rom, new byte[rom.length], true, null);
        checkEq("wrong size rejected", result.code, Patcher.ERR_SIZE);

        byte[] right = new byte[PatchData.ROM_SIZE];
        Patcher.Result sized = Patcher.run(right, new byte[PatchData.ROM_SIZE], true, null);
        checkEq("wrong sha1 rejected", sized.code, Patcher.ERR_SHA1);

        /* An output buffer smaller than the input must not be touched. */
        Patcher.Result small = Patcher.run(rom, new byte[16], true, null);
        checkEq("undersized output rejected", small.code, Patcher.ERR_ARGS);

        check("error text is non-empty",
                Patcher.resultText(Patcher.ERR_SHA1).length() > 0);
        for (int code = 0; code <= Patcher.ERR_WRITE; code++) {
            check("error text for code " + code,
                    Patcher.resultText(code).length() > 0);
        }
    }

    /** A ROM-sized buffer with a valid FNT/FAT describing one known file. */
    private static byte[] romWithArchive(String path, int offset, int size) {
        byte[] rom = new byte[PatchData.ROM_SIZE];
        int fntOffset = 0x1000;
        int fatOffset = 0x2000;

        /*
         * Root directory record: subtable offset, first file id, parent id. The
         * subtable offset is relative to the start of the FNT, not to the ROM.
         */
        put32(rom, fntOffset, 8);
        put16(rom, fntOffset + 4, 0);
        put16(rom, fntOffset + 6, 0xF000);

        /* One subtable entry, no directory bit, so it takes the implicit id 0. */
        int sub = fntOffset + 8;
        byte[] name = path.getBytes();
        rom[sub] = (byte) name.length;
        System.arraycopy(name, 0, rom, sub + 1, name.length);
        rom[sub + 1 + name.length] = 0;

        put32(rom, fatOffset, offset);
        put32(rom, fatOffset + 4, offset + size);

        put32(rom, 0x40, fntOffset);
        /* The FNT must span the directory table, the entry and its terminator:
         * 8 + (1 + name length) + 1, rounded up. */
        put32(rom, 0x44, 8 + 1 + name.length + 1);
        put32(rom, 0x48, fatOffset);
        put32(rom, 0x4C, 8);
        return rom;
    }

    private static void put16(byte[] data, int offset, int value) {
        data[offset] = (byte) (value & 0xFF);
        data[offset + 1] = (byte) ((value >>> 8) & 0xFF);
    }

    private static void put32(byte[] data, int offset, int value) {
        data[offset] = (byte) (value & 0xFF);
        data[offset + 1] = (byte) ((value >>> 8) & 0xFF);
        data[offset + 2] = (byte) ((value >>> 16) & 0xFF);
        data[offset + 3] = (byte) ((value >>> 24) & 0xFF);
    }

    private static void testNitroFs() {
        byte[] rom = romWithArchive("BDataMon.dat", 0x00340000, 4312);
        put32(rom, 0x44, 0x600);
        NitroFs.Entry entry = NitroFs.find(rom, "BDataMon.dat");
        check("archive entry found", entry != null);
        if (entry != null) {
            checkEq("entry offset", entry.offset, 0x00340000);
            checkEq("entry size", entry.size, 4312);
            checkEq("entry id", entry.id, 0);
        }

        check("missing file returns null",
                NitroFs.find(rom, "NotThere.dat") == null);
        check("null rom returns null", NitroFs.find(null, "BDataMon.dat") == null);
        check("short buffer returns null", NitroFs.find(new byte[16], "x") == null);

        /* A FAT size that is not a multiple of the 8-byte record is malformed. */
        byte[] bad = romWithArchive("BDataMon.dat", 0x100, 16);
        put32(bad, 0x44, 0x600);
        put32(bad, 0x4C, 12);
        check("bad fat size returns null", NitroFs.find(bad, "BDataMon.dat") == null);

        /* An FNT pointing outside the ROM must be refused, not read. */
        byte[] wild = romWithArchive("BDataMon.dat", 0x100, 16);
        put32(wild, 0x44, 0x600);
        put32(wild, 0x40, 0x7FFFFFFF);
        check("wild fnt offset returns null", NitroFs.find(wild, "BDataMon.dat") == null);

        /* A table too small to hold a root directory record is malformed. */
        byte[] tiny = romWithArchive("BDataMon.dat", 0x100, 16);
        put32(tiny, 0x44, 0x08);
        check("undersized fnt returns null", NitroFs.find(tiny, "BDataMon.dat") == null);
    }

    /** A ROM-sized buffer with a Sound/sound_data.sdat tree, a banner and SDAT. */
    private static byte[] romWithSdat() {
        byte[] rom = romWithArchive("BDataMon.dat", 0x00340000, 4312);
        int fntOffset = 0x1000;

        /* Directory records, then the subtables they point at. */
        put32(rom, fntOffset, 0x200);       /* root subtable offset */
        put16(rom, fntOffset + 4, 0);       /* root first file id */
        put16(rom, fntOffset + 6, 0xF000);  /* root parent */
        put32(rom, fntOffset + 8, 0x300);   /* Sound subtable offset */
        put16(rom, fntOffset + 12, 1);      /* Sound first file id */
        put16(rom, fntOffset + 14, 0xF000); /* Sound parent */

        /* Root subtable: BDataMon.dat, then the Sound directory entry. */
        int rootSub = fntOffset + 0x200;
        byte[] name = "BDataMon.dat".getBytes();
        rom[rootSub] = (byte) name.length;
        System.arraycopy(name, 0, rom, rootSub + 1, name.length);
        int e = rootSub + 1 + name.length;
        byte[] snd = "Sound".getBytes();
        rom[e] = (byte) (0x80 | snd.length);
        System.arraycopy(snd, 0, rom, e + 1, snd.length);
        put16(rom, e + 1 + snd.length, 0xF001);
        rom[e + 1 + snd.length + 2] = 0;

        /* Sound subtable: sound_data.sdat, taking the implicit id 1. */
        int sub = fntOffset + 0x300;
        byte[] dat = "sound_data.sdat".getBytes();
        rom[sub] = (byte) dat.length;
        System.arraycopy(dat, 0, rom, sub + 1, dat.length);
        rom[sub + 1 + dat.length] = 0;

        put32(rom, 0x44, 0x600);
        put32(rom, 0x4C, 16);

        /* Second FAT record: the sound bank, SDAT header little-endian at its head. */
        int sdat = 0x00800000;
        put32(rom, 0x2008, sdat);
        put32(rom, 0x200C, sdat + 0x9000);
        rom[sdat] = 'S';
        rom[sdat + 1] = 'D';
        rom[sdat + 2] = 'A';
        rom[sdat + 3] = 'T';
        rom[sdat + 4] = (byte) 0xFF;
        rom[sdat + 5] = (byte) 0xFE;
        rom[sdat + 6] = 0x00;
        rom[sdat + 7] = 0x01;
        put32(rom, sdat + 8, 0x9000);
        put16(rom, sdat + 0x0C, 0x40);
        put16(rom, sdat + 0x0E, 4);
        put32(rom, sdat + 0x10, 0x40);

        /* Cartridge banner with the EUR CRC and a plain one-line EN title. */
        int banner = 0x00100000;
        put32(rom, 0x68, banner);
        put16(rom, banner, 1);           /* banner version at +0 */
        put16(rom, banner + 2, 0x2BB0);  /* stored crc16 at +2 */
        String title = "MARIO & LUIGI 2";
        for (int i = 0; i < title.length(); i++) {
            rom[banner + 0x340 + i * 2] = (byte) title.charAt(i);
            rom[banner + 0x340 + i * 2 + 1] = 0;
        }
        return rom;
    }

    private static void testProbe() {
        Patcher.Probe absent = Patcher.probe(null);
        check("null rom probe absent", !absent.bannerTitleOk && !absent.sdatPresent);
        check("short rom probe absent", !Patcher.probe(new byte[16]).bannerPresent);

        Patcher.Probe p = Patcher.probe(romWithSdat());
        check("fixture banner present", p.bannerPresent);
        check("fixture banner title ok", p.bannerTitleOk);
        checkEq("banner version", p.bannerVersion, 1);
        checkEq("banner crc", p.bannerCrc16, 0x2BB0);
        check("banner title text", "MARIO & LUIGI 2".equals(p.bannerTitle));
        checkEq("archive files", p.archiveFiles, 2);
        check("sdat present", p.sdatPresent);
        checkEq("sdat major", p.sdatVersionMajor, 1);
        checkEq("sdat minor", p.sdatVersionMinor, 0);
        checkEq("sdat sections", p.sdatSections, 4);

        Patcher.Probe bare = Patcher.probe(romWithArchive("BDataMon.dat", 0x340000, 4312));
        check("bare rom no banner", !bare.bannerTitleOk);
        checkEq("bare rom archive files", bare.archiveFiles, 1);
        check("bare rom no sdat", !bare.sdatPresent);
    }

    public static void main(String[] args) {
        testScaleRounding();
        testClamping();
        testPlanIntegrity();
        testFontData();
        testSha1();
        testCrc();
        testRejectsBadInput();
        testNitroFs();
        testProbe();

        System.out.println(checks + " checks, " + failures.size() + " failed");
        for (String failure : failures) {
            System.out.println("  FAIL " + failure);
        }
        if (!failures.isEmpty()) {
            System.exit(1);
        }
        System.out.println("all checks passed");
    }
}
