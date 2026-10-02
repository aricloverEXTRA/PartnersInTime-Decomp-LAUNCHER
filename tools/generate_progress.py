#!/usr/bin/env python3
"""Generate the source-controlled decompilation progress tracker.

The tracker deliberately measures only DSD ranges marked as ARM9 code. ARM7
autoloads are still mixed code/data images, so their symbolic-source byte count
is reported separately instead of manufacturing a misleading denominator.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from html import escape
from pathlib import Path
from typing import Iterable, Sequence


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SVG = ROOT / "docs" / "progress.svg"
DEFAULT_JSON = ROOT / "docs" / "progress.json"

STATUS_C = "matching_c"
STATUS_ASM = "symbolic_asm"
STATUS_REMAINING = "remaining"

COLORS = {
    STATUS_C: "#2dd4a0",
    STATUS_ASM: "#38bdf8",
    STATUS_REMAINING: "#263244",
}

SECTION_RE = re.compile(
    r"^\s+(?P<section>\.\S+)\s+"
    r"start:(?P<start>0x[0-9a-f]+)\s+"
    r"end:(?P<end>0x[0-9a-f]+)"
    r"(?:\s+kind:(?P<kind>\w+))?",
    re.IGNORECASE,
)
HEADING_RE = re.compile(r"^(?P<name>\S.*):$")
FUNCTION_RE = re.compile(
    r"^(?P<name>\S+)\s+kind:function\([^,]+,size=(?P<size>0x[0-9a-f]+)\)\s+"
    r"addr:(?P<address>0x[0-9a-f]+)",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class Range:
    start: int
    end: int

    @property
    def size(self) -> int:
        return self.end - self.start


@dataclass(frozen=True)
class CoverageRange(Range):
    status: str
    label: str
    source: str


@dataclass(frozen=True)
class Unit(Range):
    status: str
    label: str


@dataclass
class Component:
    key: str
    label: str
    config_dir: Path
    code_ranges: list[Range]
    functions: list[tuple[str, Range]]
    c_ranges: list[CoverageRange]
    asm_ranges: list[CoverageRange]
    units: list[Unit]

    @property
    def total(self) -> int:
        return sum(item.size for item in self.code_ranges)

    def status_bytes(self, status: str) -> int:
        return sum(item.size for item in self.units if item.status == status)


@dataclass(frozen=True)
class Rect:
    x: float
    y: float
    width: float
    height: float


def parse_int(value: str | int) -> int:
    return value if isinstance(value, int) else int(value, 0)


def human_bytes(size: int) -> str:
    if size < 1024:
        return f"{size} B"
    if size < 1024 * 1024:
        return f"{size / 1024:.1f} KiB"
    return f"{size / (1024 * 1024):.2f} MiB"


def module_configs(version: str) -> list[tuple[str, str, Path]]:
    config_root = ROOT / "config" / version / "arm9"
    configs = [
        ("arm9", "ARM9 resident", config_root),
        ("itcm", "ARM9 ITCM", config_root / "itcm"),
        ("dtcm", "ARM9 DTCM", config_root / "dtcm"),
    ]
    overlays_root = config_root / "overlays"
    for directory in sorted(
        overlays_root.glob("ov[0-9][0-9][0-9]"),
        key=lambda path: int(path.name[2:]),
    ):
        configs.append(
            (
                f"arm9_{directory.name}",
                f"Overlay {int(directory.name[2:]):02d}",
                directory,
            )
        )
    return configs


def parse_linked_sources(version: str) -> set[str]:
    manifest = ROOT / "config" / version / "arm9" / "linked_sources.txt"
    sources: set[str] = set()
    for raw_line in manifest.read_text(encoding="utf-8").splitlines():
        source = raw_line.split("#", maxsplit=1)[0].strip()
        if source:
            sources.add(source.replace("\\", "/"))
    return sources


def parse_delinks(
    path: Path, matching_sources: set[str]
) -> tuple[list[Range], list[CoverageRange], list[CoverageRange]]:
    code_ranges: list[Range] = []
    code_sections: set[str] = set()
    c_ranges: list[CoverageRange] = []
    asm_ranges: list[CoverageRange] = []
    source: str | None = None

    for line in path.read_text(encoding="utf-8").splitlines():
        heading = HEADING_RE.match(line)
        if heading:
            source = heading.group("name")
            continue

        section = SECTION_RE.match(line)
        if not section:
            continue
        item = Range(parse_int(section.group("start")), parse_int(section.group("end")))
        if source is None:
            if section.group("kind") == "code":
                code_ranges.append(item)
                code_sections.add(section.group("section"))
            continue

        source_path = ROOT / source
        suffix = Path(source).suffix
        if (
            section.group("section") in code_sections
            and source.replace("\\", "/") in matching_sources
            and source_path.is_file()
        ):
            if suffix in {".c", ".cpp"}:
                c_ranges.append(
                    CoverageRange(
                        item.start,
                        item.end,
                        STATUS_C,
                        source_path.stem,
                        source,
                    )
                )
            elif suffix == ".s":
                asm_ranges.append(
                    CoverageRange(
                        item.start,
                        item.end,
                        STATUS_ASM,
                        source_path.stem,
                        source,
                    )
                )

    return code_ranges, c_ranges, asm_ranges


def parse_functions(path: Path) -> list[tuple[str, Range]]:
    functions: list[tuple[str, Range]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = FUNCTION_RE.match(line)
        if not match:
            continue
        start = parse_int(match.group("address"))
        functions.append(
            (match.group("name"), Range(start, start + parse_int(match.group("size"))))
        )
    return functions


def intersect(left: Range, right: Range) -> Range | None:
    start = max(left.start, right.start)
    end = min(left.end, right.end)
    return Range(start, end) if start < end else None


def clip_coverage(
    coverage: Iterable[CoverageRange], code_ranges: Sequence[Range]
) -> list[CoverageRange]:
    clipped: list[CoverageRange] = []
    for item in coverage:
        for code_range in code_ranges:
            overlap = intersect(item, code_range)
            if overlap:
                clipped.append(
                    CoverageRange(
                        overlap.start,
                        overlap.end,
                        item.status,
                        item.label,
                        item.source,
                    )
                )
    return clipped


def status_at(
    address: int,
    c_ranges: Sequence[CoverageRange],
    asm_ranges: Sequence[CoverageRange],
) -> tuple[str, str]:
    for item in c_ranges:
        if item.start <= address < item.end:
            return STATUS_C, item.label
    for item in asm_ranges:
        if item.start <= address < item.end:
            return STATUS_ASM, item.label
    return STATUS_REMAINING, "Not yet decompiled"


def split_range(
    item: Range,
    label: str,
    c_ranges: Sequence[CoverageRange],
    asm_ranges: Sequence[CoverageRange],
) -> list[Unit]:
    boundaries = {item.start, item.end}
    for coverage in (*c_ranges, *asm_ranges):
        if item.start < coverage.start < item.end:
            boundaries.add(coverage.start)
        if item.start < coverage.end < item.end:
            boundaries.add(coverage.end)
    ordered = sorted(boundaries)
    result: list[Unit] = []
    for start, end in zip(ordered, ordered[1:]):
        status, source_label = status_at(start, c_ranges, asm_ranges)
        display_label = source_label if status != STATUS_REMAINING else label
        result.append(Unit(start, end, status, display_label))
    return result


def build_units(
    code_ranges: Sequence[Range],
    functions: Sequence[tuple[str, Range]],
    c_ranges: Sequence[CoverageRange],
    asm_ranges: Sequence[CoverageRange],
) -> list[Unit]:
    units: list[Unit] = []
    covered: list[Range] = []

    for name, function in sorted(functions, key=lambda item: item[1].start):
        for code_range in code_ranges:
            clipped = intersect(function, code_range)
            if clipped:
                units.extend(split_range(clipped, name, c_ranges, asm_ranges))
                covered.append(clipped)

    # Account for code-section gaps (for example resident ARM9's .init range).
    for code_range in code_ranges:
        boundaries = {code_range.start, code_range.end}
        for item in covered:
            overlap = intersect(code_range, item)
            if overlap:
                boundaries.update((overlap.start, overlap.end))
        ordered = sorted(boundaries)
        for start, end in zip(ordered, ordered[1:]):
            if any(item.start <= start and end <= item.end for item in covered):
                continue
            units.extend(
                split_range(
                    Range(start, end), "Unmapped code", c_ranges, asm_ranges
                )
            )

    return units


def load_patch_ranges(version: str) -> tuple[dict[str, list[CoverageRange]], int]:
    patch_path = ROOT / "reasm" / version / "patches.json"
    document = json.loads(patch_path.read_text(encoding="utf-8"))
    by_module: dict[str, list[CoverageRange]] = {}
    arm7_bytes = 0
    for patch in document["patches"]:
        source = str(patch["source"])
        if not (ROOT / "reasm" / version / source).is_file():
            continue
        size = parse_int(patch["size"])
        module = str(patch["module"])
        if module.startswith("arm7"):
            arm7_bytes += size
            continue
        start = parse_int(patch["address"])
        by_module.setdefault(module, []).append(
            CoverageRange(
                start,
                start + size,
                STATUS_ASM,
                Path(source).stem,
                source,
            )
        )
    return by_module, arm7_bytes


def collect_progress(version: str = "eur") -> tuple[list[Component], int]:
    patch_ranges, arm7_bytes = load_patch_ranges(version)
    matching_sources = parse_linked_sources(version)
    components: list[Component] = []
    for key, label, config_dir in module_configs(version):
        delinks = config_dir / "delinks.txt"
        symbols = config_dir / "symbols.txt"
        missing = delinks if not delinks.is_file() else symbols
        if not delinks.is_file() or not symbols.is_file():
            raise FileNotFoundError(f"missing progress input: {missing}")
        code_ranges, c_ranges, linked_asm_ranges = parse_delinks(delinks, matching_sources)
        if not code_ranges:
            continue
        c_ranges = clip_coverage(c_ranges, code_ranges)
        patch_asm_ranges = clip_coverage(patch_ranges.get(key, []), code_ranges)
        asm_ranges = clip_coverage(linked_asm_ranges + patch_asm_ranges, code_ranges)
        functions = parse_functions(symbols)
        units = build_units(code_ranges, functions, c_ranges, asm_ranges)
        component = Component(
            key,
            label,
            config_dir,
            code_ranges,
            functions,
            c_ranges,
            asm_ranges,
            units,
        )
        if sum(item.size for item in units) != component.total:
            raise ValueError(f"unit coverage does not equal code size for {key}")
        components.append(component)
    return components, arm7_bytes


def worst_ratio(row: Sequence[float], side: float) -> float:
    if not row or side <= 0:
        return float("inf")
    total = sum(row)
    smallest = min(row)
    largest = max(row)
    if smallest <= 0 or total <= 0:
        return float("inf")
    side_squared = side * side
    total_squared = total * total
    return max(
        side_squared * largest / total_squared,
        total_squared / (side_squared * smallest),
    )


def layout_row(
    row: Sequence[tuple[object, float]], rect: Rect
) -> tuple[list[tuple[object, Rect]], Rect]:
    area = sum(value for _, value in row)
    result: list[tuple[object, Rect]] = []
    if rect.width >= rect.height:
        row_width = area / rect.height if rect.height else 0
        cursor = rect.y
        for item, value in row:
            height = value / row_width if row_width else 0
            result.append((item, Rect(rect.x, cursor, row_width, height)))
            cursor += height
        return result, Rect(
            rect.x + row_width, rect.y, rect.width - row_width, rect.height
        )

    row_height = area / rect.width if rect.width else 0
    cursor = rect.x
    for item, value in row:
        width = value / row_height if row_height else 0
        result.append((item, Rect(cursor, rect.y, width, row_height)))
        cursor += width
    return result, Rect(
        rect.x, rect.y + row_height, rect.width, rect.height - row_height
    )


def squarify(
    items: Sequence[tuple[object, int]], rect: Rect
) -> list[tuple[object, Rect]]:
    positive = [(item, size) for item, size in items if size > 0]
    if not positive or rect.width <= 0 or rect.height <= 0:
        return []
    def stable_key(item: object) -> tuple[object, ...]:
        if isinstance(item, Component):
            return (item.key,)
        if isinstance(item, Unit):
            return (item.start, item.end, item.status, item.label)
        return (str(item),)

    positive.sort(key=lambda item: (-item[1], stable_key(item[0])))
    scale = rect.width * rect.height / sum(size for _, size in positive)
    remaining = [(item, size * scale) for item, size in positive]
    output: list[tuple[object, Rect]] = []
    current = rect

    while remaining:
        row: list[tuple[object, float]] = [remaining.pop(0)]
        side = min(current.width, current.height)
        while remaining:
            candidate = [*row, remaining[0]]
            if worst_ratio([value for _, value in candidate], side) > worst_ratio(
                [value for _, value in row], side
            ):
                break
            row.append(remaining.pop(0))
        laid_out, current = layout_row(row, current)
        output.extend(laid_out)
    return output


def totals(components: Sequence[Component]) -> dict[str, int]:
    return {
        "code": sum(component.total for component in components),
        STATUS_C: sum(component.status_bytes(STATUS_C) for component in components),
        STATUS_ASM: sum(
            component.status_bytes(STATUS_ASM) for component in components
        ),
        STATUS_REMAINING: sum(
            component.status_bytes(STATUS_REMAINING) for component in components
        ),
    }


def progress_document(
    components: Sequence[Component], arm7_bytes: int, version: str
) -> dict[str, object]:
    summary = totals(components)
    return {
        "schema_version": 1,
        "version": version,
        "scope": (
            "DSD ranges marked as ARM9 code; BSS, assets, and ARM7 mixed images "
            "are excluded"
        ),
        "totals": {
            "mapped_code_bytes": summary["code"],
            "matching_c_bytes": summary[STATUS_C],
            "symbolic_asm_bytes": summary[STATUS_ASM],
            "remaining_bytes": summary[STATUS_REMAINING],
            "matching_c_percent": round(
                summary[STATUS_C] * 100 / summary["code"], 4
            ),
            "source_coverage_percent": round(
                (summary[STATUS_C] + summary[STATUS_ASM])
                * 100
                / summary["code"],
                4,
            ),
            "arm7_symbolic_bytes": arm7_bytes,
        },
        "components": [
            {
                "id": component.key,
                "label": component.label,
                "mapped_code_bytes": component.total,
                "matching_c_bytes": component.status_bytes(STATUS_C),
                "symbolic_asm_bytes": component.status_bytes(STATUS_ASM),
                "remaining_bytes": component.status_bytes(STATUS_REMAINING),
                "function_symbols": len(component.functions),
            }
            for component in components
        ],
    }


def svg_text(
    x: float,
    y: float,
    text: str,
    css_class: str,
    anchor: str | None = None,
) -> str:
    anchor_attr = f' text-anchor="{anchor}"' if anchor else ""
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" class="{css_class}"{anchor_attr}>'
        f"{escape(text)}</text>"
    )


def render_svg(components: Sequence[Component], arm7_bytes: int) -> str:
    width = 1200
    margin = 28
    tree = Rect(margin, 224, width - 2 * margin, 474)
    summary = totals(components)
    code = summary["code"]
    c_bytes = summary[STATUS_C]
    asm_bytes = summary[STATUS_ASM]
    c_percent = c_bytes * 100 / code
    source_percent = (c_bytes + asm_bytes) * 100 / code
    bar_x, bar_y, bar_w, bar_h = margin, 130, width - 2 * margin, 22

    lines = [
        '<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="760" viewBox="0 0 1200 760" role="img" aria-labelledby="title desc">',
        '<title id="title">Mario &amp; Luigi: Partners in Time decompilation progress</title>',
        (
            '<desc id="desc">A treemap of mapped ARM9 code. '
            f"{c_percent:.2f} percent is matching C/C++ and {source_percent:.2f} "
            "percent has matching C/C++ or maintained symbolic assembly.</desc>"
        ),
        "<style>",
        ".bg{fill:#0d1117}.title{fill:#f0f6fc;font:700 27px system-ui,sans-serif}",
        ".eyebrow{fill:#8b949e;font:600 12px system-ui,sans-serif;letter-spacing:1.5px}",
        ".percent{fill:#f0f6fc;font:800 38px system-ui,sans-serif}.subtitle{fill:#b1bac4;font:14px system-ui,sans-serif}",
        ".legend{fill:#c9d1d9;font:12px system-ui,sans-serif}.module{fill:#f0f6fc;font:700 12px system-ui,sans-serif}",
        ".module-small{fill:#f0f6fc;font:700 9px system-ui,sans-serif}",
        ".cell{stroke:#0d1117;stroke-width:.65}.component{fill:none;stroke:#0d1117;stroke-width:3}",
        ".footer{fill:#8b949e;font:11px system-ui,sans-serif}",
        "</style>",
        '<rect class="bg" width="1200" height="760" rx="12"/>',
        svg_text(margin, 34, "DECOMPILATION PROGRESS · EUROPEAN ROM", "eyebrow"),
        svg_text(margin, 68, "Mario & Luigi: Partners in Time", "title"),
        svg_text(margin, 113, f"{c_percent:.1f}%", "percent"),
        svg_text(145, 110, "matching C/C++", "subtitle"),
        svg_text(
            width - margin,
            63,
            f"{source_percent:.2f}% source-understood",
            "subtitle",
            "end",
        ),
        svg_text(
            width - margin,
            88,
            f"{human_bytes(code)} mapped ARM9 code",
            "subtitle",
            "end",
        ),
        f'<rect x="{bar_x}" y="{bar_y}" width="{bar_w}" height="{bar_h}" rx="5" fill="{COLORS[STATUS_REMAINING]}"/>',
        f'<rect x="{bar_x}" y="{bar_y}" width="{bar_w * c_bytes / code:.3f}" height="{bar_h}" rx="5" fill="{COLORS[STATUS_C]}"/>',
        f'<rect x="{bar_x + bar_w * c_bytes / code:.3f}" y="{bar_y}" width="{bar_w * asm_bytes / code:.3f}" height="{bar_h}" fill="{COLORS[STATUS_ASM]}"/>',
    ]

    legend_y = 181
    legend = [
        (STATUS_C, f"Matching C/C++  {human_bytes(c_bytes)}"),
        (STATUS_ASM, f"Symbolic ASM  {human_bytes(asm_bytes)}"),
        (
            STATUS_REMAINING,
            f"Remaining  {human_bytes(summary[STATUS_REMAINING])}",
        ),
    ]
    legend_x = margin
    for status, label in legend:
        lines.append(
            f'<rect x="{legend_x}" y="{legend_y - 11}" width="12" height="12" rx="2" fill="{COLORS[status]}"/>'
        )
        lines.append(svg_text(legend_x + 19, legend_y, label, "legend"))
        legend_x += 220
    lines.append(
        svg_text(
            width - margin,
            legend_y,
            "Rectangle area = code bytes",
            "legend",
            "end",
        )
    )

    component_layout = squarify(
        [(component, component.total) for component in components], tree
    )
    for component_obj, component_rect in component_layout:
        component = component_obj
        assert isinstance(component, Component)
        inset = 2.0
        rect = Rect(
            component_rect.x + inset,
            component_rect.y + inset,
            max(0, component_rect.width - 2 * inset),
            max(0, component_rect.height - 2 * inset),
        )
        for unit_obj, unit_rect in squarify(
            [(unit, unit.size) for unit in component.units], rect
        ):
            unit = unit_obj
            assert isinstance(unit, Unit)
            status_label = {
                STATUS_C: "matching C/C++",
                STATUS_ASM: "symbolic ASM",
                STATUS_REMAINING: "remaining",
            }[unit.status]
            tooltip = (
                f"{component.label} · {unit.label} · {human_bytes(unit.size)} · "
                f"{status_label}"
            )
            lines.append(
                f'<rect class="cell" x="{unit_rect.x:.2f}" y="{unit_rect.y:.2f}" '
                f'width="{max(0, unit_rect.width):.2f}" height="{max(0, unit_rect.height):.2f}" '
                f'fill="{COLORS[unit.status]}"><title>{escape(tooltip)}</title></rect>'
            )
        lines.append(
            f'<rect class="component" x="{component_rect.x:.2f}" y="{component_rect.y:.2f}" '
            f'width="{component_rect.width:.2f}" height="{component_rect.height:.2f}"/>'
        )
        if component_rect.width >= 62 and component_rect.height >= 28:
            css_class = "module" if component_rect.width >= 105 else "module-small"
            lines.append(
                svg_text(
                    component_rect.x + 7,
                    component_rect.y + (18 if css_class == "module" else 13),
                    component.label,
                    css_class,
                )
            )

    lines.extend(
        [
            svg_text(
                margin,
                721,
                f"ARM7: {human_bytes(arm7_bytes)} symbolic source; excluded until its mixed code/data images have a safe code denominator.",
                "footer",
            ),
            svg_text(
                margin,
                742,
                "Generated from DSD delinks/symbols and reasm/eur/patches.json · python tools/generate_progress.py",
                "footer",
            ),
            "</svg>",
            "",
        ]
    )
    return "\n".join(lines)


def render_json(
    components: Sequence[Component], arm7_bytes: int, version: str
) -> str:
    return json.dumps(
        progress_document(components, arm7_bytes, version), indent=2
    ) + "\n"


def update_file(path: Path, content: str, check: bool) -> bool:
    current = path.read_text(encoding="utf-8") if path.is_file() else None
    if current == content:
        return True
    if check:
        print(f"stale progress artifact: {path.relative_to(ROOT)}", file=sys.stderr)
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")
    print(f"wrote {path.relative_to(ROOT)}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default="eur")
    parser.add_argument("--svg", type=Path, default=DEFAULT_SVG)
    parser.add_argument("--json", type=Path, default=DEFAULT_JSON)
    parser.add_argument(
        "--check", action="store_true", help="fail instead of writing stale artifacts"
    )
    args = parser.parse_args()

    components, arm7_bytes = collect_progress(args.version)
    ok_svg = update_file(args.svg, render_svg(components, arm7_bytes), args.check)
    ok_json = update_file(
        args.json, render_json(components, arm7_bytes, args.version), args.check
    )
    summary = totals(components)
    print(
        f"matching C/C++: {summary[STATUS_C]}/{summary['code']} bytes "
        f"({summary[STATUS_C] * 100 / summary['code']:.2f}%); "
        f"C/C++ + ASM: {(summary[STATUS_C] + summary[STATUS_ASM]) * 100 / summary['code']:.2f}%"
    )
    return 0 if ok_svg and ok_json else 1


if __name__ == "__main__":
    raise SystemExit(main())
