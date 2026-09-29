# PiT Patcher

A test-only ROM patcher for the European release of *Mario & Luigi: Partners in
Time* (`ARMP`, SHA-1 `ba4ec2f99b4f2e0047601552bccf00aa73e28701`).

The patch applies the **Hard Mode** plan: enemy HP, power, defense and speed
rise, along with experience and coin drops.

| Field | Scale |
|---|---|
| Enemy HP | x11/10 |
| Enemy power | x2 |
| Enemy defense | x21/20 |
| Enemy speed | x5/4 |
| Experience | x7/4 |
| Coins | x7/4 |

## What you need

Your own copy of the European cartridge ROM. It is not included, and no part of
it is stored in this tool or its APK.

## Builds

| Platform | Artifact | How |
|---|---|---|
| Windows x64 | `pit_patcher.exe` | `build-release.ps1` |
| Windows x64 | `pit_patch.exe` | headless driver, same pipeline |
| Android | `pit-patcher-release.apk` | `android\build-apk.ps1` |

The Windows build needs MinGW gcc and the SDL2 development files. The Android
build needs a JDK 17 and the Android SDK build-tools; it has no Gradle, NDK or
native code, and requests no storage permission — files are chosen through the
Storage Access Framework.

## Using it

1. Choose your EUR ROM.
2. Choose an output file. The output must not be the source.
3. Press **PATCH ROM**.

The app verifies the ROM size, SHA-1, title, game code and header CRC-16,
locates `BData/BDataMon.dat` through the NitroFS directory table, scales the
98 stat records, recomputes the header CRC and writes the result. It patches a
copy, so a failure never damages the source.

## Important

**The patched ROM is not expected to boot.** The decompilation in this
repository is incomplete: there is no ARM interpreter, memory map, GX or HAL
implementation yet, so nothing in this project can run the game. The patcher
demonstrates a working, verified patch pipeline against real cartridge data; it
is not a playable build.

## Layout

```
src/core/            patch pipeline, ROM/FNT reader, SHA-1, PNG writer
src/platform/sdl2/   graphical patcher
tools/               plan generator and verification scripts
android/             pure-Java patcher, same generated plan
```

`tools/patch_sources/hard_mode_plan.json` and `tools/patch_sources/font8x8.txt`
are the only inputs. `tools/gen_patchplan.py` generates the C header and Java
class both builds compile against, so the two cannot drift apart.

## Verification

```powershell
python tools/gen_patchplan.py --check
python tools/check_patch_roundtrip.py --rom <rom.nds>
python tools/check_c_java_parity.py --rom <rom.nds>
```

The last one patches the same ROM with both the C and the Java build and
compares the results byte for byte. Current result: **exact match**, 98 records
and 588 fields.

## Legal

This is a fan tool for a game you must own. It contains no Nintendo code, ROM
data, extracted assets or trademarks. Do not redistribute the ROM.
