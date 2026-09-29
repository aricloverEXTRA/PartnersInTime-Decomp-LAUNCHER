#!/usr/bin/env python3
"""Verify a user-supplied Mario & Luigi: Partners in Time ROM.

This tool contains no Nintendo assets and extracts none. It validates that a
ROM the user already owns is a revision the port supports, and reports where
its NitroFS archive lives so the runtime can read assets from it.

Game content, graphics, audio and text are the property of Nintendo. Any
ROM-derived output belongs in a local, git-ignored directory and must never
be committed or redistributed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

KNOWN_ROMS = {
    "ba4ec2f99b4f2e0047601552bccf00aa73e28701": "EUR",
    "89c9136db3c3975c451a907e8bd6861ce6b81557": "USA",
    "e23db7ff44d38299a6f1c377780378b191592a7e": "USA (Rev 1)",
}

EXPECTED_TITLE = "MARIO&LUIGI"
DS_CART_SIZE = 0x04000000


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def parse_header(raw: bytes) -> dict:
    if len(raw) < 0x200:
        raise ValueError("file is smaller than a 512-byte NDS header")

    title = raw[0x00:0x0C].decode("ascii", "replace").rstrip("\x00 ")
    game_code = raw[0x0C:0x10].decode("ascii", "replace")
    unit_code = raw[0x12]

    fields = {
        "title": title,
        "game_code": game_code,
        "unit_code": unit_code,
        # Header layout per GBATEK: the ARM9/ARM7 blocks are
        # (rom_offset, entry, load, size) as four u32s starting at 0x20, and
        # the banner pointer is at 0x68. Reading these from 0x1C silently
        # yields the reserved words and the arm9 load address instead, which
        # still look like plausible numbers but describe the wrong fields.
        "arm9_offset": struct.unpack_from("<I", raw, 0x20)[0],
        "arm9_entry": struct.unpack_from("<I", raw, 0x24)[0],
        "arm9_load": struct.unpack_from("<I", raw, 0x28)[0],
        "arm9_size": struct.unpack_from("<I", raw, 0x2C)[0],
        "arm7_offset": struct.unpack_from("<I", raw, 0x30)[0],
        "arm7_entry": struct.unpack_from("<I", raw, 0x34)[0],
        "arm7_load": struct.unpack_from("<I", raw, 0x38)[0],
        "arm7_size": struct.unpack_from("<I", raw, 0x3C)[0],
        "fnt_offset": struct.unpack_from("<I", raw, 0x40)[0],
        "fnt_size": struct.unpack_from("<I", raw, 0x44)[0],
        "fat_offset": struct.unpack_from("<I", raw, 0x48)[0],
        "fat_size": struct.unpack_from("<I", raw, 0x4C)[0],
        "banner_offset": struct.unpack_from("<I", raw, 0x68)[0],
    }

    stored_crc = struct.unpack_from("<H", raw, 0x15E)[0]
    fields["header_crc_stored"] = stored_crc
    fields["header_crc_computed"] = crc16_modbus(raw[0x00:0x15E])
    fields["header_crc_ok"] = stored_crc == fields["header_crc_computed"]
    return fields


def nitrofs_present(header: dict, file_size: int) -> bool:
    fnt_off = header["fnt_offset"]
    fnt_size = header["fnt_size"]
    fat_off = header["fat_offset"]
    fat_size = header["fat_size"]

    if fnt_size == 0 or fat_size == 0:
        return False
    if fnt_off + fnt_size > file_size or fat_off + fat_size > file_size:
        return False
    return (fat_size // 8) > 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="path to the user's own ROM")
    parser.add_argument("--json", action="store_true", dest="as_json")
    args = parser.parse_args()

    if not args.rom.is_file():
        print(f"error: no such file: {args.rom}", file=sys.stderr)
        return 2

    size = args.rom.stat().st_size
    if size != DS_CART_SIZE:
        print(
            f"error: unexpected size {size} (0x{size:x}), expected "
            f"{DS_CART_SIZE} (0x{DS_CART_SIZE:x})",
            file=sys.stderr,
        )
        return 2

    digest = hashlib.sha1()
    with args.rom.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    sha1 = digest.hexdigest()

    with args.rom.open("rb") as handle:
        header = parse_header(handle.read(0x200))

    region = KNOWN_ROMS.get(sha1)
    supported = region is not None
    has_nitrofs = nitrofs_present(header, size)

    result = {
        "path": str(args.rom),
        "size": size,
        "sha1": sha1,
        "region": region,
        "supported": supported,
        "header": header,
        "title_ok": header["title"].startswith(EXPECTED_TITLE),
        "nitrofs": has_nitrofs,
    }

    if args.as_json:
        print(json.dumps(result, indent=2))
    else:
        print(f"file   : {args.rom}")
        print(f"size   : {size} (0x{size:x})")
        print(f"sha1   : {sha1}")
        print(f"region : {region or 'unrecognised'}")
        print(f"title  : {header['title']!r} (code {header['game_code']})")
        print(f"crc16  : {'ok' if header['header_crc_ok'] else 'MISMATCH'}")
        print(f"nitrofs: {'present' if has_nitrofs else 'absent'} "
              f"(fnt 0x{header['fnt_offset']:x}+0x{header['fnt_size']:x}, "
              f"fat 0x{header['fat_offset']:x}+0x{header['fat_size']:x})")
        if not supported:
            print("\nerror: this ROM revision is not supported.", file=sys.stderr)
        elif not result["title_ok"]:
            print("\nerror: header does not identify as Partners in Time.", file=sys.stderr)

    if not supported or not result["title_ok"]:
        return 1
    if not has_nitrofs:
        print("\nwarning: no NitroFS archive found; asset loading will fail.",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
