#!/usr/bin/env python3
"""Generate a Partners in Time data project from a declarative mod profile.

The profile stores multipliers only. This tool combines them with the user's
own exported data project to produce a buildable data project that the
decompilation's ``build_nds.ps1 -DataProject`` can consume.

No retail bytes are read, written or shipped by this tool: the only game data it
touches is the export the user generated from their own ROM.

Example:
    python tools/make_data_profile.py ^
        --base-project ..\\PiT\\data\\eur ^
        --profile mods\\hard_mode\\profile.json ^
        --output generated\\hard_mode
"""

from __future__ import annotations

import argparse
import json
import sys
from decimal import ROUND_HALF_UP, Decimal
from pathlib import Path
from typing import Any

PROFILE_SCHEMA = "pit-mod-profile-v1"
PROJECT_SCHEMA = "pit-data-project-v1"
ENEMY_SCHEMA = "pit-enemy-stats-v1"
U16_MAX = 0xFFFF

# Fields the data builder ignores when packing; they must survive untouched.
_PRESERVED_RECORD_KEYS = (
    "record_id",
    "name_id",
    "name_hint",
    "flags_or_ai_id",
    "unknown_04",
    "level",
    "traits",
    "unknown_10",
    "unknown_12_hex",
    "item_drop_1",
    "item_drop_2",
)


class ProfileError(Exception):
    """Raised when a profile or base project cannot be used."""


def scale_value(value: int, factor: Decimal, low: int, high: int) -> int:
    """Scale an integer with half-up rounding, then clamp.

    Half-up rounding is used rather than Python's ``round`` because ``round``
    applies banker's rounding, which would send 0.5 cases to even values and
    make the same profile produce different results for equivalent inputs.
    """
    scaled = (Decimal(value) * factor).quantize(Decimal("1"), rounding=ROUND_HALF_UP)
    result = int(scaled)
    if result < low:
        return low
    if result > high:
        return high
    return result


def load_json(path: Path, label: str) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ProfileError(f"{label} not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ProfileError(f"{label} is not valid JSON: {path}: {exc}") from exc


def active_transforms(profile: dict[str, Any]) -> list[dict[str, Any]]:
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ProfileError(
            f"profile schema must be {PROFILE_SCHEMA!r}, got {profile.get('schema')!r}"
        )
    transforms = profile.get("transforms")
    if not isinstance(transforms, list) or not transforms:
        raise ProfileError("profile must define a non-empty 'transforms' list")

    resolved = []
    for index, transform in enumerate(transforms):
        if not isinstance(transform, dict):
            raise ProfileError(f"transform #{index} is not an object")
        field = transform.get("field")
        if not isinstance(field, str) or not field:
            raise ProfileError(f"transform #{index} has no 'field'")
        if "scale" not in transform:
            raise ProfileError(f"transform {field!r} has no 'scale'")
        scale = transform["scale"]
        if not isinstance(scale, (int, float)) or isinstance(scale, bool) or scale <= 0:
            raise ProfileError(f"transform {field!r} needs a positive numeric 'scale'")
        if not transform.get("enabled", True):
            continue
        low = transform.get("min", 0)
        high = transform.get("max", U16_MAX)
        if not isinstance(low, int) or not isinstance(high, int):
            raise ProfileError(f"transform {field!r} needs integer 'min'/'max'")
        if low > high:
            raise ProfileError(f"transform {field!r} has min greater than max")
        resolved.append(
            {
                "field": field,
                "scale": Decimal(str(scale)),
                "min": low,
                "max": min(high, U16_MAX),
            }
        )
    if not resolved:
        raise ProfileError("profile has no enabled transforms")
    return resolved


def apply_transforms(document: dict[str, Any], transforms: list[dict[str, Any]]) -> dict[str, int]:
    if document.get("schema") != ENEMY_SCHEMA:
        raise ProfileError(
            f"base enemy document schema must be {ENEMY_SCHEMA!r}, "
            f"got {document.get('schema')!r}"
        )
    records = document.get("records")
    if not isinstance(records, list) or not records:
        raise ProfileError("base enemy document has no 'records'")

    applied = {transform["field"]: 0 for transform in transforms}
    for record in records:
        if not isinstance(record, dict):
            raise ProfileError("enemy records must be objects")
        for transform in transforms:
            field = transform["field"]
            if field not in record:
                raise ProfileError(
                    f"enemy record {record.get('record_id')} has no {field!r} field"
                )
            original = record[field]
            if not isinstance(original, int) or isinstance(original, bool):
                raise ProfileError(
                    f"enemy record {record.get('record_id')} field {field!r} "
                    "must be an integer"
                )
            updated = scale_value(original, transform["scale"], transform["min"], transform["max"])
            if updated != original:
                applied[field] += 1
            record[field] = updated
    return applied


def build_project_document(version: str, stat_document: str) -> dict[str, Any]:
    return {
        "schema": PROJECT_SCHEMA,
        "version": version,
        "text_documents": [],
        "dialogue_documents": [],
        "script_documents": [],
        "field_script_documents": [],
        "scene_script_documents": [],
        "stat_documents": [stat_document],
        "binary_documents": [],
    }


def summarise(document: dict[str, Any], transforms: list[dict[str, Any]]) -> None:
    records = document["records"]
    print(f"  records: {len(records)}")
    for transform in transforms:
        field = transform["field"]
        values = [record[field] for record in records]
        print(
            f"  {field:<11} x{transform['scale']}  "
            f"min={min(values):<6} max={max(values):<6} "
            f"(clamped range {transform['min']}..{transform['max']})"
        )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate a PiT data project from a mod profile."
    )
    parser.add_argument("--base-project", type=Path, required=True,
                        help="User's own exported data project (contains project.json).")
    parser.add_argument("--profile", type=Path, required=True,
                        help="Mod profile JSON with multipliers only.")
    parser.add_argument("--output", type=Path, required=True,
                        help="Directory to write the generated data project into.")
    args = parser.parse_args(argv)

    try:
        profile = load_json(args.profile, "profile")
        base_project = load_json(args.base_project / "project.json", "base project")

        applies_to = profile.get("applies_to") or {}
        version = applies_to.get("version") or base_project.get("version")
        if not isinstance(version, str) or not version:
            raise ProfileError("could not determine the target version")
        if base_project.get("version") != version:
            raise ProfileError(
                f"profile targets version {version!r} but the base project is "
                f"{base_project.get('version')!r}"
            )

        stat_document = applies_to.get("stat_document")
        if not isinstance(stat_document, str) or not stat_document:
            raise ProfileError("profile 'applies_to' must name a 'stat_document'")

        source_document = load_json(args.base_project / stat_document, "base stat document")
        transforms = active_transforms(profile)
        applied = apply_transforms(source_document, transforms)

        # The document is derived from the user's export, so provenance fields are
        # already correct and must not be recomputed here.
        for key in ("source", "source_sha1", "record_size"):
            if key not in source_document:
                raise ProfileError(f"base stat document is missing {key!r}")

        output_stat = args.output / stat_document
        output_stat.parent.mkdir(parents=True, exist_ok=True)
        output_stat.write_text(
            json.dumps(source_document, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        (args.output / "project.json").write_text(
            json.dumps(build_project_document(version, stat_document), indent=2) + "\n",
            encoding="utf-8",
        )
    except ProfileError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"Generated {args.output} from profile {profile['id']} v{profile['version']}")
    summarise(source_document, transforms)
    print("  changed records per field:")
    for field, count in applied.items():
        print(f"    {field:<11} {count}")
    print(f"  source_sha1 preserved: {source_document['source_sha1']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
