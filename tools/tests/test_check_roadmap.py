#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Tests for tools/check_roadmap.py. Run with: python3 -I tools/tests/test_check_roadmap.py"""

import importlib.util
import unittest
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("check_roadmap", Path(__file__).resolve().parents[1] / "check_roadmap.py")
check_roadmap = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_roadmap)

GOOD = """# Roadmap
## Slices
### S1 — Mission (current)
Host: `saga_mission`.
- [ ] Wire the human passes into the host.
### S2 — Town
Host: `saga_town`.
- [ ] Find the town owner.
## Backlog
- [ ] Something later.
"""


class RoadmapTests(unittest.TestCase):
    def test_good_roadmap_passes(self):
        self.assertEqual(check_roadmap.check(GOOD), [])

    def test_long_task_fails(self):
        text = GOOD.replace("Something later.", "x" * 400)
        self.assertTrue(any("characters" in p for p in check_roadmap.check(text)))

    def test_slice_without_host_fails(self):
        text = GOOD.replace("Host: `saga_town`.\n", "")
        self.assertTrue(any("S2 has no" in p for p in check_roadmap.check(text)))

    def test_exactly_one_current_slice(self):
        self.assertTrue(check_roadmap.check(GOOD.replace(" (current)", "")))
        self.assertTrue(check_roadmap.check(GOOD.replace("S2 — Town", "S2 — Town (current)")))


if __name__ == "__main__":
    unittest.main()
