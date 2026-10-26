#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Tests for tools/progress.py. Run with: python3 -I tools/tests/test_progress.py"""

import contextlib
import importlib.util
import io
import json
import tempfile
import unittest
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("re_progress", Path(__file__).resolve().parents[1] / "progress.py")
progress = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(progress)

FUNCTIONS = """id,name,group,status,size
0x08804000,FUN_08804000,container,,0x40
0x08804040,read_archive_header,container,,0x80
0x08804100,decompress_block,container,documented,0x100
0x08810000,card_db_lookup,card_db,reimplemented,64
0x08810040,card_db_get_name,card_db,verified,32
0x08820000,duel_next_phase,duel_core,traced,0x20
0x08820020,FUN_08820020,duel_core,,0x20
"""
FORMATS = """id,name,group,status
fmt_iso,iso_layout,container,documented
fmt_card,card_record,card_db,traced
fmt_pack,pack_table,card_db,
"""


def make_root(functions=FUNCTIONS, formats=FORMATS):
    directory = tempfile.TemporaryDirectory()
    registry = Path(directory.name) / "re"
    registry.mkdir()
    (registry / "functions.csv").write_text(functions, encoding="utf-8")
    (registry / "formats.csv").write_text(formats, encoding="utf-8")
    return directory


def run(*argv):
    out, err = io.StringIO(), io.StringIO()
    code = 0
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        try:
            code = progress.main(list(argv))
        except SystemExit as exit_:
            code = exit_.code
    return code, out.getvalue(), err.getvalue()


class RegistryTests(unittest.TestCase):
    def test_blank_status_is_unknown(self):
        with make_root() as root:
            rows = {r["id"]: r for r in progress.load_registry(Path(root) / "re" / "functions.csv")}
        self.assertEqual(rows["0x08804000"]["status"], "unknown")
        self.assertEqual(rows["0x08804040"]["status"], "unknown")
        self.assertEqual(rows["0x08820020"]["status"], "unknown")

    def test_sizes_accept_decimal_and_hex(self):
        with make_root() as root:
            rows = {r["id"]: r for r in progress.load_registry(Path(root) / "re" / "functions.csv")}
        self.assertEqual(rows["0x08804000"]["size"], 64)
        self.assertEqual(rows["0x08810000"]["size"], 64)

    def test_missing_registry_is_empty(self):
        self.assertEqual(progress.load_registry(Path("/nonexistent/functions.csv")), [])

    def test_rejects_bad_input(self):
        cases = {
            "duplicate": "id,name\n0x1,a\n0X1,b\n",
            "status": "id,name,status\n0x1,a,done\n",
            "size": "id,name,size\n0x1,a,big\n",
            "columns": "address,name\n0x1,a\n",
            "empty": "id,name\n,a\n",
        }
        for label, content in cases.items():
            with self.subTest(label), make_root(content) as root:
                with self.assertRaises(ValueError):
                    progress.load_registry(Path(root) / "re" / "functions.csv")


class SnapshotTests(unittest.TestCase):
    def collect(self, stage="documented"):
        with make_root() as root:
            return progress.collect(Path(root) / "re" / "functions.csv", stage)

    def test_counts_at_the_default_stage(self):
        data = self.collect()
        self.assertEqual((data["done"], data["total"]), (3, 7))
        self.assertEqual(data["percent"], 42.86)
        groups = {g["name"]: g for g in data["groups"]}
        self.assertEqual((groups["container"]["done"], groups["container"]["todo"]), (1, 2))
        self.assertEqual((groups["card_db"]["done"], groups["card_db"]["todo"]), (2, 0))

    def test_stage_changes_the_headline(self):
        self.assertEqual(self.collect("traced")["done"], 4)
        self.assertEqual(self.collect("verified")["done"], 1)

    def test_bytes_are_weighted(self):
        data = self.collect()
        self.assertEqual(data["bytes_total"], 0x40 + 0x80 + 0x100 + 64 + 32 + 0x20 + 0x20)
        self.assertEqual(data["bytes_done"], 0x100 + 64 + 32)

    def test_reached_is_cumulative(self):
        reached = self.collect()["reached"]
        self.assertGreaterEqual(reached["traced"], reached["documented"])
        self.assertGreaterEqual(reached["documented"], reached["reimplemented"])
        self.assertGreaterEqual(reached["reimplemented"], reached["verified"])

    def test_select_groups(self):
        data = progress.select_groups(self.collect(), ["card_db"])
        self.assertEqual((data["done"], data["total"]), (2, 2))
        with self.assertRaises(ValueError):
            progress.select_groups(self.collect(), ["nope"])

    def test_snapshot_round_trips_through_validation(self):
        snapshot = {"functions": self.collect(), "formats": progress.summarize([], "documented")}
        progress.validate_snapshot(json.loads(json.dumps(snapshot)))
        snapshot["functions"]["groups"][0]["items"][0][2] = 99
        with self.assertRaises(ValueError):
            progress.validate_snapshot(snapshot)


