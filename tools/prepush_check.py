#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Audit the commits about to be pushed for game content. Run before every `git push`.

Checks every file changed between the remote branch and HEAD (or every
tracked file with --all): no game binaries or extracted data, no file in
a local-only directory, no file over 1 MiB, and a clean registry check.

  python -I tools/prepush_check.py            # against origin/main
  python -I tools/prepush_check.py --all
"""

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOCAL_DIRS = ("game/", "extracted/", "research/", "ghidra/", "external/", "build/")
FORBIDDEN = (".apk", ".apks", ".xapk", ".obb", ".so", ".dat", ".dll", ".exe", ".png", ".jpg", ".wav",
             ".fsb", ".ogg", ".ttf", ".otf", ".unity3d", ".assets", ".smali", ".dex", ".gpr", ".sav")
MAX_BYTES = 1 << 20


def git(*args):
    return subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True, check=True).stdout


def changed_files(base, everything):
    if everything:
        return git("ls-files").split()
    try:
        git("rev-parse", "--verify", base)
    except subprocess.CalledProcessError:
        print(f"prepush: {base} not found, checking every tracked file")
        return git("ls-files").split()
    return git("diff", "--name-only", "--diff-filter=AMR", f"{base}...HEAD").split()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base", default="origin/main")
    parser.add_argument("--all", action="store_true", help="check every tracked file")
    args = parser.parse_args(argv)
    problems = []
    for name in changed_files(args.base, args.all):
        path = ROOT / name
        lower = name.lower()
        if lower.startswith(LOCAL_DIRS):
            problems.append(f"{name}: local-only directory")
        if lower.endswith(FORBIDDEN):
            problems.append(f"{name}: game binary or asset type")
        if path.is_file() and path.stat().st_size > MAX_BYTES:
            problems.append(f"{name}: {path.stat().st_size} bytes (over 1 MiB)")
    check = subprocess.run([sys.executable, "-I", str(ROOT / "tools" / "registry.py"), "check"],
                           capture_output=True, text=True)
    if check.returncode:
        problems.append((check.stdout + check.stderr).strip())
    if problems:
        print("prepush: do not push\n  " + "\n  ".join(problems))
        return 1
    print("prepush: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
