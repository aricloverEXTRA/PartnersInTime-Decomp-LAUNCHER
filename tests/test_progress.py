from __future__ import annotations

import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import generate_progress as progress


class ProgressTrackerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.components, cls.arm7_bytes = progress.collect_progress("eur")

    def test_statuses_partition_mapped_code(self) -> None:
        summary = progress.totals(self.components)
        self.assertEqual(
            summary["code"],
            summary[progress.STATUS_C]
            + summary[progress.STATUS_ASM]
            + summary[progress.STATUS_REMAINING],
        )

    def test_matching_c_is_derived_from_linked_source_units(self) -> None:
        linked_sources = progress.parse_linked_sources("eur")
        c_ranges = [
            item for component in self.components for item in component.c_ranges
        ]
        self.assertTrue(c_ranges)
        self.assertTrue(all((ROOT / item.source).is_file() for item in c_ranges))
        self.assertTrue(all(item.source in linked_sources for item in c_ranges))
        self.assertEqual(
            sum(item.size for item in c_ranges),
            progress.totals(self.components)[progress.STATUS_C],
        )

    def test_c_takes_precedence_over_symbolic_assembly(self) -> None:
        for component in self.components:
            for unit in component.units:
                if unit.status != progress.STATUS_ASM:
                    continue
                self.assertFalse(
                    any(
                        c_range.start < unit.end and unit.start < c_range.end
                        for c_range in component.c_ranges
                    )
                )

    def test_outputs_are_current(self) -> None:
        self.assertEqual(
            progress.DEFAULT_SVG.read_text(encoding="utf-8"),
            progress.render_svg(self.components, self.arm7_bytes),
        )
        self.assertEqual(
            progress.DEFAULT_JSON.read_text(encoding="utf-8"),
            progress.render_json(self.components, self.arm7_bytes, "eur"),
        )


if __name__ == "__main__":
    unittest.main()


def test_linked_init_code_counts_but_constructor_data_and_drafts_do_not(tmp_path, monkeypatch):
    monkeypatch.setattr(progress, "ROOT", tmp_path)
    for name in ("startup.cpp", "draft.cpp"):
        (tmp_path / name).write_text("/* fixture */\n")
    delinks = tmp_path / "delinks.txt"
    delinks.write_text(
        "    .text start:0x1000 end:0x1040 kind:code\n"
        "    .init start:0x2000 end:0x2040 kind:code\n"
        "    .ctor start:0x3000 end:0x3040 kind:data\n"
        "\nstartup.cpp:\n"
        "    .text start:0x1000 end:0x1010\n"
        "    .init start:0x2000 end:0x2010\n"
        "    .ctor start:0x3000 end:0x3010\n"
        "\ndraft.cpp:\n"
        "    .init start:0x2010 end:0x2020\n"
    )
    code, covered, _asm = progress.parse_delinks(delinks, {"startup.cpp"})
    assert code == [progress.Range(0x1000, 0x1040), progress.Range(0x2000, 0x2040)]
    assert [(item.start, item.end) for item in covered] == [(0x1000, 0x1010), (0x2000, 0x2010)]
