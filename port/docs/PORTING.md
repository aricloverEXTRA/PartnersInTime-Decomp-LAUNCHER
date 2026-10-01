# Partners in Time Port

A legal, ROM-driven PC/Android port of Mario & Luigi: Partners in Time.

**This repository contains no Nintendo assets.** The user supplies their own
cartridge dump; the port reads it in place at runtime. Nothing is extracted to
disk, and no ROM-derived bytes are committed. Generated data projects and build
output are git-ignored.

## Supported ROMs

| Region          | SHA-1                                     | Status |
|-----------------|-------------------------------------------|--------|
| EUR             | `ba4ec2f99b4f2e0047601552bccf00aa73e28701` | verified |
| USA             | `89c9136db3c3975c451a907e8bd6861ce6b81557` | recognised |
| USA (Rev 1)     | `e23db7ff44d38299a6f1c377780378b191592a7e` | verified |

Both the EUR and USA (Rev 1) dumps pass `tools/rom_ingest/verify_rom.py` and
walk cleanly through the runtime NitroFS reader. The two revisions are **not**
interchangeable: their archives hold the same paths but different sizes
(EUR 55,951,409 bytes vs USA 55,624,913 bytes across 64 files), so any
region-specific table must be selected at runtime, not baked in.

Note the USA blockage is a *decompilation* limitation, not a port one - the port
has no problem reading the USA ROM.

## Build

The MSVC generator fails on paths containing `&`. Map the folder to a drive
letter first:

```
subst P: "C:\path\to\Partners in Time Port"
cd P:\
cmake -S . -B build
cmake --build build --config RelWithDebInfo
```

## Running

```
pit.exe [--dump-nitrofs] [--dump-archive <path>] [rom-path]
```

The ROM may also be supplied via the `PIT_ROM` environment variable.

- `--dump-nitrofs` walks the archive, prints the layout, and exits. Useful for
  verifying ingest without opening a window.
- `--dump-archive <path>` lists the entries of one `.dat` container and exits,
  or reports that the file is not an offset archive.
- `--no-assets` runs the presentation shell with generated placeholder content
  and no ROM.

Controls: `F1` cycles layout (stacked / side-by-side / overlay), `F2` cycles the
Dynamic Screen focus mode, `Esc` quits.

## Current status

Done:

- ROM ingest: open, parse the 512-byte header, verify SHA-1 against the
  supported-revision list.
- **NitroFS reader** (`src/core/pit_rom.h`): full FNT/FAT walk, path lookup and
  whole-file reads. Recovers 64 files across 21 top-level directories from both
  supported revisions. `BData/BDataMon.dat` - the file Hard Mode rewrites - is
  reachable through it.
- **Offset-archive container** (`src/core/pit_archive.h`) with a
  `--dump-archive` diagnostic, verified on both revisions.
- Presentation shell: dual 256x192 framebuffers, three layouts, Dynamic Screen
  focus handling, SDL2 frontend.
