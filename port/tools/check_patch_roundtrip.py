#!/usr/bin/env python3
"""Cross-check a patched ROM against the Python profile pipeline.

The Windows patcher (C) and the Android patcher (Java) both apply the plan with
their own integer arithmetic. The original profile export was produced by
tools/make_data_profile.py using Decimal. Rounding on an exact .5 boundary is
where those three would disagree, so this compares the patched ROM's bytes
directly against the export instead of recomputing the transforms again.

Any mismatch here means a shipped patcher would write different bytes than the
balanced profile the plan was designed against.

    usage: check_patch_roundtrip.py <patched.nds> <generated export json>
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

# Recovered from the decompiled BattleEnemyStatRecord, cross-checked against
# the working NitroFS reader in src/core/pit_rom.c.
FIELDS = {
    "max_hp": 0x06,
    "power": 0x08,
    "defense": 0x0A,
    "speed": 0x0C,
    "experience": 0x20,
    "coins": 0x22,
}
RECORD_SIZE = 0x2C
RECORD_COUNT = 98


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2

    rom_path = Path(sys.argv[1])
    export_path = Path(sys.argv[2])

    raw = rom_path.read_bytes()
    export = json.loads(export_path.read_text(encoding="utf-8"))
    records = export["records"]

    if export["record_size"] != RECORD_SIZE:
        print(f"error: export record_size {export['record_size']} != {RECORD_SIZE}")
        return 1
    if len(records) != RECORD_COUNT:
        print(f"error: export has {len(records)} records, expected {RECORD_COUNT}")
        return 1

    # Find the stat table the same way the patcher does rather than trusting a
    # hard-coded offset, so a changed ROM layout fails loudly here too.
    table = None
    for candidate in range(0, len(raw) - RECORD_SIZE * RECORD_COUNT, 0x200):
        chunk = raw[candidate: candidate + RECORD_SIZE * RECORD_COUNT]
        if len(chunk) != RECORD_SIZE * RECORD_COUNT:
            break
        if all(
            struct.unpack_from("<H", chunk, i * RECORD_SIZE + FIELDS[name])[0] == rec[name]
            for i, rec in enumerate(records)
            for name in FIELDS
        ):
            table = candidate
            break

    if table is None:
        print("error: no 98-record window in the ROM matches the export")
        return 1

    checked = 0
    bad = 0
    for i, rec in enumerate(records):
        base = table + i * RECORD_SIZE
        for name, offset in FIELDS.items():
            (value,) = struct.unpack_from("<H", raw, base + offset)
            if value != rec[name]:
                bad += 1
                if bad <= 8:
                    print(f"  MISMATCH rec {i:3d} {name:<11} "
                          f"rom {value:>6} != export {rec[name]:>6}")
            checked += 1

    print(f"rom     : {rom_path} ({len(raw)} bytes)")
    print(f"export  : {export_path}")
    print(f"table   : 0x{table:08X}")
    print(f"checked : {checked} fields over {len(records)} records")
    print(f"result  : {'EXACT MATCH' if bad == 0 else f'{bad} MISMATCHES'}")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
