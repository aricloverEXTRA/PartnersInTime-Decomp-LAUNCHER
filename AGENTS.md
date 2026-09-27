# Working on Partners in Time

Reconstruct Mario & Luigi: Partners in Time as readable, editable C/C++ that
reproduces the European Nintendo DS game byte for byte. This decompilation is
fully generated with generative AI under human direction. Preserve the README
disclosure and [AI policy](docs/AI_USAGE.md).

These instructions apply throughout the repository. Detailed addresses, compiler
examples and tested routes live in the
[reconstruction reference](docs/research/RECONSTRUCTION_NOTES.md).

Start with [checkout and handoff](#start-in-the-correct-checkout), then use
[reconstruction](#reconstruct-and-integrate), [build checks](#build-and-verification),
[runtime checks](#runtime-verification) and [publication](#documentation-git-and-handoff)
for the current task.

## Session checklist

1. Read the latest request and stopping point. Use the handoff to recover work,
   then verify it; do not let an older continuation instruction start a code
   batch during a documentation or research task.
2. Enter the active checkout and inspect the branch, remotes, working tree and
   index. Record unrelated dirty paths before editing. Preserve their raw bytes
   as well as staged entries; normalized Git diffs can hide line-ending changes.
3. Read current progress and the relevant source, metadata and research sections.
   For reconstruction, identify one coherent group of genuinely unlinked ranges
   and a usable runtime route before investing in a candidate.
4. Establish what success means for this task: documentation checks, a tool test,
   or full native matching, integration and focused runtime coverage. Complete
   the applicable checks; record failures and untested paths explicitly.
5. Review and publish only verified task-owned changes. Update the private
   handoff with unfinished work and evidence, then stop at the requested boundary.

## Priorities and scope

- Follow the latest user request and requested stopping point. Research or
  documentation requests do not start another reconstruction batch. A handoff
  provides context; it does not authorize resuming a superseded task.
- Prefer fast, well-understood progress in coherent groups. Readability and
  exact matching both matter. Use native instructions, callers, shared types
  and runtime evidence. Do not brute-force source permutations or spend hours
  guessing at register allocation. Record difficult gaps and move on; revisit
  them only when new evidence explains the remaining difference.
- Small, explained inline-assembly fragments are authorized when necessary.
  Verify the whole function and disclose the fragment. Do not wrap machine-code
  dumps in C to inflate progress or mix symbolic ASM into the C/C++ percentage.
- Complete authorized work without repeated permission requests. Verified
  intermediate commits and pushes to the user's `origin` are authorized.
  Give concise progress updates and report what was actually checked.
- Use subagents only when the user or applicable instructions explicitly
  authorize delegation. Old agent names in session context are not authorization.

## Start in the correct checkout

The active checkout on this workstation is:
`C:\Users\Julian\Desktop\PartnersInTime-Decomp`.
`D:\NDS\Partners in Time` holds private research and points here through its
own `AGENTS.md`. Its `References/PartnersInTime-Decomp` is an older reference
clone. Verify these paths on another machine.

Before editing:

```powershell
Set-Location -LiteralPath 'C:\Users\Julian\Desktop\PartnersInTime-Decomp'
git status --short
git diff --cached --name-only
git branch --show-current
git log -5 --oneline
git remote -v
```

The user's remote is `https://github.com/juliangrtz/PartnersInTime-Decomp.git`;
work has used `main`. `upstream` is for reference. Preserve unrelated changes,
use the configured Git identity, and stage explicit task-owned paths. History
rewriting and attribution changes are separate tasks, not routine housekeeping.
Use descriptive commit titles without appended matching-byte counts.

Read private `build/analysis/CURRENT_HANDOFF.md` if present, then verify its
claims against Git, source and artifacts. It can be stale even at the same
`HEAD`: another private draft or uncommitted batch may have appeared afterward.
Inventory pending files and check provenance before reusing an integration script.
An object file's existence or timestamp does not prove its comparison passed.
If the previous command's output was lost, record its result as unverified until
the relevant check is recovered or rerun; preserve the draft in the meantime.

Discover actual paths with `rg --files` before reading them; do not hide failed
reads of guessed filenames. Use scoped `rg` searches and `rg -l` when only filenames
are needed. Quote PowerShell paths containing spaces; use `-g 'pattern'` for globs.
Parse large JSON reports and print selected records; recursive text searches can
dump entire single-line databases. Serialize builds, metadata edits and replays
that share ROM/save paths.

## Find the relevant evidence

| Need | Resource |
|---|---|
| Coverage and counting rules | [Progress JSON](docs/progress.json), [metric](docs/PROGRESS.md) |
| Remaining reconstruction and refactoring audit | [Roadmap](docs/DECOMPILATION_ROADMAP.md); regenerate the private inventory with `python tools/decompilation_backlog.py` |
| Source actually used in the ROM | `config/eur/arm9/linked_sources.txt` |
| Boundaries, load addresses and references | Resident `config/eur/arm9/{symbols,delinks,relocs}.txt`; overlays `config/eur/arm9/overlays/ovNNN/`; candidate object symbols and relocations |
| Source organization and subsystem roles | [Source policy](docs/DECOMPILATION_STYLE.md), [overlay map](docs/research/OVERLAY_MAP.md), [battle map](docs/research/BATTLE_MAP.md) |
| VMs | [VM maintenance](#vm-maintenance), [script VM semantics](docs/research/SCRIPT_VM_SEMANTICS.md), [Scene VM matching](docs/research/SCENE_VM_MATCHING.md) |
| Native inspection and relinking | [IDA guide](tools/ida/README.md), [reassembly plan](docs/REASSEMBLY_PLAN.md) |
| Emulator tooling and navigation | [Runtime guide](docs/research/RUNTIME_ANALYSIS.md), [tested routes](docs/research/RECONSTRUCTION_NOTES.md#runtime-verification) |
| RAM roots, object extents and graphics ranges | [EUR memory reference](docs/research/RECONSTRUCTION_NOTES.md#eur-memory-reference) |
| NitroSDK API and type research | [Cross-game reference and PiT differences](docs/research/RECONSTRUCTION_NOTES.md#nitrosdk-cross-game-reference) |
| ABI and compiler lessons | [Reconstruction reference](docs/research/RECONSTRUCTION_NOTES.md#reconstructing-and-integrating-code) |
| Battle effect wrappers and scene-object ownership | [Projection, returned handles and embedded object records](docs/research/RECONSTRUCTION_NOTES.md#battle-relative-effect-spawning) |
| Battle scheduler, queues and isolated ARM checks | [Allocation layout and boundary cases](docs/research/RECONSTRUCTION_NOTES.md#battle-scheduler-queues), [VBlank callback contracts and live replay](docs/research/RECONSTRUCTION_NOTES.md#battle-scheduler-vblank-consumer) |
| Battle scheduler construction and teardown | [Heap/file contracts, overlay transitions and replay limits](docs/research/RECONSTRUCTION_NOTES.md#battle-scheduler-lifecycle) |
| Shared sprite collection and initialization | [Renderer arguments, pool layouts and live versus isolated coverage](docs/research/RECONSTRUCTION_NOTES.md#overlay-5-sprite-collection-and-initialization) |
| Battle exit and resource slots | [Dispatch boundaries, overlay replacement and coverage limits](docs/research/RECONSTRUCTION_NOTES.md#battle-transition-dispatch-and-resource-slots) |
| Battle model animations and Mix Flowers | [Track/context ownership, model-table sentinels, wrapper ABI and renderer extents](docs/research/RECONSTRUCTION_NOTES.md#battle-model-animation-starts) |
| Task, heap and archive lifecycle | [Normal tasks](src/game/task.cpp), [IRQ tasks](src/game/task.cpp), [allocator](src/game/heap.c), [archive base](src/game/archive_io.c), [compressed archive](src/game/archive_compressed.c) |
| Hit-bonus arithmetic and RNG fixtures | [Conversion ABI, truncation and restoration](docs/research/RECONSTRUCTION_NOTES.md#battle-hit-bonus-roll) |
| Pause transitions | [Party lifecycle](docs/research/RECONSTRUCTION_NOTES.md#pause-party-initialization-and-cleanup), [transition evidence](docs/research/RECONSTRUCTION_NOTES.md#pause-transition-panels-and-controllers), [projection ABI](docs/research/RECONSTRUCTION_NOTES.md#pause-transition-projection-and-callback-abi), [setup calls](docs/research/RECONSTRUCTION_NOTES.md#pause-transition-setup-calls) |
| Pause navigation | [Page entry and return](docs/research/RECONSTRUCTION_NOTES.md#pause-page-entry-and-return), [main menu and member selection](docs/research/RECONSTRUCTION_NOTES.md#pause-main-menu-and-member-selection), [status and Cobalt Star pages](docs/research/RECONSTRUCTION_NOTES.md#pause-status-and-cobalt-star-pages) |
| Pause list rendering | [Row sprites](docs/research/RECONSTRUCTION_NOTES.md#pause-list-row-sprites), [queued drawing and markers](docs/research/RECONSTRUCTION_NOTES.md#pause-queued-row-drawing-and-markers), [row refresh](docs/research/RECONSTRUCTION_NOTES.md#pause-list-row-refresh) |
| Equipment list models | [Equipped-item markers](docs/research/RECONSTRUCTION_NOTES.md#pause-equipped-item-markers), [empty-equipment row sprites and ResourceA lifetime](docs/research/RECONSTRUCTION_NOTES.md#pause-empty-equipment-row-sprites) |
| Category and list selection sprites | [Task layouts, Q12 coordinates, model attachments and tested scrolling routes](docs/research/RECONSTRUCTION_NOTES.md#pause-selection-sprites) |
| Item-selection labels and quantities | [Label strips and screen offsets](docs/research/RECONSTRUCTION_NOTES.md#pause-item-selection-label-sprites), [quantity digits, hidden tens and boundary fixtures](docs/research/RECONSTRUCTION_NOTES.md#pause-item-selection-quantity-sprites) |
| Badge descriptions | [Panel/strip ownership, description lookup, graphics extents and suppression fixtures](docs/research/RECONSTRUCTION_NOTES.md#pause-badge-description-panels) |
| Equipment member-selection arrows | [Heading-switch inputs, failed routes and verified updater](docs/research/RECONSTRUCTION_NOTES.md#pause-member-selection-arrows) |
| Equipment stat comparisons | [Save-field addressing, cached rows, numeric helper ABI and runtime counts](docs/research/RECONSTRUCTION_NOTES.md#pause-equipment-stat-comparison-rows) |
| Party status and low-HP warnings | [Bitmap and status fields](docs/research/RECONSTRUCTION_NOTES.md#pause-party-status), [bitmap transitions and spring workspace](docs/research/RECONSTRUCTION_NOTES.md#pause-party-bitmap-transitions), [warning modes, threshold fixtures and lifetime checks](docs/research/RECONSTRUCTION_NOTES.md#pause-low-hp-warnings) |
| Unused Nawatobi minigame | [Guarded entry, exact RAM edit and research limits](docs/research/RECONSTRUCTION_NOTES.md#nawatobi) |
| Earlier batch evidence | [Milestone log](docs/BATTLE_MATCHING_MILESTONES.md); private reports linked there |
| Assets and publication boundaries | [Data modding](docs/DATA_MODDING.md), [private-content rules](docs/LOCAL_PRIVATE_CONTENT.md) |

Native bytes are private under `extract/eur/arm9/` and
`extract/eur/arm9_overlays/`; IDA databases live in `build/ida/`, and experiments
in `build/analysis/`, including `local_decompiler/`. The EUR resident `arm9.bin`
loads at `0x02004000`, not the start of main RAM. For overlay files, read the
component's `base_address` from `extract/eur/arm9_overlays/overlays.yaml` and
compute `file_offset = virtual_address - base_address`. In the verified EUR
extraction, overlays 2 and 5 load at `0x02065D40`, and overlay 7 at `0x0206AB80`.
Do not substitute a nearby function address for the load base. Check the file
bounds, function boundary and CPU/overlay identity before interpreting a listing.

Private scripts, snapshots and reports are local conveniences, not fresh-clone
dependencies. Check availability, inputs and source/ROM provenance before reuse.
If absent, establish the needed evidence with public tools and user-supplied
ROM/saves. Current native bytes, source, metadata and fresh checks take precedence
over historical notes. Check existing declarations before accepting a handoff's
claim that a shared prototype needs changing.

Use `src/game/` for resident game helpers, `src/nitro/` for SDK code,
`src/field/` (overlay 0) and `src/battle/` (overlay 2) for those subsystems, and
`include/game/` for shared game interfaces. Every other overlay has its own
directory named `<role>_ovNNN`, for example `shop_ov009` or
`attack_pocket_chomp_ov018`; run `ls src` for the current list. Private
pseudocode folder names are not authoritative source paths or component
identities.

A source file is one translation unit that owns one contiguous native range per
section. Use `tools/reorganize_sources.py` to rename or merge units: it checks
contiguity, ordering, language and basename uniqueness, and updates
`delinks.txt`, `linked_sources.txt` and `linker_aliases.json` together. Merge
only units whose ranges already touch and that serve one topic; run
`tools/verify_refactor.ps1` afterwards.

When checking candidate inventories, compare component and address ranges against
the current linked manifest and delinks. Accept variable whitespace in metadata;
ignore blank/comment manifest entries and unnamed component-wide section headings.
Otherwise an inventory can silently miss linked ranges or count the entire
component as owned. Confirm a proposed new range directly before reconstructing
or counting it; historical `EXACT` records are only discovery leads.
Check public definitions, private drafts and archived handoffs before calling a
candidate unattempted. Account for semantic renames; a prototype is not a body,
and a regex scan can miss definitions. A size-filtered inventory omits smaller
helpers, while a short pseudocode body can hide an indirect dispatcher or an
unusual register ABI. Inspect native instructions before ranking a target as easy.
Inspect code-bearing sections such as `.init` as well as `.text`. Global C++
construction can generate registration code and tables; recover their ownership
and section placement before proposing a unit. Do not move code between sections
or count initialization data merely to make the remaining inventory smaller.

## Reconstruct and integrate

1. Inspect the complete native range, callers and neighbors. Pseudocode can
   omit stack arguments, repeated FIFO stores and cached loads, or invent return
   values. Check literal pools too: disassemblers can stop at data or decode it
   as instructions. Choose a group with understood behavior and a usable route.
   Read sizes from current symbol metadata and compute the end-exclusive range;
   preserve both hexadecimal and decimal sizes in candidate records. Recalculate
   inherited sizes rather than treating a handoff's rounded estimate as a boundary.
2. Recover field widths, signedness, offsets, allocation sizes, ownership and
   virtual interfaces. Preserve arithmetic widths, integer promotion, division
   toward zero, Q12 operation order and exact truncation points. Derive each
   comparison's signedness separately. A byte/halfword access, including a load
   from a stack argument, does not establish a narrow parameter. Check caller-side
   extensions, stack and hidden ABI arguments. A register left over from a previous
   call is not an argument unless the next callee consumes its incoming value.
   Conversely, an existing prototype can omit forwarded arguments. Trace incoming
   registers through the callee and check the virtual interface before diagnosing
   a wrapper's register mismatch as a compiler problem. Restoring a missing
   argument can explain both register preservation and stack layout without hacks.
   For indirect calls, inspect the callback table and its callees: a register can
   retain a required argument from an earlier call even without a new `mov` before
   `blx`. Recover the callback type from those live inputs, not pseudocode arity.
   A later mask or narrow store also does not establish a narrow parameter;
   a full-word stack load followed by truncation may require a full-width type.
   Recover each wrapper's contract independently: it can accept signed halfwords
   and forward extended values to a factory with full-width parameters. Do not
   propagate one signature along the call chain merely to make types uniform.
   A caller ignoring `r0` does not establish a `void` return type. Check the
   callee's return paths and consumers before correcting a shared declaration;
   preserve the actual helper instead of substituting a similarly named API.
   Compiler runtime helpers also have distinct signed and unsigned contracts.
   Identify their native conversion and return ABI before resolving a compiler
   symbol; a symbol rename alone contributes no reconstructed code bytes.
   Derive display-coordinate offsets from the native drawing code; physical
   screen dimensions do not establish the engine's coordinate convention.
3. Preserve load/store order, short-circuit calls, possible aliasing and accesses
   across callbacks. `const` does not establish non-overlap. Keep native masked
   and unmasked stores, packed-field truncation and neighboring bits. Do not
   simplify apparently redundant operations without explaining the instructions.
   For queue consumers, distinguish a cached tail from live queue state and
   preserve when the head advances relative to callbacks. Read linked-list
   successors at the native boundary: callbacks may change the next node.
   Derive each callback's arguments from its own ABI; a transfer-record callback
   and a no-argument node callback are different interfaces.
4. Use existing shared records and workspace types; retain compatible raw views.
   Add size/offset checks and neutral names for unknowns. A prefix's `sizeof`
   does not prove the allocation size. Search all declarations before changing
   shared layouts or signatures, then migrate and rebuild affected callers together.
   Distinguish allocation size from the constructor's cleared extent. Include
   trailing padding in memory checks without assuming it was initialized to zero.
   Preserve verified interior data aliases and native table strides. Task payloads
   can reuse one offset for different phases; model that reuse explicitly and
   check every transition's initialization before giving the field one meaning.
   A child and its parent can also interpret the same pointer offset differently.
   Give owner, panel and child roles explicit views or a named union; verify the
   creator's links and each live allocation instead of applying one layout to
   every object in the chain.
   A common returned handle or allocation size does not establish a common record
   layout. Battle sprite and model effects occupy 48 and 56 bytes respectively;
   a matrix-animation track also occupies 56 bytes but has its own context and
   owner fields. Recover the actual factory and release contracts before choosing
   a shared type or interpreting fields beyond the handle.
   Likewise, the common battle-model interface can refer to a 440-byte primary
   model or a 304-byte alternate renderer. Use the allocation's actual extent in
   probes; reading `sizeof(BattleModel)` through every returned handle overreads
   the smaller object. See the [animation evidence](docs/research/RECONSTRUCTION_NOTES.md#battle-model-animation-starts).
   For a view based at an indexed interior address, check alignment and prove
   `view_offset + sizeof(view) <= allocation_size` for every valid index.
   Preserve whether native addressing applies the stride before the fixed field
   offset; a bounded prefix view can express that order using existing field types.
5. Keep related contiguous functions in a subsystem module. Temporary isolated
   units are acceptable around native gaps; consolidate when those gaps close.
   DSD rejects repeated section names within one unit. Keep disjoint native
   `.text` ranges in separate units while the intervening gap remains; do not
   add duplicate `.text` entries or linker workarounds to force them together.
   Basenames must be globally unique because MW's linker selects by basename.
   This compiler generally emits functions in reverse source order; verify it.
   Keep C declarations at block starts without hoisting assignments across native
   sequence points. Guard C APIs with `extern "C"` in C++; a header without its
   own linkage guard must first be included inside that block.
6. Compare size, every instruction, pools, relocations and symbol layout.
   Classify the difference before editing. Keep compiler flags and ABI fixed;
   do not weaken checks or introduce arbitrary casts/volatile accesses to match.
   A scoped volatile access requires evidence of that native access pattern.
   If only register assignments differ, record the mapping and defer the target
   unless native dataflow or a demonstrated compiler rule suggests a correction.
   Recompile drafts against current headers and associate each comparison with
   the actual source, language mode and object. Address-named dumps may be overwritten.
   Even one inline-ASM instruction can change optimization elsewhere in the
   function; putting it in an inline helper does not isolate that effect.
   Preserve the C-only draft, compare the complete result and defer the experiment
   if surrounding code regresses without an evidence-based explanation.
7. Inspect comparison text as well as exit status: some private checkers return
   success while reporting `DIFFERENT`. Equal sizes do not establish a match.
   Require the expected function identities, nonzero count and total code bytes;
   stale names can also produce an empty comparison with a successful exit status.
   Unknown relocations, missing candidate sections or unresolved functions must
   fail; never substitute original bytes for a missing candidate. Validate local
   data contents and ownership before mapping its symbol to a native address.
   A checker may resolve an address-named helper that has since been renamed.
   Use its current public declaration; address resolution does not prove the
   public linker can resolve that obsolete name.
   Record mixed private units per function. Exact neighbors do not validate a
   differing function in the same object; keep an exact-only source/object pair
   before extending a successful draft with another candidate.
8. Integrate exact functions into the manifest and component metadata. Update
   declarations and maintained ASM references together. Preserve interior entry
   points through `config/eur/arm9/linker_aliases.json`; check
   `reasm/eur/patches.json` when renaming symbols. Include compiler-emitted local
   data with its actual binding and ownership, but do not count it as code.
9. Complete the build and relevant runtime checks, update progress/evidence/map,
   then publish the coherent batch. A matching private neighbor is not linked
   progress, and an already linked helper contributes no new bytes when moved.
   Compare the actual build object as well as the isolated candidate. When
   extending a contiguous unit, recheck every function in that unit after the
   final shared-header change; an earlier comparison covers its earlier inputs.
   Inspect the compiled source object, such as
   `build/eur/src/battle/battle_script_state.o`, rather than `build/eur/delinks/...`,
   which contains original reference objects.

See the reference's compiler examples before attempting syntax or declaration
changes. Separate switch arms, initialization order, bitfields and pointer
increments can explain a specific mismatch; they are not a search space to enumerate.

## VM maintenance

The Field, Battle, Common Battle and Scene dispatchers are already linked and
byte-identical. Check their current source and manifest entries before treating
old VM requests or private near-matches as unfinished work. Dispatcher completion
does not establish that all its callees are reconstructed or that every script
path has runtime coverage.

Resolve script JSON opcodes through the schema and
`tools/report_battle_vm.py::opcode_number`; exports can contain names instead of
numbers. Follow the packed resource ID through the archive-slot resolver to the
actual script entry before choosing a save or an attack to exercise a handler.

The resident interpreter serves different overlay-specific descriptor tables,
variable namespaces and script containers. Consult the
[VM reference](docs/research/SCRIPT_VM_SEMANTICS.md) for the actual instance;
do not transfer an opcode's argument contract from one VM to another. Preserve
instruction-pointer advancement, operand decoding, yield/rewind behavior and
the order of variable reads and writes when changing handlers.

For packed battle operands, derive the expression from native instructions and
literal masks. The existing `BattleVm_PackHalfwords` combines
`((u32)low & 0xFFFF) | ((u32)high << 16)`; misleading pseudocode can describe a
64-bit shift instead. A following signed division by 16 truncates toward zero
and is not interchangeable with an arithmetic right shift. Preserve mode-bit
tests, in-place argument writes and their order relative to object lookup.
Check full-word versus halfword comparisons separately, even in related handlers.

Check each state's storage extent: [BattleAIState](include/game/battle_ai.h)
is `0xC0` bytes including its continuation tail, while embedded party-state slots
have a `0xB8` stride. Neither establishes the size of every VM buffer. Bound
reads and writes by the actual containing allocation and the fields used there.

The Scene dispatcher's opcode `0x04E` uses a documented 16-instruction inline-ASM
height calculation. The remaining C-only discrepancy was a demonstrated compiler
conversion schedule. Read the [compiler evidence](docs/research/SCENE_VM_MATCHING.md)
before revisiting it. If changing that block, run
`python tools/verify_scene_alignment.py --negative-control` in addition to the
normal matching checks. Its symbolic proof covers the modeled block; it is not
a proof of the whole VM or of source-language portability.

## Build and verification

Use Python 3.11+, Ninja and compatible Metrowerks tools in
`tools/mwccarm/1.2/base/` (internally 2.0 build 72). Native relinking also uses
LLVM's `llvm-mc`, `ld.lld` and `llvm-objcopy`. Take the complete compiler flags
from `tools/configure.py`, including `-O4,p`, ARM946E and soft float; private
comparisons must use the same language mode and ABI. Known workstation executables:

```text
C:\Users\Julian\AppData\Local\Programs\Python\Python312\python.exe
C:\Program Files\JetBrains\CLion 2023.2.2\bin\ninja\win\x64\ninja.exe
C:\Users\Julian\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe
```

If missing from `PATH`, verify the path and invoke it with PowerShell's `&`.
The private EUR ROM is `extract/baserom_PiT_eur.nds`, game code `ARMP`, SHA-1
`ba4ec2f99b4f2e0047601552bccf00aa73e28701`.

For a completed reconstruction batch, from the repository root:

```powershell
python tools/configure.py eur
ninja objects
ninja check
pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\build_nds.ps1 -DisableDataMods
Get-FileHash -Algorithm SHA1 .\PiT_eur.nds
python tools/relink_native.py --version eur --rom extract/baserom_PiT_eur.nds --output-rom build/PiT_eur_native_relinked.nds --require-matching
python tools/generate_progress.py
python tools/generate_progress.py --check
python -m pytest -q tests
```

Run dependent commands sequentially and stop on a nonzero exit status. PowerShell
can continue after a failed native command; a later success does not validate an
earlier integration or build. Check `$LASTEXITCODE` or use a subprocess runner
that raises on failure, and retain a separate log for each command.

Every C/C++ or shared-header change needs the full `ninja check`. The packaging
wrapper restores checksum fields omitted by direct packaging; verify the final
ROM hash. Keep `-DisableDataMods` to exclude private data modifications. Native
relinking must report zero differing bytes. Regenerate progress before tests
when linked ranges change; use the explicit `tests` directory to avoid private
clones and incomplete unittest-only discovery.

`ninja objects` also compiles unlinked drafts. `ninja check` alone builds only
the sources selected for the exact ROM, so it can miss a broken draft. The
refactoring gate runs both; compiling a draft does not make it matching or linked.

On this workstation the wrapper has passed under the bundled PowerShell above.
An earlier Windows PowerShell subprocess failed to resolve its final
`Get-FileHash`; check the shell and command availability when diagnosing that
failure, and retain the failed log separately from a successful rerun.

Do not replace a ROM while an emulator reads that path. A native relink to a
separate output can run beside a replay. Documentation-only work needs content,
link and Git checks; tool changes need relevant tests. Repeat checks when changes,
failures or unresolved concerns warrant it. Report unavailable/skipped checks.

## Runtime verification

Use `tools/runtime_drive.py` for deterministic inputs, `runtime_probe.py` for
hooks/registers/memory/overlays, and `runtime_session.py` for navigation and
compatible snapshots. Read their arguments and
[runtime guidance](docs/research/RECONSTRUCTION_NOTES.md#runtime-verification).

- Enumerate `PiT_SaveStates/`; older messages call it `PiT/_SaveStates`. `.sav`
  files are battery saves, not emulator snapshots. Protect originals and compare
  their hashes before/after. Keep new artifacts under ignored `build/runtime/`.
- Optional `py-desmume==0.0.9` embeds DeSmuME 0.9.12. Supplied 0.9.13 snapshots
  can show plausible RAM but hang or render incorrectly. Cold boot battery saves
  to create compatible states; do not repeatedly advance a known-bad snapshot.
- Reuse tested routes with focused checks for the current functions and required
  helpers. The driver inserts one released frame after every action. Bound entry,
  assert that the target dispatch ran, and confirm the final scene from live state.
  Inspect creation predicates before choosing a route: adjacent menu categories
  can create different callbacks. Derive child counts from the creator's loops
  and reconcile them with distinct task lifetimes, not just callback totals.
  A missed route is a coverage gap; do not remove its assertion to obtain a pass.
  Keep call discovery separate from output verification. A completed replay with
  zero target calls provides no coverage for those functions. Use actual callers,
  script opcodes and save conditions to select another route before repeating it;
  a neighboring native attack helper does not prove that a VM wrapper executes.
  A save number in a checkpoint filename does not identify its current encounter;
  inspect the producer, earlier fixtures and live enemy/script state.
  Check the save's actual entry conditions first. For example, a fully healed
  party can prevent a healing item's recipient-selection path. Use another
  suitable save or a guarded, reversible fixture and label that evidence accordingly.
  Establish required branch coverage before running. Inherited visibility or
  draw-count assumptions can depend on the save and menu; justify corrections
  from the native predicate and live inputs, retain per-call checks and reconcile
  every hidden/drawn outcome. A correctly hidden object adds no drawing coverage.
- Guard the complete native function range and owning overlay. Addresses are
  reused after transitions; positively identify a foreign owner before excluding
  a hit. The runtime tool's 90% overlay-identification threshold is only a locator,
  not an exact guard. Unexplained byte mismatches remain failures.
  Overlay ownership applies to data too. Check a cleared scene-global pointer
  at the guarded destruction boundary while that overlay owns the address.
  After a transition, establish the new owner before interpreting that address;
  verify the destination scene using its own live state and visible readiness.
- Execution hooks fire before the addressed instruction. Establish memory-write
  hook timing separately: a value read in that callback or labeled `before` in
  a diagnostic report is not necessarily the pre-store value. Respect ARM
  condition codes and distinguish helper effects from subsequent caller stores.
  In the verified DeSmuME setter replay, false conditional stores delivered no
  execution callback. Require the selected stores and independently check that
  skipped destinations remain unchanged; do not require a hook at every address.
  Pair nested returns using entry SP and LR; tail calls can share both, so finish
  all matching pending records innermost first. Read ARM9 DTCM at
  `0x027E0000..0x027E4000` directly;
  do not fold stack outputs into main-RAM mirrors.
  Check initialized stack locals after the prologue and initialization stores,
  at a guarded boundary such as the first helper entry. Derive offsets from the
  saved entry SP and native frame layout. Verify output-pointer writes at the
  helper boundary and stop inspecting locals when their stack frame ends.
- Check whether changing a callback invokes it synchronously. The EUR overlay-5
  setter at `0x02066358` installs the callback and resets task phase to zero;
  its third argument means invoke immediately. A nonzero value can run the new
  callback before the setter returns. Model that nested call and its writes;
  do not treat the argument as a phase or assume one update per frame.
- Derive expected changes independently, including native integer wrapping and
  signed division. A fresh snapshot after a helper observes its effects; it does
  not verify that helper. Check full live allocations, padding and overlapping
  views, plus separate globals touched by the call. Constructor arguments can be
  valid before a scene global is published. Decode each entry point's actual ABI:
  a creator's first argument may be a mode integer, not a task pointer. Track the
  allocated task at its factory boundary. An exact ROM replay confirms executed
  behavior; also review the C types and bounds, which matching bytes alone cannot
  establish as valid.
  For floating-point arithmetic, preserve intermediate conversions and truncation.
  An integer oracle is valid only after proving that every modeled operation is
  exact over the input domain; powers-of-two constants alone do not prove this.
- Track allocation, initialization, updates and actual release separately. Derive
  pool slots and list writes from the allocator; do not assume payloads are zeroed.
  Removal flags, callback retargeting and pool returns are different events.
  Track attached resources and bulk-pool destruction too. Check records before
  release, then only still-live pool/list state. Address reuse starts a new lifetime.
  Virtual cleanup can release a model's internal allocations before its pool slot
  is returned. Check those boundaries separately. ResourceA and ResourceB use
  different pools and slot sizes; verify each against the current shared source
  and memory reference before extending an inherited probe.
  Some live objects are embedded array slots, with no individual heap header.
  Derive their base, count and stride from the initializer and check current
  slot ownership. Battle scene objects use 70 embedded 260-byte records; see
  the [effect replay](docs/research/RECONSTRUCTION_NOTES.md#battle-relative-effect-spawning).
- Distinguish initialization, base destruction and deleting destruction. A
  destructor can unlink a task while leaving its allocation intact; a deleting
  variant additionally returns the block to the heap. Check virtual slots,
  returned pointers, task-list neighbors and allocator metadata at each boundary.
  A custom allocator may retain payload bytes after release. Preserve any native
  reads at that boundary, but retire the allocation from subsequent live-object
  checks. Confirm the supported list topology before generalizing a cleanup loop.
  Archive destruction can close an open file: inspect its actual flags and model
  the file, archive queue and result, rather than assuming a closed handle or
  accepting all helper writes through a fresh snapshot.
- Derive timer behavior from its actual entry guard and recurrence. An initial
  zero may trigger now, persist indefinitely or have another meaning; integer
  truncation can add updates. Check the relevant paths instead of importing a
  neighboring callback's timing assumption. Count callback invocations separately
  from emulator frames, and preserve phase fallthrough within a single call.
  Reaching exactly zero and crossing below zero can take different paths: a clamp
  may dispatch immediately while an exact-zero result waits for the next call.
  For cached display updates, distinguish unchanged returns, redraws and removal
  paths. One redraw can call several numeric helpers. Derive totals from report
  counters and reconcile each partition before quoting them in documentation.
- Verify RAM, mapped VRAM, palettes, OAM, ordered GPU stores and visible behavior
  as appropriate. Allocation, initialized extent and transfer size can differ.
  Hardware/FIFO readback is not the submitted sequence. Keep changing scanline
  and IRQ-driven registers in captures without asserting that they are immutable.
  Derive display engine, BG layer and tilemap buffer separately; a buffer index
  does not identify the main or sub screen. Verify both writes in a register
  clear-then-set operation, even when the final register value would be identical.
  For text, distinguish the saved item ID, name ID and description ID, and the
  graphics allocation's offset, reserved size and actual transfer extent. Check
  live lookup records against the native table and preserve its entry stride.
  Decode each hooked store's effective address, including shifted register
  indices, and reject unsupported forms. Check write-only BG scroll registers
  through the ordered stores rather than expecting readable register values.
- Keep verification proportional to what the target and its callees can touch.
  For a pure data callback, check full relevant live task/game records at each
  call; avoid copying all graphics memory on every invocation without a reason.
  Retain surrounding controller graphics checks and route captures where needed.
  Document the checked ranges and any sampling. Speed up host-side inspection
  only when the same expected results and assertions are preserved.
  If a renderer or animation helper remains observational, accept its writes
  only within explicitly bounded ranges at its return. Continue independently
  checking the caller's decisions, arguments, stores and surrounding memory;
  do not replace the entire expected state with a fresh snapshot.
  Include linked-list neighbors and roots outside the receiving object. An
  animation setter can insert an embedded palette record into a shared list;
  derive those pointer changes even when the placement choice is observational.
- Record per-function/branch counts, ROM/save/state hashes, inputs and explicit
  limits. Separate ordinary routes from RAM fixtures and document restoration.
  For a per-call fixture, preserve the exact bytes, edit only at a guarded live
  boundary, verify the expected outputs, then restore at the guarded return
  boundary before the caller resumes. Provide cleanup on failure. For a fixture
  spanning navigation, specify and verify its later restoration boundary too.
  Restoring the input alone may leave induced state behind, such as a cleared
  readiness flag. Define which induced outputs must also be restored, check them
  before restoration, and record the restored bytes separately from natural writes.
  For random-dependent fixtures, derive both the fixture's expected RNG steps
  and the ordinary call's result/state. Restore the latter before the caller
  resumes when continuation must follow the ordinary route, including return
  registers it consumes. Verify skipped RNG calls and strict threshold equality.
  A temporary removal-flag fixture proves the marking branch; a later ordinary
  group cleanup is separate evidence of release. Do not attribute it to a flag
  that was already restored. Test zero, below, equal and above a threshold where
  relevant; preserve native operand widths and signedness instead of rounding
  percentages.
  Keep generic helper checks and checks performed inside watched creators
  distinguishable in reports, with no double counting of the same call.
  A route reaching every visible row can still miss a setup flag's other branch;
  inspect the actual branch coverage before claiming a whole function is covered.
  For deterministic sampling, report observed versus fully checked calls and
  the rule; track lifetimes independently. Screenshots alone do not prove execution.
- Validate referenced artifacts: existence, hashes, image dimensions, memory
  extents and report totals. Compare baselines only for identical state, fixture,
  input and capture prefixes; stop at the earliest differing fixture, including
  a callback edit that occurs before a later navigation fixture.
  Distinguish visual inspection from hash equality and observed rasterization
  from an independent graphics oracle.
  A verified draw-list insertion does not prove a visible sprite: clipping can
  suppress it. State whether the evidence checks submission or rendered pixels.
  If repeated captures differ, preserve the mismatch and compare target-entry
  RAM, registers, checked events and graphics separately. Repeat an unchanged
  probe when needed to distinguish a probe change from existing variation.
  Equal constructor inputs or OAM do not prove identical later animation pixels;
  leave the cause unconfirmed unless the evidence explains it. See the
  [scheduler lifecycle replay](docs/research/RECONSTRUCTION_NOTES.md#battle-scheduler-lifecycle).
  Associate every route with the exact probe source/version that produced it,
  including generated or composed copies. Preserve separate variants when the
  probe changes between routes; the current script cannot stand in for all of them.
  Identify reports by a unique run tag; one route can have different saves or
  fixtures. Derive the common input prefix explicitly when comparing different
  routes, rather than assuming their full action lists match.
- Confirm visible scene readiness as well as overlay ownership. Field code can
  already be loaded while a fade still renders black. Use a bounded neutral-frame
  extension and inspect the final capture before claiming a visible return.
  A focused replay used only to finish that transition must reproduce the prior
  inputs, fixtures and capture hashes. Keep its report separate from the original
  callback oracle; observing the extension does not repeat those independent checks.
- At replay end, stop admitting new outer calls and drain pending/nested calls
  for bounded neutral frames. Keep failures and rerun corrected oracles; do not
  discard pending calls or change matching game code to satisfy a faulty model.
  Check whether the active probe imports its editable body or a composed copy.
  When composing probes, use a unique name for the previous callback wrapper;
  reusing an inherited global name can make it call itself recursively. Assert
  replacement counts and name uniqueness, inspect the generated source, and
  preserve the failed version before correcting and rerunning it.

An optional private fast reader moves the existing DeSmuME byte-read loop into
host C without changing accesses. Before reuse, validate it against the original
reader on paused RAM, DTCM and graphics ranges; see
[transition evidence](docs/research/RECONSTRUCTION_NOTES.md#pause-transition-panels-and-controllers).
Host probe code does not contribute to matching-game coverage.

### Isolated ARM boundary checks

Copied live RAM and DTCM can support focused native execution for branches that
an ordinary route misses. The scheduler reference documents this with Unicorn
configured as ARM946. Pin the CPU model, emulator version, memory/register inputs
and full native bytes, including executed helpers. Identify synthetic records,
modeled I/O and any stubs explicitly; keep those edits inside the copied state.

Derive results independently and check the relevant memory, ordered stores,
return values, stack restoration and callee-saved registers. Distinguish complete
range checks from sampled ranges and observational helper effects. Report these
cases separately from DeSmuME navigation: isolated calls do not establish live
object lifetimes, asynchronous IRQ behavior or visible gameplay. Use them to
supplement the focused live route, and state which branches each method covers.
Fixture preconditions must follow the native helper too. An allocation-failure
case needs a valid exhausted heap, rather than a null list head that the allocator
dereferences. A copied closed-file case covers that branch only; retain separate
evidence for an open file and any hardware-dependent close path.

Make branch fixtures distinguish the outcomes they claim to test. Shorter/longer
duration selection needs distinct valid roots; equal roots cannot expose a wrong
choice. Explicitly bound any emulated hardware contract: an immediate integer
square-root result does not verify DS hardware latency, busy flags or IRQ timing.

### Debug menu and Nawatobi

The debug menu teleports without necessarily initializing a complete game state.
Use the reference's tested routes for shops, save menus, Game Over, Smash Eggs,
credits and pause. Controlled access does not establish a normal story entry.
For RAM instructions, specify region, CPU/address space, dereferences, offset,
width, guard, timing and restoration; never give an unexplained dynamic address.

The tested EUR Nawatobi edit is the little-endian 32-bit phase word at
`read32(0x0208E1E0) + 0x30` in ARM9 main RAM, with overlay 7 loaded. At pause
update `0x02071F80`, require the full native-byte guard, a fully initialized pause
menu, `read32(0x0208E1E0) == r0`, vtable `read32(r0) == 0x0208D9B8` and phase 2.
Write 7 once, disable the hook and resume. The field becomes `07 00 00 00`;
do not overwrite the pointer at `0x0208E1E0` or freeze the phase. This is not
VRAM or a ROM patch. See the [procedure and coverage limits](docs/research/RECONSTRUCTION_NOTES.md#nawatobi).
The Nawatobi scene's own phase is a separate word at
`read32(0x020A6BBC) + 0x2C`; it is not the tested pause-entry edit above.

## Documentation, Git and handoff

Keep the README short and human-readable, including the user's AI disclosure
and tone. Update the overlay map when linked coverage or confirmed roles change.
Avoid vague similarity claims, inflated completion language and speculative names.
Keep detailed findings in the reference and changing candidate status in the
private handoff, rather than continually extending this guide.

Track these stages separately:

| Stage | Evidence |
|---|---|
| Exact candidate | Identified source/object; full native comparison including pools and resolved relocations |
| Integrated | Current public source, headers, manifest and ownership pass build checks |
| Runtime checked | Completed route reports, function/branch counts, lifetimes, validated artifacts and stated limits |
| Published | Task-owned commit, successful push and confirmed remote revision |

Use `pending`, `failed` or `not run` where appropriate. The progress generator
reads metadata; it does not perform matching or runtime checks. If reports are
dirty, use `git show HEAD:docs/progress.json` for committed coverage. Confirm the
live remote before calling bytes published. Count only new code ranges; ARM7,
data and symbolic ASM follow the separate categories in the metric. Documentation
commits must not include pending source or claim unverified progress.
Read changing totals and milestone targets from current reports and the latest
user request; do not hardcode a moving percentage or candidate queue into this
guide. A byte-exact ROM rebuild and a successful route do not establish complete
C/C++ reconstruction or coverage of every branch.

`docs/BATTLE_MATCHING_MILESTONES.md` has a legacy non-UTF-8 byte and mixed line
endings. Append bytes while preserving the existing prefix. Compare the working
file's prefix with a raw pre-edit copy, and compare the staged prefix separately
with the previous `HEAD` blob; Git's line-ending conversion can make those two
baselines differ. Do not decode with replacement and rewrite it.
The similarly named file under `docs/research/` is an older, different log.

Keep ROMs, extracted assets, saves, proprietary tools, IDA databases, machine-code
dumps and captures private. Never force-add ignored material. Before publication:

1. Review `git diff --check`; stage explicit task-owned paths only.
2. Review staged paths, `git diff --cached` and `git diff --cached --check`.
3. Run `python tools/check_public_content.py` **after staging**; it audits the index.
4. Commit descriptively, push to the verified user remote and confirm its revision.
   Report the commit, push status, checks and any actual coverage change.

For documentation work with an existing staged reconstruction batch, preserve
its file hashes and index entries. Commit only the reviewed documentation paths
(for example, `git commit --only -- AGENTS.md`), then verify that the earlier
staged entries remain unchanged. Check documentation links against the tree being
published as well as the working tree; an uncommitted anchor can hide a broken link.

At a stop, update private `build/analysis/CURRENT_HANDOFF.md` with the latest
request, pushed revision, last verified code batch, pending paths and checks.
For every gap, record component/address, native/candidate sizes, mismatch class
and evidence needed to retry. Record exactness per function, including mixed
private units; preserve unfinished drafts during documentation work.
Archive an obsolete handoff before replacing it. Its new opening must identify
the latest request, distinguish committed from working-tree coverage, and state
the next missing check for each pending batch. Historical continuation language
must not override the current task or turn a planned replay into completed work.

Keep failed replay commands, exit status, logs, frame/assertion and successful
reruns distinct. A suspected oracle correction is not a verified fix. Record
any live process's session ID, command and output path; inspect it before starting
a duplicate. Populated artifacts or elapsed time do not establish successful
completion. Historical build logs are not checks run by the current task.
Check the producer's final exit status and assertions even when its JSON says
`PASS`; some private probes write that report before their final aggregate checks.