class OutputTests(unittest.TestCase):
    def test_site_files_are_written(self):
        with make_root() as root, tempfile.TemporaryDirectory() as out:
            code, stdout, _ = run("--root", root, out)
            self.assertEqual(code, 0)
            self.assertIn("functions 3/7 (42.86%)", stdout)
            names = {p.name for p in Path(out).iterdir()}
            self.assertEqual(names, {"progress.json", "progress.svg", "progress.html", "badge-functions.svg", "badge-formats.svg"})
            svg = (Path(out) / "progress.svg").read_text(encoding="utf-8")
            self.assertTrue(svg.startswith("<svg"))
            self.assertIn("verified", svg)
            self.assertIn("container", svg)

    def test_empty_project_renders(self):
        with make_root("id,name\n", "id,name\n") as root, tempfile.TemporaryDirectory() as out:
            code, stdout, _ = run("--root", root, out)
            self.assertEqual(code, 0)
            self.assertIn("functions 0/0 (0%)", stdout)

    def test_check_and_errors(self):
        with make_root() as root:
            code, stdout, _ = run("--root", root, "--check")
            self.assertEqual((code, stdout.strip()), (0, "ok: 7 functions, 3 formats"))
        with make_root("id,name,status\n0x1,a,bogus\n") as root:
            code, _, stderr = run("--root", root, "--check")
            self.assertEqual(code, 2)
            self.assertIn("unknown status", stderr)

    def test_json_and_group_filter(self):
        with make_root() as root:
            code, stdout, _ = run("--root", root, "--json", "--group", "card_db")
        self.assertEqual(code, 0)
        data = json.loads(stdout)
        self.assertEqual(data["functions"]["total"], 2)
        self.assertEqual(data["formats"]["total"], 3)

    def test_log_keeps_one_row_per_day(self):
        with make_root() as root:
            for _ in range(2):
                self.assertEqual(run("--root", root, "--log")[0], 0)
            lines = (Path(root) / "re" / "progress-log.csv").read_text(encoding="utf-8").splitlines()
            self.assertEqual(lines[0], progress.LOG_HEADER)
            self.assertEqual(len(lines), 2)
            self.assertEqual(run("--root", root, "--log", "--group", "card_db")[0], 2)


class CompareTests(unittest.TestCase):
    def snapshot(self, functions):
        with make_root(functions) as root:
            return {"functions": progress.collect(Path(root) / "re" / "functions.csv", "documented"),
                    "formats": progress.summarize([], "documented")}

    def test_report_lists_progress_and_regressions(self):
        base = self.snapshot("id,name,group,status\n0x1,FUN_1,a,\n0x2,known,a,documented\n0x3,gone,a,traced\n")
        head = self.snapshot("id,name,group,status\n0x1,init_cards,a,documented\n0x2,known,a,traced\n0x4,FUN_4,b,\n")
        text = progress.report(base, head)
        self.assertIn("1 advanced", text)
        self.assertIn("unknown -> documented", text)
        self.assertIn("documented -> traced", text)
        self.assertIn("1 added to the registry", text)
        self.assertIn("1 removed", text)

    def test_no_changes_means_no_report(self):
        snapshot = self.snapshot(FUNCTIONS)
        self.assertEqual(progress.report(snapshot, snapshot), "")

    def test_cli_compare_requires_matching_stages(self):
        with tempfile.TemporaryDirectory() as out, make_root() as root:
            first, second = Path(out) / "a.json", Path(out) / "b.json"
            run("--root", root, str(Path(out) / "a"))
            run("--root", root, "--stage", "traced", str(Path(out) / "b"))
            code, _, stderr = run("--compare", str(Path(out) / "a" / "progress.json"), str(Path(out) / "b" / "progress.json"))
            self.assertEqual(code, 2)
            self.assertIn("same --stage", stderr)
            del first, second


if __name__ == "__main__":
    unittest.main()
