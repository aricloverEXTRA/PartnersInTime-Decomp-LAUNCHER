#!/usr/bin/env python3
"""Per-function candidate loop for the Partners in Time decompilation.

This mirrors the tight iterate-and-bank loop used by other matching DS
decompilations (for example khdays-decomp's ``getcand.py``/``match.py`` and
SM64DS's ``match.py``): inspect the exact native bytes for one function, then
compile a candidate with the pinned Metrowerks compiler and compare the
resulting object bytes, relocation-aware.

Subcommands:

    index                     Build/refresh the symbol -> delink object index.
    show SYMBOL               Disassemble a native function and list its calls.
    match --source FILE ...   Compile a candidate and compare it to the mirror.

``match`` understands both an in-tree source (defaults the mirror to
``build/eur/delinks/<source>.o`` and rebuilds ``build/eur/<source>.o`` through
ninja) and a scratch file compared against an explicit ``--object``.

Everything here reads the private ROM-derived objects under ``build/``; no game
bytes are written to tracked paths. Near misses are banked under
``build/analysis/`` so a later session does not repeat a dead end.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from elftools.elf.elffile import ELFFile

try:
    import capstone
except ImportError:  # pragma: no cover - only needed for ``show``
    capstone = None


# Preserve the junction path (space-free) instead of resolving to the real
# directory, which contains spaces and breaks MWCC's ``-MD`` escaping.
ROOT = Path(os.path.abspath(__file__)).parent.parent

BUILD_DIR = ROOT / "build" / "eur"
DELINKS_DIR = BUILD_DIR / "delinks"
CONFIG_DIR = ROOT / "config" / "eur"
MWCC = ROOT / "tools" / "mwccarm" / "1.2" / "base" / "mwccarm.exe"

ANALYSIS_DIR = ROOT / "build" / "analysis"
INDEX_PATH = ANALYSIS_DIR / "candidate_index.json"
NEARMISS_PATH = ANALYSIS_DIR / "candidate_nearmiss.json"

GAME_VERSION = "eur"

# Keep in step with tools/configure.py. MWCC accepts these multi-token flags.
CC_FLAGS = " ".join([
    "-O4,p",
    "-enum int",
    "-char signed",
    "-str noreuse",
    "-proc arm946e",
    "-gccext,on",
    "-fp soft",
    "-inline noauto",
    "-Cpp_exceptions off",
    "-RTTI off",
    "-interworking",
    "-w off",
    "-sym on",
    "-gccinc",
    "-nolink",
    "-msgstyle gcc",
]).split()


class Object:
    """A minimal ELF view: per-section bytes, function symbols and relocations.

    Code is not always in ``.text``: overlay delink objects split functions
    between ``.text`` and ``.init``, so each symbol keeps the section index it
    was defined in and reads are resolved per section.
    """

    def __init__(self, path: Path):
        self.path = Path(path)
        with open(self.path, "rb") as handle:
            elf = ELFFile(handle)
            self.machine = elf["e_machine"]
            self.sections = {}
            for index, section in enumerate(elf.iter_sections()):
                self.sections[index] = (section.name, section.data()
                                        if section["sh_type"] != "SHT_NOBITS" else b"")
            self.funcs = {}
            symtab = elf.get_section_by_name(".symtab")
            if symtab is not None:
                for symbol in symtab.iter_symbols():
                    if symbol["st_info"]["type"] == "STT_FUNC" and symbol["st_size"] > 0:
                        if isinstance(symbol["st_shndx"], int):
                            self.funcs[symbol.name] = (
                                symbol["st_shndx"], symbol["st_value"], symbol["st_size"])
            self.relocs = {}
            for section in elf.iter_sections():
                if str(section["sh_type"]) not in ("SHT_RELA", "SHT_REL"):
                    continue
                table = self.relocs.setdefault(section["sh_info"], {})
                linked = elf.get_section(section["sh_link"])
                for reloc in section.iter_relocations():
                    symbol = linked.get_symbol(reloc["r_info_sym"])
                    table[reloc["r_offset"]] = symbol.name

    def function_bytes(self, symbol: str) -> bytes:
        shndx, value, size = self.funcs[symbol]
        data = self.sections.get(shndx, ("", b""))[1]
        return data[value:value + size]

    def function_relocs(self, symbol: str) -> dict:
        """Relocations inside the function, keyed by function-relative offset."""
        shndx, value, size = self.funcs[symbol]
        relocs = self.relocs.get(shndx, {})
        return {offset - value: name for offset, name in relocs.items()
                if value <= offset < value + size}


class Mismatch:
    def __init__(self, offset: int, target: int, base: int):
        self.offset = offset
        self.target = target
        self.base = base


class Comparison:
    def __init__(self, symbol: str):
        self.symbol = symbol
        self.target_size = 0
        self.base_size = 0
        self.matched_bytes = 0
        self.total_bytes = 0
        self.mismatches = []
        self.reloc_diffs = []

    @property
    def is_match(self) -> bool:
        return (self.target_size == self.base_size
                and not self.mismatches
                and not self.reloc_diffs)


def compare_function(target: Object, base: Object, symbol: str):
    """Byte-compare one function, masking relocated words and checking callees."""
    if symbol not in target.funcs or symbol not in base.funcs:
        return None

    t_size = target.funcs[symbol][2]
    b_size = base.funcs[symbol][2]
    t_bytes = target.function_bytes(symbol)
    b_bytes = base.function_bytes(symbol)
    t_relocs = target.function_relocs(symbol)
    b_relocs = base.function_relocs(symbol)

    result = Comparison(symbol)
    result.target_size = t_size
    result.base_size = b_size
    result.total_bytes = t_size

    for offset in range(min(len(t_bytes), len(b_bytes))):
        word = offset - (offset % 4)
        if word in t_relocs or word in b_relocs:
            continue
        if t_bytes[offset] == b_bytes[offset]:
            result.matched_bytes += 1
        elif len(result.mismatches) < 24:
            result.mismatches.append(Mismatch(offset, t_bytes[offset], b_bytes[offset]))

    for offset in sorted(set(t_relocs) | set(b_relocs)):
        if t_relocs.get(offset) != b_relocs.get(offset):
            result.reloc_diffs.append((offset, t_relocs.get(offset), b_relocs.get(offset)))
        else:
            result.matched_bytes += 4
    return result


def load_symbol_addresses() -> dict:
    pattern = re.compile(
        r"^(?P<name>\S+)\s+kind:function\((?P<cpu>\w+),size=(?P<size>0x[0-9a-fA-F]+)\)\s+"
        r"addr:(?P<addr>0x[0-9a-fA-F]+)")
    out = {}
    for path in CONFIG_DIR.rglob("symbols.txt"):
        for line in path.read_text().splitlines():
            match = pattern.match(line.strip())
            if match:
                out[match.group("name")] = {
                    "cpu": match.group("cpu"),
                    "size": int(match.group("size"), 16),
                    "addr": int(match.group("addr"), 16),
                    "module": path.parent.name,
                }
    return out


def build_index() -> dict:
    symbols = {}
    for path in sorted(DELINKS_DIR.rglob("*.o")):
        try:
            obj = Object(path)
        except Exception:
            continue
        unit = path.relative_to(DELINKS_DIR).with_suffix("").as_posix()
        for name, (shndx, value, size) in obj.funcs.items():
            symbols.setdefault(name, {
                "object": path.relative_to(ROOT).as_posix(),
                "unit": unit,
                "section": obj.sections.get(shndx, ("?", b""))[0],
                "offset": value,
                "size": size,
            })
    addresses = load_symbol_addresses()
    for name, info in symbols.items():
        if name in addresses:
            info["address"] = addresses[name]
    return {"symbols": symbols}


def get_index(rebuild: bool) -> dict:
    if not rebuild and INDEX_PATH.is_file():
        return json.loads(INDEX_PATH.read_text())
    index = build_index()
    ANALYSIS_DIR.mkdir(parents=True, exist_ok=True)
    INDEX_PATH.write_text(json.dumps(index))
    return index


def include_flags():
    flags = ["-i", "include"]
    for path in sorted((ROOT / "libs").rglob("include")):
        if path.is_dir():
            flags += ["-i", path.as_posix()]
    return flags


def compile_scratch(source: Path, out_dir: Path) -> Path:
    lang = "-lang=c++" if source.suffix in (".cpp", ".cxx") else "-lang=c"
    command = [str(MWCC)] + CC_FLAGS + [lang] + include_flags() + [
        "-d", GAME_VERSION, "-c", str(source), "-o", str(out_dir)]
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if completed.returncode != 0:
        sys.stderr.write(completed.stdout)
        sys.stderr.write(completed.stderr)
        raise SystemExit(f"mwcc failed for {source}")
    return out_dir / (source.stem + ".o")


def disassemble(obj: Object, symbol: str, address, cpu: str) -> None:
    if capstone is None:
        raise SystemExit("the 'capstone' package is required for this command")
    value = obj.funcs[symbol][1]
    code = obj.function_bytes(symbol)
    relocs = obj.function_relocs(symbol)
    mode = capstone.CS_MODE_THUMB if cpu == "thumb" else capstone.CS_MODE_ARM
    md = capstone.Cs(capstone.CS_ARCH_ARM, mode | capstone.CS_MODE_LITTLE_ENDIAN)
    base_addr = address if address is not None else value
    for insn in md.disasm(code, base_addr):
        rel_offset = insn.address - base_addr
        note = f"   ; -> {relocs[rel_offset]}" if rel_offset in relocs else ""
        print(f"  {insn.address:08x}:  {insn.mnemonic:<8} {insn.op_str}{note}")


def cmd_index(args) -> int:
    index = build_index()
    ANALYSIS_DIR.mkdir(parents=True, exist_ok=True)
    INDEX_PATH.write_text(json.dumps(index))
    print(f"indexed {len(index['symbols'])} function symbols -> {INDEX_PATH}")
    return 0


def resolve_target(args):
    """Return ``(object, symbol, address, cpu)`` for the ``show`` command."""
    if args.object:
        object_path = Path(args.object)
        obj = Object(ROOT / object_path)
        if args.symbol not in obj.funcs:
            raise SystemExit(f"{args.symbol} not found in {args.object}")
    else:
        index = get_index(args.rebuild_index)
        info = index["symbols"].get(args.symbol)
        if info is None:
            raise SystemExit(f"{args.symbol} not found; run 'index' or pass --object")
        obj = Object(ROOT / info["object"])

    addr = load_symbol_addresses().get(args.symbol)
    address = addr["addr"] if addr else None
    cpu = addr["cpu"] if addr else "arm"
    return obj, args.symbol, address, cpu


def cmd_show(args) -> int:
    obj, symbol, address, cpu = resolve_target(args)
    shndx, value, size = obj.funcs[symbol]
    shown = obj.path.relative_to(ROOT) if obj.path.is_absolute() else obj.path
    section = obj.sections.get(shndx, ("?", b""))[0]
    print(f"symbol  {symbol}")
    print(f"object  {shown}")
    print(f"section {section} offset {value:#x}, size {size:#x} ({size} bytes)")
    if address is not None:
        print(f"address {address:#010x}")
    print()
    disassemble(obj, symbol, address, cpu)
    print()
    print("match a candidate with:")
    print(f"  python tools/candidate.py match --source <file.c> "
          f"--object {shown.as_posix()} --symbol {symbol}")
    return 0


def collect_comparisons(target: Object, base: Object, symbol):
    names = [symbol] if symbol else sorted(set(target.funcs) & set(base.funcs))
    return [c for c in (compare_function(target, base, n) for n in names) if c is not None]


def print_comparison(comparison: Comparison) -> None:
    status = "MATCH" if comparison.is_match else "DIFF "
    print(f"[{status}] {comparison.symbol}: "
          f"{comparison.matched_bytes}/{comparison.total_bytes} bytes "
          f"(target {comparison.target_size}, base {comparison.base_size})")
    for mismatch in comparison.mismatches:
        print(f"    byte {mismatch.offset:#06x}: target={mismatch.target:#04x} base={mismatch.base:#04x}")
    for offset, target_name, base_name in comparison.reloc_diffs:
        print(f"    call {offset:#06x}: target={target_name} base={base_name}")


def bank_near_miss(source: Path, target: Path, comparison: Comparison) -> None:
    data = {"records": []}
    if NEARMISS_PATH.is_file():
        try:
            data = json.loads(NEARMISS_PATH.read_text())
        except json.JSONDecodeError:
            data = {"records": []}
    object_id = (target.relative_to(ROOT) if target.is_absolute() else target).as_posix()
    records = [
        record for record in data.get("records", [])
        if not (record.get("symbol") == comparison.symbol
                and record.get("object") == object_id)
    ]
    records.append({
        "source": source.as_posix(),
        "object": object_id,
        "symbol": comparison.symbol,
        "target_size": comparison.target_size,
        "base_size": comparison.base_size,
        "matched_bytes": comparison.matched_bytes,
        "total_bytes": comparison.total_bytes,
        "mismatches": [
            {"offset": m.offset, "target": m.target, "base": m.base}
            for m in comparison.mismatches
        ],
        "reloc_diffs": [
            {"offset": offset, "target": target_name, "base": base_name}
            for offset, target_name, base_name in comparison.reloc_diffs
        ],
        "timestamp": int(time.time()),
    })
    data["records"] = records
    ANALYSIS_DIR.mkdir(parents=True, exist_ok=True)
    NEARMISS_PATH.write_text(json.dumps(data, indent=2))


def cmd_match(args) -> int:
    source = Path(os.path.abspath(Path.cwd() / args.source))

    if not source.is_file():
        raise SystemExit(f"source not found: {source}")

    tmp = None
    try:
        if args.object:
            target_path = Path(args.object)
            target = Object(ROOT / target_path)
            tmp = Path(tempfile.mkdtemp(prefix="candidate_"))
            base = Object(compile_scratch(source, tmp))
        else:
            rel = source.relative_to(ROOT)
            target_path = DELINKS_DIR / rel.with_suffix(".o")
            base_path = BUILD_DIR / rel.with_suffix(".o")
            if not target_path.is_file():
                raise SystemExit(f"no delink mirror at {target_path}; use --object "
                                 "for a standalone comparison")
            if not args.no_build:
                ninja = shutil.which("ninja") or "ninja"
                completed = subprocess.run(
                    [ninja, base_path.relative_to(ROOT).as_posix()], cwd=ROOT)
                if completed.returncode != 0:
                    raise SystemExit("ninja failed")
            target = Object(target_path)
            base = Object(base_path)

        results = collect_comparisons(target, base, args.symbol)
        if not results:
            raise SystemExit("no shared function symbols to compare")

        all_match = True
        for comparison in results:
            print_comparison(comparison)
            if not comparison.is_match:
                all_match = False
                if not args.no_bank:
                    bank_near_miss(source, target.path, comparison)

        matched = sum(c.matched_bytes for c in results)
        total = sum(c.total_bytes for c in results)
        print()
        print(f"{'>>> MATCH <<<' if all_match else 'near miss banked'}: "
              f"{matched}/{total} bytes across {len(results)} function(s)")
        return 0 if all_match else 1
    finally:
        if tmp is not None:
            shutil.rmtree(tmp, ignore_errors=True)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Per-function candidate loop.")
    sub = parser.add_subparsers(dest="command", required=True)

    index = sub.add_parser("index", help="build the symbol index cache")
    index.set_defaults(func=cmd_index)

    show = sub.add_parser("show", help="disassemble a native function")
    show.add_argument("symbol")
    show.add_argument("--object", help="delink object to read (skips the index)")
    show.add_argument("--rebuild-index", action="store_true")
    show.set_defaults(func=cmd_show)

    match = sub.add_parser("match", help="compile a candidate and compare it")
    match.add_argument("--source", required=True, help="candidate C/C++ file")
    match.add_argument("--object", help="target delink object (standalone mode)")
    match.add_argument("--symbol", help="limit comparison to one function")
    match.add_argument("--no-build", action="store_true", help="skip the ninja rebuild")
    match.add_argument("--no-bank", action="store_true", help="do not record a near miss")
    match.set_defaults(func=cmd_match)
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
