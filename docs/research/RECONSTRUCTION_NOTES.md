# Reconstruction and runtime reference

The repository [AGENTS.md](../../AGENTS.md) contains the working instructions.
Read the relevant sections here before reconstructing code or extending a probe.
This reference preserves the detailed findings and tested routes accumulated
during earlier batches; current source and fresh checks take precedence.

Paths in backticks are relative to the repository root unless explicitly absolute.
Private files under `build/`, `extract/` and `PiT_SaveStates/` are local research
resources, not files included in a fresh clone. Check their availability and
provenance before reuse. Evidence from a controlled fixture does not establish
ordinary gameplay accessibility or coverage of unexercised branches.

- [NitroSDK cross-game reference](#nitrosdk-cross-game-reference)
- [Locating evidence and comparing candidates](#locating-evidence-and-comparing-candidates)
- [EUR memory reference](#eur-memory-reference)
- [Compiler and linker behavior](#compiler-and-linker-behavior)
- [Reconstructing and integrating code](#reconstructing-and-integrating-code)
- [Runtime verification](#runtime-verification)
- Shared sprite helpers: [OAM wrapper ABI and pooled initialization](#overlay-5-sprite-collection-and-initialization)
- Battle exit: [transition dispatch and resource-slot selection](#battle-transition-dispatch-and-resource-slots)
- Battle model animations: [shared track creation and Mix Flowers](#battle-model-animation-starts)
- Battle VM motion: [directional operand decoding and live channel checks](#battle-vm-directional-motion)
- Battle VM vertical motion: [solver modes, coupled channels and copied-state checks](#battle-vm-vertical-motion)
- Code-derived findings: [pause party status](#pause-party-status)
- Pause transitions: [verified exit tasks](#pause-exit-tasks-and-transition-state),
  [projection and callback ABI](#pause-transition-projection-and-callback-abi),
  [entry/exit setup calls](#pause-transition-setup-calls)
- Tested routes: [shops](#shops), [save menus](#save-menus), [Game Over](#game-over),
  [Smash Eggs](#smash-eggs), [credits](#credits), [Nawatobi](#nawatobi)

## NitroSDK cross-game reference

The [NitroSDK manual for Bowser's Inside Story](https://inf.gg/mlbis/manual/nitrosdk)
is an external reference for SDK names, records and API behavior. Its author
identifies the research as independent reverse engineering, with function names
cross-referenced to the retail GTA: Chinatown Wars symbol map. Addresses belong
to those games; locate each PiT routine through current symbols and instructions.
Names marked `?` and unknown fields remain hypotheses.

Check types and arithmetic against PiT before adopting them. For example, the
manual describes archive-table caching, but its displayed size formula masks
`~0x3f`. PiT's `FS_LoadArchiveTables` at EUR ARM9 `0x0203E438` masks `~0x1f`:
`(fat_size + fnt_size + 63) & ~31`. Its destination also aligns to 32 bytes.
PiT's archive-name helper tests its length with signed branches, despite the
reference's unsigned declaration. These differences do not establish whether
the other game differs or the reference needs correction.

### Archive table caching

[The loader](../../src/nitro/fs/fs_archive_tables.c) owns the 300-byte range
`0x0203E438..0x0203E564`. It queries required capacity without touching the buffer
when too small; otherwise it loads FAT then FNT, installs RAM table pointers and
retains the original buffer pointer. Failed reads clear that table, while failed
opens leave its destination untouched. Either failure still installs the new
table pointers. A nonnegative short read does not clear the unread tail.

One inline `add` preserves the native separate reserve/round additions; MWCC
folds their C equivalent into `+63`. Cached table offsets preserve the native
loads before outgoing call arguments. The actual linked object and full ROM
match exactly; no additional symbolic-ASM unit is counted.

Private evidence is in `build/analysis/xhigh_from_55/` and
`build/runtime/eur_xhigh_fs_tables/`. `boot55_v1.json` records a natural 1,200-frame
cold boot using save 55: two capacity queries and one successful load, checking
the entire 80-byte archive, all 2,304 buffer bytes including padding, the 808-byte
FAT and 1,444-byte FNT against ROM, helper arguments/results and preserved
registers/stack. The final title screen was inspected; all 104 saves stayed intact.

`isolated_v1.json` supplements this with 169 ARM946 cases on copied live RAM and
synthetic records. It covers all 32 buffer alignments, insufficient/exact/extra
capacity, zero lengths, unsigned size comparisons, independent open/read failures
and short reads. All mapped RAM/DTCM/scratch outside the bounded CPU stack, 670
ordered archive stores and helper calls are checked. The real initializer and
byte-fill execute; open/read/close are explicit models. These cases do not claim
live failed cartridge I/O, asynchronous queue or hardware timing coverage.

## Locating evidence and comparing candidates

Old milestone notes describe the state at the time they were written. Current
source, `delinks.txt`, `symbols.txt`, the linked-source manifest and fresh checks
take precedence over old counts or claims that a function is unfinished.
Distinguish the working tree, committed code and pushed code. A pending source
integration can leave the checked-in progress files behind the local metadata;
do not publish its extra bytes before completing the batch's verification.
Read the latest request before following a handoff's suggested next action.
The private `build/analysis/CURRENT_HANDOFF.md`, when present, records pending
candidates and completed checks. Check its commit ID against `git log`; a
handoff can lag behind a pushed batch. It is context, not an instruction to
continue a superseded task or proof that a private candidate is linked.

Resident game helpers are in `src/game/`, SDK routines in `src/nitro/`, field
code in `src/field/`, battle code in `src/battle/`, and scene/menu/attack code
in the corresponding `src/overlayNNN/` directory. Use the overlay map to select
a subsystem; do not infer a gameplay identity from an overlay number alone.
Original ARM9 bytes are under `extract/eur/arm9/` and
`extract/eur/arm9_overlays/`; derive load addresses from component metadata.
Resident ARM9 metadata is directly in `config/eur/arm9/symbols.txt` and
`config/eur/arm9/delinks.txt`. Overlay metadata is under
`config/eur/arm9/overlays/ovNNN/`. The private pseudocode directory name
`main_game` does not imply a corresponding metadata subdirectory.
The extracted EUR resident `arm9.bin` starts at `0x02004000`, not the start of
main RAM at `0x02000000`. Derive file offsets from the component's actual base;
check lookup-table bytes against live RAM before relying on a runtime oracle.

Private IDA databases live under `build/ida/`; previous pseudocode exports and
candidate units live under `build/analysis/`, including
`build/analysis/local_decompiler/`. Recompile a candidate against current headers
before trusting an old matching object. Abandoned drafts may contain incorrect
experimental prototypes. Parse large JSON reports and select the relevant
records instead of dumping entire reports into the conversation.
Keep each comparison associated with its exact source, language mode, object
and current headers. A neighboring `.cpp` draft may predate the matching `.c`
version. Private comparison scripts can overwrite address-named candidate dumps;
do not treat an older disassembly listing as the output of the latest compile.

On this workstation, private comparison helpers include
`build/analysis/compile_private_unit.py`, `check_title_unit.py` for overlay 6,
`check_nawatobi_unit.py` for overlay 7 (also used for pause-menu candidates),
`check_overlay8_unit.py` for overlay 8, `check_overlay9_unit.py` for overlay 9,
`check_overlay15_unit.py` for overlay 15,
and `check_main_unit.py` for resident ARM9. Inspect their inputs and relocation
handling before reuse; they are local conveniences, not required public tools.
Use current public build checks as the final authority. Private paths in this
file are research references, not files supplied by a fresh clone. If one is
absent, reconstruct the needed evidence from the public tooling, native bytes
and user-supplied ROM/saves rather than treating an old report as a fresh check.
After a symbol rename, ensure the private checker resolves the new symbol in
the candidate object. A fallback to an address-named symbol or original bytes
must not turn a missing candidate function into a passing comparison.

Candidate inventories are only discovery aids. Recheck each range against the
current manifest before spending time on it: a previous candidate may already
be linked, or an old inventory may assign an overlay function to resident ARM9.
Inspect the body before calling a target easy. An indirect jump represented by
one line of pseudocode can hide a dispatcher, and a short graphics function can
omit most of its FIFO writes. Prefer a coherent group with understood callers
and a reproducible runtime route. Keep the next candidate independent of a
deferred register-allocation problem.

## EUR memory reference

These locations are for the European game (`ARMP`), using the ARM9's address
space. `read32(address)` means a little-endian 32-bit dereference. Require the
owning overlay and a live allocation before inspecting a record; overlays can
reuse addresses, and constructors can run before their global is assigned.
The sizes below are recovered record or allocation extents, not permission to
read a freed object. See the [Nawatobi procedure](#nawatobi) for the guarded
one-time state edit; the remaining entries are inspection references.

| Record | Address expression | Extent and ownership |
|---|---|---|
| Pause scene task | `read32(0x0208E1E0)` | 56 bytes, overlay 7; signed 32-bit phase at `+0x30`, menu pointer at `+0x34` |
| Pause workspace | `0x020905F0` | 90,600-byte object, overlay 7; do not dereference its first word as a pointer |
| Pause transition workspace | `0x0208E1E8` | 9,224-byte object, overlay 7; signed 32-bit progress at offset 0, not a pointer to another allocation |
| Pause party | `read32(0x0208E1E4)` | 4,428-byte allocation, overlay 7; `Overlay7Party` describes the first 332 bytes, followed by 4,096 bytes of scratch |
| Live save record | `read32(0x02059FE8)` | 1,380-byte record used by the probes; this is distinct from the on-disk battery-save size |
| Display resources | `0x0206A180` | 176-byte object, overlay 5; archive pointer at `+0x2C` |
| Menu element pool | `0x0206A240` | 20-byte object, overlay 5; element array, link array, free list, taken list and count at offsets 0, 4, 8, 12 and 16 |
| Menu element lists | `0x0206A254` | Twelve 16-byte list records, overlay 5; each contains two eight-byte sentinel records |
| Active menu element count | `0x0206A230` | Unsigned 16-bit value, overlay 5 |
| ResourceB sprite pool | `0x0206AA18` | 20-byte pool object, overlay 5; 64-byte item slots and eight-byte links |
| ResourceA model pool | `0x0206A3D8` | 20-byte pool object, overlay 5; 336-byte model slots and eight-byte links |
| Draw-node pool | `0x0206A3F8` | 20-byte pool object, overlay 5; eight-byte nodes and eight-byte links |
| Draw lists | `0x0206A40C` | Two screens of 64 twelve-byte list records, overlay 5; distinct from the menu element lists |
| Battle scene objects | `read32(0x020C0718) + 0x5B0 + 0x104 * index` | Overlay 2; 70 embedded 260-byte records, indices 0 through 69. Verify live slot/actor ownership; these are not separate heap allocations. See [relative effects](#battle-relative-effect-spawning). |
| Battle scheduler | `read32(0x020C0714)` | Overlay 2; 3,584-byte allocation. Queues, list head and scanline clock are described under [scheduler queues](#battle-scheduler-queues). The final four bytes lie beyond the constructor's clearing range. |

The shared source layouts are in
[pause_scene.h](../../include/game/pause_scene.h),
[pause_scene_internal.h](../../src/scene_menu_ov007/pause_scene_internal.h),
[overlay007_party.h](../../include/game/overlay007_party.h) and
[element_lists.c](../../src/scene_support_ov005/element_lists.c).
The menu factory at `0x0206659C` takes `(callback, list, marker)` and allocates
a 72-byte slot. It initializes the common fields through offset `0x20`, leaving
the payload at `+0x24` onward unchanged. The caller initializes its own payload.
Marking at `0x0206650C`, removal at `0x020664A8` and pool return at `0x020667B0`
are distinct boundaries. Check the release callback and slot before pool return,
then check only still-live pool, list and counter records.

For sprite and draw allocation, use the actual layouts in
[sprite_pool.cpp](../../src/scene_support_ov005/sprite_pool.cpp),
[item_pool.c](../../src/scene_support_ov005/item_pool.c),
[overlay005_resource.h](../../include/game/overlay005_resource.h) and
[draw_lists.cpp](../../src/scene_support_ov005/draw_lists.cpp).
`Overlay5ResourceOwner` has its release callback at `+0x18` and resource pointer
at `+0x1C`; these fields belong to the owner, not the attached sprite.
ResourceB attachment can install `Overlay5ResourceB_Release`, which releases the
sprite and clears the owner's resource pointer. Track the task and sprite as
separate lifetimes, using [resource_owner.c](../../src/scene_support_ov005/element_lists.c)
and the removal helper to establish the actual sequence.
Whole-pool destruction also ends allocations: verify the cleared pool pointers
before discarding their allocation records. Retain any outstanding per-task
removal assertion; a bulk free does not prove that callback completed normally.

The pause and shop probes commonly retain the following graphics ranges.
These are **capture extents**, not a universal map of all available video memory;
interpret them with the captured VRAM-bank and display configuration.

| Capture | ARM9 address | Bytes captured |
|---|---|---|
| Main / sub BG | `0x06000000` / `0x06200000` | 131,072 each |
| Main / sub OBJ | `0x06400000` / `0x06600000` | 65,536 each |
| Palettes / OAM | `0x05000000` / `0x07000000` | 2,048 each |
| Main / sub display registers | `0x04000000` / `0x04001000` | 112 each |
| VRAM-bank registers | `0x04000240` | 10 |

That is nine dumps per capture, alongside a 256 by 384 screenshot of both
screens. Validate the specific probe's manifest rather than assuming it used
these defaults. Register snapshots supplement ordered GPU-write checks; a
scroll-register readback need not reproduce the submitted value.

EUR ARM9 stack outputs can reside in DTCM at `0x027E0000..0x027E4000`.
Read that CPU-visible range directly in a focused probe. The current
`runtime_probe.py` command-line capture-range validator does not include DTCM;
do not redirect such an address to a main-RAM mirror to bypass that limitation.

## Compiler and linker behavior

Do not change compiler flags to force a match. Derive individual object builds
and comparisons from the existing Ninja/objdiff configuration. The configured
ARM946E target, signed `char`, enum width, software floating point, interworking
and C++ mode are part of the original ABI.
This compiler's configured C mode requires declarations at the start of a
block. Use a justified nested scope when needed; do not switch a whole unit
to C++ merely to accept declarations after statements.

Absolute linker symbols such as `OVERLAY_5_ID` and `OVERLAY_6_ID` carry their
numeric value in the symbol address. Resolve them from the generated
`build/eur/linker_script.lcf` when checking private objects. Preserve the
native literal relocation where required; reading such a symbol as a C variable
would dereference that number as an address.

A decompiler can fold a condition after resolving a linker value to a constant,
even though both branches remain in the original instructions. The resident
thread initializer at `0x020396D8` retains a stack-bound calculation conditional
on a literal that is zero in the EUR binary. Recover the original relocation
and linker expression before replacing such a value with a C constant; do not
invent an SDK symbol name or add volatile accesses to recreate the missing
branch. Until that evidence is available, defer the function.

### OAM and transform views

Recover the relationship between different views of a graphics buffer before
changing its type to explain a compiler mismatch. `GameOamEntry` is eight bytes,
while `func_02009598` accesses 32-byte groups through `BattleTransformRow` in
[the model-state implementation](../../src/battle/battle_model_state.cpp).
It reads and rewrites the halfwords at group offsets 6, 14, 22 and 30 using
`GameAffine_InvertQ8`. These are the affine fields spread across four OAM records.
Its callee definition uses byte-sized first/end indices; existing callers and
private drafts have differing declarations. Check those accesses and every
affected caller before introducing a shared view or correcting a prototype.
The helper's implementation establishes more than an old draft's generic name
or pointer cast. A same-sized buffer alone does not establish compatible types.

## Reconstructing and integrating code

1. Locate the original function and its boundaries in the component's
   `config/eur/arm9/**/symbols.txt` and `delinks.txt`. Inspect the instructions,
   callers and neighboring functions; decompiler pseudocode is a hypothesis.
2. Recover field widths, signedness, offsets, virtual calls and ownership from
   those accesses. Preserve original behavior, including awkward edge cases.
   Use neutral names for unknown fields; do not guess item or enemy identities.
   Check call sites and callee accesses before changing a prototype, including
   apparently unused arguments. An incorrect prototype can cause the register
   differences that otherwise look like a compiler-scheduling problem.
   Recover stack-passed arguments from the instructions before the call;
   pseudocode has omitted motion parameters and cached object loads. Check
   `LDRSB` versus `LDRB` even for small phase counters: signedness affects their
   increment and comparison behavior.
3. Keep related contiguous functions in a subsystem module. Temporary isolated
   units are acceptable while an intervening assembly gap remains; consolidate
   once that gap is recovered. Shared declarations and layouts belong in headers.
   Check source basenames across all components before adding a unit. The MW
   linker script selects objects by basename, so `attack_entry.c` would collide
   with an existing `attack_entry.cpp` even in a different directory.
4. Preserve native function order. This MWCC setup generally emits the separate
   function sections in reverse source order, so definitions usually descend
   by original address. Verify the emitted order instead of assuming it.
5. Compare the compiled instructions, sizes, relocations and symbol layout.
   Only verified units belong in `config/eur/arm9/linked_sources.txt`. A source
   file can compile or appear in objdiff while the ROM still uses its original
   delinked object; that is not completed matching C.
6. Update the component metadata and affected declarations together. Preserve
   shipped interior entry points through `config/eur/arm9/linker_aliases.json`;
   do not silently turn an interior label into a different function.
   When renaming a symbol, also check `reasm/eur/patches.json` and maintained
   assembly references. Resolve names within the correct CPU and overlay;
   a raw address alone is ambiguous. Keep external addresses and relocation
   checks intact rather than weakening validation to accept stale names.
7. Run `ninja check` for every C/C++ change, including shared-header changes.
   Verify the packaged ROM and relevant runtime behavior before reporting the
   batch complete. Record functions or branches that were not reached.

Prefer structured control flow and readable expressions. Add compile-time size
checks for recovered structures. Keep casts and offset arithmetic only where
the known layout or compiler behavior requires them.
The project's integer typedefs are in `<nitro.h>`; inspect existing includes
before assuming that a header such as `nitro/types.h` exists.

When only an object's prefix is known, describe it explicitly as a prefix view;
its `sizeof` does not establish the full allocation size. Two objects can share
a prefix while having incompatible tails. The 96-byte primary title prompt and
120-byte rumble prompt share only their first 60 bytes; do not embed the complete
primary prompt as the rumble prompt's base. Check each tail against its own
constructor and consumers, even if a larger placeholder previously fit.

The same rule applies to runtime captures: a buffer read through a `BattleModel`
pointer does not prove that the concrete allocation has the complete declared
layout. Derive the capture extent from its constructor or the verified prefix;
label a larger exploratory memory window separately from an object record.

Decode literal pools as little-endian data, not as ARM instructions. Check
ARM/Thumb state and interworking relocations when comparing calls; a private
comparison script does not replace the full module and symbol checks.
When using Capstone, verify that the listing reaches every intended function's
end. An undecodable literal-pool word can silently stop a combined disassembly
before later functions. Split the input at known function boundaries or enable
`skipdata`, then identify the actual code and data regions from metadata and
references. Skipped bytes are not proof that the remaining code was inspected.

Decompiler output can omit repeated writes to the same GPU FIFO address.
Inspect every native store and preserve its width, value and order with volatile
MMIO accesses. In the credits controller, `0x04000470` receives all three
translation components, and `0x0400046C` receives all three scale components;
the final store alone does not describe either operation. The same applies to
texture-coordinate and vertex sequences. Recover missing writes before trying
to explain a short candidate through compiler scheduling.

Preserve the input loads around those stores too. For a translation sequence,
the native code can cache depth before sending the zero X/Y components. Loading
depth only for the final FIFO write changes the instruction order even though
the three intended values are unchanged. Follow the observed load/store order
with an ordinary local variable; do not add volatile fields to force scheduling.

A stored byte or halfword does not imply a narrow function parameter. Preserve
the full-width argument and the native truncation point when callers pass an
`int`; narrowing the prototype can change caller code and signedness. A table
lookup may use that full-width index before its low byte is stored elsewhere;
preserve the lookup width and signedness separately from the destination field.
The same applies to temporaries: a halfword result can be promoted to `int` for
arithmetic and narrowed only afterward. For example, `(s16)(last_frame - 1)`
preserves a different truncation point from making `last_frame` an `s16` early.
Check the native extension instructions and the callee's parameter type before
introducing masks or casts to fix a size difference.

For example, the Nawatobi console caller passes full-width cursor coordinates;
the console setter narrows them only when storing its byte fields. Declaring
those parameters as `u8` inserts extra masks in the caller. Verify both caller
and callee, plus existing users, when correcting the shared declaration.

The same caution applies to a narrow load from a stack argument. A callee's
`LDRH` can consume the low half of a word passed without caller-side narrowing;
declaring that parameter `u16` can add extension instructions to the caller.
Follow both sides of the ABI and preserve the callee's truncation point. Keep
experimental private declarations separate from a verified shared API change.

Recover return contracts with the same care. A constant left in `r0` at return
does not by itself establish an `int` result: it may be the value of the final
store. Inspect whether callers consume that register and how the callee produces
it. Do not copy an invented `return 1` from an old draft, or change a function to
`void` merely because one caller ignores its result. Correct shared declarations
and verify every affected linked caller when the evidence supports a change.

Likewise, a value left in `r0` by one call does not establish an argument to the
next call. Follow the next callee's incoming register uses before accepting a
decompiler's apparent result-forwarding expression. This matters in the pause
status controller: the helper at `0x020784A0` does not consume incoming `r0`,
although pseudocode can connect it to the preceding call at `0x02077110`.

Check array bounds across all consumers, including adjacent scenes that share a
workspace. A save-menu replay using two panels does not establish that a shared
table has only two entries; the load menu may use another index. Establish the
extent from native data and consumers before declaring the array. Matching bytes
alone do not prove that a reconstructed C access is within its declared object.

Packed fields also need their native extraction semantics. Reuse a verified
shared bitfield view when the instructions and consumers establish its layout;
an apparently equivalent shift/mask expression can emit a different extension
sequence. Check all users and the full linked module after changing a flags
union. Keep arithmetic operations justified by the instructions: the Smash Eggs
signed halfword angle step uses multiplication by -1 (`SMULBB`), whereas unary
negation produces a different instruction. See
[pair motion](../../src/attack_smash_egg_ov015/pair_motion.c) for the matching expression.

Use the recovered C++ virtual interface when native calls go through a vtable.
`BattleModel` and its virtual methods are declared in
[battle_scene.h](../../include/game/battle_scene.h); there is no separate
`battle_model.h`. Its `BattleModelAnimationData` rows are eight bytes: two
frame halfwords followed by a four-byte unknown field. Use that stride when
reading animation ranges in a probe. A C function-pointer approximation can have
the correct size while retaining different call/load ordering. When moving a
unit to C++, check the linkage of every shared C declaration. Use guarded
`extern "C"` declarations
for C APIs, keep C++ class declarations outside them, and verify exported names
and relocations. Do not hide mangled-symbol errors in the comparison script.
Headers without their own C++ linkage guard must first be included inside
`extern "C"`. If another header has already included them outside that block,
their include guard prevents a later wrapped include from fixing the linkage.

Use the actual recovered global type when exposing workspace fields. In the
shop panel callbacks, casting a raw byte-array global to a structure prefix
still produced extra loads; declaring the global with the shared `ShopSceneWork`
type reproduced the native accesses. Its named fields must coexist with the
existing byte-array view used by background setup. Check both field offsets and
the complete workspace size, then run the full matching build for all consumers.
This is evidence about that shared layout, not permission to change aliasing
rules or enumerate arbitrary type casts until one happens to match.

Account for integer promotion and the exact point where values are rounded.
A `u8` or `u16` operand promotes to `int`; an explicit `u32` cast before a shift
may be needed to reproduce a native logical shift. Signed division truncates toward
zero, which differs from an arithmetic right shift or Python's `//` for negative
values. Preserve the native order of division and Q12 scaling, and model signed
division explicitly in runtime oracles. See the verified examples in
[title model entry](../../src/title_ov006/title_sequence_model.c) and
[trail drawing](../../src/title_ov006/title_trail.c).

Python oracles must also reproduce the native width of intermediate arithmetic.
The [display affine helper](../../src/scene_support_ov005/display_2d.c) takes nine
arguments: engine, background, horizontal scale, vertical scale, rotation,
center X/Y and origin X/Y. Pseudocode that omits the final stack arguments is
not a complete prototype. For zero vertical scale it substitutes `0x100000`;
at zero rotation, cosine is 4096, and the native low-32-bit product wraps to
zero before the arithmetic right shift by eight. An independent oracle needs
`s32(inverse * 4096) >> 8`, where `s32` wraps to a signed 32-bit value, rather
than the unbounded equivalent `inverse * 16`. This explains the zero matrix
coefficient without changing already matching game code. Validate the native
lookup-table values and ordered hardware stores as well as the resulting
coefficient bytes; affine origin readbacks can change during rendering.

Similarly, derive task completion from the actual integer recurrence. A close
animation starting at Q12 scale 4096 with step `4096 / 6 == 682` needs seven
scaling updates to reach its clamped zero, plus its one delayed update. A probe
that expects six updates would reject the native behavior. Track the created
task through its terminal flag write and distinguish that from the later free.

Preserve reads across callbacks in their original order: a linked-list callback can
change `next`, and cleanup can change state. Do not cache those fields earlier
merely to simplify the source. Distinguish deleting and non-deleting destructor
entries when reconstructing virtual calls and reporting runtime coverage.

Overlay 5's `func_ov005_0206650c` requests deferred removal by setting mask
`0x0002` in the element's 16-bit flags at offset `+0x08`. It does not immediately
free the object. Model this write separately from later list cleanup; see
[element lifecycle](../../src/scene_support_ov005/element_lists.c). Title and credits tasks
share the 72-byte `MenuElement` view in
[the frontend header](../../src/title_ov006/frontend_scene_internal.h). Reuse it instead
of copying private task definitions into each module or probe.

C comparisons yield `int`, while C++ comparisons yield `bool`; this MWCC setup
can emit different truncation masks for their results. The title sequence cleanup
matches in C with byte-sized flag input and separately scoped loop indices.
Use that type/lifetime evidence before attempting register-allocation changes;
matching only a function's size is insufficient.

For a same-size mismatch, classify the differing words before editing: changed
instructions, register operands, literal values or relocations require different
explanations. Check native load order, expression lifetime and aliasing first.
A helper that reads both inputs before writing an aliased object can preserve
native behavior that sequential field assignments do not express. Accept a
source change only when its data flow explains the difference; defer remaining
register-only mismatches instead of trying arbitrary declarations or casts.
For transformed model drawing, compute the X and Y inputs before assigning
either output field when that is the native order. `const` on an input pointer
alone does not establish that its storage cannot overlap a mutable output.

When several accesses share an embedded structure, recover that relationship
with a typed pointer and retain the native order of dependent loads. Its lifetime
can explain register use across helper calls without artificial register hints.
Narrow a local variable's lifetime to the native loop or branch when the data
flow supports it, and preserve the order of counter and pointer increments.
These changes recovered the title backdrop initializer without altering its
algorithm. A candidate with fewer instructions can instead reflect algebraic
folding, such as combining a negation and addition into one reverse subtraction;
do not mistake that for a relocation-only difference or a completed match.

An address-named data symbol may label an interior field of that same workspace,
not a separate global object. Check the native base load and field offset before
declaring another `extern`. In overlay 8, `0x0207832F` is workspace base
`0x02078290 + 0x9F`; the Game Over controllers read it through the workspace
base also used for selection fields. A separate global reference introduces an
extra literal/load and loses that relationship. Recover the shared field from
its consumers; a decompiler's separate label or an old `padding` name does not
establish independent storage or prove that the byte is unused.

Size request buffers from callee accesses, not just the apparent base type.
`ArchiveReadRequest` is 40 bytes, but `BattleArchive_ReadAsync` also writes the
halfword at offset 40, including on the raw-read path. The title loader uses a
44-byte extended raw request; a full `ArchiveCompressedRequest` is 64 bytes.
See [the shared layout](../../include/game/archive_io.h),
[the callee](../../src/game/archive_compressed.c), and
[the title loader](../../src/title_ov006/title_animation_resources.cpp).
Preserve ownership flags and the native allocator/free pairing for converted
resource tables; a non-null pointer alone does not establish ownership.

The title sequence allocates 59,340 bytes but clears only its 57,288-byte prefix.
The 512 bytes at allocation offset 57,292 are a 32x32 4bpp trail stamp; older
initializer notes incorrectly called them palette data. The rasterizer establishes
their purpose. Its two 24,576-byte destination buffers are linear pixels before
tiled VRAM upload, and the second screen uses a sequence-space y-origin of 244.
Do not infer a 192-pixel screen separation from the display height. Refer to
[the sequence layout](../../src/title_ov006/title_sequence_internal.h) and the trail
module when interpreting these captures; the remaining allocation tail is unknown.

The sequence's 25 backdrops use a 52-byte record with a 48-byte model prefix,
signed 16-bit horizontal velocity and a four-bit group. Their four groups have
4/5/8/8 elements. Construction, release and skip hooks are linked in
[title backdrop initialization](../../src/title_ov006/title_backdrop_init.c); check
current metadata before assuming that the scrolling callback or shared animation
selector has also been reconstructed. Animation IDs alone do not identify the
artwork or establish looping semantics.

Credits use a different coordinate space from the title sequence: screen value
0 adds 224 pixels to Y, before Q12 scaling. The illustration is divided into
32x32 cells held in 1,024 records of 84 bytes. Cell centers add
`(5*column+2, 4*row+2)` pixels to the layout origin; the illustration center
adds `(80, 64)`.
The X and Y table views have a 16-byte record stride. Preserve that stride and
the actual typed workspace object; treating the workspace as a cast byte array
changed the emitted address calculations. See
[credits positions](../../src/title_ov006/credits_positions.c),
[shared declarations](../../src/title_ov006/credits_transition_internal.h), and
[motion setup](../../src/title_ov006/credits_motion.c). Keep native integer operation
order and narrowing when modeling acceleration or easing; floating-point
interpolation is not an equivalent oracle.

Credits effects reuse the motion record's remaining parameter words for
different purposes. Keep shared names neutral until all consumers justify a
single meaning. Illustration loading uses a 32,768-byte texture staging buffer
and a 512-byte palette; the task schedules sixteen texture chunks and a palette
upload before clearing the workspace's `image_loading` field at `+0x8238`.
The current layouts and buffer declarations are in the shared headers above;
the state machine is in [credits image loading](../../src/title_ov006/credits_image_loader.c).
Checking those queued jobs does not establish coverage of the native upload
callbacks themselves.

Small, explained inline-assembly fragments are authorized when a specific
instruction sequence cannot reasonably be reproduced in C. Keep the surrounding
logic readable and verify the entire function. Do not disguise raw instruction
dumps as C to inflate progress. The completed Scene VM's 64-byte height-conversion
fragment is a documented example in [its matching notes](../../docs/research/SCENE_VM_MATCHING.md).
All four VM dispatchers are already linked and byte-identical (53,700 bytes);
their callees still contain work. Check the [VM reference](../../docs/research/SCRIPT_VM_SEMANTICS.md)
before treating older dispatcher research as an open task.

## Runtime verification

Use the optional dependencies in `tools/runtime_requirements.txt`. The current
`py-desmume==0.0.9` bundles DeSmuME 0.9.12. A 0.9.13 savestate can expose plausible
CPU/RAM while rendering incorrectly or hanging on the next frame. Recreate
compatible states from battery saves; do not repeatedly advance a known-bad state.

The private `PiT_SaveStates/` directory contains story checkpoints from start to
finish. Enumerate what is present and select an appropriate save. Keep originals
unchanged and compare their hashes before and after a replay. Store new states,
screenshots, dumps and reports under ignored `build/runtime/` paths.
Use `--battery-save` with a selected `.sav` to cold boot through
`tools/runtime_drive.py`; `--state` loads an emulator snapshot instead. Old
messages mention `PiT/_SaveStates`, but the current checkout uses
`PiT_SaveStates/`. Check the filesystem rather than assuming either path exists.
Run emulator replays sequentially when they share a ROM path or battery-save
backup. Parallel instances can overwrite the same emulator save sidecar. A
different output report directory alone does not isolate that shared state.

Useful title-startup checkpoints, confirmed by cold booting the supplied saves:

| Save number | Checkpoint | Participant mask | Greeting stream ID |
|---|---|---|---|
| 1 | Peach's Castle, before the Refreshroom | 3 | 45 |
| 6 | Koopa Cruiser | 12 | 46 |
| 83 | Star Shrine, before the boss | 15 | 47 |

The local evidence is under `build/runtime/eur_title_startup/`. These cover the
three observed startup selections, not the whole story or empty-slot fallback.
Choose saves from observed branch inputs rather than repeatedly replaying the
same late-game checkpoint. Matching values at several offsets in a battery save
can be duplicated records; verify the active runtime value before treating any
file offset as a universal field location.

- `tools/runtime_drive.py`: deterministic, headless DS input; supports battery
  saves or compatible states, repeated `--action` arguments and screenshots.
- `tools/runtime_probe.py`: execution hooks, registers, memory watches, diffs
  and overlay identification. `--action` and `--frames` are alternatives.
  Use `--allow-state-advance` only with a compatible state.
- `tools/runtime_session.py`: interactive navigation and compatible-state
  creation when automatic input needs investigation.

Example using an already-created compatible state:

```powershell
python tools/runtime_drive.py --rom PiT_eur.nds --state build/runtime/before.dst --action down:60 --action wait:30 --action a:1 --save-state build/runtime/after.dst --screenshot build/runtime/after.png
```

For a reproducible ordinary title-to-load-menu cold boot using checkpoint 1:

```powershell
$titleSave = @(Get-ChildItem -LiteralPath PiT_SaveStates -Filter '1. *.sav')
if ($titleSave.Count -ne 1) { throw 'Expected exactly one checkpoint-1 save' }
python tools/runtime_drive.py --rom PiT_eur.nds --battery-save $titleSave[0].FullName --action wait:1500 --action start:30 --action wait:250 --save-state build/runtime/title_check/after.dst --screenshot build/runtime/title_check/after.png
```

`runtime_drive.py` adds one released-input frame after each action, so this
three-action replay advances 1,783 frames. The route reaches the load menu;
it does not establish that the saved field state has been entered. Use a
function-specific probe for independent object and graphics assertions, and
apply the original-save hash checks described above.

Use ordinary buttons and live RAM observations to navigate automatically.
Inspect screenshots and active overlays before interpreting a trace: a save can
start beside a save block, and A may open a menu instead of advancing the field.
Selected actions and party formations also change which button performs a jump.
Give each entry route a bounded deadline and verify that its dispatch hook ran.
Waiting on an idle field save does not necessarily execute a decoded command;
if it does not, inspect the state and choose an appropriate action or checkpoint.

Guard hooks with the expected overlay/function bytes because overlays reuse
addresses. For return hooks, track the caller and stack pointer so nested calls
do not produce false observations. Check arguments, object fields and results
against independently derived expectations. For graphics, compare the relevant
RAM buffers, VRAM, palettes, OAM and display registers as well as screenshots.
Log ROM/save/state hashes, inputs, entry/return counts and uncovered branches.
Confirm that the evidence's ROM hash matches the current packaged artifact.
Execution hooks fire before an instruction: account for ARM condition codes
before counting a conditional store as a write. For VRAM uploads, derive the
destination from the active bank mapping. Separate function coverage from branch
coverage; an uncalled destructor or allocation-failure path remains unexercised
even when every compiled byte matches.

GPU FIFOs are ordered command streams; reading the register afterward cannot
recover the sequence sent to it. At each guarded native store, check the actual
destination, width and source-register value against an independently built
command list. Verify workspace counters separately, including effects of nested
draw helpers. This establishes the submitted commands, while screenshots and
VRAM/OAM captures establish the observed rendering. State explicitly when a
texture or sprite callback itself remains outside the oracle.

When reusing a private probe, check its symbol names, structure offsets, hook
guards and helper prototypes against the current checkout. A source rename or
recovered common prefix can invalidate a previously successful probe. Track
nested monitored calls separately. Refresh memory after an external helper
returns before resuming the caller's expected writes; that refresh observes
the helper's effect but does not independently verify its implementation.
State which results the oracle actually derives. For packed graphics, a simple
per-pixel or per-nibble oracle is preferable to copying the native packed-word
algorithm and potentially reproducing the same interpretation error.

Task creation can change a source object's list links when the new task is
inserted after it. Resource attachment also writes the owner's release callback
and resource pointer. Account for these helper writes using the recovered
[element list](../../src/scene_support_ov005/element_lists.c) and
[resource owner](../../src/scene_support_ov005/element_lists.c) behavior before asserting that
the entire source or task record stayed unchanged.

An oracle failure may be a wrong expectation. Inspect the native helper before
changing matching game code: for example, `func_02036988` always sets blend-mode
bit `0x40` and writes BLDCNT/BLDALPHA together as a 32-bit value. Read back the
actual register width. Validate raw archive payloads against the extracted
archive; compressed-request completion and size checks alone do not establish
that the decoded bytes were independently verified.

Derive asynchronous completion from the actual queue or loader field. For
example, `GameAudioLoader.active` is at `+0x20`; `+0x1C` is its next pointer.
Check [the audio layout](../../include/game/audio.h) and the native access before using
either in a probe. A nonzero pointer can look like a plausible busy flag.
After correcting an oracle, rerun the full affected replay and retain the
successful evidence separately; do not count the failed attempt as validation.

Keep probe work bounded. Check and release completed per-node snapshots during
long list traversals instead of rechecking all earlier nodes after every call.
Reuse the route and guards without automatically retaining every previous
per-frame oracle. The shop text-index probe retains show/hide, queued-text and
icon checks, but leaves the broader buying-price checks in their earlier probe;
this substantially reduces replay time without dropping checks for the new
index helpers. Preserve those earlier reports and identify checks that were
not repeated. Do not rebuild a ROM at the path an active emulator is reading;
a separate native-relink output does not replace that input file.
An emulator frame boundary can fall inside a monitored function. At the end of
a replay, stop accepting new outermost calls, keep observing pending calls and
their nested helpers, and allow a bounded number of neutral frames for returns.
Record these drain frames separately. Continuing to admit new calls can prevent
the capture from ever becoming idle. Diagnose pending calls from the PC, stack
and native instructions before treating a probe-end assertion as a game hang;
do not discard unfinished calls to make validation pass.

When using prior screenshots as a baseline, compare the relevant buffers and
palettes and state explicitly whether a new image was visually inspected or
only its hashes matched. Label language overrides and other artificial branch
fixtures separately from ordinary save replays.

Optional hardware can affect menu construction. With no Slot-2 accessory, the
title's rumble options can be hidden; repeated button presses alone do not
establish that the menu was reached. A controlled constructor-layout fixture
can exercise its UI without enabling the hardware availability flag. Record
the changed bytes, constructor timing and visible result, and distinguish UI
coverage from physical rumble support. Editing the layout after construction
does not exercise the constructor's hardware-present branch. Likewise, visiting
a phase does not establish that every condition or transition within it ran.
The save object's byte `+0x514`, bit 6 is the rumble preference; see
[rumble control](../../src/game/rumble_control.cpp).
This is an object-relative RAM field, not a universal battery-save file offset.

The debug menu can teleport without fully initializing the destination state.
Controlled RAM edits or temporary decoded-command substitutions are useful
probes, but record exactly what changed and when it was restored. They do not
demonstrate normal gameplay accessibility. Prefer read-only observation after
the controlled setup. Never infer complete branch coverage from a matching ROM.

### Battle model animation starts

The shared [battle model-animation unit](../../src/battle/battle_model_animation.c)
reconstructs `BattleModelAnimation_StartAttached` at `0x0206C104` (68 bytes) and
`BattleModelAnimation_Start` at `0x0206C148` (156). The
[Mix Flowers controller unit](../../src/attack_mix_flower_ov016/pair_controller_animation.c)
adds `Overlay16PairController_StartAnimation` at `0x020C374C` (264), beside the
existing 108-byte scale updater. All four functions in the final build objects
match, including their literals and relocations; the new contribution is 488 bytes.

The factory reads the animation archive through `gBattleContext + 0x48`. Its
index selects a 32-bit archive-relative offset to signed-halfword commands.
The workspace pointer at `0x020C0660` owns the model-animation pool pointer at
`+0xD214` and a four-pointer model table at `+0x1131C`. Only the first table entry
is replaced. The returned 56-byte `GameMatrixAnimationTrack` owns a 40-byte
context with X/Y/Z offsets and four model pointers; its owner pointer is at
track offset `+52`. It is a different record from a 56-byte battle model effect.

The attached wrapper accepts full-width X and signed-halfword Y/Z; the factory
accepts full-width X/Y and signed-halfword Z. Both take a full-width speed.
The factory creates the track with that speed, then stores `speed << 4` into
its signed-halfword speed field. These are separate operations, including for
negative and zero speeds. The attached wrapper stores the track into the owner's
slot and sets its backlink. The shared declaration also serves overlay-25 callers.

The controller binds resource 52 to its two scene objects, sets their animations
and packed flags, positions the secondary object, and attaches animation 849 to
the track slot at controller `+8`. It initializes the Q12 Z position, radius 20,
motion step and phase 1, preserving the phase byte's upper bits.

The private replay `build/runtime/eur_battle_model_animation/evidence_mix83_v5.json`
starts from `build/runtime/eur_attack_helpers/ov17_bros_menu83.dst`, SHA-1
`21d2e64a24b389689627292539103880c6761b47`, derived from story save 83, SHA-1
`2cb577d3008975c390a2f00e2b2cd646e4005c1b`. Nine `down:8, wait:30` pairs, then
`a:8, wait:100, a:8, wait:180, a:8, wait:3600`, plus each action's released frame,
take 4,270 frames. Guarded participant state drives 665 automatic button inputs
through frame 1,900. This replay uses no RAM edits; its starting battle checkpoint
has a controlled origin and does not establish natural story encounter entry.

Each new function runs once at frame 754. Independent checks cover track/context
creation, owner writes, the full 401,416-byte battle and 70,976-byte common-work
allocations, the 17,292-byte attack work, helper arguments, ordered controller
stores and caller stack. The track's context is unlinked and its owner cleared
at frame 775. No watched calls or tracks remain at the end. The final capture
shows the battle command menu against Petey; all 104 original save hashes are
unchanged. The six captures, six RAM/DTCM snapshots and four graphics dumps are
validated. Graphics are observations, not an independent rasterization proof.

The first replay exposed an omitted palette-list insertion during secondary
animation setup. The guarded native write links the new embedded palette record
at scene object `+0xCC` between existing nodes. The corrected oracle observes the
chosen palette position, independently derives both neighbor/root changes and
checks the other 37 existing records. It preserves the failing v1 and diagnostic
v2/v3 reports and their producer variants. Passing v4/v5 runs have identical
inputs, target records and capture/snapshot bytes. Renderer observations use the
actual 440-byte primary-model and 304-byte alternate-renderer allocations;
`sizeof(BattleModel)` does not establish the alternate allocation's extent.
Animation-setter internals, palette-placement choice, sound and later matrix-slot
return remain outside the independent checks described here.

`isolated_arm_cases_v1.json` adds 12 native ARM946 cases on copied live RAM, with
no helper stubs. They cover signed coordinate boundaries, full-width factory Y,
positive/negative/zero speeds, halfword speed truncation, null model pointers,
owner replacement and preserved reserved bytes/bits. All 4 MiB of main RAM,
the caller's stack, return value and SP/r4-r11 are checked; lower helper-stack
contents are observational within the recorded bounds. These cases supplement
the live route and do not establish asynchronous behavior or rendering coverage.

#### Mix Flower participant effect changes

[`Overlay16Participant_ChangeEffectKind`](../../src/attack_mix_flower_ov016/participant_effect_kind.cpp)
at `0x020C5988` selects the participant pose and positions its paired effect.
Kinds below 2 restore idle animation and orbit motion; entering a kind at least
2 from kind 0/1 starts an approach motion and phase 11. Other changes use phase
12. If the actor already has animation 3, those higher-kind branches preserve
the participant phase and position. Every branch still refreshes model scale,
clears the signed-halfword timer at participant `+20` and forwards the full-width
kind to the paired effect. The four 10-byte offset records at `0x020C735C` are
read as signed bytes; the last orbit argument is explicitly converted to `u16`.

The private `build/runtime/eur_high_mix_kind/evidence_mix83_v3.json` repeats the
4,270-frame checkpoint-83 route above without RAM edits: 130 complete calls,
9 approach, 30 holding and 91 idle transitions, covering all four participants
and requested kinds 1/2/3. Full battle, common and attack allocations are checked,
along with receiving models, ordered helper arguments, phase/timer writes,
model scale/mode, position deltas and SP/r4-r11. Animation and motion helper
results remain bounded observations in their receiving records. Palette, render
and texture-list placement is observed in owned nodes; outside neighbor/root
changes are derived while preserving the other nodes' order and payload.
Eight new renderers are observed with their actual 440/304-byte extents.
The first two probes failed overly narrow assumptions about renderer creation
and texture-palette neighbors; their reports and oracle versions are preserved.
The corrected replay returns visibly to the Petey battle command menu, leaves
no pending calls and preserves all 104 original saves.

`build/analysis/high_effort_50_to_55/mix_kind_isolated_v1.json` checks 48 ARM946
cases on copied RAM for the animation-3 shortcut absent from that live route.
Both previous-kind branches, large positive kinds, phase-bit preservation and
model-scale threshold boundaries are covered with the real model getters and
scale helper. The final paired-effect change is explicitly stubbed to check
forwarding only. All main RAM, DTCM, ordered writes and SP/r4-r11 are checked;
the lower 128-byte helper-stack area is observational (64 bytes used).
These cases add no live gameplay, allocator or renderer coverage.

#### Mix Flower effect reservation and first-use visuals

[`Overlay16Participant_PrepareEffect`](../../src/attack_mix_flower_ov016/participant_effect_prepare.cpp)
at `0x020C575C` uses the first idle record in the 32-slot paired-effect array.
If none is available, participant byte `+23` keeps a retry flag in bit 1 and
the low bit of the requested animation flag in bit 2. Bit 0 prevents duplicate
first-use sprite/model effects. Kind `-1` chooses kind 2 for the active
participant and kind 1 otherwise. `Overlay16PairEffect_BeginKind` accepts a
full-width integer; truncation occurs at the stored kind byte, not at its call.

The private `build/runtime/eur_high_mix_prepare/evidence_mix83_v1.json` passes
the same 4,270-frame route with 4,515 complete calls: 3,729 full-pool retries,
782 already-started returns and four first-use visual pairs. All 32 free-slot
indices occur. The 4,399 searches, 120 animated changes, 666 direct initializations
and four calls each to sprite/model/sound helpers have checked argument order.
Full battle/common/attack records, participant flags and owner stores are checked.
Animated changes reuse the preceding caller oracle. Sprite pool links and the
track's pre-scan initialization are predicted; scan results are observations
within the 48-byte track and the manager's first 20 bytes. Model pool links,
the complete initialized 56-byte record, parent and owner are predicted.
Audio internals and later effect release remain outside these independent checks.
There are no RAM edits, all 104 source saves are unchanged, and all six captures
and the complete input sequence equal the preceding verified route. The final
battle command menu was also viewed.

`build/analysis/high_effort_50_to_55/mix_prepare_isolated_v1.json` adds 92 ARM946
cases with synthetic pool phases, flags and arguments in copied RAM. They cover
full-pool flag preservation, first/last free slots, reuse, both default kinds and
full-width signed kinds narrowed at the store. The real search, kind initializer
and paired-effect initializer execute without helper stubs. Existing first-use
flags suppress spawning in these cases. Full main RAM, DTCM, ordered writes and
SP/r4-r11 are checked; the lower 128-byte stack bound is observational (64 used).
These cases supplement the live route without extending its gameplay coverage.

#### Shell attack entry

[`Overlay11Attack_UpdateEntry`](../../src/attack_shell_ov011/shell_attack_entry.cpp)
at ov011 `0x020C57CC..0x020C5AF8` coordinates the initial jump, optional support
intercept and landing effect. The shared work phase is a signed byte at `+17`.
Bit 2 of byte `+16` prevents duplicate landing effects. On completion it clears
that bit, enables controller bit 5 at `+344`, captures the projectile bounds at
`+428`, and passes update callback `0x020C54B4` to `func_ov002_020722ac`.
The callback is a real second argument, omitted by the older entry pseudocode.

Support offsets are signed-byte interior views at `0x020C5F28` and `0x020C5F29`,
with a two-byte stride. Their storage is writable: keep the reload after
`Overlay11Support_StartIntercept` and before obtaining the model to set its
halfword animation offsets at `+92/+94`. This is the immediate intercept helper
at `0x020C3078`, rather than the scheduling helper at `0x020C303C`.

Private reports `build/runtime/eur_high_shell_entry/evidence_red83_v2.json` and
`evidence_green83_v2.json` use the established checkpoint-83 battle menu and
ordinary buttons, with no RAM edits. They exercise configurations 0 and 1,
respectively; each checks 34 completed calls: phase 0 once, phase 1 nine times,
phase 2 twenty-three times and phase 3 once. Each reaches one landing effect and
one callback handoff. The 2,150/2,110-frame routes return to the visible command
menu. Both final captures and their earlier attack-menu selections were viewed;
all 104 original saves are unchanged.

The caller oracle checks the entire 612-byte attack allocation, the callback
actor's 136-byte record, receiving 260-byte scene objects and selected renderer
records between helper boundaries. It predicts caller stores, helper argument
order, channel/model query results, callback replacement and SP/r4-r11. Motion,
animation, resource and support internals are bounded observations in their
receiving records; the arc duration and twelve collision-bound bytes are also
observed. Other battle/global memory, sound and rasterization are outside this
independent model. These routes both have a support actor; they do not cover
the no-support branch or establish natural story entry or complete lifetimes.

The preserved first Red Shell replay failed at frame 382 because the oracle
omitted the support updater's phase-6 store to projectile byte `+419`. Native
instructions show bits 4-5 receive `support.side + 1` when motion channel 3
finishes. Version 2 predicts that store and bounds the associated follow-motion
observation to the receiving scene object; both reruns pass without game changes.

#### Shell support update and boost input

[`Overlay11Support_Update`](../../src/attack_shell_ov011/shell_support_update.cpp)
at `0x020C33E8` updates the optional support character. Its 28-byte record uses
an unsigned button halfword at `+20`, a signed sound-handle offset at `+24`,
state/side/formation bits at `+26` and a four-bit resting animation in bits 1-4
of `+27`. On catching the shell, projectile byte `+27` bits 4-5 receive
`support.side + 1`. The follower call takes `(shell, support_object, 3)`.
Boosting advances a signed counter and emits the two effect types at separate
table-defined intervals. Expiration restores the resting animation and scale,
stops a nonzero sound handle and returns to the input-wait state. Scale arguments
are signed halfwords, matching the resident scale helper's definition.

The private reports under `build/runtime/eur_high_shell_support/` contain four
passing replays: `red83_v2`, `green83_v2`, `red83_boost_v2` and
`green83_boost_v2`. The ordinary routes take 2,150/2,110 frames and reproduce
the preceding entry replay's complete action sequence and capture hashes.
Each executes 205 support updates: 23 catching, 134 waiting for input,
16 returning and 32 idle. The additional routes retain those frame budgets and
add guarded support-button presses without RAM edits. Red uses ten automatic
presses and checks nine boost starts, 116 boost ticks and seven expirations;
Green uses eight presses and checks seven starts, 120 ticks and five expirations.
The update counts remain 205 per route. Final battle menus were viewed, no calls
remain pending and all 104 source saves are unchanged.

The oracle checks the complete 612-byte attack allocation, actor records,
260-byte scene objects and 440/304-byte renderers between helper boundaries.
It independently predicts the updater's decisions, complete helper ABI, ordered
own stores, signed division/remainder, model/channel query results, scale-helper
writes and SP/r4-r11. Resource, animation and motion effects are observations
bounded to the receiving records. The selected attached-effect slot's eight
bytes are observational; other pool/global changes, audio, rasterization and
object lifetimes are not independently modeled. Renderer/palette/texture
placement inside a receiving record is observed, while roots and unowned
neighbor links are derived and checked. States 3-5, a null support object and
the return state's non-idle actor branch are not covered by these live routes.

The first `red83_v1` probe failed at frame 533 because hiding the support also
changed a render-list neighbor. Its report and original producer/oracle remain
preserved. Version 2 includes the shared list-topology checks; all four routes
pass without game-code changes. All 22 functions from the nine linked Shell
source objects match after the shared-header changes; full verification retains
the original EUR ROM, zero native differences and 107 passing tests.

#### Shell entry wrappers and object hiding

The Green and Red entry points in
[`ov11_attack_initialize.cpp`](../../src/attack_shell_ov011/ov11_attack_initialize.cpp)
tail-call the shared initializer with resource/variant pairs `0xc0000047, 1`
and `0xc0000048, 0`. The projectile hide helper tail-calls the animation setter
with `(object, -1, -1)`. Support hiding first checks for a null object, then
hides its animation, reloads the object pointer and clears its signed halfword
`effect_anchor_z` at offset 234.

Private checkpoint-83 reports under `build/runtime/eur_high_shell_lifecycle/`
(`evidence_red83_v1.json` and `evidence_green83_v1.json`) check one entry wrapper
and both hide helpers per route. Tail target, arguments, unchanged LR/SP and
preserved r4-r11 are checked; the initializer's resulting 612-byte work area
has the expected resource and variant. Its internal writes remain observational.
Hide checks cover the full work area, embedded 260-byte receiving object,
440/304-byte models, and the support helper's own halfword store. Animation
writes are observed only in the receiving records; renderer, palette and texture
list roots and neighboring links are derived independently. The null-support
branch was not reached. Other global writes, allocation lifetimes and rendering
internals are outside this oracle.

Both ordinary routes use unchanged inputs and no RAM edits. Their full capture
sequences match the preceding support runs; final captures show the battle
command menu. The existing entry and support oracles also run during these
replays. All original saves are preserved.

#### Mix Flower participant exit

[`Overlay16Participant_BeginExit`](../../src/attack_mix_flower_ov016/mix_participant_exit.c)
at `0x020C4F9C` chooses resource 56 or 57 from the participant variant's low
bit. Variants 0/1 use component animation 2 and move to `entry_offset_x + 288`;
variants 2/3 use animation 3 and move to `entry_offset_x - 32`. Both use channel
3 for eight updates, preserve the object's freshly loaded Y, play sound 61 and
enter phase 9. The two model-flag stores stay separate: bit 10 selects facing,
then bit 11 is cleared while preserving the other bits.

The private `build/runtime/eur_high_mix_exit/evidence_mix83_v1.json` passes the
same 4,270-frame Mix Flower route. All four participants exit at frame 2,028:
variants 2/3 target X=16, variants 0/1 X=336; their Y values are 112 or 152.
The exit oracle independently checks ordered flag/phase stores, destinations,
complete helper arguments, model lookup and SP/r4-r11. It checks the complete
attack allocation, the 260-byte receiving scene object and its 440/304-byte
renderers between calls. Resource/motion helper writes are observations within
that object; animation writes are observations within the object/renderers.
Other battle/global allocations, palette neighbors, helper internals, sound,
rasterization and object lifetimes are outside this exit oracle's claims.

All 665 automatic button inputs and six capture hashes equal the preceding
verified reservation route; the final battle menu was viewed. No RAM was edited,
all 104 source saves are unchanged, and no calls remain pending. The shared
animation, effect-kind and reservation oracles also passed again. All 46
functions from the 14 linked Mix Flower source objects match after the added
declaration; the full build produces the original EUR ROM and passes 107 tests.

The initial exit candidate differed only by swapping the cached work/object
registers. Declaring the work pointer before the object pointer, while retaining
the original order of their assignments, produced the complete native function.
Declaration order can therefore explain a register mismatch in this compiler;
it is not a general allocation formula. Check a specific dataflow hypothesis
against the whole function instead of enumerating source permutations.

#### Four-model table updates

`BattleModelAnimation_SetModels` at `0x0206C1E4` adds 92 matching C bytes to the
same animation unit. It returns the four-pointer table at
`read32(0x020C0660) + 0x1131C`. The first slot is always assigned, with pointer
sentinel `-1` converted to null. For slots 1 through 3, `-1` preserves the old
pointer. Null explicitly clears any slot. The Battle VM and overlay-25 caller
now use the shared pointer-typed declaration and its actual table-pointer return.

The checkpoint-55 entry replay
`build/runtime/eur_battle_model_table/evidence_entry55_v3.json` runs the setter
once at frame 70, with arguments `[0, 0x0228C570, -1, -1]`, then reaches the
visible Jump command menu against Petey at frame 706. The focused oracle checks
the full 70,976-byte workspace, its allocation header and root, ordered table
writes, unchanged slots, surrounding caller stack, return pointer and SP/r4-r11.
The existing controlled encounter fixture restores the field command and cursor
before native battle start. This remains a controlled route using story save 55,
not evidence of natural encounter entry. Original save hashes are unchanged.

The first probe incorrectly required execution hooks at conditional stores whose
conditions were false. The preserved v1 report fails that assertion. The corrected
oracle requires the stores selected by the entry arguments and verifies skipped
slots remain unchanged. Passing v2/v3/v4 have identical table-call records,
target registers and DTCM. V3 additionally saves a compatible battle checkpoint;
v4 repeats the same probe source. Execution-hook delivery differs from merely
visiting an instruction address in a disassembly.

Final captures differ across these replays, including the unchanged-source
repeat. At both captured function entries, six main-RAM bytes differ outside
the table workspace: `0x02060E04..05`, `0x020CD0AC`, `0x020CD0B0` and
`0x023FFDED..EE`. The checked table workspace and final mapped VRAM, palette and
OAM dumps agree. The cause of the different rendered images remains unconfirmed;
do not claim image-identical replays from the matching setter checks.

`isolated_v1.json` supplements the live call with all 16 sentinel combinations
and an all-null case on copied RAM. Each uses native ARM946 execution without
stubs, checks every main-RAM and DTCM byte, ordered writes and preserved registers,
and seeds nonzero table entries to expose preservation versus clearing. Neither
this leaf setter nor these checks establishes subsequent rendering or model use.

### Battle VM directional motion

The [motion handlers](../../src/battle/battle_vm_motion.cpp) reconstruct three
contiguous functions at `0x02078460..0x020787F0` (912 bytes): ballistic motion,
scaled accelerated motion and motion solved from a peak distance. Their six
Battle VM opcodes are `0x062..0x067`, with and without duration writeback.
The [shared operand decoder](../../src/battle/battle_vm_operands.h) also serves
the main dispatcher; both final objects remain byte-identical, including the
full 19,168-byte dispatcher.

Each handler resolves its object ID as an unsigned halfword, then conditionally
combines literal operand pairs into a signed 32-bit value and divides by 16
toward zero. The mode bit determines whether a decoded variable already supplies
that value. Peak-distance motion decodes argument 7; ballistic motion decodes 6;
scaled motion decodes 6 and 8 in that order. The getter precedes these mutations.
The writer branches pass the native motion helper's duration to `VM_WriteVariable`.
The schema names the recovered velocity, acceleration, distance and duration-choice
operands; the last operand is a used halfword flag, not an unused field.
Shared declarations preserve the handlers' void interface and the helper's
integer result, without assigning a meaning to an incidental return register.

Two private replays in `build/runtime/eur_battle_vm_directional/` cover all three:

- `evidence_petey55_v1.json` starts from the controlled encounter checkpoint
  produced by the model-table entry replay. Three pairs of Mario/Luigi Jump
  confirmations (`a:8, wait:100, a:8, wait:1000`, then the same with `b`) run
  6,720 frames, including released frames. Six ballistic calls and one
  peak-distance call pass, all using literal operands and duration writeback.
- `evidence_shrowser103_v1.json` uses the compatible story-103 checkpoint
  `eur_story_103_bowser6.dst` at Shrowser's introduction. Twenty-four
  `a:8, wait:90` pairs and `wait:2400` run 4,801 frames. Eighteen scaled-motion
  calls pass with literal operands and no writeback. Its input record pins the
  existing dialogue-advance producer; the full earlier navigation history was
  not reverified by this replay.

The independent oracle predicts object lookup, decoded command words, signed
32-bit arithmetic, integer square-root results, duration selection, motion-list
membership, channel parameters, frame/deferred deltas and VM result stores.
It checks the full 401,416-byte battle allocation, motion-list root, command,
VM, caller stack, helper arguments/results, SP and preserved registers. All 25
calls return before the replays finish; all 104 original saves remain unchanged.
The final captures show continuing battles. Later animation and rendering are
observations, not an independent rasterization or complete battle-lifecycle proof.

`isolated_v2.json` adds 30 native ARM946 cases on copied live RAM. These cover
variable operands, negative packed remainders, both writeback choices, opposed
motion, distinct shorter/longer roots, high-half ID truncation and interactions
with earlier active/later deferred channels. No function is stubbed. DS square-root registers
use an immediate unsigned integer model, so hardware latency and IRQ timing are
outside these cases. Main RAM and caller DTCM are checked independently; lower
helper-stack bytes remain observations within the recorded bounds.

Static discovery found 459 uses of the broader `0x05E..0x067` families. Resolve
named opcodes through the schema when scanning JSON; treating them as numeric
strings produces a false empty inventory. Shrowser also exercises the separate
vertical handler at `0x020789EC`, while the combined movement handler at
`0x020787F0` has a separate Shrowser script entry at offset `0x2344`. See the
[vertical-motion reconstruction](#battle-vm-vertical-motion) for their own checks.

### Battle VM vertical motion

The [motion unit](../../src/battle/battle_vm_motion.cpp) now also includes
`BattleVm_MoveWithVerticalDuration` at `0x020787F0` (508 bytes) and
`BattleVm_StartVerticalMotion` at `0x020789EC` (456 bytes). The five contiguous
handlers total 1,876 matching C++ bytes through `0x02078BB4`; the full dispatcher
still matches. These two functions implement opcodes `0x05E..0x061`.

Both combine literal halfwords in place. The first parameter is divided by 16
toward zero before reaching the vertical solver. Profile 4 also divides the
second parameter by 16 and selects velocity/acceleration motion; other profiles
pass that packed parameter directly as the arc curve. The vertical-only wrapper
compares the profile's low halfword and resolves its object before decoding.
The combined wrapper compares the full word and decodes before object lookup.
It first starts vertical motion, then passes that solver's duration to `MoveTo`
on the independently selected position channel. Both optionally write the duration
to the VM result variable. The schema names these channels and target Z explicitly.

The independent oracle models both vertical solvers, immediate position changes,
list insertion, channel replacement, deferred deltas and the following `MoveTo`.
It checks the full containing battle allocation, list/root words, command, VM,
caller stack, ordered helper arguments/results, SP and preserved registers.
The native helpers execute in every case; none is stubbed.

- `build/runtime/eur_battle_vm_vertical/evidence_shrowser103_intro_v2.json`
  runs 1,801 frames from the compatible story-103 introduction checkpoint:
  twelve `a:8, wait:90` pairs, then `wait:600`, including released frames.
  Two vertical-only calls at frames 303 and 315 pass with literal operands,
  profile 3, no result write and duration 32. The final capture shows the ongoing
  Shrowser battle. V1 is a separate passing replay; V2 adds the final capture
  and a compatible output checkpoint. All 104 original saves remain unchanged.
- `isolated_v1.json` checks 43 native ARM946 cases on copied Shrowser RAM/DTCM:
  21 combined-motion cases and 22 vertical-only cases. These cover both profiles,
  variable/literal decoding, signed remainders, result flags, immediate movement,
  negative/zero durations, discriminants, ID truncation and active/deferred or
  shared channels. Profile `0x10004` selects different helpers in the two wrappers,
  confirming the full-word versus halfword comparison. Every main-RAM byte and
  caller DTCM is checked; lower helper-stack bytes remain bounded observations.

The combined wrapper has copied-state execution coverage only. The input-only
`shrowser103_auto_v1` attempt never saw the generic party jump-enable flag, sent
no inputs and failed when the battle allocation ended at frame 6,179. Preserve
that failed route and its producer; it does not establish reflection gameplay.
The intro replay's earlier checkpoint history was not reverified. Integer SQRT
modeling excludes hardware latency and IRQ timing, and the checks do not prove
later animation, rendering or completion of the battle. Artifact and final-build
checks are recorded in `artifact_validation_v1.json` in the same private directory.

### Alternate model effects and VM model sources

[Alternate model effects](../../src/battle/battle_alternate_model_effects.cpp)
cover `0x0206C2D4..0x0206C520` (588 matching C++ bytes). The attached wrapper
binds the returned task to an owner slot. The factory allocates a 304-byte model
from the heap at battle context `+0xE15C`, initializes its palette/controller,
starts an animation, and stores its position and scale. This is a different heap
field from `resource_heap_id` at `+0xE158`. The shared task pool at `+0x945C`
contains 192 slots of 44 bytes: a 12-byte task prefix and a 32-byte payload with
model pointer, 20-byte palette record and three signed coordinates.

The factory accepts full-word resource/animation IDs, X/Y and scale, and signed
halfword Z. The attached wrapper also narrows Y. Resource lookup uses the low
16 bits; animation selection uses the low 8 bits. Native resource setup forwards
a fourth argument from the load state's `resource_id`, omitted by pseudocode.
The updater subtracts camera offsets, clamps negative depth, and reacts to model
flag bit 2 by unlinking the palette, stopping and deleting the model, clearing
its pointer and disabling the callback. Actual task-pool return happens later.
The cached model pointer for the paired X/Y stores, integer promotion in the
depth clamp, and two separate nullable deletion guards reproduce the native code.

[Model-source resolution](../../src/battle/battle_model_source.cpp) adds 52 bytes
at `0x0207905C`. Positive signed IDs are narrowed to 16 bits and resolve through
the scene object's alternate-model pointer at `+0xC4`. Nonpositive values pass
through unchanged. The dispatcher now uses the shared typed declaration; its
complete 19,168-byte body still matches.

Runtime evidence is private under `build/runtime/eur_battle_alternate_effects/`:

- `evidence_controlled55_v4.json` runs 1,120 frames with the previously tested
  Petey Jump inputs. Four resolver calls, one factory call, seven update calls
  and the shared model-table setter pass their independent caller-state checks.
  Temporary opcodes `0xBC` and `0xB8` replace two waiting `0x74` commands while
  the enemy auxiliary script is nonnull. The original commands, descriptor-derived
  rewind, yield result and model table are restored before VM continuation.
  Descriptors are 32-bit entries at `read32(vm + 12)`, not halfword entries or
  the VM's first word. The temporary model finishes naturally; no completion-flag
  fixture fires, and its task returns to the 192-slot free list. The final capture
  shows the ongoing battle and command wheel. All 104 battery saves are unchanged.
- `isolated_v4.json` passes 12 groups on copied live RAM/DTCM and both display
  engines' MMIO readbacks: six factory variants, an attached call, twelve updates
  spanning active/completed state, and six resolver sentinel/ID cases. Native
  helpers execute without stubs. Factory cases vary high argument bits, negative
  depth/scale and resource selection. Resolver cases reuse the last copied state
  after cleanup, with a synthetic alternate-model pointer.

Checks cover the full battle allocation and caller frames, owner/task fields,
model stores and palette-list links. Renderer/constructor internals and heap
effects outside those ranges remain bounded observations; copied MMIO is not a
hardware timing model. No ordinary selection of the injected commands or
independent rasterization is claimed. The 81,854 exported battle commands contain
no `0xB8..0xBB` calls and four `0xBC` calls, so the factories' normal story route
remains unestablished. Earlier entry/idle probes reached no targets; V3 used an
incorrect descriptor address despite obtaining the same rewind. Keep those
attempts as historical artifacts, not accepted coverage. The accepted run is V4.

### Battle model stop, animation and descriptor helpers

The primary and alternate model stop helpers call their renderer's stop/release
methods, then clear animation-active bit 8. The alternate path also unlinks a
non-null sprite palette. `BattleModel_StartAnimation` clears trigger bit 2,
passes the low eight bits of the animation ID and signed low sixteen bits of the
argument to the virtual animation method, and forwards its result.

[Primary descriptor setup](../../src/battle/battle_model_descriptor.cpp)
initializes a 96-byte work record once: it copies the shared 88-byte renderer
descriptor, adjusts screen/allocation flags, clears two state bytes and the
controller pointer, and retains the two intervening padding bytes. The ready
byte makes later calls return zero without changing the record.

Private `eur_battle_model_controls/evidence_brat50_round_v1.json` verifies all
four helpers during the same 2,820-frame Shrooboid Brat replay: 358 calls in total.
The existing capture-effect checks also pass, with identical screenshots.
Wrapper flags, argument widths, ordered calls, palette unlinking and descriptor
contents are independent checks. Virtual renderer effects on the 128-byte model
prefix are observations at call returns. All live descriptor calls find it ready.
`isolated_v1.json` passes 25 copied-RAM cases, including cold initialization,
nonzero ready bytes, animation truncation and null/single/head/middle/tail palettes
on both screens. Virtual entries use native no-op callbacks to isolate wrapper
contracts; palette unlink, flag update and copy helpers execute normally. Checks
cover all mapped memory except 256 bytes of call-stack scratch, restored SP and
callee-saved registers; non-initializer nonstack writes are also checked in order.
These fixtures add no navigation or renderer-implementation coverage. All 104
original saves remain unchanged.

### Resident window opening and skin selection

`GameWindow_Open` and `GameWindow_DrawSkin` add 528 matching resident bytes.
The latter selects front/back buffers and aligned or shifted skin rendering.
The shifted renderer uses the manager's 3,744-byte scratch region at +96;
it lies inside the complete 3,908-byte manager. Skin indices 0 through 3
reference the four native entries at 0x0205671C.

Private `eur_window_skin_open/evidence_cold1_v6.json` records one natural skin-1
call while talking to Toad from the original first story save, after 3,671
frames of normal keypad navigation. Both 4,224-byte pixel buffers and the
manager scratch are bounded observational renderer outputs; buffer selection,
helper arguments and the remaining manager/window bytes are independently checked.
The dialogue and returned field view were visually inspected. Earlier routes
missed the targets; the first successful entry exposed the initially omitted
scratch writes in the probe, corrected in the frozen v2 oracle.

`isolated_v2.json` passes 80 ARM946 cases: 64 skin/buffer/alignment combinations
and 16 opening cases. Native rasterizers, clearing and tilemap writing execute.
Raster pixels and scratch are observational; clearing and tilemap values are
independently derived. Opening has isolated coverage only: preparation is
stubbed to return an already prepared slot or failure, and nonzero decoration
is stubbed. Checks include mapped memory outside 512 stack bytes and the named
observational regions, ordered calls, results, SP and r4-r11. Both ROM builds,
both source objects and 81 tests pass; all 104 original saves are unchanged.

### Battle effect controls and shop equipment markers

Thirteen functions add 1,416 matching bytes and bring matching C/C++ to
781,856 / 1,563,700 (50.0004%). Battle code now includes ground-ripple creation,
capture restart, chain actor/sequence controls, chain stop/detachment, segment
summation and sub-BG0 tile/map uploads. Shop code adds equipped-item markers,
panel-model drawing, selected-model removal and applying selected equipment.
The ground-ripple task uses the existing 44-byte pool record; its initialized
payload is only 14 bytes. Chain controls reuse the 7,088-byte overlay-25 work
layout. Preserve the actor argument's unsigned-halfword ABI in the property VM.

Private `eur_final_controls/isolated_v3.json` checks 372 ARM946 cases across all
13 functions. Actor lookup/status, allocation, queues, clearing, selection,
equipment application, deferred task removal and complete draw-list insertion
execute natively. The independent model checks full mapped RAM/DTCM/MMIO outside
256 stack scratch bytes, relevant allocations, arguments, returns and SP/r4-r11.
Four transfer cases use explicit immediate-copy stubs for the two VRAM helpers;
their byte ranges and the tile callback's ordered 32-bit scroll-register write
are checked, but DMA, bank mapping, timing and rasterization are not modeled.

The save-65 selling route (`evidence_shop65_sell_v3.json`) checks 32 marker calls
with full native guards, task/sprite/save/panel state and restores the temporary
shop-entry command. This route starts on consumables: the checked marker calls
take the not-visible path. Drawing and the other functions are covered by the
isolated cases. Screenshots confirm the selling menu and visible field return.
The earlier buying routes missed the targets; their failed assertions remain
recorded. Both ROM builds and 81 tests pass, all 104 original saves are unchanged,
and no inline assembly was added.

### Battle point chains and ring transition drawing

Four functions add 1,348 matching bytes. The ring transition emits concentric
ellipses using the original trigonometry and alpha tables. Point-chain update
anchors Q8 particles to projected scene objects, adjusts an attached endpoint
and invokes five solver steps before drawing. Resource model effects use the
shared 64-slot task table; their y coordinate is a signed halfword, consistent
with the attached factory and the VM's projected position record.

Private `eur_battle_links_curtain/isolated_v1.json` checks 327 ARM946 cases:
38 ring phases, all 65 slot-search outcomes, 32 solver-wrapper combinations and
192 chain-anchor cases. Original lookup, projection, matrix upload and immediate
or deferred object adjustment execute. All ordered ring GPU writes are compared
against independent calculations. Full battle context, 28-byte task records,
16-byte particles, scene slots and mapped RAM/DTCM/MMIO outside 256 stack bytes
are checked with arguments, returns and SP/r4-r11. Unreconstructed chain solver
and drawing calls are explicit no-op stubs; the slot wrapper uses a synthetic
44-byte task with owner binding. These checks do not model rasterization or
hardware timing. The normal 3,110-frame Chomp route did not invoke the resource
slot wrapper; its failed coverage assertion is retained. No live coverage for
these four functions is claimed. Both ROM builds and 81 tests pass; all 104
original saves are unchanged. No inline assembly was added.

### Field camera motion setup

Four C++ functions add 1,428 matching bytes: timed and speed-based entity
tracking, timed movement and movement with acceleration/deceleration. They use
the shared 52-byte camera record, preserve unrelated flag bits, clamp absolute
or relative targets and retain native Q12 arithmetic. The profile setup applies
a minimum velocity of one when an active axis rounds to zero. C++ bitfield
assignment reproduces the native one-bit truncation without inline assembly.

Private `eur_field_camera_setup/isolated_v1.json` checks 193 ARM946 cases on
copied live RAM: default/external records, active-axis combinations, relative
mode, coordinate limits and option bits. Full 11,216-byte areas, 52-byte camera
records and 1,312-byte entities are checked, along with helper arguments,
returns, SP/r4-r11 and all mapped RAM/DTCM/MMIO except 256 stack scratch bytes.
Native direction lookup and signed division execute. Positive Q12 division and
the unreconstructed profile-vector helper are explicit stubs; synthetic vector
outputs test the caller's minimum-speed corrections. Hardware timing is outside
this check. The 4,111-frame save-1 replay reached none of these four functions;
its failed coverage assertion is retained, and no live camera coverage is
claimed. Both ROM builds and 81 tests pass; all 104 original saves are unchanged.

### Pocket Chomp adult movement and velocity ratios

Four functions add 576 matching bytes. The adult exit chooses a side and derives
travel time from the current speed. Jump setup uses the remaining horizontal
motion duration and the scaled target height, then resets the input grade.
A visibility callback writes render state 31 in phases 1 and 2. The battle VM's
ratio helper derives initial velocity and acceleration with signed truncation.

Private `eur_chomp_adult_motion/evidence_chomp83_v1.json` checks 17 jumps and
17 exits during 3,110 frames from save 83, using 17 guarded automatic keypad
presses. Both exit directions are reached. Full work/controller records, target
lookup, threshold tables, arithmetic results, caller stores and helper arguments
are checked. Movement/animation effects on scene objects and motion-list links
remain bounded observations. The final battle image was inspected visually.

`isolated_v1.json` checks 326 ARM946 cases: all visibility phases, both exit
directions, threshold extremes, elapsed-time rounding, four motion channels and
signed velocity ratios. Native lookup, threshold selection, height scaling and
division execute fully; 396 movement/animation calls are explicit stubs. Full
mapped RAM/DTCM/MMIO outside 256 stack bytes and the calling convention are
checked. Visibility and velocity-ratio coverage is isolated-only. Both ROM
builds and 81 tests pass; all 104 original saves are unchanged.

### Pocket Chomp attack callbacks

[`chomp_attack_update.cpp`](../../src/attack_pocket_chomp_ov018/chomp_attack_update.cpp)
reconstructs the adjacent update and entry callbacks at `0x020C6A20..0x020C6D68`.
The 656-byte work area contains two 36-byte adult controllers at offset 304,
two 32-byte support controllers at 376, the 192-byte Chomp at 440 and active
participant pointers at 632. The entry callback updates adults and supports
as pairs; the main callback updates all adults before all supports. Preserve
that order. Once the center and adults finish entry, it selects a target and
installs the main callback. Subsequent rounds choose participants through
the selection helper. The final phase returns the backdrop, hides and clears
participant objects, resets the actor callback and detaches the work root.
This root clear does not itself free the allocation.

Private `build/runtime/eur_high_chomp_update/evidence_chomp83_v6.json` passes
4,111 frames from checkpoint 83 without RAM edits. Its first 3,110 frames use
the previous Chomp route's identical inputs, 17 automatic keypad presses and
capture hashes. A further 1,001 neutral frames let the attack finish; the final
capture shows the battle command menu. All 2,922 callback invocations are checked:
entry phases 0/1 run 1/160 times, main phases 0/1/2 run 2,726/1/34 times.
The route reaches 18 further rounds, one stop transition and one final cleanup.
The no-target branch and unsupported phase values were not reached.

The oracle independently checks loops, phase decisions, ordered helper arguments,
own stores, object lookups, idle-query results, callback-reset writes, SP and
r4-r11. It checks the full work allocation, 136-byte party records, embedded
260-byte scene objects, 440/304-byte models and surrounding renderer, palette
and texture lists. Child-controller results remain observations within named
participant/work fields and attack object/model records; target selection is
also observed. Newly queued 24-byte texture records and their placement are
observed, while existing neighbor/root links are derived. Damage, other globals,
motion-list topology, allocation lifetimes, sound and rasterization are outside
this oracle. All 104 original saves retain their hashes.

Earlier failed probes are preserved: missing child rating fields, newly queued
texture nodes, and the collision helper's stop-bit write. The original automatic
route also ended before cleanup, motivating the neutral extension. These were
probe corrections and coverage extensions; the reconstructed game code did not
change. Both actual compiled functions match, and the full build retains the
golden EUR ROM, zero native-relink differences and 107 passing tests.

### Pocket Chomp phase updates

[`chomp_center_update.cpp`](../../src/attack_pocket_chomp_ov018/chomp_center_update.cpp)
reconstructs the full 1,008-byte updater at `0x020C4518..0x020C4908` in C++.
It advances entry/reveal/run/exit phases, waits for motion channels, handles delayed
launches and responds to collisions. Pending-launch and alternate-bounce timers
are processed after the phase switch, including after a helper changes the phase.
The sound helper at `0x020C2C14` accepts a full-width ID and narrows it internally;
narrowing the caller's local value produces two extra instructions. Work offset
632 points to the active 36-byte adult controller, so the distance helper now
uses that type instead of the 32-byte support type. All 53 functions across the
18 compiled Chomp objects remain exact after the shared declaration changes.

Private `eur_high_chomp_center/evidence_chomp83_v2.json` checks 2,922 invocations
over 4,111 frames with no RAM edits. Phase counts are 0:834, 1:1868, 4:60, 6:44,
7:22, 8:9, 9:13 and 11:72. It checks 19 delayed launches, 18 completed rounds
and one collision response. Inputs, automatic presses and every capture match
the prior outer-callback replay; the final battle command menu was inspected.
The full 656-byte work allocation, party/scene/model records and neighboring
renderer/palette/texture lists are checked. Own stores, timer/phase decisions,
ordered helper arguments, sound visibility, distance clamping, speed lookup,
motion queries and the collision stop bit have independent expectations. Child
motion/animation writes have bounded observations; collision geometry, other
globals, resource lifetimes, screen/audio output and rasterization are unproved.

The live route does not enter phase 2 or expire an alternate-bounce timer.
`isolated_v2.json` supplements it with 76 copied-RAM ARM946 cases covering all
32 phase values, motion-query outcomes, timer boundaries, directions, speed
threshold extremes and collision return values. Lookup/query helpers execute
natively; other child helpers are explicit stubs. The test checks ordered own
stores, arguments, SP/r4-r11 and all mapped RAM/DTCM except 256 stack bytes.
This proves caller branches under those fixtures, not live child transitions.
Failed versions preserve an overly broad live phase assertion and Unicorn hooks
for conditionally skipped ARM stores; the latter now honors CPSR conditions.
Both ROM checks pass, 107 tests pass and all 104 original saves are unchanged.

### Pocket Chomp participant updates

[`chomp_adult_update.cpp`](../../src/attack_pocket_chomp_ov018/chomp_adult_update.cpp)
and [`chomp_support_update.cpp`](../../src/attack_pocket_chomp_ov018/chomp_support_update.cpp)
add 1,824 matching C++ bytes. The adult controller advances jumps, filters the
active support's button, searches input grades from 4 down to 2, then dispatches
landing/damage/return helpers. Phases 2/3 and 4/5 deliberately fall through to
shared logic. The support controller handles attacks, swings and hiding. The
verified 656-byte work layout is shared with the outer callbacks. All 55 Chomp
functions across 20 compiled objects remain exact; no inline assembly was added.
Capturing both input-window bounds before the test preserves the original eager
loads. Keeping the timer as a field expression avoids the register exchange
introduced by an unnecessary additional local.

Private `eur_high_chomp_participants/evidence_chomp83_v3.json` checks 11,688 calls
over the ordinary 4,111-frame route. Its inputs, 17 automatic presses, captures
and final graphics equal the previous Chomp replay. It reaches 17 grade-4 inputs
and 19 hit dispatches: two later misses occur after automatic input stops.
`evidence_support83_v3.json` checks another 1,992 calls on a second 4,111-frame
route, with guarded support-button presses and no RAM edits. It reaches 14 input
triggers, one target-range hit, five attack expirations, seven swing impacts and
six completed swings. Both final captures show the battle command menu.

For automatic support input, phase 1 belongs to the active support pointer at
work offset 636. The other support can swing in phase 3, but its button is not
filtered by the adult input checker. Press it only when no adult is in phase
2/3 with grade zero; otherwise the extra button can count as a wrong input.
The initial support route omitted that participant and never reached phase 4.
The successful route adds actual keypad input, without changing game RAM.

The oracle independently checks phase/timer decisions, input filtering, timing
rows and grades, ordered calls/stores, target/trait queries, enemy flag writes,
sound visibility and SP/r4-r11. It checks the full attack work, party records,
the target enemy's 672-byte prefix, embedded scene objects, models and neighboring
graphics lists. Child damage/motion/animation outputs remain bounded observations.
The support-hit helper publishes a feedback effect at work+12 and changes the
enemy record; an earlier probe lacked these observations. Allocation lifetimes,
other globals, screen/audio output and rasterization remain outside this oracle.

`isolated_v1.json` adds 149 ARM946 cases on copied RAM/DTCM, including all 32
phase values for each controller, six null-support cases, timer/key boundaries,
wrong buttons, grade outcomes, the special target-trait branch and swing flags.
Lookup/query helpers and the enemy flag setter execute natively; other children
are explicit stubs. Full mapped memory except 256 stack bytes is compared.
These fixtures cover caller branches, not live child lifetimes. Failed replays
and exact producer versions are retained. Both ROM checks and 107 tests pass;
all 104 original saves retain their hashes.

### Pocket Chomp support boost

[`chomp_boost.cpp`](../../src/attack_pocket_chomp_ov018/chomp_boost.cpp)
reconstructs `PocketChomp_Boost` at 0x020C3C34-0x020C3CFC. A support swing
boosts the Chomp only in phases 1 and 2: it starts a 60-frame alternate-animation
timer, sets the variant bit and render state, selects animation 4, then passes
the scaled travel speed to the existing motion helper. Preserve the native
double-precision expression `384.0 * speed / 256.0` and final integer truncation.
The first candidate matches all 200 bytes; no inline assembly is needed.

Private `eur_high_chomp_boost/evidence_support83_v1.json` checks seven calls on
the ordinary 4,111-frame support-input route: four in phase 1, one in phase 2,
and two inactive calls in phase 0. The oracle independently checks ordered
stores, phase guards, the speed-table row, every software-double argument and
result, and SP/r4-r11. It checks the attack allocation, party records, embedded
scene objects, models and graphics-list neighbors. Animation and motion child
writes are bounded observations; their lifetimes, internal motion lists, other
globals and rasterization are not independently proved. Inputs, every capture
and final graphics data equal the previous support route. The final capture
shows the battle command menu; no game RAM was edited.

`isolated_v1.json` adds 124 ARM946 cases using copied RAM/DTCM: all 32 phase
values, both directions and travel-speed threshold boundaries, including
progress 65535. Table lookup and software-double helpers execute natively;
animation and motion are explicit no-write stubs. Full mapped memory except
256 stack bytes is checked. These fixtures prove caller boundaries, not live
child behavior. All 56 Chomp functions in 21 compiled objects remain exact;
the full build reproduces the EUR ROM and passes 107 tests. All 104 original
saves retain their hashes.

### Pocket Chomp pursuit preparation

[`chomp_prepare_pursuit.cpp`](../../src/attack_pocket_chomp_ov018/chomp_prepare_pursuit.cpp)
reconstructs `PocketChomp_PreparePursuit` at 0x020C42EC-0x020C43DC. It places
the Chomp on the requested side, selects animation 2, clears its alternate
animation and bounce state, then schedules or immediately starts the pursuit.
The initial target lookup is retained even though the caller ignores its
result. The direction contract is 0 or 1; the typed animation argument is s8.
The first candidate matches all 240 bytes, without inline assembly.

Private `eur_high_chomp_pursuit/evidence_support83_v1.json` follows the ordinary
4,111-frame support-input route and checks two delayed starts: direction 0 with
delay 21, then direction 1 with delay 8. It checks the lookup result, ordered
reset stores, helper arguments, branch choice and SP/r4-r11. The full work,
party records, embedded scene objects, models and graphics-list neighbors are
checked; position, animation and launch child writes are bounded observations.
Inputs, all captures and final graphics match the prior support route. The
final capture shows the battle command menu. No game RAM was edited.

`isolated_v1.json` covers 20 ARM946 cases on copied RAM/DTCM: both directions,
all-clear/all-set flags, and delays INT_MIN, -1, 0, 1 and INT_MAX. Object lookup
executes natively; position, animation and launch helpers are explicit no-write
stubs. The zero-delay branch is covered only by these isolated cases. Full
mapped memory except 256 stack bytes is compared. Child internals, motion-list
updates, allocation lifetimes and rasterization are not independently proved.
All 57 Chomp functions across 22 compiled objects remain exact. The build
reproduces the EUR ROM, passes 107 tests and preserves all 104 original saves.

### Battle interface text quads

`BattleInterface_DrawText` adds 484 matching bytes. It draws a text-buffer quad
using the caller's active matrix, texture allocation and signed scroll position.
The explicit division call preserves the original load order. The state record
is 72 bytes; the transform is 64 bytes and serves only as a null guard here.

Private `eur_battle_text_quad/evidence_menu83_v3.json` checks 208 of 11,223
observed calls during 3,741 frames of natural Bros-menu navigation from save 83.
Sampling takes the first 16 calls per resource/scroll/delay/progress bucket.
Each sample checks the three arithmetic helper contracts, unchanged drawing
records, calling convention and all 16 ordered GPU stores. Sixteen samples call
the still-unlinked scroll helper; its effects are observational, with drawing
inputs captured at its guarded return. The final menu capture was inspected.

`isolated_v2.json` checks 370 ARM946 cases without stubs, including a null
transform, signed coordinate/scroll boundaries, texture dimensions and native
scroll-delay/fractional-progress paths. It checks full mapped RAM, DTCM and MMIO
outside 256 stack bytes. Full-tile scrolling, asynchronous uploads and rasterized
pixels are outside this oracle. The first live probe failed on a register alias;
the final version also corrects an over-wide transform check. All 104 original
saves remain unchanged. Both ROM builds and 81 tests pass.

### Field roaming destinations and party lifecycle

Four functions add 972 matching bytes. Random roaming makes at most eight
attempts, selects an option and direction, and clips diagonal destinations at
the configured boundaries. Its range comparisons use unsigned subtraction.
The party lifecycle initializes the 1,440-byte record and dispatches deletion
through each of its six auxiliary slots before destroying the spatial base.

Private `eur_field_roaming_lifecycle/evidence_cold1_v1.json` verifies 35 natural
calls from save 1: four empty constructors, 27 destination selections and four
deleting destructors. RNG state and direction vectors are independently checked.
Auxiliary deletion, attached-renderer cleanup and linear-motion helper effects
are observational within bounded records; caller decisions are checked before
these helpers. The final field image was inspected visually.

`isolated_v2.json` covers 137 ARM946 cases, all eight directions, clipping at all
four boundaries, eight failed attempts, zero distance and sparse/full auxiliary
slots. It executes native RNG, vector arithmetic, constructors, clearing,
null-renderer base destruction and parent heap coalescing. Auxiliary deletion
and linear-motion setup account for 128 explicit no-op stubs. Full mapped memory
outside 256 stack bytes and the calling convention are checked. The nondeleting
destructor is isolated-only. The first isolated version used the wrong synthetic
auxiliary vtable; v2 corrects it to 0x020C1194 and reruns all cases. All 104 saves
are unchanged. Both ROM builds and 81 tests pass; the first build exposed a stale
VM symbol reference, fixed before the successful second build.

### Field variable-entity animation and roaming options

Ten functions add 1,588 matching bytes. The 1,328-byte variable entity now has
named bobbing and bounce counters, a countdown, enable/disable controls and its
remaining lifecycle functions. The bobbing table has four signed bytes, each
used for eight frames; the bounce table has seventeen. The halfword countdown
preserves the native unsigned decrement. Roaming options occupy four 12-byte
slots. Their five-word script inputs are narrowed only on assignment; bounds
are whole pixels, and movement converts distance to Q12.

The shared entity updater at 0x020B4990 consumes an entity-array argument when
resolving a support entity. The deferred subclass now explicitly forwards that
argument too. All eleven functions in its actual source object remain exact;
this ABI correction contributes no additional matching bytes.

Private `eur_field_variable_roaming/evidence_cold1_v1.json` checks 44 natural
calls: eight option insertions covering all four slots, two bounds assignments,
32 bobbing updates and two deleting destructors. Own-state changes are checked
before common engine calls. Common-update effects, renderer changes, variable
writes and attached-renderer destruction are observational, not independently
reconstructed by this oracle. The final field image was inspected visually.

`isolated_v1.json` checks 314 ARM946 cases and full copied RAM/DTCM/MMIO outside
256 stack bytes. Constructors, clearing, null-renderer destruction, heap merging
and nonmoving roaming stop execute native helpers. There are 131 explicit stubs
for renderer calls, variable writes, common updates and nested bounce completion.
Bobbing and bounce indices stay within their native tables. Six new entry points
are covered only in isolation. All 104 saves remain unchanged; both ROM builds,
the two new source objects and 81 tests pass.

### Field task lifecycle, transfer queries and OAM preparation

Twelve functions add 884 matching bytes. FieldSystem allocates a 44-byte graphics
IRQ task and a 40-byte frame-finalization task. Their virtual tables distinguish
base destruction from heap release. The transfer pointer is at FieldSystem+920,
after its 292-byte time-hole state; only the transfer's leading halfword is typed.
The contact helper recognizes the existing 1360-byte FieldAuxiliaryEntity and its
owner at +1352. Its other input uses an interaction-bearing entity prefix.

Private `eur_field_task_contacts/evidence_cold1_v5.json` checks both constructors
and 96 OAM-preparation calls on a normal save-1 cold boot: 32 inactive and 32 per
screen. OAM and affine counts are bytes, and the caller reloads them after drawing.
Ordered helper arguments, timer output, the HUD return-pointer chain and complete
area records are checked. Renderer output and the palette's 40-byte effect state
are observational; this is not a raster-output oracle. HUD motion flags and Y are
modeled independently. The final field capture was inspected visually.

`isolated_v2.json` adds 141 ARM946 cases on copied RAM, checking all twelve entry
points and complete mapped memory outside 256 stack bytes. Native task insertion,
unlinking and heap coalescing execute without stubs. The contact and OAM cases
make 43 explicit stubbed callback invocations; they verify caller control flow
and arguments, not those callback bodies. Destruction, transfer and contact paths
have isolated coverage only. All 104 original saves remain unchanged. Eight actual
source objects, both byte-identical ROM builds and 81 tests pass.

### Field palette controls, camera stops and input masks

Seven functions add 560 matching bytes. Field palette controls share the existing
20-byte object-palette and 24-byte background-palette records. The background
pointer uses the resident vtable at 0x02050D60; slot 0xBC reverses an effect at
0x02011C58. Its complete object is 1,936 bytes. Camera stop reads a 52-byte
record and optionally copies its target coordinates; layer stop uses three
56-byte records. The input helper intersects the two party input masks.

Private `eur_field_effect_motion_controls/evidence_walk55_v2.json` checks 778
input-mask calls during ordinary walking and pause/return from checkpoint 55.
The other six functions are covered only by the 265 cases in `isolated_v2.json`.
These compare full copied RAM/DTCM/MMIO outside 256 stack bytes, helper arguments
and results, SP and r4-r11. Native palette-reversal helpers execute completely.
Sixteen general-angle cases stub positive fixed-point division; nine axis,
diagonal and zero-vector cases execute without that stub. Hardware timing and
raster output are outside the isolated checks. The final field capture was
visually inspected, and all 104 original saves remain unchanged. Four actual
source objects, both byte-identical ROM builds and 81 tests pass.

### Battle object links and Pocket Chomp

Sixteen functions add 1,216 matching bytes: link state creation, offsets, following
flags, gravity and parameter setters, plus two flexible-link controls. The two
creators reserve 252 or 508 bytes; the flexible state is 268 bytes. Only the
508-byte trail has the parameter at offset 504. Source and target offsets are
separate, and native segment velocities narrow after the divisions.

Private `eur_battle_object_links/evidence_chomp83_v1.json` checks 146 calls over
3,110 frames, including one new arena reservation and 17 retained allocations.
Automatic key presses follow guarded Pocket Chomp phase/timer state; no RAM is
patched. All observed object/state writes, slot ownership and arena cursor,
helper arguments and return values are checked independently. The remaining
functions and branches use 144 ARM946 cases in `isolated_v1.json`, including both
creator sizes, new/reused/retained slots and skipping a still-owned slot. Every
helper executes natively, without stubs. These cases compare full mapped memory
outside 256 stack bytes and do not execute the installed rendering callback.
Five source objects, both ROMs and 81 tests pass. The final battle capture was
inspected, and all 104 original saves remain unchanged.

### Field brightness, blending and time-hole state

Thirteen new functions add 1,552 matching bytes: alpha and master-brightness
updates, brightness start/reversal, vertical scrolling, time-hole state setup,
color-cycle initialization, camera-origin offsets and planar script pause/resume.
Private `eur_field_screen_transitions/evidence_walk55_v1.json` checks 1,620 calls
on the natural save-55 walking/pause/return route. It covers four timed brightness
starts/completions, 60 intermediate steps and 64 ordered brightness register
writes. All 808 alpha updates take the inactive path; the other functions and
branches have separate isolated coverage. `isolated_v2.json` checks 264 ARM946
cases against copied live RAM/DTCM/MMIO, including full mapped memory outside
256 stack bytes. Clear, division, brightness and motion helpers run natively,
without stubs. MMIO backing does not model display timing, IRQs or rasterization.
Six final source objects, both ROM builds and 81 tests pass. The final field
capture was inspected; all 104 original saves remain unchanged.

### Field windows, camera and script controls

Fourteen functions add 1,796 matching bytes: message-window queries/closing,
camera positioning/projection, frame submission, script-update gates and entity
script controls. Private `eur_field_area_services/evidence_walk55_v1.json` checks
2,606 calls during natural save-55 walking and pause/return: screen bounds, frame
submission, VM gating, party updates and window closing. Other targets are covered
by `isolated_v1.json`, with 209 ARM946 cases and full mapped-memory comparison
outside stack scratch. Clamp and linear/scaling/vertical helpers execute natively;
other helpers have explicit no-effect stubs. Projection checks ordered register
writes without modeling rasterization or FIFO timing. Live VM, renderer and party
effects beyond the caller checks remain observational. All ten source objects,
both ROMs and 81 tests pass; all 104 saves are unchanged.

### Field interaction bounds, input forwarding and action models

Six functions add 1,356 matching bytes: Q12 line intersection, script/2D
interaction-bound refresh, party input forwarding and region-71 action models.
Private `eur_field_interaction_input/evidence_walk55_v1.json` checks 2,685 calls
during natural walking and pause/return from save 55, including 12 intersections.
It does not reach the script refresher. The input wrapper checks argument order and
52 manager bytes outside its parties; child input effects remain observational.
Renderer effects remain observational, with its two-byte animation field accepted
at helper return. `isolated_v1.json` adds 180 ARM946 cases, including signed bounds
indices, parallel lines, aliased outputs and action guards. Bounds setters execute
natively; division, input and renderer stubs have explicit contracts. All six
source objects and both ROMs match; 81 tests pass and 104 saves are unchanged.

### Touchscreen replies, card DMA and memory arenas

Six functions add 1,236 matching C bytes: the touch FIFO callback, card DMA
wrapper, console-type cache and three arena helpers. Private
`eur_nitro_input_arenas/evidence_boot55_v2.json` checks 21 arena/console calls
on a cold boot to the title screen. That route does not reach touch replies
or card DMA. `isolated_v2.json` checks 136 ARM946 cases using copied live memory,
including touch ring wrapping/errors and ordered DMA register writes. It uses
explicit console-detector results and a no-effect touch callback; invalid replies
trap at termination. DMA transfers and asynchronous timing remain outside that
model. Both ROMs match, all 81 tests pass and all 104 saves are unchanged.

### Save-menu number and time drawing

Two functions add 860 matching C bytes alongside the existing quad renderer.
Decimal drawing clips overflow to the requested digit width, suppresses leading
zeros when requested and advances the draw depth for each submitted digit.
The summary display caps coins at 999,999 and time at 99:59.

Private `eur_save_menu_numbers/evidence_save55_v1.json` checks 1,260 number calls,
180 summary calls and 6,660 quad calls over 180 frames in initialized save menu
55. It checks child arguments, depth counters, the complete 10,248-byte menu
workspace and ordered GPU writes. Forty-five isolated ARM946 cases cover digit
and time boundaries, leading zeros and alpha zero with native division/quad
helpers and full mapped-memory comparison except 256 stack bytes. There are no
helper stubs or live fixtures. The final capture and all four graphics dumps
match the preceding quad replay. Both ROMs match, 81 tests pass and 104 saves
are unchanged. GPU submission is checked; rasterization remains observational.

### Credits and menu textured quads

Two functions add 856 matching C bytes. Both accept ten full-word arguments
and submit palette, polygon, matrix, texture-coordinate and vertex commands.
Alpha zero skips all GPU writes. The menu draw counters are at shared workspace
`+0x188`; credits use the existing workspace's depth and polygon fields.

Private `eur_frontend_quads` records 6,660 checked calls from initialized save
menu 55 and 19,360 from controlled credits entry at checkpoint 86. Every call
checks the ordered GPU stores, unchanged counter words and preserved registers.
The credits request uses one 72-byte command fixture, restored before transition;
it does not establish ordinary story completion. Final captures show the save
menu and credits end screen. Another 48 isolated ARM946 cases check zero-alpha,
palette formats, zero scale and signed coordinates against full mapped memory
except 256 stack bytes. These checks verify command submission; rasterization
is observed. Both ROMs match, 81 tests pass and all 104 saves are unchanged.

### Field party restoration and story presence

Three functions add 1,980 matching C bytes: party-indicator initialization,
controller snapshot restoration and story-driven party presence. Restoration
preserves the native duplicate copy of action slot 2 and leaves slot 3 intact.
The compact snapshot retains member snapshots by pointer; its storage must
outlive their use. Existing controller/snapshot types describe the packed fields.

Private `eur_field_party_restore/isolated_v1.json` checks 136 ARM946 cases,
including every combination of three story flags, screen sides and packed
snapshot boundaries. It compares all mapped memory except 256 stack bytes;
story-variable reads are explicit stubs, while indicator initialization runs
natively. `evidence_return55_v1.json` checks two indicator calls during natural
pause exit from save 55 and reaches the visible field. That route does not call
the snapshot restore or story-presence function. Their coverage is isolated.
Both ROMs are byte-identical, all 81 tests pass and all 104 saves are unchanged.

### Field scene requests and transitions

Sixteen functions add 3,180 matching C/C++ bytes: the field transition dispatcher,
pause/save/shop requests and preparation, credits/game-over requests and cleanup,
and five fade predicates. The low four bits at `FieldSystem + 0x258` select the
transition phase; odd phases wait for area fades and even phases prepare the
destination. Area fade activity is bit 0 of the byte at area `+0x2402`.

Private `eur_field_scene_transitions/evidence_pause55_v2.json` checks 67 calls
during natural Start-button entry from story save 55 and reaches a visible pause
menu. `isolated_v2.json` checks 94 ARM946 cases across all 16 functions, including
every phase, both area fade flags, request fields and nullable cleanup. It checks
all mapped memory except 256 stack bytes and ordered helper arguments. Complex
fade/cache/resource/IRQ helpers use explicit no-effect stubs in isolated cases;
their internals and natural entry to the other destinations are not covered.
The first live probe used the wrong status width; the first isolated probe had
an incorrect stub target. Both failures are retained separately from corrected
passing runs. Both ROMs are byte-identical; 81 tests pass; 104 saves are unchanged.

### Battle motion and shared value helpers

Nine functions add 1,280 matching C bytes: model-animation and alternate-effect
slot selection, axis-rotation and sine-displacement channel setup, path updates,
resource-effect attachment, item-effect values and inclusive interval overlap.
The slot helpers use the actual 64-entry context arrays. Motion setup preserves
signed halfword parameters and native duration arithmetic; item effects retain
the packed kind and amount fields and the original rounding rules.

Private `eur_battle_helpers/isolated_v1.json` passes 48 ARM946 cases using copied
live RAM, including slot exhaustion, coordinate wrapping, reversed intervals,
item rounding and nonpositive motion durations. All mapped memory is checked
except 256 stack bytes. Native division, channel creation and owner binding run;
attached-effect factories, rotation and path decoding have explicit stubs.
This batch adds no natural gameplay-route coverage. Both ROM builds match the
original, all 81 tests pass, and all 104 story saves remain unchanged.

### Battle sound scheduling

[Battle sound](../../src/battle/battle_sound.c) adds 1,036 matching C bytes.
Six functions manage 16 sound slots and 20-byte pooled tasks. A negative repeat
interval measures the first effect through its audio handle; a zero repeat count
continues until cancellation. Timers wrap as signed halfwords. Repeat counts
occupy 14 bits, with the audio handle in the upper two bits. Cancellation clears
the callback; the task pool subsequently releases the slot.

Private `eur_battle_sound/evidence_entry55_v1.json` verifies 102 calls across
five functions over 706 frames. `isolated_v1.json` passes 44 copied-RAM cases
across all six functions: timer wraparound, slot exhaustion, cancellation,
argument truncation, repeat counters and handle completion. It compares all
mapped memory except 256 stack bytes and executes native pool insertion.
Audio entry points have explicit return-value stubs in isolated checks;
live audio arguments and returned handles are checked, without claiming an
independent audio-engine or audible-output verification. The retained exit probe
fails its coverage requirement because this route makes no sound calls.
Both rebuilt ROMs match the original, and all 104 saves remain unchanged.

### Battle capture fading and transition setup

[Capture fading](../../src/battle/battle_capture_fade.c),
[transition preparation](../../src/battle/battle_transition_prepare.c) and
[task creation](../../src/battle/battle_transition_start.c) add 1,392 C bytes.
The fade captures slot 14 once, waits while upload is pending, and adjusts alpha
between 0 and 31 while drawing the captured image relative to the camera.
Coordinate differences truncate to signed halfwords before Q8 translation.
The transition code allocates or reuses 28-byte task slots, selects entry/return
effects through the encounter flags, submits a black quad and restores battle
display layers through the transfer queue. The power-control access is 16-bit.

Private `eur_battle_capture_transition/evidence_entry55_v1.json` and
`evidence_exit55_v2.json` pass 706/901-frame controlled routes, exercising all
six functions: 778 idle fade calls and five setup calls. Final entry images vary
between unchanged probe runs; checked calls, VRAM, palettes and OAM agree.
The repeat report retains the small RAM differences without assigning a cause.
The exit fixture is restored before destruction; its final
black screen does not establish a visible field return. The first exit probe
missed QueueConfigure's busy-bit write; that failed report and oracle are retained.
`isolated_v1.json` passes 21 copied-RAM cases covering fade thresholds, pending
uploads, coordinate wrapping, capture setup, task reuse and display masks.
Checks cover full mapped memory except 256 stack scratch bytes, preserved
registers, ordered direct RAM/GPU writes and independently modeled capture-queue
effects. Draw/capture fade branches have isolated coverage only; rasterization
and IRQ timing are not simulated. Live transition initializer payloads are
observed within their 28-byte slots. Both ROMs match the original; 104 saves
remain unchanged.

### Common frame callbacks and reward-controller setup

Shared battle resources now include frame callback dispatch, OAM reset and graphics
cleanup. Four 12-byte task slots start at workspace offset 70484; their callbacks
are at +8. The OAM callback at +428 changes from ResetOam to OamReady after the
screen-1 counters are cleared. Reward items use a 1344-byte allocation: callbacks
at +0/+4, packed flags at +1240, fade at +1244 and eight 12-byte item entries at +1248.
The frame callbacks reread shared roots after calls. Closing sets phase 7 only
when the item count is nonzero.

Private `eur_battle_common_frame/evidence_entry55_v1.json` and
`evidence_exit55_v1.json` pass 706/901-frame controlled routes with 2,851 calls.
Checks cover caller stores and callback arguments, OAM counters, cleared graphics
roots and preserved registers. Renderer/destructor internals remain observations.
`isolated_v1.json` adds 15 status, callback and constructor cases. Status/callback
cases check full mapped memory except 256 stack bytes; construction checks native
allocation/zeroing and the complete payload, with actor binding explicitly skipped.
No natural reward route is established. Both ROMs remain byte-identical; 104 saves
are unchanged. The first build failed due to C++ linkage on heap declarations;
including their C declaration before the internal graphics headers fixed it.

### Common battle-resource initialization and loading

[Common resources](../../src/battle/battle_common_resources.cpp) reconstructs
`0x0206A240..0x0206A608` (968 C++ bytes). Initialization allocates the shared
70,976-byte workspace, graphics buffer, sprite and palette records, matrix/model
animation pools and dialogue resources. The loader requests slot 20, waits for
completion, builds its sprite catalog, binds reserved actors 16/17 and positions
both at (-128, -256, 0) before reporting ready.

Private `eur_battle_common_resources/evidence_entry55_v6.json` passes a 706-frame
controlled battle entry: one initialization and 21 load calls, including phases
0, 1 and 2. The encounter injection is restored before native battle entry.
The probe checks the entire workspace, pool initialization, helper arguments,
caller stores, actor positions and render-list topology. Graphics/dialogue/model
helper internals remain bounded observations. Embedded actor palettes begin at
object + 204; inserting another palette can change their neighbor links.
`isolated_v2.json` adds 14 copied-RAM cases for pending/completed/invalid states
and preservation of upper flag bits, checking all mapped memory and ordered
writes. Allocation failure and workspace reuse remain untested. Earlier failed
probes preserve the missing palette-neighbor, auxiliary-model and helper-stack
checks that were corrected. Both builds match the original ROM; all 104 saves
are unchanged. The final capture shows the battle command menu against Petey.

### Battle render-override reservation and mesh queries

[Render control](../../src/battle/battle_scene_render_control.c) reconstructs
`0x020BB1C4..0x020BB2D4` (272 C bytes). ReserveRenderOverride rounds the requested
state size up to four bytes. The arena at `object->resource->stream_state`
contains a cursor followed by twelve-byte slots: render callback, state pointer,
16-bit state size and 16-bit owner ID. A current slot of the same size is retained;
otherwise the function scans for an unused matching slot, checking its recorded
owner through GetById. The first null state pointer ends the scan and receives
new storage from the cursor. Binding clears the callback, records the caller's
ID and sets its render-override pointer. The state bytes are not initialized here.

The mesh query returns 0 for other render modes, 1 without a task, 2 for the hit
callback, 3 for the fade callback and 4 for another callback. Battle VM opcode
`0xAE` uses this query.

Private `eur_battle_render_control/evidence_brat50_round_v1.json` repeats the
2,820-frame Shrooboid Brat route and independently verifies its 1,028-byte
reservation, including slot/cursor writes and untouched state storage. The
previous capture-control checks also pass, and every screenshot matches the
earlier replay. `isolated_v1.json` passes 26 ARM946 cases covering alignment,
retained slots, stale ownership, occupied slots, size mismatches, scan termination
and all mesh results. Every case checks all mapped RAM/DTCM/MMIO, ordered writes,
return values and preserved registers. Mesh queries and reservation reuse have
isolated coverage only. Synthetic records stay within copied RAM; the live route
uses normal inputs and preserves all 104 original saves.

### Battle sprite-grid capture controls

[Sprite-grid capture](../../src/battle/battle_sprite_grid_capture.c) adds seven
matching C functions at `0x020B9424..0x020B99BC` (1,432 bytes). Battle VM opcodes
`0xB3..0xB7` call its initialization, start, advance, finish and active-state
controls. EUR enemy record 34, Shrooboid Brat, selects packed AI resource `0xD004`:
`BAI_mon_3_hn.dat` entry 4 contains the eleven relevant script commands.

The effect uses a 1,028-byte state through `read32(read32(0x020C0DC8) + 4)`.
The public C source uses an eight-byte prefix of the twelve-byte render-override
slot. Signed phase is at `+0`, delay ranges at `+2/+4`, selected/visited cell
masks at `+6/+8`, counts at `+10/+11`, signed displacement at `+12`, scanline
fields at `+13..+15`, signed timer at `+16`, and flags at `+17`.
Start selects among sixteen cells without replacement. Phases 0 and 5 report
inactive. Finish clears flag bit 0; Advance preserves it. The queued callback
ramps displacement between 0 and 8 and arms four 16-line bands on sub-screen
BG1. Alternating bands add displacement to the shared signed origin plus 128;
vertical offset is origin plus 32. Offsets wrap to nine bits at `0x04001014`.

Private `eur_battle_grid_capture/evidence_brat50_round_v3.json` checks 2,820
frames after normal entry from save 50: one initialization, one start, 92 active
queries, 241 queued updates, 960 IRQ callbacks and one finish. All four bands
execute 240 times, and the effect returns to phase 0. The recorded cold-boot
route loads the Koopaseum save and jumps both adults onto the raised blue panel
to activate the elevator; it uses no RAM edits. State, override and caller
records, RNG steps, callback arguments, queue insertion and ordered target
display-register stores are checked. DISPSTAT checks use the value loaded by
the game: hardware status bits can change before its following store.

`isolated_v1.json` adds 50 native ARM946 cases on copied live RAM, including
Advance, rejected/reused starts, signed count and delay boundaries, upper mode
bits, terminal phases, invalid band indices and wrapped display coordinates.
Nineteen leaf cases check all mapped memory and ordered writes. Other cases
use the same bounded record and helper checks as the live probe. The reserve
helper's resource-pool mutations remain observations; there is no independent
pixel renderer or proof of asynchronous IRQ timing. All 104 original saves
retain their hashes. The first probes failed on literal-pool decoding, register
aliases and changing DISPSTAT bits; their sources and failures remain private.

### Battle dialogue controls

[Dialogue controls](../../src/battle/battle_dialogue_control.cpp) and
[teardown](../../src/battle/battle_dialogue_destroy.cpp) add 624 matching C++
bytes at `0x020660FC..0x02066314` and `0x02066B6C..0x02066BC4`.
The controller is `read32(read32(0x020C0660) + 0x208)`; its four 204-byte
window records start at `read32(controller + 0xF00)`. Allocation tests the
front-buffer pointer at record `+0x74`; the open-state check only tests the
closing bit at `+0x68`, even for an unallocated slot. Negative control indices
scan/close all four slots; nonnegative ones narrow to signed 16 bits.
The visual-property query instead uses a full-width index and the pointer at
record `+0xC4`: property 0 tests byte `+0xD0 >= 2`, property 1 adds signed
halfwords `+4` and `+0x10`, and other properties return zero. Visuals occupy
276 bytes within the manager; texture-upload fields reuse their later bytes.

Private `eur_battle_dialogue/evidence_battle_start55_v1.json` checks 411 live
frames from the existing controlled dialogue checkpoint: 197 allocation tests,
197 open-state tests and three close-all calls, each invoking slots 0 through 3.
Dialogues close and gameplay continues; all 104 original saves retain their hashes.
`isolated_v3.json` adds 28 native ARM946 cases using copied live RAM, covering
query boundaries, index narrowing, empty/full scans and both teardown branches.
Predicates preserve all captured memory. Teardown also checks normal/IRQ task
and sprite-allocation list roots and neighbors. Native UI callbacks, renderer
internals and freed heap headers remain bounded observations. Visual-property
queries and manager teardown have isolated coverage only; captures are not an
independent rasterizer. The later dialogue checkpoint produced zero target calls;
isolated v1/v2 exposed incomplete list modeling and are retained as failed attempts.

### Battle transition dispatch and resource slots

The matching build includes two more overlay-2 helpers:

| Address | Function | C/C++ bytes |
|---|---|---:|
| `0x0209234C` | `BattleObjectData_ResolveSlot` | 68 |
| `0x020B5F38` | `BattleTransition_UpdateExitWait` | 164 |

The slot helper extends [battle_script_state.c](../../src/battle/battle_object_load_control.c).
A 16-bit handle's high nibble selects a one-based archive slot; its low 12 bits
become the entry index at offset 36 of that 44-byte request. The request array
starts at battle-context offset `0x284` and contains 14 records. Native code
reloads the context pointer before returning the selected slot.

[battle_transition_dispatch.cpp](../../src/battle/battle_transition_dispatch.cpp)
reduces the signed sub-screen brightness field at context offset `0x24` by two.
An exact-zero result returns and waits for another invocation. Crossing below
zero instead clamps to zero and continues immediately. Runtime flag bit 5 then
blocks dispatch. When clear, save flags at `read32(0x02059FE8) + 0x560`, bits
9..12, select an initializer from the four-entry table at `0x020C03C4`.
The live task remains in `r0` across the indirect call; pseudocode omits that
argument. The shared context flags now expose bit 5 separately, and the existing
ten-byte encounter view names the transition selector without changing layout.

The batch adds 232 matching C/C++ bytes. The slot helper replaces 68 bytes
already counted as symbolic assembly, so combined source coverage grows by
164 bytes. The actual build objects match all three functions in the two units,
344 bytes including the existing 112-byte script-state helper. Both packaged
and native-relinked ROMs retain the EUR SHA-1; all 81 tests passed. Private build
and object records are `build/analysis/battle_transition_dispatch_build_validation.json`
and `battle_transition_dispatch_object_provenance.json` in the same directory.

Private runtime evidence is under `build/runtime/eur_battle_transition_dispatch/`:
`evidence_entry55_v1.json` verifies two slot calls during the checkpoint-55
encounter route, and `evidence_exit55_v2.json` verifies 17 exit-wait calls:
16 countdown returns and one initializer dispatch. The 1,607 frames include
19 completed calls and 18 ordered caller stores. Checks cover the complete
401,416-byte battle allocation, save record, 28-byte task slot, caller stack,
callback argument, initializer effects, return value where applicable, SP and
callee-saved registers. The live selector is 1 with standard direction.

Entry uses the previously documented temporary field-VM encounter command and
restores its 72 bytes and cursor before native battle start. Exit selects turn
state `0x500D` at a guarded update with all four party AI scripts idle, then
restores the original state before context destruction. It does not demonstrate
a natural victory. Both battle roots are checked at guarded destruction returns;
field text and read-only data subsequently replace the battle overlay. The final
capture is black, as in the earlier controlled exit, so a visible field return
is not established. The failed `exit55_v1` report retained a final-zero assertion
against addresses already reused by the field overlay. Its corrected replay
preserves identical fixture records, target events, input snapshots and capture.

`isolated_arm_cases_v1.json` adds 16 native ARM946 cases in copied live RAM,
without helper stubs: first/last archive slots, low-12-bit boundaries, signed
brightness extremes, zero versus underflow, blocking bit 5 and all four valid
exit selectors. Full main RAM, caller stores/stack, arguments, return values,
SP/r4-r11 and DTCM outside the bounded helper stack agree with the independent
model. These cases do not cover the alternate VCount-IRQ initializer or live
object lifetimes. `artifact_validation_v1.json` validates 15 images/memory
artifacts, report/source versions and all 104 unchanged battery saves.
Graphics dumps and the inspected battle-command-menu capture are observational;
this batch does not independently verify rasterization.

### Battle scheduler queues

The initial [battle_scheduler_queues.c](../../src/battle/battle_scheduler_queues.c)
batch reconstructed nine routines at `0x020724C8..0x0207282C`, totaling 868 bytes. Both the
isolated public object and the actual build object match every instruction,
literal and relocation. No assembly or compiler flags were added. The 100-byte
`BattleTaskQueue_Enqueue` previously had a symbolic-assembly reconstruction, so
this adds 868 C bytes and 768 bytes to combined C/assembly coverage.

| Entry | Routine | Bytes |
|---|---|---:|
| `0x020724C8` | `BattleScheduler_ElapsedScanlines` | 64 |
| `0x02072508` | `BattleTransfer_EnqueueAfterMapping` | 156 |
| `0x020725A4` | `BattleTransfer_EnqueueBeforeMapping` | 176 |
| `0x02072654` | `BattleTaskQueue_Promote` | 92 |
| `0x020726B0` | `BattleTaskQueue_Enqueue` | 100 |
| `0x02072714` | `BattleScheduler_Idle` | 4 |
| `0x02072718` | `BattleSchedulerNode_Unlink` | 64 |
| `0x02072758` | `BattleSchedulerNode_UnlinkForCleanup` | 64 |
| `0x02072798` | `BattleSchedulerNode_Insert` | 148 |

The [internal layout](../../src/battle/battle_scheduler_internal.h) follows the
native producers, VBlank consumer at `0x0207282C`, main consumer at `0x020729D4`
and constructor at `0x02072FB0`. `BattleMain_Create` allocates 3,584 bytes for the
scheduler; its constructor clears 2,492 bytes starting at offset 1,088. The final
four padding bytes therefore remain outside that clearing operation.
The VBlank consumer and [scheduler lifecycle](#battle-scheduler-lifecycle) are
also reconstructed, along with the [main frame driver](#battle-scheduler-frame-driver)
at `0x020729D4`.

All offsets below are from `read32(0x020C0714)` in ARM9 main RAM:

| Storage | Offset and extent |
|---|---|
| Ordered node list | Pointer at `+1092`; each node's first 16 bytes contain next, update callback, VBlank callback, signed halfword priority and padding |
| IRQ task | Pointer at `+1104`; constructor allocates 40 bytes |
| Promoted task | Twelve-byte record at `+1116` |
| Task ring | 32 twelve-byte records at `+1128`; halfword head/tail at `+1512/+1514` |
| Transfers before bank restoration | 32 sixteen-byte records at `+1516`; 32 deferred records at `+2028`; halfword head/tail at `+2540/+2542` |
| Transfers after bank restoration | 32 sixteen-byte records at `+2544`; 32 deferred records at `+3056`; halfword head/tail at `+3568/+3570` |
| Clock and flags | Unsigned halfword VBlank count at `+3572`; flags at `+3574`: promoted task bit 0, VBlank-ready bit 1, inside-VBlank bit 2 |

The transfer names describe execution around restoration of texture/palette-bank
mappings. While inside VBlank, producers append to the first empty deferred
record. Otherwise they append to their 32-entry ring and wrap its tail. A transfer
record holds its callback and three full-width argument words; existing callers
use specialized views of those words. Task promotion copies callback, state,
timer and argument in native order, clears the source callback and sets flag 0.
Node insertion is stable for equal priorities and compares the full signed input
before storing its low halfword. Both unlink copies clear the list head when
removing its first node, even when that node has a successor.

The first queue draft narrowed its next index too early, adding two instructions.
Keeping it full-width until the final halfword store recovered the native code.
The scanline helper required the frame-counter read before adjustment of VCOUNT.
The unlink loops required the native early return immediately after relinking.
These corrections followed the differing instructions; no source permutations
were used. The unrelated keyframe-updater draft remains unlinked.

The private live probe is `build/analysis/probe_battle_scheduler.py`. It replays
the same 380-frame Bro Flower setup prefix, checkpoint-83 save and compatible
`ov17_bros_menu83.dst` snapshot described under [hit-bonus rolls](#battle-hit-bonus-roll).
`build/runtime/eur_battle_scheduler/evidence_flower83.json` records 1,016 complete
calls: 774 before-mapping transfers, 207 after-mapping transfers, 26 task enqueues,
one promotion and eight scanline queries. Every call checks all 3,584 scheduler
bytes and any touched records against an independent model. The replay covers
380 deferred before-mapping transfers and ring wrapping in all three queues.
VCOUNT and the frame counter are checked at their guarded native loads.
No RAM or code is edited, and no calls remain pending.

Separately, `build/analysis/check_battle_scheduler_arm.py` runs 74 isolated ARM946
cases against copies of that paused RAM capture. All nine routines execute with
complete original-byte guards. Cases cover visible/VBlank scanline boundaries,
frame-counter extremes, transfer/task ring edges, empty and populated deferred
queues, promotion including source/destination aliasing, both unlink copies,
head removal with a successor, predecessor traversal, stable equal-priority
insertion and full-width priorities that truncate only at the store.
The native task/IRQ-enable and IRQ-mask helpers execute without stubs.

`isolated_arm_cases.json` records exact return values, preserved registers, all
four MiB of copied main RAM, all 16 KiB of DTCM including expected stack writes,
the modeled I/O page, ordered record stores and temporary IE/IME masking writes.
Synthetic list records occupy scratch bytes only in the copied RAM. These cases
establish controlled function behavior; live node lifecycle, asynchronous IRQ
timing and full-game navigation through those fixtures remain outside their scope.

`artifact_validation.json` verifies eight 256-by-384 screenshots, four graphics
dumps, the main-RAM/DTCM snapshots and all 104 unchanged source saves. Every image
and graphics dump matches the ordinary hit-bonus replay with identical inputs;
the final Bro Flower setup screen was visually inspected. Graphics equality is
separate from an independent rasterization check. The build report is
`build/analysis/battle_scheduler_build_validation.json`: full matching build,
golden packaged/native ROMs, zero differing bytes, progress/check and 81 tests pass.
Earlier build logs from before the four-byte padding correction are kept separately.

### Battle scheduler VBlank consumer

`BattleScheduler_VBlank` at `0x0207282C..0x020729D4` adds 424 matching C bytes
in the existing scheduler module. Both isolated and actual build objects match
all ten functions, totaling 1,292 bytes. The first consumer draft was exact;
its bank-restore declarations were aligned with the existing signed-int APIs
without changing the result. Shared node-helper declarations now preserve their
pointer returns and C linkage, with compatible prefix casts in the lifecycle
caller. All caller bytes still match.

The consumer increments the unsigned halfword counter even when not ready.
When ready, it clears ready/active flags, sets inside-VBlank, saves the texture
and texture-palette bank masks, and drains the first transfer ring while those
banks are mapped to LCDC. It restores the banks before draining the second ring,
then calls each node's no-argument VBlank callback and clears inside-VBlank.
Each ring caches its tail and advances its head before calling its records;
next-node reads follow callbacks. Preserve those boundaries when changing code.
The four bank helpers were already linked C and contribute no new coverage.

`build/analysis/probe_battle_scheduler_vblank.py` composes the existing leaf-queue
probe with `battle_scheduler_vblank_oracle.py`. The checkpoint-83 Bro Flower
setup route runs 380 frames and checks 380 ready calls, 3,420 controller stores,
1,520 bank-helper calls, 600 transfer callbacks and 380 node callbacks. The
before ring visits 394 records with 13 wraps; the after ring visits 206 records
with six wraps. The separate leaf model still checks all 1,016 calls described
above. No live memory is edited and no calls remain pending.

The controller oracle independently checks its own stores, cached-tail traversal,
callback targets/arguments/order, reset-helper return masks, and final GX state
and VRAM-bank controls. It retains all 3,584 scheduler bytes, the 1,380-byte save,
pointer roots and visited node prefixes. Arbitrary callback effects are observed
only within explicit task/queue, flag, GX and bank-control ranges; other retained
bytes must remain unchanged. This route's observed RAM changes are the first
four bytes of the deferred-before queue, also covered by the separate producer
oracle. DISP3DCNT readback and rendering remain observational. The report is
`build/runtime/eur_battle_scheduler_vblank/evidence_flower83_v2.json`.

A separate `check_battle_scheduler_vblank_arm.py` runs 32 isolated ARM946 cases
on copies of the captured RAM/DTCM. They cover the not-ready branch with both
inside-VBlank flag values, counter wraparound, empty/wrapped queues, null and
native idle callbacks, node chains, all 16 texture masks and seven palette masks.
Native bank helpers and idle callbacks execute without stubs. Checks include
all four MiB of copied main RAM, all 16 KiB DTCM with predicted final stack bytes,
the modeled I/O page, ordered controller stores and callbacks, bank-reset returns,
SP and preserved registers. The synthetic nodes and I/O are explicit inputs in
the copy. These cases do not establish asynchronous IRQ behavior, live synthetic
node lifetimes or hardware register readback; bank-helper store order is outside
their independent checks. See `isolated_arm_cases.json` in the same directory.

All eight images, four graphics dumps and both memory snapshots match the earlier
ordinary route and have validated extents/hashes. The final setup screen was
inspected; all 104 source saves remain unchanged. `artifact_validation.json`
records those checks. The first guard-loader attempt failed before ROM loading
because a placeholder overlay has no `.text` range; its source/failure record is
preserved. Using extracted overlay-table bases fixed the loader, and the v2 replay
passed without removing runtime assertions.

`build/analysis/battle_scheduler_vblank_build_validation.json` records the full
matching build, both golden ROM hashes, zero-difference native relink, generated
progress and all 81 tests. Source and log hashes were rechecked after the intervening
documentation commit; no source changes followed that full build. Final ten-function
object comparisons are pinned by `battle_scheduler_vblank_object_provenance.json`.

### Overlay-5 sprite collection and initialization

[sprite_pool.cpp](../../src/scene_support_ov005/sprite_pool.cpp) now includes
`Overlay5Sprite_CollectOam` at `0x02068B20` (156 bytes).
[item_pool.c](../../src/scene_support_ov005/item_pool.c) includes
`Overlay5ObjectSprite_Init` at `0x020695EC` (96 bytes), and incorporates the
already-linked 32-byte release at `0x020695CC` without counting it again.
Final build objects match all 15 sprite-pool functions (960 bytes) and all
18 item-pool functions (1,060 bytes).

The collection wrapper uses a 336-byte embedded sprite slot from the pool at
`0x0206A3D8`. When active, it adds signed halfword offsets at `+332/+334` to
positions at `+92/+94`, optionally subtracts the camera's signed Q12 coordinates,
calls the resident renderer, then restores both positions. The active byte is
at `+324`, camera pointer at `+328`, and camera coordinates at `+20/+24`.
Each halfword store is a separate truncation point, before camera subtraction.

The renderer at `0x0200A4CC` consumes four arguments: sprite, OAM buffer, and
two byte-count pointers. Its previous declaration omitted the last three.
Native incoming-register use and the virtual interface in
[draw_lists.cpp](../../src/scene_support_ov005/draw_lists.cpp) established the ABI;
correcting it immediately resolved the wrapper's 12-byte/register mismatch.
This was an ABI defect, not evidence for register-allocation permutations.

The initializer uses a 64-byte ResourceB slot from the pool at `0x0206AA18`.
It sets active byte `+48`, clears eight bytes at `+20`, zeros selected fields,
and stores the full-width screen argument into byte `+16`. Its matrix at
`+56..+64` and other untouched bytes are preserved. Native stack halfword
store/reload around `MI_CpuFill16` justifies the scoped `vu16` local. Callers
with shorter prefix views still receive full 64-byte allocations; see the
[shared declaration](../../include/game/overlay005_resource.h).

Private `probe_overlay5_sprite_helpers.py` and `overlay5_sprite_helpers_oracle.py`
run the existing checkpoint-20 healing routes. Reports are in
`build/runtime/eur_overlay5_sprite_helpers/`: `evidence_mario_v1.json` uses
`target20.dst` with `a:8,wait:100`; `evidence_luigi_v1.json` uses `heal20.dst`
with `left:8,wait:8,a:4,wait:8,wait:100`. These are compatible snapshots from
the earlier menu-item-effect work, not the original battery saves.

Across 243 frames, 3,648 collection calls and 32 initializers finish with no
pending calls or live RAM fixtures. Independent checks cover 14,912 ordered
caller stores, complete slots, pool roots/array headers, the 1,380-byte save
record, camera prefix, caller stack, arguments and SP/r4-r11. Native clear and
the renderer's mapping-global update at `0x0205A8AC` are predicted separately.
All live collection calls use the no-camera branch; 243 have nonzero offsets.
2,308 calls append OAM entries and 1,340 append none. This counts submission,
not visible rasterization. All live initializer screen arguments are 1.

The native renderer's output is observational only within its 1,024-byte OAM
buffer and two byte counters. Surrounding tracked state stays independently
checked. Matrix arithmetic, other untracked helper memory, rasterization and
asynchronous IRQ timing are outside the oracle; allocation/release coverage is
not attributed to these two functions.

`check_overlay5_sprite_helpers_arm.py` adds 11 ARM946 cases on copied live RAM:
inactive sprites, signed offsets, halfword wrapping, positive/negative/fractional
camera coordinates, and screen arguments 0, 1, `0x123456AB` and `0xFFFFFFFF`.
Native code runs without stubs. Sprite fixtures disable the renderer through
its real flag-bit-8 early return, so these cases add no drawing coverage.
Complete main RAM, caller stores and registers agree; the renderer's helper
stack is explicitly observational, with remaining DTCM checked independently.

`artifact_validation_v1.json` validates seven images, eight graphics dumps,
four RAM/DTCM snapshot pairs and all 104 unchanged saves. Both final menu
captures were inspected. Build evidence in private
`overlay5_sprite_helpers_build_validation.json` records full matching, both
golden ROMs, zero-difference native relinking, progress checks and 81 tests;
source/object/log/ROM hashes were revalidated before publication.

### Battle scheduler lifecycle

[battle_scheduler_lifecycle.cpp](../../src/battle/battle_scheduler_lifecycle.cpp)
reconstructs six contiguous routines at `0x02072D90..0x02073068`, adding 728 C/C++
bytes. Each first draft matched; final build objects also match all six routines
and the ten neighboring queue functions. The 956-byte main-driver gap keeps these
two modules separate. No assembly or compiler-flag changes were needed.

| Entry | Routine | Bytes |
|---|---|---:|
| `0x02072D90` | `BattleSchedulerIrq_Delete` | 44 |
| `0x02072DBC` | `BattleSchedulerIrq_Destroy` | 36 |
| `0x02072DE0` | `BattleSchedulerIrq_Init` | 48 |
| `0x02072E10` | `BattleScheduler_Delete` | 212 |
| `0x02072EE4` | `BattleScheduler_Destroy` | 204 |
| `0x02072FB0` | `BattleScheduler_Init` | 184 |

Initialization constructs the archive/task prefix, disables the normal task,
resets texture/model lists, clears the scheduler workspace, publishes its global
and allocates the IRQ task. The scheduler occupies 3,584 bytes; its final four
padding bytes remain untouched. The IRQ allocation is 40 bytes despite its
24-byte shared task prefix, and its trailing sixteen bytes are not initialized.
The fifth archive-constructor argument is the full stack word `0xFFFF7F7F`.

Both destruction variants unlink the IRQ task, release nodes and optional
buffers, clear the scheduler global, reset lists and destroy the archive prefix.
Only the deleting variant frees the scheduler itself. The native node loop reads
its successor after the custom allocator releases the block; that allocator
retains payload bytes. Preserve this order without treating the released payload
as a live allocation. Multi-node cleanup remains outside the runtime cases below.

Private `build/analysis/probe_battle_scheduler_lifecycle.py` and
`battle_scheduler_lifecycle_oracle.py` check ordered caller stores and helper
arguments, returns, full live allocations, save data, task-list neighbors, heap
metadata and texture/render roots. Open-file destruction additionally checks
`FS_CloseFile` entry/return, the complete 68-byte file and 80-byte archive records,
and success under the observed idle-ROM archive preconditions. Hardware lock
internals, asynchronous IRQ timing and helper MMIO-store order are not independent
oracles here; final IE/IME are checked.

Four checkpoint-55 replays total 3,214 frames: `entry55_v4`, `exit55_v4`,
`retry55_v4` and `entry55_v4_repeat`, with reports under
`build/runtime/eur_battle_scheduler_lifecycle/`. Each constructor and each
deleting destructor completes twice. No calls or input fixtures remain pending.
Entry restores the temporary 72-byte field-VM encounter command and its cursor
before battle proceeds. Exit restores the turn-state word before destruction.
The retry variant temporarily selects exit mode 1, verifies native clearing of
the ten-byte encounter request, then restores the original mode at the context
destructor's return. It reaches an active two-choice retry/load menu.

The controlled state-0 exit verifies cleanup and complete field-overlay text and
read-only data, but remains black, as did the historical route. It does not prove
a natural field return. An earlier final-zero assertion failed because the field
overlay reuses `0x020C0714` for read-only data. The corrected check verifies both
battle roots while overlay 2 still owns them, before checking the destination
overlay. The earlier missing-file-close oracle and failed exit reports remain
preserved; neither failure required changing the matching game source.

`check_battle_scheduler_lifecycle_arm.py` adds nine native ARM946 cases on copied
live RAM/DTCM. All six routines execute, including non-deleting destruction,
exhausted-heap allocation failure, and a single node with two optional buffers.
Native helpers run without stubs. Independent predictions cover all four MiB of
main RAM, the modeled I/O page, caller stores/calls, returns, SP and r4-r11.
DTCM outside the observed stack extent is preserved; helper stack contents inside
that extent are observational. Copied closed-file fixtures are separate from the
live open-file checks. Null-IRQ teardown and multi-node cleanup retain static
matching evidence only. See `isolated_arm_cases_v1.json` in the runtime directory.

`artifact_validation_v4.json` validates six images, sixteen graphics dumps,
twelve RAM/DTCM snapshot pairs and all 104 unchanged saves. The battle command
wheel and retry menu were visually inspected. Final battle animation pixels vary
even when repeating the unchanged probe; the cause is unconfirmed. Constructor
inputs, complete lifecycle records, captured BG VRAM, palettes and OAM agree.
This is not a claim of deterministic final rendering or an independent rasterizer.

`build/analysis/battle_scheduler_lifecycle_build_validation.json` records full
matching, golden packaged/native ROMs, zero-difference relinking, progress checks
and 81 passing tests. `battle_scheduler_lifecycle_object_provenance.json` pins the
final objects and source. Their hashes and the build logs were revalidated after
the documentation interlude; subsequent work changed private probes and docs only.

### Battle hit-bonus roll

[battle_hit_bonus.c](../../src/battle/battle_hit_bonus.c) reconstructs
`BattleParty_RollHitBonus` at `0x020720FC..0x020722AC` (432 bytes). It preserves
the native floating-point operations, intermediate truncations and badge branch
layout. The public isolated object and actual build object both match completely.
No assembly or compiler flags were added. The unsigned integer-to-double helper
at `0x02044BC0` is now named `_dfltu` in resident metadata, matching the compiler's
symbol. Native inspection establishes normalization of unsigned `r0` and the
IEEE-754 result in `r1:r0`; the adjacent `_dflt` additionally handles signed input.
This helper rename adds no C/C++ coverage.

The user argument is a `BattlePartyActor`; the optional target is a `BattleActor`.
With no target, the routine resolves the user's unsigned target ID first.
`actor.unk_00e` holds the Stache value copied from the save during battle entry.
A zero value returns false without drawing a random number. Otherwise the base
chance is the truncation toward zero of
`64 * (256 * stache - 128 * target_level) / 65536`, using the target's unsigned
seven-bit level. The native implementation evaluates this through separate
binary64 conversions and operations; keep those source expressions intact.

If the user has a scene-object pointer, its formation maps through the byte table
at `0x020BE8FC` to a saved party member. The low badge byte at
`read32(0x02059FE8) + 36 * member + 1048` selects the modifier: item `0x300E`
multiplies the truncated base by `640/256`, and `0x3028` by `1152/256`, each with
another truncation toward zero. The result is clamped to 1 through 99, then
compared with `Random_NextModulo(100)` using a strict signed less-than test.
Bro/Ice Flower setup calls this for six target slots. Its consumers at
`0x020C2D24` and `0x020C4C10` choose hit kind 6 when true, otherwise kind 1.
The public name describes that confirmed bonus selection without renaming the
shared stat field or assigning a speculative item name.

The private probe is `build/analysis/probe_battle_hit_bonus.py`. Both 380-frame
routes use checkpoint 83 and the compatible `ov17_bros_menu83.dst` snapshot
(SHA-1 `21d2e64a24b389689627292539103880c6761b47`). Two Down presses, each followed
by 30 neutral frames, then `a:8`, `wait:100`, `a:8`, `wait:180`, with a released
frame after every action, enter Bro Flower setup. All six rolls occur at frame
197. `build/runtime/eur_battle_hit_bonus/evidence_fire83.json` records ordinary
inputs; `evidence_fire83_cases.json` adds six guarded per-call fixtures:

- Zero Stache returns false and consumes no random number.
- A null target argument exercises target lookup; negative Stache clamps to 1.
- Maximum signed-halfword Stache clamps to 99 and accepts a lower draw.
- Badge `0x300E` truncates base 9 times 2.5 to 22.
- Badge `0x3028` truncates base 9 times 4.5 to 40.
- A draw equal to chance 88 returns false.

The oracle checks complete 148-byte party storage, 672-byte enemy storage,
the 1,380-byte save record and the six-byte formation map. An independent integer
model checks the native binary64 result: the signed-halfword stat, seven-bit
level, power-of-two scaling and badge products all remain exactly representable
at these stages. Division toward zero models each conversion to integer.
The RNG state at `0x02060E04`, its zero-seed fallback, next seed and modulo result
are derived separately from [random.c](../../src/game/random.c). Across both
routes, all 12 returns and 11 RNG steps pass; optional lookup runs once.

Each fixture restores its changed stat, object-pointer, target-ID, level and/or
badge bytes at the guarded return. It also restores the RNG state and `r0` result
that the ordinary call would have produced, before the caller can consume them.
Those original expectations agree with the corresponding ordinary-run records.
All six restorations succeed; original save files are untouched. These cases
verify controlled function behavior, not naturally encountered equipment or stats.

`artifact_validation.json` validates 16 screenshots, eight graphics dumps and
all 104 unchanged source saves. The ordinary route's eight captures match the
identical prefix of the earlier flower replay. Separately, the restored fixture
route produces the same captures, final graphics buffers and final RNG state
as the ordinary run. Both final setup screens were visually inspected. Later
damage, attack completion and independent rasterization are outside this probe;
the boolean's meaning is supported by its native consumers rather than a later
damage replay. No calls remain pending.

Full matching checks, golden packaged/native ROM hashes, zero-difference native
relinking, progress generation and all 81 tests pass. Current evidence is in
`build/analysis/battle_hit_bonus_build_validation.json`; earlier build logs from
before the shared forward declaration are retained separately.

### Battle relative-effect spawning

[battle_relative_effects.c](../../src/battle/battle_relative_effects.c) contains
`BattleSpriteEffect_SpawnRelative` at `0x02071E40` (200 bytes) and
`BattleModelEffect_SpawnRelative` at `0x02071F08` (252 bytes). The contiguous
range ends at `0x02072004`. Both the isolated public object and the actual build
object match every byte, including relocations and the model helper's literal
pool. The shared [effect API](../../include/game/battle_effect.h) replaces the
inconsistent local declarations in overlays 12, 13, 14 and 18. The native model
wrapper forwards its factory's returned pointer in `r0`; ignored returns in
other callers had obscured that contract.

Both helpers optionally project a reference object's position through
`BattlePosition_StoreViewRelative`. They add the resulting x/y/z to full-width
offsets and narrow to signed halfwords only when calling the spawn factory.
The model helper takes a separate optional parent. Without a parent, it adds
the main view origin at battle-context offsets `+0xCB9C/+0xCB9E` before spawning.
The first draft followed scheduled machine instructions by updating x/z/y;
using the coordinate tuple's x/y/z source order recovered the native register
allocation in both helpers. No assembly, flags or parameter-width changes were
needed to obtain the match.

The Cannonballers replay uses the existing checkpoint-83 battle snapshot
`build/runtime/eur_attack_helpers/cannon_setup83.dst`, whose SHA-1 is
`850ad8741a5474f336724f26698a502f1fb52457`. Its source save is
`83. Star Shrine - Third area (Before boss).sav`, SHA-1
`2cb577d3008975c390a2f00e2b2cd646e4005c1b`. After `a:8`, `wait:2200`, and the
driver's released frames, the replay has advanced 2,210 frames. Ordinary A/B/X/Y
inputs at frames 417/447/477/507 exercise the four launch windows. This run
does not inject battle commands or modify RAM; the prior battle snapshot's
setup is documented in `build/runtime/eur_attack_helpers/cannon_setup83.json`.

All 13 new helper calls return: six sprite calls with a reference, one model
call with a reference and no parent, and six model calls with a parent and no
reference. Seven independent projection checks cover raw-position flag 0,
view 0 and nonnegative projected depth. They verify the output's six bytes and
unchanged two-byte padding, exact helper arguments and all relevant object
records at call/return boundaries. The model factory's coordinate, parent and
scale fields are checked, and each returned handle is forwarded unchanged.
Other factory internals and the remaining bytes of the 48-byte sprite or
56-byte model effect record are observed, not independently reconstructed by
this probe. Effect cleanup, sprite-factory internals, alternate/raw views,
negative-depth clamping, sprite calls without a reference and extreme offsets
remain outside this runtime coverage.

The initial probe incorrectly assumed each scene object had its own heap
header and failed at frame 33. `BattleEntry_InitializeObjects` at `0x02074AD4`
instead establishes 70 records at `context + 0x5B0`, each 260 bytes. Its loop
at `0x02074CB0..0x02074D34` assigns object IDs and lookup pointers, advances by
`0x104`, and stops at 70. The corrected probe checks that full native initializer
range, record alignment and current slot/actor ownership, then compares the
entire 260-byte record. This corrects the oracle's ownership model; it does
not relax the per-call checks or change game code. The failed probe and report
are retained separately.

The successful report is
`build/runtime/eur_battle_relative_effects/evidence_cannon83_v2.json`, produced
by `build/analysis/probe_battle_relative_effects.py`; its source hash is pinned
in the report. `artifact_validation.json` verifies five 256-by-384 captures,
four graphics dumps and all 104 unchanged source saves. The final screenshot,
both 128 KiB BG buffers and each display's 512-byte palette prefix match the
earlier afterimage replay with identical inputs. The attack and final battle
menu were visually inspected. Full OAM and the remaining palette bytes have
validated capture extents/hashes, without a matching earlier baseline. No calls
remain pending and no drain frames were required.

Full `ninja check`, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Build logs and source hashes are in
`build/analysis/battle_relative_effects_build_validation.json`. The private
launch-controller and adjacent resource-effect drafts remain unmatched; their
bytes are excluded from this 452-byte addition.

### Shops

The private `build/analysis/probe_shop_buy_panel.py` checks the buying panel's
prices, discounts, affordability, quantities, new-item flags and show/hide behavior.
It uses compatible story-86 or story-65 HUD states and the corresponding original
battery saves. Its `coin86`, `equipment65` and `beans65` reports are under
`build/runtime/eur_shop_buy_panel/`; the selected shop IDs are 0, 2 and 14.

Entry substitutes one 72-byte decoded command at guarded EUR ARM9 field dispatch
`0x020823F8`, using opcode `0x121` with the shop ID, fade 1 and return-screen flag 1.
All original command bytes are restored at guarded request helper `0x0206ACF8`.
Ordinary buttons then select items, cancel quantity selection and leave the shop.
This establishes controlled scene entry, not interaction with the shopkeeper
through a normal story script. Older private shop probes used return-screen flag
0 and could finish with black screens; retain flag 1 for the tested visible return.

The oracle checks complete 936-byte buying panels and 1,380-byte live saves,
original-ROM item records, price arithmetic and ordered helper arguments with
SP-matched nested returns. Panel setup is observed helper output; the caller's
enable flag, row/list/category calls and hide-selection writes are derived.
Capture RAM/VRAM, palettes, OAM and both screens. Verify scene/panel pointer cleanup
at the destructor return, before the field overlay reloads and reuses those
addresses; a later nonzero value at the old shop global is not evidence of a leak.

The separate `coin86_limits` and `beans65_limits` fixtures temporarily set funds
to zero and inventory to the native limit at two guarded affordability calls.
Each edit is restored at that call's SP-matched return. They exercise rejection
branches, not real purchases or persistent changes to saves. The artifact verifier
checks source-save hashes and compares their captures with the ordinary routes.
Read each report's actual coverage before extending it; item setup, rendering
internals and unexercised controller branches remain outside this oracle.

`build/analysis/probe_shop_panel_lifecycle.py` extends those three ordinary-button
routes with constructor and destructor checks. It snapshots the full buying
allocation (936 bytes) and inventory allocation (848 bytes), including nested
base calls. The base prefix is 828 bytes. Buying initialization writes two
parallel two-byte arrays at offsets 929 and 931; inventory initialization writes
only the low two bytes of the word at offset 828. Preserve the remaining bytes.
Expected vtables, initialized fields and position-helper writes are derived;
parent oracles inherit child expectations, not observed constructor output.
Validate the object before the heap free and never read it after release.
The routes exercise base initialization/destruction and both derived initializers
and deleting entries, but not the four other destructor wrappers. Reports in
`build/runtime/eur_shop_panel_lifecycle/` and the private
`verify_shop_lifecycle_artifacts.py` compare captures with the buying-panel batch.

`build/analysis/probe_shop_text_tasks.py` checks queued item/stock text updates
and item-icon lookup. Tasks use the shared 72-byte pool; full-width arguments
at offset 40 are narrowed only when the callback calls the renderer. The checks
derive all eleven text-call arguments, icon indices from original item records,
128-byte icon copies and the deferred-removal flag at task offset 8. Allocation
and text-renderer internals remain observed helper outputs. Icon lookup leaves
the renderer and live save unchanged. Scrolling exercises the item callbacks;
a six-item equipment list fits without scrolling and does not queue those tasks.

The `coin86_purchase` fixture changes the live Mushroom inventory byte to one
at byte-guarded EUR ARM9 field request `0x0206ACF8`, before scene construction:
`write8(read32(0x02059FE8) + 0x48E, 1)` in ARM9 main RAM. Ordinary buttons
buy one Mushroom; independent checks verify the seven-coin cost, quantity change
from one to two and stock-text callback. It restores the original inventory byte
and little-endian currency word at `read32(0x02059FE8) + 0x488` at guarded shop
deletion, before field return. The equipment
and bean routes have no purchase fixture. Reports and captures are under
`build/runtime/eur_shop_text_tasks/`; source battery saves retain their hashes.

The focused `probe_shop_text_indices.py` reuses those inputs and fixtures. Name
indices use record halfword `+2`, adding one when quantity is not one; description
indices use halfword `+4` or `+6` according to the live category byte in
`data_ov009_0207e28c`. Both native functions retain a full-width accumulator and
narrow to `u16` at return. A `u16` local can instead move narrowing into the
conditional increment. Preserve the native null-record behavior too: the name
helper still adds the plural offset to zero. The three routes exercise singular,
plural and both description selectors; invalid records and index wraparound
remain unexercised. Reports under `build/runtime/eur_shop_text_indices/` check
unchanged renderer/save/selector records and compare every capture with the
preceding text-task batch. The broader buying-price API checks are not repeated.

`probe_shop_menu.py` adds cursor creation/drawing, option model/text drawing and
menu-task removal checks to those same routes. The separate
`probe_shop_menu_navigation.py coin86_menu_purchase` pass uses ordinary down/up
buttons to visit all three choices before continuing the purchase route.
Reports are under `build/runtime/eur_shop_menu/`. Menu rows use every second
point in the shared list; preserve the signed selection byte and narrowing to
an unsigned row argument. Model offsets are signed halfwords, sprite coordinates
are Q12 words, and option drawing requires a list scale greater than 409.
The probe derives row geometry, draw priorities, zero-angle matrices, draw-list
appends and full 72-byte task records; model checks cover a 128-byte prefix and
sprite checks cover a 64-byte prefix, not an inferred full allocation.
Model initialization and virtual animation internals remain observed helper
outputs. `ShopMenuNavigation_Clear` marks groups 3 and 5 for deferred removal;
it does not free them during that call. Check each live task's flag change and
unchanged links before the later cleanup. The four final routes cover all five
new entry points, all three selected rows and both sides of the draw threshold.
Negative coordinate and invalid-row branches remain unexercised. The artifact
verifier checks all captures and compares identical routes and unchanged starting
captures with the preceding text-index reports; later navigation frames are new
evidence, not an identical-input baseline.

`shop_help_text.c` owns the adjacent help-text draw and background initialization
functions. MWCC C with a named `GameTextToken` result reproduces the native
return-buffer slot; discarding the result or using the C++ draft changes three
stack offsets. The draw loop tests termination at its head, and the tilemap loop
increments its pointer separately from the store. Preserve the explicit stack
loads for the fill value rather than replacing them with a constant argument.

`probe_shop_help_text.py` repeats all four menu routes while replacing the prior
per-frame menu oracle with focused help-text checks. Derive character and map
VRAM addresses from main DISPCNT/BG0CNT, message pointers from the current language
table and original item records, all fourteen text-init arguments and the full
48-byte initialized text state, preserving reserved bits and padding. It checks
24,576-byte character clears, 1,024 sequential tilemap halfwords, 5,120-byte pixel
clears and 4,096-byte copies at character base + 20,480. Text-token rasterization
remains observed helper output; the copy and final VRAM buffers are independently
compared. The initial message-only oracle missed kind-1 item descriptions and
was corrected from the native lookup before rerunning the full affected route.
The final reports in `build/runtime/eur_shop_help_text/` cover kind-1 descriptions
and kind-2 menu messages; kind-0 names, empty strings, alternate languages and
invalid entries remain unexercised by the new functions. Every capture matches
the preceding menu-batch route, and all source saves retain their hashes.

`probe_shop_graphics.py` and `shop_graphics_oracles.py` extend the text-index
routes with the party-label callback, bitmap values and placeholder dashes,
packed OBJ placeholders and 4bpp-to-8bpp glyph expansion. The four reports in
`build/runtime/eur_shop_graphics/` exercise every new entry point. They derive
pixels/nibbles independently, including transparent pixels, palette offsets,
linear-to-tiled copying, sprite coordinates and draw-list appends. All captures
match the preceding help-text batch. Earlier help-token checks are retained in
their own reports, not repeated by this graphics probe.

Derive buffer sizes from `shop_resources.cpp`: bitmap pixels allocate 53,248
bytes, of which 49,152 are initialized and transferred; packed and expanded
glyph allocations are 2,816 and 5,632 bytes; scratch and strip buffers are each
5,120 bytes. The OBJ placeholder clears 512 scratch bytes but transfers only
256. Help text uses a separate 6,144-byte allocation. Check each full allocation
and preserve unused tails rather than assuming that a clear or upload covers it.
The bitmap value routine mixes unsigned value division with signed divisor
reduction. Glyph expansion reads the source again after the low-nibble store;
`const` does not rule out aliasing. Preserve these native operations.

This replay exercises two- and three-digit bitmap values, visible and hidden
label callbacks, even-X OBJ placeholders and 32-/128-byte glyph expansions.
Zero/one-digit values, odd-X placeholders, overlapping glyph buffers, zero-length
expansion and invalid coordinates remain unexercised. Visibility and mirror
flags are counted separately; those counts alone do not establish every combined
branch. Controlled shop entry and purchase fixtures remain distinct from normal
NPC navigation. Neighboring bitmap and number helpers still contain native gaps;
do not count an entire private candidate file because one function matches.

The adjacent scale-task creator/updater are in `shop_list_scale.cpp`, with
`shop_list_point_model.cpp` on the far side of the remaining model-initializer
gap. Scale tasks store duration/current/target/step/delay at offsets 36/40/44/48/52
in the shared 72-byte task pool. After a nonzero delay expires they request sound
234; scaling starts on the following update. The last scaling update writes the
exact target before calling the clamping setter and marking the task for deferred
removal. The point-model callback uses the list's compressed X coordinate and
workspace-relative Y, rounds each Q12 value toward zero, then stores halfwords.
The last point's draw priority decreases by two while entering or in a nonzero
list phase. Preserve the native subtraction rather than replacing it with a
constant assignment solely because the preceding offset starts at zero.

`probe_shop_list_effects.py` with `shop_list_effects_oracles.py` verifies these
fields and tracks every created scale task through its complete update sequence.
It derives clamped scale, model coordinates, priorities and draw-list appends,
checking 72-byte tasks, 656-byte lists, 1,380-byte saves and 128-byte model prefixes.
Allocation and sound-queue internals remain observed helper outputs; their call
arguments and the caller's writes are checked independently. Reports and capture
comparisons are in `build/runtime/eur_shop_list_effects/`. Use the recorded branch
counts when extending the routes; ordinary positive scaling does not establish
negative-step, division-by-zero or clamp-boundary coverage.

The currency callback and display-initialization wrapper are adjacent in
`shop_display_control.cpp`. Currency uses 72-byte tasks, with animation fields
at offsets 32 through 60. Only part zero updates the value; the other part still
submits its sprite. Coins are a signed word at live-save offset 1160, beans an
unsigned halfword at 1164, capped at 999,999 and 999 respectively. The animation
uses Q8 values and durations 1/8/16/32. Its six-digit renderer accepts a full
32-bit value; the older `u16` prototype incorrectly narrowed currency arguments.
The corrected prototype preserves the bytes of existing linked callers.

`shop_panel_zoom.cpp` owns open/close updates; `shop_panel_zoom_start.cpp` owns
the separate close-task creator. `ShopSceneWork` names the busy/visible bytes
at offsets `0xA1/0xA2` and signed Q12 scale at `0xA4`, preserving the background
code's byte view. The 72-byte tasks hold step/delay at offsets 40/44. Updates
derive panel coordinates around Y=96, submit the full nine-argument affine
transform, and change BG2 visibility and deferred-removal flags at completion.

`probe_shop_display_zoom.py`, `shop_display_zoom_oracles.py` and
`verify_shop_display_zoom_artifacts.py` cover all five functions across four
routes in `build/runtime/eur_shop_display_zoom/`. They derive full task records,
64-byte sprite prefixes, live-save/work/panel fields, draw-list appends and 217
numeric renders using per-nibble font decoding and independent tiled copies.
Checks preserve the full 2,816-byte packed font, 5,120-byte scratch/strip buffers
and 65,536-byte main OBJ VRAM. Both created close tasks complete after eight
updates. Nested affine checks derive the matrices and all 112 ordered hardware
stores; the corrected zero-scale overflow expectation was rerun successfully.
All captures match the preceding list-effects routes and all source saves remain
unchanged. Initializer helper outputs, allocation and sound internals are observed
with checked call arguments. The routes cover coins/beans, part zero/one, optional
party initialization, both zoom directions, delays and final clamps. Currency
changes cover a seven-coin decrease with duration one; increases, cap enforcement,
longer animations and special-shop initialization remain unexercised.

`shop_quantity.cpp` owns eight adjacent helpers for the quantity popup: total
price and decimal digits, selected-icon copying, item-name rendering, three
sprite callbacks and quantity arrows. The 72-byte task view names part/vertical offset at +40,
cached total/base tile at +44, previous quantity at +48 and item at +52. Workspace
unit price is a signed word at +0xA8 and quantity a signed halfword at +0xAC.
The digit callback updates the name only when changing to or from quantity one,
and suppresses a zero tens digit. Visibility and scale thresholds control drawing;
closed panels mark their remaining tasks for deferred removal. Sprite roles that
have not been identified from their art retain generic behavioral names.

`probe_shop_quantity.py` and `shop_quantity_oracles.py` repeat the four preceding
routes with focused quantity checks. They derive full task records, sprite
prefixes, matrix halfwords, draw-list appends, total/digit caches and removal
flags. Selected icons copy 128 bytes from the current row into main OBJ VRAM;
item names mirror 1,152 bytes to sub OBJ unless the special-shop flag is set.
These copies preserve the complete 65,536-byte buffers. Name-renderer pixels are
observed helper output with independently checked arguments and subsequent copy;
numeric rendering remains independently checked per nibble and tiled transfer.
All 680 artifacts from identical routes match the display/zoom batch.

The additional `probe_shop_quantity_selection.py` uses ordinary Up/Down holds
before confirming one purchase. Its 2,073-frame report under
`build/runtime/eur_shop_quantity_selection/` records 50 distinct quantities from
1 through 66, including visible tens digits and both singular/plural transitions.
Held-input acceleration skips some intermediate values; this is not coverage of
every integer in that interval. The six captures before the added inputs match
the ordinary purchase baseline. Later captures have different input/timing and
are separate evidence. All five runs total 8,601 frames, preserve all original
saves and return to the field without pending calls. Special-shop mirroring bypass,
invalid panel indices, negative quantities/prices and out-of-range scales remain
unexercised. Earlier currency/zoom checks remain in their own reports.


The quantity-arrow callback uses a model resource at task offset +28 and a
selling-mode alias at +44. The upper arrow independently checks stock capacity
and funds when buying, or available quantity when selling; the lower arrow is
hidden at quantity one. Hidden arrows move to (-128, -128) but still enter the
draw list. The shared panel virtual interface names adjustment at slot +12 and
quantity at +16. Preserve the caller's explicit halfword narrowing of quantity,
load order around callbacks, and the scale read before model scale stores.

`shop_price_adjustment.cpp` owns mode setting and three adjacent draw callbacks.
The task pointer is at workspace +0x8BC; modes 0 through 4 map to task phases
0, 100, 200, 300 and 400. The 72-byte task shares parent/part/cache fields with
its children. Value and marker widths derive from the virtual adjustment times
selected quantity; coordinates use signed Q12 division toward zero. The symbol
only requires a nonempty panel, while value/marker also require an enabled panel.
Both model coordinates are computed before either is stored, matching the
native cached loads. The styled numeric helper takes five arguments, including
the stack argument; its rasterized pixels remain observed helper output.

`probe_shop_adjustment.py` and `shop_adjustment_oracles.py` cover these five new
functions on the four preceding routes. Supplemental capacity and funds probes
use separate directories under `build/runtime/eur_shop_adjustment_capacity/`
and `eur_shop_adjustment_funds/`. Ordinary Up/Down holds reach 98 Mushrooms with
one already owned (99-item capacity), or 38 Ultra Mushrooms at 32 coins each
with 1,235 coins (the next quantity would cost 1,248). The capacity route returns
to one and buys one using the existing restored fixture. The funds route uses
unchanged inventory/currency and cancels without a purchase. The six runs total
10,282 frames and 33,740 returns through the new functions, with 95 capacity-only
and 176 funds-only blocked upper-arrow updates. They independently derive full
tasks/parents/panels/saves, model/sprite writes and 26,170 draw-list appends;
416 ordinary numeric renders are checked independently, while 187 styled-number
outputs are recorded with checked arguments and unchanged font data.

The funds route's initial final-coverage assertion incorrectly equated a list
longer than seven rows with scrolling callbacks. Two Down presses remain within
the initial visible rows; native selection code queues replacement text only
when scrolling across an edge. Correcting that route-specific assertion and
rerunning the full replay passed, without changing game code. All 108 screenshots,
972 graphics dumps and 104 source saves validate. Baseline comparisons cover
750 artifact pairs with identical starting state/input/timing; later supplementary
captures are separate evidence. The capacity and affordability popups were
visually inspected. Selling, bean-funded quantity arrows, zero-count panels,
mode zero, missing mode tasks and invalid mode inputs remain unexercised.
The nearby percentage renderer is still a native gap; do not infer its matching
status from these callbacks or their runtime helper observations.


`shop_panel_slide.cpp` owns the 20-update vertical transition used between
buying and selling scenes. Its 72-byte task stores remaining updates/current/
target/velocity/acceleration at +36/+40/+44/+48/+52. `ShopSceneWork` names
`panel_offset_y` at +0x8C8, consumed by the panel Y query. Entry starts at
-112 pixels and targets zero; exit starts at zero and targets -112 pixels,
snapping there when it reaches -80. The native update reuses the loaded target
when clamping the zero-target case. Preserving that load and the creator's
branch layout resolves the old draft's four-byte/register differences.

The earlier B-to-close routes do not execute these functions. At the category
menu, Up from Items wraps to Sell; A enters selling, B returns to buying, and
B closes the shop. For the purchase route this sequence replaces the final
four actions, beginning at frame 1,317; replacing only the last two acts after
the shop has already closed. Regular equipment/bean routes replace the last two
actions at frame 1,057. No extra RAM fixture is needed for this transition.

`probe_shop_panel_slide.py`, `shop_panel_slide_oracles.py` and their artifact
verifier cover those three routes. Twelve allocated tasks finish 240 updates,
with both directions and clamps. Expected positions and velocities use the
arithmetic-series formula from the initial state, independently of the native
recurrence; complete 72-byte tasks, 2,492-byte workspace and 1,380-byte live saves
are checked at each boundary. Allocation internals remain observed helper output.
Removal is deferred, and all three scene instances per route are checked before
overlay reuse. Twenty updates need not occupy exactly twenty emulator frames:
one bean-shop entry had an extra frame before completing its update sequence.
The verifier checks all twenty ordered updates and their recorded completion.

The three passing replays total 5,981 frames and 605 checked returns, including
252 through the new functions. All 58 screenshots, 522 graphics dumps and 104
source saves validate; 340 unchanged starting artifact pairs match the previous
batch. Later Sell transitions have different inputs and are separate evidence.
The moving panel and selling scene were visually inspected. The original numeric
oracle verifies 150 renders, while quantity/adjustment callback oracles remain
in the preceding batch's reports. Initial no-coverage routes are retained as
failed discovery attempts; they are not included in these passing totals.
Reports: `build/runtime/eur_shop_panel_slide/`.


`shop_subscreen_text.c` initializes sub-engine BG0 and draws the selling message.
The native 24-byte local initializer is a 4-by-3 halfword table, selected by
workspace variant/mode at +0x97/+0x98. An explicit manual copy produced different
instructions; a local aggregate initializer reproduces the native halfword copy,
and the two-dimensional declaration reproduces the separate row/column address
calculation. The module owns both its 420-byte code range and the 24-byte data
range at `0x0207E31C`; the latter does not count toward matching code coverage.
The compiled table is local `@312`. Metadata must describe the actual local
symbol, as with the existing overlay-2 initializer; the first full build matched
all module bytes but correctly rejected the old global table name. Reconfiguring
with the local symbol made the complete module/symbol checks pass. Verify the
emitted symbol again if future includes change the compiler's anonymous labels.

The message buffer at workspace +0x70 allocates 14,336 bytes, while this function
clears and copies 10,240. It clears 24,576 character bytes, writes 1,024 sequential
tilemap halfwords and enables BG0 before drawing. The VRAM copy begins 6,144 bytes
into the character region. The shared native VRAM getters read main DISPCNT bank
offsets even when selecting the sub engine; preserve their actual behavior.
Both clear values use native stack loads, as in the earlier help-text functions.

`probe_shop_subscreen_text.py` and `shop_subscreen_text_oracles.py` repeat the
three Sell/return routes. They check the native/local 24-byte tables, localized
message pointer, all 14 text-init arguments and the complete 48-byte initialized
text state, clears, tilemap, BG0 flag and VRAM transfer. Checks preserve the full
14,336-byte allocation, 131,072-byte sub BG VRAM, workspace, renderer and live
save. The 316 text tokens' pixels/state are observed helper output, followed by
an independently checked copy; the allocation tail remains protected.

The three runs total 5,981 frames and 356 checked returns, including three
initializer calls. They cover variant/mode pairs (0,0), (0,1), (3,2), English
language index 1 and message IDs 40/41/46. Other variants, languages, empty
messages and already-enabled BG0 remain unexercised. All 58 screenshots, 522
graphics dumps and 104 source saves validate; all 580 artifact pairs match the
preceding slide routes. The bean-shop selling message was visually inspected.
The ordinary numeric oracle checks 150 renders; slide and quantity/adjustment
oracles are retained in their earlier reports rather than repeated here.
Reports: `build/runtime/eur_shop_subscreen_text/`.

`shop_party_bitmap.cpp` draws a member's equipped item name and icon into the
shop bitmap. Its complete 484-byte C++ function at `0x0207BF1C` matches using the
shared `ShopSceneWork` type. The original top-tested token loop stops on NUL or
`FF 00`; the coordinate tables contain four pairs of halfwords. Flattening those
pairs introduced an extra address calculation in the initial private draft.
The native halfword stack clear and eight-byte token return storage are retained.

`probe_shop_party_bitmap.py` and `shop_party_bitmap_oracles.py` repeat the same
three Sell/return routes. Equipment and bean shops exercise 24 renderer calls,
covering all four members, both item categories and both native callers. The
oracle derives localized names from the original item records/category selector
and live language tables, all 14 text-init arguments and the padding-preserving
48-byte text state, the 832-byte scratch clear, 104-by-12 bitmap clear, centered
4bpp-to-8bpp tile copy with palette offset 16 and 8bpp icon copy with offset 80.
Independent pixel coordinates preserve transparent pixels and untouched tails.

Live `GameHeapBlock` headers establish complete allocations: bitmap 53,248 bytes,
scratch 5,120 and icons 1,536. The oracle also preserves their headers, the
complete localized string allocation, workspace, renderer and live save. The
346 text tokens' state and first 832 scratch bytes remain observed renderer
output; subsequent bitmap copies are independently derived. These runs use
English, NUL termination, no clipping and an already-set dirty flag. `FF 00`,
other languages, clipped names and an initially-clear dirty flag remain untested.

All three replays pass 5,981 frames and 377 checked root returns; the retained
numeric oracle verifies 150 renders. All 58 screenshots, 522 graphics dumps and
104 source saves validate, with all 580 artifact pairs identical to the preceding
selling-message batch. Equipment names and party icons were visually inspected.
Each route closes three scene instances, with no pending calls or drain frames.
Earlier caption, slide and quantity/adjustment oracles are not repeated here.
Reports: `build/runtime/eur_shop_party_bitmap/`.

`shop_confirmation_cursor.cpp` covers the contiguous 232-byte clear/create/draw
group at `0x02076FBC` through `0x020770A4`. The shared 72-byte row task now names
its cursor coordinates at +40/+44. Creation sets (86,112), attaches a model,
starts animation zero and sets the low two model flag bits to one. Drawing adds
80 pixels per Yes/No selection from the signed workspace byte +0x9F and submits
at priority 10. The clear routine preserves an indirect tail call to group 3's
deferred-removal helper. The nearby 68-byte prompt-label callback is a separate
unlinked gap; this cursor group does not establish its execution coverage.

The ordinary confirmation route opens an equipment item with two A presses,
moves right/left between Yes and No and cancels with B; the bean route moves to
No and confirms with A. Neither purchases an item or changes inventory through
a fixture. `probe_shop_confirmation_cursor.py` checks 2,742 frames and 532 root
returns, including 486 new-function calls (two creates, 482 draws, two clears).
It derives task fields, attachment/pool changes, flags, (86/166,112) positions,
draw-list appends and deferred removal. Allocator, resource initialization and
animation outputs are explicitly observed helpers.

Models occupy complete 336-byte slots in the shared pool, established by native
`0x02068CE4` allocation and `0x02068DF0` selection. Check each slot, full tasks,
workspace and save at helper boundaries. The task's removal flag is set one
frame before its unlink/resource release and pool return. Check the last task
fields before the pool return, then only still-live list/pool records. A tail
call can put several pending returns at the same address and SP; finish their
nested records in order before processing the caller's next instruction.

Reports under `build/runtime/eur_shop_confirmation_cursor/` validate 30 screenshots,
270 graphics dumps and 104 unchanged saves. All 160 equipment-route artifacts
match the earlier discovery replay; the bean route uses a different cancellation
sequence. Both confirmation positions were visually inspected. The retained
numeric oracle verifies 17 renders; broader buying, bitmap and selling checks
remain in their earlier reports. Successful purchases, post-purchase equipment
prompts and empty-group clearing are outside this route's coverage.

`shop_equipment_highlight.cpp` reconstructs the contiguous 1,172-byte group at
`0x02076B28` through `0x02076FBC`: stop, start and per-frame update. The signed
workspace flag at +0xAF controls the fade. Start copies the 2 KiB map at +0xB6,
clears the left half for a one-member party, initializes sub-screen blending and
creates a 72-byte task. Update selects the eligible party group, writes two
2-by-3 tile patterns and switches the background's vertical offset between zero
and -88. Its Q12 alpha fades between zero and eight; division by six leaves a
remainder, so both complete transitions take seven updates before clamping.
The separate `data_ov009_0207eaf2` symbol is the native alias of that map inside
the workspace. Loading it directly preserves the original literal reference.

The matching pause-resource code supplied the tile-loop counterpart: write
`map[32 * row + column]`, allowing the compiler to derive the row stride, instead
of manually introducing the decompiler's cached index. Incrementing the column
before the 16-bit tile counter preserves the native induction-update/truncation
order. This resolved both register and scheduling differences without inline
assembly, compiler changes or source permutations. All three complete functions
match, including their literal pools.

Private `probe_shop_highlight.py` checks the ordinary confirmation and short
scrolling routes after the established controlled shop entry from checkpoint 65.
Reports under `build/runtime/eur_shop_highlight/` cover 2,482 frames, 954 new
function returns, 1,928 independently expected GPU stores and 948 twelve-entry
map updates. The four selected clothing items exercise both eligible-group
results and both vertical offsets. Oracles preserve the full 2,492-byte workspace,
1,380-byte save, 72-byte task, selected-panel allocation and 128 KiB sub BG memory
at helper boundaries; item flags are checked against original resident records.
Allocator output is explicitly observed; initialized task fields, fade state,
map changes, helper results and register writes are independently derived.

Both tasks reach removal after fade-out. Here marking and pool return occur
later in the same scheduler pass, unlike the confirmation cursor's next-frame
removal. Hook ordering establishes the lifetime even when frame numbers tie.
The artifact verifier initially assumed a later frame; that assumption was
corrected and the verifier passed against both successful replay reports.
Full task contents and neighbor links are checked before pool return, then only
live pool records and counters are read. All 32 screenshots, 288 graphics dumps
and 104 source saves validate; all 160 confirmation-route artifact pairs match
the preceding cursor replay. Both highlighted portrait groups were visually
inspected. One-member clearing, unavailable party groups, early fade interruption
and successful purchases remain outside these routes' coverage.

The stock-row group adds 1,008 matching bytes: caption placement at `0x0207218C`,
quantity digits, icons, text segments and row updates at `0x02072470` through
`0x02072788`, followed by stock-panel hide/show through `0x02072818`.
The 72-byte task stores its row, quantity, palette and priority as halfwords at
+40/+42/+44/+46, and tile/position/offset words at +48 through +64. ResourceB
sprites occupy complete 64-byte slots, unlike ResourceA models' 336-byte slots.
The panel's 224-byte layout preserves its old byte views while naming enabled,
filter mode and filter item at +0/+210/+212. Row positioning uses the signed ring
index and global vertical offset; filtering compares the exact item or bits
24 through 30 of its usable-item record. Two typed 20-byte record pointers
preserve native address calculation and field-load order in the matching update.

Private `probe_shop_stock_rows.py` and `verify_shop_stock_rows_artifacts.py`
validate checkpoint 30 and 86 routes under `build/runtime/eur_shop_stock_rows/`:
1,362 frames, 46,449 new-function returns, 5,199 row updates, 39,860 independently
derived sprite submissions and 134 deferred task removals. All 28 screenshots,
252 graphics dumps and 104 unchanged source saves validate. The routes exercise
six versus nine rows, hidden and already-enabled show paths, filter modes zero
and one, both palette results and suppression of 1,384 leading-zero digits.
Full task/parent/sprite, workspace, save and panel allocations are compared at
helper boundaries. Panel setup and row/caption factory outputs remain observed
helpers. Sprite attributes, positions and draw-list links are independently
derived; captured native-renderer pixels are observations, not a pixel oracle.
Filter mode two, empty stock and successful purchases were not exercised.

The first oracle checked the caller's x store at the X helper's return address,
before that store executed. Moving caller-owned expectations to later helper
boundaries fixed the replay. Requiring leading-zero suppression in every route
also failed for checkpoint 86's two-digit stock; the successful suite checks it
on checkpoint 30. Despite its filename, `eur_story_030_hud_verified.dst` opens in
the save menu. Its tested prelude is `wait:180 b:8 wait:240`, which cancels the
menu and reaches the guarded FieldVM shop entry. Both final replays finish shop
cleanup with no pending hooks; no inventory fixture or purchase is needed.

`ShopStockPanel_RebuildList` subsequently reconstructs the 356-byte initializer
at `0x02073098`. It forwards both the workspace menu selector at +0x98 and the
caller's selection to `ShopStock_MapMenuCategoryToItemClass`; the pseudocode
omitted that second argument. It selects the inventory array and item tag,
packs entries whose signed quantity byte is nonzero, and resets the filter and
ring indices. The native clear is 198 bytes: it clears 99 of the 100 item slots
and preserves the final slot. The shared panel now names its item limit at +2.

Private `probe_shop_stock_setup.py` and `verify_shop_stock_setup_artifacts.py`
check checkpoint 30 and 86 category changes and exit in 1,152 frames. Four
initializer calls produce usable/Bros. item counts of 6/5 and 13/10 respectively.
The probes independently derive the full panel allocation at mapping, clear,
initial-renderer and return boundaries, including omitted zero-count entries,
item tags, pointers, resets, untouched fields and the nonzero final slot `00 EA`.
The workspace, save, pointers and heap header remain unchanged across the call.
All 14 screenshots, 126 graphics dumps and 104 source-save hashes validate.
The initial text renderer remains an observed helper. A separate ordinary
equipment-menu route did not call this initializer and failed its coverage
assertion; it is not counted as a successful replay. Clothing and badge branches,
empty inventory and independently calculated renderer pixels remain unexercised.
Reports are under `build/runtime/eur_shop_stock_setup/`.

### Save menus

The private `build/runtime/eur_save_state_transfer/save55.dst` is an initialized
save menu derived from checkpoint 55. The similarly named `load55.dst` was
recorded after choosing Start and entering the field; its name does not establish
an initialized load menu. Use a cold boot and the recorded input schedule when
you need the load menu's construction or entry callbacks.
`build/analysis/probe_save_write_effects.py` records ordinary confirmation input
from it and verifies the brightness and background-zoom callbacks. Its report
is `build/runtime/eur_save_write_effects/evidence_save55.json`. The animation
starts while saving is in progress, before the final "Saved!" message; keep
that timing distinct from successful completion of the storage operation.
The probe derives full task records, blend-register writes, timers and helper
arguments, while affine-display and storage internals remain outside its oracle.
Use the source-save hash checks for every replay, including runs that write to
the emulator's battery-save copy.

`build/analysis/probe_save_write_setup.py` extends this route to effect-task
creation, rollback setup and menu exit. Its reports under
`build/runtime/eur_save_write_setup/` separate ordinary Save & Continue
(`evidence_continue55.json`) and Save & Quit (`evidence_quit55.json`) from the
controlled error-status run (`evidence_rollback_status55.json`). These reports
are runtime evidence, not proof that the corresponding source batch has been
committed; check the current manifest and Git state separately.

The error fixture uses EUR ARM9 main RAM with overlay 8 loaded. At guarded
controller entry `0x0206B598`, `r0` is the task pointer. It requires the 32-bit
phase at `r0 + 0x20` to be 201, timer at `r0 + 0x24` to be zero and completed-write
result at `r0 + 0x3C` to be 1, then writes `02 00 00 00` to that result once.
The native handler displays the failure message and starts rollback. This tests
the response to an injected status after a completed write; it does not simulate
a storage I/O failure. The observed heap address is not a universal patch.

The setup oracle derives writes to the 384-byte workspace prefix, created
72-byte tasks and their list links, plus blend and background registers. Initial
constructor records and allocation addresses are observed helper outputs. Exit
checks cover the 72-byte exit task and 48-byte scene task: `quit_to_title` is a
32-bit field, and the native exit uses positive brightness for quit and negative
brightness for continue. Text transitions, movement/fade callback internals and
physical audio remain outside this oracle. Consult the source layout and probe
before extending it; the workspace prefix does not describe the full allocation.

For normal save-menu entry and selection, `build/analysis/probe_save_menu_control.py`
leaves that initialized menu with B and re-enters by jumping at the save block.
Its `confirm55` and `navigation55` reports under `build/runtime/eur_save_menu_control/`
cover entry, both panel texts, directional wrapping on fresh presses, clamping
while holding a direction, cancellation and both save choices. They check complete
task records and a 416-byte workspace prefix. Text-stream results and workspace
changes inside model/dialog/scroll helpers are explicitly observed outputs;
do not describe those helper internals as independently verified by this probe.
The tested entry uses occupied slot zero; the empty-slot fallback remains untested.

`build/analysis/probe_save_confirmation.py` checks the adjoining confirmation and
write controller. Its seven reports under `build/runtime/eur_save_confirmation/`
separate normal continue/quit/Yes/No/B input from three error-response fixtures:
completed-write results 1 to 2 or 3, and a successful storage-probe return changed
to zero. They check complete task, workspace, live-save and lock/busy records;
the probe result and selected text/effect workspace changes are observed helper
outputs. The result-2 path invokes the selection callback synchronously, so model
its immediate state and workspace changes before checking the parent's return.
The probe-failure response reaches phase 1001 and remains on its error message.
These fixtures verify the controller's responses, not physical storage failures.

The private `build/analysis/probe_save_load_motion.py` checks save/load cursor
acceleration and updates, load-panel setup and all three panel-text widths.
Its reports are under `build/runtime/eur_save_load_motion/`. The save run starts
from the compatible menu snapshot; the load run cold boots checkpoint 55 and
navigates both file panels and the Start/Copy/Delete submenu before entering the
field. It independently derives signed division, complete 72-byte motion tasks,
the 416-byte workspace, input locks, created-task fields/list writes and BG
priorities. Constructor records and text-stream results are observed outputs.
The motion structure's checked 68-byte size describes only its prefix; the pool
allocates 72-byte tasks. Do not infer a two-entry text-width array from the save
menu alone: the load entry calls panel 2, and the shared array has three entries.

For load-menu file selection and exit, use the private
`build/analysis/probe_load_menu_control.py` and its `start55`/`cancel55` reports
under `build/runtime/eur_load_menu_control/`. Ordinary input covers both fade
signs and scene destinations, signed file-index wrapping, opposing directions,
and A/X taking priority over B/Y in this selector. Other menus use different
input priorities; recover each controller's order. The oracle derives complete
task/workspace/scene records, callback resets and both master-brightness registers;
dialog workspace effects are observed outputs with the caller's mode write
checked separately. A pre-Start title-animation frame varied in sub-screen OAM;
record that difference rather than claiming all cold-boot screenshots are equal.

The save scene's resource initializer has a direct literal-pool alias for the
second `SaveMenuSummary`: `0x02078330` is `0x02078290 + 0xA0`. Preserve that
verified alias when calling `SaveMenu_BuildLiveSummary`; deriving the pointer
from the workspace base introduced an extra ADD in the matching compiler.
The remaining accesses still use the shared workspace views.

Private `build/analysis/high_effort_50_to_55/probe_save_scene_resources.py`
checks this initializer after ordinary B/A exit and re-entry from `save55.dst`.
Its `occupied55_v1` and `empty55_v1` reports under
`build/runtime/eur_high_save_scene_resources/` each contain one complete call,
full scene/workspace/live-save/storage/text checks, independently derived
summary fields and panel writes, and eight ordered affine stores. Shared
resource-loader workspace effects and location drawing are bounded observations.
The empty-slot fixture clears the active slot's occupancy bit in the byte at
EUR ARM9 `0x0205E334`, once at fully guarded `SaveScene_LoadResources` entry
(`0x0206C474`). It restores the original byte at that same scene's guarded
`SaveScene_Delete` entry (`0x0206C87C`) after B cancels the menu; the induced
menu resources are discarded by normal scene cleanup. No save is confirmed.
Both the empty preview and the subsequent field return were visually checked;
this is an injected occupancy condition, not a naturally empty saved file.

### Game Over

The private `build/analysis/probe_game_over_entry.py` records a controlled entry
from checkpoint 86 using
`build/runtime/states/eur_story_086_hud_verified.dst`. At guarded EUR ARM9 field
dispatch `0x020823F8`, `r0` is the VM pointer and `r2` points to a 72-byte decoded
command. The fixture saves that record, substitutes opcode `0x123` with fade
argument 1, then restores all 72 bytes at native request helper `0x0206A9A4`,
before the transition. It changes decoded RAM once, not the ROM or stored script.
Validate the loaded code bytes and both pointer ranges before any substitution;
do not reuse an observed heap or command-buffer address as a fixed patch.

The historical report `build/runtime/eur_game_over_entry/evidence_086.json`
records scene initialization after 481 frames. It establishes a controlled
entry route, not a natural battle defeat or coverage of selection and exit.
Treat the old script as a route reference: its return hooks predate stack-pointer
matching and its name-prefix discovery can select newly linked functions without
an appropriate oracle. Before extending it, explicitly select monitored functions,
guard the request helper as well as the dispatcher, track nested returns by stack
pointer and derive expectations for each newly checked task/workspace field.

For the reconstructed entry, selection and exit controllers, use the newer
`build/analysis/probe_game_over_control.py`. Its `saved86` and `castle86` reports
under `build/runtime/eur_game_over_control/` cover ordinary menu input after the
controlled entry, returning to Shroob Castle and Peach's Castle respectively.
The `no_choices86` run additionally clears the byte at workspace `0x02078290 +
0x9F` and live-save `read32(0x02059FE8) + 0x516` once at guarded entry phase 0.
It verifies the no-choice dialog and X-triggered title return, not a naturally
empty save. The probe derives task/selection/flag/callback/fade/scene changes;
text/model/scroll workspace effects and the unpacked save payload are observed
helper outputs. Its return hooks track stack pointers. Check the report and
artifact verifier for actual branch coverage before extending the route.

`build/analysis/probe_game_over_resources.py` extends those runs to resource
initialization, checking the full `0x2808`-byte workspace, the text-pixel clear,
panel setup and ordered affine stores. Its `empty_slot86` fixture clears only
the active slot's occupancy bit in the RAM header at `0x0205E334`, once at
guarded resource entry `0x02070858`. This covers empty-slot setup and the
single-choice response; it does not delete a supplied save. Initial common
resources, stored summaries and image/text helper results are observed outputs.
Reports and the artifact verifier are under the corresponding private paths.

Do not assume affine origin readbacks stay constant across rendering. The
resource probe observed BG2Y advance without a CPU write during text setup;
DeSmuME's [renderer updates affine coordinates per scanline](https://github.com/TASEmulators/desmume/blob/master/desmume/src/GPU.cpp).
For this initializer, check the four ordered stores per background at guarded
`G2x_SetBGyAffine_` instructions, including destination, width and source-register
value. Check the coefficients separately. Record the submitted origins rather
than refreshing an expected origin from a later readback and calling it derived.

### Smash Eggs

For battle attack research, the compatible private snapshot
`build/runtime/eur_attack_helpers/ov17_bros_menu83.dst` provides a checkpoint-83
Bros. item menu. Inspect the visible item list and trace the selected command;
the filename does not prove which attack is currently selected. The native
loader mapping recorded in `build/analysis/probe_battle_attack_loader.py` maps
command 8 to overlay 15 and callback `0x020C5B4C`. This identifies a dispatch
route. The subsequent `build/analysis/probe_ov15_actors.py` replay confirmed
the visible Smash Eggs selection, actor command 8 and that callback together;
its successful report is `build/runtime/eur_overlay15/evidence_final83.json`.
At callback entry, check the actor's command halfword at `+0x7C` and callback
pointer at `+0x78`. The menu field at battle-context `+0x11A` can still contain
2 at this point; it does not identify the active attack. During battle,
`gBattleContext` is the pointer stored in ARM9
main RAM at `0x020C0718`; dereference it before reading object-relative fields.
Verify snapshot provenance and byte guards before reusing this route, and
distinguish a controlled encounter from normal story navigation.

The follow-up `build/analysis/probe_ov15_pair.py` adds targeting, RNG, retreat,
visibility and badge-table checks. Its successful reports are
`build/runtime/eur_overlay15/evidence_pair83.json` and `evidence_badge83.json`
in the same directory. Enemy eligibility is observed helper output; candidate
order, RNG state and target choice are independently derived. Both observed
selections had one eligible enemy, so they do not cover empty or multi-enemy
selection. The badge run overrides one predicate return register at caller
`0x020C5D30`; it does not demonstrate ordinary badge-equipping coverage.

The compatible `build/runtime/eur_overlay15/item_select83.dst` starts with
Smash Eggs selected. Read the probe's input schedule before reuse: it adds
timed A/B and X/Y pulses during the native attack update. Four `down:8` /
`wait:30` pairs from the preceding item menu take 160 frames, including the
released-input frame after each action. A successful attack replay returning
to the command wheel does not establish that the item-reward branches ran.
The public pair-state layout is a prefix; if it grows during reconstruction,
update the probe's record size with it. The entry/launch extension in
`build/analysis/probe_ov15_entry.py` checks 36-byte pairs and the complete
584-byte entry context. Its reward fixture changes the initialized pair's
signed launch counter at `+0x17` once, after an enemy hit, then lets the native
end transition run; the counter is distinct from the phase bits at `+0x16`.
The final reward fixture also overrides one random-helper return to select the
reward branch. Consult `build/runtime/eur_overlay15/evidence_reward_final83.json`
for both interventions and the visible reward capture; do not describe that
run as ordinary reward probability or completion of six natural launches.
The ordinary entry replay is recorded separately in `evidence_entry83.json`.
The older actor/pair probes retain their original 28-byte prefix checks.
Use the current shared
[attack layout](../../include/game/overlay015_attack.h) and native allocation/caller
evidence before interpreting fields beyond that prefix.

### Credits

For credits, the private `build/analysis/probe_credits_transition.py` established
a route from checkpoint 86 and the compatible state
`build/runtime/states/eur_story_086_hud_verified.dst`. It substitutes one 72-byte
decoded field command with opcode `0x122`, fade argument 1, and restores every
original byte at the credits request helper before the scene transition.
Inspect its guards and the recorded fixture before reuse. Successful reports
are under `build/runtime/eur_credits_transition/`; the final illustration uses
the fade transition, so an early capture alone does not cover it. This route
verifies controlled credits entry, not ordinary completion of the story.

The subsequent private probes `build/analysis/probe_credits_effects.py` and
`build/analysis/probe_credits_followup.py` reuse this route for particle effects,
spiral motion, illustration loading and music. Their successful reports are
`build/runtime/eur_credits_effects/evidence_story86_full.json` and
`build/runtime/eur_credits_followup/evidence_story86_full.json`. They include
full particle/task records, independently derived RNG and motion results,
decoded image and palette checks, and graphics captures. Allocation addresses
and newly constructed child prefixes are explicitly observed helper outputs.
Read each report's uncovered branches before extending it; a successful music
state sequence does not independently verify audio mixing or physical output.

`build/analysis/probe_credits_small.py` extends that route to cloud movement,
star blinking, grid fade and radial delay. Its successful report is
`build/runtime/eur_credits_small/evidence_story86_full.json`. It derives complete
task/particle records, the shared workspace fields and RNG state without
refreshing expectations from helper outputs. Consult its branch observations:
positive-X radial launches and a zero RNG seed were not exercised. This is
evidence for those update routines, not independent verification of every
native graphics callback. Keep new probes equally explicit about their limits.

`build/analysis/probe_credits_gradient.py` adds an ordered GPU/workspace store
oracle and checks the full 52-byte workspace prefix around the native sprite
pass. Its report is `build/runtime/eur_credits_gradient/evidence_story86_full.json`.
It records both background variants, nested star/cloud call order and counter
effects; the sprite and texture rendering internals remain outside its oracle.
Before reusing captures as a baseline, verify their recorded sizes and hashes
and compare the shared graphics buffers with the preceding credits replay.

### Pause party status

These findings come from EUR overlay 7 instructions and current shared types.
The party bitmap pair and low-HP stop/update routines are linked and have runtime
verification; the low-HP creator remains private. Check the manifest and handoff for the
other candidates' status; do not count a matching private object as linked code.
Private evidence includes `build/analysis/pause_party_status.cpp` and the complete
instruction and literal-pool listing `build/analysis/pause_party_status_native.txt`.

The existing [pause workspace](../../src/scene_menu_ov007/pause_scene_internal.h)
is a 90,600-byte `PauseSceneWork` at ARM9 RAM `0x020905F0` while overlay 7 is
loaded. It is an object at that address, not a pointer stored there.
The party-availability bytes start at `+0x116`. The indicator control byte is
`hp_warning_mode`, at `+0x2DD` (`0x020908CD`), with the existing raw aliases
retained; reuse that field rather than
inventing a separate global for the same storage. This byte is distinct from
the 32-bit pause phase used by the Nawatobi entry procedure below.

The native callback at `0x02080D4C` gets its attached ResourceA model before
examining the control byte. Zero requests deferred task removal; two returns
without submitting the model. Other values take the HP-dependent path, then
submit the model with argument 60. The stop helper at `0x02080C2C` clears the
byte. The creator at `0x02080C40` sets it to one and creates an indicator for
each available party member. The separate low-HP replay below exercises all
three control cases through ordinary menu navigation.

For the callback's HP check, `gSaveData` is the pointer stored at `0x02059FE8`.
Member `i` is at `read32(0x02059FE8) + 0x3F8 + 0x24 * i`, using the existing
[SavePartyMember](../../include/game/save_data.h). Maximum HP is the unsigned
16-bit field at member `+0x0C`; current HP is at `+0x0E`; level is the byte
at `+0x18`. These are live RAM offsets, not offsets to edit blindly in a `.sav`.
The callback compares `100 * current_hp > 25 * max_hp` with signed 32-bit
products after unsigned halfword loads. The products fit in a signed 32-bit
integer. Preserve this multiplication and comparison rather than introducing
division, rounding or floating point.

Above the threshold, the callback stores -256 into both model animation offsets
at `+0x5C` and `+0x5E`. At or below it, the callback loads the task's 32-bit Y
and X fields at `+0x30` and `+0x2C`, respectively, before writing X and Y as
halfwords. The task's member index is at `+0x28`. The private task layout is
72 bytes; independently confirm the actual allocator and attached model's
complete 336-byte slot when adding a runtime oracle. In particular, check the
threshold equality case, suppression/control paths and eventual pool return.

The linked [party bitmap pair](../../src/scene_menu_ov007/pause_party_bitmap.cpp)
comprises `PausePartyBitmap_Rebuild` at `0x0207923C` (356 bytes) and
`PausePartyBitmap_DrawValue` at `0x020793A0` (396 bytes). Rebuild clears a
4,480-byte member slice through workspace `owned8c` at `+0x8C`, then draws
level/current/max HP labels and values. The formatter caps the value to its
requested decimal width and optionally suppresses leading zeroes.
[Pause resource initialization](../../src/scene_menu_ov007/pause_resources.cpp)
establishes the complete allocation as 17,920 bytes: four 112-by-40 member
slices. Its other bitmap, `owned88`, has a separate 49,152-byte allocation.
The clear wrapper's native stack halfword store/load explains its scoped
volatile zero. Both functions match without assembly or compiler flag changes.
Call instructions establish eight arguments to `0x0207952C`, six to
`0x020793A0`, and seven each to `0x02081334` and `0x02080DEC`. Pseudocode can
omit arguments, infer an extra reused stack value, or infer narrow parameters
from callee stores; retain the caller's actual full-width argument behavior.

`build/analysis/probe_pause_party_bitmap.py` verifies the linked pair through
normal equipment-menu navigation on checkpoints 65 and 86. Returning from
equipment comparison rebuilds all four HP/level slices. The starting snapshot
`build/runtime/states/pause_subscene_menu65.dst` selects Cobalt Star: press Down
twice, with released frames and animation waits, to reach equipment. The
checkpoint-86 HUD state uses Start to open the menu, then one Down selection.
Each route enters equipment, opens the comparison, switches the character with
Right, and backs out. The reports retain the precise input schedules.

The two passing reports under `build/runtime/eur_pause_party_bitmap/` contain
eight rebuilds and 24 value draws in 2,350 frames. The probe checks the complete
bitmap allocation and heap header, font allocation and header, 90,600-byte
workspace, 1,380-byte save, pointers and offset table at helper boundaries.
Nested calls share independently derived expectations. Decimal formatting is
checked against padded strings; each destination pixel is derived from tile
coordinates, source glyph bytes, palette offset and transparency. The native
glyph-copy helper remains unlinked, but its 76 calls have this pixel oracle.
All 144 signed-division calls are checked for quotient and remainder.

The HP label starts at X = -6 on later rows. Native code uses linear addresses
without horizontal clipping, and all these writes remain within the member
slice. An initial oracle incorrectly required nonnegative X; correcting it to
check each linear destination address produced a passing rerun. The earlier
Cobalt Star and item-list discovery routes made no calls and failed coverage;
they are not passing evidence. No game-code correction or RAM fixture was used.

The suite covers two/three-digit values, 52 drawn digits, 20 leading blanks,
zero-clearing and transparent preservation. It does not exercise zero HP, value
saturation, explicit leading zeroes or a non-three-digit width. The artifact
verifier checks 34 screenshots, 306 graphics dumps and all 104 unchanged source
saves. Both routes finish with no pending calls. Final GPU output and screenshots
are observed, not independently rendered by the oracle. These results do not
verify the low-HP callbacks; their lifecycle and threshold checks are recorded
separately below.

The linked [clock separator callback](../../src/scene_menu_ov007/pause_numbers.cpp),
`PauseClock_UpdateSeparator` at `0x02080EF4` (164 bytes), increments its signed
timer at task `+0x24`. At 30 it subtracts 30, toggles the word at `+0x28`, and
either draws tile 268 at (72, 161) or clears the 4-by-8 rectangle at (74, 165).
The existing overlay-5 element pool establishes the complete task slot as
72 bytes. The callback does not allocate or release the slot itself.

`build/analysis/probe_pause_clock.py` checks 235 callbacks during 406 frames of
normal waiting and B exit from the checkpoint-65 pause snapshot. It verifies
227 wait returns, four show transitions and four hide transitions, with exactly
30 callback updates between toggles. Expectations cover the full task and pool
record, 49,152-byte bitmap and heap header, font allocation/header, workspace,
save and pointer. Helper arguments, per-pixel transparent glyph copies, rectangle
clears and the background-dirty flag are independently checked. The focused
artifact verifier validates nine screenshots, 81 graphics dumps and all 104
unchanged source saves under `build/runtime/eur_pause_clock/`. No fixtures or
pending calls remain. Task creation/destruction, invalid timer states and final
GPU output are outside this callback oracle; the two graphics helpers remain
unlinked even though their effects are checked here.

### Pause party bitmap transitions

[pause_party_transition.cpp](../../src/scene_menu_ov007/pause_party_bitmap.cpp)
reconstructs the creator at `0x02078F5C` (100 bytes) and controller at
`0x02078FC0` (636 bytes). Both match without assembly or compiler changes.
The shared [task and mode definitions](../../include/game/pause_party_bitmap.h)
preserve the 72-byte task: phase at +32, equipment kind at +40, column at +44
and step at +48. Creator modes 0 and 1 select clothing and badges; mode 2
restores the ordinary party status. Its first argument is a scalar mode.

The controller prepares members 0/1 and 2/3 in separate updates, then copies
six-column strips across the 112-by-40 member bitmaps. Forward phases 0/1/2
end with a four-column strip at column 108. Reverse phases 100/101/102 start
at column 106; the last strip starts at zero and leaves the signed column at
-6. The last reverse strip overlaps the preceding strip by two columns.
Equipment preparation suspends HP warnings; completion of the reverse path
restores them and creates the ordinary numeric displays. Preserve helper-call
order and task-field readbacks; there is no fallthrough between these phases.

The creator marks group 9, allocates its controller on list 5, then calls the
already linked `MenuSpring_AdvanceActiveChains`. Its four 236-byte records start
at `0x020A67DC`, four bytes after the spring workspace base `0x020A67D8`.
Capture 948 bytes from that base, independently of the pause workspace, which
ends there. Each available member advances the signed state at the corresponding
chain's offset zero. Do not count the existing spring helper as new code.

Two ordinary routes use story checkpoints 65 (`clothing_arrows`, 2,380 frames)
and 86 (`badge_members`, 1,820 frames). All four creators and 84 updates pass
independent checks for group removal flags, task allocation, 16 spring-state
increments, all three modes and six phases, helper arguments, warning modes,
clamps and actual task returns. The strip-copy oracle independently derives
all 72,320 destination-byte writes across 76 calls and 12,160 member rows.
It checks both complete bitmap allocations (49,152 and 17,920 bytes), their
heap headers, the live task/game records and the separate spring workspace.
The unreconstructed text/status producers are observed at return, not claimed
as independently verified by this transition probe.

The first replay stopped at frame 334 because the probe incorrectly equated
832 bytes of text scratch use with the allocation size. The linked resource
initializer and live heap header establish 12,288 bytes; the corrected probe
checks that entire allocation. The failed report and original probe are kept
separately. No game-code change was needed. Both corrected replay processes
pass, with all four transition lifetimes complete; all 154 watched tasks and
42 model slots return, with no pending calls, live watched tasks or drain frames.
Final overlay-0 guards pass. All 104 original saves remain unchanged.

All 72 screenshots and 648 graphics dumps validate and equal the complete
ordinary-input baselines. The clothing bitmap display and final checkpoint-86
field were inspected. No RAM fixtures were used. Unavailable-member branches,
invalid modes/phases, producer internals and independent final rasterization
remain outside this coverage. Full matching checks, golden EUR ROM packaging,
zero-difference native relinking, generated progress and all 81 tests pass.
An initial Windows PowerShell packaging invocation failed to resolve its final
`Get-FileHash` command; invoking the same wrapper with the available PowerShell
runtime completed successfully and confirmed the golden hash.

Private reports are under `build/runtime/eur_pause_party_transition/`, with
`clothing_arrows/evidence_clothing65_v2.json`,
`badge_members/evidence_badges86.json` and `artifact_validation.json`.
They identify `build/analysis/probe_pause_party_transition.py` and its hash;
its generator composes the existing low-HP checks without overwriting that
probe. Build/replay logs use the `pause_party_transition_` prefix. The strip-copy
helper at `0x020796F4` remains private and nonmatching; its independent runtime
oracle does not add its native bytes to the C/C++ total.

### Pause low-HP warnings

[pause_hp_warning_stop.cpp](../../src/scene_menu_ov007/pause_hp_warning_stop.cpp) and
[pause_hp_warning_update.cpp](../../src/scene_menu_ov007/pause_numbers.cpp)
reconstruct the 20-byte stop routine at `0x02080C2C` and 160-byte updater at
`0x02080D4C`. The shared [task and mode definitions](../../include/game/pause_hp_warning.h)
preserve the complete 72-byte task and the existing workspace byte. The stop
routine takes no arguments; its caller overwrites the return register before
using it. The two source units remain separate around the unreconstructed
268-byte creator. All 180 bytes match without assembly or changed compiler flags.

Two `clothing_arrows` replays at story checkpoints 65 and 86 run 4,760 frames.
Every one of the 16,344 warning updates and both stop calls is independently
checked: 4,848 model draw insertions, 11,488 suspended returns and eight removal
marks. The checks cover complete live task/model/game records, initial member
and coordinate fields, unsigned HP reads, signed products, threshold comparisons,
pixel stores and actual resource/task pool returns. All eight warning tasks
finish their lifetimes. Across the retained surrounding checks, all 248 watched
tasks and 62 model slots are returned. Both routes finish in the field with full
overlay-0 guards, no pending calls and no live watched tasks.

Checkpoint 65 uses ordinary inputs and no RAM fixture. At checkpoint 86, the
probe temporarily sets maximum HP to 100 and current HP to 0, 24, 25 and 26 for
each member, one guarded callback at a time. All 16 four-byte edits at member
offset `+0x0C` are restored and verified before that callback returns, during
frames 85 through 88. This establishes zero, below-threshold, equality and
above-threshold behavior independently of naturally occurring save values.
The ordinary route only exercises HP above the threshold. The fixtures do not
establish a story encounter or independently verify the rendered warning icon.

All 82 screenshots and 738 graphics dumps validate, and all 104 original saves
remain unchanged. The complete ordinary route's 41 images and 369 dumps equal
the earlier member-arrow replay. For the fixture route, only its two images
and 18 dumps before the first RAM edit are compared with the earlier checkpoint-
86 route. The ordinary pause screen and fixture route's final field were inspected.
Model initialization/animation internals and rasterization are observed rather
than independently derived. Full matching checks, golden EUR ROM packaging,
zero-difference native relinking, generated progress and all 81 tests pass.

Private reports are under `build/runtime/eur_pause_hp_warning/`, with
`clothing_arrows/evidence_normal65.json`, `evidence_thresholds86.json` and
`artifact_validation.json`. Reports identify the composed
`build/analysis/probe_pause_hp_warning.py` and its hash. Both replay processes
completed successfully. An inherited artifact assertion initially expected
equipped markers to be drawn on both saves; checkpoint 86's equipped items are
outside its first nine rows, so all 5,496 marker updates correctly omit drawing.
The corrected validator retains the independent lookup/visibility oracle and
count reconciliation. The failed artifact log is preserved separately.

### Pause numeric displays

The linked [number module](../../src/scene_menu_ov007/pause_numbers.cpp) contains
`PauseNumber_Create` at `0x02080DEC` (264 bytes), the existing 164-byte clock
callback, and `PauseNumber_Update` at `0x02080F98` (924 bytes). Consolidation
adds 1,188 matching bytes; the clock is not counted again. The
[72-byte task layout](../../include/game/pause_numbers.h) packs the member index
and leading-zero flag into one byte. Its six kinds select level, current HP,
maximum HP, coins, hours and minutes. Creation preserves payload fields that
the native factory and constructor leave untouched. All seven creator arguments
retain the callers' full-width behavior before the native narrowing stores.

The updater uses Q8 interpolation, signed division, states 0/10/11 and a cached
integer value. It caches before limiting the rendered value to its digit width.
Party/coin interpolation divides the difference by eight; time updates divide
by one. The save pointer is at `0x02059FE8`, coins at save `+0x488` and the 60 Hz
play-time counter at `+0x518`. Hours divide frames by 216,000; minutes use the
within-hour count and cap at 59 from 21,596,400 frames onward. These time
calculations and cap comparisons use unsigned values, unlike digit division.
The source save view is a 1,308-byte prefix, not the full 1,380-byte allocation.
Separate identical palette switch arms and native constant-initialization order
explain the final instruction layout; no assembly or compiler flag change is used.

Private `build/analysis/probe_pause_numbers.py` starts from the checkpoint-65
HUD state, opens the pause menu with Start, waits and exits with Start. Two
421-frame reports under `build/runtime/eur_pause_numbers/` verify 30 creations
and 6,240 updates in total. Each route reaches all six kinds, all four party
members, widths two/three/six, leading-zero drawing/suppression and cached
no-redraw returns. The oracle derives factory slots, list links and counters,
full task contents, interpolation/cache changes and decimal glyph selections.
It checks 270 signed quotient/remainder calls, 92 pixel-derived glyph copies,
44 rectangle clears, complete bitmap/font allocations and heap headers,
the workspace and save. All 30 tasks remain live until whole-pool shutdown;
the replay checks live payloads before shutdown and cleared pool pointers
afterward. It does not claim the individual removal path was exercised.

`evidence_ordinary65.json` uses no fixture. `evidence_hp_fixture65.json` changes
member-zero current HP from 77 to 70 once at a guarded idle update, then restores
77 after 12 callbacks. The two native interpolation sequences each take eight
updates. This controlled RAM edit is separate from ordinary gameplay and is
recorded with its pointer, halfword address, guard and restoration. Subsequent
screenshots and graphics dumps match the ordinary route. Both runs finish with
no pending calls or drain frames. The artifact verifier checks 12 screenshots,
108 graphics dumps and all 104 unchanged source saves. Time/display saturation,
unsupported kinds and other widths remain unexercised; final GPU output is
observed. The clock's previous callback oracle was not rerun for this batch.

### Pause blend background

`SceneMenu_PrepareBlendBackground` at `0x02081560` (260 bytes) is linked in
[scene_menu_background.cpp](../../src/scene_menu_ov007/scene_menu_background.cpp).
It configures main BG0, optionally clears its scroll and sets the existing
window manager's `display.bits.fixed_main_scroll`, then masks and adds a tile
bias to the visible 32-by-24 map. The flag belongs to the complete 3,908-byte
`GameWindowManager` pointed to by the scene manager's first word; it is not an
unidentified scene-controller field. The scene manager allocation is 43,056
bytes. Its initialization and heap headers confirm these separate extents.

Both calls in `SceneMenu_StartBlend` pass character bank 8 or 12 and explicitly
narrow the tile bias to a halfword. The shared setup declaration now uses
`u16` for the bank and bias, matching the register setter's parameter type and
the native setup body without redundant narrowing. Full module checks also
verify both callers. The final source uses existing window/display types and
matches without assembly or compiler flag changes.

Private `build/analysis/probe_scene_menu_background.py` follows ordinary
equipment/comparison/back/exit routes from checkpoints 65 and 86. Passing
reports `evidence_equipment65_verified.json` and `evidence_equipment86.json`
under `build/runtime/eur_scene_menu_background/` cover four setup calls in
2,060 frames. They independently check the fixed-scroll flag, BG field masks,
60 ordered GPU stores, derived VRAM map pointers, all 3,072 ordered tile writes
and full main/sub mapped BG VRAM. Complete window/scene allocations and heap
headers, workspace, save, palettes, OAM and stable GPU fields are checked.
Both bank/bias pairs (12/4096 and 8/0) run on each save. Thirty screenshots,
270 graphics dumps and all 104 unchanged source saves validate; no fixture,
pending call or drain frame remains. Reset-disabled calls and other parameters
remain unexercised; final rendered output is observed.

The first checkpoint-65 probe stopped at frame 143 because its store collector
only recognized unconditional `strh`, omitting the native mosaic helper's
`strheq` and `strhne`. The corrected collector uses instruction IDs and checks
ARM condition flags before counting a write. Both final routes pass after
that oracle correction; no game-code change was required.

### Pause sprite positions and lifetime

The linked [sprite callbacks](../../src/scene_menu_ov007/pause_mode_sprite.cpp) are
`PauseModeSprite_Update` at `0x0207E93C` (112 bytes) and
`PauseModeSprite_UpdateTimed` at `0x0207E9AC` (160 bytes). Their
[task layout](../../include/game/pause_mode_sprite.h) is a 72-byte pool slot.
Both obtain their ResourceA sprite before comparing task mode (`s16`, `+0x28`)
with the signed byte at `PauseSceneWork +0x110`. A mismatch requests removal.
Otherwise they convert both signed Q12 coordinates to pixels with division
toward zero, store the two sprite halfwords at `+0x5C/+0x5E`, and submit to
draw list 5. The timed version first decrements a nonzero 32-bit counter at
task `+0x24`; reaching zero requests removal without drawing. An initial zero
leaves the timer inactive. Preparing both coordinate divisions before the
assignments reproduces the native interleaved sign corrections without assembly
or compiler changes. ResourceA's concrete sprite slot is 336 bytes; the common
`BattleModel` interface does not establish a larger allocation.

Private `build/analysis/probe_pause_mode_sprite.py` follows clothing selection
from checkpoint 65 and badge selection from checkpoint 86. Enter Equipment,
select Clothing or Badges, choose a character, confirm an item, then back out
and close the pause menu. The reports `evidence_clothing65.json` and
`evidence_badges86.json` under `build/runtime/eur_pause_mode_sprite/` record
3,360 frames, 960 ordinary and 720 timed callbacks, 1,668 submissions, six timer
expirations and six removals caused by a mode change. Each timed sprite starts
at 120 and receives exactly 120 updates. All 12 tracked tasks are actually
unlinked, release their resources and return to their pools. No fixture,
unfinished call, remaining task or drain frame is present.

The independent oracle checks full tasks, sprite slots, workspace and save,
signed arithmetic, helper arguments, and draw-pool selection before submission.
It derives the free/taken link changes, node, list header and old tail writes,
including overlapping sentinel views. It also checks task unlinking, cleared
resource/release fields, counters and both pool returns without reading freed
slots. Tracking begins at the first callback; creator bodies, virtual sprite
cleanup and final graphics are observed rather than independently verified.
The artifact verifier validates 44 screenshots, 396 graphics dumps and all
104 unchanged source saves. Negative coordinates, an initial zero timer, and
the timed callback's mode-mismatch path remain unexercised. Both ordinary
routes use the 120-update timer; badge selection does not add zero-timer coverage.

The earlier `equipment65` and `items65` discovery routes completed navigation
but failed the final coverage assertion: neither reached these callbacks.
Their failure reports and logs are retained. The deeper clothing/badge routes
establish execution coverage; no oracle or game-code correction was needed.

### Pause list-row sprites

The linked [row callbacks](../../src/scene_menu_ov007/pause_list.cpp) cover
`PauseListRow_UpdateDigitSprite` at `0x02073FA0` (252 bytes),
`PauseListRow_UpdateMarkerSprite` at `0x0207409C` (176 bytes), and
`PauseListRow_UpdateTextSprite` at `0x0207414C` (152 bytes). The parent and child
[tasks](../../include/game/pause_list_row.h) occupy 72-byte slots; their attached
ResourceB sprite occupies 64 bytes. Text uses four segments and copies the
parent's position, tile selection and palette bank. The marker uses parent
tile +16. Quantity digits preserve two signed divisions and suppress a leading
zero except in the units position. Clothing/badge eligibility retains the
native repeated kind queries and short-circuit order. The shared sprite union
preserves raw attribute access while naming tile, priority and palette bits.

Private `build/analysis/probe_pause_list_row.py` uses ordinary clothing selection
on checkpoint 65, badge selection on 86, and consumable items on 65. The three
reports under `build/runtime/eur_pause_list_row/` cover 4,080 frames and observe
80,343 target calls. They independently check 1,770 complete callback samples:
1,132 text, 283 marker and 355 digit calls. The deterministic sample includes
the first two calls per object lifetime/selector/row-input configuration and
all calls on frames divisible by 60. Branch counts describe checked samples:
all four text segments, list kinds 0/2/3, divisors 1/10, 315 drawn digits and
40 leading-zero suppressions. The oracle checks 710 signed divisions and
1,730 draw-list submissions.

Checks include full parent/child tasks, sprite slots, the actual 4,428-byte
party allocation and heap header, 90,600-byte workspace, 1,380-byte save,
display resources, helper arguments/results and unchanged padding. Expected
digits, packed attribute writes and draw-pool/list changes are derived
independently. Lifetime tracking runs independently of body sampling: all
171 tracked tasks release their ResourceB sprites, unlink and return to their
task pools. Tracking starts at the first callback, not allocation. No fixture,
pending call, live tracked task or drain frame remains.

The artifact audit validates 57 screenshots, 513 graphics dumps and 104
unchanged source saves. The 44 clothing/badge screenshots and 396 corresponding
graphics dumps equal the preceding sprite-lifetime probe at identical input
and capture boundaries. Visual inspection confirms readable lists and quantities;
these captures are observed output, not an independent pixel oracle. Unsampled
bodies, row creation/parent updates, final GPU rendering, unsupported divisors
or segments, and unobserved kinds/empty-item eligibility remain outside coverage.

### Pause list visibility and row measurement

The linked [list controls](../../src/scene_menu_ov007/pause_list.cpp) cover
`PauseList_MeasureRowWidth` at `0x020742E0` (148 bytes), `PauseList_Hide` at
`0x02074374` (132 bytes), and `PauseList_Show` at `0x020743F8` (308 bytes).
Show allocates 340 OBJ tile units, prepares the current list, creates up to
nine visible rows and adds category-specific tasks. Hide remembers the first
row and selection by list kind, marks task group 2 for removal, frees the
Key Items image buffer when present, and unlinks the sprite allocation.
Measurement resolves the selected entry's name, clears both packed cursor
coordinates and forwards the text helper's width. Empty clothing/badge entries
use the native fallback-name path. That path matches statically but was not
exercised by these replays.

The shared [party prefix](../../include/game/overlay007_party.h) retains its
332-byte layout and existing byte views while naming visibility, five saved
first-row/selection pairs and the owned image pointer at +280. The allocation
is still 4,428 bytes. Raw workspace declarations remain with the existing raw
consumers so the new module can use the actual `PauseSceneWork` declaration.
Native branch conditions require an unsigned `count < 9` and a signed
`first + row >= count` within the same loop. Assignment order explains the
fallback-name conditional instructions; no compiler changes or assembly are used.

Private `build/analysis/probe_pause_list_control.py` checks every target call
on clothing65, badges86, items65 and key_items65 routes. The reports in
`build/runtime/eur_pause_list_control/` record 4,860 frames, five show calls,
five hide calls and two width calls. Kinds 0/1/2/3 are consumables, Key Items,
clothing and badges in these observed menus. From an open consumable list,
Right switches to Key Items; its two rows show Beans and Toadbert's Drawing.
The route checks the short-list stop and a 6,144-byte image buffer whose
contents match between load and the last live snapshot before deletion.
The other lists exercise nine-row creation; 38 row-creation calls and 294
task-removal flag writes are checked across the suite.

The oracle checks full party/work/save/display records and pointer/heap
headers at helper boundaries, native direct writes, getter results, item
selection, argument order including stack parameters, width forwarding,
group-2 flags, and allocation unlinking with neighbor/head/tail changes.
Preparation, allocation and category-helper mutations are explicitly observed
helper output. Text/heap/IO internals, row constructors and final rendering
are outside the independent oracle. Marking tasks is not proof of their later
removal. Kind 4, empty lists, null image buffers and plural/fallback width
branches remain unexercised. No fixture, pending call or drain frame remains.

All 70 screenshots, 630 graphics dumps and 104 unchanged saves validate.
The three unchanged routes equal the previous row probe's 57 screenshots and
513 dumps; visual inspection identifies the Key Items menu. The first clothing
attempt rejected an uninitialized kind before the preparation helper ran;
moving that guard to actual kind consumers corrected the oracle. The first
items attempt failed only its final assertion demanding a width call on every
route; width coverage belongs to the clothing/badge routes. Corrected reruns
pass. Earlier `bros65` and `bros_right65` discovery tags are retained but
excluded from these totals: the former stayed on consumables, and the latter's
second tab was visually identified as Key Items before the final named rerun.

### Pause list selection and row copies

The linked [selection helpers](../../src/scene_menu_ov007/pause_list.cpp)
cover `PauseList_GetTileRow` at `0x0207452C` (12 bytes),
`PauseList_DrawSelectedLabel` at `0x02074538` (160),
`PauseList_RedrawSelectedRow` at `0x020745D8` (220),
`PauseList_CopySelectedRow` at `0x020746B4` (140), and
`PauseList_CheckRowAvailability` at `0x02074740` (160). The party prefix's
byte +266 has a signed tile-row view alongside its existing raw byte view.
Selected rows use `(tile_row + selected) % 9`; item lookup uses the separate
`(first + selected) % count`. Preserve those two rotations and the signed
division. Each copied row is 1,088 bytes, transferred from main OBJ memory to
sub OBJ memory at +640. The dedicated label uses main allocation offset +9,792
and returns its measured width. The availability wrapper handles disabled
categories and zero quantity before forwarding the item/member predicate result.

The private `build/analysis/probe_pause_list_selection.py` is generated by
`make_pause_list_selection_probe.py` from the control probe. Edit the generator
as well as the generated file when changing that probe. Reports under
`build/runtime/eur_pause_list_selection/` cover 6,310 frames on five routes.
The four ordinary clothing/badge/consumable/Key Items routes observe tile-row
and availability calls, but only clothing/badge selection reaches the label
helper. The additional `consume65` route reaches the standalone copy and two
redraws as Mushroom quantity decreases to one and zero. Native relocation
`0x0206CF44` identifies the redraw call after inventory decrement; the old
controller pseudocode omits that branch, so use the native instructions.

Consumption uses a controlled fixture: at fully guarded EUR ARM9 menu callback
`0x0206C6A0`, phase 100, with the consumable list initialized, four current-HP
halfwords at `read32(0x02059FE8) + 0x406 + 0x24 * member` become one once.
The recorded original halfwords are restored at frame 1,450 before emulator
destruction. The original battery saves are never edited. Ordinary A presses
then exercise item use; this is fixture-assisted coverage, not an unmodified
story-save route. Captures show one remaining Mushroom followed by its removal
from the list when exhausted.

Across the suite, 49,335 target entries are observed and 988 complete calls
are independently checked: 434 tile-row, 549 availability, two label, one copy
and two redraw calls. All label/copy/redraw calls are checked. Frequent getters
and availability calls use the first two occurrences per party/arguments/list
and row state, plus calls on every sixtieth frame. Each route uses one pause
allocation. Verified samples cover kinds 0/1/2/3, 318 available results, 221
rejected results and ten disabled-category returns. Zero-quantity early returns,
kind 4, redraw without copying and other unobserved branches remain uncovered.

Checks include full party/header/work/save/display records, four signed DIV
results, item and tile selection, all helper arguments, result forwarding,
and five 1,088-byte copies. Copy/draw roots check both complete 65,536-byte OBJ
buffers, including unaffected bytes. Drawing/width helper mutations and the
quantity/availability predicate results are observed helper output; their
internals and final rendering are outside this oracle. The first clothing
attempt incorrectly expected the width helper to preserve the party text state.
The corrected probe records that known helper mutation separately and passes
without changing matching game code. Its failure log/report are retained.

All 91 screenshots, 819 graphics dumps and 104 unchanged source saves validate.
The four ordinary routes equal the prior control probe's 70 screenshots and
630 dumps. No pending call or drain frame remains; the fixture is restored.

### Pause item lookup and rotating inventory order

`src/scene_menu_ov007/pause_item_lookup.cpp` owns four contiguous functions at
`0x02074D48..0x020750BC` (884 bytes): description ID, name ID, rotated
availability and rotating-order rebuilding. The lookup record strides are 20
bytes for consumables/badges and 28 for clothing/Bros. Items. Names use halfword
+2; descriptions use +4 for equipment and +6 for consumables/Bros. Items.
Key Items names use the consumable table, but their description ID is the
masked item index directly. Unsupported kinds return zero in the native code.
The full-width result accumulator and explicit halfword truncations matter;
the old pseudocode's apparent pointer-derived default return is incorrect.

Shared records retain their raw byte views and earlier interface-resource names
through aliases. The badge record type is shared, while its typed table extern
remains local: existing battle/shop users still declare the symbol as bytes.
Moving that extern to the shared header failed the full build; the corrected
local declaration passes every affected caller. Separate switch arms, the
native equipment-range check and post-increment array stores reproduce the
complete instructions and literal pools without assembly or flag changes.

The rebuilt order includes only entries with available quantity. Equipment
starts at item 1 and appends the empty-gear entry 0 at the end. Its returned
count is byte-sized, and the unused array tail stays unchanged. Rotated lookup
uses the signed saved-first byte for that category, performs signed division
before the category-enabled check, and forwards the availability predicate's
full integer result. These fields differ from the visible tile-row rotation.

The focused private probe and generator are `probe_pause_item_lookup.py` and
`make_pause_item_lookup_probe.py`; reports are in
`build/runtime/eur_pause_item_lookup/`. Four ordinary clothing65, badges86,
items65 and key_items65 routes check every one of 175 calls over 4,860 frames:
106 names, 6 descriptions, 7 order rebuilds and 56 rotated queries. Every
function runs for kinds 0/1/2/3. Checks include 56 signed divisions, 7 one-byte
stack outputs with neighboring bytes preserved, 100 retained/32 excluded
inventory entries, 2 empty-gear append operations and 2 disabled-category
returns. Full live party allocation/header, workspace, save, display and item
records are checked at the relevant boundaries. Quantity calculations use
save/equipment/displayed flags independently; the final availability predicate
has 15 positive and 39 negative observed returns, with its internals uncovered.

The first clothing replay stopped at frame 1347 because the hook at
`0x02074F20` survived the return to field. That address is then inside overlay
0's function at `0x02074F18`. The corrected probe excludes such entries only
when its entire native owner function matches and no checked call is pending;
all 8 foreign-overlay hits satisfy that guard. Arbitrary byte mismatches still
fail. The original log/report remain under the `_overlay_reuse` suffix, and all
four corrected routes pass. No matching game code changed for this correction.

Artifact validation passes for 70 screenshots, 630 graphics dumps and all 104
unchanged source saves. Every capture matches the prior ordinary list-control
route at the same input/state/time. Clothing and Key Items images were visually
inspected. No RAM fixture, pending call or drain is present. Kind 4, unsupported
kinds, zero-quantity rotated early return, count wrap and other unobserved
branches remain uncovered; equal native-renderer captures do not independently
verify its rendering algorithm.

### Pause item text and inventory tables

`src/scene_menu_ov007/pause_item_text.cpp` owns `0x02075B04..0x02075E10`
(780 bytes): `PauseItem_DrawText`, `PauseItem_GetText` and
`PauseItem_GetValues`. Drawing selects name, description or built-in menu text,
including the empty-clothing/badge fallback labels, then forwards all eight
renderer arguments. Text lookup handles singular/plural names, Beans and the
other Key Items, description resource tables and localized menu-table offsets.
The values helper returns the live category array and optionally writes one
byte of capacity. Its Key Items capacity source is a halfword at `0x020A6B98`;
the output truncates only at the byte store. The native fallback returns null
but does not define a capacity for unsupported categories.

Full-width temporaries followed by explicit narrowing reproduce name-index
increments and the capacity store. The draw fallback uses the same mode-first
initialization order as the already reconstructed row-width helper. Shared
prototypes replace earlier local declarations. `PauseItem_GetText` retains a
full-width plural argument because existing native callers pass a word; the
callee reads its low stack byte through a character view. Changing the shared
parameter to a byte would add narrowing instructions to those callers. The
draw wrapper's byte parameter already matches its callers. Every affected
caller passes the full matching build; no compiler flag or assembly change.

Private `make_pause_item_text_probe.py` generates `probe_pause_item_text.py`.
Reports live in `build/runtime/eur_pause_item_text/`. The four ordinary routes
are joined by `empty_clothing/` and `bros_items/` supplemental routes, both
using normal inputs from checkpoint 65. From an initialized main pause menu,
three separate Down presses followed by A open Bros. Items (menu index 3,
category 4). Its capture shows the Green Shell tutorial and attack-item list.
From the first clothing-list row, Up wraps to the final Unequip entry; both
its name and description fallbacks run. These routes use no RAM fixture.

Across 6,880 frames the probe observes 37,287 target entries and checks 1,107:
all 229 text lookups and 74 draw wrappers, plus 804 inventory getters out of
36,984. Standalone getters are sampled on the first two calls for each
party/category/output/save/count tuple and every sixtieth frame; getters
nested inside checked text calls are all checked. Each function runs for all
five categories. Checks cover 16 capacity-byte writes, 788 null-output getters,
94 plural and 36 singular name selections, two plural Beans names, two Key
Item remaps, one empty-gear name and two empty-gear descriptions. Resource
results are derived independently from the save's table pointers, language
offsets and entry offsets. Complete party/header, workspace/save/display and
the relevant pointer records remain checked; drawing also checks both entire
OBJ buffers, with the renderer's party/main-OBJ mutations labeled observed.
The renderer's pixel algorithm is outside this probe's independent oracle.

The initial clothing replay failed at frame 341 on output pointer
`0x027E3A84`. This is inside the EUR ARM9 DTCM range
`0x027E0000..0x027E4000`, not a main-RAM allocation. An attempted main-RAM
mirror interpretation was rejected by unequal reads. The corrected probe
retains the DTCM address, verifies that it lies in the caller's stack extent,
and checks all four output/neighbor bytes there. Nine DTCM snapshots cover
three stack addresses. Both initial failures remain under `_stack_mirror`
and `_mirror_assumption` suffixes; all six corrected replays pass. Do not
normalize CPU-visible TCM addresses into ordinary main RAM.

All 104 screenshots, 936 graphics dumps and 104 unchanged source-save hashes
validate. The four unchanged input routes equal the preceding 70 images and
630 dumps; supplemental routes have separate directories. The Bros. Items
and Unequip captures were visually inspected. There are no fixtures, pending
calls or drain frames. All 252 foreign-overlay hits were excluded only after
exact full owner-function guards. Unsampled getter bodies, singular Beans,
other unobserved fallback cases and invalid-mode behavior remain uncovered.
Mode 2 also ran with an otherwise unused category value of 96; do not require
an inventory category when that mode only reads the menu-text table.

### Pause party initialization and cleanup

`src/scene_menu_ov007/pause_party_lifecycle.cpp` owns the contiguous 152 bytes at
`0x020762DC..0x02076374`. Both functions matched their first compiled draft.
`PauseParty_Init` clears the image pointer and initializes the embedded
48-byte `GameText` using the caller's font table and the scratch buffer at
party +332. Its fourteen arguments set x/y 0, spacing 1, leading 3, color 1,
space width 6, tile pitch 31, unused 0, alignment sentinel 255 and margin 0.
`PauseParty_Destroy` deletes a nonnull image array, clears that pointer and
returns the party address. It does not free the party allocation itself;
the surrounding resource code owns that later operation.

Private `make_pause_party_lifecycle_probe.py` generates the focused probe;
reports and the artifact verifier are under the corresponding private
analysis/runtime names. Four routes check six initializers and six cleanup
calls over 5,240 frames. Each checks the complete 4,428-byte live allocation,
its 16-byte header, workspace, save, display and globals. The text oracle
independently derives all initialized fields and reset masks, including
language, fonts, scales, cursor/style/bounds and retained reserved bits.
Uninitialized payload, text padding and unused scratch bytes stay unchanged.
The constructor runs before the global party pointer is assigned; check its
argument allocation, not an assumed nonnull global. An incoming image-pointer
field contained allocation residue and was correctly cleared without freeing it.

The first artifact verifier assumed one initialization/cleanup pair per route.
The successful clothing/badge replays instead showed the final Start reopening
the menu, reusing the same heap address for a second object. The corrected
routes add another Start and wait to close that menu as well. Each now checks
two complete initialization/cleanup pairs; item and Key Item routes check one.
The initial reports/logs remain under `_reopened_menu`, with the original
artifact-verifier failure retained. This was a route-coverage correction, not
a game-code or per-call oracle failure.

All 74 screenshots, 666 graphics dumps and 104 unchanged source saves validate.
The common input prefixes equal 70 prior screenshots and 630 dumps; the four
additional captures cover the final exits. The final clothing capture shows
the field. No fixture, pending call or drain is present, and 3,164 foreign-overlay
hits have exact owner guards. All six cleanup calls saw a null image pointer;
nonnull image cleanup and the surrounding heap allocation/free internals are
not claimed as covered by this probe.

### Pause queued row drawing and markers

`PauseList_QueueRowDraw`, `PauseList_DrawRowTask` and `PauseItem_CopyMarker`
own `0x020757C4..0x020758F0` in `pause_item_row_draw.cpp` (300 bytes).
`PauseItem_GetMarkerId` extends `party_values.c` through `0x02075400`
with 160 new bytes; its existing neighbors are not new coverage. The creator
allocates a 72-byte element in list 11 before marker 1, then stores the row
and item as words at offsets 40 and 44. The callback draws four text segments,
copies the marker at row offset +1024, and sets the deferred-removal flag.
It reloads the global party pointer between helpers, as the native code does.

Marker selection reads byte 9 of the existing typed item records: 20-byte
consumable/badge records and 28-byte clothing/Bros. Item records. Key Items
return -1. Empty clothing selects 11 for member values 0/1 and 12 otherwise
using an unsigned shift. The copy helper skips negative IDs; otherwise it
copies 64 bytes from the archive-1/entry-69 buffer at workspace offset 192
to main OBJ memory. The sampled live allocation is 1,280 bytes. The term
marker describes its row-graphics role; individual glyph meanings remain
unassigned. No compiler flags or assembly fragments were changed.

Private `make_pause_item_row_draw_probe.py` composes the focused probe from
`pause_item_row_draw_probe_body.py`. Reports and their artifact verifier are
under `build/runtime/eur_pause_item_row_draw/` and `build/analysis/`.
Seven routes cover 9,270 frames: clothing, badges, consumables, Key Items,
empty clothing, Bros. Items and a separate clothing-scrolling route.
All 105 ID selections, 87 copy calls, six creators and six callbacks are
checked, plus all six task removals. ID/copy coverage includes all five
categories, two skipped negative IDs and two empty-clothing selections for
member 0. The other empty-clothing member cases remain uncovered.

The first clothing route reached only ID selection and copying, so its
all-functions coverage assertion failed after the replay. Its `_no_scroll`
log/report remain private. Native callers establish that lists longer than
nine rows queue a task only when scrolling across a visible edge. The added
route opens clothing, presses Down eleven times and Up twelve times, then
exits with three B presses; each direction press is followed by a 30-frame
wait. It checks three forward and three backward queued updates. The final
field capture was inspected. All seven final routes pass.

The oracle derives factory free-list selection and complete slot/list/link/
counter writes before allocation. It checks row arguments, the removal flag,
unlinking and pool return as separate events, with no freed-slot reads.
All 85 transfers independently check the selected 64 atlas bytes, both full
OBJ buffers and the untouched atlas allocation. Full party/header, workspace,
save, display and globals are also checked. The text renderer's arguments and
resulting party/main-OBJ changes are observed; its pixel algorithm and atlas
decompression are outside this oracle. Queued callbacks ran for clothing only.

All 169 screenshots, 1,521 graphics dumps and 104 unchanged source saves
validate. Matching input prefixes equal 113 prior images and 1,017 dumps;
the scrolling route has its own directory. There are no fixtures, pending
calls, live tracked tasks or drain frames. Full native guards remain active;
these routes produced no foreign-overlay hits. Invalid inputs and unobserved
marker IDs remain uncovered. Both ROM build paths retain the original SHA-1,
native relinking reports zero differing bytes, and all 81 tests pass.

### Pause inventory-list preparation

`PauseList_Prepare` at `0x02075F6C..0x0207616C` contributes 512 bytes in
`pause_list_prepare.cpp`. It selects the category from the main menu and its
submenu, captures the selected member, gets the live inventory pointer and
byte-sized limit, restores saved positions, and clears/rebuilds the slot list.
Only items with a nonzero available quantity enter the list. Clothing and
badges append the empty-equipment entry last. The function then constrains
the saved window/selection to the resulting count and requests a full redraw.

`Overlay7Party` now names bytes 2/3 as `item_limit` and `member`, preserving
the old raw view and the 332-byte prefix layout. The submenu array is the
existing `gSceneMenuSubmenus` alias at `0x02090717`, within the pause workspace
at +0x127. Resolve literal-pool references from their words and relocation
metadata; their appearance in an instruction listing is not a decoded address.
The initial matching-size draft differed around initialization order. Native
code evaluates the equipment start before resetting the count; the already
matching rotating-order helper uses that order too. Moving the count reset
after the category test produced the complete match, with no assembly or
compiler-flag changes.

Private `make_pause_list_prepare_probe.py` composes the focused probe from
`pause_list_prepare_probe_body.py`. Eight final routes under
`build/runtime/eur_pause_list_prepare/` check all ten observed preparations
over 11,660 frames. All five categories run. The oracle checks all 249
quantity results independently from inventory bytes and equipped-member
records, including signed truncation, unavailable-equipment subtraction and
the empty-gear quantity. It verifies 149 kept items, 94 excluded items, six
empty-gear appends, seven retained windows, two short-list clamps and one
full-window clamp. All party fields, padding and unused slot bytes remain
checked across the helper calls, together with the complete allocation/header,
workspace, save, display, globals and both OBJ buffers.

The original seven routes did not cover a restored full window, which the
artifact audit detected. Exiting pause entirely and reopening it also missed
that branch: the new party instance resets its saved positions. Both failed
coverage checks remain in the private `_full_window_assumption` and
`_field_exit` logs/reports. The final supplemental route scrolls to Unequip,
closes only the clothing list with B, waits, then reopens it with A in the
same pause instance. The second preparation restores first item 9 and
selection 8; its visible Unequip row and final return to the field were
inspected. All eight final replays and the artifact verifier pass.

All 234 screenshots, 2,106 graphics dumps and 104 unchanged source saves
validate; common input prefixes equal 226 prior images and 2,034 dumps.
There are no fixtures, pending calls or drain frames. The 15,828 hits after
overlay replacement were excluded only after matching the complete overlay-0
owner at `0x02075D28`. Full redraw arguments and its resulting party/main-OBJ
changes are observed, not an independent renderer check. Selection reduction,
invalid submenu/default cases and empty inventory remain uncovered. Both ROM
build paths reproduce the original hash, native relinking reports zero
differing bytes, and all 81 tests pass.

### Pause list movement

`PauseList_MoveSelection` at `0x020754D0..0x0207567C` adds 428 bytes to
`party_navigation.c`, alongside its four already linked neighbors (208 bytes).
It updates the signed selection byte, clamps or wraps short lists, and scrolls
a nine-row window for longer lists. Cursor sound 231 depends on the absolute
index changing; description drawing depends on the selected item changing.
The getter uses the unsigned view of the selection byte, while the index
arithmetic uses its signed view. Keep those access-site distinctions.

Two ternary assignments in the first draft produced one unconditional byte
store each instead of the native pairs of conditional stores. Explicit if/else
assignments recovered the missing eight bytes and matched the complete function.
Consolidation into the existing C module preserved assignment order while moving
local declarations to block starts. All five functions remain exact; no assembly
or compiler-flag changes were needed.

The private movement probe checks five routes from story checkpoints 65 and 86:
clothing and badge scrolling, consumables, Key Items and Bros. Items. Across
8,822 frames it observes 4,717 entries and fully checks 728 calls. Every nonzero
movement call is checked. Idle calls are sampled deterministically: the first
two per observed constructor lifetime, arguments, list and selection tuple,
plus every sixtieth frame. Branch counts describe checked calls. The callers
invoke movement every frame, including direction zero; held input drives repeat
movement while fresh presses enable wrapping.

All five item categories run, with short/long lists, wrapping and held-input
clamps in both directions. The oracle verifies 1,456 getter results, 1,532 signed
divisions, 90 sound/description calls and 24 scrolls (20 queued-row scrolls and
four full refreshes). It independently models control-byte changes, selection
and helper arguments while checking the complete 4,428-byte party and header,
90,600-byte workspace, save, display, globals and both entire OBJ buffers.
Wrapped redraw and description-renderer output are observed, not independently
derived pixels. Constructor entry identifies lifetimes; this probe does not
verify constructor bodies, audio internals or queued-task allocation/removal.

The original consumables replay failed at frame 1,348 because its foreign-owner
fallback assumed overlay 0. The corrected replay identifies the complete
336-byte overlay-8 owner at `0x02075390` by exact native bytes. Its final capture
shows the save menu after the route's extra field inputs. Key Item and Bros.
Items routes also encounter that owner. The old failure log/report remain
preserved; all five final routes pass with no pending calls, fixtures or drain.
No threshold-based byte comparison was accepted as proof of ownership.

Private evidence is under `build/runtime/eur_pause_list_navigation/`; its focused
generator/body/probe and the shared `verify_pause_list_graphics_artifacts.py`
are in `build/analysis/`. All 189 screenshots and 1,701 graphics dumps validate,
with 95 images and 855 dumps equal to matching input prefixes of prior routes.
All 104 original saves retain their hashes. Unsampled idle bodies, invalid
inputs and unvisited branches remain outside this runtime coverage.

### Pause list graphics initialization

`PauseList_InitGraphics` at `0x0207616C..0x020762DC` adds 368 bytes to
`pause_party_lifecycle.cpp`; its 152 neighboring initialization/cleanup bytes
were already linked. It resets both five-byte saved-position arrays and the
list control bytes, clears 4,224 main-OBJ bytes, and builds digits 0 through 9.
Each digit is rendered into scratch tiles at party offsets 332 and 1,356, then
copied as two 32-byte tiles to both screens' OBJ buffers.

The first draft already had the correct size and instructions apart from ten
stack references. Explicit clear blocks allocated their local zero values in
different slots from the native code. The inline clear wrappers already used
in nearby pause-resource code reproduce the native separation of caller text
buffer, hidden `GameTextToken` return temporary and wrapper locals. Using that
same structure yields an exact 368-byte match, including the literal pool;
all three functions in the consolidated module match. The volatile zero accesses
are supported by native stack stores/loads, not a register-allocation workaround.

Two ordinary clothing/badge routes at checkpoints 65/86 exercise four complete
initializations across separate pause lifetimes over 3,740 frames. The probe
checks every initializer, all ten digit strings, the hidden result-pointer ABI,
cursor masks, saved-position/control resets, all zero fills, OBJ address results
and all 160 tile transfers. It checks complete party/header/work/save/display/
global/OBJ records. The text decoder's state, scratch pixels and token result
are observed; copies from those pixels to both screens are independently
verified. Constructor and allocator bodies are not checked by this probe.

Private reports are under `build/runtime/eur_pause_list_digit_tiles/`, with
`make_pause_list_digit_tiles_probe.py`, its body and composed probe in
`build/analysis/`. All 48 screenshots and 432 graphics dumps validate and equal
the prior routes, all original saves remain unchanged, and no calls are pending.
The complete movement/graphics batch adds 796 bytes, passes full `ninja check`,
reproduces the EUR ROM hash through packaging and native relinking, reports zero
native differences, and passes all 81 tests. The working total at publication is
729,148 / 1,563,700 matching C/C++ bytes (46.63%); overlay 7 is 64,392 / 142,264
(45.26%). Matching C/C++ plus symbolic assembly is 46.96%.

### Pause equipment highlight

`PauseEquipmentHighlight_Stop`, `Start` and `Update` own the contiguous
`0x02077164..0x020774CC` range (872 bytes) in
[pause_equipment_highlight.cpp](../../src/scene_menu_ov007/pause_equipment_highlight.cpp).
All three matched their first private draft. The state machine parallels the
already reconstructed shop highlight, with pause-specific SUB BG1 selection
and item eligibility. No compiler flags or assembly fragments changed.

The signed control byte is at workspace `+0x2DC`: 1 keeps the task active,
0 requests a fade-out and -1 cancels immediately. The selected item is an
unsigned halfword at `+0x2DE`, accessed through the native interior alias
`0x020907F0 + 0xDE`. It is an inventory index, not a bitmask. The shared workspace
keeps the old four-byte view alongside these named fields without changing size.
The 72-byte task holds phase at `+0x20`, alpha at `+0x28`, target at `+0x2C`
and step at `+0x30`; the factory and constructor leave other payload bytes intact.
The fade step divides by six, but integer truncation requires seven updates to
clamp between 0 and 32,768. The native code reloads the party global between
category lookup and eligibility checking.

All four native start call sites are guarded by category equal to 2 (clothing):
`0x0206BB48`, `0x0206C5B0`, `0x0207CB70` and `0x0207CD74`. A badge-menu replay
therefore does not exercise the update function's category-3 fallback. The
initial `badges86` replay failed its positive execution assertion after observing
only an inactive stop; its log and failure report are retained. The separate
`badges_inactive86` replay verifies the observed inactive stop and absence of
creation/update on that route. This is a corrected coverage expectation derived
from the callers, not a change to the matching game code.

Private `make_pause_equipment_highlight_probe.py` composes a focused probe from
`pause_equipment_highlight_probe_body.py`; reports are under
`build/runtime/eur_pause_equipment_highlight/`. Clothing, clothing scrolling,
immediate exit and inactive badges cover 6,520 frames. They observe four starts,
1,812 updates, four stops and four removals; all starts/stops/removals and 113
updates are fully checked. Stable phase-2 updates sample the first two per task
lifetime/control state/item/availability/category tuple and every sixtieth frame.
Control fields are checked across every callback entry, independently of sampling.

The four task lifetimes verify factory free-list selection, complete slots,
list/counter writes, deferred marks, unlinking and actual pool return. Both
eligibility results run during clothing scrolling. Three tasks fade out; pressing
Start with the clothing list open cancels the fourth immediately and returns
to the field. The probe independently checks 252 ordered blend, display-enable,
vertical-offset and direct blend-clear stores, plus full party/workspace/save,
display-resource and both BG/OBJ records. Party constructor/free hooks establish
readable lifetimes; their bodies are not newly verified here.

The dedicated artifact verifier passes all 120 screenshots, 1,080 graphics dumps
and 104 unchanged source saves. Common input/state prefixes equal 118 images
and 1,062 dumps from the earlier preparation routes. No fixtures, pending calls,
live tracked tasks or drain frames remain. The scrolling highlight and final
field capture were inspected. Native guards positively identify foreign overlay-0
owners after pause exits. Start-while-active, unavailable-party branches,
unvisited task phases and unsampled stable bodies remain uncovered; final GPU
rasterization is observed. Full public build checks, both golden ROM build paths,
progress checks and all 81 tests passed for this source and shared-header revision.

### Pause exit tasks and transition state

The exit subset in [pause_transition_tasks.cpp](../../src/scene_menu_ov007/pause_transition_tasks.cpp)
is `0x0206D418..0x0206D51C` (260 bytes): `PauseScene_PrepareExitTask` drains the
archive queue and requests exit; `PauseScene_FadeOutTask` darkens both screens
and advances the scene to cleanup. The latter matched immediately. Preparation's
only differing instruction was `MOV 255` where the original materializes -1
with `MVN`; a local signed-byte access preserves that value without changing
the shared workspace's existing unsigned view.

[pause_scene_control.cpp](../../src/scene_menu_ov007/pause_resources.cpp) owns
`0x02070AE8..0x02070B50` (104 bytes): `PauseScene_RequestExit` sets scene phase 5
and resets the menu callback's phase, selecting the simple fade for mode 0
and the existing shutter transition otherwise. `PauseTransition_GetProgress`
returns the first signed word of the 9,224-byte transition workspace at
`0x0208E1E8`. Both functions matched their first drafts. The Scene VM now uses
the recovered exit prototype and scene-pointer type; its native byte output
remains exact. The native dispatcher narrows the decoded mode to a byte before
the call, while the request function accepts a full-width integer.

Private `make_pause_exit_probe.py` and `pause_exit_probe_body.py` produce
`probe_pause_exit.py`; reports are under `build/runtime/eur_pause_exit/`.
The final routes are `normal_exit65_pool`, `fade_exit65_pool` and `repeat_exit65`,
covering 3,190 frames. They check every preparation (six), request (three), fade
update (17) and watched removal (three). The progress getter is observed 54,464
times and fully checked 998 times: the first two calls per distinct value and
all calls on every sixtieth frame. All 17 observed values from 0 through 16,384
in increments of 1,024 are represented in the checked samples.

The ordinary route exits the clothing list with Start. The alternate route
verifies the normal caller's arguments, then changes `r1` from 1 to 0 once at
the guarded `PauseScene_RequestExit` entry (`0x02070AE8`, frame 522). The native
callback reset, initialization, all 16 brightness decrements, scene phase 6 and
actual pool return then run. This verifies the alternate mode with a controlled
argument fixture; it does not establish a natural script route to that mode.
Both screens' 32 conditional brightness stores are checked in order, including
ARM condition flags. No RAM value is frozen and no battery save is modified.

Full task, scene, transition, workspace, save, display, BG/OBJ and brightness
records are checked at the relevant boundaries. Expected effects of the nested
request are composed into its caller's expected record. All three archive
flushes had empty queues; nonempty request processing is not newly verified.
The probe also checks 749 menu factories and ten pool-cleanup calls, including
repeat calls on already empty pools. It closes the three watched task lifetimes
at their actual pool return and avoids reading their slots after release.

The first long clothing replay exposed missing bulk-pool destruction tracking
when pause reopened. Its failure at frame 1,512 is retained. After that correction,
the same route failed its exit-entry coverage assertion: it exercised the getter,
but never entered preparation/request. The final repeated-exit route starts with
the known clothing-list exit and adds two ordinary pause open/close cycles; it
checks reuse across the pool lifetimes and ends in the field.

All 41 screenshots, 369 dumps and 104 unchanged source saves pass the dedicated
artifact verifier. Common input/state prefixes match 31 images and 279 dumps;
fixture comparisons stop before the argument edit. Fade and final field captures
were inspected. No pending calls, live watched tasks or drain frames remain.
Unvisited phases, nonempty archive queues, unsampled getter bodies and natural
entry to mode 0 remain uncovered. Full matching checks, golden packaged ROM,
zero-difference native relink, progress checks and all 81 tests pass.

### Pause transition panels and controllers

[pause_transition_tasks.cpp](../../src/scene_menu_ov007/pause_transition_tasks.cpp)
owns `0x0206D418..0x0206E0D4`: the existing 260-byte exit subset and 3,000 new
bytes for six callbacks. Entry/exit panels use the shared 64-byte sprite,
progress controllers coordinate the screen transition, the window callback
clips the main screen, and the alternate entry callback fades into the menu.
[pause_transition_projection.cpp](../../src/scene_menu_ov007/pause_transition_projection.cpp)
adds the 180-byte projection at `0x0206E1C8..0x0206E27C`; its separate unit is
temporary while the intervening 244-byte affine-row producer remains native.
Only the 3,180 new bytes count toward progress. The shared sprite's named
`screen` byte preserves its raw 20-byte prefix view and 64-byte size.

All seven functions match completely. The 532-, 396-, 856- and 380-byte bodies
matched their first drafts; the 676-byte controller matched after using the
existing factory declaration. The previously exact 160-byte callback and
180-byte projection remain exact against current headers. No assembly or
compiler-flag changes were used. Preserve the controller's phase-1 fallthrough
to phase 10, signed counters and separate masked/unmasked stores.

Four completed routes in private `build/runtime/eur_pause_transitions/` cover
3,960 frames: `normal65_regs`, `normal86_reader`, `fadein65_reader` and
`repeat65_reader`. Ordinary routes use story checkpoints 65 and 86; the repeated
route opens and closes pause three times and ends in the field. Every observed
transition callback is checked: 820 entry-panel, 408 exit-panel, 210 entry-progress,
102 exit-progress, 125 window and 18 alternate fade-in calls. Nested projections
are all checked. Other projection calls check the first occurrence of each
X/Y/angle tuple and every call on each sixtieth frame: 6,204 of 71,652 total
projection entries are independently verified.

The oracle derives wrapped signed arithmetic, packed sprite fields, all four
projection outputs, draw-list allocation, task/owner counters and window/blend
effects. It checks 684 ordered conditional GPU stores, 955 task factories,
546 ResourceB attachments, 60 actual task returns, 44 associated sprite returns
and 12 bulk-pool cleanup calls. All 61 watched callback lifetimes close: 60 by
removal and one by retargeting the alternate fade-in task to the normal menu
callback. Retargeting ends callback coverage, not the task allocation. Task and
sprite records are checked before their respective pool returns.

Alternate entry uses a controlled fixture at chooser `0x0206E918`, frame 42:
change the byte at `read32(0x02059FE8) + 0x55D` from 0 to 1, then restore it to 0
at the chooser's return in the same frame, after checking that the selected
callback is `0x0206DF58`. This exercises the native fade-in and subsequent normal
inputs; it does not establish a story checkpoint that naturally selects this
mode. The callback reaches scene phase 2 and retargets itself after 16 brightness
steps. Original battery saves remain unchanged.

All 52 PNGs and 468 graphics dumps pass the artifact verifier; 39 images and
351 dumps equal earlier captures with matching state/input prefixes. Fixture
comparisons stop before the edit. Exit frame 529, alternate menu frame 100 and
repeated-route final field frame 1,650 were inspected. No pending calls, watched
tasks or drain frames remain, and all 104 source-save hashes are unchanged.
Full matching checks, golden packaged ROM, zero-difference native relink,
generated-progress validation and all 81 tests pass.

The first probe failed at frame 44 because DISPSTAT/VCOUNT were treated as
immutable across a long helper call. Those scanline-dependent values, and
IRQ-driven affine registers, remain in captures but are excluded from immutable
record comparisons. A later benchmark variable shadowed the probe's `start`
function; that frame-44 failure and the corrected successful replay are retained.
The original slow replay completed before a proposed stop was issued; its
session returned exit 0 and its full report is retained.

Private `desmume_copy_bytes.c` and `fast_desmume_memory.py` move the loop over
DeSmuME's existing exported byte-reader into native host code. Every address and
byte access is preserved; the helper does not read emulator host RAM directly.
Boundary checks passed. Each accelerated route then compared all 4 MiB of main
RAM, 16 KiB of DTCM and the nine graphics capture ranges with the original reader
while paused: 4,608,234 identical bytes per route. Measured reads took about
0.012 seconds versus 0.66 seconds. This changes probe overhead only and remains
an optional local tool, not a fresh-clone dependency or matching-game progress.

Affine-row producer outputs and alternate fade-in setup-helper effects are
observed, not independently derived here. Final rasterization, unvisited task
phases, unsampled external projections, arbitrary angle/step ranges and natural
alternate-entry selection remain coverage limits. The generator, body and
composed probe under `build/analysis/` are `make_pause_transitions_probe.py`,
`pause_transitions_probe_body.py` and `probe_pause_transitions.py`; the dedicated
verifier is `verify_pause_transitions_artifacts.py` in the same directory.

### Pause transition projection and callback ABI

These EUR overlay-7 ABI findings explain the source and focused transition
replays above. The earlier exit-only replay did not independently verify these
bodies. Use the current manifest and recorded route evidence for their status.

The projection at `0x0206E1C8` takes seven arguments:

```c
void PauseTransition_Project(int x, int y, int angle,
                       int *out_x, int *out_y,
                       int *out_width, int *out_height);
```

At entry, `r0..r3` hold `x`, `y`, `angle` and `out_x`; the other three pointers
are at entry `sp + 0`, `sp + 4` and `sp + 8`. Account for the prologue before
interpreting later stack loads. The function explicitly masks the full-width
angle with `0xFFFF` before shifting by four to index the interleaved sine/cosine
table. A halfword parameter or cast does not reproduce that same instruction
sequence with this compiler.

Outputs are stored in the order Y, height, width, X. The stored width is read
back before calculating X; preserve that read and possible output aliasing.
The perspective calculation needs signed 32-bit wrapping before arithmetic
shifts and division toward zero. An unbounded Python integer model can produce
the wrong expectation even when a formula looks equivalent.

The progress callback at `0x0206D730` reads its signed step from task `+0x2C`.
In phase 1 it computes a sum and its `0xFFFF`-masked value, stores both in that
order to the transition workspace's first word, then compares and passes the
cached masked value. Re-reading the global after each store introduces extra
loads; deleting the first store also changes the native sequence.

The panel callback at `0x0206D51C` projects directly into the attached
`Overlay5ObjectSprite` coordinates, with width and height in stack locals.
The window callback at `0x0206D7D0` instead supplies four stack-local outputs.
A probe must check the actual live destination extents, including DTCM where
applicable, rather than assuming every output points into the transition object.
The panel's packed tile field is ten bits wide, and its affine call explicitly
narrows width and height to unsigned halfwords. Preserve neighboring attribute
bits and caller-side narrowing without changing the full-width callee signature.

The focused transition reports add arithmetic, output, GPU-store and lifetime
assertions to the earlier exit-only evidence. The affine-row producer at
`0x0206E0D4` still has observed outputs rather than an independent body oracle;
keep that distinction when extending the adjacent code.

### Pause transition setup calls

These are code-derived EUR overlay-7 findings from the native callers and
existing shared headers. They do not establish independent runtime verification
of the setup bodies. Check the manifest and private handoff for integration status.

The exit setup at `0x0206E27C` and entry setup at `0x0206E594` both call
`GameSpriteAllocation_Allocate` at `0x0200856C`. Its existing declaration in
[sprite_output.h](../../include/game/sprite_output.h) has eight arguments:

```c
int GameSpriteAllocation_Allocate(GameSpriteAllocation *allocation,
    int screen, int mode, u32 tiles, u8 shared, u32 resource,
    u8 dirty, u32 first_tile);
```

Both callers pass `&WORK.sub_allocation`, screen 1, mode 0, 384 tiles, shared 0,
dirty 1 and first tile 0. The resource argument differs: exit passes `0xFFFF`,
while entry passes `0xFF00`. At callee entry the four stack arguments are
shared, resource, dirty and first tile at `sp + 0`, `+4`, `+8` and `+12`.
The following `Overlay5Display_LoadObjTiles` at `0x02068220` takes four
arguments: sub-screen engine, `WORK.owned94`, `WORK.sub_allocation.offset` and
12,288 bytes. Its existing [display declaration](../../include/game/overlay005_display.h)
already matches this call. Do not copy omitted or invented pseudocode arguments
into a shared-header refactor.

The delay callback at `0x0206E8DC` reads the 32-bit counter at task `+0x24`.
A nonzero value is decremented and the callback returns, even when that decrement
reaches zero. A value already zero at entry calls `GameRumble_PlayRepeated(0, 2)`
and requests deferred task removal. Thus an initial delay of four triggers on
the fifth callback; an initial zero triggers immediately. The request does not
free the task. This differs from timed callbacks whose zero means indefinite.

The chooser at `0x0206E918` first calls `0x02080AE4`, then reads the unsigned
save byte at `read32(0x02059FE8) + 0x55D` once. If nonzero, it stores byte minus
one in `WORK.selected_menu` and selects `PauseScene_FadeInTask`; otherwise it
selects the entry setup at `0x0206E594`. Both callback changes pass `invoke = 0`;
the callback setter resets the task's phase to zero. The chooser then sets the phase at
`read32(0x0208E1E0) + 0x30` to 1. Earlier alternate-entry evidence observes this
selection; it does not independently check the chooser's complete body or the
preceding setup helper. The focused page-transition checks below now cover the
chooser itself; the preceding setup helper remains observed.

### Pause page entry and return

The page-transition batch added the contiguous `0x0206E8DC..0x0206F04C` range
to [pause_page_tasks.cpp](../../src/scene_menu_ov007/pause_page_tasks.cpp): delayed rumble (60 bytes), entry
selection (124), page closing (532) and page opening (1,188), totaling 1,904
new matching C++ bytes. Shared task layouts are in
[pause_navigation.h](../../include/game/pause_navigation.h). The existing
scroll-state view at `0x020906F0` is now shared through
[scene_menu_motion.h](../../include/game/scene_menu_motion.h), preserving its
interior anchor and layout.

Closing fades for four updates, hides the selected page, waits for dependent
tasks, restores background priorities and returns to the main menu callback.
Opening creates the selected page, waits for dependencies, uploads glyph tiles
where needed and fades for eight updates. The final callback depends on page
0 through 4. During that fade, forward/back button presses update the queued
direction; these writes are checked separately from the subsequent dispatch.

The setter at `0x02066358` always installs the callback and resets task phase
to zero. Its third argument is **invoke immediately**, not an initial phase.
With a nonzero argument it calls the new callback before returning. This explains
the phase-0 and phase-1 opening calls observed in the same frame. Do not give
that parameter phase semantics when reconstructing callers or building an oracle.

Four completed private replays under `build/runtime/eur_pause_pages/` cover
8,260 frames. `pages65_stores` and `pages86_stores` open and close all five pages
using ordinary inputs at story checkpoints 65 and 86. `alternate65_stores`
repeats them with the guarded chooser-byte fixture described above, restored at
the chooser return in frame 42. `queued65` presses A then B during the opening
fade and covers both queued-direction writes. All four routes end in the field.

Every observed target call is checked: 320 openings, 208 closings, four choosers
and 15 delayed-rumble updates. The oracle derives direct task/workspace writes,
callback arguments, 720 ordered GPU stores and 14,592 OBJ-upload bytes. It checks
1,536 factories, 871 ResourceB attachments, eight pool cleanups and seven watched
task lifetimes through actual removal. Callback changes do not end allocation
tracking. Full live task, work, party, scene, save and graphics records are checked
at 4,049 boundaries; there are no pending calls, live watched tasks or drain frames.

All 109 screenshots and 981 graphics dumps pass artifact validation, and all
104 original save hashes remain unchanged. Ten screenshots and 90 dumps equal
earlier captures over identical state/input prefixes. Checkpoint-65 page screens
at frames 1,240 and 2,100 and each route's final field image were inspected.
The optional host reader matched the original reader across 4,608,234 paused
RAM/DTCM/graphics bytes on each route. Full matching checks, golden packaged ROM,
zero-difference native relink, generated-progress validation and all 81 tests pass.

Two initial probes failed at frame 204: the store hook omitted register-indexed
addressing, then the oracle incorrectly expected scroll-register readback to
equal the submitted value. The corrected probe checks the indexed store and
its ordered value directly. Full display captures remain available; write-only
scroll values are excluded from immutable readback expectations. Both failures
and successful reruns are retained. No game-code correction was needed.

Setup/list/menu/sound/rumble helper effects are observed at return unless explicitly
derived above. The native renderer, physical rumble, other input/flag combinations
and unvisited states remain outside this coverage. Probe sources are private
`make_pause_pages_probe.py`, `pause_pages_flow.py` and composed
`probe_pause_pages.py`; `verify_pause_pages_artifacts.py` validates the reports.

### Pause main menu and member selection

[pause_menu_control.cpp](../../src/scene_menu_ov007/pause_menu_control.cpp) adds
`PauseMenu_GetMemberPosition` at `0x0206F240` (96 bytes) and
`PauseMenu_UpdateTask` at `0x0206F2A0` (1,204). The adjacent
`PauseMenu_CanSelectMember` at `0x0206F04C` (152) extends
[pause_page_tasks.cpp](../../src/scene_menu_ov007/pause_page_tasks.cpp). All 1,452
new bytes match without assembly or compiler-flag changes. The intervening
preferred-member function at `0x0206F0E4` remains native.

The main-menu task has a five-update selection delay and separate queued
action/movement words. Fresh presses can wrap at the first/last menu entry;
held repeats clamp there. Confirmation waits for the background task, copies
32 bytes from main OBJ palette +384 to sub OBJ palette +384, then installs and
immediately invokes the page-opening callback. The two cached input halfwords
at workspace +2/+4 retain separate native reads. Its 72-byte view is shared in
[pause_navigation.h](../../include/game/pause_navigation.h).

The member grid at `0x0208D964` is `[[3, 2], [1, 0]]`: babies above adults,
Luigi on the left and Mario on the right. Reverse lookup writes two full-width
coordinates and defaults to `(0, 0)` when no entry matches; callers consume
the pointer outputs, not an inferred return value. Availability reads the
four-byte alias at `0x02090706`. Equipment selection also checks the highlighted
item and party category. The existing item view at `0x020907F0` is now shared
through the pause internal header, preserving its halfword at workspace +0x2DE.

Six ordinary-input replays under `build/runtime/eur_pause_menu_core/` cover
8,494 frames: five-page entry/return at checkpoint 65; clothing recipients at
65; badge recipients at 86; held-input and boundary movement; queued confirmation;
and queued cancellation. Every observed target call is checked, including
2,838 main-menu calls, 14 reverse lookups and eight availability calls. All four
member positions run; clothing accepts both adults and rejects both babies,
while badge selection accepts all four. Twelve reverse lookups write into
ARM9 DTCM stack storage, with neighboring bytes preserved. Both boundary wrap
and clamp paths run, as do queued up/down repeats and confirm/cancel actions.

The probe independently models 36 callback installations, including eight
synchronous page openings. Existing page/chooser/rumble checks remain active:
160 opens, 104 closes, six choosers and 30 rumble updates. It verifies 256 copied
palette bytes, 12,544 OBJ-upload bytes, 360 ordered GPU stores, 1,258 factories,
689 ResourceB attachments and 12 complete watched task lifetimes through actual
removal. Full live records are checked at 6,059 boundaries. No pending call,
watched task or drain frame remains. Final overlay-0 native guards and inspected
screens show the field on every route.

All 122 screenshots, 1,098 graphics dumps and 104 unchanged source-save hashes
validate; 46 images and 414 dumps equal earlier common input/state prefixes.
Both recipient-selection screens and every final field image were inspected.
The first clothing route stopped at the main menu with a watched task still
live. Its failure report is preserved; adding the missing ordinary exit input
produced the passing `clothing65_exit` replay. Earlier discovery reports are
excluded from the six-route totals. The final probe checks field code ownership
instead of interpreting the former pause BSS after overlay 0 replaces it.

Private tooling: `make_pause_menu_core_probe.py`, `pause_menu_core_flow.py`,
composed `probe_pause_menu_core.py`, and `verify_pause_menu_core_artifacts.py`.
Full matching checks, the golden packaged ROM, zero-difference native relink,
generated-progress validation and all 81 tests pass. Opaque setup/background,
label, palette-reset and sound helpers are observed at return. Main-menu phases
1000/1001, doubled queued movement, unavailable-member early return, invalid
member IDs, non-equipment availability and final rasterization remain outside
this runtime coverage. The preferred-member and shutter setup gaps are private.

### Pause status and Cobalt Star pages

[pause_page_control.cpp](../../src/scene_menu_ov007/pause_page_control.cpp) owns
`PauseStarPage_UpdateTask` at `0x0206ABD0` (208 bytes) and
`PauseStatusPage_UpdateTask` at `0x0206ACA0` (992). All 1,200 bytes match without
assembly or compiler-flag changes. The page-opening dispatcher uses these names;
the status task's 72-byte view is shared in
[pause_navigation.h](../../include/game/pause_navigation.h).

Both controllers enter phase 100 by falling through from phase zero. They handle
Start as a direct exit, and B or a previously queued cancellation as a return to
the main pause menu. Status selection preserves its signed member byte and wraps
across four slots, checking availability before accepting a member. Offset +48
changes meaning from queued direction to blend step during phases 501 onward;
+52 retains the previous member. Each of the two Q12 fades updates the accumulator
before increasing its step by 2,048, then divides by 4,096 with signed truncation.
From the tested initial state each fade takes nine callback invocations.

Six ordinary-input replays at checkpoints 65 and 86 cover 7,086 frames: all five
pages, status member switching, direct exits from both target pages, and B queued
during each opening fade. Every observed target call is checked: 998 status and
339 Cobalt Star updates. The routes cover all four members, both selection
directions, both wrap boundaries, seven complete pairs of fades, normal return,
direct exit and queued cancellation. All six end in the field, with positive
overlay-0 native guards, no pending calls and no watched task left alive.

Independent checks include 14,336 BG-map copy bytes, the separate four-byte member
mode array at `0x020A6B8C`, and both ordered halfword stores in each BG screen-base
and character-base update. Tilemap buffers `0x06001800` and `0x06002000` both
belong to main BG. The suite checks 725 GPU stores, 26,432 total copied bytes,
1,081 task factories, 520 ResourceB attachments, 12 pool cleanups and 12 watched
task lifetimes through actual removal. Full live records are checked at 7,463
boundaries. The existing page/main-menu checks remain active, including 45
callback installations and ten synchronous page openings.

All 116 screenshots, 1,044 graphics dumps and 104 unchanged original saves
validate. Forty-seven images and 423 dumps equal earlier common input/state
prefixes. All four status displays, the Cobalt Star display and every final
field screen were inspected. Full matching checks, the golden packaged ROM,
zero-difference native relink, progress validation and all 81 tests pass.

Private evidence is under `build/runtime/eur_pause_page_control/`. Tooling:
`make_pause_page_control_probe.py`, `pause_page_control_flow.py`, composed
`probe_pause_page_control.py` and `verify_pause_page_control_artifacts.py`.
Equipment setup/show/hide, background, label and sound helpers remain observed
where their operations are not independently derived above. Partial-party
availability skips, conflicting simultaneous inputs, invalid phases and final
rasterization are not covered. The tilemap-frame and preferred-member drafts
remain private; their presence does not add matching coverage.

### Pause list-row refresh

`PauseListRow_Refresh` at `0x020741E4` adds 252 exact bytes to
[pause_list_row.cpp](../../src/scene_menu_ov007/pause_list.cpp), completing the gap
before the list-control module. It uses the existing 72-byte
[row task](../../include/game/pause_list_row.h). The callback wraps the tile-row
index modulo nine before applying scroll offsets, derives the OBJ tile number,
updates the item and quantity, and selects the normal or unavailable palette.
Preserving that arithmetic order and the native conditional-store order resolved
the comparison without assembly or compiler changes.

Four ordinary-input replays at checkpoints 65 and 86 check every observed row
callback: 20,817 updates across 7,624 frames. They cover consumables, key items,
clothing, badges and Bros. Items, all nine row slots/tile positions, and both
palette outcomes. Tile-row and item getter results, coordinate/tile calculations
and full row records are independently derived; quantity and availability helper
results are observed before checking their use by this callback. All 53 watched
task lifetimes end in actual removal. Existing menu/page/member checks remain
active, and all four routes return to the field with positive native guards,
no pending call and no watched task remaining.

The pages and clothing routes retain full graphics checks at every row call.
The badge and key-item routes check the row's full live task and game-data
records at call boundaries; this callback writes no graphics memory. Existing
controller GPU checks and frame graphics captures remain enabled on all routes.
The host probe skips Python's changed-byte counting loop when native byte equality
already proves a captured block unchanged. Probe versions and their hashes are
recorded separately; none of these host changes affects game matching coverage.

All 112 screenshots, 1,008 graphics dumps and 104 unchanged original saves
validate; 95 images and 855 dumps equal earlier common input/state prefixes.
Item, clothing, badge and key-item displays and every final field screen were
inspected. An initial key-item route made one extra B press after reaching the
field, jumping into the save block and opening the save menu. Its frame-1,674
failure is preserved. Removing that extra input produced the passing
`keys65_field` replay ending at frame 1,484; the field guard was retained.

Full matching checks, the golden packaged ROM, zero-difference native relink,
progress validation and all 81 tests pass. Private evidence is under
`build/runtime/eur_pause_row_refresh/`; tools are `make_pause_row_refresh_probe.py`,
`pause_row_refresh_flow.py`, composed `probe_pause_row_refresh.py` and
`verify_pause_row_refresh_artifacts.py`. Invalid row/count inputs, negative
tile-row states, quantity/availability helper internals and rasterization are
outside this callback's independent coverage. The equipment-category and map
decoder drafts remain private with classified differences.

### Pause equipped-item markers

[pause_equipped_markers.cpp](../../src/scene_menu_ov007/pause_equipped_markers.cpp)
owns `PauseEquippedMarker_Update` at `0x02073AF4` (416 bytes). It finds each
displayed member's equipped clothing or badge among the first nine visible
inventory rows. A missing or off-screen item produces no draw entry. Badge
partners keep separate vertical offsets when sharing an item; an unshared badge
cancels that offset and uses a one-pixel adjustment. The task uses the shared
72-byte [row-task header](../../include/game/pause_list_row.h) and existing
`SavePartyMember` fields. Native addition order is preserved; no assembly or
compiler-flag changes are needed.

Three ordinary-input replays at checkpoints 65 and 86 cover 5,650 frames and
every observed marker call: 10,908 updates, 77,382 independently checked item
lookups and 5,454 draw-list insertions. They cover all four members, clothing,
badges, list scrolling, the nine-row clamp, off-screen equipped items, both
unshared-badge adjustments and 100 shared-badge updates. Full task, sprite and
game-data records are checked at call boundaries. Draw-pool allocation, linked
list insertion and helper return values are independently derived. Initial
marker setup is observed before the updater runs.

Existing row, menu, page and member-selection checks remain active, including
24,543 row refreshes. All 45 watched lifetimes end in actual removal, including
12 equipped-marker tasks with their sprite resources returned. All three routes
finish in the field with full native overlay-0 guards and no pending call,
watched task, drain frame or RAM fixture. The suite checks 813 task factories,
472 ResourceB attachments, six pool cleanups and 171 controller GPU stores.
Pure data callbacks use focused task/game records; controller graphics checks
and per-action captures remain enabled.

All 123 screenshots, 1,107 graphics dumps and 104 unchanged original saves
validate. Seventy-one images and 639 dumps match earlier common input prefixes.
Clothing, badge and scrolled-list screens and all three final field screens
were inspected. Full matching checks, the golden packaged ROM, zero-difference
native relink, generated progress and all 81 tests pass.

Private evidence is under `build/runtime/eur_pause_equipped_markers/`. Tools:
`make_pause_equipped_markers_probe.py`, `pause_equipped_markers_flow.py`, composed
`probe_pause_equipped_markers.py` and `verify_pause_equipped_markers_artifacts.py`.
Each report identifies the probe source and hash. Unequipped members, lists of
nine or fewer entries, a partner not displayed, invalid IDs/categories and final
rasterization are outside this runtime coverage. Row quantity/eligibility and
opaque controller helpers retain their earlier stated limits. The neighboring
marker and row creators remain private drafts with classified differences.

### Pause empty-equipment row sprites

[pause_empty_row.cpp](../../src/scene_menu_ov007/pause_empty_row.cpp) owns
`PauseList_CreateEmptyRowSprite` at `0x02073428` (136 bytes) and
`PauseList_UpdateEmptyRowSprite` at `0x020734B0` (172). The creator attaches a
ResourceA model to a list task, loads asset slot 82, starts animation zero and
sets the model's low two flag bits to 2. The updater finds item ID zero within
the first nine visible rows, writes the model's pixel coordinates and submits
it to draw list 39. Otherwise it submits no entry. The existing list-show caller
uses the new public declaration. All 308 bytes match without assembly or changed
compiler flags.

Two ordinary scrolling routes at checkpoints 65 and 86 cover clothing and
badges across 4,060 frames. Both creators and every one of the 2,204 updater
calls are checked, including 19,836 item lookups, 99 empty-row draw insertions
and 2,105 hidden updates. The tested empty entry is at row index eight. Pool slot and
link selection, owner attachment, asset-table lookup, final flags, row lookup,
halfword coordinates and draw-list insertion are independently derived.
Full 336-byte model records remain checked while allocated. Model initialization,
animation and virtual cleanup effects are observed at helper boundaries.

ResourceA uses the pool at `0x0206A3D8`; its return helper is `0x02068E14`.
The probe verifies the pool is unchanged by virtual cleanup before independently
deriving the return's link writes. It stops reading the model after pool return.
Both models and their tasks complete their lifetimes. Existing row, equipped
marker and controller checks remain active: 19,836 row refreshes, 8,816 equipped
marker updates, 503 task factories, 282 ResourceB attachments and 114 controller
GPU stores. All 32 watched tasks are removed, and both routes finish in the field
with full native guards and no pending calls, live tasks, drain or RAM fixture.

All 124 screenshots, 1,116 graphics dumps and 104 unchanged original saves
validate; 72 images and 648 dumps equal earlier common input prefixes. Scrolled
clothing/badge lists and both final field screens were inspected. Full matching
checks, the golden packaged ROM, zero-difference native relink, generated progress
and all 81 tests pass. An initial link check found the list-show caller still
using the old name; migrating that reference fixed the build.

Private evidence is under `build/runtime/eur_pause_empty_row/`. Tools:
`make_pause_empty_row_probe.py`, `pause_empty_row_flow.py`, composed
`probe_pause_empty_row.py` and `verify_pause_empty_row_artifacts.py`. Reports
identify the exact probe source. Lists of nine or fewer entries, empty entries
in other rows, invalid indices, model-helper internals and rasterization remain
outside this independent coverage. The bean-display drafts are separate private
work; their presence does not add linked coverage.

### Pause selection sprites

[pause_category_cursor.cpp](../../src/scene_menu_ov007/pause_category_cursor.cpp)
reconstructs the equipment category cursor's creator at `0x0207B1B0` (200 bytes)
and updater at `0x0207B278` (80). It loads model asset 75 and positions the hand
beside the selected Clothing/Badges row. The shared
[task layout](../../include/game/pause_selection_sprites.h) is 72 bytes, with
pixel origins at offsets 40 and 44. The page-opening caller uses the public
no-argument creator declaration; the incoming register value is not consumed.
[pause_list_cursor.cpp](../../src/scene_menu_ov007/pause_list_cursor.cpp) owns the
selection marker at `0x0207F4D8` (156 bytes) and cursor at `0x0207F574` (132).
Both follow the selected list row, retaining the native Q12 conversion and
signed division before storing pixel halfwords. All 568 bytes match without
assembly or compiler changes.

Two ordinary-input scrolling replays at story checkpoints 65 and 86 cover
4,060 frames: four category cursor creations, 724 category updates, 2,180 list
cursor updates and 2,180 selection-marker updates. All nine visible list rows,
both equipment kinds and both category rows are covered. The oracles derive
origins, selected indices, Q12 truncation and all 5,084 draw-list insertions;
4,360 selection-index lookups are independently checked. They validate arguments
for 2,180 marker animation calls and observe animation internals at return.
Complete live task and 336-byte model records are checked at call boundaries.
Generic ResourceA attachment checks cover 65 calls, plus six attachments derived
inside the category and empty-row creators. All 40 watched tasks are removed,
including ten actual model-pool returns. Existing row, marker and page checks
remain active; both routes end in the field with full overlay-0 byte guards,
no pending calls, watched tasks, drain frames or RAM fixtures.

All 124 screenshots and 1,116 graphics dumps match the previous complete
scrolling routes. Category selection, both scrolled lists and both final field
screens were inspected; all 104 original saves are unchanged. Full module and
symbol checks, golden ROM packaging, zero-difference native relinking, generated
progress and 81 tests pass. The final declaration cleanup also passes the full
matching check. DSD requires separate source units for these two disjoint text
ranges; the initial combined-section failure remains in the private build log.

Private evidence is under `build/runtime/eur_pause_selection_sprites/`; scripts
are `make_pause_selection_sprites_probe.py`, `pause_selection_sprites_flow.py`,
composed `probe_pause_selection_sprites.py` and
`verify_pause_selection_sprites_artifacts.py`. Each report records its actual
probe source and hash. The category creator's alternate-origin flag was always
set; the other origin, other menus/item kinds, invalid indices, animation/model
initialization internals and final rasterization are outside independent runtime
coverage. Adjacent scroll-arrow and controller drafts are not linked progress.

### Pause member-selection arrows

[menu_equipment_member_arrow.cpp](../../src/scene_menu_ov007/menu_equipment_member_arrow.cpp)
reconstructs the 288-byte updater at `0x0207F268`. Its 72-byte task uses the
`member_arrow` view in [menu_equipment.h](../../include/game/menu_equipment.h):
Q12 offsets at 40/44, sine amplitude at 48, accumulated angle at 52 and signed
direction at 56. The updater follows the equipment heading, selects animation
7 for the arrow matching the switch direction and animation 1 otherwise, and
bobs horizontally while idle. Only the sine lookup masks the angle to 16 bits;
pixel conversion uses signed division by 4096. Closing the heading marks the
task for removal. The neighboring 332-byte creator remains unreconstructed;
its private draft differs in register allocation and scheduling.

The ordinary `clothing_arrows` route from story checkpoint 65 runs 2,380 frames
and checks all 2,804 updater calls: 2,802 independent draw-list insertions and
two removal marks. It visits all four members, both switch directions, both
animations, 2,658 sine lookups and 62 angle wraps. Initial task/model fields are
checked at each arrow's first update. Complete live task, model and game records
remain checked, including the actual model and task pool returns. Across all
watched callbacks, 120 tasks are removed and 27 model slots are returned; both
arrows finish their lifetimes. The route returns to the field with complete
overlay-0 guards and no pending calls, live watched tasks, drain or RAM fixtures.

The first clothing member-grid route completed 1,780 frames but failed the
required switching-coverage assertion: that grid never sets the heading's
switch byte. The corrected route inserts four Right presses, then Left and
Right, while the clothing list is open, waiting 90 frames after each eight-frame
press. The native input block at `0x0206B5D0..0x0206B698` requires equipment
kind 2 and accepts Left/L or Right/R. The subsequent badge member-grid attempt
completed 1,820 frames but created no member-arrow tasks and failed its target
count assertion. These failed routes are preserved and excluded from coverage.

All 41 screenshots and 369 graphics dumps validate. Nine images and 81 dumps
match the unchanged prefix of the earlier clothing member-grid route; the new
switching inputs have no prior full-route baseline. The changing heading and
final field were inspected, and all 104 original saves remain unchanged. Full
matching checks, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Animation internals and rasterization
are observed, not independently derived; other menu contexts and signed angle
accumulator overflow remain untested.

Private evidence is `build/runtime/eur_pause_member_arrows/artifact_validation.json`
and `clothing_arrows/evidence_clothing65.json`. The report identifies the composed
`build/analysis/probe_pause_member_arrows.py` and its hash; the initial grid-only
probe is preserved separately. Logs use the `pause_member_arrows_` prefix.
The artifact validator initially inherited a requirement to draw an empty row
from a scrolling route. This route's nine visible entries are all nonempty:
all 1,374 empty-row updates correctly remain hidden. The correction retains
the per-call item lookup, hide/draw oracle and count reconciliation, and does
not claim that this route covers the empty-row drawing branch.

### Pause equipment stat comparison rows

[menu_equipment_display.cpp](../../src/scene_menu_ov007/menu_equipment_display.cpp)
now includes the 760-byte `MenuEquipment_UpdateStatRow` at `0x0207A388`.
It extends the existing contiguous display unit; the heading creator, comparison
arrow and number-strip updater continue to match. The existing
[72-byte equipment task](../../include/game/menu_equipment.h) carries the parent
pointer at +16, stat index at +41 and cached current/preview values at +56/+60.
Valid stat indices are HP, power, defense, speed and stache (0 through 4).
The HP row compares maximum HP.

The callback reads the parent's member as unsigned and its closing flag as signed.
Closing marks the row for removal. Otherwise, five initialized signed halfwords
receive the clothing bonuses. Current values are unsigned save halfwords; the
preview adds the signed bonus to the base stat and truncates to 16 bits. An
ineligible item substitutes 65535. If both cached values are unchanged, no redraw
occurs. Changed values select equal/higher/lower comparison markers or the
unavailable marker, then call the existing numeric-strip helpers. Preserve the
separate comparisons and the stat-index readbacks across rendering calls.

The old private draft addressed a member inside the save array before reading
its fields, compiling to 748 bytes. Native instructions apply the 36-byte member
stride before the fixed save offsets. A 1,052-byte prefix containing the existing
`SavePartyMember` preserves that addressing; even for member 3, the full
prefix ends at save offset 1,160, within the 1,380-byte live record. That change
produces the exact 760 bytes without assembly or compiler-flag changes. The old
private comparison unit also contains already linked functions and is not an
additional coverage source.

Two ordinary replays use checkpoint 65's `clothing_scroll` route (2,010 frames)
and checkpoint 86's `clothing_arrows` route (2,380 frames). All 12,570 row calls
are checked independently: 12,452 cached-value returns, 108 redraws and ten
removal marks. The 157 numeric-helper calls comprise 49 current-value draws,
89 preview draws and 19 unavailable displays. Preview comparisons cover 50 equal,
22 higher and 17 lower results. All five stats and all four members are exercised.
The eligibility checks cover 10,060 allowed and 2,500 disallowed active callbacks.

At the first helper boundary, the probe checks all five initialized halfwords
at entry SP minus 44 through minus 35, after the native prologue has run. It
independently derives 62,800 bonus outputs from guarded 28-byte clothing records,
checks native widths and truncation, helper arguments, cached values and comparison
bytes, and retains full live task/parent/work/party/save records. The locals are
no longer inspected after their stack frame ends. Numeric rendering helpers are
observed with the full 12,288-byte scratch allocation, heap header and 65,536-byte
main OBJ range; their glyph conversion/transfer internals are not independently
verified by this row probe.

Both emulator processes pass, as does artifact validation. All ten comparison
rows complete their lifetimes; all 164 watched tasks and 42 model slots return.
Both routes finish with full overlay-0 field guards, no pending calls, live
watched tasks, drain frames or RAM fixtures. All 104 original saves remain
unchanged. All 102 screenshots and 918 graphics dumps validate. The first route's
61 images and 549 dumps equal its complete ordinary baseline. For checkpoint 86,
the older baseline used HP fixtures, so comparison is limited to two images and
18 dumps before its first edit. The unavailable preview and final field were
inspected. Preview overflow/wrap, invalid task fields and independent final
rasterization remain outside the exercised coverage.

Full matching checks, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Private evidence is under
`build/runtime/eur_pause_equipment_stat_row/`: `clothing_scroll/evidence_scroll65.json`,
`clothing_arrows/evidence_arrows86.json` and `artifact_validation.json`.
Reports identify `build/analysis/probe_pause_equipment_stat_row.py` and its hash;
its generator composes the preserved transition probe with the new row oracle.
Build and replay logs use the `pause_equipment_stat_row_` prefix. Neither replay
nor artifact validation needed an oracle correction or a rerun.

### Pause item-selection label sprites

[menu_item_selection_sprites.cpp](../../src/scene_menu_ov007/menu_item_selection_sprites.cpp)
reconstructs `MenuItemSelection_UpdateOffsetSprite` at `0x0207D8B0` (212 bytes)
and `MenuItemSelection_UpdateLabelStrip` at `0x0207D984` (412 bytes). These two callbacks cover
624 contiguous bytes and use the shared 64-byte sprite and
[72-byte owner/child views](../../include/game/menu_item_selection.h), with size
and offset checks. The neighboring model callbacks remain private candidates.

Both callbacks add signed Q8 owner offsets, multiplied by 16, to Q12 positions,
divide toward zero to obtain pixels, apply their pixel offsets and convert back
to Q12. The offset sprite subtracts 4 horizontally and 24 vertically. Label strips
use the owner's half-width and each child's offsets. The second screen adds 240
pixels here; this is a subsystem-specific translation. Preserve the native
readbacks between stores and the arithmetic widths.

The workspace byte at `WORK + 0x12D` selects the packed attribute mode. The label
callback reads `WORK + 0x12E`: values above 1 mark the task for removal; otherwise,
zero uses draw order 5 and nonzero uses 59. A negative signed owner status selects
the next palette bank. Its interior bank-byte alias starts at `0x0206A1F1`, with
the verified 20-byte palette stride. The offset sprite always uses order 5.
Native inspection recovered the final order branch omitted by the old pseudocode.
The offset callback matched directly; preserving the native positive-status arm
first made the label callback exact. The existing palette bitfield retains both
native low-four-bit masks. No assembly or compiler-flag changes were needed.

The checkpoint-65 equipment-scroll route and checkpoint-86 member-arrow route
each continue by opening the item menu, selecting a mushroom and cancelling.
Their successful replays cover 5,830 frames and independently check all
540 offset-sprite and 5,720 label-strip calls on both
screens. They verify full live task, parent, sprite, workspace, party, save and
display records, position/attribute/palette writes, helper arguments, draw-list
allocation and insertion, and actual resource/task return. There are
6,258 new draw submissions and
2 checked label-removal flags. Rasterization and the
unreconstructed creators/model callbacks are not independent oracles here.

Both snapshots start at full HP, which the native eligibility predicate rejects
for healing-item selection. At the second guarded pause-entry chooser, each run
temporarily lowers Mario's current HP by one, then restores the exact halfword
after cancellation and the final field guard. Three additional, separate
single-call fixtures check negative status, draw order 59 and the state-2 removal
branch. Inputs are restored at callback return; the removal flag is checked and
then restored before the caller resumes. Actual task release is subsequently
verified through ordinary group cleanup, not attributed to that transient flag.
These fixtures do not establish ordinary access from the unmodified full-health
snapshots. Reports separate their branches from the naturally exercised paths.

All 28 new sprite-task lifetimes finish; all
238 watched tasks and 66 model slots return.
Both runs finish with full overlay-0 field guards, no pending calls or live watched
tasks, and all fixture inputs restored. All 104 original saves remain unchanged.
All 126 screenshots and 1134 graphics dumps validate;
81 images and 729 dumps equal the
preceding equipment baselines before each run's first fixture. The checkpoint-86
route also executes label callbacks during equipment selection, so its comparison
stops at the earlier per-call fixture. Selected-item rendering was inspected.
No sampling is used for the new callbacks.

The original final capture was still black despite the loaded field overlay.
Separate focused replays reproduced all 126 original screenshot hashes and
1,134 graphics-buffer hashes with the same inputs and fixtures, restored HP at
the original end, then advanced 180 neutral frames per route. Three captures at
60-frame intervals retain full field-code guards and all nine graphics ranges.
Both final images, at frames 2,910 and 3,280, were visually inspected and show the
party in the field. The six new images, 54 dumps and two compatible field snapshots
validate; all 104 source saves remain unchanged. This follow-up observes fade
completion; it does not repeat the independent callback checks above or establish
an exact fade duration. Its separate reports are under
`build/runtime/eur_pause_item_selection_visual/`, using the same route/report
names, and record the source report and focused probe hashes.

The first checkpoint-65 attempt correctly failed its entry assertion at full HP.
The next attempt exercised both callbacks but failed the required local-removal
coverage: cancellation destroys the group instead. Both failed reports and probe
versions remain private. The passing version adds the documented fixtures and
retains every per-call check. The checkpoint-86 probe separately requires all six
member-arrow lifetimes created across equipment entry, item entry and cancellation;
its source is preserved separately from the first route's probe.

Full matching checks, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Reports are under
`build/runtime/eur_pause_item_selection/`: `clothing_scroll/evidence_scroll65_v3.json`,
`clothing_arrows/evidence_arrows86.json` and `artifact_validation.json`.
Their exact probe paths/hashes are recorded in the reports. Private build and
replay logs use the `pause_item_selection_` prefix; candidate/probe provenance
is pinned in `build/analysis/pause_item_selection_batch_provenance.json`.

### Pause item-selection quantity sprites

`MenuItemSelection_UpdateQuantitySprite` at `0x0207D6B0..0x0207D8B0`
adds 512 bytes to [menu_item_selection_sprites.cpp](../../src/scene_menu_ov007/menu_item_selection_sprites.cpp).
The complete 1,136-byte unit remains exact. The quantity callback matched on its
first private compilation; naming the shared fields and integrating it preserved
all three functions' matches. No assembly or compiler-flag changes were needed.

The owner's quantity is an unsigned byte at `+43`. The 72-byte child uses the
same slots as label offsets: `+44` is digit 0 for tens or 1 for units, and `+48`
is the base tile. The [shared header](../../include/game/menu_item_selection.h)
expresses these roles as unions and checks the field offsets and allocation sizes.
Position conversion, second-screen translation and blend attributes follow the
adjacent offset sprite; the horizontal adjustment is `8 * (digit + 13) - 4` pixels.

Tens are hidden below 10, after position and blend-attribute writes but before
changing the tile or palette or submitting a draw. Otherwise the callback selects
the quotient or remainder by 10, adds the tile base and masks to ten tile bits.
Signed negative owner status selects the next palette bank. Preserve the other
attribute bits, the native palette masks and draw order 5.

The checkpoint-65 and checkpoint-86 routes from the label-sprite batch each
exercise both quantity digits on both screens. The independent oracle checks
all 1,080 calls: 810 draw submissions and 270 hidden tens. The ordinary cached
quantities are 4 and 50, respectively. Sixteen additional per-call fixtures per
route cover 0, 9, 10 and 99 for every screen/digit pair; the 99 cases also set
negative status. They edit only the owner's cached quantity/status, verify the
complete callback result, then restore both bytes before the caller resumes.
The earlier one-HP entry fixture and three label fixtures remain separately
recorded. These are fixture-assisted routes, not unmodified full-health entry.

All eight quantity-task lifetimes complete; all 246 watched tasks return. The
replays cover 6,190 frames, including 180 neutral field frames per route after
HP restoration. Full field-code guards and final captures confirm visible return.
The selected quantities 4 and 50 and both final field scenes were inspected.
All 132 screenshots and 1,188 graphics dumps validate; pre-fixture baseline
equality covers 81 images and 729 dumps. All 104 original saves are unchanged.
Every target call checks full live task, owner, sprite and game records, getter
arguments, expected stores, draw-list insertion and actual resource/task release.
Graphics captures and the surrounding controller checks remain in place.
There are no pending calls, live watched tasks or drain frames. Rasterization,
the creator, quantities above 99, nonzero tile bases and invalid digit/screen
fields are outside this runtime coverage.

The initial composed probe failed at frame 43 because its wrapper name collided
with an inherited wrapper. The corrected probe uses a distinct name; both full
replays and artifact validation pass. The failed probe/log remain separate.
Full matching checks, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Evidence is under
`build/runtime/eur_pause_item_quantity/`: `clothing_scroll/evidence_quantity65_v2.json`,
`clothing_arrows/evidence_quantity86_v2.json` and `artifact_validation.json`.
The reports identify `build/analysis/probe_pause_item_quantity_v2.py` by hash;
build/probe provenance is in `build/analysis/pause_item_quantity_batch_provenance.json`.

### Pause badge-description panels

[menu_badge_description.cpp](../../src/scene_menu_ov007/menu_badge_description.cpp)
reconstructs `MenuBadgeDescription_UpdateStrip` at `0x0207BEE0` (132 bytes)
and `MenuBadgeDescription_UpdatePanel` at `0x0207BF64` (476 bytes), contiguous
through `0x0207C140`. Both functions match completely, including relocations and
literal pools. The initial panel draft confused the description lookup with the
name lookup and the sub-screen allocation offset with a sprite pointer. Correcting
those two native dataflow errors produced an exact match; no assembly or compiler
changes were needed.

The native creator at `0x0207B88C` creates these callbacks only for badges
(item kind 3). One panel owns fourteen text strips, arranged in two rows of seven.
All tasks occupy 72-byte slots and begin with timer 1. The
[shared header](../../include/game/menu_badge_description.h) distinguishes the
panel's owner from a strip's parent panel. The owner's transition flag is at
`+60`; the panel's cached item, x/y position and settled flag are at
`+40`, `+44/+48` and `+52`.

The byte at workspace `+0x12E` suppresses both callbacks and clears the panel's
settled flag. Otherwise the panel selects its y position and animation from the
signed member byte at `+0x115`. The saved badge uses the unsigned interpretation:
`save + 36 * member + 1048`. Preserve that addressing order and the unsigned
16-bit conversion of `badge + 0x3000`. Members 0 through 3 fit within the
1,380-byte save allocation. This workspace byte is distinct from `selected[0]`.
The transition/settled flags gate position refresh and cached text updates.
Strips copy the panel's position and submit at order 20; the model uses order 22.

The independent runtime checks cover all 13,350 new callback invocations on
story saves 65 and 86: 890 panel calls and 12,460 strip calls. There are 887 panel
draws, 12,431 strip draws, 30 initial-delay returns and two suppression returns.
All four members, both animations, pending/settled combinations and frozen/refreshed
positions occur. Text updates eight times, remains cached 823 times and is frozen
56 times. Both panel lifetimes and all 28 strip lifetimes complete; all 86 watched
tasks return, with no pending calls or drain frames.

Save 86 follows ordinary inputs. Save 65 adds two guarded per-call fixtures that
set workspace `+0x12E` to 1: one strip callback and one settled panel callback.
Each verifies suppression, then restores the input before the caller resumes.
The panel fixture also verifies its `1 -> 0` settled change and restores that
induced output. Both edits are restored, and all 104 original saves are unchanged.
These fixtures establish the suppression branches, not a natural story trigger.

Description IDs are independently checked against the native 20-byte badge records
at `0x02050290`, field `+4`. Animation internals are observed within the 336-byte
model. Text rendering observes the 4,428-byte party workspace and its bounded
3,584-byte sub-OBJ transfer; the surrounding sub-OBJ memory must remain unchanged.
The reserved graphics allocation is 7,168 bytes in both routes. Caller decisions,
arguments, stores, full live records, draw-list insertion and pool returns are
checked independently. The creator, glyph/animation algorithms, final rasterization,
invalid member indices and arithmetic overflow remain outside that coverage.

Both 1,820-frame replays pass. The description panels and final field captures
were visually inspected. All 62 images and 558 graphics dumps validate; 38 images
and 342 dumps match the applicable earlier input prefixes. The first artifact
validator incorrectly required save 65 to hide an equipped marker and assumed
all badge markers were unshared. Live inputs explain both differences: all equipped
badges are visible on save 65, and a paired member shares a badge. The corrected
validator retains the full per-call checks and reconciles both marker outcomes.

Full matching checks, golden EUR ROM packaging, zero-difference native relinking,
generated progress and all 81 tests pass. Evidence is under
`build/runtime/eur_pause_badge_description/`: `badge_members/evidence_badges86.json`,
`badge_members/evidence_badges65_cases.json` and `artifact_validation.json`.
Both reports pin `build/analysis/probe_pause_badge_description.py`; the independent
artifact validator is `verify_pause_badge_description_artifacts_v2.py` under
`build/analysis/`. Build and source provenance is recorded in
`build/analysis/pause_badge_description_batch_provenance.json`.

### Nawatobi

When explaining a memory edit, specify CPU/address space, ROM region, pointer
dereferences, field offset, access width and timing. The tested EUR Nawatobi
entry uses ARM9 main RAM with overlay 7 loaded. From a fully initialized pause
menu, break at pause update `0x02071F80` and perform this one-time edit
(pseudocode; `read32` and `write32` use little-endian 32-bit accesses):

```text
task = read32(0x0208E1E0)
assert task == r0 and read32(task) == 0x0208D9B8
phase_address = task + 0x30
assert read32(phase_address) == 2
write32(phase_address, 7)
```

The edited word is at `read32(0x0208E1E0) + 0x30` in ARM9 main RAM.
Its four bytes become `07 00 00 00`. `0x0208E1E0` stores the task pointer;
do not overwrite that pointer or add the offset before dereferencing it.
This is a runtime state field, not a VRAM address or a ROM patch.

Disable the edit hook and resume so the native transition can advance the
phase. The task is dynamically allocated; an observed task address is not a
universal address to patch. Its code is in `src/scene_menu_ov007/nawatobi_*`;
the selector and rope simulation were exercised, but a normal entry route
remains unconfirmed.
The compatible private starting snapshot is
`build/runtime/states/pause_subscene_menu65.dst`, derived from checkpoint 65.
Verify its provenance and the pause-update byte guard before reusing it.
The private capture records are `build/runtime/eur_nawatobi/evidence_065.json`
and `build/runtime/eur_nawatobi/rope_evidence_065.json`. Keep these local; cite
the observed setup and its limits when sharing instructions with researchers.

`build/analysis/probe_nawatobi_render.py` extends the route with follower/model
and rope-sprite oracles. Its `menu65`, `level1_65` and `level2_65` reports are in
`build/runtime/eur_nawatobi_render/`. From the selector, A starts Level 1;
one Down selection followed by A starts Level 2. The recorded button schedules
exercise all three follower animations on both screens and interior rope draw
orders. They verify controlled entry followed by native level selection and
updates, not successful level completion or a normal story entry route.
Nawatobi's rope sprite copy adds 208 pixels between screens; this differs from
the title sequence's 244 and the credits' 224. Use the subsystem's own layout.

Several different fields are called a phase or state. These are the recovered
EUR ARM9 access locations, with overlay 7 loaded; only the pause phase above is
the tested one-time entry edit. `r0` below means the argument at the specified
function's guarded entry, and `read32` dereferences a little-endian pointer.

| Field | Address or pointer expression | Access |
|---|---|---|
| Pause scene phase | `read32(0x0208E1E0) + 0x30` | 32-bit word |
| Nawatobi scene phase | `read32(0x020A6BBC) + 0x2C` | 32-bit word |
| Selector / level task phase | `r0 + 0x20` at `0x0208C5BC` / `0x0208C4D0` | 32-bit word; timer at `r0 + 0x24` |
| Active level / selected row | `0x020A6BCC` / `0x020A6BCD` | Bytes; selected row is read signed and is zero-based |
| Menu initialized / script running flags | `0x020A6BCE` / `0x020A6BCF` | Bytes |
| Scene script shared variable zero | `read32(0x020A6B90) + 0x37EC` | 32-bit level argument |
| Primary script instruction pointer | `read32(0x020A6B90) + 0x386C` | 32-bit pointer; zero means stopped |

The level callback copies the active level into shared variable zero before
starting the primary script. The primary start clears its own `0xB8`-byte slot,
preserving the shared variables; see
[the script manager](../../src/scene_menu_ov007/scene_script_manager.c). Its callback phases
0, 1 and 2 mean start, wait for the script, and a 60-update return delay. These
are code-derived meanings, not a claim that a runtime replay exercised every
transition. The selector displays five rows but accepts ordinary confirmation
only for the first two; the extra rows do not establish three playable levels.
Check the current link metadata and probe reports before claiming a neighboring
selector function is also reconstructed.

`build/analysis/probe_nawatobi_menu.py` checks complete console records, level
tasks, shared variables and primary script slots. Its reports are under
`build/runtime/eur_nawatobi_menu/`. Ordinary menu and Level 1/2 runs are separate
from `evidence_terminated_level1_65.json`: that fixture clears the initialized
primary script pointer once at a guarded level-callback entry. It verifies the
stopped-script branch, all 60 delay decrements and callback return to the selector;
it does not establish natural level completion or the script's own cleanup.
The ordinary level captures match the preceding rendering probe's graphics
buffers. Use the recorded fixtures and uncovered branches when extending it.


## Hammer input and impact

The overlay-21 input, impact, return and approach functions cover
`0x020C2BCC..0x020C3380` and `0x020C3864..0x020C395C` (2,220 bytes).
See [resolution](../../src/attack_hammer_ov021/hammer_resolution.cpp) and
[progression](../../src/attack_hammer_ov021/hammer_progression.cpp).
Primary and secondary input timers are separate. An exact-zero secondary timer
changes phase before that call's decrement; primary timeout is strictly below
zero. Input equality uses the complete masked button word, not a single-bit test.
Return movement accepts the three established result states and selects their
different durations. Its Q12 distance is rounded by `FX_Sqrt`, then multiplied
and divided by 0x80000 with signed truncation toward zero.

Impact coordinates use a full-width delta for the damage popup and separate
signed-halfword narrowing for view projection. The effect table has 36-byte
records indexed by primary/secondary swing and the observed hit-bonus result.
The third argument forwarded by the update is unused by the impact function.
Typed inline projection reproduces the native conversion order without ASM.

Fresh live checks used private checkpoint-22 and checkpoint-83 state chains,
with automatic button input and timeout variants, no new RAM edits and all
104 original saves unchanged. Final reports are
`build/runtime/eur_high_hammer/high_hammer_{perfect,timeout,save22,save22_timeout}_v3.json`;
the producer is `build/analysis/high_effort_50_to_55/probe_hammer.py`.
The four 2,010-frame replays check 578 complete 28-byte input states and four
calls each to approach, impact and return, plus the immutable 64-byte configs.
They cover susceptible and immune targets, successful primary timing and primary
timeout, movement arguments and integer square roots, view-coordinate results,
effect selection, damage forwarding, status arguments and screen-effect dispatch.
Secondary swings and early inputs remain uncovered; the routes do not
establish coverage of both hit-bonus outcomes.
Damage/RNG internals, helper-owned allocations, GPU transfers and rasterization
are observational. A matching ROM and these routes do not establish full coverage.

When naming traced call targets, resolve an address in the resident and currently
loaded overlay metadata. A global address-to-name map across every attack overlay
can silently label overlay-21 code as overlay-26 code at the same load address.
The final replay corrects that issue and repeats the earlier routes.


## Jump support input and landing effects

[Support update](../../src/attack_jump_ov020/support_update.cpp) owns
`0x020C326C..0x020C34A4` (568 bytes), and
[landing effects](../../src/attack_jump_ov020/landing_effect.cpp) own
`0x020C2EAC..0x020C2EFC` (80 bytes). Both compiled source objects match fully,
with no new ASM. Support timing uses the main participant's formation config:
signed halfwords at +48/+50 bound the main timer inclusively. A masked button
word must equal the expected key; a different key from the main participant's
key marks failure. Phase 2 falls through to the channel-3 completion check in
the same update, even if the vertical-position condition has not changed phase.
Landing effects use the baby variant for formations 2/3 and the normal variant
otherwise; the called shared helper projects and spawns a model effect.

Three fresh 1,420-frame DeSmuME replays cover successful support timing, expiry
without input, and a wrong button within the window. They complete 474 support
updates and six landing-effect calls. The successful route traverses support
phases 0 through 4; effects cover formations 2 and 4. Per-call checks compare the
entire 32-byte support/main records, unchanged 116-byte configuration, input
flags and every direct helper's arguments. Motion-channel activity is derived
from its callback; model completion is observed from the returned model flag.
The original 104 save files remain unchanged.

Private reports: `build/runtime/eur_high_jump_support/high_jump_{perfect,timeout,wrong}_v1.json`.
Producer: `build/analysis/high_effort_50_to_55/probe_jump_support.py`.
The checkpoint is `eur_attack_helpers/ov17_native_start83.dst`, with provenance
in the adjacent JSON: Save 83 entered encounter -32748 via a borrowed 0x11C
command from room 306, script offset 0x2926; 72 script bytes were restored before
the checkpoint. These replays make no further RAM edits. This is a controlled
battle entry, not ordinary story-route coverage. Early input, null support and
other formations remain untested; helper internals and rendered pixels are
observational. The inspected success capture shows the paired jump in battle.


## Field screen-wipe transfers and cleanup

The field's pointer at +0x2BC8 owns a 24-byte screen-wipe record. Pattern modes
0..3 use tile/map buffers; mode 4 uses two 192-halfword circle tables within one
allocation; mode 5 holds eight signed rectangle extents. Flag bit 12 retains
the final mask. Finish deactivates it; a second finish on an inactive retained
mask requests cleanup. Clear releases the appropriate buffers and restores the
display state. It darkens the screen for odd pattern modes or a zero final mask.

Matching sources are [upload](../../src/field/field_screen_wipe_upload.cpp),
[rectangle](../../src/field/field_screen_wipe_rectangle.cpp) and
[cleanup](../../src/field/field_screen_wipe_cleanup.cpp): 1,988 bytes total,
`0x02073C54..0x02074000`, `0x0207431C..0x02074400` and
`0x020744DC..0x02074810`. The 796-byte updater, 220-byte circle-table builder
and 1,344-byte initializer remain native. No new ASM is used.

Window plane selection and the effect-enable bit are distinct operations;
explicit inline helpers preserve their native instruction order. Rectangle
coordinates are shifted before masking to the register's byte fields. The
circle upload toggles the selected buffer bit, writes the first horizontal
extent and streams the remaining halfwords to WIN1H with HBlank DMA 1 or 2.
Only the base of the double buffer is freed.

Fresh live tests use Save 65's initialized field checkpoint and recovered script
parameters: pattern mode 2 from room 0/offset 0x086E; a 60-frame rectangle from
room 15/0x0234; and room 549's retained circle, closing circle and reveal pattern
at 0x2892, 0x28C8 and 0x2BF4. Each decoded command is temporarily replaced and
restored on dispatcher return. Restoration must wait until then: the dispatcher
reads some arguments again after the initializer returns. This is controlled
effect entry, not coverage of those rooms' normal story sequences.

The three 480-frame replays pass 2,880 uploads, 61 rectangle calculations,
five finishes and four clears, with 551 ordered direct MMIO stores checked.
Each target return checks the entire 11,216-byte area, 24-byte effect record,
owner flags and preserved registers. Direct helper arguments and memory fills
are checked; background-address getters, allocation/free internals and DMA
execution remain observational. Active effects were exercised on field screen
0; screen 1 had idle uploads only. Retention and later reinitialization were
observed, but a second finish of an inactive retained mask was not exercised.
Odd pattern modes, sub-screen effects and all clipping boundaries are untested.
Captures show the rectangle and circle masks and a visible final field; they
are visual observations, not an independent rasterization oracle. All 104
original saves are unchanged.

Private producer: `build/analysis/high_effort_50_to_55/probe_field_wipe.py`.
Final reports: `build/runtime/eur_high_field_wipe/high_field_{pattern,rectangle,circle}_v2.json`.
The preserved v1 probe passed the pattern route but failed on the rectangle
because Capstone's `ip` alias was not mapped to DeSmuME's `r12`; all routes were
repeated successfully after correcting the probe. Actual source objects,
the changed area-state caller, the full build, native relink and 107 tests pass.


## Room-owned field effect animations

[Field effect animations](../../src/field/field_effect_animation.cpp) reconstruct
the contiguous 1,160-byte range `0x02074D50..0x020751D8`: room data loading,
request completion, sprite/model starters and active-track queries. No ASM is
used. Two buffer arrays at area +0x22DC follow IDs at +0x2330 and counts at
+0x2358; IDs 0..3 also populate four special pointers at +0x22E4. Requests are
44 bytes. Completion frees their array, while animation buffers remain owned
by the room. A zero total leaves the existing request pointer/count untouched.

The model wrapper resolves four entity IDs, mapping -1 to null, and passes the
embedded four-model array at +0x2B18. Its signed-halfword speed is extended for
the existing full-width starter. Sprite completion checks the command pointer;
model completion checks the track's finished bit. The Field VM caller remains
byte-exact after adopting these declarations.

Live replays: ordinary Save 65 and Save 11 loads cover empty resource sets.
A controlled Save 65 route queues room 359 through the normal room-change
function, then dispatches that room's original sprite command at script offset
0x110A: slot/index 0, position 128/62, speed 256. Both temporary 72-byte decoded
commands are restored at dispatcher return. Entry 4's 888-byte payload and
reported size match the extracted archive; the archive also occurs unchanged
in the golden ROM. Captures show the star effect. This route is not a normal
story entry. The three selected replays cover 7,799 frames, seven loaders,
seven completion polls and one sprite starter. Entire area records, pointer
arrays, caller stores, request contents before submission and helper arguments
are checked; archive queue mutations and allocator/playback internals remain
observational. All 104 original saves are unchanged.

Separate ARM946 execution on copied live RAM passes 27 query, wrapper and poll
boundary cases. Synthetic records and starter/delete stubs verify null/finished
tracks, both slots, signed forwarding, entity sentinels and pending requests.
They do not establish live model playback or heap-release behavior. No model
start appears among the exported reachable field commands. Live coverage does
not include the four special IDs, a populated second resource set or a pending
read poll. Full ROM build, native relink and all 107 tests pass.

Private reports: `build/runtime/eur_high_field_animation/{cold65_v1,cold11_v1,room359_v3,isolated_v1}.json`.
The cold routes use `probe_field_animation_v1.py`; room359_v3 uses
`probe_field_animation.py`. `check_field_animation_isolated.py` uses the preserved
room359_v2 RAM; v2/v3 call records and fixtures are identical. All producers are
under `build/analysis/high_effort_50_to_55/` and are not build dependencies.


## Room placements and entity update order

[Placed entity creation](../../src/field/field_placed_entities.cpp) reconstructs
`0x0207741C..0x02077A44` (1,576 bytes); [list initialization](../../src/field/field_entity_update_order.cpp)
reconstructs `0x0207682C..0x020768F8` (204 bytes). Both match without ASM.
Two placement arrays at area +0x2318 and counts at +0x2350 contain 28-byte
records. The room mask and optional VM variable determine each enabled flag.
Subtype selects the constructor and allocation: 0/1 use 1,440 bytes,
2/3/5/6 use 1,312, 4 uses 1,316, 7 uses 1,328, 8 uses 688 and 9 uses 236.
Do not infer all allocation extents from the shared runtime-entity prefix.

There are 32 placement slots at +0x29D8, followed by list head/tail at
+0x2A58/+0x2A5C. The retained 36-pointer raw view includes these adjacent fields.
Reset writes entity order at +5 and previous/next links at +0x10/+0x14, then
sets the roots and null endpoints. Its native precondition is at least one
entity. The subsequent dependency-based reorder remains unreconstructed.

Ordinary cold loads of Saves 1, 65 and 103 pass 6,699 frames, six creation calls
and 1,925 resets. Across 81 placements, subtypes 0/1/2/3/7/8/9 execute, including
enabled and disabled records; five placements read a VM variable. Lists of
3, 8, 11, 12, 20 and 27 entities are checked. The oracle checks the complete
area, spawn records, constructor arguments, pointer publication and complete
live entity allocations during resets. Allocator results, VM variable reads
and constructor internals are observational; constructor writes are accepted
only inside the newly allocated object. Final captures show normal field scenes.
All 104 original saves remain unchanged.

No live coverage is claimed for subtypes 4/5/6, a populated second set,
allocation failure, unsupported subtypes, single-entity or full 32-slot lists.
A type-2 room executes the forced low mask bits with an already all-ones mask,
so their distinct effect is not tested. Full native relink, golden ROM build
and 107 tests pass. Private reports are
`build/runtime/eur_high_field_spawn/{cold1_v1,cold65_v1,cold103_v1}.json`, produced
by `build/analysis/high_effort_50_to_55/probe_field_spawn.py`.


## Party entity placement

[Party placement](../../src/field/field_party_placement.cpp) reconstructs
`0x020BC3E8..0x020BC684` (668 bytes) without ASM. It initializes the spatial
base, installs the party vtable, clears the 128-byte extension and applies
adult/baby motion, collision snapshots and presentation defaults. The placement
record must be non-null: native collision setup reads it before a later guard.
The auxiliary-count table selects up to six owned 1,360-byte objects, which
are published after their constructor returns. The party allocation is 1,440 bytes.

Ordinary Save 1 and Save 65 loads cover eight calls in 4,466 frames, both age
groups, enabled/disabled babies and 32 auxiliaries. Per-party auxiliary counts
are 1, 3 and 6. The oracle checks the full party allocation, placement record,
direct and virtual call arguments, motion resets and auxiliary publication.
Base construction is observational only within its 1,312-byte prefix, auxiliary
construction only within each new 1,360-byte allocation; allocator internals
are observational. The checked motion reset includes its locomotion helper.
Final captures show normal field scenes. All 104 original saves are unchanged.
The second screen/resource set, zero auxiliaries and allocation failure remain
untested. No RAM fixtures are used.

Actual party/caller objects match; full ROM, native relink and 107 tests pass.
Private reports: `build/runtime/eur_high_party_placement/{cold1_v1,cold65_v2}.json`;
producer: `build/analysis/high_effort_50_to_55/probe_party_placement.py`.
The Save 65 repeat preserves identical call records and final image; its producer
exit was captured explicitly. It is not counted as additional coverage.


## Field entity update passes

[FieldArea_UpdateEntities](../../src/field/field_entity_update.cpp) owns
`0x02075D28..0x020760E4` (956 bytes). It updates room effects when the scene
transition permits, handles pending room changes, then visits the live entity
list in three passes: movement/contact callbacks, completion callbacks and
render-priority updates. Type 9 records skip all three; type 8 records skip the
spatial part of the first pass and the priority pass. Enabled party entities
also update their six auxiliary slots. List successors, the area head and
auxiliary pointers are reloaded after callbacks, as in the original code.

The first-pass virtual slot `0x40` receives the area entity table: party update
`0x020B9FF8` retains that argument and indexes it when following a support entity.
The priority slot `0xA0` accepts the full register value. The area's two-bit field
is forwarded without an extra byte conversion; renderer writes eventually
truncate it. The existing party wrapper's byte parameter remains unchanged,
and its whole compiled unit still matches. This ABI correction removed the
new caller's extra mask; the complete 956-byte function matches without ASM.

Three ordinary routes using Saves 1, 65 and 103 observe 2,743 calls over 7,176
frames. The first call and every 61st subsequent call are fully checked:
46 calls, with 4,448 helper calls, including 400 auxiliary updates and 428
independently modeled support-clearance calls. The oracle checks decisions,
call order, direct/virtual arguments, native-byte guards, caller writes and
callee-saved registers. Full area, system, party-manager and current entity
allocations are compared at caller boundaries. Arbitrary helper internals are
observational only within those enumerated scene records; effect/window/layer
helpers may refresh only the area record. This does not verify those helpers'
internal behavior or graphics output.

The routes cover transition phase 0, entity types 0/1/2/3/7/8/9 and party mode 6.
Room-departure and pending-party-transition branches, other transition phases,
types 4/5/6, suppressed entities and the partner vertical-sync store remain
uncovered by the sampled calls. No RAM fixtures are used. Final captures were
inspected: Saves 65 and 103 show field scenes, while Save 1 reaches the save menu
through ordinary movement and A input. All 104 original saves remain unchanged.

Full build, original ROM hash, native relink and 107 tests pass. Final formatted
source objects and the affected party wrapper compare exactly. Private evidence:
`build/runtime/eur_high_field_update/{cold65_v2,cold1_v2,cold103_v2}.json`, produced
by `build/analysis/high_effort_50_to_55/probe_field_update.py`. The initial
`cold65_v1` failed in the probe because a branch target was mistaken for a helper
return. The correction pairs returns with the actual call site and stack pointer;
its successful rerun is retained separately.


## Field frame snapshots and party forwarding

`FieldArea_BeginFrame` (`0x020760E4..0x020761B4`, 208 bytes) snapshots the area's
phase and, in phase 71, camera/offset fields before clearing this frame's offsets.
It calls each non-null, non-suppressed entity through virtual slot 8.
`FieldArea_ResetEntityRenderOrder` (`0x02076390..0x0207645C`, 204 bytes) restores
placement order using the separate render-list links at entity `+0x18/+0x1C`,
order byte `+6` and area head/tail `+0x2A60/+0x2A64`. It requires at least one
entity, just like the update-list reset. Raw header views remain available.

The four party wrappers in [field_party_frame.cpp](../../src/field/field_party_frame.cpp)
cover `0x020BB56C..0x020BB69C` (304 bytes). They apply the spatial base operation
first, then forward frame initialization, navigation pointers, renderer release
or renderer allocation to each non-null auxiliary slot. Virtual slot `0x84`
receives the navigation surface and resource pointers; its base implementation
stores them at entity `+0x2B0/+0x2B4`. Slots are re-read after callbacks.
All six functions matched on the first compiled draft, without ASM.

Two automatic routes from Saves 65 and 1 run for 5,466 frames. Save 65 enters
and leaves pause; Save 1 finishes in the field, without evidence of a pause
round-trip. Both final captures were inspected. Of 3,332 area-frame and 5,440
party-frame calls, the oracle fully checks 56 and 178 respectively: first/every
61st outer entry, plus nested party calls in checked area frames. It also checks
all six render-list resets, twelve allocations and twelve navigation setups.
Reset lengths are 8, 11, 20 and 27; navigation setup forwards to 48 auxiliaries.
Area phases include 0, 9, 12, 13, 14, 17, 50, 60 and 71. Null/suppressed area
slots were not observed, although empty auxiliary slots were exercised.

Full area/entity/auxiliary records are checked at caller boundaries. Frame
snapshots, list links, callback order/arguments and navigation pointer writes
are independently modeled. Other helper effects are observed only within the
receiving object: spatial base helpers within 1,312 bytes, party calls within
1,440 bytes and their owned 1,360-byte auxiliaries. Renderer heap internals and
graphics are not independently verified. All 104 original saves are unchanged.

The separate release wrapper did not execute live. Twenty isolated ARM946 cases
on copied Save 65 RAM check all four wrappers with five auxiliary-presence masks,
including empty and all-six slots. They compare all 4 MiB of main RAM, scratch
outside the exact native stack frame, stack restoration and preserved registers.
The navigation setter executes natively with synthetic pointer values; frame,
allocation and release helpers are ABI-only stubs that clobber caller-saved
registers. These cases establish forwarding behavior, not actual renderer release,
heap lifetime or live coverage of the release wrapper.

Full build/native relink, original ROM hash and 107 tests pass. Final source
objects, including the neighboring entity updater, compare exactly. Private
reports are `build/runtime/eur_high_field_frame/{pause65_v2,pause1_v2,isolated_wrappers_v1}.json`;
producers are `probe_field_frame.py` and `check_field_party_wrappers_isolated.py`
in `build/analysis/high_effort_50_to_55/`. The earlier `pause65_v1` failed only its
final all-functions coverage assertion because release was absent; that failed
report and producer are preserved, and the successful replay states the gap.


## Field screen visibility

[Entity visibility](../../src/field/field_entity_visibility.cpp) reconstructs
`0x020A695C..0x020A6AB4` (344 bytes); the [area pass](../../src/field/field_visibility_pass.cpp)
adds `0x0207ED28..0x0207EDC0` (152 bytes). Both match without ASM on the first
compiled draft. Virtual slot `0x34` returns an integer screen-boundary predicate:
the planar vtable at `0x020C1008` points to `FieldEntity2D_IsOutsideScreen`, while
the party vtable at `0x020C1244` points to the spatial test at `0x020A79DC`.
The latter also uses the greater of screen Y and the signed value at `+0x3D0`
for the lower boundary. State bits 10/11 at entity `+0x184` are the current and
previous outside-screen result; shared declarations and consumers now name them.
The history shifts even when property bit 0 suppresses the predicate call.

Automatic renderer toggles require animation support, a renderer, no blink mode
and property bit 7 clear. State bit 9 suppresses hiding but does not suppress
showing. Showing can reset renderer state through the resident helper's virtual
callback; this batch does not reconstruct that helper. The area pass runs only
in phase 71 with no scene transition and skips type-9 markers. Its entity count
and entries are read again as the loop advances.

Two ordinary routes from Saves 65 and 1 run for 5,210 frames. Deterministic
first/every-61st sampling checks 720 of 43,853 entity calls and 45 of 2,714 area
calls. The oracle checks full entity allocations and area/system records,
callback order/arguments, history bits and both known boundary predicates.
It exercises types 0/1/2/3/7/8, enabled/disabled predicates, inside/outside results
and marker skipping. All observed predicates are the two modeled implementations.
The sampled calls do not toggle a renderer, and all sampled area calls use phase
71 with transition zero. Final captures show field scenes; no pause round-trip
is established. All 104 original saves remain unchanged; no live RAM fixtures
are used. Renderer checks cover the 312-byte base prefix, not a full allocation;
graphics are not independently checked.

Nineteen separate ARM946 cases use copied RAM with synthetic records and stack.
They cover hide/show, already-correct visibility, disabled animation/predicate,
null renderer, blink suppression, the asymmetric bit-9 guard, explicit visibility,
history, null system, active transition, inactive area, markers and an empty list.
All 4 MiB of RAM, scratch outside the exact native stack frame, SP and r4-r11 are
checked. Predicate, toggle and area-child helpers are ABI stubs; these cases
verify caller decisions and writes, not actual renderer restart, graphics or live
coverage of those branches.

Full build, original ROM hash, native relink and 107 tests pass. Final source
objects and the affected planar/update units match. Private reports:
`build/runtime/eur_high_field_visibility/{cold65_v1,pause1_v1,isolated_v1}.json`.
Producers are `probe_field_visibility.py` and `check_field_visibility_isolated.py`
in `build/analysis/high_effort_50_to_55/`.


## Spatial directional motion

[FieldEntity3D_ResolveDirectionalMotion](../../src/field/field_directional_motion.cpp)
reconstructs `0x020A8300..0x020A8478` (376 bytes), matching on the first compiled
draft without ASM. The unused entity parameter is retained. Direction is a
full-width integer; elevation is read as an unsigned halfword from the stack.
Output pointers are independently optional. The Bros.-Ball caller now uses the
shared prototype, retaining its explicit direction conversion.

Planar mode returns sine/cosine-scaled X/Y and zero Z. Spatial mode additionally
scales X/Y by elevation cosine, with division toward zero at each stage. Z is
the raw speed-times-elevation-sine product: it has no division by 4096. Preserve
this asymmetry and the X/Y/Z store order, including aliased outputs. The tested
speed range has magnitude at most 24,576 and keeps signed products in range;
no broader source-language overflow guarantee is established.

A 90-frame DeSmuME run starts from the ordinary Save 65 field capture. A guarded
Field VM opcode `0xC5` fixture, direction zero, invokes the actual Bros.-Ball
manager/wrapper and the new function once. The decoded command and script cursor
are restored before the wrapper continues, and replay of the original command is
checked. The target's entire 4 MiB main RAM and 16 KiB DTCM are compared, excluding
only its 16-byte native stack frame; output words and preserved registers are
independently checked. This exercises spatial mode with X/Y outputs, null Z and
zero elevation, producing X=0 and Y=-24576. It is controlled script execution,
not evidence of ordinary story entry into a launcher.

After the run, a private pre-fixture emulator state restores all main RAM and
DTCM exactly, including induced gameplay changes. The restored image is identical
to the pre-fixture capture; the fixture and restored images were inspected.
All 104 original saves remain unchanged. There is no independent graphics oracle.

Thirty-four isolated ARM946 cases execute the entire function and original angle
table without stubs: both modes, all output-presence masks, aliased outputs,
cardinal/fractional angles and positive/negative/zero speeds. Full main RAM,
scratch outside the exact stack frame, SP and r4-r11 are checked. These cases
supplement the live fixture; they do not establish gameplay or renderer coverage.
Actual source and all 30 functions in the affected party-transition unit match;
full ROM/native relink and 107 tests pass. Private reports are
`build/runtime/eur_high_field_directional/{script65_v1,isolated_v1}.json`, produced
by `probe_field_directional.py` and `check_field_directional_isolated.py` in
`build/analysis/high_effort_50_to_55/`.


## Entity script property getters

The [planar getter](../../src/field/field_planar_properties.cpp) reconstructs
`0x020A3640..0x020A3854` (532 bytes), and the [spatial getter](../../src/field/field_spatial_properties.cpp)
adds `0x020A8478..0x020A86A4` (556 bytes), both without ASM. The shared
[property enum](../../include/game/field_entity_properties.h) names IDs 0..23.
The planar draft matched immediately; the spatial draft matched after replacing
a switch-based type test with the native bounded `0x403` mask and explicit
result initialization. Types 0/1/10 classify party members and auxiliaries.
The meaning of the related-object pointer at `+0x500`, flag 7 at `+0x38C` and
mask `+0x3A8` remains neutral in the interface.

Positions divide signed Q12 values toward zero; screen coordinates and renderer
animation fields retain signed halfword values. The speed query prefers an active
linear controller, then an active orbit, then the entity's free movement speed.
Renderer properties require a live renderer. The spatial extension adds vertical
state, contact bits, related-object classification, Z, the signed support index
and the first set bit of `+0x3A8` (or -1 for an empty mask). Unknown IDs return zero.
Properties 4/5 delegate to the existing base interaction helper.

Two ordinary Save 65/Save 1 routes run for 4,903 frames and check every
executed getter: 8,378 planar and 8,377 spatial calls.
There are 8,365 nested planar delegations, so these are not distinct script
requests. Live IDs are 1/2/10/11/12/16, on entity types 0/1/2/7/8. Full receiving
entity records remain unchanged; queried renderer reads are checked over their
312-byte base prefix. Returned values and helper targets/arguments are modeled
independently. Save 65 ends in the field, Save 1 in the save menu; both captures
were inspected. No live RAM fixtures are used; all 104 original saves are unchanged.

A separate 103-case ARM946 run executes both full getters and the original base
helper without stubs. It covers all 24 IDs and invalid IDs, controller precedence,
interaction states, vertical/contact flags, null and varied related-object types,
negative coordinates and first-set-bit masks including zero and bit 31. All
4 MiB of main RAM stay unchanged; scratch outside the exact 8/16-byte call-chain
frame, SP and r4-r11 are checked. This supplies branch evidence rather than live
script coverage. No independent graphics oracle is claimed.

Final source objects, full ROM/native relink and 107 tests pass. The first gate
failed only the source-comment convention; opening comments were added and the
complete gate passed. Private reports:
`build/runtime/eur_high_field_properties/{cold65_v1,cold1_v1,isolated_v2}.json`.
Producers in `build/analysis/high_effort_50_to_55/` are `probe_field_properties.py`
and `check_field_properties_isolated.py`, with `field_property_oracle.py`.
The earlier isolated v1 also passed, but allowed an unnecessarily large stack
exclusion for the leaf base helper; v2 checks the tighter native frame.


## Party contact hints and automatic bounds

[Contact hints](../../src/field/field_party_context.cpp) select the active
member's hint from locomotion, facing, contact mask and the related entity's
contact mode, then clear the inactive group's hint. Scene transitions suppress
both changes. An absent or non-field area preserves the active hint.
[Automatic bounds](../../src/field/field_party_boundary.cpp) gate the leader's
paired-bounds lookup on scene/area transitions, a manager flag, presentation
readiness and brightness. Exactly -4096 and +4096 suppress the lookup; nearby
values do not. A successful record selects one of two transition helpers or
returns without dispatch. The existing lookup updates current/previous signed
byte indices. Both new functions matched their first compiled drafts without ASM.

An ordinary Save 65 load and movement route checks every target call: 693 bounds
checks, 692 hint updates and 490 real paired-bounds lookups across 2,477 frames.
All live lookups return -1 and all computed hints are zero. The route covers
area, transition and fade guards; it does not cover successful exits or nonzero
hints. Full manager (16,764 bytes), system (952), area (11,216), member (1,440)
and scanned bounds records are checked, including independently derived lookup
returns and index writes. The final capture shows the field. No live RAM
fixtures are used; all 104 original saves remain unchanged.

Another 220 isolated ARM946 cases on copied RAM cover both groups, all eight
contact modes/facings, null and suppressed inputs, both fade limits and adjacent
values, successful bounds in all four cardinal directions, all dispatch modes
and a second-record hit. They execute both complete native functions and the
real lookup. The two transition helpers are explicit no-op ABI stubs: their
arguments are checked, their internals are not. Full main RAM, scratch outside
the exact call-chain frame, restored SP and callee-saved registers are checked.
These synthetic cases do not establish live exit or graphics coverage.

Actual source objects, the action-update caller and neighboring model-update
wrapper compare exactly. Full ROM/native relink checks and 107 tests pass.
Private evidence: `build/runtime/eur_high_party_contact/{cold65_v1,isolated_v1}.json`;
producers `probe_party_contact.py`, `check_party_contact_isolated.py` and shared
`party_contact_oracle.py` under `build/analysis/high_effort_50_to_55/`.


## Field physical and scripted input

[FieldArea_ReadInput](../../src/field/field_input_read.cpp) combines physical
held/pressed input with the area's scripted input. It intersects the caller mask
with both common input masks, rotates the remaining physical directional bits,
then adds the script's held/pressed bits. A byte parameter selects whether to
apply the two script masks. Physical and script inputs are read before either
output halfword is stored; identical or overlapping input/output records retain
this order. The source matched its first compiled draft without ASM.

An ordinary Save 65 load and left/right movement checks all 1,130 calls over
2,477 frames, 565 with script masking and 565 without. Physical input is nonzero
in 70 calls; this route has no scripted input and no directional rotation. Both
real helper arguments and returns, all area/input records, output stores and
callee-saved registers are checked. The final capture shows the field and all
104 original saves are unchanged. There are no live RAM fixtures.

Another 136 isolated ARM946 cases execute the complete reader, common-mask
helper and directional-remapping helper with no stubs. Synthetic inputs cover
all four rotations and directional combinations, both script-mask choices,
zero/full/high masks and aligned output aliases, including reversed physical
and scripted input words. Full main RAM, stack memory outside the exact 40-byte
call-chain frame, SP and callee-saved registers are checked. These are separate
from gameplay coverage and do not provide an independent graphics oracle.

The actual reader, common-mask and shared-header area-state objects compare
exactly; full ROM/native relink checks and 107 tests pass. Private reports:
`build/runtime/eur_high_field_input/{cold65_v1,isolated_v2}.json`; producers
`probe_field_input.py` and `check_field_input_isolated.py` under
`build/analysis/high_effort_50_to_55/`. The earlier isolated v1 included an
unaligned output fixture; v2 removes it to keep fixtures within the halfword
pointer contract. The remapping helper remains native code and is not counted
as new C/C++ progress here.


## Item collection sound selection

[Item collection sounds](../../src/field/field_item_sound.cpp) use byte 8 of a
valid tagged item record to select one of three sound classes. Each class has
a separate entity/placement sound and delay. Coins (`65535`) select the first
class without a record lookup. Other IDs are narrowed to 16 bits for lookup,
while the coin comparison uses the full incoming word. The native routine
requires a valid item and source value; it does not handle arbitrary tags or
out-of-range table indices. Typed record-prefix helpers preserve each table's
20/28-byte stride. Both the first draft and named-prefix cleanup match without ASM.

A controlled Save 83 replay starts from the recorded pre-pickup state in room
459. A guarded decoded D0 command positions the parties at a block using its
live placement record; the 72 command bytes are restored before the real room
wrapper. D0 replaces the room scripts, so no original-script replay is claimed.
An ordinary A press collects item `0x1003` at frame 660: class 1 selects sound
342, delay 6 and the first free audio slot. The 811-frame run checks the whole
selector and real delay/queue helpers, including arguments and independently
modeled queue writes. Full 4 MiB main RAM and 16 KiB DTCM agree outside the exact
24-byte call-chain stack frame. The pre-fixture emulator state is reloaded at
completion; main RAM/DTCM and before/restored screenshots are identical. Field
captures were inspected and all 104 original saves are unchanged.

Another 212 isolated ARM946 cases cover all 99 native inventory records plus
coins with both source values, each free queue slot, a full queue, full-word IDs
with valid low halves, and explicit copied-table fixtures for zero/signed sound
IDs. The selector and both real helpers execute without stubs. Full main RAM,
scratch outside the exact 8/24-byte frame, SP and callee-saved registers are
checked. All native delays are nonzero; actual playback/IRQ behavior and audible
output are not independently verified by these checks.

Actual selector and all four collection-caller functions match; full ROM/native
relink checks and 107 tests pass. Private evidence lives in
`build/runtime/eur_high_field_item_sound/{block83_v2,isolated_v1}.json`; producers
`probe_field_item_sound.py` and `check_field_item_sound_isolated.py` are under
`build/analysis/high_effort_50_to_55/`. The first replay failed before pickup
because its oracle treated D0's synchronization argument as a fade argument.
The VM explicitly supplies fade zero and forwards synchronization separately;
the corrected replay checks the complete wrapper argument list. The failed run
and its producer are preserved, and its pre-fixture state was restored too.


## Shop equipped descriptions and attached sprites

`ShopEquipment_UpdateDescription` (`0x020756B8`, 524 bytes) positions the
description marker for the selected party member and redraws the equipped-item
description when its cached item changes. The parent controller and latch delay
that change during panel movement. `ShopPanel_DrawAttachedSprite` (`0x020758C4`,
292 bytes) combines the parent's Q12 position and signed sixteenth-pixel offsets,
adds the child's pixel offset and the native 240-pixel sub-screen offset, selects
blending, then submits the sprite to draw list 5 or 59. Removal is deferred by a
flag; the two callbacks have different phase thresholds.

The shared task and parent views are 72 bytes. ResourceA owns a 336-byte model
slot in the pool at `0x0206A3D8`; ResourceB owns a 64-byte sprite slot in the pool
at `0x0206AA18`. The workspace view is only a prefix of the full 2,492-byte record.
Its selected member at `+0x8D` has signed comparisons but unsigned save-array
indexing; phase `+0x8B7` is unsigned. The text call uses description kind 1,
sub-screen OBJ offset 10,880 and a seven-column, two-row output of 3,584 bytes.
Whole functions match without inline assembly. MW emits these functions in
reverse source order; the public unit preserves their native address order.

Private `eur_high_shop_equipment/badges65_v2.json` checks 3,352 sprite calls and
395 description calls across 2,121 frames. Save 65 uses the established guarded
decoded shop-command fixture, restored at the request wrapper. Ordinary inputs
select Badges, purchase a badge, open member selection and return to the field.
Coverage includes both screen offsets, both blend states, members 0, 1 and 3,
one countdown, three text redraws, 377 cache hits and 14 latched updates. Full
task, parent, resource, workspace and live-save records are checked at call
boundaries, as are helper arguments and draw-pool/list writes. Animation output
is observed only inside its 336-byte receiver; existing palette nodes and roots
are checked unchanged. Text output is observed in the 2,096-byte renderer and
3,584-byte OBJ range, with surrounding sub OBJ memory checked unchanged.

The separate `isolated_v1.json` checks 124 copied-RAM cases, including all four
members, clothing/badge IDs, cache/latch combinations, countdowns, phase values
0/1/2/255, blend values and signed coordinates. Both callbacks and the real
accessors, marking and draw-pool helpers execute. Animation and text use explicit
ABI stubs. Checks cover full main RAM, stack bounds, SP and callee-saved registers.
These cases do not establish live removal or renderer coverage. Rasterization,
purchase logic and task release are not independently verified by this probe.
Screenshots show the description and visible field return; all 104 source saves
retain their experiment-baseline hashes. Earlier clothing routes missed the
description callback; the first badge run stopped on a probe register-alias error.
Those failed reports and the original producer are retained separately.

+
+### Shop description strips
+
+The adjacent `ShopEquipment_DrawDescriptionStrip` (`0x02075628`, 144 bytes)
+follows the description owner's Q12 position after its countdown. Its parent
+pointer refers to that 72-byte owner, whose own parent is the 72-byte panel
+controller. The strip keeps its position while the controller is active and the
+owner is latched; otherwise it uses the owner's X and `166 * 4096 - owner.y`.
+It submits to draw list 20 or marks itself for deferred removal on a nonzero phase.
+
+The extended unit's three functions are exact. Private
+`eur_high_shop_strip/badges65_v1.json` repeats the preceding inputs and checks
+5,530 additional strip calls: 14 countdowns, 5,320 following updates and 196
+latched draws. Full strip, owner and controller records are checked separately,
+along with the sprite, workspace, save and draw-list changes. The same producer
+also rechecks the previous 3,747 callbacks. Its `isolated_v1.json` passes 156 cases,
+including 32 new strip countdown/latch/phase cases; the strip and real marking,
+accessor and draw helpers execute without stubs. The earlier animation/text stubs
+apply only to the other callback cases. All original saves remain unchanged.
+
+
+## Game boot entry
+
+The resident `main` (`0x02004B20..0x02004D40`, 544 bytes) is reconstructed in
+[game_boot.c](../../src/game/game_boot.c). It initializes graphics and VBlank,
+RTC and both task schedulers, clears the two OAM buffers and input, sets repeat
+delays and frame-timing hold durations, decodes console-type flags, then enters
+the game loop. The native master-interrupt sequence reads and writes a halfword
+at `0x04000208`; preserve that access width and the preceding read. The combined
+workspace spans `0x0206032C..0x02060B6C` (2,112 bytes), including the existing input,
+OAM and timing aliases. Unknown console-type fields retain bit-based names.
+
+The first C draft and actual public object match completely without assembly.
+Private `eur_high_game_boot/cold65_v1.json` checks a real cold start using Save 65,
+without a savestate or RAM fixture: all 14 ordered calls and their arguments,
+three hardware stores, seven RAM stores, full workspace, both DMA OAM fills and
+input clearing. The observed console flags are `0x82000001`; reaching the real
+nonreturning game loop completes the caller check. Ordinary title/load/navigation
+inputs continue to a visible field screen across 2,477 frames. No VBlank callback
+occurred during this startup check; the probe's bounded IRQ-timing observation
+path was not exercised. Other initializer internals and rasterization remain
+outside its independent expectations.
+
+The separate `isolated_v1.json` passes 256 combinations of high, middle and low
+console flags, including defaults and unrelated bits, with power/display patterns.
+The complete native entry executes; all initialization calls are explicit ABI
+stubs, modeling only DMA fills, input clear and the console result. These checks
+cover full main RAM, 8 KiB of modeled MMIO, ordered hardware writes, the eight-byte
+stack frame and callee-saved registers. A synthetic return from the game-loop stub
+tests the entry's epilogue; it does not claim that real gameplay returns there.
+All 104 original save hashes remain unchanged. The full ROM/native relink and
+107 tests pass. Resident coverage changes; overlay coverage is unchanged.
+

## Pause list selection sprite creation

[PauseList_CreateSelectionSprites](../../src/scene_menu_ov007/pause_list_sprite_create.cpp)
at `0x0207EA4C..0x0207EE84` creates a cursor and selected-row marker for nonempty
lists. Lists longer than nine rows also receive two scroll arrows. Item kinds
0/1 can create a heading unless the caller suppresses it or the workspace's
`+0x11F` selector is zero. The selector values 1/2/3 choose animations 0/11/10;
other nonzero values still create the heading but skip that animation call.
The function preserves repeated list-kind queries and the lower arrow's flag
store before its animation call. The first C++ draft matches all 1,080 bytes.

All tasks occupy 72-byte pool slots. Arrow positions use signed halfword scroll
origins and Q12 task coordinates; task offset 16 links to the page controller.
Heading offset 40 stores one member byte, followed by Q12 x/y, target y and step.
The creator reads the selected-menu index as signed and uses the existing member
mapping table. It attaches 336-byte ResourceA models and uses assets 75/78.

Three ordinary-input routes from checkpoint 65 cover Clothing, Items, Key Items
and Bros. Items over 3,524 frames. Four calls to the new creator and one nested
heading-arrow creation are checked, including lists with 18/13/2/7 entries,
both heading-suppression values, both scroll-arrow outcomes and heading selector
1. The oracle independently checks helper order/arguments, query results,
15 task/model allocations and attachments, task-group links, caller fields,
and full task/model/workspace/party/save/display records. Model initialization
and animation are observed only within each 336-byte receiver. Thirty external
texture/palette list insertions are derived from validated neighboring nodes
and roots. All routes return visibly to the field with full overlay-0 guards;
no RAM fixtures are used in these replays. Later cleanup and rasterization are
outside this creator oracle's independent coverage.

The separate 300-case copied-RAM check combines kinds 0..4, counts 0/1/9/10/255,
both suppression values and heading selectors 0/1/2/3/4/255, varying all five
selected-menu indices and signed scroll coordinates. It executes the complete
native creator, queries, asset lookup and task/model pool helpers. Explicit
initialization/animation stubs change flag bits and clobber caller-saved registers,
checking store order across callbacks; the nested heading-arrow creator is an
ABI stub here and is checked separately in the live route. Expected effects are
compared against all 4 MiB of main RAM, bounded scratch memory, SP and r4-r11.
These fixtures do not establish live behavior for the synthetic combinations.

Private reports are `eur_high_pause_list_sprite_create/clothing65_v3.json`,
`items65_v4.json`, `bros65_v4.json` and `isolated_v2.json`. Producers and failed
versions are preserved under `build/analysis/high_effort_50_to_55/`. Initial
probes corrected texture-list link offsets and recognition of a field function
overlapping the watched pause address; the first item route reopened the pause
menu after exiting. The first isolated guard confused an interior instruction
with a function start from another overlay. Failed runs remain separate from
the passing reports. Actual source objects, the golden ROM, native relink and
107 tests pass; all 104 original saves retain their experiment-baseline hashes.


## Pause item heading and scroll-arrow updates

`PauseList_UpdateItemHeading` (`0x0207EE84`, 664 bytes) slides the item-list
heading in, delays and animates selection changes, publishes its position for
the attached arrows, then marks its task for removal after sliding out.
The close state falls through to its first movement immediately. Member lookup
uses the signed menu index and compares the task's signed member byte with an
unsigned mapping-table value. The 72-byte task reuses the equipment-heading
layout; positions and steps are Q12.

`PauseList_UpdateScrollArrow` (`0x0207F388`, 336 bytes) positions either scroll
arrow, queries the list edge and selected row, and chooses animations 1/2 or 7/8.
At an unselected edge it moves the model to (-128, -128) before submitting it to
draw list 8. Both functions attach to existing contiguous source units. Their
full objects match without assembly. The small inlined position setter already
used in `menu_equipment_motion.cpp` preserves the native evaluation order of
both Q12 arguments. This resolves the new heading's eight-instruction mismatch
and the older scroll-arrow draft; the original drafts remain private.

Two ordinary checkpoint-65 routes check 366 heading updates and 1,890 scroll-arrow
updates across 2,674 frames. They cover entry, idle, a complete delayed selection
change, close and removal marking, plus all six combinations of arrow direction
and scroll/selected-edge/hidden-edge behavior. The oracle derives full task and
model changes, workspace position publication, both query results and 2,255
draw-list insertions. Scroll-arrow animation calls use frame -1; their output is observed
only within the 336-byte model, with surrounding texture/palette lists checked.
The previous creator checks remain active: three outer creations and one nested
heading-arrow creation. Marking does not prove later resource release, and this
probe does not independently verify rasterization.

All 30 screenshot hashes equal the previously inspected routes, including both
visible field returns. An additional 348 copied-RAM cases comprise 192 heading
and 156 arrow cases: threshold equality and overshoot, delays, signed member
comparison, forced close, inactive phases, negative Q12 rounding, both directions
and short/long list states. The full native callbacks and query/mark/draw helpers
execute; animation alone uses an explicit ABI stub. Checks compare all main RAM,
scratch outside a 64-byte frame envelope, SP and callee-saved registers. These
synthetic combinations are isolated evidence, separate from the live routes.

Private reports are `eur_high_pause_heading_arrows/clothing65_v1.json`,
`items65_v1.json` and `isolated_v1.json`; producer hashes and captures are checked.
The full ROM/native rebuild and 107 tests pass, all five functions in the two
extended source units compare exactly, and all 104 source saves are unchanged.


## Pause status numbers

[pause_status_number.cpp](../../src/scene_menu_ov007/pause_status_number.cpp)
reconstructs `PauseStatus_UpdateNumber` (`0x02078AC0`, 300 bytes). Its first C++
draft matches using the shared `SavePartyMember` layout. The 72-byte task stores
sprite index/member/stat as bytes at +40/+41/+42; digit count, cached value,
divisor and Q12 coordinates occupy +44 through +60. The callback selects level,
current/max HP, power, defense, speed, stache, experience or next-level threshold.
Unknown stat IDs preserve the cached value. Level/experience share a word, and
the threshold excludes its neighboring clothing byte. The sprite-pointer table
at `0x02090900` indexes a contiguous 48-element array of 64-byte sprites; live
checks validate each selected pointer against `WORK + 0x310` and its stride.

An ordinary checkpoint-65 status route checks all four members and all nine
stats: 5,364 callbacks across 1,340 frames. Each nested decimal-rendering call
is independently checked too, including its ten arguments, 38,144 signed
division results/remainders, 13,890 submitted digits and 5,182 suppressed leading
digits. The oracle derives tile, palette, shape and coordinate writes plus draw
pool/list insertion. It checks full tasks, the complete 3,072-byte sprite array,
90,600-byte pause workspace, 4,428-byte party object, 1,380-byte save context
and 176-byte display context at the watched boundaries. This establishes draw
submission; it does not independently model rasterization or later task release.

All 25 screenshots equal the prior status-switch route. The four member displays
and visible field return were also inspected directly. Another 176 isolated
copied-RAM cases test all members, the nine selectors plus defaults 9/255, distinct
halfword values, maximum and high-bit 24-bit fields, neighboring packed bytes
and preservation of the cached value. These execute the complete native callback
with an explicit numeric-renderer ABI stub. All ten forwarded arguments, all
main RAM, the 32-byte stack envelope, SP and callee-saved registers are checked.
These synthetic values do not establish live renderer support for every value.

Private evidence: `eur_high_pause_status_number/status65_v1.json` and
`isolated_v1.json`, with producer/input/capture hashes validated. The actual
compiled source object matches all 300 bytes, the full ROM and native relink
remain exact, all 107 tests pass and all 104 original saves are unchanged.


## Pause key-item model switching

[pause_key_item_model.cpp](../../src/scene_menu_ov007/pause_key_item_model.cpp)
reconstructs `PauseKeyItem_UpdateModel` (`0x02073754`, 576 bytes). The callback
uses a 72-byte task, phase +32, cached item +40 and ResourceA owner fields +24/+28.
On selection change it releases the current model, drops asset slot 84, queues
the new archive-group-3 asset and waits for its key to leave the request queue.
It attaches the loaded model only if the selection still matches, then resets
the phase. A hidden list waits for the read before closing archive table 3 and
marking the task. The native code retains the model pointer across release and
can still submit that pooled slot during the same callback; this order is kept.

Key-item records have ten-byte stride. Their asset/animation halfwords and signed
coordinate bytes use existing interior aliases. Modeling that stride removes
the draft's extra multiply/shift sequence. The established inline position setter
uses full-width pixel sums until the final halfword stores, matching the native
evaluation schedule without assembly. All 576 compiled bytes match.

An ordinary checkpoint-65 route checks 663 callbacks across 1,404 frames, switching
between Beans and Toadbert's Drawing and back. It covers three load requests,
three waiting and three ready checks, three model attachments and 659 draw-list
insertions. ResourceA allocation/return links, asset-slot writes, queue-key
readiness, caller fields and coordinates are independently derived. The oracle
also verifies six texture/palette list insertions and four removals. Full task,
model, pause workspace, party, save, display and archive records are checked at
the watched boundaries. Model initialization/animation/finish internals are
observed within their 336-byte receiver; the archive loader's output is observed
within the 13,148-byte archive. Heap and resource-loader internals are outside
this focused check. Pool return is distinct from final task-group cleanup.

The first replay exposed a missing oracle rule for palette-list unlinking;
the second completed the callbacks but incorrectly required the hidden-list
close branch on an exit that instead uses group cleanup. Both failed reports
are retained. The corrected `keys65_v3` passes, ends visibly in the field and
has 15 validated captures. Its seven common-prefix captures equal the prior
item-list route. Both displayed key objects and the field return were inspected.

Another 64 copied-RAM cases execute the complete callback with explicit helper
contract stubs. They cover hidden-list wait/mark, changed selection during loading,
absent/existing models, inactive phases and signed coordinate overflow. The stubs
check ordered arguments, mutate initialization/animation flags and clobber volatile
registers. All main RAM, scratch outside the 24-byte frame, SP and callee-saved
registers are checked. These cases do not execute loader, cleanup or rendering
internals and do not establish ordinary gameplay coverage of those branches.

Private evidence is under `eur_high_pause_key_item/`: `keys65_v3.json` and
`isolated_v1.json`, with producer/input/capture hashes verified. Full ROM/native
checks and all 107 tests pass; all 104 original saves retain their baseline hashes.


## Shop item-list selection

[shop_item_panel_navigation.cpp](../../src/shop_ov009/shop_item_panel_navigation.cpp)
reconstructs `ShopItemPanel_MoveSelection` (`0x02070334`, 440 bytes). It moves the
signed-byte selection, clamps or wraps at list boundaries, scrolls the visible
window when needed, plays the cursor sound when the logical row changes and
refreshes help text when the item ID changes. Duplicate IDs can therefore change
the row and sound without redrawing the description. The delta and wrap arguments
are full words; the selection store truncates to a byte. The initial item lookup
and modulo precede the count check, so the routine requires a nonempty list.
Keeping the final item as `u16` matches the helper return/argument contract and
removes an unnecessary extension. The complete actual linked object is exact;
no assembly is used.

Checkpoint 65 enters the shop through a guarded decoded scene-command fixture,
restored before the wrapper resumes. Ordinary inputs then browse clothing and
badges without buying. The final `wrap65_v3` route checks 1,397 calls in 2,408
frames: 36 sound/help changes, eight ordinary scroll steps, three scroll wraps,
and short-list wrapping and clamping. It checks helper order, arguments, query
and signed-division results, and independently derives selection and ring/window
changes. Full buying-panel allocation (936 bytes), inline items, shop workspace
(2,492 bytes), save (1,380 bytes) and owner roots are checked at each watched
boundary. Shop roots and vtables establish panel ownership.

Scroll-task allocation, help-text rendering and audio internals are outside this
focused state oracle. Captures show the clothing/badge lists and visible field
return; they are observations of rendering. All 33 final-route captures have
verified hashes/dimensions. The earlier `buy65_v1` route entered confirmation
too soon and covered only unchanged selection; it passed its limited checks.
`buy_sell65_v2` actually browsed two buying categories, despite its provisional
name: 1,160 checked calls and eight scroll steps. Its six common-prefix captures
equal the final route. No selling-panel coverage is claimed.

Another 760 isolated ARM946 cases execute the complete native function on copied
RAM with explicit helper-contract stubs. They cover small/equal/large row counts,
both list ends and wrap modes, ring positions, repeated IDs and signed-byte
truncation of large deltas. Whole main RAM, scratch outside the 24-byte frame,
SP and callee-saved registers are checked. These cases do not execute rendering,
audio or task-allocation internals. Empty-list calls are excluded because the
native pre-check lookups do not support them.

Private evidence: `eur_high_shop_item_move/{wrap65_v3,isolated_v1}.json`, with
the earlier route reports retained. Full source build, golden ROM, native relink
and 107 tests pass; all 104 original saves retain their baseline hashes.


## Shop buying-list equipment markers

[shop_buy_marker.cpp](../../src/shop_ov009/shop_buy_marker.cpp) reconstructs
`ShopBuyMarker_Update` (`0x02070AD0`, 556 bytes). Its first draft matches completely,
using the established save-member layout and selling-marker logic. The callback
finds the equipped clothing or badge within the visible buying rows. Badge partners
share a row when both are available and wear the same badge; otherwise the marker's
vertical offset is adjusted. Position and draw priority follow the animated list
points. Scale at or below 409 suppresses submission while retaining the updated
position; larger scales set a zero-angle affine matrix before drawing.

The checkpoint-65 clothing/badge route checks 5,844 callbacks across 2,408 frames,
1,461 per member. It covers 840 absent rows, 62 collapsed-list callbacks, 4,942
matrix updates and draw-list insertions, and both shared/unshared badge cases.
Equipment lookup, ordered helper arguments/results, Q12 positions, division,
matrix entries and draw-pool/list writes are independently derived. Full task72,
sprite64, buying-panel936, animated-list656, shop-work2492 and save1380 records,
owner roots and touched draw-list records are checked at the watched boundaries.
Task/resource pool membership and the ResourceB release callback establish ownership.

The shop is entered using the guarded decoded-command fixture documented for
[list selection](#shop-item-list-selection), then navigated by ordinary inputs.
The fixture is restored before the wrapper resumes. No purchase is made. All 33
capture hashes equal the prior selection route, including the visible field return;
clothing, badges and final field were inspected. These captures observe rendering;
draw submission alone is not an independent pixel oracle. Final task release is
outside this focused check.

Another 864 isolated ARM946 cases execute the whole function on copied RAM with
explicit helper-contract stubs. They cover four members, clothing/badges,
empty/absent/last-visible rows, missing/different/shared badge partners, scales
0/409/410/4096 and signed nonintegral Q12 offsets. Whole main RAM, scratch outside
the 32-byte frame, SP and callee-saved registers are checked. Stubbed helper
results, matrix and draw-list writes are derived independently; these cases do
not execute the real helpers or establish object lifetimes or rendered pixels.

Private reports: `eur_high_shop_buy_marker/clothing_badges65_v1.json` and
`isolated_v1.json`, with source, input and capture hashes validated. The actual
linked object is exact, the full build reproduces the golden ROM, native relinking
reports no differences, and all 107 tests pass. All 104 original saves are unchanged.


## Battle object sprite tiles

[battle_object_sprites.c](../../src/battle/battle_object_sprites.c) reconstructs
`BattleObjectData_ReleaseSprites` (0x020680B0, 132 bytes) and
`BattleObjectData_UploadSprites` (0x02068134, 180 bytes). Both compiled source
functions match without assembly. The final object places the definitions in
native order; MWCC emits them in reverse source order. The nine functions in the
updated upload-caller object also match.

The common-work allocation at `read32(0x020C0660)` starts with sixteen 24-byte
`GameSpriteAllocation` records. The lookup key combines the resource ID's high
byte with the texture set's body ID. Release skips copied resources and null
texture sets, then unlinks every linked matching slot. Upload selects the first
unlinked slot, reads the packed tile count, allocates shared tiles for the sub
screen and uploads component 2 at `0x06600000 + allocation.offset`. The native
loop falls through to slot 16 when all slots are occupied; reconstruction
preserves that behavior without claiming normal reachability or safety.

Private `build/runtime/eur_high_battle_object_sprites/evidence_entry55_v2.json`
and `evidence_exit55_v2.json` cover 706 and 901 frames. The controlled story-55
entry restores the decoded command and script cursor at the battle wrapper;
the exit restores the battle state at destruction. Entry observes two uploads
and sixty release calls; exit adds twenty release calls. Release takes the null
texture path 76 times and copied-resource path four times. Neither route reaches
a matching unlink; that branch is covered separately below. The entry capture
shows the Petey battle command menu. The controlled exit reloads field code but
its final capture is black, so this is not evidence of a visible field return.

The probe checks complete native bodies, live resource-slot ownership, containing
heap extents, full battle (401416-byte) and common-work (70976-byte) allocations,
sprite list nodes/roots and mapped sub-OBJ VRAM. Caller decisions, ordered helper
arguments and the packed count are independent checks. Allocator placement and
compaction are observed only within existing list nodes, the selected record,
roots and sub-OBJ VRAM; the upload is observed only within its selected tile
allocation. These bounds do not independently verify rasterization or allocation
placement. The exit captures and RAM/DTCM snapshots are identical across v1/v2.
Entry's final animation pixels, some callback timing and RAM captures vary even
between unchanged v2/v3 probes using the same byte-identical ROM. Both repeats
pass the caller checks with the same function/branch totals; the cause of that
variation remains unconfirmed. The extra entry repeat covers 706 frames. All
104 original saves retain their baseline hashes.

`isolated_v1.json` adds 122 ARM946 cases on copied RAM: copied/null-resource
guards, no/head/middle/tail/all matching slots, each first-free index including
native slot-16 fallthrough, packed counts 0/1/1023 and forwarded upload results.
The complete target bodies execute with the real resident unlink and packed-field
query. Allocation/upload use explicit ABI stubs; their execution is not covered
by those isolated cases. Checks cover the complete 4 MiB RAM image, stack bounds,
SP and r4-r11. This establishes unlink list writes without attributing them to
ordinary gameplay or to the live exit route.


## Battle numeric and cached OAM callbacks

[battle_number_oam.c](../../src/battle/battle_number_oam.c) reconstructs
`BattleOam_DrawNumber` (0x020684D8, 664 bytes).
[battle_oam_callbacks.c](../../src/battle/battle_oam_callbacks.c) adds
`BattleNumberModel_Draw` (0x0206A674, 188) and `BattleCachedOamModel_Draw`
(0x0206A730, 116). All three actual compiled source functions match without ASM.

The callback renderer owns either a 28-byte numeric payload or a 24-byte cached
OAM payload. Both contain a compact 12-byte `BattleOamTransform`; the matrix and
position record called `BattleSpriteTransform` elsewhere is 64 bytes and has a
different layout. Numeric payloads optionally read an unsigned byte, halfword or
word from a source pointer; widths other than 2 and 4 select a byte. The result
is cached as a word, then truncated to 16 bits for drawing. Hidden models neither
read the source nor draw. Cached OAM counts are truncated from 16 to 8 bits.

Number drawing emits at most three digits, right-to-left at eleven-pixel steps.
Signed division and a 16-bit quotient truncation are preserved. A nonzero leading
setting adds exactly one zero: setting 1 follows the emitted digits; other values
anchor it relative to the original X. X/Y are narrowed at each OAM helper call.
The first remainder assumes a valid digit-table index; the verified caller passes
an unsigned 16-bit value. Native stack analysis confirms nine OAM helper arguments;
the apparent tenth argument in pseudocode is a saved local X coordinate.

An unmodified controlled battle-entry route did not execute these targets. The
exported battle scripts contain one `configure_object_attachment` command in
`BAI_sugiyama`, entry 0, and it selects text mode 5 rather than modes 3/4. That
does not prove the numeric modes are unreachable through every native caller.

Private `build/runtime/eur_high_battle_number/evidence_fixture55_v4.json` records
706 frames with nine temporary draw-callback replacements on an existing live
text renderer. Seven numeric callbacks, two cached callbacks and six nested
number draws execute eighteen real OAM helper calls. Source widths, cached values,
hidden paths, one/two/three-digit values and leading settings are checked. The
ordinary catalog contains two text frames, so eleven temporary catalog boundaries
select ten existing OAM entries as synthetic glyphs. These are not authentic
number graphics. The original text callback is skipped during each replacement.

At the guarded wrapper return, the full renderer allocation (324 bytes), original
payload (148), OAM output (1024), count bytes and all changed catalog boundaries
are restored and compared with their backups. Checks cover full battle/common
allocations, payload ownership, ordered helper arguments and native truncations.
OAM helper effects are observed only within its output buffer and count bytes;
there is no independent pixel or normal-gameplay-equivalence claim. The final
capture shows the battle command menu. All 104 original saves are unchanged.

The v1/v2 fixture attempts exposed the incomplete ordinary catalog; v2 reached
the OAM count guard with an invalid wrapped count. Failed reports and producers
are retained, and those emulator instances were destroyed without saving state.
The corrected bounded catalog passes in v3 and again after integration in v4.
`isolated_v1.json` adds 333 cases using copied v3 RAM/DTCM, complete native target
bodies and the real nested number-draw function. OAM emission is an explicit ABI
stub. Cases cover source widths, visibility, 16-bit value/count truncation, signed
coordinate narrowing and leading-zero placement. Full RAM/DTCM, stack bounds,
SP and r4-r11 are checked. The full build also passes golden-ROM comparison,
native relinking, progress checks and all 107 tests.


## Field camera target and motion updates

[field_camera_update.cpp](../../src/field/field_camera_update.cpp) reconstructs
`FieldArea_UpdateCameraTarget` (0x02072A9C, 1000 bytes) and
`FieldArea_AdvanceCameraMotion` (0x02072E84, 652). Both compiled source functions
match completely without assembly. The shared camera record remains 52 bytes.

The target check stops axes at their destinations, tests tracked entities against
the previous/current camera positions and clamps completed moves to map limits.
Its braking threshold sums discrete speed steps, including the last nonpositive
step, and compares this with the Q12 distance. Each squared component is rounded
before addition. The step function applies enabled velocity axes to the camera
origin, updates speed and advances elapsed time. Tracking builds a temporary
absolute motion and recursively steps it; an expired duration is clamped to one.
X is evaluated before Y at the origin helper, which matters for exact compilation.

Private `eur_high_field_camera_update/evidence_entry55_v1.json` records a
185-frame checkpoint-55 replay: 740 calls to each target, all with inactive axes.
This establishes ordinary entry and return behavior, not active-motion coverage.
`evidence_fixture55_v2.json` adds 25 guarded per-call fixtures on live default
motion records. It checks 743 step calls (including three recursive calls), 740
target checks and three nested setup calls. Cases cover enabled axes, pause,
acceleration and its cap, braking and its minimum speed, exact braking-distance
equality, timed completion, clamping, corner stopping and tracked-target crossings.
Entity tracking executes both timed and profiled setup; an expired duration is
also exercised.

Full 11,216-byte areas, 52-byte motion records and the live 1,440-byte party
entities are checked, with heap extents, area ownership, complete native-byte
guards, helper arguments, returns, SP and callee-saved registers. The parent's
temporary-record observation is bounded to 52 bytes and the nested origin writes
to eight bytes; the child calls are independently checked. The existing vector
helper's eight-byte result remains observational. Square-root results are checked
against independent integer arithmetic; DS hardware timing is not modeled.

Each fixture restores the entire area and the ordinary return value before the
caller continues. These are controlled boundary cases, not natural story camera
sequences. All five captures and the four final VRAM/palette/OAM ranges equal the
unmodified replay; the final field scene was visually inspected. No pixel oracle
is claimed. The initial fixture v1 passed with a 1,312-byte entity prefix; v2
strengthens this to the full party allocation and checks ownership on each call.
All 104 original saves remain unchanged. The full gate passes golden-ROM rebuild,
native relinking, progress consistency and all 107 tests.


## Field background layer motion

[field_layer_motion.cpp](../../src/field/field_layer_motion.cpp) reconstructs
four contiguous functions at 0x020721EC-0x02072A30 (2116 bytes): target checks,
per-frame movement, timed setup and speed-profile setup. All four actual source
functions match completely, without assembly. The field VM's two setup callers
use the shared declarations and remain fully exact.

The area owns three 56-byte records through the pointer at +0x2BC4. Motion uses
Q12 coordinates; the target updater divides coordinate differences by 16,
truncating toward zero, before calling background virtual slot +0x18 with Q8
scroll deltas. The tested area's +0x2500 pointer refers to a live 1936-byte
FieldBackground with resident vtable 0x02050D60 and slot +0x18 at 0x02013470.
The shared area now exposes this background view alongside its navigation view.
Do not infer the allocation size from the shorter navigation prefix.

The native step reads both enabled velocity components before writing either
position. The completion mask combines each layer's two-bit axis mask shifted by
the layer index, so adjacent layers' bits overlap. Profile completion clamps
crossed positive/negative destinations; timed completion instead copies the
current position into the destination and clears activity. Braking includes the
final nonpositive speed step. Timed setup leaves disabled-axis velocities alone;
profile setup returns early for zero displacement and forces nonzero active-axis
velocities to at least one signed Q12 unit when the vector helper rounds to zero.

Private `eur_high_field_layer_motion/evidence_entry55_v1.json` records a 185-frame
checkpoint-55 field replay, with 370 step calls and 370 target checks, all inactive.
`evidence_fixture55_v1.json` adds 21 guarded, restored cases to the same route:
individual axes and all three layers, pause, acceleration/cap, braking/minimum,
positive/negative target crossings, timed completion, exact braking-distance
equality, distance scaling and negative fractional scroll deltas. Full area and
layer allocations, helper arguments/returns, SP and callee-saved registers are
checked. Vector length is independently derived. Existing vector output remains
observational within eight bytes; renderer observations are limited to the current
layer's two scroll words, dirty byte and 4096-byte tilemap. The surrounding
1936-byte background and affected tilemap allocations are checked in full.

Fixtures restore area, layer records, background, tilemaps and the ordinary return
value before the caller resumes. All five captures and the four final graphics
ranges equal the ordinary run; the final field scene was visually inspected.
These are controlled active cases, not natural story scroll sequences. Setup is
covered separately by 240 isolated ARM946 executions on copied checkpoint RAM:
all three indices, zero/single/dual axes, signed division, stop-bit truncation and
zero/nonzero vector output. Direction, division and vector helpers are explicit
stubs in those isolated tests; they check setup behavior and ABI, not the helper
implementations or live script execution. Full main RAM/DTCM and stack outside
the native 32-byte frame are checked. All 104 original saves remain unchanged.
The full build, golden ROM, native relink, progress checks and 107 tests pass.


## Field camera focus transitions

[field_camera_focus.cpp](../../src/field/field_camera_focus.cpp) owns the two
functions at 0x02073740-0x02073860. Beginning a transition disables following,
remembers the signed-byte entity index and starts a 12-pixel-per-frame move toward
the entity's screen anchor. Completion restores following, caches X and Y minus Z,
and optionally starts an auxiliary room script. A signed script index of -1
suppresses that call. The shared area exposes these fields at +9210, +9211 and
+9212 without changing their existing aliases or layout. Both complete source
functions match, without assembly.

Private `eur_high_field_camera_focus/evidence_entry55_v1.json` is a 185-frame
checkpoint-55 baseline with no focus calls. `evidence_fixture55_v1.json` adds two
controlled pending requests, with script -1 and script 0, through native party
update 0x020A146C. The live party manager's +28 area pointer and the area's +9214
signed target byte and +9216 signed script halfword are the request fields;
the current camera target at +9210 is separate. Entry requires complete native
guards, the owning field system, verified area/manager/entity allocations and all
four room script states inactive. Each request completes in two frames and checks
one begin, one completion and one nested motion setup. This establishes controlled
engine entry, not a natural story camera sequence.

The oracle checks whole area/entity allocations, ordered helper arguments, SP and
callee-saved registers. Nested motion setup is independently modeled; its parent's
52-byte result observation is bounded. The vector helper's eight-byte output is
observational. Auxiliary script-state initialization is independently checked;
busy/disabled auxiliary slots and other entity indices were not exercised here.
Each completed request reloads the original emulator state and verifies equality
of all 4 MiB main RAM and 16 KiB DTCM before the ordinary input route. All five
subsequent captures and four final graphics ranges match the baseline; graphics
remain observational. All 104 original saves are unchanged. Full build, golden
ROM, native relink, progress checks and 107 tests pass.


## Field spin motion and partner positioning

[field_party_spin_motion.cpp](../../src/field/field_party_spin_motion.cpp)
reconstructs three functions at 0x020B97F8-0x020B9A68, without assembly. The two
rotation helpers add signed halfword speed to the angle, wrap it to 15 bits and
set one of eight Q12 facing directions before accelerating the next frame's speed.
Positive acceleration clamps at 2867 or 2048; negative acceleration stops at zero.
The state record's +16 halfword now has an angular-acceleration alias. Its angle
and speed remain unsigned in shared storage because other users need that view;
these helpers explicitly preserve the native signed reads and halfword wrapping.

Partner positioning runs only for member locomotion states 0-3 with a partner.
A partner in states 0-3 receives the member's full XYZ position and the existing
3D setter's previous-position/contact updates. Otherwise, it receives XY offset
four pixels opposite the member's facing direction, through the planar setter.

Private `eur_high_field_spin/evidence_spin83_v3.json` records 551 frames from the
checkpoint-83 action-mode state: 59 slow and 734 fast rotation calls, covering
steady speed, acceleration and the fast speed cap. Every call checks full
1440-byte members, the owning 16764-byte manager, embedded state records, helper
arguments, SP and callee-saved registers. The existing facing/animation helper's
effects remain observational within member fields +378/2, +388/4, +404/4, +414/2
and +972/4; facing itself is checked against the independently calculated angle.
Renderer allocations and auxiliary renderer effects are not independently checked
by this probe. The first replay failed because its observation bounds omitted the
animation refresh's contact-dirty flag; corrected v2 and final v3 pass.

`evidence_fixture83_v1.json` adds four partner cases at the fully guarded entity
update boundary 0x02075F0C: 3D, planar, inactive and absent partner. It temporarily
selects movement mode 6 and the required locomotion/partner fields. Both complete
member allocations are restored at 0x02075F28 before the caller continues; cleanup
also restores on failure. Position-setter writes and direction-vector results are
independently derived. All eight captures and four final graphics ranges equal the
ordinary route, and the final field scene was visually inspected. These partner
cases are controlled entries, not natural action coverage.

Another 338 isolated ARM946 calls use copied live RAM: 210 rotation cases cover
signed limits, wraparound, positive/zero/negative acceleration and threshold
crossings; 128 partner cases cover both early exits, both setters and all eight
directions. The facing helper is explicitly stubbed; position setters and the
direction helper execute their complete native code. Whole RAM/DTCM, SP, r4-r11
and stack outside the maximum native call depth are checked. The initial stack
guard omitted the direction helper's eight saved bytes; the corrected 24-byte
partner depth passes. These checks do not add live gameplay coverage.
All 104 original saves are unchanged. Complete source objects, the updated entity
caller, golden ROM, native relink, progress checks and all 107 tests pass.


## Battle reward-list entries

[battle_reward_item_entries.c](../../src/battle/battle_reward_item_entries.c)
owns 0x0206DE30-0x0206DFD0 in overlay 2. The 12-byte entry contains the item ID,
three byte counts and a cached halfword from item-record offset 12. Initialization
uses categories 1..3; lookup uses count indices 0..2, or -1 for any category.
Initialization clears all four bytes at entry offset 4, including the trailing
byte, and narrows the item ID/count. Lookup compares the full input integer with
the stored unsigned halfword and returns the first qualifying index, or -1.
The cached value participates in overflow selection; its general item-table
meaning remains unnamed. Callers must provide valid tagged items and list bounds.

The four separate checked item-table accessors reproduce the native predicated
loads. Lookup tests a positive entry count before constructing its category
cursor and keeps the two successful returns distinct. These dataflow differences
explain the initial compiler mismatches; no assembly or source search was used.

Private `eur_high_reward_entries/evidence_fixture55_v1.json` records a controlled
victory entry from the initialized checkpoint-55 battle state. At fully guarded
BattleMain_Update, with no pending party AI and no reward controller, the fixture
selects turn phase 0x5029. Native transitions construct the result display and
1344-byte reward controller. At initializer completion 0x0206E568, known item IDs
and counts are placed in context offsets 140..209. Native resource loading then
reaches the list builder. In 89 frames, six initializations and nine searches
cover all four item tags, all three reward categories and successful/absent
category-zero searches. Every call checks the complete reward allocation, SP
and r4-r11. Other callbacks and rendering remain observational.

The complete savestate is reloaded after list assembly, with equality checks
over main RAM and DTCM before 30 neutral frames. The restored battle was visually
checked. This proves a controlled entry, not a naturally completed battle.
Another 480 isolated ARM946 calls use copied live RAM and real item tables:
360 initializations cover first/middle/last slots, category and narrowing edges;
120 searches cover empty/negative counts, duplicate IDs, all categories, misses
and full-word ID comparisons. Whole RAM/DTCM, scratch memory outside the native
16-byte stack frames, ordered writes, returns and callee-saved registers are
checked without helper stubs. The first isolated probe incorrectly allowed no
stack frame for lookup; its preserved failure was corrected to the native frame.
All 104 original saves are unchanged. Actual source objects match completely;
the full build, golden ROM, zero-difference native relink and 107 tests pass.


## Battle reward item names and quantity labels

[battle_reward_item_text.c](../../src/battle/battle_reward_item_text.c) selects
localized names at 0x0206C6E4-0x0206C848. Action items, usable items, badges and
clothing use text tables 2, 6, 9 and 11. The name ID comes from item-record offset
2; a signed count greater than one selects the following plural entry. The input
item remains a full word; only its tag and low twelve index bits select a record.
Invalid item tags are outside the native caller contract and dereference null.

[battle_reward_counter_initialize.c](../../src/battle/battle_reward_counter_initialize.c)
initializes a 24-byte count label at 0x0206CB7C-0x0206CC08. It caps the unsigned
total at 99, keeps a halfword copy across name lookup and formatting, and stores
the item, count, text pointer, coordinates, two-frame delay and row byte. Decimal
text is space-padded to two characters and terminated with zero; the fourth
buffer byte and effect pointer are preserved. Three separate flag-byte writes
clear bits 0/1 and set bit 2 without clearing the remaining flags. An explicit
zero-initialized plural flag and a retained halfword amount explain the original
compiler differences. No assembly is used.

Private `eur_high_reward_text/evidence_fixture55_v1.json` extends the guarded,
restored victory fixture described above by one frame, through load phase 2.
Over 90 frames it independently checks seven name lookups and five count-label
initializations, plus the previous fifteen entry-helper calls. The checks cover
all item classes and singular/plural names. Localized table offsets and returned
string pointers are derived from the live save context and resource contents;
decimal output, helper arguments, the entire 1344-byte reward allocation, SP and
r4-r11 are checked. The restored battle capture equals the preceding fixture's
capture and was visually inspected. Natural victory navigation is still untested.

Another 400 isolated ARM946 cases use copied live RAM and native text/decimal
helpers without stubs. They cover signed count extremes, high item-ID bits,
singular/plural boundaries, counts 0/1/2/98/99/100/65536/0xFFFFFFFF, and coordinate
and row truncation. Whole RAM/DTCM, ordered label writes, returns, callee-saved
registers and scratch outside the maximum native call depth are checked. The
current language's real tables are used; other language selections are untested.
Full source-object comparisons, golden ROM, native relink, progress checks and
107 tests pass. All 104 original saves are unchanged.


## Battle reward-label drawing

[battle_reward_counter_update.c](../../src/battle/battle_reward_counter_update.c)
reconstructs 0x0206C848-0x0206CB7C. It marks a row visible even when its label is
already complete. Otherwise it counts down, draws one name glyph, switches to
the decimal buffer when the name ends, then draws two digits at fixed positions.
Acceleration clears the delay and speeds a live matrix-animation track to
0x4000. The first draw starts animation 759, plays sound 242 and clears the two
tile rows behind the label. Name completion, digit drawing and effect creation
use distinct bits in the existing byte flags. The native second argument is
unused. Reordering the name-end branch to match native fall-through resolved
the initial 15 differing words; the full function matches without assembly.

[battle_reward_items_begin_display.c](../../src/battle/battle_reward_items_begin_display.c)
at 0x0206D6E0-0x0206D740 marks an empty list ready and clears its draw callback.
For a populated list it clears readiness and the display phase, then installs
the native display updater. Its first draft matched completely.

The label renderer is a 324-byte callback-renderer allocation, accessed through
the existing `BattleCallbackModelView`. Its owned buffer is 148 bytes; the
64-byte upload record begins at buffer offset 28, with flag byte at offset 60.
The updater clears upload bits 0/2 and sets bit 1, preserving the rest. The
callback view's 320-byte prefix does not establish the allocation size. Existing
matrix-animation tracks are 56-byte pooled records, distinct from renderers.

Private `eur_high_reward_update/evidence_signal55_v7.json` extends the controlled
victory route. Neutral input and a separate confirmation-button trial both
stopped at phase 0x502C without reaching the new updater. Its native transition
tests bit 2 in the result-panel primary model's flags. At frame 300, the fixture
requires the guarded turn dispatcher, phase 0x502C, an initialized reward work
with no update/draw callback, a 440-byte panel model and animation mode 0x1000.
It supplies that bit once, verifies the returned phase 0x5008 and restores the
original flag word before the caller resumes. The resulting display state is
later removed by reloading the entire original savestate, with main RAM/DTCM
equality before 30 neutral frames. A is held during frames 350..353.

The 500-frame run checks one display start and 134 label updates: 34 delays,
57 name characters, five name completions, five digit draws and 33 completed
entries. It verifies the full reward work, callback renderer and owned buffer,
existing animation tracks, helper arguments, return values and SP/r4-r11.
Track/context creation and text initialization are independently modeled.
GameText_Next remains observational only within its 48-byte cursor, eight-byte
return token, 8192-byte pixel buffer and shared 480-byte glyph scratch at
0x0205AAB4-0x0205AC94. Sound internals and rasterization are not independently
modeled. The final reward-list and restored-battle captures are byte-identical to
earlier captures that were visually inspected. This is a
controlled readiness test, not natural victory coverage.

Another 172 isolated ARM946 cases cover empty/nonempty starts, signed delay
limits, completion flags, name/digit transitions and active/inactive tracks.
Native animation creation, text initialization, glyph drawing and tile filling
execute; only sound is stubbed. Whole RAM/DTCM, returns, callee-saved registers
and scratch outside the observed stack depth are checked (maximum 304 bytes).
The same bounded glyph observations apply. The first isolated guard mistakenly
treated another overlay's interior address as a resident function entry; guards
now select full native bodies present in the copied image. The next run exposed
the shared glyph scratch, whose native decoder and symbol bounds established
the added observation range. Failed producers/reports are retained. The final
live and isolated runs pass, as do actual source-object comparisons, the full
golden-ROM build, zero-difference relink and 107 tests. All 104 saves are unchanged.


## Battle reward-panel rendering

[battle_reward_items_draw.cpp](../../src/battle/battle_reward_items_draw.cpp)
reconstructs 0x0206CED4-0x0206D270. It submits the visible item icons and label
layer, then a top panel sprite, one middle sprite per row and a bottom sprite.
It collects these into a sub-screen OAM group and restores the panel's original
Y coordinate. Tagged item records supply icon animation IDs at byte 9.

The reward workspace embeds three 260-byte scene objects at offsets 12, 272
and 532. Two eight-element arrays of 24-byte counters start at 792 and 984;
eight four-byte item/visibility rows start at 1208. The shared workspace type
now names these arrays without changing its 1344-byte size. Using typed row
entries removed an extra induction variable from the first draft. Declaring
saved locals in their observed stack order while retaining their initialization
order resolved the final differences. The complete 924-byte function matches
without assembly. The two panel/icon controllers allocate 304 bytes each;
the label callback renderer allocates 324, with a 148-byte owned buffer.

Private `eur_high_reward_draw/evidence_signal55_v3.json` replays the previous
controlled reward-display route for 500 frames and verifies 200 complete draw
calls. Two, four and six icons are visible during 25, 29 and 146 calls,
respectively; all four item classes occur. The oracle checks helper order and
arguments, the full workspace and three renderer allocations, the common
workspace, OAM reservation/group bookkeeping, visibility flags, restored panel
position and SP/r4-r11. Model-controller effects remain observational only in
model offsets 84..92, 100..124 and 128..304. Draw helpers may update the
1024-byte OAM output, two local count bytes, label payload and shared four-byte
sort-key override pointer at 0x0205A8AC. Rasterization is not independently
modeled. The final list was visually inspected; its capture and the restored
battle capture are byte-identical to the preceding verified route. State
restoration checks main RAM and DTCM before neutral frames. The route still
uses controlled victory/readiness fixtures, not natural victory navigation.

Another 54 isolated ARM946 cases cover list counts 0/1/8, hidden/all/alternating
rows, panel Y values -40/30/250 and initial OAM reservations 0/32. Native helpers
execute without stubs. The copied hardware interface permits only 32-bit main
and sub DISPCNT reads, using values captured at the live draw entry; other I/O
accesses fail. This does not model hardware timing. Whole main RAM/DTCM and
scratch outside the observed native stack depth are checked; maximum depth is
304 bytes. The same bounded renderer observations apply.

The first isolated attempt exposed the missing sub DISPCNT read in the label
renderer. A later whole-RAM comparison exposed the shared sort-key pointer;
native renderer code calls the existing override setter with model offset 80.
An intentionally near-full reservation of 120 entries then exceeded the
128-entry output buffer. That invalid-capacity fixture and its overwrite trace
are retained separately; the passing cases keep enough space for the complete
group. The game code was not changed to accommodate these probe corrections.
All ten reward source objects, the golden ROM, zero-difference native relink
and 107 tests pass. All 104 original saves remain unchanged.


## Field party member binding and leader palettes

`FieldArea_BindPartyMembers` (overlay 0, `0x0207BED8`, 448 bytes) selects
the leader/follower from the area's entity slots using the two byte indices
at controller +2/+3. It publishes both links, assigns the two embedded 32-byte
state records and sets controller flag bit 4. Flag bit 3 selects either zero
or seven for collision-policy bits 12..14 on both members; the signed 64-bit
policy's remaining bits and both native read/modify/write steps are retained.

Leader movement mode 6 or controller flag bit 3 selects palette clearing.
Otherwise palette animation starts in slot 0: animation 2 normally, 3 for the
special-contact flag. `FieldParty_ClearLeaderPaletteAnimation` at `0x0209CB90`
and `FieldParty_StartLeaderPaletteAnimation` at `0x0209CBFC` are 108 and 164
bytes. Controller bit 30 records whether the override is active; `force`
bypasses the redundant-update guard. Clearing binds animation -1 and requests
a base-palette reload. Starting unpauses the slot and selects mode 1.

Private `build/runtime/eur_high_party_bind/cold65_v1.json` records an automated
2233-frame cold load of save 65: two binding calls, one palette start and three
clears, including a skipped clear. The final field capture was inspected.
The checks independently cover whole area (11216), manager (16764) and member
(1440) records, links, collision policies, callback arguments and flag changes.
Virtual renderer effects are accepted only within the observed 316-byte
renderer allocation; palette-track/global mutations and graphics internals
remain observational. All 104 original saves are unchanged.

`build/analysis/high_effort_50_to_55/party_bind_isolated_v2.json` adds 64 ARM946
cases on copied entry RAM/DTCM, covering both groups, policy states, movement
modes 0/6, force, active bit and contact mode. They execute compiled matching
target bodies and native virtual callees, with no stubs or hardware maps.
Caller records, high collision-policy bits, stack restoration and r4-r11 are
checked; maximum stack use is 64 bytes. These are isolated boundary checks,
not additional live gameplay coverage. Both probes and their input hashes are
recorded privately; they are not fresh-clone build dependencies.


## Field queued music and default actions

`FieldArea_ApplyQueuedMusic` (overlay 0, `0x0207C098`, 216 bytes) runs only
on field screen 0 while save-context byte `gSaveData + 0x514`, bit 0 is clear.
The party manager's word at +4 contains signed current music in bits 17..23,
signed queued music in bits 24..30 and a bank-clear flag in bit 31. Queue -1
means no change; zero stops playback, other values call `GameAudio_SetMusic`.
Afterward the queued value is read again, copied into current music and reset
to -1. Bit 31 requests clearing bank 1 even without a queued sequence; this
function does not clear that flag. Signed encoding does not make every
representable value a valid music sequence.

`FieldParty_ResetDefaultActions` (`0x0209CD84`, 124 bytes) does nothing without
a leader or outside movement modes 0/6. Otherwise it resets action selection
through the existing helper. Special-contact mode with movement mode 6 then
writes action 8 to both member slots. The shared controller and member layouts
remain unchanged; the music fields now have names instead of one unknown byte.

Private `eur_high_party_music/controls65_v2.json` under `build/runtime/`
checks one no-change music call and one ordinary action reset over 2453 frames.
The cold-load inputs are followed by the documented R/L/X/Y controls; the final
inspected capture shows the Thwomp Volcano save menu, with no save confirmation.
`music_cold1_v2.json` checks a real music-7 request over 2633 frames and ends
in the visible castle field. No RAM fixtures are used; all 104 original saves
retain their baseline hashes. The earlier cold65_v1 run failed its coverage
requirement because it reached music but no action reset, not a memory mismatch.

The oracle checks whole applicable area/manager/controller/member records,
queue guards, sequence arguments, consumption order and action-table results.
Audio helper internals, sound output and unrelated audio globals are outside
that caller oracle. `build/analysis/high_effort_50_to_55/party_music_isolated_v1.json`
adds 40 music cases and 193 action cases on copied ARM946 RAM/DTCM. Audio calls
are explicitly stubbed there; action reset/refresh/member helpers execute native.
Screen/disabled gates, signed queue boundaries, bank clearing, null leader and
all 16 movement modes are covered, with SP/r4-r11 and untouched DTCM checks.
The native default-action pairs are all zero; language variations do not reach
the helper's localized action-9 branch. Maximum stack use is 24 bytes.
These fixtures supplement the live calls and do not establish additional
gameplay or sound-system coverage.


## Field room resource and request allocation

`FieldResources_AllocateRoomBuffers` (overlay 0, `0x020783C0`, 600 bytes)
allocates primary, secondary and alternate resource arrays for two room sets.
Their strides are 24, 20 and 20 bytes. A nonpositive count stores a null pointer;
positive counts allocate from the main heap and clear the requested extent.
Only five of the six special-resource indices are reset to -1.

The temporary heap receives 44-byte ordinary requests for each primary resource
and two for each secondary/alternate resource. Each primary resource also gets
two 64-byte compressed requests. Both primary counts remain cached across the
ordinary-request allocation. The routine clears both request arrays, publishes
current cursors and resets the two 16-bit outstanding-read counters. Zero-size
request arrays still call the native allocator and byte-fill helper; they return
null and perform no payload writes. Positive allocation failure is not guarded
by this caller. `func_0202cbd4` is its actual fill helper, distinct from
`MI_CpuFill8`; substituting the latter changes call targets.

Private `build/runtime/eur_high_resource_allocate/cold65_v1.json` records two
calls and nine allocations in a 2233-frame ordinary cold load of save 65.
The main/temporary heap pairs are 0/1 and 1/0, so both allocation directions
are exercised live. Whole area records (11216 bytes), allocation pointers,
visited heap headers/neighbors, full payloads including slack, zero-fill extents,
counters and cursors are independently checked. The visible field capture was
inspected; all 104 original save hashes remain unchanged.

`build/analysis/high_effort_50_to_55/resource_allocate_isolated_v1.json` adds
22 ARM946 cases on copied entry RAM/DTCM: empty, individual, mixed and eight-per-
category sets with both heap assignments. The compiled matching caller and
native allocation/fill helpers run without stubs or hardware maps. Checks cover
all write destinations, heap links, untouched sixth index, SP/r4-r11, DTCM and
unused scratch; maximum stack use is 92 bytes. These fixtures do not test
exhausted positive allocations, asynchronous resource loading or graphics.


## Field entity palette profiles

`FieldEntity_SetPaletteProfile` (overlay 0, `0x02076F40`, 368 bytes) implements
field commands `0x073` and `0x074`. Selectors below 16 search both standard
palette sets for an inclusive `first <= selector <= first + count` interval.
Larger selectors search alternate palettes by their low nibble, including
shipped values above 127. The matched 24-byte palette and parallel 20-byte
secondary resource supply the renderer's palette and animation controller.
A valid matching entry is required: the native routine dereferences a null
resource when the search fails.

The entity stores the selector, with bit 7 marking alternate palettes. The
renderer receives the low nibble at +96, palette pointer at +44 and controller
at +132. Virtual slots +96/+120/+136 reset controller work, select the current
resource animation and clear all eight auxiliary animation tracks. Both field
renderer vtables (`0x020C1594`, `0x020C14D4`) resolve these slots to resident
`0x0200CD68`, `0x0200C7A4` and `0x0200C5D8`. The middle callback takes full-width
animation/reset arguments; the caller sign-extends its stored animation ID.
The field resource and renderer headers now expose these shared fields and slots
while retaining their raw views.

Private `build/runtime/eur_high_palette_profile/live65_v2.json` records two
controlled calls in 180 frames of the initialized save-65 field. Selectors 16
and 17 select alternate-set-0 entries 0 and 1 from the current live tables.
Temporary decoded command `0x073` edits are restored at dispatcher return,
together with the affected entity and renderer. Restoring the entry registers
and PC replays the interrupted original command; both original calls are also
observed returning. No original save is written. Whole area (11216), entity
(1440), renderer (316), visited palette/resource arrays and accessed controller
table entries are checked independently. The three native controller callbacks
run without stubs. The final visible Thwomp Volcano field capture was inspected.
The earlier `live65_v1` run checked entry 0 twice; it is not extra branch coverage.

`build/analysis/high_effort_50_to_55/palette_profile_isolated_v1.json` adds 56
ARM946 cases on copied live RAM/DTCM. Synthetic tables cover both palette
families and sets, first/later matches, inclusive interval ends, empty first
sets, high alternate selectors, absent controllers and missing/default/specific
animation tracks. Actual matching compiled code and native callees execute;
all writes are bounded, and full receiving records, SP/r4-r11, DTCM and unused
stack are checked. Maximum stack use is 124 bytes. These fixtures do not prove
normal story-trigger coverage or subsequent palette transfers to VRAM.


## Party follower height gate

`FieldParty_UpdateFollowerHeightGate` (overlay 0, `0x0209C930`, 608 bytes)
updates each party from `FieldPartyManager_UpdateActions`. It requires a loaded
area with region value 71 and a room matching the party's ten-bit movement
state. In member mode 3, movement enabled and neither follower visibility flag
7 nor 8 set, it calls the separation predicate in both directions. Their low
result bits are ORed into party flag 19; the other paths clear that flag.

The embedded follower state starts at party +152. Its bit 1 gates following;
bit 2 suppresses setting that gate. A set gate is cleared and the path restarted
when separation ends, or when the follower has positive relative height, its
previous-vertical-motion flag is clear and its current Z is at or below the
previous Z. The path initializer receives the follower, two null arguments,
the saved distance at party +180 and a zero fifth argument. Afterwards, setting
the gate requires a higher follower, no active vertical motion and separation.
A two-level link through entity +1264 suppresses it if the final entity index
matches the leader. This link is distinct from `support_entity` at +1272.
The shared header retains the raw follower-state view alongside these fields.

Private `build/runtime/eur_high_follower_height/controls65_v1.json` checks 298
calls and 440 native predicate invocations over 213 frames without RAM edits;
its A input opens the save menu, and no save confirmation follows. A second
route, `movement65_v3.json`, checks 578 calls and 616 predicate invocations in
403 frames, including 164 region-gated returns. Its final capture shows the
visible Thwomp Volcano field. The shorter `movement65_v2` ended during a black
transition; the extended route has the same first 223 input frames and recorded
call records. The short run is not separate visible-readiness evidence.

Each applicable call independently checks the whole party (8356 bytes), area
(11216), both member allocations (1440 each), accessed links, caller flags,
height decisions, arguments and preserved registers. Native separation results
are observed; the geometry implementation is outside this oracle. Live calls
only observe a zero combined separation bit and never restart the path.

`build/analysis/high_effort_50_to_55/follower_height_isolated_v1.json` adds 121
ARM946 cases on copied RAM/DTCM. Both screen slots, area/region/room gates,
member modes, visibility and movement flags, both predicate results, signed
height and equality boundaries, restart/set/suppress conditions and link
indices are covered. The actual compiled caller executes; the predicate and
path initializer are explicitly stubbed at guarded native entries with checked
arguments. Full records, all write destinations, SP/r4-r11, untouched DTCM and
unused stack are checked; maximum stack use is 24 bytes. These cases verify
the caller, not collision geometry or path reconstruction. All original save
hashes remain unchanged.


## Follower bindings and slope distance

`FieldParty_BindFollowerState` (overlay 0, `0x020B941C`, 36 bytes) replaces
non-null state/target bindings at member offsets 1384/1388, then clears only
state flag bit 0. A null state argument retains the previous binding, which
must be valid. The state is an embedded 2208-byte party record.

`FieldParty_GetSlopeDistanceScale` (`0x020BB1A0`, 316 bytes) returns Q12 one
for a null surface or a surface of another type. For type 0 it samples the
triangle's XY center and a direction-dependent offset using native height
helper `0x020BDC88`. Direction is 0..7. Signed center division truncates toward
zero. The absolute height difference is multiplied by a signed halfword
coefficient, rounded by adding 2048 before shifting 12, narrowed to `fx32`,
then subtracted from 4096. Surface slope-axis value 1 chooses one coefficient
table; all other values choose the other. The member argument is unused.
The shared sampler declaration retains a full-width flag; native code reads
its low byte. Both current callers pass 1.

Private `build/runtime/eur_high_follower_setup/cold65_v1.json` checks four
ordinary bindings during a 2233-frame cold load of save 65. Both arguments
are non-null in these calls. Full member/state records, pointer stores,
enabled-bit preservation, SP and r4-r11 are checked. The final visible Thwomp
Volcano field capture was inspected; original saves are unchanged.

`live43_v5.json` uses the initialized `eur_story_043_navigation.dst` state,
whose observed room is 150 (Peach's Castle). Eight controlled calls sample a
loaded sloped triangle in all directions. Three produce increasing heights,
three decreasing heights and two equal heights; all use slope axis 0.
Member/surface records, input tables, six sampler arguments and caller return
arithmetic are checked independently. Native geometry executes without stubs,
but its height results are observed rather than independently recomputed.

The probe temporarily redirects execution at a guarded Field VM entry.
DeSmuME executes the already-decoded 36-byte push before redirecting; the probe
restores those stack bytes and SP at the slope-function entry. At return it
restores the original registers and lets the VM entry execute normally.
All eight original commands are observed completing, and the final visible
castle capture was inspected. This is controlled-call coverage, not a normal
story trigger. Earlier attempts are retained: the save-65 room lacked type-0
surfaces, and initial redirection attempts did not correctly handle the already
decoded instruction. They are not successful verification runs.

`build/analysis/high_effort_50_to_55/follower_setup_isolated_v1.json` adds 188
ARM946 cases on copied live RAM/DTCM: eight binding combinations, 160 direction/
axis/height-difference cases, 15 other surface types, a null surface and four
signed center-division cases. The geometry helper is explicitly stubbed in
these cases, with all six arguments and output pointers checked. Actual
compiled matching callers execute; full records, native write bounds,
SP/r4-r11, DTCM and unused stack are checked. Maximum stack use is 48 bytes.
These fixtures do not validate the geometry implementation or graphics.


## Party locomotion and direction speed

Overlay 0's `FieldPartyEntity_MapLocomotionState` (`0x020BAE58`, 620 bytes)
selects a category from the locomotion-state table, applies field flag 7's
category-1 suppression, then handles categories 0/1 according to subtype and
movement mode. Mode 8 can read the member's and linked member's state-record
bits 20..21; it short-circuits when the first record has value 2.

`FieldPartyEntity_UpdateLocomotionState` (`0x020BB0C4`, 220 bytes) requires
property flag 0. States 0..3 use the native 3D updater except for nonzero
subtype in mode 6. In mode 6, subtype 0 also copies the resulting state to a
partner whose state is at most 3. States 57/70 advance by one when current Z
is strictly below previous Z. The final virtual category mapper still runs;
the 3D update path invokes it twice in total.

`FieldPartyEntity_GetDirectionVector` (`0x020BB484`, 232 bytes) starts with
movement speed, then optionally uses the airborne speed or the native slope
multiplier. The slope check requires a type-0 ground surface and at least one
of its three vertex heights strictly above the member. The Q12 product rounds
before narrowing. The selected scalar is cached at member +304, exposed as
`party_direction_speed` alongside the existing shared velocity view. Ground
surface +1292 likewise retains its old raw alias. The native vector helper
writes X/Y and returns Y; idle direction -1 produces zeros.

Private `build/runtime/eur_high_party_locomotion/live43_v1.json` records an
ordinary 276-frame keypad route in the loaded room-150 castle state. All 1104
outer updates and 552 speed calls pass independent full-record, argument,
output and register checks. The updates contain 552 native 3D updates and
1656 mapper calls; those nested calls are not counted again as outer calls.
Partner-state copying and paired-subtype skipping each occur 552 times.
Speed directions are -1 (412 calls) and 0/2/4/6 (35 each). Native slope and
airborne replacement branches are not reached on this route. The final castle
capture was inspected and all 104 original saves remain unchanged.

`build/analysis/high_effort_50_to_55/party_locomotion_isolated_v1.json` adds 470
ARM946 cases on copied live RAM/DTCM: 198 mapper, 107 updater and 165 speed
cases. These cover modes/subtypes, category suppression and linked records,
partner copying and disabled/special states, signed height boundaries,
airborne selection, surface types and strict vertex-height tests, Q12 rounding,
directions and aliased output pointers. The actual compiled functions and
native 3D/vector helpers execute. Only the slope multiplier is explicitly
stubbed, with arguments checked and prescribed Q12 results. Whole records,
all native writes, SP/r4-r11, DTCM and unused stack are checked; maximum stack
use is 32 bytes. This does not verify slope geometry or graphics.


## Auxiliary animation transitions and follower offsets

`FieldParty_UpdateAuxiliaryAnimationTransition` (overlay 0, `0x020931B0`,
444 bytes) updates auxiliary slot 0 (1360 bytes) using its renderer's signed
animation ID and facing direction. State-record bit 11 optionally copies
animation-table bits into the auxiliary's collision/synchronization flags.
Three signed-byte table entries become Q12 offsets. Direction modes and the
target pointer change immediately when requested, or when the member renderer's
signed word at +100 is at most its signed halfword animation speed at +90.
The deferred transition uses the next animation index. Native code caches the
packed facing word before that increment, then extracts direction bits 2..4.
The party state retains its raw view alongside the new flag alias.

`FieldParty_GetFollowerOffset` (`0x02093108`, 168 bytes) uses the opposite
facing direction and requested distance. A type-0 ground surface with any of
its first three vertices above the member applies the reconstructed slope
distance factor, with Q12 rounding before narrowing. The fifth argument is the
Y-output pointer on the caller's stack; the function also returns Y from the
native vector helper. The earlier private draft omitted the slope helper's
third surface argument; the current public call includes it.

Private `build/runtime/eur_high_party_auxiliary_transition/hammer83_v1.json`
checks 11 ordinary calls in 231 frames from the initialized baby-hammer state83.
All face direction 6 and span animation IDs 0..5. There is one immediate
transition, four deferred transitions with flag copying, three deferred waits,
and three deferred transitions without flag copying. Full party/member/
auxiliary/state and actual renderer allocations, table reads, flags, offsets,
targets, SP and r4-r11 are checked independently. The final Star Hill field
capture was inspected. The target calls no helpers and uses no stubs.

`spin83_offsets_v2.json` checks one ordinary follower-offset call in the known
551-frame spin route, facing direction 6, with native vector arithmetic. It
does not enter the slope branch. The final Star Hill capture was inspected.
The earlier `hammer83_offsets_v1` attempt reached no offset call and failed
its coverage assertion; it is not successful offset verification.

`build/analysis/high_effort_50_to_55/party_auxiliary_transition_isolated_v1.json`
adds 492 ARM946 cases on copied live RAM/DTCM: all eight directions, animation
IDs 0..7, flag copying on/off, immediate/waiting transitions and signed threshold
values below/equal/above, including halfword endpoints. Automatic next-animation
fixtures use IDs 0..6 to stay within the direction's table entries. No stubs
are used; 360 cases retarget and 132 wait, with maximum stack use 16 bytes.
`party_follower_offset_isolated_v1.json` adds 206 cases for eight directions,
null/other/triangular surfaces, strict vertex-height boundaries, signed/zero
distances, Q12 rounding and aliased outputs. The actual caller and vector helper
execute; the slope-distance helper is explicitly stubbed with checked arguments
and prescribed coefficients in 0..4096. Maximum stack use is 24 bytes.
Both suites check whole relevant records, native write bounds, stack arguments,
SP/r4-r11, DTCM and unused stack. All 104 original saves are unchanged.
These isolated fixtures do not establish geometry or rendered-graphics coverage.


## Party trail visibility and stopping

`FieldPartyTrail_UpdateDelayedVisibility` (overlay 0, `0x02091DC8`, 144 bytes)
reuses the emitter interval byte as a one-shot countdown. On expiry it shows
the selected auxiliary and copies its owner's facing direction. A zero byte
does nothing. `FieldPartyTrail_StopCopies` (`0x02091D34`, 148 bytes) hides and
stops bound renderers in the emitter range, reloading that range after callbacks.
Its second virtual call uses slot `0x30` (`FieldAuxiliary_StopSpecialRenderer`),
which resets the auxiliary resource index before stopping the renderer.

Private `build/runtime/eur_high_party_trail_control/hammer83_v1.json` checks
20 delayed-visibility calls and one stop call in 231 ordinary gameplay frames;
`roll83_v1.json` checks another ten and three in 349 frames. The stop calls
process one or five bound auxiliaries. All observed delays are zero, so these
routes do not cover countdown expiry. Both final Star Hill captures were
inspected. Whole party/owner/auxiliary records, timer changes, slot selection,
callback arguments, live loop bounds and preserved registers are checked.
Helper effects remain observational within each selected auxiliary's entity
prefix and renderer allocations; the resource-index change is independently
checked. This does not verify helper internals, heap/global-list effects or
rasterization. All 104 original saves retain their trial-baseline hashes.

`build/analysis/high_effort_50_to_55/party_trail_control_isolated_v1.json` adds
306 ARM946 cases using copied live RAM/DTCM and synthetic records: six slots,
eight directions, delay values 0/1/2/255, all valid start/count ranges, mixed
bound/unbound renderers and callbacks that grow or shrink the active range.
The actual compiled callers execute; visibility, facing and renderer-stop
helpers are explicit stubs with checked arguments, prescribed bounded effects
and caller-saved register clobbers. Full records, write bounds, stack restoration,
r4-r11, DTCM and unused stack are checked. Maximum stack use is 24 bytes.
These fixtures supplement the live routes without claiming live expiry coverage.


## Single-copy party effects

`FieldPartyTrail_StartSingleCopy` (overlay 0, `0x02091E58`, 560 bytes)
selects an explicit auxiliary slot or the first non-null invisible slot. The
caller must leave a usable slot when requesting automatic selection. It writes
the emitter state, applies signed table offsets to Q12 coordinates, spawns the
copy, sets its facing and animation, and attaches it to the owner or another
auxiliary. Direction-relative mode uses an eight-byte record for each facing.
The original table pointer is stored before selecting that record. Facing is
read again after the spawn callback. The animation offset and delay are full
integers: emitter storage truncates each to a byte, but the initial visibility
choice tests the full delay and the animation addition uses the full offset.

Private `build/runtime/eur_high_party_trail_single/hammer83_v2.json` checks one
ordinary call in 231 gameplay frames: explicit slot 1, direction-relative mode,
facing 6, no delay and target auxiliary 0. The final capture equals the inspected
Star Hill hammer capture from the preceding trail-control check. Full party,
owner, auxiliary records, selected renderers and table input are tracked;
emitter fields, offset arithmetic, call arguments, target and final flags are
independent expectations. Spawner effects are observational within the selected
auxiliary and renderers; later helper observations are limited to its entity
prefix and renderers. Heap/global-list effects and rasterization are outside
this oracle. The first probe failed at frame 22 because it checked the caller's
final stores at the helper return, before those stores executed; the corrected
probe retains helper-boundary checks before modeling subsequent caller stores.
No game-code change was needed. Failed v1 and successful v2 remain separate.

`build/analysis/high_effort_50_to_55/party_trail_single_isolated_v1.json` adds
244 ARM946 cases on copied RAM/DTCM with synthetic records: all six slots and
eight directions, automatic selection past null/busy slots, relative-byte versus
flag-bit semantics, owner/other/self targets, signed table and animation values,
Q12 wrapping and full-word delays whose stored byte becomes zero. Callback
fixtures also change owner facing to verify its reload. The actual 560-byte
caller executes; spawning, facing and animation helpers are explicitly stubbed
with argument checks, bounded prescribed effects and caller-saved register
clobbers. Full records, write bounds, SP/r4-r11, DTCM, stack arguments and unused
stack are checked; maximum stack use is 56 bytes. All 104 saves are unchanged.
These fixtures do not prove helper implementation or live delayed-effect coverage.


## Navigation bounds restoration

`FieldEntity_LoadNavigationBounds` (overlay 0, `0x020AE520`, 220 bytes)
implements Field VM opcode `0x058`. With a bounds resource, it compares the
unsigned 16-bit animation ID with the unsigned byte animation count and selects
entry zero when the ID is out of range. Byte 1 of the six-byte animation record
selects a five-byte bounds record through a signed byte index. Signed x, y,
width, height and depth become Q12 navigation bounds; all five input bytes are
loaded before the output stores. Without a resource, the rectangle is
-8..8 by -8..0 with vertical extent 32 pixels, and the index is preserved.
The explicit byte offset groups `stride * animation + 1` before adding the
base, preserving the native multiply/add sequence used by the matching compiler.

Private `build/runtime/eur_high_navigation_bounds/live43_v1.json` records three
controlled native calls over 180 frames in Peach's Castle: current animation,
out-of-range animation and no resource. Each uses a guarded live Field VM
boundary, a verified 1,440-byte member and real resource records. Full member
and input records, outputs, SP and preserved registers are checked. The original
member bytes, registers and interrupted VM stack state are restored before each
original VM command completes. The final capture shows the ordinary field scene.
No occurrence of opcode `0x058` was found in the existing event-script export;
this does not establish its absence from every game script. These calls prove
controlled live execution, not an ordinary story invocation or graphics output.

`build/analysis/high_effort_50_to_55/navigation_bounds_isolated_v2.json` adds
206 ARM946 cases on copied live RAM/DTCM with synthetic member and resource
records. These cover unsigned animation/count boundaries, signed indices and
bounds extrema, defaults and source/output aliasing. The actual compiled
function executes without helper stubs. Entire records and source pools, write
bounds, SP/r4-r11, DTCM and unused stack are checked; maximum stack use is eight
bytes. There are 140 fallback-selection, 62 current-selection and four default
cases. The first report classified animation zero/count zero by its unchanged
selected index; v2 corrects that label to the fallback branch and repeats all
cases. Expected memory results were unchanged. All 104 original saves retain
their trial-baseline hashes.


## Body overlap masks

`FieldEntity_GetBodyOverlapMask` (overlay 0, `0x020AE5FC`, 196 bytes)
compares two entities' body volumes. Bits 0, 1 and 2 indicate X, Y and height
overlap. For each axis, it takes the smaller of the two directed penetration
distances. X/Y require a positive distance; height permits zero. Positions,
bounds and arithmetic use the native 32-bit Q12 representation. Explicit cached
positions and comparisons preserve the native load order and signed minimum.

Private `build/runtime/eur_high_body_overlap/controlled43_v1.json` checks three
controlled calls over 180 frames in Peach's Castle: two real party members in
both argument orders, then one member against itself. Both complete 1,440-byte
allocations remain unchanged; return masks are 5, 5 and 7. The independent model
checks wrapped signed distances, results, SP and preserved registers. Original
registers and the interrupted VM stack are restored, and all three original VM
commands complete. The inspected final capture shows the field scene. Ordinary
walking/jumping and hammer routes (`walk43_v1`, `hammer83_v1`) produced no calls;
controlled coverage is not an ordinary gameplay invocation or a graphics oracle.

`build/analysis/high_effort_50_to_55/body_overlap_isolated_v1.json` adds 439
ARM946 cases using copied live RAM/DTCM and two synthetic records. Tests cover
separated, touching and overlapping intervals, unequal X/Y widths, signed
extrema and wrapping, reversed arguments, self-comparisons and degenerate or
negative bounds. All eight masks occur. The actual compiled function executes
without helpers or stubs; complete records, return values, SP/r4-r11, DTCM and
unused stack are checked, with native writes permitted only in the scratch
stack. Maximum stack use is 16 bytes. Extreme arithmetic fixtures do not prove
that those records occur during gameplay. All 104 original saves are unchanged.


## Copy Flower exit arcs

`Overlay17Participant_BeginExitArc` (overlay 17, `0x020C52F4`, 320 bytes)
starts vertical channel 3 and horizontal channel 2 for the selected group tuning
duration, moving toward X = 352. Y is saved before the vertical-motion helper
and compared with the current Y afterward. Animation command `0x400C` precedes
the formation check. Even formations restart the current animation at its last
frame and negate its signed 16-bit rate; all formations enter phase 7. The
animation getter's full result survives the length lookup and narrows to a byte
only at the virtual setter. Frame subtraction and rate negation narrow to signed
16-bit values. Keeping that conversion at the call matches the native schedule.

Private `build/runtime/eur_high_copy_flower_leave/copy83_v1.json` checks ten
ordinary calls over 1,810 frames, starting at the verified Copy Flower setup
checkpoint for story save 83. Inputs are A for eight frames and a wait, with
up to ten ordinary timed A/B/X/Y presses selected from the live input window.
Four calls take the reverse-animation branch and six take the forward branch.
The inspected final capture shows the battle command menu. Participants are
296-byte embedded slots at workspace + 304, not separate heap allocations; the
workspace allocation is 16,356 bytes, larger than the shared prefix type.
The whole workspace, watched renderer allocations and tuning table are checked.
Duration lookup, call arguments, conversions, final phase, SP and preserved
registers have independent expectations. Movement/animation helper effects are
observational only within the current 260-byte scene-object prefix and watched
models. Animation ID/length results are observed; active-model selection and
tuning results are independently checked. This does not verify helper internals,
heap/global palette lists or rasterization. No game-record RAM fixture is used.

`build/analysis/high_effort_50_to_55/copy_flower_leave_isolated_v1.json` adds
208 ARM946 cases on copied RAM/DTCM with synthetic workspace, models and virtual
table. It covers all formations, either model, byte/halfword conversion extremes,
durations 0/1/65535 and signed coordinates. Explicit helper stubs check arguments,
write prescribed state and clobber caller-saved registers. A changed Y tests the
cached value; eight animation callback fixtures change formation/model selection
to test their reload. Whole records, table, root, SP/r4-r11, DTCM and unused stack
are checked; native writes are limited to the phase halfword and scratch stack.
Maximum stack use is 40 bytes. These fixtures verify the caller, not helper
implementations or validity of every synthetic state in gameplay. All 104
original saves retain their baseline hashes.


## Copy Flower target cycling

`Overlay17Attack_SelectNextTarget` (overlay 17, `0x020C2B8C`, 156 bytes)
cycles enemy IDs, skips the previous target and first prefers selectable enemies
without trait 1. A second pass allows that trait; exhaustion returns zero.
The argument and cycle counter are unsigned 16-bit values. Incrementing the
argument in place preserves the native input lifetime and avoids a premature
conversion of a separate local candidate.

The native skip has an edge case: after encountering previous ID 65, its extra
increment produces ID 66 before the normal wrap check runs again. Preserve this
behavior; it does not establish a seventh enemy slot in the normal game layout.

Private `build/runtime/eur_high_copy_flower_target/controlled83_v2.json` checks
three controlled calls at guarded Copy Flower exit-arc boundaries, with previous
IDs 60, 61 and 62. Results are 0, 60 and 60. All selection and trait helpers run
normally; an independent predicate model verifies their arguments, ordered calls
and return values. The full 401,416-byte battle-context allocation and its root
remain unchanged. Original registers and the decoded 16-byte entry push are
restored before all three original exit-arc calls complete. The 1,810-frame route
records A for eight frames plus 50 automatically timed input windows; the final
capture shows the continuing Copy Flower attack. These are controlled selection
calls, not ordinary target-change coverage. No game-record RAM fixture is used.
Version 2 adds explicit input records and corrects the resumed-caller label;
entry RAM/DTCM and final image hashes equal the preserved version 1.

`build/analysis/high_effort_50_to_55/copy_flower_target_isolated_v1.json` adds
1,153 ARM946 cases with the compiled selector and native lookup, selectability,
status and trait helpers, without stubs. Synthetic records cover all six previous
IDs, all 64 selectability masks and three trait masks, plus one explicit ID-66
selection case. Rejections separately use zero/negative HP, missing resources
and the exclusion flag; trait values 0, 1, 2 and 3 are distinguished. A deliberately
allocated seventh record makes the native ID-66 read observable safely in copied
RAM; 83 cases reach it and one returns it. This is not evidence for a valid seventh
enemy in gameplay. Ordered helper calls/results, final results, whole context/root,
SP/r4-r11, DTCM and unused stack are checked; native writes are permitted only to
the scratch stack. Maximum stack use is 40 bytes. All 104 original saves retain
their baseline hashes.


## Copy Flower input and idle slots

`Overlay17Participant_ConsumeInput` (overlay 17, `0x020C4474`, 156 bytes)
returns immediately when the participant's two-bit input state is already set.
Otherwise the pressed mask must equal the formation's key-table entry exactly;
additional keys reject the input. Success sets input state 1 and clears that key
from the attack's pending input mask, leaving the pressed mask unchanged.
`Overlay17Participant_FindIdleSlot` (`0x020C4794`, 60 bytes) returns the first
of eight slots with phase zero, or -1. The phase is independent of the active bit.
The named two-byte flag view preserves the existing participant layout; the
search computes its first flag offset from shared member layouts and advances
by the 296-byte participant stride.

Private `build/runtime/eur_high_copy_flower_input/copy83_v2.json` checks 13
ordinary searches over 1,810 frames from `copy_verified_setup83.dst`, using
A for eight frames and ten timed input windows. Slots 0 through 3 are observed;
the final capture shows the battle command menu. The full 16,356-byte workspace,
root, return value, SP and r4-r11 are checked. The earlier `copy83_v1.json` failed
its coverage requirement because this route never called the input helper;
its 13 successful search checks did not establish input coverage. Native callers
place that helper in phases 6/12 and a target-unavailable path, rather than the
ordinary successful input window.

`build/runtime/eur_high_copy_flower_input_controlled/controlled83_v1.json`
checks three input calls at guarded ordinary exit-arc entries: already consumed,
wrong keys and accepted exact keys. Temporary participant input bits and attack
input masks exercise those branches. An independent model checks the entire
workspace, root and key-table bytes when read. The workspace is restored and
verified before each original exit-arc call resumes; all three complete. The
original registers and decoded entry push are also restored. This separate
1,810-frame route uses 50 timed input windows and ends during the continuing
attack. It establishes controlled helper behavior, not ordinary invocation.

`build/analysis/high_effort_50_to_55/copy_flower_input_isolated_v1.json` adds
576 ARM946 cases using the compiled functions without stubs: 320 input cases
cover all formations and input states, five key combinations and four pending
masks; 256 searches cover every occupancy mask. Nonzero phases and unrelated
flag bits vary independently. Checks include full workspace/root/table ranges,
ordered data stores, search results, SP/r4-r11, unchanged DTCM and unused scratch
stack. Maximum stack use is eight bytes. These copied-memory fixtures do not
establish gameplay lifetimes or asynchronous behavior. Screenshots are observed,
not independently rendered. All 104 original saves retain their baseline hashes.


## Bro/Ice Flower participant control

[`participant_update.cpp`](../../src/attack_bro_flower_ov014/participant_update.cpp)
owns 0x020C53D4-0x020C5724 (848 bytes), and
[`participant_prepare.cpp`](../../src/attack_bro_flower_ov014/participant_prepare.cpp)
owns 0x020C4F10-0x020C50A0 (400 bytes). The update handles entry particles,
automatic handoff, primary/secondary input, animation-marker launches and
preparation of the next projectile. It rereads timer, phase and flags after
children that can change them. Preparation selects the first idle projectile,
chooses mode 1/2 from the active participant when mode is -1, and starts effects
once. Participant +16 is the effect-owner slot; flag bit 2 at +24 records that
the initial effects have started. Model effects use the 56-byte layout, with
parent at +44 and owner backlink at +52. They are distinct from 48-byte sprite
effects. Modes below 2 choose model effect 847 and sprite 540/542; modes at
least 2 choose model 846 and sprite 539/541 for Bro/Ice Flowers respectively.

The update's first C++ draft matches. Preparation initially extracted and
rewrote the old six-bit unknown field, adding 28 bytes. Naming its evidenced
single bit reproduces the native extraction and OR store; the whole function
then matches. Both functions are pure C++, and all four update callers plus
the five preparation calls use the public names. All 42 linked functions across
11 current overlay-14 objects compare exactly after the shared-header change.

Private `eur_high_flower_participant/evidence_{fire,ice}83_v1.json` checks
1,482 and 740 ordinary updates over 2,790 and 2,830 frames. The fire route
uses the established checkpoint/menu inputs; all ten captures match its prior
baseline. Ice uses 41 timed keypad events and exercises repeated primary throws.
`eur_high_flower_prepare/evidence_{fire,ice}83_v1.json` separately checks 3 and
41 preparation calls, including all four sprite IDs and both model IDs. Its
inputs, captures and final graphics match the corresponding participant runs.
Both routes return visibly to the battle command menu. No RAM is edited.
Caller stores, decisions, ordered arguments including stack parameters, model
selection, effect-owner backlink/parent and SP/r4-r11 are independently checked.
Child writes within bounded work, actor, object, model and effect records are
observations; allocation lifetimes, graphics lists, other globals and rendered
pixels are not independently derived.

Participant `isolated_v1.json` adds 92 copied-RAM ARM946 cases for timers,
flags, input priority, stop requests, secondary throws, marker boundaries and
child-mutated fields. Preparation `isolated_v2.json` adds 64 cases for signed
mode boundaries, first/last/full projectile pools, both variants and one-time
effect flags. Native model lookup/free-slot search execute; other helpers are
explicit stubs. Full mapped memory except 256 stack bytes is checked. These
fixtures are separate from gameplay coverage: phases 6/7/8 and the stop branch
are covered only in isolation. Preparation v1 failed its test-driver return
check after 50 cases: it applied ARM condition filtering to the artificial
return sentinel. V2 handles that sentinel directly and passes all 64 cases;
the failed report and producer are retained. The final EUR ROM is exact,
107 tests pass and all 104 original saves retain their baseline hashes.


## Bro/Ice Flower projectile control

[`Overlay14Projectile_Update`](../../src/attack_bro_flower_ov014/projectile_update.cpp)
owns overlay 14's 0x020C4464-0x020C4734 range (720 bytes). It waits for model
and motion-channel completion, advances flights, starts bounces, dispatches
impact handling, hides projectiles and returns slots to phase zero. Channel
activity is the nonzero callback at object +28 +40*channel. Phase and level
are reread after motion helpers, which can change them. Low-level bounce
completion emits sprite effect 287 for Bro Flowers or 535 for Ice Flowers,
plus model effect 409 with a null parent. The first current typed C++ candidate
matches fully; an older aggregate draft exists in private actor-helper notes.
The separate 300-byte target-movement draft remains unlinked with six register
differences. All 43 linked functions in 12 current overlay-14 objects are exact.

Private `eur_high_flower_projectile/evidence_{fire,ice}83_v1.json` repeats the
previous participant routes over 2,790 and 2,830 frames. It observes 23,712 and
11,840 calls and independently checks 1,302 and 1,808: every nonzero-phase
call, plus the first idle call for each of the 32 embedded projectile slots.
The remaining idle calls are counted only. The routes check 42 flight advances,
120 bounces, 44 resets and 22 low-level bounce effects. All inputs, captures and
final graphics match the participant baselines; both final images show the
battle command menu. No RAM edits are made. Ordered stores/calls, phase/level
decisions, native model/channel query results, effect arguments including stack
parameters, model-effect parent and SP/r4-r11 are independently checked. Child
writes within the complete work, actor prefixes, scene objects, models and
effect payloads are bounded observations. Heap lifetimes, graphics lists, other
globals, sound and rasterization are not independently proved.

`isolated_v1.json` adds 46 ARM946 cases on copied live RAM/DTCM: phases 0-9
and default values, active/inactive channels, all projectile levels, both
effect variants, animation completion and child-mutated phase/level/channel
fields. Native model/channel queries execute; motion, impact, reset and effect
children are explicit stubs. Full mapped memory except 256 stack bytes is
checked. Phases 3/4/7, the impact-call branches and the Bro Flower low-level
effect branch are covered only in isolation. These fixtures do not prove child
algorithms or gameplay lifetimes. The full EUR ROM remains byte-identical,
107 tests pass and all 104 original saves are unchanged.


## Copy Flower round and return controller

[`CopyFlowerAttack_Update`](../../src/attack_copy_flower_ov017/copy_attack_update.cpp)
reconstructs overlay 17's outer controller at 0x020C60BC-0x020C6298 (476 bytes).
It copies the four formation input bits, updates the linked participants, then
spawns clones or waits for their return. Subsequent phases restore party objects,
wait for readiness, unlink the display, release eight participant models and
clear the attack callback/root. Clearing the root does not itself prove that
the 16,356-byte work allocation is freed. Participant successors are read after
their update calls, and dispatch uses the signed phase after child updates.
The flag walk retains the shared 296-byte stride. The complete C++ function
matches without assembly; the separate 900-byte initializer remains unlinked.

Private `eur_high_copy_update/evidence_copy83_v1.json` checks 679 ordinary calls
over 1,810 frames from `copy_verified_setup83.dst`: phases 0/1/2/3 execute
614/29/35/1 times, with 13 clone starts. Ten timed keypad inputs match the prior
input-helper route; no RAM edits are used. The final image is identical to that
baseline and shows the battle command menu. Ordered outer calls/stores, input
masks, list traversal, phase/timer decisions, idle-slot and tuning results,
callback/root clearing and SP/r4-r11 are independently checked. Child writes
within the complete work, party/enemy prefixes, scene objects and model payloads
are bounded observations; RNG outcomes and child readiness returns are observed.
Allocation lifetimes, graphics-list neighbors, other globals, sound and
rasterization are not independently proved. Final graphics RAM is captured,
but the prior report supplies no graphics-RAM baseline for comparison.

`isolated_v1.json` adds 29 ARM946 cases on copied RAM/DTCM: signed timer limits,
first/last/no idle slot, pending/completed stops, both readiness results,
cleanup and out-of-range signed phases. Two child stubs change the next list
link or dispatch phase to check that the caller rereads them. Idle-slot search
and tuning lookup execute natively; other children use explicit return/write
stubs. Full mapped memory except 256 stack bytes is checked. These fixtures
do not establish gameplay lifetimes or child algorithms. All 42 linked functions
in 17 current overlay objects are exact; stale objects from former source names
are excluded. The EUR ROM is exact, 107 tests pass and 104 saves are unchanged.


## Save-storage initialization

`SaveStorage_Initialize` (resident ARM9, `0x02028CE8`, 388 bytes) repairs
invalid save headers before the menus use the buffer. A bad magic signature
clears all 8,192 bytes; an unsupported version with valid magic clears only
8,128 bytes and preserves the final 64-byte footer. The default eight-byte
signature is copied in and the directory is cleared. When backup storage is
enabled, signature, settings, directory and four slots are written in order,
with a wait after each slot. A failed write or wait returns zero immediately.
Only a bad magic signature triggers the final footer write; its return value is
ignored by the original. Invalid settings then clear eight bytes, covering the
four-byte settings record and the first four bytes of the directory.

The old C aggregate-assignment draft generates a byte-copy loop and totals
324 bytes. Compiling the same body as C++ emits the native eight byte loads
followed by eight stores and matches the complete 388-byte function. This is a
compiler-language difference, not a volatile barrier or an assembly workaround.
The final linked object and full ROM are exact.

Private `build/runtime/eur_high_save_storage_init/cold65_v1.json` records one
ordinary call at cold-start frame 15 with a current signature and valid settings.
A copied ROM imports a copy of story save 65; 2,477 frames of title/load/field
inputs reach the volcano field, confirmed by the final capture. Independent
checks cover signature/settings results, ordered helper arguments, the entire
8,192-byte save buffer, 44 backup globals, eight signature bytes, SP and r4-r11.
The real signature and checksum helpers execute. No RAM fixture or backup-write
branch is involved; the screenshot is observational rather than a graphics oracle.

`build/analysis/high_effort_50_to_55/save_storage_init_isolated_v1.json` adds
32 ARM946 cases on copied RAM/DTCM: valid, old-version and bad-magic signatures;
disabled storage; valid/invalid settings; failures at each of the three header
writes and all four slot writes/waits; and either footer return value. Twenty-two
cases return failure. Actual signature, checksum, memory-fill and directory-clear
helpers run. Backup writes, waits and footer writes are explicitly stubbed with
no buffer effects, so these fixtures verify the caller's decisions and arguments,
not cartridge I/O or those helpers' internals. Nonzero stub results include -1.
Full buffer, globals, signature, return value, preserved registers, DTCM and
unused stack are checked; writes stay within the buffer and scratch stack.
Maximum stack use is 64 bytes. All 104 original saves retain their baseline hashes.

### Battle healing-item badge bonuses

[battle_badge_healing.cpp](../../src/battle/battle_badge_healing.cpp) reconstructs
`BattleItemEffect_ApplyBadgeBoost` at overlay 2 `0x020768A4..0x02076998`.
The item user comes from the halfword at `gBattleContext + 0x20`; its party
actor's resource pointer at `+0x6C` supplies the member index in the low byte
of its first halfword. This selects the badge at live-save offset
`0x418 + 36 * member`, independently of the healing target.

Badge IDs `0x3008` and `0x3010` scale the HP increase by 150 and 200 percent.
The native calculation adds 50 before signed division by 100, then adds the
target's signed current HP and clamps to its signed maximum. Other badges
return the supplied HP unchanged, without clamping. Preserve the division's
truncation toward zero, including for negative deltas.

Private `high_effort_50_to_55/probe_battle_badge.py` replays the established
checkpoint-83 item route from `eur_attack_helpers/item_damaged_menu83.dst`.
`build/runtime/eur_high_battle_badge/healing83_v1.json` passes over 1,720 frames:
one ordinary call at frame 642 heals Mario from 87 to 100 HP, with badge 25
(no bonus). The checkpoint inherits controlled encounter-entry provenance;
this replay makes no RAM edits. Full native guards, the real party lookup's
argument/result, return value, SP/r4-r11, the 401,416-byte battle allocation,
1,380-byte live save, roots and resource member are checked. The final visible
battle menu was inspected; this is not an independent graphics oracle.

`check_battle_badge_isolated.py` and `battle_badge_isolated_v1.json` in the
same private analysis directory pass 736 ARM946 cases using the actual compiled
function and native lookup, without stubs. Copied RAM fixtures cover four users
and four targets, both bonus badges and a neutral badge, ten arithmetic cases,
all 256 byte-sized badge values, and a remapped resource member with a nonzero
upper byte. Return values, unchanged records, helper arguments/results,
SP/r4-r11, DTCM and unused scratch stack are checked; stores are restricted to
the 16-byte stack frame. Boosted branches have isolated coverage, not live
item-use coverage. All 104 original saves remain unchanged.

The final source object matches all 244 bytes. `battle_badge_build_v1.log`
records the full build, golden ROM SHA-1, zero differing native-relink bytes,
updated progress and 107 passing tests. Runtime producers, snapshots and
source/object hashes are pinned by `battle_badge_validation.json`.

### Battle shared-model resource control

[battle_shared_model_resource.cpp](../../src/battle/battle_shared_model_resource.cpp)
reconstructs `BattleObjectData_ControlResources`, overlay 2
`0x02068878..0x02068970` (248 bytes). Operation 0 sets the resource's copy bit
from the low bit of its argument. Operation 1 allocates a 304-byte alternate
model from the heap ID at `gBattleContext + 0xE15C`, initializes it, stores the
returned model at common-workspace offset 70444, and binds the resource and
embedded palette at 70448. It disables animation before selecting the model as
the second entry of the four-model animation table: first entry cleared, last
two preserved. Operation 2 delegates sprite release; other values do nothing.
The original only guards the constructor against a null allocation; subsequent
binding/preparation calls still receive that null pointer. No new recovery path
has been invented.

Private `high_effort_50_to_55/probe_shared_model.py` extends the existing
common-resource entry probe. `build/runtime/eur_high_shared_model/`
`evidence_entry55_v3.json` passes 706 frames and two completed target calls:
copy mode at frame 24 and shared-model creation at frame 70. The checkpoint-55
encounter command and VM cursor are temporarily supplied at guarded field
boundaries and restored before battle initialization. This is controlled
encounter entry, followed by native setup; the visible battle menu was inspected.
All 104 original saves remain unchanged.

The new oracle checks full battle/workspace allocations (401,416/70,976 bytes),
resource fields, ordered helper arguments, constructor result propagation,
shared model/table writes, animation-bit clearing and SP/r4-r11. It verifies the
allocated model's 304-byte extent and independently predicts its render-list
insertion. Constructor/binder effects on the model and embedded palette remain
bounded observations. Sprite allocation/palette lists are checked for consistent
head/tail/backlinks and unchanged existing payloads; their changed links are
observed rather than independently predicted. Allocation internals, renderer
internals and rasterization are not independently proved. Existing transition
and common-resource checks also pass. The first two probes stopped at frame 71
because the new oracle omitted an existing palette node's changed link; their
reports and producer versions remain preserved.

`check_shared_model_isolated.py` / `shared_model_isolated_v1.json` pass 40
ARM946 cases with the compiled function. They cover copy-bit truncation and
neighboring-bit preservation, unsupported operation values, native sprite-release
no-op paths, and native animation-table updates. Allocation, constructor, binder
and animation-preparation calls are explicitly stubbed in the creation cases,
including a changed constructor return and a null allocation: these establish
forwarding only, not safe handling of real allocation failure. Full tracked
records, ordered nonstack stores, SP/r4-r11 and DTCM are checked. Actual linked
sprite removal is outside this isolated test.

The final source object matches all 248 bytes. `shared_model_build_v1.log`
records the full build, golden ROM hash, zero native-relink differences and 107
passing tests. `shared_model_validation.json` pins source objects, probes and
reports; the actual ROM still matches the European original.


## Overlay 5 static initialization

[`static_initializers.cpp`](../../src/scene_support_ov005/static_initializers.cpp)
reconstructs the complete 332-byte `.init` range, 0x02069D6C-0x02069EB8.
Four initializers clear the element, sprite, draw-node and item pools and
register their destructors. Another constructs two registry markers at a
28-byte stride and registers their array cleanup. Two empty initializers are
retained. Pool layouts and the MSL destructor-node layout have shared headers.
The original `.ctor` table remains the caller; constructor code is not counted
as data. Metrowerks requires `define_section` plus a `section ... begin/end`
pair to emit `.init`. It emits these functions in reverse source order.
Check both the actual object sections and linked addresses: matching individual
functions alone would miss incorrect section placement or order.

Private `eur_high_scene_initializers/boot65_v1.json` checks all seven functions
across three overlay loads during 2,477 cold-boot/navigation frames, using the
copied battery data from story save 65. It independently checks all 60 pool
stores, 24 ordered helper calls, full pool/registration records, both markers,
the destructor-chain root and return/SP/r4-r11. The final capture shows ordinary
field gameplay. No RAM edits or savestate loading are used; 104 original saves
remain unchanged. Graphics hashes are observations, not a rendering oracle.

`isolated_v1.json` checks another 21 ARM946 calls on copied boot RAM/DTCM with
three initial memory patterns and different prior destructor roots. All native
callees execute, without stubs. Ordered stores, callback arguments and all mapped
memory except 256 stack bytes are checked, including retained marker links and
padding. These cases prove initialization, not later pool allocation/destruction
or asynchronous hardware behavior. All 200 functions in the 13 checked overlay-5
and MSL objects match. Full build, golden EUR ROM, native relink and 107 tests pass.


## Smash Eggs actor finish

[`actor_finish.cpp`](../../src/attack_smash_egg_ov015/actor_finish.cpp) owns
0x020C4570-0x020C4728. It disables actor/egg hit descriptors, selects the finish
animation or completes an airborne support motion, starts pair retreat and sets
the attack's completion flag. The active model pointer is deliberately cached
before resource and animation calls. Phase 9 writes phase 11 before retreat;
other phases write phase 7 after retreat. Packed upper bits survive both writes.
Separate local declarations preserve native register allocation without changing
the order of loads; the complete function matches without inline assembly.

Private `eur_high_egg_finish/live_v1.json` replays 2,110 frames from
`eur_overlay15/item_select83.dst`, using the existing button-only pair-test policy.
The checkpoint comes from the restored-command encounter documented above, not
a normal story encounter entry. Two calls cover phases 9 and 11: 15 ordered
helper calls and five own stores, with complete function guards, return/SP/r4-r11,
the 584-byte attack work, owner prefix, all 70 embedded scene objects and their
nonnull models checked. Native child writes inside these bounds are observations;
hit-descriptor globals, animation-list neighbors and heap lifetimes are not
independently predicted. The final capture shows the battle command wheel and
the attack pointer is cleared. VRAM/palette/OAM hashes are retained observations.
All 104 original saves remain unchanged; this replay applies no RAM fixtures.

`isolated_v2.json` supplements this with 128 ARM946 cases on copied RAM/DTCM:
all 32 phases, both formation parities and both model selectors. Child calls are
stubbed with checked arguments and explicit synthetic mutations to test reloads,
ordering and the cached model pointer. Own ordered stores and all mapped memory
except 256 stack bytes are checked. These cases do not prove child algorithms
or live gameplay coverage of the early phases. Version 1 failed because the probe
changed CPU mode after setting banked SP; the corrected setup passes all cases.
All 24 functions across nine actual overlay-15 objects match; the full build,
golden EUR ROM, zero-difference native relink and 107 tests pass.


## Smash Eggs pair-hit resolution

[`pair_hit.cpp`](../../src/attack_smash_egg_ov015/pair_hit.cpp) reconstructs
0x020C4728-0x020C4A68 without ASM. Phases 8-10 increment the signed hit count
only when `78 * count + height <= 380`, start the pair's return and toggle
the alternate-actor bit. Other phases choose an enemy, calculate power using
double arithmetic (`work.power + 64.0 * count`), roll the hit bonus, apply damage
and start another launch. The alternate actor is randomized only when the
unsigned table threshold is strictly greater than the random roll; equality
toggles the existing bit without consuming another random number. Damage narrows
to a signed halfword for projected HP and an unsigned halfword for the launch.

The shared attack header now declares the damage target as `BattleActor *`,
matching its native dereferences and the existing hammer caller. Threshold
values are 32-bit integers, while threshold boundaries are signed halfwords.
The Smash Eggs context exposes power at offset 260 with size/offset checks.

Private `eur_high_egg_pair_hit/live_v2.json` uses the same restored-command
checkpoint83 and button policy as the finish-transition replay. In 2,110 frames,
one phase-4 hit checks ordered calls/stores, the double-helper ABI and results,
clamped index, threshold lookup and every nested random call's seed/result.
The observed hit has power 222, damage 34 and random roll 32 against threshold
30; it takes the toggle path. All three random calls are checked, including
target selection and the bonus roll. The attack returns to the command wheel.
Full work, party/enemy prefixes, embedded scene objects and models are checked
between caller events. Child effects, target eligibility, hit-bonus decision and
damage result remain bounded observations, not independent child-algorithm
proofs. Graphics captures are observations; all 104 original saves are preserved.

`isolated_v1.json` adds 199 ARM946 cases on copied entry RAM/DTCM. These cover
all phases, both parities, signed count extremes, the height threshold below,
at and above 380, index clamping, random-threshold equality, bonus selection
and signed/unsigned damage narrowing. Original double, RNG/division, clamp and
threshold-lookup routines execute; other engine calls are explicitly stubbed.
All mapped memory except 1,024 stack bytes, ordered writes, helper arguments
and results, SP and r4-r11 are checked. These cases do not establish live
support-hit coverage, object lifetimes or rendering. An earlier live probe
failed on an incorrect RNG guard address; version 2 uses the symbol boundary
0x0202CB6C and passes. All 87 functions in 31 checked objects match; the final
full build, golden EUR ROM, zero-difference native relink and 107 tests pass.


## Smash Eggs sequence and return

[`smash_eggs_sequence.cpp`](../../src/attack_smash_egg_ov015/smash_eggs_sequence.cpp)
owns 0x020C5570-0x020C597C. The controller updates both participants and the egg
pair until the finish flag, both idle checks and the pair's linked-phase check
allow exit. It starts the return movement and polls it in the same update,
wraps the even-formation actor from beyond x=256 to x=-32, and finally restores
actors, hides effects and clears the callback and attack root. A nonnull reward
record adds one item before restoration. Keep the individual model flag writes
and repeated motion queries: the original does not merge them. The initial
two model lookups also remain even though their results are unused.

The function matches without ASM. A bitfield gives the native completion-bit
extraction; an inline absolute-position wrapper retains the original per-axis
deltas. Separate loop counters preserve the native register allocation. Use a
unique source basename: the linker selects object files without directories.

Private `eur_high_egg_sequence/live_v1.json` checks all 278 calls during the
2,110-frame checkpoint83 replay: 228 in phase 0, one in phase 1, 48 in phase 2
and one in phase 3. It checks 1,561 ordered helper calls and 11 own stores,
including one screen-edge wrap, phase-1 fallthrough and callback/root clearing.
The same restored-command encounter and button policy as the preceding Smash
Eggs probes are used, with no RAM edits. Full work, party prefixes, embedded
objects and models are checked between events. Child effects are bounded
observations; motion-channel and virtual-animation query results are observed.
There is no reward on this route. The final capture shows the command wheel;
VRAM/palette/OAM hashes are observations, and all 104 original saves are unchanged.

`isolated_v2.json` checks 64 copied-RAM ARM946 cases for the exit gates, both
parities, x=256/257, movement/animation states, optional reward and signed unknown
phases. Native object/model lookup and idle/linked helpers execute; other children
are explicit stubs. All mapped memory except 256 stack bytes, ordered own writes,
arguments, SP and r4-r11 are checked. The reward case proves the caller's item
and quantity arguments, not actual inventory mutation or live reward coverage.
Version 1 failed because Unicorn hooks also see conditionally skipped ARM
instructions; version 2 checks CPSR conditions before invoking the live oracle.
All 26 functions in the 11 linked overlay-15 objects match. Full build, golden
EUR ROM, zero-difference native relink and 107 tests pass.


## Smash Eggs allocation and entry setup

[`smash_eggs_initialize.cpp`](../../src/attack_smash_egg_ov015/smash_eggs_initialize.cpp)
owns 0x020C5B4C-0x020C5E4C. It allocates the 584-byte work, initializes both
controllers, loads resources and sets entry coordinates. Formation >=4 also
initializes support hit motion. Resource indices use the 27-bit resource field;
home tables use formation parity. The entry callback uses the common action-actor
signature and accesses its party prefix. Shared declarations now live in
`overlay010_attack.h`. The entire function matches without ASM.

Private `eur_high_egg_initialize/live_v1.json` checks one constructor at frame117
of the 2,110-frame checkpoint83 replay: formations4/5, no badge, center176/148,
28 helper calls and17 own stores. Allocation header/root/584-byte extent, caller
arguments, writes and preserved registers pass. Child effects in tracked work,
actors, objects and models are observations; resource and heap internals are not
independently proved. No RAM edits; the final screen returns to command selection.

`isolated_v1.json` covers432 copied-RAM cases: all36 formation pairs, badge on/off,
center -32768/176/32767 and zero/nonzero work fill. Object lookup and badge table
addition execute natively; other helpers are explicit stubs. Ordered caller
writes,60 badge halfword updates and mapped memory outside256 stack bytes pass.
This does not establish live badge or synthetic-allocation lifetime coverage.
All58 functions in23 affected linked objects match; full build, golden EUR ROM,
native relink and107 tests pass. All104 original saves remain unchanged.


## Common attack-work allocation

[`attack_work_create.cpp`](../../src/attack_common_ov010/attack_work_create.cpp)
owns 0x020C2978-0x020C2A64. `Overlay10Attack_CreateWork` allocates and clears the
requested extent, stores the user and unsigned target halfword, snapshots enemy
state and averages party stats. Action-item flags select the averaging count.
Only bit12 of the battle mode word changes; the mode argument contributes its
low bit. The caller must supply a valid action item: the native lookup tests
bit12 rather than the entire tag and does not handle a null result afterwards.
Shell and Cannonballer callers explicitly cast their party prefixes and returned
specialized work views; their factory declarations now share the common types.

Private `eur_high_attack_work/linked_live_v1.json` checks the factory during the
checkpoint83 Smash Eggs replay:584-byte allocation and clear, five child calls,
five own stores and return/register state. Collection, snapshot and averaged-stat
outputs are observations confined to their work ranges.48 copied-RAM cases in
`isolated_v1.json` execute the native clear for304/584/612 bytes with trailing
guards, exercise four count indices and mode values, and preserve targetFFFF.
Other children are stubs; no live alternate-attack or allocator-internal coverage
is claimed. Eight functions in five linked objects remain exact. Golden ROM,
native relink,107 tests and preservation of104 original saves pass.


## Title model resource loading

[`title_model_resources_load.cpp`](../../src/title_ov006/title_model_resources_load.cpp)
selects the first matching archive prefix, indexes 20-byte model or 8-byte other
records, and applies the language offset only for localized entries. Its four
texture-table slots distinguish borrowed data from allocated conversion arrays;
each ownership bit follows the corresponding size query. Allocation sizes round
down to whole halfwords. A valid archive prefix and resource slot are required.

The cold-boot save-83 route checks all three model loads, including a localized
entry and an auxiliary archive read. It reaches the visible load menu in 1,783
frames without RAM edits. Checks cover the complete function, child arguments,
ordered stores, resource records, allocation extents and preserved registers.
Generated tables are observed only within their requested output ranges.

Private `build/runtime/eur_high_title_resource/live_v2.json` and `isolated_v1.json`
record the evidence. The 48 copied-RAM cases cover all ownership combinations,
both entry strides, language indices 0/1/5, slots 0/15 and odd size rounding.
Their archive, allocation and conversion helpers are explicit stubs; these cases
do not establish alternate live languages, conversion algorithms or heap lifetime.
The full ROM, linked loader and caller objects match; 107 tests pass and all 104
original saves are unchanged.


## Battle sub-screen render stage

[`battle_subscreen_render.cpp`](../../src/battle/battle_subscreen_render.cpp)
walks the sub-screen model list, updates anchors for owned models, calls each
renderer and then prepares matrix animation and OAM. It reads each successor
after the virtual callback and reloads OAM counts after preparation calls.
The parent forwards the battle context in `r0`; the stage does not read it.
Keeping that unused parameter preserves the observed call ABI. Both functions
have `void` interfaces: the parent's sole caller ignores the result, and the
stage's final call is the existing `void` OAM builder. Their full compiled ranges
remain exact, without ASM or an invented C return value.

Private `build/runtime/eur_high_subscreen/live_v2.json` checks all 2,110 stage
calls in the save-83 Smash Eggs replay: 8,079 model visits, 5,969 owner branches
and 28,818 child calls. Arguments, order, forwarded context and preserved
registers are checked; receiving model-prefix changes are observations. Child
algorithms and allocation lifetimes are outside this orchestration check.
Twelve copied-RAM cases in `isolated_v1.json` use explicit child stubs to test
empty lists, mixed ownership, callback changes to successors and changed OAM
counts. Full RAM and registers are checked, excluding the temporary stack area.
The final live capture shows the battle command menu; graphics memory is captured
as evidence, without an independent rasterization claim.


## Mix Flowers allocation and resource setup

[`mix_flowers_initialize.cpp`](../../src/attack_mix_flower_ov016/mix_flowers_initialize.cpp)
allocates 17,292 bytes, initializes four participants and 32 embedded paired
effects, prepares ten resource buffers and installs the entry callback. Formation
bit 0 determines the adult ordering and the side-dependent resources. The two
resource-table aliases at `0x020C72F0` and `0x020C72F4` retain their eight-byte
stride. Resource 54's copy flag is cleared without changing neighboring bits.
The center's X coordinate is a signed halfword; the entry offset stores X minus
128 as a halfword. The optional badge adjustment advances the motion table.

Private `build/runtime/eur_high_mix_initialize/live_v1.json` checks the complete
initializer, its 82 child calls and seven own stores on the save-83 Mix Flowers
route. The attack returns to the battle menu. All six captures, recorded inputs
and four graphics-memory dumps match the preceding Mix Flowers replay.
`isolated_v1.json` adds 72 copied-RAM ARM946 cases for formation indices, both
badge states, signed-coordinate limits and nonzero initial bytes. Those cases
execute native lookup and motion-table helpers; other children are explicit
stubs. Live child writes are bounded observations, not independent proofs of
resource loading, buffer payloads, heap lifetimes or rendering. Full caller
matching and the checked runtime paths do not establish every child branch.


## Shop member-selection eligibility

[`shop_member_selection.cpp`](../../src/shop_ov009/shop_member_selection.cpp)
checks the party member selected by the shop's two-by-two grid. It rejects
absent members, then either accepts the scene's bypass mode or asks whether
the selected item can be equipped. The grid table contains full words, but the
member ID uses only the low byte. The presence flags retain their existing
interior data alias; workspace byte `+0x9B` controls the bypass.

Private `build/runtime/eur_high_shop_member/live_v2.json` records an allowed
and a denied selection after buying Shell Slacks and opening the change-now
prompt. The controlled shop-entry command is restored before ordinary navigation;
all 104 original saves remain unchanged. The final capture shows the field again.
The first route reached only purchase confirmation and failed the coverage check;
its `live_v1.json` remains separate. Ninety-six copied-RAM cases in
`isolated_v1.json` cover all four cells, absence, bypass, both clothing groups,
badges and row wrapping. The predicate and query helpers execute without stubs.
Checks cover arguments, returns, preserved registers and unchanged RAM; these
fixtures do not add live object-lifetime or graphics coverage.


`ShopScene_GetMemberGridPosition` performs the reverse lookup, comparing full
32-bit table entries in row-major order. It writes column before row, returns
the first match, and falls back to `(0, 0)`. Its sole native caller uses the
outputs at task offsets `+40` and `+44`; it does not consume a return value.
Private `build/runtime/eur_high_shop_grid/live_v1.json` verifies one lookup
while opening the equipment prompt, including both stores and the complete
72-byte receiving task. The earlier eligibility and buying-marker checks remain
active; the final capture shows the field. Forty-eight copied-RAM cases in
`isolated_v1.json` cover all members, missing IDs, full-word comparisons,
duplicate entries and aliased output pointers. They check ordered writes and
all RAM outside the 16-byte native stack frame, without helper stubs. The live
route buys equipment only in copied emulator state; original saves are unchanged.
These checks do not establish every gameplay route or independent rendering.


## Bro Flower participant palette pulse

[`participant_palette_fade.cpp`](../../src/attack_bro_flower_ov014/participant_palette_fade.cpp)
implements the render override installed by `Overlay14Participant_StartFade`.
It skips nonzero render passes, prepares the primary model and obtains its
512-byte palette buffer. Unless both dirty and buffered flags are set, it copies
the source palette using the original overlap-safe direction. The effect adds
RGB555 color with saturation; the interpolation amount is
`floor(min(frame, 16-frame) * 31 / 13)`. Frame advances through 0..15. A stop
request removes the override only at frame zero, and that call still submits
the model for drawing. The fade record is embedded at participant `+28`, rather
than separately allocated.

The observed model vtable at `0x02050BB0` maps offsets `0x9C`, `0xA0`, `0xA4`
to the already linked palette-buffer, palette-set and palette-source wrappers.
The shared C/C++ interfaces now use their pointer contracts and names. Existing
Battle VM and status-visual callers refresh palettes, not animation IDs; their
recompiled code remains exact. The callback and current Flower units, those two
callers and resident palette wrappers total 62 checked functions in 16 actual
source objects. Obsolete object files from earlier source organization are not
part of that set; select objects through the current linked manifest.

Private `build/runtime/eur_high_flower_fade/evidence_fire83_v1.json` checks
384 pulse calls, two stop calls and 386 skipped passes over 2790 frames.
`evidence_ice83_v1.json` checks 256 pulse calls, two stops and 258 skipped passes
over 2830 frames, using the previous 41 automatic button presses. Both routes
preserve their prior input sequences, screenshots and final display-memory
hashes and return visibly to battle command selection. No RAM fixtures are
used; all 104 original saves remain unchanged.

The oracle independently checks call arguments, palette queries and copies,
all 256 resulting colors, the effect template, frame/stop stores and preserved
registers. It checks the full attack work, actors, scene object, primary model,
palette and touched roots. Model preparation/drawing writes are accepted only
within the 440-byte model record; other renderer state and hardware
rasterization are not independently proved. `isolated_v1.json` adds 162 ARM946
cases for both variants, frames 0/1/8/9/15, stop requests, all dirty/buffered flag
combinations, both copy directions and nonzero passes. Palette operations run
natively; preparation and drawing are no-op stubs. The isolated checks predict
all main-RAM changes and preserve DTCM outside a bounded 512-byte stack region.
They add no gameplay, allocation-lifetime or upload coverage.


## Bro Flower enemy effect stages

[`enemy_effect_stage.cpp`](../../src/attack_bro_flower_ov014/enemy_effect_stage.cpp)
raises an enemy's visual effect stage at 1, 6 and 12 successful hits. It can skip
intermediate stages and never lowers the current stage. Six bytes at Flower
work offset 8996 hold these stages; the existing raw eight-byte view remains
available. The receiving scene objects are embedded in the attack work, three
260-byte slots per enemy starting at offset 9004, with a 780-byte enemy stride.
A stage change positions the selected slot, sets its depth relative to the
enemy, starts its animation and reloads the resulting model pointer before
writing its scales. Fire uses 128/153; Ice uses 256/256.

The original palette-check routes did not execute this helper. The ordinary
secondary throws reach it: for combined formations 4/5, use X/Y rather than
A/B once a level-two projectile is ready. Private
`build/runtime/eur_high_flower_stage/evidence_fire83_v1.json` and
`evidence_ice83_v1.json` each record 58 calls, three stage changes and 55 unchanged
returns, using 58 automatic button presses and no RAM edits. Fire runs 2790
frames and retains the successful discovery route's captures; Ice runs 2830
frames on its new input sequence. Both final captures show battle command
selection. The earlier zero-call discovery reports remain separate.

The oracle independently checks thresholds, ordered arguments and stores,
position-helper outputs, depth and scale values, and preserved registers.
Checks cover the complete 13724-byte attack work, enemy actor and scene object,
touched primary models and roots. Movement and animation helper writes remain
bounded observations within the receiving object/model; they do not establish
allocation lifetimes, list neighbors or graphics correctness. The inherited
palette oracle remains active. All 104 original saves are unchanged.

`isolated_v1.json` adds 350 ARM946 cases across all six enemy slots, both
variants, unchanged and skipped stages, exact thresholds, 65535 hit counts and
the position helper's special resource cases. Lookup and position calculation
execute natively; movement applies fixture coordinate deltas, while an animation
stub replaces the model pointer to test the caller's reload. The checks predict
all main-RAM changes, including 16-bit coordinate/depth wrapping, and preserve
DTCM outside a bounded 64-byte stack region. These fixtures add no live gameplay,
model lifetime, animation or rendering coverage.


## Field sprite animation graphics setup

[FieldSpriteAnimation_InitializeGraphics](../../src/field/field_sprite_animation_start.c)
reconstructs `0x020BF33C..0x020BF510` (468 bytes) in ordinary C. An active
animation returns immediately. Otherwise it selects 3D BG0, configures geometry
and orthographic projection, preserves all five ordered DISP3DCNT writes, and
sets seven matrix elements. Existing alpha blending is reused; otherwise the
animation records ownership so Stop can disable it later.

The pointer at FieldSystem +0x3AC owns a 3,920-byte GameSpriteWindowManager,
allocated by FieldSystem's resource setup. Its +0x54 word is base.display, and
bit 25 fixes main-window scrolling. The former ModelRenderDescriptor view was
incorrect; Start and Stop now share an explicitly bounded window prefix. The
prefix is not the allocation size.

A Save 65 cold replay uses the established restored decoded-command fixtures
to enter room 359 and start its part-0 effect at script offset 0x110A. Across
3,333 frames, one initializer enables blending. The oracle checks all 18 own
stores in order and eight helper calls, full sprite state (10,324 bytes),
FieldSystem (952 bytes), window manager (3,920 bytes), SP and r4-r11. Register
operands are captured immediately before their reads. Real graphics helpers
execute, but their hardware effects are observational. The final capture shows
the field; this controlled route does not establish ordinary story entry.

Another 108 isolated ARM946 cases cover active returns and both blend branches
with varied preserved flag/register bits: 1,296 stores and 576 helper entries.
These use no-effect helper stubs and modeled read/write I/O. They check caller
arguments, full RAM/DTCM, synthetic records, surrounding scratch memory and ABI;
they do not verify hardware timing, rasterization or live object lifetimes.
All 104 original saves remain unchanged. The final actual source objects match,
and full build/native relink plus 107 tests pass.

Private reports are `build/runtime/eur_high_sprite_graphics/room359_v2.json` and
`isolated_v1.json`; producers are `probe_sprite_graphics.py` and
`check_sprite_graphics_isolated.py` under `build/analysis/high_effort_50_to_55/`.
The failed v1 probe and its source are preserved: its register accessor did not
translate Capstone's `ip` alias to `r12`. The successful probe also expands the
old 96-byte descriptor check to the actual 3,920-byte window allocation.


## Party preparation for battle entry

[FieldSystem_PreparePartyForBattle](../../src/field/field_battle_transition.cpp)
reconstructs `0x0206B85C..0x0206BA2C` (464 bytes). The encounter setup and its
entry wait routine call it before the battle scene handoff. The signed five-bit
field at FieldSystem +0x258, bits 6..10, selects preparation for separated
partners, Bros-Ball, an active state record, auxiliary placement or recovery.
The save request selects party 1 only when its formation equals 1; bit 8 of
the halfword at save +0x560 selects the initiating member. Both callers discard
the return register. The member flag cleared at +10, bit 8, gates ordinary
entity updates in FieldArea_UpdateEntities.

A normal 353-frame Bros-Ball navigation attempt from the compatible Save 83
checkpoint reaches neither the wrapper nor encounter setup. Separate controlled
calls at guarded Field VM boundaries test both parties, both member selectors
and signed preparation modes -16/0/1/4/7/15: 24 calls and 52 ordered stores.
Full system (952 bytes), party manager (16,764), both selected members (1,440
each), save prefix (1,380), SP and r4-r11 are checked. The mode-4 state record
is required to belong to that party's embedded records. Inputs and induced
changes, registers and the already-decoded VM stack are restored before each
of the 24 original commands completes. No helper or stub executes in these
controlled branches. The final capture shows the original field scene.

An additional 264 isolated ARM946 cases cover formations -1/0/1, both member
selectors with different movement modes, both special-contact flags, all
preparation branches and representative signed/default values. They check
1,152 ordered stores and call arguments, full RAM/DTCM and synthetic records,
surrounding scratch memory and ABI. FieldVertical_Stop executes its real native
48-byte implementation 120 times. Seven other party helpers use no-effect ABI
stubs (120 calls); their internal transitions are not validated here. These
cases do not establish natural encounter, live special-action or graphics
coverage. All 104 original saves remain unchanged.

The actual compiled wrapper matches; the complete build produces the golden
ROM, the native relink has zero differences, and all 107 tests pass. Private
reports are `build/runtime/eur_high_party_prepare/controlled83_v1.json`,
`isolated_v1.json` and `discovery_ball83_v2/evidence.json`. Producers are
`probe_party_prepare_controlled.py` and `check_party_prepare_isolated.py` in
`build/analysis/high_effort_50_to_55/`; discovery uses `tools/runtime_probe.py`.


## Field handoff to battle

[FieldSystem_PrepareBattleScene](../../src/field/field_battle_transition.cpp) owns
`0x0206B1FC..0x0206B3A0` (420 bytes). Before releasing the field, it saves each
screen's three background scroll coordinates: six signed halfwords at save
+0x564 for X and six at +0x570 for Y. Values use arithmetic Q8 shifting followed
by halfword truncation. These extend the checked save prefix to 1,404 bytes;
the earlier 1,380-byte transfer view is not an allocation-size claim.

The airborne-entry bit (FieldSystem +0x258, bit 4) requests trail-copy stopping.
Preparation mode 1 ends the selected member's hammer swing; modes 2/3/5/6 reset
the party's action state. Common field teardown then runs. Finally, two separate
stores set the persistent transition fields to 23 and 46 while preserving
the flags published by that helper. The disassembly is authoritative here:
pseudocode collapses these two stores into one expression.

A 706-frame Save 55 replay reaches the battle command menu using the established
room-306 scripted encounter command fixture; command bytes and script cursor
are restored at encounter setup. One handoff checks all 14 own stores and the
teardown call, full live system/areas/backgrounds/party records and save prefix
before teardown, plus SP/r4-r11. The teardown's allocations, copies, releases
and graphics effects are observational. Old object records are retired across
that call; the save prefix remains checked, and persistent state is captured
at helper return before independently checking the two final stores. This
route executes preparation mode 0 and does not cover optional party helpers.

Another 264 isolated ARM946 cases check formations -1/0/1, both member indices,
airborne-entry selection, every preparation branch and signed/default values.
Two scroll patterns include signed 32-bit extremes, negative Q8 boundaries and
halfword truncation. All 3,696 own stores, call arguments, complete RAM/DTCM,
synthetic records and surrounding scratch memory are checked. Four helpers are
stubbed; the teardown stub supplies two distinct persistent-state patterns to
verify the caller preserves the newly returned flags. Helper internals, live
lifetimes and hardware behavior are not established by these isolated cases.

Actual handoff, party-preparation and dispatcher objects match. Golden ROM,
zero native differences and 107 tests pass; all 104 original saves are unchanged.
Private evidence: `build/runtime/eur_high_battle_handoff/{entry55_v1,isolated_v1}.json`.
Producers: `probe_battle_handoff.py` and `check_battle_handoff_isolated.py` under
`build/analysis/high_effort_50_to_55/`. The final battle capture was inspected.


## Battle entry wait gates

[FieldSystem_IsBattleEntryPending](../../src/field/field_battle_transition.cpp) covers
`0x0206B3A0..0x0206B85C` (1,212 bytes). It returns one while preparation remains
pending and zero when the field dispatcher may advance. The party and initiating
member come from the encounter request, with formation 1 selecting the second
party. FieldSystem +0x258 bit 4 updates delayed trail visibility; bit 5 waits for
the selected member's vertical velocity to become nonpositive. At that point it
clears bit 5, freezes that member's updates and invokes party preparation if the
signed five-bit preparation mode is nonzero.

Other branches wait for hammer states 25/26 and a small state-record field,
separation/ball clearance, auxiliary state byte 3, or both recovery states at
most 3. Mode 4 changes to mode 3 when the leader reaches locomotion state 22.
Ready paired branches freeze both members. Approaches -2/-1 rebind the selected
member's resources, using its current facing direction, update-bounds 1 and
speed 256. Modes 2/3 use shared resource `55 + member index`; modes 5/6 use
`57 + (member index & 1)`. The native range test for hammer states adds the
wrapped 16-bit negative bound; the state-record check extracts an 11-bit field.
These explain the first candidate's instruction differences without register
permutations.

A 440-frame controlled replay makes 96 native calls on the live field system:
both parties and both selectors across 24 waiting/release scenarios. All 76
ordered stores, return values, full system (952), party manager (16,764), selected
members (1,440 each), save prefix (1,380), SP and r4-r11 are checked. The fixtures
clear airborne entry and use approach zero, so no helper executes. Every input
and induced change, decoded VM stack and register state is restored before the
original VM resumes; all 96 original calls complete. The final field capture
was inspected. This is controlled function coverage, not ordinary battle entry.

Another 3,888 isolated ARM946 calls use copied live RAM/DTCM and synthetic records.
They cover formations -1/0/1, both selectors and airborne bits, approaches
-8/-3/-2/-1/0/7, plus signed preparation modes and positive/zero/negative velocity.
All 4,248 ordered stores, return values, helper register/stack arguments,
full RAM/DTCM and scratch outside the measured stack, SP and r4-r11 are checked.
Trail visibility (1,944 calls), party preparation (1,440) and renderer rebinding
(144) are explicit no-effect stubs; their internals and live graphics/lifetimes
are outside this isolated check. The first fixture producer failed on a missing
optional elapsed field; its log/source are retained, and the corrected producer
completed with exit zero. No game-code change was needed.

Actual wait/handoff/preparation/dispatcher objects match; golden ROM, zero native
differences and 107 tests pass. All 104 original saves are unchanged. Private
reports: `build/runtime/eur_high_battle_wait/controlled83_v1.json` and
`isolated_v2.json`; producers `probe_battle_wait_controlled.py` and
`check_battle_wait_isolated.py` in `build/analysis/high_effort_50_to_55/`.


## Time-hole arrival participants

[FieldSystem_PrepareTimeHoleArrival](../../src/field/field_time_hole_arrival.cpp)
covers `0x02069B24..0x02069D38` (532 bytes). The field VM passes four writable
32-bit entity indices, an anchor index and four arrival directions. A missing
index is -1. Baby entities in movement mode 6 are excluded by writing -1 back
into the caller's argument array; the old declaration incorrectly marked that
array const. The function saves behavior, disables visibility and updates,
places accepted participants at the anchor and records their angle/radius.
Party resource setup is called only for entity subtypes 0/1.

The shared 292-byte time-hole state now exposes the participant mask at bits
14..17, four direction bytes at +4, four signed index bytes at +20, four angle
halfwords at +36 and four Q12 radii at +44. Departure and tunnel modes reuse
this storage, so the previous opaque views remain available. Each squared
coordinate is rounded and shifted separately before their sum reaches FX_Sqrt.
The initial private C++ candidate and actual integrated object both match fully;
no inline assembly is needed.

Twelve controlled native calls on the Save 83 field state cover missing indices
and excluded-baby masks, using two live anchors. The real 104-byte preparation
helper is independently checked, including its complete 292-byte reset, anchor
index and coordinates. All 88 own stores, full live system (952), main area
(11,216), manager (16,764), anchor/baby records (1,440 each), argument scratch,
SP and r4-r11 are checked. All inputs and induced changes, borrowed argument
storage, decoded VM stack and registers are restored before each original VM
call completes. The final field capture was inspected. This route does not
exercise an accepted participant's renderer or a natural time-hole transition.

Another 384 isolated ARM946 cases exercise every four-slot participation mask,
baby movement modes 0/6, saved behavior modes 0/3/7, two anchors and two coordinate
and direction patterns. These execute the actual arrival function and real
preparation, behavior, position and facing helpers. They check 7,104 own stores
and 8,928 stores from those helpers, full RAM/DTCM/scratch outside the measured
stack, all call arguments, SP and r4-r11. Memory clearing is modeled; party
resource setup, renderer refresh and two virtual callbacks use no-effect stubs.
The atan/sqrt stubs assert the known-axis inputs (65,536, 0) and 1,048,576 and
return 16,384 and 65,536. SDK math internals, graphics, hardware and live resource
lifetimes are not established by these isolated cases.

Golden ROM, zero native differences and 107 tests pass. Arrival, preparation,
activity helpers and the field VM's actual objects match. All 104 original saves
are unchanged. Private reports: `build/runtime/eur_high_time_hole_arrival/`:
`controlled83_v1.json` and `isolated_v1.json`. Producers in
`build/analysis/high_effort_50_to_55/`: `probe_time_hole_arrival_controlled.py`
and `check_time_hole_arrival_isolated.py`.


## Battle Rumble Pak scheduling

[Battle rumble](../../src/battle/battle_rumble.c) reconstructs
`0x02065E30..0x02066004` (468 bytes), replacing symbolic ASM with C. The old
`BattleScreenEffect_*` names described these functions incorrectly: their two
playback paths call the resident timed/repeated Rumble Pak drivers. Callers,
the VM reference and maintained ASM symbols now use `BattleRumble_*` names.
The playback functions have no meaningful return contract; callers discard r0.
Serialized opcode names for 0x101/0x102 retain their historical screen-effect
spellings; their argument/effect documentation now identifies rumble playback.

Four 12-byte slots live at +70,484 in the 70,976-byte common workspace reached
through `data_ov002_020c0660` (0x020C0660). Each slot contains a signed 32-bit
delay, signed 16-bit duration/repeat amount, mode byte, zero-based pattern byte
and callback pointer. A null callback marks a free slot. Pattern arguments are
one-based; nonpositive values are ignored. Zero delay calls the driver directly;
any nonzero delay queues in the first free slot, or is dropped if all four slots
are occupied. Queued amounts truncate to 16 bits and are sign-extended when
played; immediate amounts retain all 32 bits. Positive delays decrement to zero
and play on the following update. A nonpositive delay dispatches mode 0/1 and
clears the callback; other modes only clear it.

The first candidate matched both playback functions. Native LDRSH established
the signed amount field; the free-slot loop advances a pointer before adding the
fixed workspace offset. Those corrections made all four functions exact.
Integration also required reverse source order for MW's function emission,
forward declarations and synchronized maintained ASM references. Failed build
logs v1-v3 are retained separately; the final v4 gate passes 107 tests, the
golden ROM and native relinking with zero differences. All actual compiled
functions match their complete native ranges, including literal pools.

A 400-frame replay from the derived Save 55 battle-menu state checks 66 controlled
calls at the guarded common-frame boundary and 112 ordered stores. Fixtures cover
both playback types, immediate/positive/negative delays, ignored patterns,
pattern-byte truncation, signed queued amounts, all 16 slot-occupancy masks and
both recognized/one unknown callback modes. The real free-slot function executes
40 times; each resident playback driver executes seven times. Full common
workspace, root, 28-byte rumble control region, stack and callee-saved registers
are checked. Every fixture and induced write, borrowed decoded stack and register
value is restored before the original common-frame call completes; all 66 original
calls complete. The final Jump-menu capture was inspected, and all 104 original
saves are unchanged. The actual absent-Pak flag is zero; this verifies that driver
branch, not enabled cartridge playback or physical vibration. These are controlled
calls, not natural attack-trigger coverage, and no isolated replay is claimed.

Private evidence: `build/runtime/eur_high_battle_rumble/controlled55_v1.json`.
Producers and failed/successful build logs are under
`build/analysis/high_effort_50_to_55/`, including `probe_battle_rumble.py`,
`battle_rumble_oracle.py` and `battle_rumble_build_v4.log`.


## Reward-model cleanup and rumble stop

[Reward cleanup](../../src/battle/battle_reward_model_cleanup.cpp) owns
overlay 2 `0x02065D40..0x02065DFC` (188 bytes). The reward initializer at
`0x0206DFD0` calls it in phase zero, at `0x0206E008`. It gets object ID 16
and traverses twelve contiguous, embedded 260-byte scene slots. It stops each
alternate model (+196), unlinks the embedded palette (+204), rereads the model
pointer, calls deleting virtual slot +20 when present and clears the pointer.
It then stops, rereads, deletes and clears the primary model (+192). Virtual
stop uses slot +40. The explicit small inline deletion helper retains the
native null check; no inline assembly was needed.

`BattleRumble_Stop`, appended to [battle rumble](../../src/battle/battle_rumble.c),
owns `0x02065DFC..0x02065E30` (52 bytes). It calls resident `GameRumble_Stop`
only when the byte at `0x0205A00C` is nonzero. It leaves the four delayed
request slots intact. Common VM opcode 0x103 now calls the named wrapper;
its historical serialized opcode name is retained.

The controlled reward route uses the Save 55 battle state with SHA-1
`1e565ff9db7da1e8dfc463ff6e657bbdbe8aa38e`. At the guarded idle battle update
it sets phase 0x5029, then injects reward arrays at initializer completion.
The 89-frame replay checks one cleanup, two alternate stop/delete pairs,
two palette unlinks and two pointer stores. Full twelve-slot storage, root,
lookup, list neighbors and roots, call arguments, SP and r4-r11 are checked.
Real virtual methods execute with native guards; their model-internal, heap
and resource-list effects remain observational. Freed models are not read
after deletion. No primary model is present in this live call.

The first oracle returned immediately from a helper-return hook that was
also an owning-pointer store. Its next comparison therefore failed at offset
196. Preserved v1 fails; v2 processes both events and passes without changing
the matching game code. Full 4 MiB main RAM and 16 KiB DTCM match the restored
savestate before 30 neutral frames. The restored battle menu was inspected;
the transitional reward capture is not a completed victory-screen check.
This is a controlled phase entry, not a natural victory route.

A separate 400-frame live replay calls the stop wrapper once at the guarded
common-frame boundary with the actual absent-Pak flag zero. Full 70,976-byte
workspace, root, 28-byte rumble controls and ABI are checked, and all fixture
writes, decoded stack and registers are restored before the original frame
call completes. Its final battle-menu capture was inspected.

Twenty-three isolated ARM946 cases use copied RAM/DTCM. Twenty cleanup cases
combine five slot occupancy patterns with four virtual-stop pointer-clear
masks. The real 148-byte palette unlink executes 112 times, with 560 ordered
helper stores; 112 caller stores are checked. Three stop cases use enable
bytes 0, 1 and 255. Lookup, virtual stop/delete and rumble-driver calls are
stubbed with checked arguments; stop stubs optionally clear the owning slot.
Full RAM/DTCM and 128 KiB scratch outside measured stack writes, ordered stores
and ABI are checked. These cases cover missing live branches but do not prove
model lifetimes, IRQ behavior, enabled hardware or visible gameplay.

All 104 original saves are unchanged. Full verification passes 107 tests,
the golden ROM and native relinking with zero differences. All seven affected
compiled functions match, including the extended rumble unit and common VM.
Reports are in `build/runtime/eur_high_reward_cleanup/`
(`evidence_reward55_v1.json`, `evidence_reward55_v2.json`, `isolated_v1.json`)
and `build/runtime/eur_high_battle_rumble/stop55_v1.json`. Producers are
`battle_reward_cleanup_oracle.py`, `probe_battle_reward_cleanup.py`,
`probe_battle_rumble_stop.py` and `check_battle_cleanup_isolated.py` under
`build/analysis/high_effort_50_to_55/`. The failed oracle is preserved as v1.


## Battle callback and compact render helpers

[Attack callback installation](../../src/battle/battle_attack_callback.c) at
overlay 2 `0x020722AC..0x020722D8` clears the attack-work timer at +0 and
bit 0 of its flag byte at +16, then stores the callback at actor +120. It
does not immediately invoke the callback. Attack-specific allocations share
that prefix; later layouts remain separate.

[OAM transform initialization](../../src/battle/battle_oam_transform_init.c)
at `0x0206B08C..0x0206B0C0` traverses count twelve-byte entries, writes both
Q8 scales as one and zeros rotation/offsets. It preserves the flags word at +8;
a nonpositive count writes nothing. The shared transform exposes these packed
words through unions without changing its layout.

[Saved-primary rendering](../../src/battle/battle_impact_saved_primary.c) at
`0x0206B5C0..0x0206B5F0` forwards the particle, its object's primary model and
three sign-extended halfword coordinates to `BattleImpactParticle_RenderSavedModel`
at `0x0206B0C0`. That 1,280-byte renderer remains native and contributes no
new C/C++ coverage. A private draft shares much of the existing impact renderer,
but differs in coordinate conversion and register allocation; it is deferred.
Its native fifth argument is read as a full word, unlike the tested narrow-
parameter draft's halfword load.

The Bro Flower replay uses the Save 83 battle-menu state with SHA-1
`21d2e64a24b389689627292539103880c6761b47` and ordinary recorded inputs.
Across 2,790 frames it verifies four callback installations (three nonnull,
one null), twelve ordered stores and 96 wrapper/helper calls. Callback checks
cover the full 13,724-byte attack allocation and 401,416-byte battle context.
Wrapper checks cover the 36-byte particle task, 260-byte scene object, root,
primary-model selection and signed coordinates. The real renderer executes,
but its model/global/graphics effects are observational. SP and r4-r11 are
checked at return. The attack work is released and the battle menu is visible
at the end; no RAM edits are used. Failed v1 used unsupported host alias `ip`;
corrected v2 uses `r12` and passes with unchanged game code.

A separate 400-frame Save 55 replay makes five controlled initializer calls
with counts -1, 0, 1, 2 and 4. Synthetic transform entries temporarily occupy
four slots within the live common workspace. Fourteen ordered stores, unchanged
flag words (including nonzero sentinels), full 70,976-byte workspace, root,
28-byte rumble controls and ABI are checked. Fixture/decoded-stack/register
state is restored before each original common-frame call, and all five finish.
This tests initialization in the emulator, not a natural allocation route or
rendering those fixture entries. Final captures from both routes were inspected.
All 104 original saves remain unchanged. No isolated replay is claimed.

Reports: `build/runtime/eur_high_battle_small_controls/flower83_v1.json` (failed),
`flower83_v2.json` and `init55_v1.json` (passed). Producers under
`build/analysis/high_effort_50_to_55/`: `probe_battle_small_controls.py`
and `probe_oam_transform_init.py`. Full verification passes 107 tests, the
golden ROM and zero native differences; selected actual source objects are
compared separately in `battle_small_controls_actual.txt`.


## Mix Flower paired-effect update

[The updater](../../src/attack_mix_flower_ov016/pair_effect_control.c), overlay 16
`0x020C4910..0x020C4B18`, advances animation, convergence and reset phases of
each 524-byte paired effect. The attack allocation holds 32 slots at +400;
its 32-byte controller is at +17168. The first active-model pointer is cached
across the reset-request helper. Phase 4 either tests projected Y <= -160,
checks controller radius, or waits for animation channel 3. The secondary
object always follows the primary through the existing relative-position helper.
Separate phase 5 and 6 arms preserve the native duplicated reset blocks.

The Save 83 replay starts from the derived battle-menu checkpoint with SHA-1
`21d2e64a24b389689627292539103880c6761b47`. Recorded navigation and 665 guarded
automatic button inputs use no RAM fixtures. Across 4,270 frames, 49,216 calls
are observed and 39,367 fully checked: every nonidle call, plus the first idle
call for each of 32 slots. Checked phase counts are 0:32, 1:61, 2:633, 3:897,
4:37740 and 6:4. There are 673 ordered caller stores, 665 growth/arrival calls
and one controller-animation start. Existing animation checks also verify one
attach/start/release sequence. The final battle-menu capture was inspected.

Checks cover the full 17,292-byte attack allocation, owned scene objects and
models, controller, camera, palette/render/texture-list neighbors, ordered
arguments/stores and ABI. Radius results and bounded animation/reset/spawn
effects are observed at helper return; this does not independently verify
their algorithms. The 665 newly observed renderers and palette records have
validated allocation extents and ownership; external list/root changes are
derived independently. Allocation internals, later releases and rendered pixels
are not proved by those checks. Failed v1 omitted a newly inserted renderer's
external link; preserved v2 corrects the oracle and passes with unchanged game code.

Another 148 isolated ARM946 cases use copied main RAM/DTCM from the earlier
Mix Flower replay, synthetic records, the actual compiled updater and real
model lookup, projection, channel and position helpers. Cases cover phases
0/1/2/3/5/6/7/31, controller phases 0..6/31, projected Y -161/-160/-159,
radius outcomes, kinds 0/1/2/255 and moving versus immediate position updates.
They check 60 caller stores, 1,182 real helper stores, full memory outside
measured stack writes and ABI. Reset, animation, controller start, radius,
growth, spawn and finish helpers are explicit stubs with checked arguments.
These cases supplement missing live paths; they do not establish gameplay
lifetimes, heap behavior or hardware timing. The first host producer failed
while writing its output due to a shadowed path variable; v2 passes.

All 104 original saves remain unchanged. Full verification passes 107 tests,
the golden ROM and zero native differences; 48 functions in 16 affected actual
source objects match. The initial build failed only the required module-comment
check; the corrected source passes. Reports are under
`build/runtime/eur_high_mix_pair_update/`: `evidence_mix83_v1.json` (failed),
`evidence_mix83_v2.json` and `isolated_v2.json` (passed). Producers under
`build/analysis/high_effort_50_to_55/` are `probe_mix_pair_update.py`,
`mix_pair_update_oracle.py` and `check_mix_pair_update_isolated.py`;
failed versions and logs are retained.


## MSL exception frame records

[Frame decoding](../../src/msl/unwind.c) reconstructs resident ARM9
`0x02046E5C..0x02046FC0`. [Handler lookup](../../src/msl/object_lifetime.c)
adds `0x020483F4..0x02048518` to the adjacent object-lifetime unit.
The shared [context layout](../../src/msl/unwind_internal.h) is 112 bytes,
with sixteen saved register words at +28, frame size/adjustment at +96/+100,
a halfword mask at +104 and flags at +106..+108. Register restoration copies
saved words into this context; it does not directly restore CPU registers.

Header bits 6/5/7 select a frame pointer, a sixteen-byte stack header and r7
versus r11 as the frame pointer. The second byte supplies saved-register bits
4..11, with bit 14 added explicitly. One unsigned encoded integer gives frame
size; bit 6 adds another for stack adjustment. Skipping a header consumes the
same encoding. Table binding always returns the thirty twelve-byte native
entries at `0x02048D90..0x02048EF8`, regardless of its address argument. This
table remains native data; its end coincides with the start of `.init`.

Handler lookup clears both output pointers, searches the exception table,
selects inline or referenced instructions and scans delta-coded intervals.
Entry ends are inclusive. At an address shared by adjacent entries, preserve
the binary search's selected entry; a first-match linear scan can disagree.
Handler interval starts and ends are also inclusive. An explicit interval-start
temporary preserves the native operand order while keeping the C readable.

Controlled live tests use the Save 55 checkpoint with SHA-1
`496c6a6836f08c15e95655bb5d78c4cde65dc688`. At the guarded battle common-frame
entry, 2,048 bytes of the live common workspace temporarily hold synthetic
contexts, records, encoded headers and saved words. Twenty-eight calls across
400 frames check 139 ordered writes, helper arguments, returns where meaningful,
the full 70,976-byte workspace, root, read-only exception data and ABI.
All original workspace bytes, a 256-byte stack window and registers are restored
before each original frame update; all 28 updates finish. The final Gritzy Caves
battle-menu capture was inspected. Both live runs pass; the second uses the
corrected boundary oracle and produces the same final capture.

The isolated replay uses copied live main RAM/DTCM, 128 KiB synthetic scratch,
ARM946 execution, the actual five compiled functions and real unsigned-decoder
and entry-search helpers, without stubs. Its 609 cases comprise 64 header skips,
320 header decodes, 60 register-mask cases, 161 lookups and four table queries.
Lookup cases include 150 native-table boundaries and eleven explicitly synthetic
table/handler-interval cases. The checks cover 4,115 ordered writes, full memory
outside the bounded CPU stack, return values, SP and r4-r11. Initial isolated
v1 failed on a shared inclusive entry endpoint; v2 corrects only the independent
search model and passes. The failed producer/model and log are preserved.

These are controlled routine checks, not evidence of a naturally raised game
exception, stack transfer, cleanup execution or destructor dispatch. All 104
original saves are unchanged. Full verification passes 107 tests, the golden
ROM and zero native differences; all eight functions in the affected source
objects match. The first build failed because this toolchain has no `stddef.h`;
the layout checks now use the repository's address-of-member convention.
Private reports are `build/runtime/eur_high_msl_unwind/live55_v1.json`,
`live55_v2.json` and `isolated_v2.json`. Producers under
`build/analysis/high_effort_50_to_55/` are `msl_unwind_oracle.py`,
`probe_msl_unwind.py` and `check_msl_unwind_isolated.py`.


## MSL RTTI destruction

[RTTI destructors](../../src/msl/type_info.c) cover the eight resident entry
points at `0x02048BE0..0x02048CE8`. Native RTTI names establish `type_info`,
`__class_type_info` and `__si_class_type_info`. Their vtables remain native data
at `0x02059ED4`, `0x02059EF4` and `0x02059EE4`. Base and complete destructors
remain separate ABI entries; deleting variants additionally call
`GameHeap_Delete`. Every entry returns the original pointer, including deleting
variants, which perform no object reads after release. The shared prefix is
eight bytes; single-inheritance descriptors add a four-byte base-type pointer.

Controlled DeSmuME calls use the Save 55 checkpoint with SHA-1
`496c6a6836f08c15e95655bb5d78c4cde65dc688`. A temporary, valid four-block heap
in the common workspace tests all combinations of free predecessor/successor
and two heap cursor positions. All eight entries and the real Delete/Free/Merge
helpers execute: 29 controlled calls, 168 ordered writes and 29 completed
original common updates across 400 frames. Checks include helper arguments,
returns, ABI, all 70,976 common bytes, the 512-byte heap table, root and RTTI
data. The original workspace, heap table, root, 256-byte stack window and
registers are restored before each original update. Released storage is checked
as allocator state, not as a live object. The final Gritzy Caves menu was viewed.

The isolated ARM946 replay uses copied live RAM/DTCM and 128 KiB scratch.
Its 384 cases combine eight entries, four free-neighbor patterns, four cursor
positions and heap IDs 0, 1 and 31. All compiled target bytes and real helpers
are guarded; no helpers are stubbed. The checks cover 1,332 ordered writes,
full memory outside the bounded CPU stack, returned pointers and callee-saved
registers. Heap 1's distinct cursor update is included. These are controlled
routine checks, not evidence of naturally allocated RTTI objects, exception
dispatch or asynchronous heap operations. All 104 original saves are unchanged.

Full verification passes 107 tests, the golden ROM and zero native differences;
all eight actual source functions match. Private reports are
`build/runtime/eur_high_msl_type_info/live55_v1.json` and `isolated_v1.json`;
their producers are `probe_msl_type_info.py`, `msl_type_info_oracle.py` and
`check_msl_type_info_isolated.py` under `build/analysis/high_effort_50_to_55/`.


## MSL catch records and exception specifications

[Catch setup](../../src/msl/catch_record.c) at `0x02047318..0x02047388`
writes a record into the saved frame: object, type name, destructor and adjusted
object pointer. For a pointer exception (`'*'`), the fourth word points to the
fifth word, which contains the adjusted pointer value. Other exceptions point
directly into the thrown object. The context view is only a 28-byte prefix;
the complete context includes saved registers. The native caller consumes no
return from this initializer; a `void` definition reproduces the complete
function, whereas returning the stored member adds loads.

[Specification matching](../../src/msl/exception_spec.c) at
`0x020475D4..0x02047660` scans a count of packed little-endian type pointers and
stops at the first match. The native type matcher remains unreconstructed.
Each local candidate pairs its type pointer and adjustment output, preserving
the native stack layout and store before the helper call. The adjacent
[handler-kind accessor](../../src/msl/object_lifetime.c) adds
`0x020483D8..0x020483F4`: null yields zero, otherwise the low five bits select
the action. These three entries add 280 linked C bytes.

Controlled DeSmuME checks use the Save 55 checkpoint with SHA-1
`496c6a6836f08c15e95655bb5d78c4cde65dc688`. Across 400 frames, four catch setups,
eight kind queries and thirteen specification queries produce 46 ordered stores
and fourteen real type-matcher calls. All 25 original common-frame updates
complete after restoration of the full 70,976-byte workspace, root, 256-byte
stack window and registers. Inputs occupy 2,048 temporary workspace bytes;
checks cover the full workspace, read-only RTTI data, arguments, meaningful
returns and ABI. The final Gritzy Caves menu was inspected. Initial live v1
failed because the probe attempted a zero-length emulator write for an empty
list; v2 skips that host write and passes. Game code did not change.

The isolated ARM946 replay passes 834 cases: 36 catch setups, 257 kind queries
and 541 specification queries. It checks 1,246 ordered stores and 542 real
matcher calls, all handler bytes/null, positive/negative frame adjustments,
packed-list alignments, builtin names, pointer/reference qualifiers, void-pointer
and catch-all entries. Full copied main RAM/DTCM and 128 KiB scratch outside the
bounded CPU stack are checked; no helpers are stubbed. This does not cover class
hierarchy encodings, a naturally thrown exception, destructor dispatch or CPU
unwind. All 104 saves remain unchanged.

All seven functions in the three affected compiled objects match. Full
verification passes 107 tests, the golden ROM and zero native differences.
Private reports are `build/runtime/eur_high_msl_catch/live55_v2.json` and
`isolated_v1.json`; producers under `build/analysis/high_effort_50_to_55/` are
`msl_catch_oracle.py`, `probe_msl_catch.py` and `check_msl_catch_isolated.py`.
The failed v1 probe/model and report are preserved separately.


## MSL active catch search

[Active catch search](../../src/msl/active_catch.cpp) reconstructs
`0x02047660..0x020477B4`. It copies a 24-byte record and 112-byte context to a
private cursor, advances through handler actions, and resolves action 13's
signed offset relative to the saved frame pointer. Only the original context's
type, object, destructor and active-catch fields are updated. The record embeds
the shared 20-byte lookup prefix; its sixth word remains opaque. Explicit scalar
copies followed by the 84-byte saved-state aggregate preserve native copying.
Actions 1, 14 and 19 terminate this search; jumps are consumed inside the
advance helper after a valid skippable action. The advance helper remains native.

Controlled DeSmuME checks on the Save 55 checkpoint
(`496c6a6836f08c15e95655bb5d78c4cde65dc688`) pass 18 calls and 781 ordered
stores across 400 frames. Checks include scalar and aggregate copies, actual
handler/decoder helpers, returned pointers, ABI, full 70,976-byte common workspace,
root and read-only RTTI data. The workspace, root, 512-byte CPU stack window and
registers are restored before all 18 original updates complete. The final Gritzy
Caves menu was viewed. Live v1 failed a probe-local variable shadowing error;
v2 exposed an invalid action-19 fixture. The preserved v3 model excludes it.

The isolated ARM946 replay passes 576 cases and 24,955 ordered stores, using
copied live RAM/DTCM, 128 KiB scratch, compiled target bytes and real helpers.
It covers immediate catches, skippable actions 2..12 and 15..18, multi-action
streams, encoded operand lengths, signed frame offsets and jumps after skips.
Full memory outside the bounded CPU stack, ordered calls/stores, return and ABI
are checked. An earlier fixture incorrectly started with a jump; the failed
version is preserved. Frame-boundary traversal, terminal errors, naturally
raised exceptions and CPU unwind are not covered. All 104 saves remain unchanged.

The actual compiled function matches all 340 bytes. Full verification passes
107 tests, the golden ROM and zero native differences. Reusing the shared
record prefix leaves the compared machine code and ROM unchanged; the runtime
reports therefore cover those same final bytes. Private reports are
`build/runtime/eur_high_msl_active_catch/live55_v3.json` and `isolated_v2.json`;
their producers under `build/analysis/high_effort_50_to_55/` are
`probe_msl_active_catch.py`, `msl_active_catch_oracle.py` and
`check_msl_active_catch_isolated.py`.


## Battle trail model copies

[Trail drawing](../../src/battle/battle_trail_draw.cpp) reconstructs overlay 2's
`0x020BD5B0..0x020BD740`. The two native callers at `0x020B885C` and
`0x020B9FA0` pass 12-byte point records. The renderer uses each point's first
three signed halfwords as relative translations. It saves the model's 64-byte
matrix and animation fields, optionally resets the first three matrix rows,
applies the initial translation and then accumulates one translation per copy.
The fourth row survives reset. Per-point depth is `-y / 16`, rounded toward
zero; an arithmetic shift would differ for small positive heights. The shared
model type supplies virtual drawing and matrix access. No new ASM is used.

Controlled DeSmuME calls use an initialized, allocated 440-byte party renderer
in the Save 55 checkpoint (`496c6a6836f08c15e95655bb5d78c4cde65dc688`). Five
cases cover disabled rendering, zero/negative point counts, and three points
with reset off/on. The points include heights +17 and -17. Checks independently
derive cumulative Q12 matrix results and verify all helper arguments, full model,
70,976-byte common workspace, roots, saved matrix and ABI. All real helpers run:
four matrix getters, eight directional copies, ten matrix translations and six
draw calls. Observed GPU submissions include 108 writes to VTX_16, 162 to
VTX_XY, and six begin/end pairs. These counts confirm submission, not an
independent pixel or renderer oracle.

Draw calls additionally check full main RAM and DTCM, excluding the bounded CPU
stack and asynchronous reverb capture buffer `0x0205B300..0x0205E300` owned by
the audio loader. The sort-key override at `0x0205A8AC` is modeled from the
draw helper's selector. Its update and audio DMA explained an initial full-RAM
assertion; the failed report is preserved. An earlier probe failed by calling a
nonexistent host memory method. Corrected v3 and v4 pass. Before each original
common update resumes, the probe restores the full workspace, model, roots,
saved matrix, sort-key cache and 1,040-byte stack/argument window. All five
original updates complete over 400 frames. The final Gritzy Caves menu was
viewed, and all 104 original saves remain unchanged. No natural trail activation,
isolated boundary sweep or independent visible-trail comparison is claimed.

The first valid C++ draft matches all 400 bytes; the actual integrated source
object also matches. Full verification passes 107 tests, the golden ROM and
zero native differences. Private evidence is
`build/runtime/eur_high_battle_trail_draw/live55_v4.json`, produced by
`build/analysis/high_effort_50_to_55/probe_battle_trail_draw.py`; v1/v2 failures
and the successful v3 source/report remain separate.


## Command wheel actor mapping

[Actor mapping](../../src/battle/battle_wheel_actor.c) reconstructs overlay 2's
`0x0209A580..0x0209A768`. It reads the selected entry's signed 15-bit icon ID,
ignoring the dismiss bit. Icons 13/14 through 19/20 select adults or babies
according to the two adult slots' formation indices; 21/22 select Mario.
Nonzero `enabled_only` excludes the even icons. Icon 11 returns zero, while
unsupported entries return -1. The two switches preserve native branch order.
Call sites are `0x02083A08`, `0x02083BF4`, `0x0209A408`, `0x0209A490`,
`0x0209A4FC` and `0x0209A558`.

The first draft differed in eight words: four conditionally selected return
pairs were emitted in the opposite order. Expressing those conditions in the
native order produces an exact 488-byte function without ASM. The integrated
source object also matches. The first full gate caught a missing opening module
comment; the corrected source passes all 107 tests, golden-ROM packaging and
zero-difference native relinking.

The Save 55 checkpoint (`496c6a6836f08c15e95655bb5d78c4cde65dc688`) supplies
live initialized party state. Across 500 frames, 216 controlled calls cover
18 icon values, both dismiss-bit values, three mode values and two formation
pairs, rotating through all five entry indices. Signed icon extremes, defaults
and all actor pairs are included. An independent arithmetic mapping checks
each result; all 432 real party lookups execute with checked arguments and
returns. The full context allocation, its root, the two embedded 148-byte party
slots and preserved registers remain unchanged by each call. Fixtures and the
256-byte CPU stack window are restored before all 216 original common updates.
The final Gritzy Caves command menu was viewed; all 104 original saves remain
unchanged. This is controlled live coverage, not natural target-menu navigation.

The first probe incorrectly expected separate party allocations and stopped
before injecting any call. The initializer and shared frame layout establish
embedded slots at context `+0x4DA8`, stride 148. The corrected v2 probe checks
those exact relationships and the containing allocation bounds. Private evidence
is `build/runtime/eur_high_battle_wheel_actor/live55_v2.json`, produced by
`build/analysis/high_effort_50_to_55/probe_battle_wheel_actor.py`; the failed v1
report and source are retained separately.


## Party rebound update and shadow

[Rebound updates](../../src/battle/battle_party_rebound.c) reconstruct overlay 2's
`0x02077C38..0x02077E78`: a 512-byte task callback and its 64-byte partner-shadow
helper. The still-native creator at `0x02077E78` copies a 16-byte formation preset
into a 28-byte slot in the context's `+0x8B44` pool. Packed IDs occupy two seven-bit
fields. The update follows the actor while channel 3 runs, decrements a signed
16-bit delay, and either ends the reaction or reduces the next arc by its decay
percentage. The shadow copies x/y and uses the signed projected y position to
choose z=-192 or the native 282-unit offset. Source order preserves aliasing
when both object pointers are equal. These constants come from the instructions.

The callback matches on its first valid C draft. The shadow's earlier private
draft emitted 68 bytes instead of 64; expressing the nonnegative branch first,
as in native code, removes the extra branch. Both integrated source functions
match completely without ASM. Full verification passes 107 tests, golden-ROM
packaging and zero native differences.

Forty-eight controlled live calls use the Save 55 checkpoint
(`496c6a6836f08c15e95655bb5d78c4cde65dc688`). They cover active/inactive channel 3,
three delay values including signed-halfword wrap, four coordinate pairs including
both projection signs and signed extremes, and separate/aliased shadow objects.
All real party lookup, object lookup, shadow and channel-query helpers run.
An independent memory model checks their arguments, results and expected stores,
the complete context allocation, its root and preserved registers. The fixture
uses a verified free pool slot temporarily; the slot, full context and 256-byte
CPU stack window are restored before every original common update. All 48 original
updates finish over 400 frames. The final Gritzy Caves menu was viewed, and all
104 saves remain unchanged.

This replay deliberately keeps the delay nonnegative after decrement. Effect
creation, sounds, amplitude decay/square root/new-arc setup, animation cleanup,
knockout and natural recoil activation remain untested by this replay. Exact
matching covers the whole function; the live coverage is limited to the paths
listed above. Private evidence is
`build/runtime/eur_high_battle_party_rebound/live55_v1.json`, produced by
`build/analysis/high_effort_50_to_55/probe_battle_party_rebound.py`.


## Party target menu construction

[Target-menu construction](../../src/battle/battle_wheel_targets.cpp) reconstructs
overlay 2's `0x0209A0C8..0x0209A580`. Mode 1 and modes >=4 offer the group target
when any party member is eligible. Other modes build individual actor icons from
the adult slots' formations and all four actors' exclusion flags. Usable-item
effect kind 2 adds the appropriate separated adult entries. Every list ends with
icon 11. The selection searches for the requested actor, the current actor,
its partner, then the first entry. Each icon write preserves bit 15; only native
angle writes reset angles, leaving other entry animation fields intact.

The first C++ draft matches all 1,208 bytes. Replacing its temporary raw item
view with the shared `UsableItemRecord.effect_kind` preserves the match, as does
the final named-offset source. The actual build object matches completely,
without ASM. Full verification passes 107 tests, golden-ROM packaging and
zero-difference native relinking.

The Save 55 checkpoint (`496c6a6836f08c15e95655bb5d78c4cde65dc688`) supplies
initialized party slots. A 360-case live matrix combines six formation pairs,
five modes, six exclusion masks and two item kinds, rotating requested/current
actors. A temporary item descriptor occupies a verified free pool slot. Expected
list contents and selection are derived independently, and checked against the
full context allocation, root and preserved registers at helper boundaries and
return. All 1,104 direct party lookups and 1,427 actor-mapping calls execute;
the mapper performs 2,854 additional real party lookups with checked arguments.
Selection outcomes cover requested actor (125), current actor (47), partner (6)
and first entry (182). Fixtures preserve unrelated flag/animation bits so the
comparison also detects unintended resets.

The probe restores the full context, root and 256-byte stack window before
each original common update; all 360 updates complete over 600 frames. The final
Gritzy Caves menu was viewed and all 104 saves remain unchanged. These are
controlled live calls, not natural item-menu navigation or malformed-pointer
coverage. Private evidence is
`build/runtime/eur_high_battle_wheel_targets/live55_v1.json`, produced by
`build/analysis/high_effort_50_to_55/probe_battle_wheel_targets.py`.


## Scene object captions

[Scene text](../../src/scene_menu_ov007/scene_text.cpp) reconstructs overlay 7's
`0x02083C20..0x02083E6C`: two-line rendering and upload-task creation. The Scene VM
keeps its five-argument interface; only mode, table and entry are consumed. Mode
zero truncates both lookup indices to unsigned halfwords, clears 4,096 bytes of
the 12,288-byte scratch allocation, and handles `FF 00` as a line break. Each
line queues eight strips. The first row uses width 256; the second uses the text
cursor's width. The ready halfword is at `read32(0x020A6B90) + 0xA7EA`.

The 72-byte factory matches on its first draft. The renderer matches all 516
bytes after forwarding the current row, rather than a literal zero, to the
first sprite-creation call. The final source objects match, with no ASM. The
initial full build caught reversed function-section order; reversing definitions
as required by MWCC fixed it. The complete verification then passed 107 tests,
golden-ROM packaging and a native relink with zero differences. The adjacent
140-byte upload callback remains unlinked; its private candidate differs.

The original `MenuAI/BAI_iwasaki.dat` entries identify this as Bros. demonstration
text. Normal Save 65 menu navigation visits Green Shell and Bro Flower demos,
then returns to the visible field. Over 2,480 frames the probe checks six caption
calls, 96 upload-task creations, 414 real glyph steps, 12 sprite-creation calls,
six newline and six terminator paths. Table 3 entries 1, 2, 6 and 7 are reached.
It independently checks initialization, lookup results, clearing, cursor edits,
ordered helper arguments, task pool/list insertion and the final ready flag.
Checked caller memory comprises the complete 90,600-byte workspace, 43,056-byte
scene allocation, 12,288-byte scratch and 48-byte local text record; task checks
include all 72 bytes and affected pool/list neighbors. Group cleanup marks 80
existing tasks across the six calls. All 104 original saves remain unchanged.

Glyph rendering is observed within the text record and 4,096-byte drawing area;
sprite helper internals, asynchronous tile transfers, independent pixel output
and nonzero-mode early returns are outside this replay's coverage. No RAM edits
or substituted helpers are used. The demonstrations and final field capture
were visually checked. Private evidence:
`build/runtime/eur_high_scene_text/bros65_v1.json`, produced by
`build/analysis/high_effort_50_to_55/probe_scene_text.py`.


## Cannonballers landing bounces

[Landing bounces](../../src/attack_cannonball_ov012/landing_bounce.cpp) reconstructs
overlay 12's `0x020C34E0..0x020C3888` as 936 bytes of matching C++, without ASM.
The caller enters this path when the target enemy's two-bit trait field equals
one; ordinary successful hits take another route. The first call times the
return home by summing three arcs. Subsequent calls start the individual bounces
at heights derived from 200, repeatedly multiplied by 153.6 and divided by 256.0.
Preserve the double operations and integer truncation at every step. Velocity
comes from `FX_Sqrt((int)(4096.0 * height)) / 16`. Resource mode and formation
select the two models' resources and animations independently.

The home-position helper reads only the shared formation byte at controller
offset 24. The first call resets the halfword at offset 22 and creates model
effect 27 and sprite effect 12 after projection. Preserve both signed-halfword
truncations around `12 - effect_anchor_z`; replacing the expression with literal
12 loses native operations. The sound choice is narrowed to `u16` locally; this
does not change the sound helper's full-width interface. Every call clears the
vertical channel's callback/deferred flag, starts the arc and sets phase 12.

The first private draft was 928/936 bytes with 30 differing words. Reading the
native double literals, matching conditional assignment order and restoring the
local sound-ID conversion yielded a complete match. The final compiled source
object, full ROM checks, 107 tests and zero-difference native relink all pass.

The ordinary checkpoint-83 Cannonballers replay reached no calls to this path.
The focused replay therefore uses four explicit trait fixtures. At the real
`BattleActor_HasTrait1` entry from the guarded overlay-12 controller, it sets bits
6-7 of the live enemy word at `actor + 0x29C` to one. It executes the native
query, verifies its result, and restores the entire original word before the
caller resumes. The induced bounce route then runs to ordinary attack cleanup;
it is not claimed to reproduce the original attack outcome.

Over 2,210 frames, the replay checks 12 calls: all four formation variants and
bounce indices 0, 1 and 2. It independently validates ordered helper arguments,
double conversion/multiply/divide results, square roots, 24 arc durations,
home/projection results, own model flags, channel resets, counter/state stores
and preserved registers/stack. Checks cover the complete 452-byte attack record,
both 260-byte scene objects and their 440/304-byte models at helper boundaries.
Animation/resource writes are bounded observations in the receiving objects and
models; movement internals are observed in the receiving object. External lists,
effect/sound internals, independent pixels/audio and out-of-range bounce inputs
are outside this replay's checks. The attack root is cleared at completion;
the return to the command menu was visually checked. All 104 saves are unchanged.
Private reports: `build/runtime/eur_high_cannonball_landing/cannon83_v1.json`
(no target coverage) and `cannon83_trait_v2.json` (passed), each with its preserved
probe version under `build/analysis/high_effort_50_to_55/`.

## Battle command availability

[Command availability](../../src/battle/battle_command_availability.c) reconstructs
all 1,028 bytes at overlay 2 `0x02099A74..0x02099E78` in C. Basic commands pass;
Bros Items require a current descriptor and the selected inventory-mask bit.
Item checks also rewrite flag bit 13 in each party actor. HP, full-health status,
the seven-bit item mode, the saved party form and adult/baby relationships decide
which targets remain eligible. Revive checks can exclude a baby because its adult
is down or has a positive transition state. Status items inspect five signed
channels. Retreat checks the context's bit 10 and both adult transition states.

The previous public draft needed native branch order, four explicit HP checks
and the existing actor/runtime bitfield views to match. The vertical-menu and
wheel-selection neighbors remain separate, unlinked drafts. The selected-item
bit expression uses an unsigned one; valid inventory indices are 0..31.

Live input replays from Save 55 and Save 83 battle checkpoints verified one basic
command and one full-health healing-item rejection, including all four exclusion
flags. These checkpoints descend from the previously restored room-306 encounter
fixture; this is normal menu input within a controlled encounter, not a natural
story encounter. At each return, the checks cover the full 401,416-byte context,
a 1,380-byte save prefix, roots, item-descriptor prefix, real party-getter calls
and preserved registers. Both final screens were inspected; all 104 saves remain
unchanged. A failed reader API call was fixed before the successful replays.

A separate 1,314-case ARM946 run on copied RAM executes the compiled routine and
real party getter without stubs. It covers all 32 inventory bits, null states,
HP and signed-status boundaries, revive relationships, item modes 0..4/63/64/127,
retreat guards and invalid commands. Full mapped memory outside the 32-byte stack
frame is checked; every other store must target a party exclusion halfword.
These cases supplement live coverage and do not establish gameplay reachability
of every synthetic combination. Private reports are
`build/runtime/eur_xhigh_command_available/{natural55_v2,menu83_v1,isolated_v1}.json`;
each records its producer hashes. The full build remains byte-identical.

## Cartridge backup specification

[Backup geometry](../../src/nitro/card/card_backup_spec.c) reconstructs
`CARDi_SetBackupSpec`, resident `0x02042A44..0x02042C1C` (472 bytes). The low
byte of the type selects a device family; the next byte is the capacity exponent.
A native table selects page/sector sizes, address width and operation delays.
It clears the 40-byte geometry suffix of the shared 64-byte command while keeping
unrelated request fields. Type zero leaves the previous result untouched;
unsupported combinations clear the type/capacity and set result 3.

The inherited C draft differed only in the final failure-path scheduling.
A seven-instruction inline-assembly fragment keeps its two clears, shared-command
reload and error store in native order. The remaining table logic is C; the
whole actual source object matches. Supported type constants have exponents
below 32, which is the defined C shift domain used by the tests.

Ordinary Save 55 cold boot called this routine once at frame 5 with type `0x0D01`,
selecting an 8 KiB device, 32-byte pages, two-byte addressing and delay 79.
The replay checks the complete 480-byte work record and 64-byte command, real
40-byte fill output and helper/return ABI. The title screen was inspected and
all 104 original saves are unchanged. Separately, 576 copied-RAM ARM946 cases
cover every table entry, zero/unsupported types, all 32 defined exponents and
three high-word patterns. They execute the real byte-fill helper, check every
mapped byte outside the 16-byte stack frame and verify ordered clear/type/error
stores. This establishes configuration behavior, not backup hardware timing.
Private reports: `build/runtime/eur_xhigh_backup_spec/{boot55_v1,isolated_v1}.json`.

## Copy Flower hit resolution

[Hit resolution](../../src/attack_copy_flower_ov017/hit_resolution.cpp) reconstructs
all 704 bytes at overlay 17 `0x020C4994..0x020C4C54` in C++. Input code 1 rolls
the hit bonus and requests rumble; the other codes select the failed-input row.
Three 16-byte effect rows choose sprite/model effects, reaction and sound. The
routine calculates damage, updates the enemy snapshot and rating, replaces an
unusable target and stops the other participants if none remains. The successful
hit count increases after the rating call, so that call sees the previous score.

The shared work prefix now names score `+0xCC`, target `+0x102`, power parameters
`+0x104/+0x108`, selection-mode bit 0 at `+0xA84` and four party pointers at
`+0xA98`. Its recovered size is `0xAA8`; the full allocation remains 16,356 bytes.
A cached `u16` target preserves the caller's native zero extension. Once instruction
flow matched, the remaining register/stack differences were resolved by one
explicit declaration-order change derived from the native assignments. In this
function, the competing long-lived locals followed reverse declaration order;
this is an observed MWCC behavior, not a general register-allocation guarantee.

A Save 83 checkpoint replay uses ten timed button inputs and then lets input fail.
It checks 11 calls (ten ordinary hits, one failed input), 162 direct helper calls,
full attack/party/enemy/object records, own stores, snapshot/rating/variant updates
and register/stack restoration. Damage/reaction writes are observed within the
target enemy/object records; the feedback factory's handle choice is observed in
the work area's four-byte owner slot. Bonus/damage/selection return values and
external effect/graphics allocation remain observational. The final battle menu
was inspected, the attack work pointer was cleared and all 104 saves remain unchanged.
This established checkpoint descends from a controlled encounter.

Another 264 ARM946 cases execute the compiled caller with explicitly modeled
children: 32 ordinary, 40 bonus and 192 failed hits, including 136 target
replacements and 64 stop requests. They cover all input codes/formations, both
selectors, zero/65/66 choices and signed coordinate/damage boundaries. Full mapped
memory outside the 64-byte stack is checked. These cases verify caller behavior,
not the real children or natural reachability. The initial live probe missed the
feedback owner write; the initial isolated hook treated unexecuted conditional
instructions as executed. Both failures and corrected passing producers are kept.
Private reports: `build/runtime/eur_xhigh_copy_hit/{copy83_v2,isolated_v2}.json`.


## Battle scheduler frame driver

`BattleScheduler_Update`, overlay 2 `0x020729D4..0x02072D90`, is 956 bytes of
matching C++ in [battle_scheduler_frame.cpp](../../src/battle/battle_scheduler_frame.cpp).
It requeues deferred transfers before updating the node list, then services the
promoted task while at least 15 visible scanlines remain. It submits the buffer
swap and enables VBlank processing before using spare time for archive opens,
compressed reads, overlays, audio loading and finally queued battle tasks, in
that order. A promoted queued task forces a VBlank wait. A cleared callback
advances the ring head, including wrap from 31 to zero.

The node successor is read after its update callback. An empty list invokes the
scheduler's deleting virtual method when the archive argument is non-null.
The driver can observe VBlank between work items; its count is cleared at the
native boundaries. Expressing the work loop with an explicit early return
recovered the native control flow: the first draft's condition at both loop ends
added 16 bytes. No assembly was needed. The actual source object matches the
complete range, including its literal pool; the full gate passes 107 tests,
the original ROM hash and a native relink with zero differing bytes.

The private live producer is `build/analysis/xhigh_from_55/probe_battle_frame.py`;
its successful version and oracle are also preserved with `_live_v2` suffixes.
`build/runtime/eur_xhigh_battle_frame/evidence_flower83_v2.json` replays the
established Save 83 Bro Flower setup checkpoint for 381 frames, including one
frame to finish pending calls. Checkpoint provenance remains the earlier
controlled encounter; this replay adds only keypad input, without RAM edits.
It checks 380 complete driver calls, 3,170 direct helper calls and 3,080 caller
stores. Branch coverage includes 380 deferred before-mapping transfers,
278 queued-task callbacks, 301 compressed-read steps, 42 overlay steps, eight
promoted-task callbacks, seven promotion waits and one ring wrap.

Driver checks cover call order and arguments, branch decisions from guarded
live reads, ordered own RAM/GPU stores and the register/stack ABI. Arbitrary
callback/archive/audio effects and interrupt arrival remain observations.
Alongside these checks, the existing independent queue model verifies 1,016
leaf calls and the VBlank model verifies 380 consumers, with their full-record
checks and bounded callback observations. These are nested measurements, not
additional frame-driver calls. Eight screenshots and four graphics dumps were
validated; the final attack-setup screen was visually inspected. Rendering is
observed rather than independently rasterized.

`isolated_v1.json` adds 146 ARM946 cases with an independent high-level driver
model. Every called helper is an explicit stub. Cases include zero/one/32
deferred records, node-link mutation, null callbacks, an empty list with either
argument, callback retention, task promotion, queue wrap and all combinations
of pending archive/audio work. The clock fixtures distinguish elapsed scanline
177 (accepted) from 178 (deferred), plus negative and later values. Full copied
4 MiB RAM, 16 KiB DTCM, 64 KiB scratch and the modeled I/O page are checked,
except the 40-byte CPU stack; ordered calls/stores and ABI are checked too.
The deleting-method stub clears the root and does not establish real heap cleanup.
These fixtures do not simulate asynchronous IRQ timing or graphics hardware.

The first live probe stopped at frame 11 because it expected the instruction
immediately after a conditional wrap store; DeSmuME skipped two instructions
whose condition was false. `flower83_v1` and its producer/oracle are retained.
The correction checks the actual next executed instruction; matching game code
was unchanged. Both successful producers exited zero, no calls remained pending,
and all 104 original saves are unchanged. Artifact hashes and coverage totals
are checked in `build/analysis/xhigh_from_55/battle_frame_validation.json`.


## Save-menu panel column fades

`SaveMenuPanel_UpdateFade`, overlay 8 `0x0207297C..0x02072A50`, reconstructs
212 bytes in [save_panel_fade.c](../../src/save_menu_ov008/save_panel_fade.c).
Each 72-byte task has a delay, panel and column indices, and Q12 opacity and
velocity at offsets 36 through 52. After the delay, it advances opacity, clamps
at 31 for a positive velocity or zero for a nonpositive velocity, and marks the
task for removal. Otherwise it divides toward zero and updates one byte of the
two-by-fourteen opacity table at `0x0207AA72`. Marking and later pool return are
separate operations. Row addressing before the column offset recovers the native
dataflow; the compiler's reverse local allocation order resolves the remaining
register differences. The callback is pure C, with no inline assembly.

The actual linked object is exact. The full gate passes all 107 tests, the
original EUR ROM hash and a native relink with zero differing bytes. The private
producer is `build/analysis/xhigh_from_55/probe_save_panel_fade_live_v2.py`.
Reports live under `build/runtime/eur_xhigh_save_panel_fade/`.

The ordinary checkpoint-55 save route (`save55_v1.json`) completed 708 frames
but did not run this callback: both location labels had ID 13, so the creator
used its immediate path. Its target-coverage assertion therefore failed.
`different55_v2.json` repeats the same buttons with one bounded RAM fixture.
At the fully guarded creator `0x020727D8`, indices at `0x0207AA64` are checked
as 0 and 1. The probe saves the halfword at `0x0207AA6C` and temporarily copies
the existing third name ID from `0x0207AA70` there, making the comparison differ.
It restores and verifies the original two bytes at the creator's SP-matched
return, in the same frame, before the new callbacks run. This tests a controlled
label-change path; it does not establish a natural change of location.

The live check covers 588 updates: 224 delay steps, 168 rising and 168 falling
steps, and fourteen clamps at each endpoint. All fourteen columns of both panels
run. The probe checks complete task records, the full 10,248-byte workspace,
pool metadata, mark-helper arguments and effects, and stack/callee-saved
register restoration. It observes and validates 28 pool-slot creations, then
independently verifies all 28 later unlinks and returns through the real pool
cleanup, including list neighbors, pool links and the live element count.
The creator's other internals and graphics rendering remain observational.

Nine screenshots and 54 VRAM/palette/OAM dumps are hash/extent checked. The five
capture points before the frame-209 fixture exactly match the ordinary run.
The save animation at frame 217 and visible field return at frame 708 were
inspected. All 104 supplied saves remain unchanged; saving uses the emulator's
battery copy.

`isolated_v2.json` adds 238 ARM946 cases, executing the compiled callback and
the actual mark helper without stubs. Cases cover every panel/column, delayed
updates, exact and crossed clamp thresholds, zero velocity, and signed Q12
rounding. The independent model checks ordered writes, helper arguments, full
4 MiB RAM and 16 KiB DTCM outside the eight-byte stack frame, a 4 KiB scratch
page, and the return ABI. These copied-memory cases do not add live lifetimes,
IRQ or raster coverage. The first isolated harness set SP before switching ARM
mode and failed on an unmapped write; setting the mode before the banked SP
fixed the harness. Failed producers and logs are preserved. Artifact hashes
and totals are recorded in the private `save_panel_fade_validation.json`.


## Field corner-overlap correction

`FieldNavigation_ResolveCornerOverlap` (overlay 0, `0x020AA30C..0x020AA5B0`)
reconstructs 676 bytes as [readable C](../../src/field/field_corner_correction.c).
The native query at `0x020AA5B0` combines obstructed corners with corners outside
all scanned surfaces. A partial mask selects an XY step of one world unit
(4096 in Q12). The caller shifts the four corner coordinates and entity position,
then refreshes the navigation cursor and swept bounds before querying again.
Masks 0 and 15 stop immediately; opposite-corner masks 5 and 10 have no step.
Positive iteration limits bound the corrections; zero and negative limits do not.
The two native callers use 16 and 8. Their return register is not consumed.

The shared query declaration takes `FieldRuntimeEntity *`; the party caller now
passes its embedded entity explicitly. All eight functions in that existing
unit, as well as the new actual build object, remain exact. Initialization and
switch-block order account for the draft's differences; no assembly is needed.
The full gate passes 107 tests, the original EUR ROM hash and native relinking
with zero differing bytes.

Private reports are under `build/runtime/eur_xhigh_corner_correction/`:

- `walk43_v1.json`: 465 frames from the initialized checkpoint-43 castle state,
  using movement and a jump. One ordinary call returns the native query's zero
  mask without correction. The complete 1440-byte member, 11216-byte area,
  corner arrays, navigation records and SP/r4-r11 are checked. Ownership follows
  the live area list and allocation header; the query's geometry is observed.
- `fixture43_v3.json`: the same route, with one guarded correction at frame 429.
  At the complete native entry, the iteration argument is changed from 8 to 1.
  At its SP-matched query return, the ordinary zero mask is checked and replaced
  with 1. The independent model verifies all sixteen stores, including the XY
  step, corner arrays, cursor and swept bounds. At the outer return, the original
  member and arrays are restored and read back before the caller resumes. The
  query's observed scratch registers and the flags of the ordinary `cmp r0,#0`
  return are restored too. This is a controlled mask fixture, not a naturally
  encountered obstruction. All eight screenshot hashes and six final graphics
  dumps equal the ordinary replay; its final visible castle image was inspected.
- `isolated_v1.json`: 349 ARM946 cases run the compiled caller on copied RAM and
  DTCM, with an explicit stub for the mask query and synthetic entity/corner/
  surface inputs. They cover every direction, immediate stops, repeated masks,
  mixed sequences, positive/zero/negative iteration limits, both orderings of
  current/previous coordinates, null surfaces, scan equality, skipped surfaces
  and the end sentinel. All 693 query calls and 6712 ordered stores agree with
  the independent model. Full 4 MiB RAM, 16 KiB DTCM and 4 KiB scratch memory
  match outside the 32-byte callee stack, with SP/LR and r4-r11 preserved. These
  checks do not validate the stubbed geometry, live lifetimes, IRQs or rendering.

The first controlled producer (`fixture43_v2`) failed because its register reader
did not translate Capstone's `sb` alias to `r9`. Its report/source/log are retained;
only the probe was corrected. Successful producer versions are
`build/analysis/xhigh_from_55/probe_corner_correction_live_v1.py` and `_live_v3.py`;
the isolated producer is `check_corner_correction_isolated_v1.py`. Artifact hashes,
extents and counts are checked in `corner_correction_validation.json` there.
All 104 original save files remain unchanged.


## Battle auxiliary-object pause control

`BattleScene_SetAuxiliaryObjectsPaused` (overlay 2, `0x02076C38..0x02076C74`)
reconstructs 60 bytes in [battle_auxiliary_pause.c](../../src/battle/battle_auxiliary_pause.c).
It normalizes any nonzero input to 1, then sets property 46 on object IDs 40..55.
The existing property setter writes each scene object's `time_paused` halfword
at `+0x1A` and bit 9 in each non-null renderer's flags at `+0x7C`. Other bits
survive. These objects occupy embedded 260-byte slots in the 401416-byte battle
allocation; the field-object pointer table starts at context `+0x4CC8`.

Battle opcode `0x08C` calls this helper. The private decoded real scripts contain
eight uses in `BAI/BAI_mon_0_hn.dat`, entries 0, 1 and 9. That establishes static
call sites; the new live check does not claim to execute those enemy scripts.
The actual helper and updated 19168-byte dispatcher objects are exact. The full
gate passes 107 tests, the original ROM hash and zero-difference native relinking.

Private `build/runtime/eur_xhigh_auxiliary_pause/controlled83_v1.json` runs 180
neutral frames from `eur_attack_helpers/ov17_bros_menu83.dst` (SHA-1
`21d2e64a24b389689627292539103880c6761b47`). This checkpoint derives from save 83
through the previously documented controlled encounter; it is not a natural
story encounter entry. At four fully guarded battle-VM entries (`0x02079950`),
the probe redirects to the helper with 1, 0, -5 and 65536. DeSmuME executes the
already-decoded VM push before redirection; its 28 bytes and SP are restored at
the target entry. All 64 native property calls are checked against independent
object/renderer expectations. Each call checks the complete battle allocation
and eight live 304-byte alternate renderers, their ownership and allocation
bounds, the normalized arguments and callee-saved registers.

At each target return, all tracked original memory is restored and read back;
the original VM registers are restored on re-entry, and each original command
is observed returning. The unmodified baseline has no target calls. All seven
screenshots and six final graphics dumps match that baseline. The final visible
Bros menu was inspected. This verifies controlled pause/resume effects, not
sustained animation timing or naturally reached `0x08C` instructions.

`isolated_v1.json` adds 48 ARM946 cases using the actual compiled helper, native
property setter and native object lookup, with no stubs. Synthetic inputs cover
zero and seven distinct nonzero words, null/primary/alternate/both/shared renderer
pointers and the copied live layout. All 768 property calls, 768 lookups and
1600 ordered stores match the independent model. Full 4 MiB RAM, 16 KiB DTCM
and 64 KiB scratch memory are checked outside the 40-byte call stack; SP/LR and
r4-r11 are preserved. These cases do not prove renderer construction or IRQ
behavior. All 104 original saves are unchanged. Versioned producers and artifact
validation are in `build/analysis/xhigh_from_55/`, named
`probe_auxiliary_pause_live_v1.py`, `check_auxiliary_pause_isolated_v1.py` and
`auxiliary_pause_validation.json`.


## Field area and entity script startup

`FieldArea_StartScripts` (`0x0207EB90..0x0207EBC4`) initializes the embedded
manager at area `+0x2570`, using the signed startup-script halfword at `+0x28`.
A value of -1 selects the room header's default entry. The native area state
machine resets this field to -1 and copies a queued transition's script into it
when applicable. It then calls `FieldScriptManager_StartEntityScripts`
(`0x02089188..0x02089280`). These overlay-0 functions add 300 bytes of matching
C/C++ without inline assembly.

The area entry points are in [field_area_scripts.c](../../src/field/field_area_scripts.c).
The now-contiguous startup and auxiliary functions share
[field_script_manager.cpp](../../src/field/field_script_manager.cpp).

The entity loop caches the count at area `+0x2B32`, but reads each pointer from
the 32-slot table at `+0x29D8` as it proceeds. The following words hold linked-list
heads and other area state; the older 86-pointer script-context view was too
broad. An entity's signed halfword at `+0x0C` selects its startup script; -1
skips it. The unsigned halfword at `+0x08` identifies a suppression variable:
0xFFFF bypasses the lookup, otherwise a nonzero value suppresses startup. Bit 6
of `+0x0A` selects one of the two script banks. The loop begins the embedded
204-byte state at `+0x20` with context type 3, then immediately runs it.

Private `build/runtime/eur_xhigh_field_script_start/cold65_v1.json` records an
ordinary 2233-frame cold boot and load of save 65, with no RAM edits. Two area
starts examined 31 entities: 14 scripts started, 13 entities had no script and
four were suppressed. Full native guards, manager initialization, global
registration, helper arguments, script-state outputs, current entity/area
preservation and ABI checks passed. Entity table ownership and heap bounds were
checked. The final Thwomp Volcano screen was inspected; all 104 original saves
are unchanged. Execution of the started scripts' wider game effects remains
observational.

`isolated_v1.json` adds 52 ARM946 cases using the actual compiled wrapper and
loop, native initializer, script-begin helper, variable reader and memory clears.
Only script execution is stubbed. Synthetic cases cover empty and 32-entity
tables, both banks, signed script indices, disabled/active states, suppression
outcomes and mutations to the count and subsequent entities. All 1014 ordered
helper calls and full mapped memory outside a 128-byte stack allowance match
the independent model. These cases do not establish live lifetimes, rendering
or IRQ behavior. The private `field_script_start_validation.json` records the
versioned producers, artifacts, actual source-object comparisons and full build
gate; 107 tests, the original ROM hash and zero-difference native relinking pass.


## NitroSDK cartridge signature classification

`OSi_ReadCartridgeSignatureType` (`0x02039A60..0x02039B34`) adds 212 bytes of
pure C to [os_console_type.c](../../src/nitro/os/os_console_type.c). It allocates
a lock ID, masks ARM9 IRQs and checks the cartridge lock at `0x027FFFE8`. If
bit 6 of the owner is set, or its own lock attempt succeeds, it reads the GBA
bus at `0x08000000` and conditionally `0x08000004`, using 32-bit accesses. The
little-endian words spelling "NINTENDO" select `0x01000000`; any mismatch
selects `0x02000000`. It restores IRQ state after each iteration. Reading under
an existing ARM9 owner does not finish the loop: it exits only after acquiring
and releasing the lock itself. The allocated ID is retained.

This is a concrete PiT difference from the cross-game manual's fixed
`OS_GetConsoleType` result. PiT's already reconstructed caller caches a result
assembled from this signature class and other hardware flags. The cartridge
and rumble interfaces now use the shared lock declarations, including the
native halfword return of `OS_ReadOwnerOfLockWord`. All 26 functions in the
three affected source objects remain exact. The full gate passes 107 tests,
the original ROM hash and zero-difference native relinking. The first gate
exposed old void lock declarations in the rumble driver; removing those
duplicates resolved it.

Private `build/runtime/eur_xhigh_os_cartridge_type/boot65_v1.json` checks one
ordinary boot call at frame 5 during a 1200-frame cold boot. It acquires lock ID
66, reads `0xFFFFFFFF` from the accessible GBA bus, returns `0x02000000`, and
releases the lock. Bitmap, lock-word and bus-control effects, helper arguments,
results, IRQ/FIQ control bits and ABI preservation match independent expectations.
The title screen was inspected and all 104 original saves remain unchanged.
The bus value is checked at the actual load, after access is acquired; an earlier
snapshot taken while ARM7 owns the bus can read a different value.

`isolated_v1.json` adds 216 ARM946 cases with the actual compiled caller and all
native helpers, without function stubs. They cover both signature words,
lock-ID bitmap selection, four IRQ/FIQ mask combinations, an existing ARM9 owner
and failed lock attempts followed by explicit owner transitions. All 2880 ordered
helper calls, 540 word reads and 1692 stores match the model. Full mapped RAM,
DTCM, I/O, GBA and return pages are checked outside a 128-byte stack allowance;
the shared-memory page aliases the last main-RAM page. These deterministic
fixtures do not prove physical cartridge behavior, asynchronous ARM7 races or
IRQ scheduling. Versioned probes, immutable actual-object copies, failed and
successful build logs and artifact hashes are recorded in private
`build/analysis/xhigh_from_55/os_cartridge_type_validation.json`.


## Smash Eggs reward message

[Reward-message construction](../../src/attack_smash_egg_ov015/reward_message.cpp)
recovers overlay 15 `0x020C2BB8..0x020C2E78`: 704 bytes of C++ without assembly.
The item's high nibble selects localized text table 2, 6, 9 or 11. A 15-byte
control prefix reserves the icon gap, followed by a 32-byte slice of the name.
The measured width is saved at attack-context `+454`; the 128-byte message
starts at `+456`. The window X coordinate is `114 - round_up(width, 8) / 2`,
with the native nine-bit truncation. The battle opener subsequently rounds
main-screen coordinates down to whole tiles.

Tracing that opener into resident `0x0201D320` showed that the shared
[window properties](../../include/game/window.h) had reversed pointer names:
fonts are at `+16`, text at `+20`. Extent 1023 selects a direct string; other
values index a text archive. The declarations and scene/reset callers now use
the correct names and offset checks. All 19 functions in the three affected
source objects remain exact (6304 bytes). Both full gates pass 107 tests, the
original ROM hash and zero-difference native relinking.

The engine's backward copy at `0x02015EF4` is not a standard `memmove`. Its
initial destination-end load constructs a carry from two adjacent words; it
writes one to four carry bytes beyond the requested span, depending on source
alignment. The reward buffer has room for them. Preserve that native helper
instead of replacing it with a library copy. All 16 source/destination alignment
combinations were checked in DeSmuME against a model derived from those loads;
each call restores the full attack context, stack scratch, general registers,
SP/LR and CPSR before the ordinary forward-copy call resumes.

Private `build/runtime/eur_xhigh_egg_reward_message/reward83_v4.json` records
a 2110-frame restored-encounter replay. Two declared fixtures advance the
initialized launch counter and select one reward roll. The constructor executes
once at frame 572: text table 2, name 18, width 79 and handle 0. The frame-596
capture shows the Mix Flower message. Full context/header, item prefix, stack
records, helper arguments, localized lookup, clears, copy, text initialization
and ABI checks pass. Text measurement's eight-byte output is observed; the
caller independently uses it to construct the request. Window allocation and
rendering internals remain observational. Common captures and graphics dumps
match the earlier replay without the copy-boundary calls. Reloading the initial
checkpoint restores all main RAM and DTCM; all 104 original saves are unchanged.

`isolated_v3.json` adds 174 ARM946 cases: four categories, six languages, tile
rounding boundaries through width 65535, overlap in both directions and varied
window results. It executes the compiled caller and native lookup, clears, copy,
text initialization and reset. Measurement and window opening are explicit
stubs. All 1392 helper calls and complete main RAM/DTCM outside 256 stack bytes
match the model. These cases do not establish live allocation, measurement,
rendering or IRQ behavior. Private `egg_reward_message_validation.json` records
immutable objects, versioned probes and artifacts. The first isolated attempt
exposed the copy-oracle assumption; the second missed Unicorn's terminal-return
notification. Their failed logs are retained separately from the passing run.


## Battle window close

[BattleWindow_Close](../../src/battle/battle_window_close.cpp) reconstructs
`0x02070558..0x020705E0` in pure C++ (136 bytes). The four visual records begin
at manager `+0x1198`, each 276 bytes; the motion prefix starts eight bytes into
each record. It is embedded, not a separate scene-object allocation. Closing a
sub-screen window sets the manager's pending-upload bit, then detaches motion,
clears the complete visual record and calls the resident window close routine.
The live manager allocation is 5612 bytes, including four bytes beyond its
5608-byte structure; checks preserve that padding.

Private `build/runtime/eur_xhigh_battle_window_close/reward83_v2.json` verifies
one main-screen close at frame 605 of the established 2110-frame Smash Eggs
reward route. It models motion unlink, resident release, scroll-list removal
and tilemap rebuilding, checking the complete manager/window allocations and
separate tilemaps. The reward closes and battle continues. Five common captures
and their graphics dumps equal the earlier reward replay. The existing launch
counter/RNG fixtures remain declared; reloading the checkpoint restores all
main RAM and DTCM, and all 104 original saves retain their hashes.

`isolated_v1.json` adds 200 ARM946 cases for both screens, all four slots,
empty/absent/head/interior/tail motion lists, empty slots, scroll attachments
and remaining windows. The compiled caller and all executed helpers run without
stubs. Full main RAM and DTCM outside 256 active stack bytes, helper arguments
and callee-saved registers match the independent model. These cases supplement
the live main-screen route; sound-engine effects and asynchronous IRQ behavior
are not covered. The first live probe failed because it assumed allocation size
equaled structure size; its corrected successor checks the actual allocation.
`battle_window_close_validation.json` records source versions and artifacts.
All 23 functions in the affected window/reward objects remain exact (2896 bytes);
the full gate passes 107 tests, the original ROM hash and native relinking.

## Elder Princess Shroob projectile recall

[`Overlay25EffectSequence_RecallProjectiles`](../../src/elder_princess_shroob_ov025/ov25_effect_sequence_advance_sprite.cpp)
reconstructs overlay 25 `0x020C3A08..0x020C3C5C` (596 bytes) in C++.
After its signed countdown reaches zero, it recalls each active actor 44–49
toward actor 43, with a Z offset of -24. The native square-root unit supplies
the distance; motion channel 2 receives scaled acceleration 16. Context
`+27108` holds six active entries, while bit `i` of `+27132` selects child
parameter 56/57 and animation 17/19. Each child gets an RNG timer and an angle
from `atan2(dy-dz, dx)`. Positive X subtracts half a turn with unsigned
16-bit wrapping. Child callbacks occupy `work + 6800 + 36*(i+1)`.

Private `eur_xhigh_ov25_recall/live_v3.json` checks 100 calls over 1870 frames:
99 countdown updates and one recall of two projectiles. It verifies complete
7088-byte work state, the 12-byte arena slot, 70 scene records, timer/RNG/angle
results, hardware square-root inputs/results, native motion/list writes,
helper arguments and preserved registers. The work state belongs to a resource
arena; its slot supplies the size and owner. It has no individual heap header.
Animation changes in the receiving object/model allocations are bounded
observations; audio and rasterization are outside the independent oracle.

The route starts from the previously controlled giant-phase checkpoint derived
from save 103, rather than that save's normal Shrowser encounter. At the guarded
task dispatcher `0x020B60FC`, frame 320 replaces the primary callback
`0x020CBE28` with native initializer `0x020C4354`. Native code loads the resources
and initializes the anchor/projectiles. At frame 700 a second four-byte callback
edit changes `0x020C5DA4` to `0x020C3D78`, bypassing preceding motion/launch phases
that require additional attack-script state. Recall starts at frame 902 and
launches at 1001. This checks a partial sequence, not normal attack completion.
The final capture still shows the effect. Checkpoint reload restores all main
RAM and DTCM; all 104 original saves retain their hashes.

`isolated_v1.json` adds 225 ARM946 cases across positive/zero/negative timers,
five active-slot masks, three variant masks and three RNG seeds. All six slots,
both animation choices and positive/negative/zero X directions are covered.
The compiled caller and native lookup, motion, RNG and atan execute; animation
and audio use guarded stubs. Integer square-root/divider results model MMIO
without hardware latency or IRQ behavior. Full main RAM, DTCM outside the
observed stack, ordered helper arguments and callee-saved registers are checked.
These synthetic cases are separate from live gameplay coverage.

The failed discovery guard at frame 167 encountered another identified overlay.
Two failed oracle versions incorrectly assumed a heap allocation and omitted
the motion wrapper's Q8 scaling. Preserved reports and corrected runs distinguish
those probe errors from the matching game code. The final five-function build
object matches all 1660 bytes; the full gate passes 107 tests and both ROM checks.

### Staggered projectile launches

[`Overlay25Projectile_LaunchNext`](../../src/elder_princess_shroob_ov025/ov25_projectile_update_trail.cpp)
reconstructs `0x020C4FD8..0x020C5260` (648 bytes) in C++. After the initial
120-update delay set by `WaitSpin`, it skips inactive entries and launches each
remaining projectile 60 updates apart. Reaching index 6 switches to `WaitAll`.
The target's stored coordinates at actor offsets 24/26/28 determine the motion;
horizontal deltas use the signed division `(-4096*z)/(dz + 24)` and then
truncate toward zero after multiplication. Motion uses initial velocity 256
and acceleration 13 through the existing Q8 wrapper. Runtime flag bit 16 can
retarget the child to Mario or Luigi based on Mario's status eligibility.
The hit descriptor targets the party pair, retains the spawning enemy's ID,
and receives status 2 with chance/magnitude 5.

Private `eur_xhigh_ov25_sequential/live_v1.json` records 1001 neutral frames
from `eur_ov25_projectile_prepare/prepare103.dst` (SHA-1
`c2c8c46bf017c3f5a9b048ecfbf536dc68170d17`). That checkpoint has a controlled
origin: at the earlier guarded dispatcher, the mode-0 native initializer was
selected and the shared preparation table was set to actor IDs 56/57 plus
four empty slots. The new replay makes no RAM edits. All 240 calls are checked:
237 waits, launches at frames 150/210, and completion at frame 270 after skipping
slots 2–5. Battle continues through later boss callbacks. Full work/slot/scene
records, party records, motion/list outputs, angle and division results, hit
descriptor/list links and callee-saved registers match independent expectations.
Animation effects in receiving object/model records are bounded observations;
audio and graphics internals remain outside the oracle. The two common captures
and graphics dumps equal discovery; projectile and final battle images were
inspected. Checkpoint reload restores main RAM/DTCM, and 104 saves are unchanged.

`isolated_v1.json` adds 360 ARM946 cases, including skipped/all-empty masks,
indices -1/0/1/3/4/5, positive/zero/negative timers, both target variants,
retargeting with positive/zero Mario HP, and empty/already-active hit lists.
The compiled caller and native lookup, eligibility, division, atan, motion,
hit configuration and status helpers execute. Animation/audio are guarded stubs;
square-root/divider MMIO uses immediate modeled results. Full main RAM and DTCM
outside the observed stack are checked. The cases comprise 144 waits, 99 launches
and 117 completions; they do not establish normal gameplay entry or IRQ timing.
All three functions in the final source object match 924 native bytes. The full
gate passes 107 tests and both exact ROM checks.

### Spinning orb and projectile emission

[`Overlay25Projectile_UpdateEmission`](../../src/elder_princess_shroob_ov025/enemy_projectile_effects.cpp)
reconstructs `0x020C5374..0x020C5798` (1060 bytes) in C++. Each update rotates
the orb and adjusts its projected depth relative to its owner. Once movement
channel 2 finishes, it starts the next pair of model animations, selects the
Mario/Luigi effect variant and activates that slot's trail callback. Empty
slots are skipped. After the last slot, the native move uses object 50's X
directly and `192 - z`, then hands over to `WaitSpin`; the X argument is not
a coordinate difference. Movement duration retains the native signed Q8
conversion and division after the hardware square root.

Private `eur_xhigh_ov25_emission/live_v1.json` records 1070 frames from the
controlled giant-phase `phase103.dst` checkpoint (SHA-1
`f83e3507a0e81de426d54c5da1d4593a365d5f8c`). At guarded dispatcher frame 320,
the native mode-0 initializer is selected and its shared target table is set
to actor IDs 56/57 plus four empty slots. Native initialization and every later
phase run without another edit. This is controlled sequence coverage, not a
naturally selected story attack. All 55 controller calls pass: two advances,
52 waits and the final transition. Emissions occur at frames 571 and 581;
motion durations are 44, 10 and 49 updates. The controller reaches subsequent
spin/launch phases and the battle continues.

The oracle independently checks full work/slot/scene records, receiving model
allocations, projection inputs and stack outputs, rotation/depth stores, child
state, slot selection, motion/list effects, helper arguments and preserved
registers. Model-animation and animation-setting effects in receiving records
are bounded observations; animation pools, audio and rendering internals are
outside this oracle. All six screenshots and their VRAM/palette/OAM dumps equal
discovery. The orb and final battle images were inspected. Checkpoint reload
restores main RAM and DTCM; all 104 original saves are unchanged.

`isolated_v2.json` adds 768 ARM946 cases with the compiled controller and native
projection, object/model lookup, channel query and motion/list helpers. It covers
empty/mixed/full masks, indices -1/0/1/3/4/5, both effect variants, active/idle
motion, raw/projected positions, both views and zero/nonzero movement durations.
Synthetic slots share valid copied model allocations. Model-animation start,
animation setting and sound are guarded stubs; SQRT is immediate modeled I/O.
Full main RAM, DTCM outside the observed 112-byte stack, projected stack outputs,
helper order and ABI pass, with 35,296 stores. These cases do not verify hardware
latency, IRQ timing or live lifetimes. The first isolated attempt stopped before
execution because slot 50 has only an alternate model; the corrected fixture
uses the orb's valid primary model and slot 50's valid alternate model.

The final actual unit contains five exact functions totaling 2608 bytes.
The complete build gate passes 107 tests, the golden ROM hash and zero native
relink differences. No inline assembly was added.


## Three-part sub-screen drawing adapter

[`battle_three_part_draw.cpp`](../../src/battle/battle_three_part_draw.cpp)
converts a three-part effect's matrix into the alternate model's sprite position
and affine coefficients, invokes its virtual renderer, then registers the OAM
group. Translation divides by 256 and coefficients by 16, truncating toward zero
before halfword stores. The Y offset is a full-width argument. Depth remains a
full word and is reloaded after drawing for the sort key. Local byte counters
capture the renderer's output; their promoted differences go to `GameOam_AddGroup`.
That helper leaves all global counters unchanged when no objects were emitted,
even if the local affine counter advanced.

Private `build/runtime/eur_xhigh_three_part_draw/live_v1.json` checks 8,766 calls
over 8,820 frames using the established controlled Princess Shroob encounter:
6,058 empty draws, 752 first groups and 1,956 subsequent groups. No new RAM edits
were needed. Checks cover the complete 304-byte receiving model allocation,
64-byte transform, OAM counters and groups, 1,024-byte OAM output, helper arguments
and preserved registers. Virtual-renderer writes within the model, OAM buffer
and two stack counters are bounded observations; the adapter's conversions and
group bookkeeping are independently checked. The effect and final battle-menu
captures were inspected, without claiming an independent rasterization check.
The full checkpoint RAM and DTCM were restored; all 104 original saves are intact.

`isolated_v1.json` adds 216 ARM946 cases using the compiled adapter and native
group helper. They cover signed matrix extremes, halfword truncation, three Y
offsets, four counter results and first/subsequent groups. The virtual renderer
is an explicit stub, including a depth-alias case to test the post-call reload.
Full RAM, scratch storage, DTCM outside the observed stack, ordered stores and
the call ABI are checked. These cases add no live allocation-lifetime, renderer,
IRQ-timing or hardware coverage. The entire linked function matches without ASM;
the complete ROM and native relink pass, as do all 107 tests.


## Credits illustration texture uploads

[`credits_texture_upload.c`](../../src/title_ov006/credits_texture_upload.c)
uploads one 2,048-byte chunk from the credits' 32,768-byte staging buffer into
the inactive texture slot. The chunk index is task argument 0; workspace
`variant` selects which slot is already visible. Source and destination offsets
advance by 2,048 per chunk, and the inactive slot adds either 0 or 32,768 bytes.
The callback flushes the source cache range, begins texture loading, requests
the transfer, waits for completion while restoring texture-bank mappings, and
marks its task for removal. Marking does not establish actual pool release.

Private `build/runtime/eur_xhigh_credits_texture/live_v1.json` checks 176 calls
over 6,000 frames: 96 for variant 0 and 80 for variant 1, covering every chunk
0 through 15 in both slots. The established save-86 credits-entry fixture changes
one decoded command and restores its 72 bytes at the guarded scene request.
Each transfer is checked at `0x020387D0`, after the DMA wait and before bank
restoration. The entire mapped texture banks must match the preceding contents
with only the intended chunk replaced. All 360,448 uploaded bytes matched.

Checks also cover the full 72-byte task, 52-byte workspace prefix, 32,768-byte
staging buffer, GX state and bank registers, helper order and arguments, removal
flag and preserved registers. Bank mapping and loader globals follow the native
table and established bank contracts. Captures at frames 1,500 and 6,000 show the
credits illustrations and were inspected. Full RAM and DTCM were restored from
the checkpoint, and all 104 original saves remain unchanged. Palette uploads,
cache hardware timing, IRQ scheduling, pool release and independent pixel
rendering are outside these checks.

The complete callback and its existing loader caller compile exactly. The full
ROM rebuild, zero-difference native relink and 107 tests pass. The nearby palette
upload callback remains native and is not counted as reconstructed C.


## Shop current-item count labels

[`shop_owned_count_label.cpp`](../../src/shop_ov009/shop_owned_count_label.cpp)
keeps the four label strips beside the moving current-item count panel. Each
child copies its parent's Q12 position from task offsets 48/52 to its 64-byte
sprite and submits it at draw order 59. Workspace byte `+0xAE` is 1 while open;
[`shop_owned_count_close.c`](../../src/shop_ov009/shop_owned_count_close.c)
clears it to request the exit. The native parent eventually sets it to 2, when
the label callbacks mark their tasks for removal. Both callers discard the
close helper's return register; the C interface takes no arguments and is void.

Private `build/runtime/eur_xhigh_shop_owned/live_v1.json` checks 5,252 draws,
one close request and all four child-removal calls over 2,041 frames. Draws cover
the open and closing phases; the final capture confirms a visible field return.
The controlled save-65 shop-entry command is restored before ordinary navigation.
Checks include task and resource pool membership, complete 72-byte parent/child
tasks, 64-byte sprites, 2,492-byte workspace and touched draw-pool/list records.
Coordinates, helper arguments, list appends and removal/close flags are checked
independently. Marking tasks does not establish their subsequent pool release;
screenshots observe rendering without a pixel oracle. Full checkpoint RAM/DTCM
were restored, and all 104 original saves remain unchanged.

Both complete compiled functions match without ASM. The golden ROM, native
relink and 107 tests pass. The intervening setup, digit renderer and parent
motion callback remain native; they are not included in this coverage increase.


## Shop selling-price bonus

[`shop_sell_bonus.c`](../../src/shop_ov009/shop_sell_bonus.c) supplies the selling
bonus percentage to the inventory-price calculation and its percentage display.
The native discount helper sums Stache for consecutive displayed party members,
stopping at the first absent member. It computes `ceil(((sum + 100) / 8) *
factor / 100)` with the division by eight truncated first, then clamps to 5..50.
The reconstructed wrapper doubles that discount and caps the bonus at 99%.
The price caller applies this bonus to the item's base selling price; the
wrapper itself neither changes inventory nor transfers coins.

Private `build/runtime/eur_xhigh_shop_price_bonus/live_v1.json` checks 3,349
calls across seven item factors during 1,091 frames of selling-menu navigation.
Entry uses the previously verified save-65 shop command fixture, restored at the
scene request before navigation. The oracle independently predicts the native
discount and wrapper result, checks helper arguments and preserved registers,
and verifies unchanged scene allocation, workspace and live-save prefix. The
sell panel and final field return were visually inspected. All live calls use
four displayed party members and take the uncapped branch.

`isolated_v1.json` supplements that route with 224 ARM946 cases on copied RAM:
eight presence patterns, seven factors and four Stache values. These execute the
actual compiled wrapper and native discount with no stubs; 66 cases reach the
99% cap. Full main RAM remains unchanged, as does DTCM outside the 16-byte stack
frame; both ordered stack stores, helper inputs/results and preserved registers
are checked. Synthetic inputs do not establish live gameplay or IRQ behavior.
Checkpoint RAM/DTCM were restored and all 104 original saves are unchanged.

The 56-byte source object and complete ROM match exactly; native relinking and
107 tests pass. The discount helper remains native and contributes no new C
coverage in this batch.


## Scene axis-rotation channels

[`scene_axis_motion.c`](../../src/scene_menu_ov007/scene_axis_motion.c)
reconstructs the two functions behind Scene opcode `0x068`. Setup takes a channel
and eight signed halfword parameters: two axis endpoints, angular speed and
angle. It computes signed `angle * 256 / speed`, begins the channel and stores
all sixteen parameter bytes. Multiplication avoids shifting a negative signed
angle. The update copies the object's three coordinates to a six-byte local,
calls the native rotation helper with `speed * elapsed_q8 / 256` and copies the
result back. Division truncates toward zero. The 1,132-byte geometry helper
remains native and contributes no C coverage in this batch.

The fresh, byte-identical export of all three MenuAI scene archives contains
6,585 reachable commands and no `0x068` uses. That does not rule out uses in
unexported data. Consequently the runtime evidence is a controlled call fixture,
not a claimed ordinary script route. Private
`build/runtime/eur_xhigh_scene_axis/controlled_v1.json` enters the Bros. tutorial
through the pause menu, then invokes setup and update for each of the four
channels on embedded object 16 at a fully guarded Scene-VM boundary. The four
pairs execute at frame 245, with positive/negative speeds and elapsed values
0, 128, 256 and 384. Checks cover the full 43,056-byte manager allocation and its
heap header, all 56 object slots, motion-list state, channel initialization,
parameter stores, helper arguments, coordinate copying and preserved registers.
The geometry helper's six-byte output is a bounded observation; its mathematics
and hardware square-root timing are not independently checked.

Each pair restores the manager, list root and 544 bytes of caller stack before
the original VM command resumes. All four original commands return normally.
Five screenshots and twenty graphics captures match the unmodified 1,030-frame
baseline, including the tutorial and visible field return. Both replays restore
the complete checkpoint RAM/DTCM, and all 104 original saves remain unchanged.

`isolated_v1.json` adds 240 ARM946 cases on copied RAM: 128 setup cases covering
all channels, list insertion/reuse, active-channel replacement and deferred
deltas; and 112 updates covering signed inputs and fractional elapsed values.
The actual compiled functions and native division/channel helpers execute.
The update's geometry helper is explicitly stubbed to write three halfwords.
Checks cover full main RAM, DTCM outside the observed stack (at most 64 bytes),
3,016 ordered non-stack stores, helper inputs/results and preserved registers.
These cases use nonzero setup speeds and update products within signed 32-bit
range; they do not establish geometry, IRQ or object-lifetime behavior.

Both functions (280 bytes) and the updated 9,196-byte Scene dispatcher match
their native ranges exactly. The complete ROM, native relink and 107 tests pass.


## Scene sine-displacement setup

[`scene_axis_motion.c`](../../src/scene_menu_ov007/scene_axis_motion.c) also
implements `SceneObject_StartSineDisplacement` at `0x02085AB0`. Scene opcode
`0x057` passes the object, channel and seven full-width integers; argument 2 of
the script command is unused. Negative speed reverses both speed and phase.
Zero cycles sets duration to zero for indefinite motion. Otherwise setup uses
signed division of `speed - 1 + ((cycles << 16) - phase)` by speed and returns
without changing the object when the result is nonpositive. The C expression
uses unsigned intermediates to preserve the native 32-bit wrapping. Successful
setup starts the channel and stores six halfwords, preserving the remaining
four parameter bytes. The 348-byte sine-update callback remains native.

The native compiler division helper returns the numerator when its denominator
is zero. This behavior was read from its instructions and exercised with the
actual helper; the reconstruction retains that call. It is not an inferred
hardware division contract or a recommendation for valid motion inputs.

As with axis rotation, the 6,585 exported reachable scene commands contain no
`0x057` uses. Private `build/runtime/eur_xhigh_scene_sine/controlled_v1.json`
therefore uses six guarded calls in the initialized Bros. tutorial scene:
one indefinite, three finite and two rejected requests, including negative and
zero speeds. The complete 43,056-byte scene manager, heap header, object-slot
ownership and motion list are checked. Expected duration, child-helper effects,
parameter stores, early-return preservation and ABI are modeled independently.
Each call restores the scene and 544 caller-stack bytes before the original VM
command resumes. All six original commands return normally. Five screenshots
and twenty graphics captures match the unmodified 1,030-frame baseline; the
tutorial and field return were inspected. Both runs restore checkpoint RAM/DTCM,
and all 104 original saves remain unchanged.

`isolated_v1.json` verifies 320 ARM946 cases with the actual compiled setup and
native division/channel helpers, with no stubs. It covers all four channels,
list insertion/reuse, active-channel replacement, deferred deltas and twenty
phase/speed/cycle combinations, including `INT_MIN`, zero speed, cycle-shift
wrapping and halfword truncation. Outcomes are 48 indefinite, 80 finite and
192 rejected requests. Checks include full main RAM, DTCM outside the observed
stack (at most 72 bytes), 2,424 ordered non-stack stores, helper arguments/results
and preserved registers. These calls do not execute the sine-update callback or
establish animation, IRQ timing or subsequent object lifetime.

The new setup adds 168 matching C bytes. The adjacent rotation functions remain
exact in the renamed, contiguous 448-byte unit, as does the 9,196-byte dispatcher.
The complete ROM, native relink and 107 tests pass.


## Pause text-panel motion and strips

[`pause_text_panel.cpp`](../../src/scene_menu_ov007/pause_text_panel.cpp) owns
overlay 7 `0x0207B658..0x0207B864`: the 76-byte child-strip callback and the
448-byte panel updater. Both use 72-byte task-pool slots. The parent owns a
336-byte model; strips own 64-byte sprites and follow their parent's Q12 position.

The shared activity flag is the 32-bit word at pause workspace `+0x2B8`.
It is distinct from the equipment flag at `+0x2C0`. A visible empty list clears
the panel flag. Phase 0 applies acceleration, velocity and position in that order,
then decrements the timer. Reaching zero snaps to signed halfword pixel targets;
an initial zero instead becomes -1. Closing starts at phase 1000 with vertical
velocity of eight pixels per update, reversed for the upper panel. Phase 1001
marks the parent only after it passes -48 or 240 pixels; equality still draws.
Strips mark themselves when the parent's closing halfword becomes nonzero.

Both pixel conversions are evaluated before the model's halfword stores.
This source data flow explains the six instruction differences in the inherited
draft; the resulting functions are pure C++ and match in the actual build object.
Multiplying signed targets by 4096 preserves the native scale without shifting
negative signed values.

Private `build/runtime/eur_xhigh_pause_text_panel/clothing_v2.json` records a
1,970-frame ordinary save-65 clothing-menu route: 1,359 panel updates and 13,468
strip updates. Six panels arrive and later exit, three at each screen edge;
49 child strips mark themselves. All 55 task creations and removal returns are
tracked. The oracle checks complete task, parent, model/sprite, 90,600-byte pause
workspace, 4,428-byte party allocation and header, save prefix and display
records, plus draw-pool/list changes and helper ABI. Submitted draw entries are
verified independently; this is not an independent rasterization proof.

The first route (`clothing_v1`) ended after Start reopened the pause menu and
failed its final lifetime check. The corrected producer retains that exact
prefix, whose screenshots and graphics hashes agree, and closes the menu with
one additional Start/wait pair. The clothing screen and final field screen were
visually inspected. Both runs restore checkpoint RAM/DTCM; all 104 saves remain
unchanged. Removal returns are observed, without claiming an independent proof
of the allocator's cleanup internals.

`isolated_v1.json` adds 116 ARM946 cases on copied RAM: visible/hidden and empty
lists, independent activity flags, timer 0/1/2, both exit thresholds just below,
equal and above, signed coordinate truncation and full-width child positions.
The actual compiled callbacks and native get/count/mark/draw/pool helpers run
without stubs. Full RAM and DTCM outside the observed stack (at most 32 bytes),
1,297 ordered nonstack stores, helper results and preserved registers are checked.
These synthetic cases do not establish additional live lifetimes or IRQ timing.


## Pause bean digits and dismissal helpers

[`pause_bean_digit.cpp`](../../src/scene_menu_ov007/pause_bean_digit.cpp),
[`pause_menu_cursor_close.cpp`](../../src/scene_menu_ov007/pause_menu_cursor_close.cpp)
and [`pause_text_panel_close.cpp`](../../src/scene_menu_ov007/pause_text_panel_close.cpp)
reconstruct three small overlay-7 helpers: `0x02073708` (76 bytes), `0x0207FE60`
(48 bytes) and `0x0207B2C8` (20 bytes). All are pure C++.

The bean callback obtains its 64-byte sprite, reads the selected row, resolves
the item through the party's circular list and submits the sprite at priority 41
only for item 0. Selecting another key item hides the pre-existing digits.
Cursor dismissal marks every task in group 3 and resets palette-effect entry 0
to the native default: zero fields except Q8 speed 256 and paused flag 1.
The text-panel close request clears the workspace word at `+0x2B8`; the existing
panel callback consumes it on a later update.

Private `build/runtime/eur_xhigh_pause_menu_tails/keys_v1.json` is an ordinary
1,404-frame save-65 route. It checks 1,989 bean callbacks: 1,263 submissions while
Beans is selected and 726 hidden calls for Toadbert's Drawing. All three digit
task creations and removal returns are observed. One cursor dismissal marks
seven group-3 tasks and resets the palette entry; four panel-close requests
clear an active flag. No RAM fixtures are used.

The oracle checks full task/sprite records, the 90,600-byte pause workspace,
the 4,428-byte party allocation and header, save prefix, display records,
selected-row/item helper results and independently derived draw-list changes.
Cursor checks include all marked task records and list roots, the complete
12-byte palette-controller allocation and header, and its entries. The default
entry's 20 expected bytes are independently specified and checked against ROM.
Stack and callee-saved registers are preserved. Task removal returns are observed;
allocator cleanup internals, rasterization and IRQ timing are outside this probe.

The Beans screen, hidden digits on Toadbert's Drawing and final field screen were
visually inspected. Eight screenshot/graphics captures are retained. Checkpoint
RAM/DTCM is restored and all 104 saves remain unchanged. All three actual source
objects and every function in the three migrated caller units match; the full
ROM, native relink and 107 tests pass. The bean-digit creator remains native.


## Pause equipment dismissal

`MenuEquipment_RequestClose` (`0x02079D08`, 20 bytes) clears the workspace word
at `+0x2C0`, which the equipment/status display tasks consume to begin closing.
`PauseEquipment_CloseItemSelection` (`0x0207B864`, 40 bytes) marks task group 3
and unlinks the shared sub-screen tile allocation at workspace `+0x2F8`.
Both are pure C++; the status-page caller uses the named close API.

Private `build/runtime/eur_xhigh_pause_cleanup/combined_v3.json` records an
ordinary 1,730-frame save-65 route through status and clothing selection, with
no RAM fixtures. Each helper executes once. The oracle checks the cleared flag,
13 marked tasks and the group roots, the entire 90,600-byte workspace, party
allocation and header, save prefix and display records. At the unlink call it
checks all 31 sprite-allocation records on both screen lists and their roots.
The target allocation is already detached, with zero neighbors and its linked
bit clear; the native helper tolerates this. This route does not cover unlinking
a currently linked allocation or prove subsequent task-pool cleanup.

The clothing-selection and final field screens were inspected. Nine screenshots
and 36 graphics dumps are retained; graphics are observational. Helper arguments,
stack and preserved registers pass; full checkpoint RAM/DTCM is restored and all
104 saves are unchanged. Both actual source objects and the migrated caller unit
match completely. Golden ROM, zero native differences and 107 tests pass.

Two failed probes remain recorded. `combined_v1` reached the status helper but
used the consumable-item route and missed equipment cleanup; its planned HP
fixture never ran. `combined_v2` reached cleanup but incorrectly required a
linked tile allocation. Native inspection explained the detached case; the
corrected oracle in `combined_v3` preserves all seven earlier captures and
their 28 graphics hashes. Both failures restored the checkpoint and saves.


## Initial game heap regions

`GameHeap_Initialize` at resident ARM9 `0x02029C04` reconstructs 480 bytes of
pure C and extends the existing [allocator](../../src/game/heap.c). It clears
all 32 region records, configures the first five and initializes their block
headers. Heap 1 is the occupied high-end sentinel linked to heap 0; the other
initial blocks are free. The block heap ID uses five bits and preserves the
remaining header bits.

| Heap | First block | Payload bytes | Role |
|---:|---:|---:|---|
| 0 | `0x020CC200` | 3,227,104 | Main RAM |
| 1 | `0x023DFFF0` | 0 | Main-RAM end sentinel |
| 2 | `0x01FF87A0` | 30,800 | ITCM |
| 3 | `0x027E0060` | 15,120 | DTCM |
| 4 | `0x027FF000` | 3,056 | Shared RAM |

The native literal pool retains link-time quantities: main/ITCM/DTCM low
bounds, a 1,024-byte IRQ stack reservation and a zero system-stack reservation.
The existing DTCM-start symbol supplies `0x027E0000`. Explicit constants in
`config/eur/arm9/linker_aliases.json` keep this arithmetic in C without folding
it to final addresses; the linker assigns values without allocating bytes.
These names describe recovered roles, not recovered original symbol spellings.
The native initializer writes first pointers, cursors, sizes and IDs in groups.
Preserving that order reproduces every instruction and literal. All ten
functions in the compiled allocator object, totaling 1,500 bytes, match.

Private `build/runtime/eur_xhigh_heap_initialize/boot55_v2.json` records one
ordinary initializer call at frame 5 of a 1,200-frame cold boot. Its independent
oracle checks the complete 512-byte region table and all 3,276,160 bytes in the
five initial regions, including headers, untouched payload and the function's
40 stack bytes. It checks the clear helper's arguments/output, SP and r4-r11.
The first probe failed because it assumed all payload bytes were untouched:
the initial DTCM region includes the current boot stack, whose frame spans
`0x027E3B4C..0x027E3B74`. The corrected probe models every saved register and
the stack-resident zero; the failed producer and report remain separate.

The title screen was inspected. Four graphics dumps are observational; this
does not independently verify rasterization or IRQ timing. There were no RAM
fixtures, all 104 original saves stayed unchanged, and the emulator was
destroyed after the replay. Natural header reserved bits were zero, so this
route does not claim runtime coverage of nonzero reserved-bit preservation.
The full build, golden ROM hash, zero-difference native relink and 112 tests
pass. This is game allocator setup; SDK arena definitions supplied context,
while all arithmetic and matching came from PiT's binary.


## Moving the main heap boundary

`GameHeap_RebaseMain` (`0x0202974C`, 188 bytes) moves the main region's first
block past scene storage. It saves all four header words before moving them,
updates the first/search pointers, adjusts both the region size and the first
block's payload size by the old-minus-new address difference, and repairs the
back-link at heap 1's cursor. `GameHeapBlock.heap_flags` provides a raw view
alongside the existing five-bit heap ID, preserving the other header bits.

The native boundary expression is `(boundary + 1023) & ~511`, with unsigned
wrapping. Zero first substitutes that expression applied to the EUR main arena
low address, then applies it again: the resulting default is `0x020CC400`,
512 bytes above the initial `0x020CC200` block. Do not simplify this to ordinary
512-byte alignment. Native dataflow also computes the boundary before copying
the header; this ends the incoming argument's lifetime before the four-register
copy. Those facts produce an exact pure-C reconstruction without register hints.

The adjacent `GameHeap_InitializeSystem` (`0x02029730`, 28 bytes) calls `OS_Init`
before `GameHeap_Initialize`. Both extend the existing allocator unit. Its 12
actual compiled functions total 1,716 matching bytes; the migrated battle
creator's five-function unit also matches completely (2,512 bytes).

Private `build/runtime/eur_xhigh_heap_boundary/boot55_v1.json` checks the startup
wrapper's call order, return stack and preserved registers, repeats the complete
heap-initializer oracle, and checks three boundary moves during a normal
1,200-frame boot. `pause65_v1.json` checks three more during a 440-frame ordinary
pause-menu round trip. Each mover check covers the full 512-byte region table,
the complete contiguous main-heap span containing both headers and the end
cursor, the copied header, all 32 stack-frame bytes, SP and r4-r11. Together the
routes exercise expanding/shrinking moves and explicit/default boundaries.
They do not cover identical source/destination headers or arithmetic overflow.

The title, pause menu and field return were inspected. A repeated unchanged
boot (`boot55_v2.json`) passes again with equal initializer inputs, initialized
table, mover inputs and resulting tables. Its title image and OAM capture differ;
both BG captures and palettes match. Earlier boot captures also differed. These
are preserved observations, with no confirmed cause or claim of pixel equality.
Graphics/IRQ timing and `OS_Init` internals are outside the wrapper oracle.
There were no RAM fixtures; pause-checkpoint RAM/DTCM was fully restored, all
104 saves stayed unchanged, and all emulator instances were destroyed.
Full build, golden ROM, zero-difference native relinking and 112 tests pass.


## Hammer progression

[Hammer progression](../../src/attack_hammer_ov021/hammer_progression.cpp) adds
the 832-byte updater at overlay 21 `0x020C39B0`. It caches the scene object,
active model and configuration before processing input, then reads the new phase.
Impact phases count down once, dispatch with `result == 1` and `phase == 10`,
set the timer to -1, and wait for animation completion before returning.
Early input has its own advance and repeating sound path.

Eight inline-ASM instructions preserve the native interleaving of the result
field extraction and the two boolean tests. The pure C++ candidate is the same
size but differs in eleven instruction words. The fragment fixes that scheduling
and phase-register difference; the complete 832-byte function matches. Entry
and approach are consolidated into the same contiguous unit. All five actual
compiled overlay units, totaling 5,280 bytes, match; this completes its mapped
code coverage, without claiming every possible runtime state is tested.

Private evidence is in `build/analysis/xhigh_from_55/hammer_progression_*` and
`build/runtime/eur_xhigh_hammer_progression/`. Five ordinary save-22 routes cover
primary success/timeout, early input, and secondary success/timeout: 10,050 frames
and 710 updater calls, visiting phases 0 through 16. Each route dispatches one
impact; observed `(success, advanced)` pairs are `(1,0)`, `(0,0)` and `(0,1)`.
The full native function and called helpers are guarded. Checks cover ordered
call arguments, the full 28-byte attack state between helper boundaries,
unchanged 64-byte configuration, SP and r4-r11. Helper effects are bounded
observations; damage, RNG, graphics transfers and rasterization are not independent
oracles. Success/early-input captures and the final battle menu were inspected.

The four `*_v1` routes use `probe_hammer_progression.py`; the secondary timeout
uses `probe_hammer_progression_v2.py`. Both use `hammer_progression_oracle.py`.
All producers exit successfully, restore the full RAM/DTCM checkpoint and leave
all 104 original saves unchanged. No live RAM fixtures are used. Full ROM/native
verification and 112 tests pass. Invalid phases, signed-overflow timer inputs,
zero arrival delay and alternate formations are outside this live coverage.


## Save-menu location panel requests

[Save panel resources](../../src/save_menu_ov008/save_menu_panel_resources.c)
now includes `SaveMenu_CopyLocationPanel` and `SaveMenu_QueueLocationPanel`, the
96-byte range `0x02071A50..0x02071AB0` in overlay 8. The former resolves source
panels 0 and 1 through the two-byte mapping at `0x0207AA64`; every larger unsigned
value selects image buffer 2. It updates the destination mapping before queuing
the transfer. The factory allocates a 72-byte overlay-5 task in list 11, marker 1,
with callback `0x02071AB0`, then stores full-width panel/image arguments at
offsets 40 and 44. Both functions return the allocated task pointer. The upload
callback remains native code; it narrows those arguments before selecting its
graphics buffers. Do not interpret arbitrary factory arguments as valid uploads.

The adjacent location-label helpers share the same source unit, renamed from
`save_menu_panel_text.c` using the reorganization tool. All four functions,
332 bytes, match in the actual compiled object. The new functions are pure C.
The complete build reproduces the original ROM, the native relink has zero
differing bytes, and all 112 tests pass.

Private evidence is under `build/runtime/eur_xhigh_save_location_requests/`.
`copy55_v3.json` follows ordinary buttons from the existing copy-confirmation
checkpoint: 698 frames, one panel-0-to-panel-1 copy, followed by loading the game.
`erase55_v3.json` starts at the second deletion confirmation: 688 frames and
one buffer-2 request for panel 0. Each route checks one allocation, upload,
removal mark and actual release. The oracle derives every pool/list/counter
change, checks all allocated pool records and link records plus the complete
10,248-byte menu workspace, and checks arguments, returned pointers and preserved
registers/stack. The upload's 72-byte task and arguments are checked; its graphics
writes remain observational. Main/sub VRAM, palettes and OAM are captured, and
the copied panel, empty erased panel and resumed field were visually inspected.

The first erase attempt (`erase55_v1.json`, producer exit 1) used a checkpoint
already past deletion and correctly failed its target-coverage assertion. The
earlier checkpoint fixes the route; no game code or live RAM fixture was needed.
Both successful routes restore the entire checkpoint RAM and DTCM afterward.
All 104 original save files remain unchanged. Their final producer is
`build/analysis/xhigh_from_55/probe_save_location_requests_v3.py`.

`isolated_v2.json` adds 164 ARM946 cases on copied live RAM: all nine combinations
of the two mapping bytes, both destination panels, eight unsigned source values
including `0xFFFFFFFF`, and twenty direct factory argument cases. Actual compiled
requests and native allocator helpers execute without stubs. All 3,096 ordered
stores, 636 function entries, helper arguments, returned pointers and mapped
memory outside the 32-byte CPU stack frame are checked. These synthetic cases
do not run the upload callback or establish live IRQ behavior. The separate
live routes supply task-lifetime and visible-menu evidence.


## Session save-context initialization

[Session save initialization](../../src/game/session_save_context.cpp) reconstructs
the resident range `0x02005BA8..0x02005D74` (460 bytes, pure C++). It returns
immediately if `gSaveData` already exists. Otherwise it allocates and clears
1,420 bytes from heap 1, copies supplied settings or the defaults at
`0x02048F08`, derives runtime options, and initializes the party and text tables.
Supplied settings additionally copy their two final words to script storage at
context offset `0x48`. The default path takes the rumble option from bit 0 of
`0x0205A00C`.

The shared `GameSessionSettings` layout now reflects signed halfwords at offsets
2 and 4 and alignment padding at offset 6. Native C++ memberwise assignment
copies fields but leaves that padding alone. The settings remain 16 bytes, with
their final two words at offset 8. All seven actual functions in the new unit
and the two affected session units match: 1,248 bytes, including 460 new bytes.
The full gate reproduces the original ROM, reports zero native differing bytes
and passes all 112 tests.

Private replay `build/runtime/eur_xhigh_session_save/boot65_v2.json` checks an
ordinary cold boot and load from save 65: 2,477 frames, one supplied-settings
initialization and all five ordered helper calls. The full allocation, settings,
root pointer, rumble byte, stack and preserved registers are checked. Heap
allocation writes are independently derived and verified before text loading.
Party effects are observed only within 284 bytes at context `+0x3F8`; text-table
writes are observed only within 56 bytes at `+0x520`. The text loader's further
allocations retire the initial neighboring heap metadata from this caller oracle.
An earlier probe failed because it incorrectly required that metadata to remain
unchanged through text loading; `boot65_v1.json` and its producer remain preserved.
Main/sub VRAM, palettes and OAM are captured, and the loaded field was visually
checked. All 104 original saves are unchanged; no live RAM fixtures were used.

`isolated_v1.json` supplements the ordinary route with 262 copied-RAM ARM946
cases: 256 supplied-settings combinations, four default rumble-byte values and
two existing-context returns. It checks the actual compiled initializer with
native allocation, clear and copy helpers, all mapped memory outside 44 stack
bytes, helper order/arguments and 2,604 ordered session stores. Party and text
initialization are explicit no-op stubs in these cases. Synthetic language bits
check copying and extraction, not valid gameplay language configurations; these
cases do not establish live IRQ, resource lifetime or rendering behavior.
