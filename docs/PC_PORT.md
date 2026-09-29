# PC port evaluation

This document evaluates what a native PC build of Partners in Time could
realistically look like, given the current state of this repository. It is an
architecture assessment and a staging proposal, not a commitment to build one.

Nothing here changes the primary goal. The verified European ROM build remains
the source of truth and the correctness oracle; a port is a second build target
that consumes the same maintained sources.

## Summary

A PC port is feasible from this repository, but the decisive question is not
how much of the decompilation is left. It is **where the hardware boundary is
drawn**. The expensive part is the Nintendo DS 2D/3D pipeline and the ARM7,
not the gameplay logic — a large share of the gameplay is already data.

Two properties of the current tree matter more than the completion percentage:

1. **Content is largely solved as data.** Once the field, battle, and scene VMs
   run natively, 475,711 reachable script commands execute without a single
   further decompiled function. This is what moves the project into feasible
   territory at all.
2. **The maintained code is already portable in shape.** Around thirty absolute
   address literals exist across all of `src/`; everything else works through a
   base pointer plus offset. The per-overlay symbol namespaces (`func_ov002_…`)
   already solve the problem that overlays 11 through 26 all load at the same
   runtime address.

The important qualification: this repository optimizes for byte matching, which
is the wrong objective function for a port. A port needs semantic equivalence,
not identical register allocation. The 23,492-byte reconstructed field dispatcher
is nearly usable as port code today — it compiles and covers all 290 slots —
while it correctly remains unlinked for the ROM build at 95.96% fuzzy
similarity. The same holds for the 19,168-byte battle dispatcher and the
9,196-byte scene dispatcher.

## Measured starting point

| Quantity | Value |
|---|---:|
| Mapped ARM9 code | 1,563,700 B across 22 components |
| ARM9 function symbols | 4,956 |
| Byte-matching C | 878,932 B (56.21%) |
| Maintained symbolic ARM9 assembly | 4,040 B |
| Symbolic ARM7 | 43,852 B |
| Relink coverage | 43 components, 420 section units, 31,138 relocations, zero differing bytes |
| Reachable VM commands | 475,711 (638 Field rooms, 14 Battle archives, 3 Scene archives) |
| Editable data records | 10,510 strings, 98 enemies, 765 treasures, 99 items, 4 shops |
| Linked translation units that build under clang | 748 / 943 (79.3%) |

Regenerate the code figures with `tools/generate_progress.py`; see
[`PROGRESS.md`](PROGRESS.md) for the methodology.

