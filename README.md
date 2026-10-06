> ℹ️ **AI-generated decompilation**: This decompilation is fully generated using generative AI, under human direction and supervision.

> If your response to that is that this project is "just [unmaintainable AI slop](https://github.com/tangosdev/sm64ds-decomp) anyway", feel free to leave. **You are not welcome here.**

> If, however, you're interested in how the project works, why this approach was chosen, and what human direction and verification are involved, read on.

> [More information here.](https://github.com/aricloverEXTRA/PartnersInTime-Decomp/blob/main/docs/AI_USAGE.md)

# Mario & Luigi: Partners in Time

A work-in-progress C/C++ decompilation of the Nintendo DS game. The goal is to
rebuild the game from readable, editable source and make it easier to mod.

The European version currently rebuilds byte for byte. Verified C/C++ gradually
replaces the remaining assembly, with emulator checks to confirm game behavior.

## Progress

[![Decompilation progress](docs/progress.svg)](docs/PROGRESS.md)

The main percentage measures C/C++ that compiles to the original ARM9 code,
byte for byte. Assembly coverage is tracked separately.

⚠️ This percentage does not reflect the completion of the project. Even with a fully matching codebase there are still many tasks, the most important one being the field/battle scripts.

## Build the European version

You will need:

- Windows, Python 3.11 or newer, and Ninja on `PATH`.
- A compatible Metrowerks ARM toolchain, with `mwccarm.exe` and `mwldarm.exe`
  in `tools/mwccarm/1.2/base/`.
- Your own decrypted European ROM at `extract/baserom_PiT_eur.nds`.
  See [ROM requirements and checksums](extract/README.md).

Game assets and proprietary tools are not included; supply your own ROM and
lawfully obtained compiler.

From the repository root, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_nds.ps1 -DisableDataMods
```

This configures the project, builds the sources, and produces `PiT_eur.nds` in
the repository root. To check that the compiled code matches the original:

```powershell
ninja check
```

Open the built ROM in your emulator. CLion users can also use the shared
**Build and Run EUR NDS** configuration after setting its emulator path.

To include a local data mod from `data/eur/project.json`, omit
`-DisableDataMods`. See the [modding guide](docs/DATA_MODDING.md) for editing
text, stats, and scripts.

## Documentation

- [Progress and how it is measured](docs/PROGRESS.md)
- [Source style and matching guidelines](docs/DECOMPILATION_STYLE.md)
- [Modding text, stats, and scripts](docs/DATA_MODDING.md)
- [Emulator automation and RAM/VRAM analysis](docs/research/RUNTIME_ANALYSIS.md)
- [Game subsystem map](docs/research/OVERLAY_MAP.md) and [script VM reference](docs/research/SCRIPT_VM_SEMANTICS.md)
- [Reassembly tools and roadmap](docs/REASSEMBLY_PLAN.md)
- [Detailed reconstruction milestones](docs/BATTLE_MATCHING_MILESTONES.md)
- [Keeping extracted game content local](docs/LOCAL_PRIVATE_CONTENT.md)
