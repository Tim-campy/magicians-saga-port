#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check that relative links in the Markdown files outside docs/archive/ point to existing files.

  python -I tools/check_links.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINK = re.compile(r"\]\(([^)\s]+)\)")
SKIP = ("docs/archive/", ".claude/", "build/", "external/", "extracted/", "research/", "game/", "ghidra/")


def main():
    broken = []
    for path in sorted(ROOT.rglob("*.md")):
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith(SKIP):
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for target in LINK.findall(line):
                if target.startswith(("http://", "https://", "mailto:", "#")):
                    continue
                if not (path.parent / target.split("#")[0]).exists():
                    broken.append(f"{rel}:{number}: {target}")
    for item in broken:
        print(item)
    print(f"check_links: {len(broken)} broken link(s)")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main())
