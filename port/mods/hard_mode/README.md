# Hard Mode

A difficulty profile for Mario & Luigi: Partners in Time (European version).

Hard Mode is a **data** mod. It changes enemy statistics only, so it does not
require the decompilation to be complete and does not modify any reconstructed
game code.

## Tuning

The balance target is a **shorter, denser loop**, not a longer grind: fights are
meaningfully more dangerous, but you level faster and clear them in less total
time.

| Stat | Multiplier | Field type |
|---|---|---|
| Enemy HP | x1.10 | `u16` at `0x06` |
| Enemy POW | x2.00 | `u16` at `0x08` |
| Enemy DEF | x1.05 | `u16` at `0x0A` |
| Enemy SPD | x1.25 | `u16` at `0x0C` |
| Coin rewards | x1.75 | `u16` at `0x22` |
| Experience | x1.75 | `u16` at `0x20` |

All results are clamped to the `u16` range. Nothing is clamped in practice: the
largest retail values become `max_hp` 10999, `defense` 1049, `coins` 700,
`experience` 2625 and `power` 460.

### Why these numbers

The reconstructed damage formula is the whole basis for the tuning. From
`PiT/src/battle/battle_damage_calculation.c:201`:

```c
damage = (attacker_level * attacker_power * scale_q8 / 256) / defender_defense;
```

**Defense is a divisor, not a subtrahend.** There is no `attack - defense` term
and no floor, so:

- turns to kill scale with `max_hp * defense` **multiplied together**
- damage dealt is inversely proportional to defense, and can be floored to 1

This is why the earlier draft felt like a slog rather than a difficulty bump.
HP x1.5 and DEF x1.5 together produced **x2.25 turns per battle**, not x1.5,
and experience was disabled - so every battle took more than twice as long for
exactly the same reward.

Experience is a flat per-enemy award (`battle_enemy_defeat.c:175` just does
`total = earned + stats->experience`) with no dependence on HP, turns or damage
taken. That makes it the one lever that actually shortens progression, which is
why it carries the largest boost.

### Derived effect

| Measure                    | Earlier draft | This tuning |
|---------------------------|---------------|-------------|
| Turns per battle           | x2.25         | **x1.16**   |
| Damage you deal            | x0.67         | x0.95       |
| Damage taken per battle    | x5.62         | **x2.31**   |
| Battles needed per level   | x1.00         | **x0.57**   |
| EXP per turn of combat     | x0.44         | **x1.52**   |
| Coins per turn of combat   | x0.67         | **x1.52**   |
| **Total turns per level**  | x2.25         | **x0.66**   |

Net: about 131% more danger per battle, 34% less time per level, and the reward
per unit of effort is 3.5x better than the earlier draft.


## No game data is shipped

`profile.json` contains multipliers and nothing else. It holds no bytes, names,
stats or other data derived from the retail ROM.

The tool combines the profile with **your own** exported data project, so the
generated project is derived from your ROM. Generated output is written under
`/generated/`, which is git-ignored and must never be committed.

Before distributing this mod, fill in `authors` in `profile.json`; the declared
`CC-BY-4.0` license requires attribution.

## Usage

Export the editable project from your own matching EUR ROM once:

```powershell
cd ..\PiT
python .\tools\data_mod.py export `
  --version eur `
  --files-root .\extract\eur\files `
  --project-root .\data\eur
```

Generate the Hard Mode data project:

```powershell
cd ..\PartnersInTime-Port
python tools\make_data_profile.py `
  --base-project ..\PiT\data\eur `
  --profile mods\hard_mode\profile.json `
  --output generated\hard_mode
```

Build the ROM with the profile applied:

```powershell
cd ..\PiT
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\build_nds.ps1 `
  -DataProject ..\PartnersInTime-Port\generated\hard_mode
```

The output `PiT_eur.nds` differs from your base ROM only in
`BData/BDataMon.dat`. Re-running the build without `-DataProject` restores the
vanilla table.

## Verification performed

- Generation over all 98 retail enemy records, with no field reaching the `u16`
  clamp.
- ROM rebuild with the profile applied succeeded; ROM size unchanged at
  67,108,864 bytes. Hard Mode ROM SHA-1
  `2430b97852414d8f500c6fc0f2172ba5bbc3ccb8`.
- The data-mod report shows exactly one changed file, `BData/BDataMon.dat`,
  with an unchanged size of 4,312 bytes (98 records x 44 bytes).
- End-to-end byte diff of `BData/BDataMon.dat` between the base ROM and the
  modded ROM, decoded through the port's own offset-archive reader: 597 of 4,312
  bytes differ, confined to the six intended `u16` fields at `0x06`, `0x08`,
  `0x0A`, `0x0C`, `0x20` and `0x22`. `name_id`, `flags_or_ai_id`, `traits`, the
  14 unknown bytes and both item-drop fields are untouched.
- Spot-checked rounding, e.g. record 1: HP 13 -> 14 (x1.10), POW 16 -> 32
  (x2.00), DEF 20 -> 21 (x1.05), SPD 15 -> 19 (x1.25), EXP 6 -> 11 (x1.75), all
  half-up.
- `ninja check` still reports 925/925 passing after the modded build, confirming
  no reconstructed code was affected.
- `source_sha1`, `record_id` and `name_id` are preserved so the builder still
  validates the document against your own ROM.

### Note on `tools/build_nds.ps1`

The wrapper probes only `python.exe`/`python` on `PATH` and requires 3.11+. On
this workstation `python` is 3.9.1, so the build stops even though `py -3.12` is
installed. The error message suggests running it "from a shell where `py -3`
selects it", but the script never actually tries `py -3`. Work around it by
putting a new enough interpreter first, using the launcher to find the install:

```powershell
$env:PATH = (py -3.12 -c "import sys,os; print(os.path.dirname(sys.executable))") + $env:PATH
```


## Adding another profile

Copy `profile.json`, change `id`/`name`, adjust `transforms`, and point
`--profile` and `--output` at the new files. Profiles are independent, so
several can coexist and be selected at build time via `-DataProject`.
