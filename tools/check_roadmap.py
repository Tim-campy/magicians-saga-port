#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check that docs/ROADMAP.md stays a queue: short tasks, slices with a host, one current slice.

  python -I tools/check_roadmap.py
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MAX_TASK = 300
SLICE = re.compile(r"^### (S\d+) ")


def check(text):
    problems, slices, current = [], {}, []
    slice_name = None
    for number, line in enumerate(text.splitlines(), start=1):
        heading = SLICE.match(line)
        if heading:
            slice_name = heading.group(1)
            slices[slice_name] = False
            if "(current)" in line:
                current.append(slice_name)
        elif line.startswith("## "):
            slice_name = None
        elif slice_name and line.startswith("Host:"):
            slices[slice_name] = True
        if re.match(r"^\s*- \[[ x]\]", line) and len(line) > MAX_TASK:
            problems.append(f"line {number}: task has {len(line)} characters (max {MAX_TASK}); move details to the docs")
    if not slices:
        problems.append("no slices (### S<n> ...) found")
    for name, has_host in slices.items():
        if not has_host:
            problems.append(f"slice {name} has no 'Host:' line")
    if len(current) != 1:
        problems.append(f"exactly one slice must be marked (current), found {len(current)}")
    return problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("path", nargs="?", type=Path, default=ROOT / "docs" / "ROADMAP.md")
    args = parser.parse_args(argv)
    problems = check(args.path.read_text(encoding="utf-8"))
    for problem in problems:
        print(problem)
    print(f"check_roadmap: {'ok' if not problems else str(len(problems)) + ' problem(s)'}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
