#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Tests for tools/registry.py on a synthetic code map. Run with: python3 -I tools/tests/test_registry.py"""

import contextlib
import csv
import importlib.util
import io
import json
import tempfile
import unittest
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("re_registry", Path(__file__).resolve().parents[1] / "registry.py")
registry = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(registry)


def method(image, token, namespace, type_name, name, va, end, status="structurally_mapped"):
    return {"image_name": image, "token": token, "namespace": namespace, "type_name": type_name,
            "method_name": name, "elf_va": va, "next_distinct_mapped_method_va": end, "status": status}


CODE_MAP = {"mapped_methods": [
    method("Assembly-CSharp.dll", "0x06000002", "main", "Main", "OnUpdate", 0x2000, 0x2100),
    method("Assembly-CSharp.dll", "0x06000001", "main", "Main", "OnCreate", 0x1000, 0x1040),
    method("Assembly-CSharp.dll", "0x06000003", "", "QuestResultForm", "Init", 0x3000, 0x3010),
    method("Assembly-CSharp.dll", "0x06000004", "ecs", "World", "Abstract", 0, None, "null_slot_unresolved"),
    method("KairoLibrary.dll", "0x06000001", "kairo.unity.ui.text", "TextLayout", "Draw", 0x4000, 0x4080),
    method("mscorlib.dll", "0x06000001", "System", "Object", "ToString", 0x5000, 0x5010),
]}


class Root:
    def __init__(self):
        self.directory = tempfile.TemporaryDirectory()
        self.path = Path(self.directory.name)
        (self.path / "re" / "status").mkdir(parents=True)
        (self.path / "docs").mkdir()
        (self.path / "docs" / "boot.md").write_text("# Boot\n", encoding="utf-8")
        self.code_map = self.path / "map.json"
        self.code_map.write_text(json.dumps(CODE_MAP), encoding="utf-8")

    def status(self, text):
        (self.path / "re" / "status" / "boot.tsv").write_text("id\tstatus\tevidence\tnote\n" + text, encoding="utf-8")

    def run(self, *args):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            registry.main(["--root", str(self.path), *args])
        return out.getvalue()

    def rows(self, which):
        with (self.path / which).open(newline="", encoding="utf-8") as handle:
            return {r["id"]: r for r in csv.DictReader(handle)}

    def close(self):
        self.directory.cleanup()


class RegistryTests(unittest.TestCase):
    def setUp(self):
        self.root = Root()

    def tearDown(self):
        self.root.close()

    def test_build_keeps_only_game_methods_with_code(self):
        self.root.run("build", str(self.root.code_map))
        rows = self.root.rows("research/functions-full.csv")
        self.assertEqual(list(rows), ["game:0x06000001", "game:0x06000002", "game:0x06000003", "kairo:0x06000001"])
        self.assertEqual(rows["game:0x06000001"]["name"], "main.Main.OnCreate")
        self.assertEqual(rows["game:0x06000001"]["size"], "64")
        self.assertEqual(rows["game:0x06000001"]["address"], "0x1000")
        self.assertEqual(rows["game:0x06000001"]["group"], "boot")
        self.assertEqual(rows["game:0x06000003"]["group"], "unassigned")
        self.assertEqual(rows["kairo:0x06000001"]["group"], "ui")

    def test_public_copy_names_only_methods_with_a_status(self):
        self.root.status("game:0x06000001\ttraced\tdocs/boot.md\tentry point\n")
        self.root.run("build", str(self.root.code_map))
        public = self.root.rows("re/functions.csv")
        self.assertEqual(public["game:0x06000001"]["name"], "main.Main.OnCreate")
        self.assertEqual(public["game:0x06000001"]["status"], "traced")
        self.assertEqual(public["game:0x06000002"]["name"], "game:0x06000002")
        self.assertIn("ok", self.root.run("check"))

    def test_group_overrides_use_the_longest_prefix(self):
        (self.root.path / "re" / "groups.csv").write_text(
            "type_prefix,group\nQuestResultForm,rewards\nkairo.unity.ui.text.TextLayout,text\n", encoding="utf-8")
        self.root.run("build", str(self.root.code_map))
        rows = self.root.rows("research/functions-full.csv")
        self.assertEqual(rows["game:0x06000003"]["group"], "rewards")
        self.assertEqual(rows["kairo:0x06000001"]["group"], "text")

    def test_apply_picks_up_new_statuses_and_check_detects_drift(self):
        self.root.run("build", str(self.root.code_map))
        self.root.status("game:0x06000002\tdocumented\tdocs/boot.md\t\n")
        with self.assertRaises(SystemExit):
            self.root.run("check")
        self.root.run("apply")
        self.assertEqual(self.root.rows("re/functions.csv")["game:0x06000002"]["status"], "documented")
        self.assertIn("ok", self.root.run("check"))

    def test_bad_status_rows_are_rejected(self):
        self.root.run("build", str(self.root.code_map))
        for text in ("game:0x06000001\tnamed\tdocs/boot.md\n",
                     "game:0x0600ffff\ttraced\tdocs/boot.md\n",
                     "game:0x06000001\ttraced\tdocs/boot.md\ngame:0x06000001\ttraced\tdocs/boot.md\n"):
            self.root.status(text)
            with self.assertRaises(SystemExit):
                self.root.run("apply")

    def test_check_requires_existing_and_fitting_evidence(self):
        self.root.run("build", str(self.root.code_map))
        for text in ("game:0x06000001\ttraced\tdocs/missing.md\n",
                     "game:0x06000001\treimplemented\tdocs/boot.md\n"):
            self.root.status(text)
            self.root.run("apply")
            with self.assertRaises(SystemExit):
                self.root.run("check")

    def test_lookup_by_name_and_address(self):
        self.root.run("build", str(self.root.code_map))
        self.assertIn("main.Main.OnUpdate", self.root.run("lookup", "onupdate"))
        self.assertIn("main.Main.OnUpdate", self.root.run("lookup", "0x2080"))
        self.assertIn("no match", self.root.run("lookup", "0x9999"))


if __name__ == "__main__":
    unittest.main()
