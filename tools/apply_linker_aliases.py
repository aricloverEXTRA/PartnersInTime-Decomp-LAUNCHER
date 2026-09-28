"""Preserve interior entry labels and link-time constants used by compiled C.

The assignments export addresses or scalar values; they allocate no bytes.
Interior offsets come from the symbol map; constants are explicit per-version
values in the manifest. Module/symbol checks still verify exact bytes and layout.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess


def apply_aliases(script: str, entries: list[dict], symbols_root: Path) -> str:
    exported = set()
    for entry in entries:
        object_name = Path(entry["source"]).with_suffix(".o").name
        anchor = re.compile(r"^([ \t]+)" + re.escape(object_name) + r"\(\.text\)$", re.M)
        matches = list(anchor.finditer(script))
        if len(matches) != 1:
            raise ValueError(f"Expected one text placement for {object_name}, found {len(matches)}")
        if "constants" in entry:
            # These are values, not allocated data. Keeping them at link time
            # preserves arithmetic on arena boundaries and stack reservations.
            if set(entry) != {"source", "constants"} or not entry["constants"]:
                raise ValueError("A constant entry requires only source and nonempty constants")
            lines = []
            for name, value in entry["constants"].items():
                if not re.fullmatch(r"[A-Za-z_]\w*", name):
                    raise ValueError(f"Invalid linker constant name: {name}")
                if not isinstance(value, str) or not re.fullmatch(r"0x[0-9a-fA-F]{1,8}", value):
                    raise ValueError(f"Invalid 32-bit linker constant: {name}={value}")
                if name in exported or re.search(r"\b" + re.escape(name) + r"\s*=", script):
                    raise ValueError(f"Alias already defined: {name}")
                exported.add(name)
                lines.append(f"{name} = {value};")
            match = matches[0]
            insertion = "\n" + "\n".join(match[1] + line for line in lines)
            script = script[:match.end()] + insertion + script[match.end():]
            continue
        symbols = {}
        for line in (symbols_root / entry["symbols"]).read_text().splitlines():
            match = re.fullmatch(
                r"(\w+) kind:(function\(arm,size=(0x[0-9a-f]+)\)|label\(arm\)) "
                r"addr:(0x[0-9a-f]+)", line
            )
            if match:
                symbols[match[1]] = (int(match[4], 16),
                                      int(match[3], 16) if match[3] else 0)
        owner = entry["owner"]
        start, size = symbols[owner]
        if not size:
            raise ValueError(f"Alias owner is not an ARM function: {owner}")
        lines = []
        for name in entry["labels"]:
            address, label_size = symbols[name]
            if label_size or not start < address < start + size or (address - start) % 4:
                raise ValueError(f"{name} is not an aligned interior ARM label of {owner}")
            if name in exported or re.search(r"\b" + re.escape(name) + r"\s*=", script):
                raise ValueError(f"Alias already defined: {name}")
            exported.add(name)
            lines.append(f"{name} = {owner} + 0x{address - start:x};")
        match = matches[0]
        insertion = "\n" + "\n".join(match[1] + line for line in lines)
        script = script[:match.end()] + insertion + script[match.end():]
    return script


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lcf", type=Path, required=True)
    parser.add_argument("--config-root", type=Path, required=True)
    parser.add_argument("--dsd", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--objects", type=Path, required=True)
    args = parser.parse_args()
    subprocess.run([str(args.dsd.resolve()), "lcf", "-c", str(args.config),
                    "--lcf-file", str(args.lcf), "--objects-file", str(args.objects)], check=True)
    manifest = args.config_root / "linker_aliases.json"
    if not manifest.exists():
        return
    result = apply_aliases(args.lcf.read_text(), json.loads(manifest.read_text()), args.config_root)
    args.lcf.write_text(result)


if __name__ == "__main__":
    main()
