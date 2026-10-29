#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Tests for tools/ci_status.py on synthetic run lists. Run with: python3 -I tools/tests/test_ci_status.py"""

import importlib.util
import unittest
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("ci_status", Path(__file__).resolve().parents[1] / "ci_status.py")
ci_status = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ci_status)


def run(name, conclusion, sha="abcdef1234", status="completed"):
    return {"name": name, "status": status, "conclusion": conclusion, "head_sha": sha,
            "created_at": "2026-10-29T10:00:00Z", "html_url": "https://example.invalid/run"}


class SummaryTests(unittest.TestCase):
    def test_latest_failure_is_flagged_per_workflow(self):
        runs = [run("linux", "failure"), run("progress", "success"), run("linux", "success")]
        lines, failed = ci_status.summarize(runs, 3)
        self.assertEqual(failed, ["linux"])
        self.assertEqual(len(lines), 3)

    def test_older_failure_and_running_runs_are_not_flagged(self):
        runs = [run("progress", None, status="in_progress"), run("progress", "failure")]
        self.assertEqual(ci_status.summarize(runs, 3)[1], [])
        runs = [run("progress", "success"), run("progress", "failure")]
        self.assertEqual(ci_status.summarize(runs, 3)[1], [])

    def test_limit_per_workflow(self):
        runs = [run("progress", "success") for _ in range(5)] + [run("linux", "success")]
        lines, _ = ci_status.summarize(runs, 2)
        self.assertEqual(len(lines), 3)


if __name__ == "__main__":
    unittest.main()
