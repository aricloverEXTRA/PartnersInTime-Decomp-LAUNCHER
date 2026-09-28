# Decompilation source policy

The project aims for readable, editable C that also reproduces the verified
European ROM exactly. Neither requirement is optional.

## Source organization

- Keep related functions from one contiguous original code region in one
  subsystem module. Do not create a permanent source file for every function.
- A small temporary file is acceptable while surrounding code is still a raw
  or symbolic assembly gap. Merge it into the subsystem module when the gap is
  translated; do not introduce linker tricks solely to make unrelated address
  ranges appear to be one compilation unit.
- Arrange function definitions so that the linked object preserves the original
  runtime order. MWCC 1.2 emits the separate function sections used here in
  reverse declaration order, so multi-function source units generally list
  their functions in descending runtime-address order. Headers own shared
  structures, enums, flags, and public declarations.

### Directory layout

`src/game/` holds resident game helpers, `src/nitro/` and `src/msl/` mirror the
Nintendo SDK and Metrowerks library module layouts, and `src/field/` and
`src/battle/` are overlays 0 and 2. Every other overlay has a directory named
`<role>_ovNNN`, for example `shop_ov009` or `attack_pocket_chomp_ov018`: the
role states what the code does and the padded overlay number keeps it tied to
the delink metadata and the [overlay map](research/OVERLAY_MAP.md). Headers
private to one overlay live beside its sources and are included with a
relative path; shared interfaces belong in `include/game/`.

### Renaming and merging units

Use `tools/reorganize_sources.py` with a plan file rather than moving units by
hand. It refuses anything that cannot be a valid translation unit - members
from different components, a gap between their ranges in any section, members
out of address order, mixed languages, a colliding basename - and rewrites
`delinks.txt`, `linked_sources.txt` and `linker_aliases.json` together. Two
units may only be merged when their native ranges already touch, and merging
unrelated topics because their addresses happen to be adjacent is not an
improvement. The plan must use independent operations and keep linked units
separate from unlinked drafts. The tool checks object basenames across both
languages and refuses to apply a plan over dirty source or metadata files;
preserve or commit that work first. Run `tools/verify_refactor.ps1` afterwards.

### Language per unit

Whether a reconstructed unit is `.c` or `.cpp` affects its ABI and object model:
shared headers such as `include/game/task.h` and
`include/game/field_entity.h` declare virtual classes under `__cplusplus` and
plain structs with a vtable pointer otherwise, so the language changes what the
code means. Change a unit's language only with evidence - virtual dispatch
using a single scratch register, a constructor installing a vtable, an entry in
`.ctor` - and only when the rebuilt object still reproduces the original bytes.
Matching C++ output supports that reconstruction; it does not prove the original
source language or translation-unit boundaries. A mismatch after changing
language can also come from declarations, layout, optimization or source shape.
Record that evidence and retain the matching version until the cause is known.

## Readability and matching

- Prefer structured `if`, `switch`, and loop constructs. Use `goto` only when a
  structured equivalent cannot reproduce the original code, and document the
  matching constraint next to it.
- Replace confirmed numeric values with domain names. Keep unknown fields and
  flags explicitly named `unknown` rather than assigning speculative meaning.
- Add compile-time size checks to every sufficiently reconstructed structure.
- Treat decompiler output as a starting hypothesis, not source code. Simplify
  casts, temporary variables, and control flow, then run the matching check.
- Every C change must pass `ninja check`; milestone changes must also produce a
  native relink with zero differing bytes and pass the unit tests.

## Comments

Comments carry the knowledge that the code cannot: what a record is for, who
owns it, which convention a number follows, and why the code is shaped the way
it is. Use `/* */` consistently in reconstructed sources and headers.

- Open each module with a block comment naming its role, its component and its
  native range, as in `src/battle/battle_hit.c`. The range is checked against
  the delinks entry by `tests/test_module_comments.py`, so keep it accurate
  when a unit moves.
- Document a structure where it is declared, not at each use. Say what the
  record is, what owns it and what its lifetime is; call out units (Q8, Q12,
  frames, scanlines) and any field whose meaning depends on the caller's phase.
- Explain a union's arms. Several records here read the same bytes differently
  depending on which subsystem is looking, and that is invisible from the
  declaration alone.
- Where the code is shaped by the match rather than by the problem - a retained
  redundant test, a preserved evaluation order, an unsplit switch - say so next
  to it, so the next reader does not "fix" it.
- Do not describe what a statement does when the statement already says it, and
  do not invent meaning for a field that is still named `unknown`.

## Interior entry labels

Some shipped calls target labels inside another function. If that function is
compiled from C/C++, preserve those addresses in `config/eur/arm9/linker_aliases.json`.
`tools/apply_linker_aliases.py` derives each offset from the original symbol map
and adds a function-relative linker assignment after its source object. These
aliases emit no bytes. They must identify aligned interior labels of the named
owner; the normal module and symbol checks still verify the complete output.
Do not reinterpret such a target as a different routine without native evidence.

The same manifest can give a source a `constants` map of symbol names to
32-bit hexadecimal values. This preserves arithmetic on link-time arena bounds
and stack reservations without allocating data or folding it prematurely in C.
Recover each value from the target binary, declare it as an external symbol,
and use its address as the value; never dereference it. Heap initialization is
the first user. These values are specific to the configured game version.

## Evidence levels

Names and comments should distinguish three levels of knowledge:

1. **Confirmed:** established by data flow, multiple callers, data formats, or
   a runtime observation. This may use a gameplay-facing name.
2. **Inferred:** strongly suggested but not yet observed. Record the evidence
   in the relevant research map and avoid over-specific public names.
3. **Unknown:** retain an offset, bit number, or neutral role name and add it to
   the runtime-probe backlog when its meaning affects modding.

## Runtime evidence workflow

Runtime probes should answer one narrow question at a time. Record the ROM
hash, battle/enemy, save state, breakpoint, inputs, before/after memory, and the
observed on-screen event. Useful first probes are:

| Question | Breakpoint or data | Observation |
|---|---|---|
| Which party actor ID maps to each character form? | `BattleActor_GetById` at `0x02076F64` | ID, actor pointer, scene object, visible character |
| What do active-model bits 10 and 11 control? | `0x020A3310` and `0x020A3348` | Caller, requested value, model flags, visible render change |
| Which hit kinds map to damage/reaction behavior? | `BattleHitDescriptor_Configure` at `0x0209E10C` | source, target, low six flag bits, resulting animation/effect |
| How are final damage modifiers applied? | `BattleDamage_CalculateBase` at `0x0209BF38` and `BattleDamage_DispatchHit` at `0x0209DE8C` | registers, stats, intermediate and final damage |
| What does save field `+0x558` represent? | `BattleActor_CanReceiveStatus` at `0x02076BD4` | field value, party composition, baby-status eligibility |

Overlay 2 is loaded at its fixed runtime addresses during battle.
`gBattleContext` is the pointer stored at `0x020C0718`; actor HP is the signed
halfword at actor offset `+0x06`. Begin with read-only breakpoints and memory
logging. Only after a hypothesis is reproducible should it change a public C
name or structure field.

For scripted emulator sessions, [`py-desmume`](https://pypi.org/project/py-desmume/)
is the preferred first automation candidate. Keep it out of the mandatory build
dependencies; introduce a pinned optional probe environment only when the first
repeatable RAM experiment is implemented.
