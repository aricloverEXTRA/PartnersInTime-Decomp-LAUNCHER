from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from apply_linker_aliases import apply_aliases


@pytest.fixture
def mapping(tmp_path):
    (tmp_path / "symbols.txt").write_text(
        "Owner kind:function(arm,size=0x40) addr:0x02001000\n"
        "Interior kind:label(arm) addr:0x02001018\n"
        "Outside kind:label(arm) addr:0x02001040\n"
        "Unaligned kind:label(arm) addr:0x02001019\n"
        "Function kind:function(arm,size=0x4) addr:0x02001020\n"
    )
    return tmp_path, {"source": "src/example.cpp", "symbols": "symbols.txt",
                      "owner": "Owner", "labels": ["Interior"]}


def test_keeps_existing_layout_and_uses_function_relative_address(mapping):
    root, entry = mapping
    script = "SECTIONS {\n    example.o(.text)\n    unrelated.o(.text)\n}\n"
    result = apply_aliases(script, [entry], root)
    assignment = "    Interior = Owner + 0x18;\n"
    assert result.replace(assignment, "") == script
    assert result.index(assignment) < result.index("unrelated.o")


@pytest.mark.parametrize("label", ["Outside", "Unaligned", "Function"])
def test_rejects_entries_that_are_not_interior_labels(mapping, label):
    root, entry = mapping
    entry["labels"] = [label]
    with pytest.raises(ValueError, match="interior ARM label"):
        apply_aliases("    example.o(.text)", [entry], root)


@pytest.mark.parametrize("script", ["", "    example.o(.text)\n    example.o(.text)"])
def test_rejects_missing_or_ambiguous_owner_placement(mapping, script):
    root, entry = mapping
    with pytest.raises(ValueError, match="one text placement"):
        apply_aliases(script, [entry], root)


def test_rejects_duplicate_symbol_definitions(mapping):
    root, entry = mapping
    with pytest.raises(ValueError, match="already defined"):
        apply_aliases("    example.o(.text)\n    Interior = 0;", [entry], root)


def test_link_time_constants_do_not_allocate_or_move_sections(tmp_path):
    entry = {"source": "src/heap.c", "constants": {"ArenaLow": "0x020cbfe0", "StackSize": "0x0"}}
    script = "SECTIONS {\n    heap.o(.text)\n    next.o(.text)\n}\n"
    result = apply_aliases(script, [entry], tmp_path)
    assignments = "    ArenaLow = 0x020cbfe0;\n    StackSize = 0x0;\n"
    assert result.replace(assignments, "") == script
    assert result.index(assignments) < result.index("next.o")
    with pytest.raises(ValueError, match="already defined"):
        apply_aliases(result, [entry], tmp_path)
    with pytest.raises(ValueError, match="one text placement"):
        apply_aliases("    unrelated.o(.text)", [entry], tmp_path)


@pytest.mark.parametrize("name,value", [
    ("Bad;Name", "0x0"), ("Value", "0x100000000"),
    ("Value", "0x1; injected = 0"), ("Value", -1),
])
def test_rejects_invalid_link_time_constants(tmp_path, name, value):
    entry = {"source": "src/heap.c", "constants": {name: value}}
    with pytest.raises(ValueError, match="Invalid"):
        apply_aliases("    heap.o(.text)", [entry], tmp_path)
