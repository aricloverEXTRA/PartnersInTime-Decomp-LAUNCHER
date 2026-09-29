package com.partnersintime.patcher;

/**
 * NitroFS directory walking, matching walk_dir() in src/core/pit_rom.c.
 *
 * <p>Only the FNT/FAT structures are interpreted, and only far enough to locate
 * the one file the plan names. No archive contents are decompressed and no file
 * data is cached, so this stays cheap enough to run on the UI thread's worker.
 *
 * <p>The FNT holds a table of 8-byte directory records
 * ({@code u32 subtable_offset, u16 first_file_id, u16 parent_id}) that runs until
 * the first subtable begins. Each subtable entry packs kind and name length into
 * one byte (bit 7 set means directory, bits 0-6 are the length) followed by the
 * raw name. Directory entries are followed by a {@code u16} holding
 * {@code 0xF000 | dir_id}; file entries carry no id and take the next implicit id,
 * counting up from the directory's first_file_id. A zero byte ends a subtable.
 */
public final class NitroFs {

    private static final int FNT_DIR_BIT = 0x80;
    private static final int FNT_NAME_MASK = 0x7F;
    private static final int FNT_DIR_ID = 0xF000;
    private static final int MAX_DEPTH = 32;

    /** A located archive file. */
    public static final class Entry {
        public final int id;
        public final int offset;
        public final int size;
        public final String path;

        Entry(int id, int offset, int size, String path) {
            this.id = id;
            this.offset = offset;
            this.size = size;
            this.path = path;
        }
    }

    private NitroFs() {
    }

    private static int rd16(byte[] d, int o) {
        return (d[o] & 0xFF) | ((d[o + 1] & 0xFF) << 8);
    }

    private static int rd32(byte[] d, int o) {
        return (d[o] & 0xFF) | ((d[o + 1] & 0xFF) << 8) | ((d[o + 2] & 0xFF) << 16)
                | ((d[o + 3] & 0xFF) << 24);
    }

    /** Decodes a Shift-JIS name to text for display and comparison. */
    static String decodeName(byte[] rom, int offset, int length) {
        StringBuilder sb = new StringBuilder(length);

        for (int i = 0; i < length; i++) {
            int b = rom[offset + i] & 0xFF;

            if (b >= 0x20 && b <= 0x7E) {
                sb.append((char) b);
            } else {
                sb.append('?');
            }
        }
        return sb.toString();
    }

    private static Entry walk(byte[] rom, int fntOffset, int fntSize, int fatOffset,
            int fileCount, int dirId, int depth, String prefix, Entry want) {
        if (depth > MAX_DEPTH || (long) dirId * 8 + 8 > fntSize) {
            return null;
        }
        int rec = fntOffset + dirId * 8;
        int subOff = rd32(rom, rec);
        int nextId = rd16(rom, rec + 4);

        if (subOff == 0 || subOff >= fntSize) {
            return null;
        }
        int cursor = fntOffset + subOff;

        while (cursor < fntOffset + fntSize) {
            int flags = rom[cursor] & 0xFF;
            if (flags == 0) {
                return null;
            }
            int rawLen = flags & FNT_NAME_MASK;
            boolean isDir = (flags & FNT_DIR_BIT) != 0;
            int needed = 1 + rawLen + (isDir ? 2 : 0);
            if ((long) cursor - fntOffset + needed > fntSize) {
                return null;
            }

            int nameOffset = cursor + 1;
            String name = decodeName(rom, nameOffset, rawLen);

            if (isDir) {
                int id = rd16(rom, nameOffset + rawLen);
                cursor += needed;
                if ((id & 0xF000) != FNT_DIR_ID) {
                    return null;
                }
                Entry hit = walk(rom, fntOffset, fntSize, fatOffset, fileCount,
                        id & 0x0FFF, depth + 1, prefix + name + "/", want);
                if (hit != null) {
                    return hit;
                }
            } else {
                cursor += needed;
                if (nextId >= fileCount) {
                    continue;
                }
                int start = rd32(rom, fatOffset + nextId * 8);
                int end = rd32(rom, fatOffset + nextId * 8 + 4);
                String path = prefix + name;
                if (want != null && path.equals(want.path)) {
                    return new Entry(nextId, start, end - start, path);
                }
                nextId++;
            }
        }
        return null;
    }

    /**
     * Locates {@code path} in the archive, or returns null when it is absent.
     *
     * <p>A null result and a malformed filesystem are deliberately
     * indistinguishable to the caller: neither is a supported ROM, and the
     * patcher reports the same actionable message for both.
     */
    public static Entry find(byte[] rom, String path) {
        if (rom == null || rom.length < 0x200) {
            return null;
        }
        int fntOffset = rd32(rom, 0x40);
        int fntSize = rd32(rom, 0x44);
        int fatOffset = rd32(rom, 0x48);
        int fatSize = rd32(rom, 0x4C);

        if (fatSize < 8 || (fatSize % 8) != 0) {
            return null;
        }
        int fileCount = fatSize / 8;
        if ((long) fntOffset + fntSize > rom.length || fntSize < 0x40
                || (long) fatOffset + fatSize > rom.length || fntOffset == 0) {
            return null;
        }
        return walk(rom, fntOffset, fntSize, fatOffset, fileCount, 0, 0, "",
                new Entry(-1, 0, 0, path));
    }
}
