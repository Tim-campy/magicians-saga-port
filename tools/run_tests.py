#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run every Python test (tests/test_*.py and tools/tests/test_*.py) in isolated mode.

  python -I tools/run_tests.py
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    tests = sorted(ROOT.glob("tests/test_*.py")) + sorted(ROOT.glob("tools/tests/test_*.py"))
    failed = []
    for test in tests:
        result = subprocess.run([sys.executable, "-I", str(test)], cwd=ROOT, capture_output=True, text=True)
        name = test.relative_to(ROOT).as_posix()
        print(("PASS " if result.returncode == 0 else "FAIL ") + name)
        if result.returncode:
            failed.append(name)
            print((result.stdout + result.stderr).strip()[-2000:])
    print(f"{len(tests) - len(failed)}/{len(tests)} test files passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