The clang figure comes from `tools/check_portability.py`, which compiles every
unit in `config/eur/arm9/linked_sources.txt` with a clang translation of the
`CC_FLAGS` in `tools/configure.py`. It does not link and does not touch the
matching build. See [clang portability baseline](#clang-portability-baseline).

## Three candidate architectures

### A. Register-level HAL — a DS GPU on the host

Game code is left as it is and keeps writing `DISPCNT`, `DISPCAPCNT`, and the
G3 geometry FIFO. The host implements the GPU: a 2D compositor for both engines
(four BG layers each, affine and extended modes, OBJ, VRAM bank mapping), a 3D
rasterizer (geometry FIFO, matrix stack, polygon attributes, fog and toon),
display capture, and VCount-IRQ display swapping.

- **For:** minimal decompilation pressure. Graphics code that is not yet
  understood still works. This game needs that: per-scanline raster
  interpolation and `DISPCAPCNT`-driven transitions are the presentation layer,
  not an optional effect. See `src/battle/battle_raster_effects.c` and the grid,
  line, cylinder, curtain, and iris transition units next to it.
- **Against:** this is an emulator-grade GPU. Output stays at 256x192 per
  screen; upscaling is a shader concern and widescreen is effectively out.

### B. Semantic renderer

The DS graphics API surface — the Nitro GX/G2D/G3D layer plus the engine's own
render code in overlays 5 and 7 — is replaced by a modern scene renderer, with
native loaders for the NSBMD/NSBCA/NCGR/NSCR/NCLR family.

- **For:** real resolution independence, widescreen, and a clean base for
  enhancements.
- **Against:** it requires that every graphics-touching call site is understood
  first. This is the most expensive possible path and would gate the port on
  complete reconstruction of overlays 0, 2, 5, and 7.

### C. Static recompilation as a base, with progressive substitution

ARM9 code is mechanically translated to C and linked against the same HAL as in
(A). Every already-decompiled function is substituted for its recompiled
counterpart by symbol. The ARM7 is not recompiled; it is replaced (see below).

This suits the current tree unusually well, because the metadata that normally
sinks such an effort already exists here: exact section boundaries, 420
fixed-address units, a complete relocation catalogue, per-overlay symbol
namespaces, and a round-trip pipeline that proves the inputs are correct.

The ARM-specific hazards are tractable rather than absent: ARM/Thumb
interworking, `LDR pc` jump tables, ITCM as its own recompiled unit, and
overlays as separate module tables dispatched on the currently loaded overlay.

## Recommendation

The realistic end state is not a pure source port. It is a layered build in
which (C) bootstraps, (A) supplies the platform, and (B) is adopted per
subsystem only where it pays for itself:

```
game/          portable game code (C/C++), both targets
  |- natively decompiled     <- grows
  '- statically recompiled   <- shrinks
platform/nds/  real hardware (today's ROM build, mwccarm)
platform/pc/   HAL: OS, GX, sound, filesystem, input, save, timing
```

The ROM build is kept and becomes the test oracle. Byte identity is a harder
regression check than any port project normally has. Combined with the existing
`tools/runtime_drive.py` and py-desmume workflow documented in
[`research/RUNTIME_ANALYSIS.md`](research/RUNTIME_ANALYSIS.md), differential
testing is close to free: drive emulator and port with identical input, then
compare framebuffers and RAM snapshots.

### HAL surfaces

| Layer | What it replaces | Cost |
|---|---|---|
| OS | NitroSDK threads, message queues, mutexes, alarms, IRQ dispatch, DMA, cache operations | Fibers (Win32 fibers or `ucontext`). Keep the threading model as-is; the alarm and VBlank coupling is wired through the game code. Medium. |
| Memory | 4 MB main RAM, ITCM/DTCM, VRAM banks | One reserved arena at a fixed base. Also resolves the 64-bit problem below. Small. |
| GX | Both 2D engines, the 3D pipeline, display capture | **The main body of work.** Emulator-grade. Large. |
| Sound | The entire ARM7 | The ARM7 is dropped; the SSEQ/SBNK/SWAR sequencer is reimplemented natively and the PXI FIFO survives only as an API shim. Medium. |
| Filesystem | NitroFS/FAT plus the game archives | Substantially covered by existing tooling in `tools/`. Small. |
| Input | Keypad, touch screen, hinge | The field VM drives real touch mechanics (`0x132`–`0x135`, touchscreen mask erase), so pointer mapping is required, not optional. Small. |
| Save | Backup memory over ARM7 SPI | A host file. Small. |

Note the consequence for ARM7 work: the 43,852 bytes of symbolic ARM7 source
are valuable as documentation and as emulation evidence, but they are not a
port target. A port deletes that CPU.

### The 64-bit problem

The maintained code casts between `int` and pointer as a matter of course — for
example in `src/battle/battle_display_capture.c`, where a task queue call is
returned as `int`. Under LP64 this breaks broadly. Three options, in order of
practicality:

1. **Build the PC target as 32-bit** (i686 or arm32). It costs nothing, works
   immediately, and makes a wasm32 browser build nearly free as a side effect.
2. **Map a DS address-space arena**: place main RAM at a fixed low address and
   keep handles as `u32`. This leaves a later 64-bit build open.
3. **Type everything through.** This is the direction the matching
   decompilation takes anyway, but on its own schedule.

Endianness is not a concern; both targets are little-endian. The residual risks
are ARM `LDR` rotation on unaligned access and MWCC structure padding. The first
is untested. The second has now surfaced and is measured: MWCC aligns 64-bit
integers to 4 bytes where AAPCS uses 8, which is enough to change the size of
`FieldRuntimeEntity` and `OsAlarm`. See
[clang portability baseline](#clang-portability-baseline).

## What the shipped result looks like

- **One repository, two targets.** `ninja` continues to produce the verified
  EUR ROM; a separate CMake target produces the PC executable. CI builds both,
  and `ninja check` remains the gate.
- **No assets in the repository and none in a release.** The port requires the
  user's own ROM on first run and extracts to a local application directory.
  This is the discipline the project already enforces, and it is the reason a
  port could be published at all. See
  [`LOCAL_PRIVATE_CONTENT.md`](LOCAL_PRIVATE_CONTENT.md).
- **Two screens stay two screens.** The transitions actively copy between the
  displays and swap them on a VCount IRQ. Layout is a presentation option
  (stacked, side by side, bottom screen as an overlay), not a reconstruction
  into a single screen.
- **Modding is the actual payoff.** The data tooling is further along than the
  code: one high-level language across all three VMs, conflict-safe merging
  into the shared `FEvent/FEvData.dat` archive, and editable enemy, item, shop,
  and treasure tables. On PC that becomes a live mod loader instead of a ROM
  rebuild. See [`DATA_MODDING.md`](DATA_MODDING.md).
- **Enhancements, honestly sorted.** Higher internal 3D resolution, remappable
  controls, pointer-as-stylus, texture packs, and ordinary save files are all
  realistic. Widescreen and true 60 fps are not, for as long as architecture
  (A) carries the presentation: the 2D layers assume a 256-pixel field of view
  and animation timing counts frames throughout.

## Effort

Extrapolating from the matching-byte percentage produces a number that is not
useful. The better question is how much of the image is port-relevant:

- **Port-relevant:** resident ARM9 and overlays 0, 2, 5, 6, 7, 8, 9, and 25,
  approximately 1.0–1.1 MB. The fifteen attack overlays (~180 KB) are also
  required but are highly schematic and become close to templated after the
  first few.
- **Not port-relevant:** the ARM7 in full (replaced), parts of the SDK graphics
  layer (replaced), and anything the recompiler covers in the interim.
- **Already solved:** the 475,711 script commands.

With (C) as the base, a booting and playable target is plausible on a scale of
months, and the decompilation becomes continuous quality improvement rather
than a precondition. With (B) alone, a small team is looking at years.

## What to do now, so the port does not become a fork

These are cheap while the remaining code is still being written and expensive
later.

1. **Draw the HAL boundary now.** No new absolute addresses in `src/`; MMIO
   through accessors declared in `include/hardware.h` rather than raw
   `volatile` casts. `battle_display_capture.c` currently writes
   `REG_DISPCAPCNT` directly, which is the pattern to retire first.
2. **Tighten structure discipline.** Every newly proven offset becomes a typed
   field rather than `base + 0xNNN`. This is already the direction of the
   recent field VM work with ROM-shaped bitfields; the port simply needs it
   applied consistently. This is compatible with
   [`DECOMPILATION_STYLE.md`](DECOMPILATION_STYLE.md) as written.
3. **Add an empty second build target** that compiles the existing C and C++
   sources with clang. It costs about a day and keeps portability honest from
   now on, instead of discovering the truth about it in three years.
   **Done:** `tools/check_portability.py`. See
   [clang portability baseline](#clang-portability-baseline) for the measured
   result and the two ABI facts it exposed.

## Clang portability baseline

`tools/check_portability.py` compiles the 943 translation units listed in
`config/eur/arm9/linked_sources.txt` with clang 19 targeting `arm946e-s`,
translating every `CC_FLAGS` entry in `tools/configure.py`: `-fshort-enums`,
`-fsigned-char`, `-fno-merge-constants`, `-mfloat-abi=soft`, `-fno-exceptions`,
`-fno-rtti`, `-fshort-wchar`, `-nostdinc`, and C89/C++98 dialects selected per
file extension. It writes objects to `build/portability/` and does not link.

748 of 943 units compile (432 of 511 `.c`, 316 of 432 `.cpp`). All 195 failures
fall into three groups, and none of them is missing game logic:

| Units | Cause |
|---:|---|
| 172 | 64-bit integer alignment. See below. |
| 22 | Metrowerks `asm { ... }` blocks. |
| 1 | Assignment to a cast expression, an MWCC extension. |

### 64-bit integer alignment is the one that matters

MWCC aligns `s64`/`u64` to **4 bytes** on ARM946E with `-fp soft`. Clang follows
AAPCS and aligns them to **8**. Every struct containing a 64-bit member therefore
gains padding before that member, and the struct's own alignment rises with it.
Measured:

| Struct | MWCC | clang | Evidence |
|---|---:|---:|---|
| `FieldRuntimeEntity` | `0x520` | `0x528` | `include/game/field_entity.h:753` `s64 collision_policy` |
| `OsAlarm` | 44 | 48 | `include/nitro/os_alarm.h:15,18,19` `u64 fire, period, start` |
| `FieldEntity` | `0xEC` | `0xEC` | no 64-bit member, matches |
| `FieldRenderObject` | `0x138` | `0x138` | no 64-bit member, matches |

The `typedef char ..._SizeCheck[(sizeof(T) == N) ? 1 : -1]` asserts already in
the tree are what surface this, and they are **correct**: they encode the layout
the ROM actually uses, and MWCC is the compiler that produced it. The 172
failing units are reporting a real ABI difference, not a reconstruction defect.

Two consequences for a port:

- A port that adopts clang's natural layout will silently disagree with the ROM
  about the offsets of every field after a 64-bit member. `FieldRuntimeEntity` is
  the worst case, because it is the field-entity base record and 171 units touch
  it.
- Silently "fixing" the asserts to make clang happy would be a correctness
  regression in the matching build. If a port needs clang's layout, it has to
  say so explicitly, and it has to translate ROM offsets rather than trust its
  own struct.

### Metrowerks inline assembly

22 units contain `asm { ... }` blocks, which clang cannot parse on ARM; there is
no MS-style inline-assembly equivalent. Some are deliberate byte-matching
fragments, for example `src/battle/battle_capture_transform.c:29`, which keeps
the base-buffer and sub-buffer additions separate because Metrowerks otherwise
folds the C expression into one offset. A port has to supply these as intrinsics
or plain C, and accept that the result will not be byte-identical.

## Principal risks

1. **DS 2D/3D fidelity.** This game's transitions are hardware effects rather
   than graphics programming. A compositor that is one scanline out is visible
   immediately.
2. **Audio timing.** The sound thread hangs off alarms and the PXI queue; a
   naive callback mixer drifts.
3. **The two large dispatchers.** They are the bottleneck for the ROM build,
   but they are explicitly *not* the bottleneck for a port. That asymmetry is
   the strongest argument for opening a port branch early rather than waiting
   for the decompilation to finish.