- Hard Mode data mod: reproducible multiplier profile and generator.
- **Standalone patcher** (`src/core/pit_patcher.c`, two frontends). Verifies the
  cartridge, locates the stat table through the NitroFS reader, applies the plan
  and recomputes the header CRC. Built and packaged for Windows and Android;
  see [The patcher](#the-patcher).

Not done:

- The game itself. No ARM code runs yet; screens still show generated
  placeholder content.
- Entry-type classification inside graphics containers, so a palette can be
  paired with its tile data and a real in-game asset rendered. The pixel decoders
  themselves are done: 4bpp/8bpp BG tiles and the 2bpp OBJ cells.
- Runtime mod manager and the in-game Hard Mode toggle. The patcher is an
  offline tool, not a live mod loader.


## NitroFS format notes

Verified against both supported revisions. The layout is easy to get subtly
wrong - three details cost real debugging time:

1. The FNT opens with a table of **8-byte directory records**
   (`u32 subtable_offset`, `u16 first_file_id`, `u16 parent_id`). The table is
   **not** limited to eight entries; it runs until the first subtable begins
   (26 directories in the EUR dump, ending exactly at offset `0xD0` = 26 * 8).
   Entries past the table decode as subtable bytes, so bounds-check the record
   and require a plausible `subtable_offset`.
2. Subtable entries pack **kind and name length into a single byte**: bit 7 set
   means directory, bits 0-6 are the name length. The name follows as raw bytes
   (Shift-JIS - not ASCII, so do not treat names as text).
3. **File entries carry no id field.** Only directory entries are followed by a
   `u16` holding `0xF000 | dir_id`. File ids are implicit, counting up from the
   directory's `first_file_id`. Reading a `u16` after a filename silently eats
   the next entry's length byte and desynchronises the whole walk.

The first archive file id also comes from the FNT root record, **not** from
deriving it from the FAT's ARM-binary entries.

## The launcher

A separate, self-contained tool rather than part of the presentation shell. It
has no dependency on the port's runtime, so it works today even though the game
does not.

`src/core/pit_patcher.c` is the core pipeline. It checks the cartridge size,
SHA-1, title, game code and header CRC-16; finds `BData/BDataMon.dat` through
the NitroFS reader; optionally scales the 98 stat records; recomputes the header
CRC; and writes a copy. `src/core/pit_ingest.c` then exports the generated data
the decompilation needs — the cartridge banner as a PNG, the NitroFS file tree,
a probe of `sound_data.sdat`, and a manifest — next to the copy. The source ROM
is never modified.

The launcher has three tabs — **ROM**, **MODS**, **ABOUT** — and the Hard Mode
mod is an option inside **MODS**, **off by default**. With the mod off, the
launcher verifies the user's own EUR cartridge and prepares the copy without a
mod; the output is suffixed `.prepared.nds`. With the mod on, the 98 stat
records are scaled and the output is suffixed `.hardmode.nds`. The two prepared
copies are named differently so they cannot be mistaken for each other.

### One plan, two languages

The Windows build is C/SDL2 and the Android build is pure Java. Rather than
hand-copying constants, both compile against data generated from one
authoritative plan:

```
tools/patch_sources/hard_mode_plan.json   constants only
tools/patch_sources/font8x8.txt           original 8x8 font
        |
        v  tools/gen_patchplan.py
        +--> src/core/pit_patch_data.h
        +--> android/.../PatchData.java
```

`gen_patchplan.py --check` fails if either generated file is stale. The plan
holds no retail data: an expected hash, a NitroFS path, a record stride, the
field offsets recovered from `BattleEnemyStatRecord`, and multipliers. Every stat
value is read from the user's own ROM at patch time.

### Rounding is exact, never floating point

Scales are carried as integer numerators and denominators, and rounding is
half-up integer arithmetic. A `.5` boundary is the one place a float would make
the three implementations disagree — C, Java and `tools/make_data_profile.py`
(which uses `Decimal`) could each round differently and produce three plausible
patched ROMs. The parity check exists to make that impossible.

### The header CRC seed is 0xFFFF

The DS documentation describes this CRC-16 as starting from 0. This cartridge
stores `D0BC` in the header, and only the 0xFFFF seed (polynomial `0xA001`, over
`0x000..0x15D`) reproduces that value. Getting this wrong makes every ROM look
corrupt.

### Verification

```powershell
.\check.ps1                                        # no ROM needed
.\build-release.ps1                                # builds, renders, packages
.\android\build-apk.ps1                            # signed APK
python tools\check_patch_roundtrip.py <patched.nds> <export.json>
python tools\check_c_java_parity.py --rom <rom.nds>
```

Current results, against the supported EUR cartridge:

| Check | Result |
|---|---|
| Patched ROM vs. the balanced Python profile export | exact match, 588 fields over 98 records |
| C output vs. Java output, mod on | byte-identical |
| C output vs. Java output, mod off (prepare-only) | byte-identical |
| Java unit tests | 964 checks, 0 failed |
| UI layout | rendered headlessly to PNG and text on every Windows build |

The last one matters more than it looks. `pit_patcher.exe --screenshot` draws
through the same `render()` the window uses and `--dump-ascii` serialises the
canvas to text, so a layout that cannot be drawn fails the build instead of
shipping. The UI checks cover all three tabs — the ROM tab at rest and
mid-patch, the MODS tab with the mod off and on, and the ABOUT tab — so a
layout change to one screen cannot silently regress another.

The prepared or patched ROM is **not** expected to boot. Nothing in this project
can run the game yet, and the patcher demonstrates a verified patch pipeline,
not a playable build.

## Android frontend

The Android app is plain Java against the platform APIs: no SDL, no NDK, no
Gradle. `Patcher.java`, `NitroFs.java` and `Sha1.java` have no `android.*`
dependency at all, which is what makes them testable on a desktop JVM — 964
class-path checks run without a device. `MainActivity` adds Storage Access
Framework file access and a worker thread; the app declares no storage
permission, so it only ever touches the two documents the user picked.

`PatcherView` draws the 480x320 interface by hand: the same three-tab design the
C build renders, with text blitted from the same generated 8x8 font the C build
uses. `tools/check_ui_parity.py` makes the two agree by failing if any shared
layout or palette constant differs. No font files and no bitmap assets ship in
the repository, and the launcher icon is vector XML.

Built with `aapt2` → `javac` → `d8` → `zipalign` → `apksigner` directly.
Signing is v1/v2 only, because the v3 block carries a SHA-256 signature that
older platforms reject.


## Game data container formats

### The outer `.dat` container: offset archive (solved)

The `.dat` files are **not** raw bitmaps. They use one outer container, which
the decompilation's data tooling implements as
`PiT/tools/data_mod.py :: parse_offset_archive`:

```text
u32   table_size      always (entry_count + 1) * 4
u32   offsets[count + 1]
u8    payload[]
```

`offsets[0] == table_size` (payload starts right after the table),
`offsets[count] == file size` (the table spans the whole file), and entry `i` is
`payload[offsets[i] .. offsets[i+1])`. Offsets are non-decreasing, empty entries
are legal and preserved, and a `0xFFFFFFFF` word marks a partially used table.

The leading word is the **table size, not an entry count** - that is why
treating it as a count never reconciled with the file length. Confirmed entry
counts match the values the decompilation documents independently:

| File                       | Entries | Independent confirmation |
|----------------------------|---------|---------------------------|
| `Treasure/TreasureInfo.dat` | 283     | "283 original file/room entries" |
| `BData/mfset_MonN.dat`      | 6       | "six ROM language slots" |
| `Title/TitleBG.dat`         | 38 (EUR) / 27 (USA Rev 1) | - |

Implemented in `src/core/pit_archive.h`; inspect any container with
`--dump-archive <nitrofs-path>`.

### Files that are not archives

`BData/BDataMon.dat` has no offset table. It is a flat array of fixed 44-byte
enemy records with no header: 4,312 bytes = 98 records in EUR, 4,092 = 93 in
USA Rev 1. The stride matches the decompilation's documented enemy layout
(`name_id`, `flags_or_ai_id`, `level`, `max_hp`, `power`, `defense`, ... , 0x2C
total), and the first EUR record decodes to `name_id=0x00A0`, `level=0x14`,
`max_hp=1`, `power=0x1000`, `defense=0x0C00`. This is the file Hard Mode
rewrites, so it is parsed directly rather than through the container.

### Inner resource descriptor (partially solved)

Entries inside a graphics container are `GameGraphicsResource` descriptors,
24 bytes, defined in `PiT/include/game/graphics_resource.h`:

| Offset | Type | Field |
|--------|------|-------|
| `0x00` | `u16` | flags: `normal_boundary:3` @7, `texture_format:3` @10, `color256:1` @14 |
| `0x02` | `u16` | `extra_count` |
| `0x04` | `u32` | `normal_tile_counts` |
| `0x08` | `u32` | `alternate_tile_counts` |
| `0x0C` | `u16` | `animation_count` |
| `0x0E` | `u16` | `frame_count` |
| `0x10` | `u16` | `group_count` |
| `0x12` | `u16` | `object_count` |
| `0x14` | `u16` | `texture_offset_layouts` |
| `0x16` | `u16` | unknown |

`GameGraphics_GetSection` (native `0x0200A2FC`, 0x160 bytes; not yet
reconstructed as C) returns a pointer into this descriptor. Sections 1-5 are
parallel arrays immediately after the 24-byte header, in this order:

| Section | Base offset | Stride | Count field |
|---------|-------------|--------|-------------|
| 0 | `+0x00` | - | the descriptor itself |
| 1 | `+0x18` | 8 | `animation_count` |
| 2 | `+0x18 + anim*8` | 4 | `frame_count` |
| 3 | `+0x18 + anim*8 + frames*4` | 4 | `group_count` (a `{u16 first, u16 end}` range) |
| 4 | `+0x18 + anim*8 + frames*4 + groups*4` | 12 | `object_count` (`GameGraphicsObject`) |
| 5 | section 4 + `object_count*12` | - | - |
| 6-9 | section 5 + `(section-6) * object_count*8` | - | - |

Two lookup tables, read directly out of native ARM9:

- `data_02049940[12]`, indexed by `(object.shape << 2) | object.size`, gives
  object byte counts: `sq_8:64, sq_16:256, sq_32:1024, sq_64:4096, h_8x16:128,
  h_16x8:256, h_32x8:512, h_16x16:2048, h_32x16:128, h_16x32:256, h_32x32:512,
  h_64x32:2048`. These are DS 4bpp/8bpp tile counts, which is what pins the
  texture encoding to 8x8 tiles of 32 or 64 bytes.
- `data_020499bc[8]`, indexed by `texture_format`, gives the GX texture format.

Texture offsets are stored in 8-byte units, and `GameGraphics_BuildTextureOffsets`
uses `shift = !color256`, so 4bpp and 8bpp resources share one offset table.

Not yet done: entries are a mix of resource types, and the descriptor above only
applies to some of them. `BObjUI.dat` entry 0, for example, decodes to
`animation_count=0xFFFF`, so it is not a `GameGraphicsResource`. Palettes are
raw BGR555 (`Title/TitleBG.dat` entry 2 is exactly 512 bytes = 256 entries).

The blocker is the section table, not the pixel formats. `GameGraphics_GetSection`
is the function that turns a section number into a pointer inside a resource, and
it is still declared-only in the decompilation: there is no implementation in
`src/`, only a prototype in `include/game/graphics_resource.h`. Its callers do
show what lives where - section 1 is model animation data, section 2 frame
entries, sections 3 and 4 the group ranges and `GameGraphicsObject` table - but
section 0, the one holding the tiles, has no confirmed layout.

Three native lookup tables have been transcribed from the EUR ARM9 binary
(loaded at `0x02004000`; subtract that from the symbol address to get a file
offset), and their declared types in `src/` were used to confirm each width:

| Symbol | Address | Type | Value |
|---|---|---|---|
| `data_020499bc` | `0x020499BC` | `u8[8]` | `0x00 0x06 0x04 0x05 0x09 0x03 0x04 0x00` |
| `data_02049ba8` | `0x02049BA8` | `u16[4]` | `32, 64, 128, 256` |
| `data_02049bb0` | `0x02049BB0` | `u16[16]` | `1, 4, 16, 64, 2, 4, 8, 32, 2, 4, 8, 32, 0, 0, 0, 0` |

`data_02049ba8` indexed by `color256` is bytes per tile: 32 for 4bpp, 64 for
8bpp. `data_020499bc` indexed by the 3-bit `texture_format` is the resolved
texture format, with indices 6 and 7 mapping to none. `data_02049bb0`, indexed
by `(shape << 2) | size`, is tile count per OBJ cell: the square row
`1, 4, 16, 64` is exactly `1^2, 2^2, 4^2, 8^2` for 8/16/32/64 px OBJ, and shape
3 is all zeros because square/horizontal/vertical are the only valid shapes. The
fourth horizontal/vertical value reads 32, which does not follow the `2, 4, 8`
progression, so it is recorded but not interpreted.

These three tables size the **4bpp/8bpp model texture** path
(`GameSpriteAnimator_UploadTiles` in `sprite_animator_tiles.c`). They do not
describe the 2bpp OBJ sprite path: `GameSpriteImage_Decode8x8/8x12/8x16` are
called from `sprite_image_decode_wide.c` and from nowhere else in the
reconstructed source, so the loader for the `BObj*`/`FObj*` containers is still
original code. Knowing the tile size is therefore not the missing piece for
those; a palette location and the section 0 offset still are. Guessing at
classification here produces plausible-looking garbage.

## 2bpp OBJ cell decoding

The game's OBJ sprites do not use the 4bpp/8bpp BG encoding above. They use a
bit-plane format that `GameSpriteImage_Decode8x8/8x12/8x16` expand into OBJ tile
memory, now decoded by `pit_tiles_decode_obj2bpp` in `pit_gfx`.

A cell is 8 pixels wide and 8, 12 or 16 pixels tall. The two index bits of each
pixel come from two interleaved bit planes rather than a packed byte stream: for
column `c` and row `r`, bit `4 * c + (r % 4)` of one 32-bit word is the low bit
and the same bit of the next word is the high bit. Each group of four rows
therefore consumes a word pair, making a cell `8 * height / 4` bytes - 16, 24
or 32. Cells are stored tightly packed; the 20-byte row pitch in the original
expanders belongs to the destination VRAM buffer, not the stored data.

Verified against the decompiled expanders: transcribing all three functions
verbatim and comparing them to `pit_tiles_decode_obj2bpp` over a 4x4 cell grid
of pseudo-random data for each cell height gives **4608 pixels compared, 0
mismatches**, and the module compiles clean under both MSVC and GCC.


## Asset renderer: first verified milestone

The cartridge banner is the first graphic decoded end to end from a real ROM,
which validates the whole path (ROM reader, BGR555 palette, 4bpp tile decode,
blit into a `pit_screen`, PNG output) before any of the game's own resource
containers are involved.

Modules: `pit_gfx` (image, palette, tile decode, blit, RGBA8), `pit_png`
(dependency-free PNG writer), `pit_nds_banner` (banner parse). CLI:
`--render-banner <out.png>`.

Verified: the 32x32 icon decodes **pixel-exact, 0/1024 mismatches**, against an
independent Python decoder reading the same ROM bytes. All 16 4bpp indices are
exercised. Confirmed on both EUR and USA Rev 1.

### Banner layout corrections

Three things here are easy to get wrong, and each produced plausible-looking but
wrong output before being caught against the data:

- **The icon is at `0x020`, not `0x010`.** `0x004..0x01F` is reserved and reads
  as zero, so decoding from `0x010` yields a correct-looking image with a blank
  strip down one side. The palette is at `0x220`, not `0x210`. Titles start at
  `0x240`, with no padding in any field.
- **The `0x840` block ends at `0x840`, but titles do not repeat.** The Japanese
  field holds the same UTF-16LE English string as the English field, because
  this release has no Japanese text. Assuming Shift-JIS there, as the format
  documents, yields a single stray character. `copy_title` detects UTF-16LE and
  falls back to a raw byte copy.
- **A byte-wise NUL scan cannot terminate a UTF-16LE field.** Every ASCII code
  unit has a zero high byte, so the scan stops at the first character. Scan for
  a zero *code unit* instead.

### The banner CRC is not reproducible

The `0x002` checksum does not match CRC-16/CCITT-FALSE, and is not any other
standard CRC-16. A search over every prefix length against polynomials `0x1021`,
`0x8005`, `0xA001`, `0x8408`, `0x0589`, `0x3D65`, `0x8BB7`, `0x1DCF` with the
usual seeds, reflected and non-reflected, found no match for either supported
ROM. It is a vendor-specific value, not a data-integrity check, so the port
reports it without gating rendering on it.

### 4bpp tile size

`tile_bytes` must be computed as `TILE_DIM * TILE_DIM * bits_per_pixel / 8`.
Deriving a byte count per pixel first and dividing that truncates to zero for
4bpp, which collapses every tile onto the same address and renders each 8-pixel
band repeated across the whole image. This produced a structurally valid PNG
that looked plausible and was wrong.
