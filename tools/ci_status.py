#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Show the latest GitHub Actions runs of this repository (public API, no token needed).

Run it at the start of a session instead of waiting for CI after a push.

  python -I tools/ci_status.py
  python -I tools/ci_status.py --limit 10
"""

import argparse
import json
import sys
import urllib.error
import urllib.request

REPO = "Tim-campy/magicians-saga-port"


def summarize(runs, limit):
    """Latest `limit` runs per workflow as lines; a failed latest run is flagged."""
    lines, seen, failed = [], {}, []
    for run in runs:
        name = run.get("name") or "?"
        count = seen.get(name, 0)
        if count == 0 and run.get("status") == "completed" and run.get("conclusion") not in ("success", "skipped"):
            failed.append(name)
        if count < limit:
            state = run.get("conclusion") or run.get("status")
            lines.append(f"{name:10} {run.get('head_sha', '')[:7]} {state:12} {run.get('created_at', '')}  {run.get('html_url', '')}")
        seen[name] = count + 1
    return lines, failed


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--limit", type=int, default=3, help="runs to show per workflow")
    args = parser.parse_args(argv)
    url = f"https://api.github.com/repos/{REPO}/actions/runs?per_page=30&branch=main"
    try:
        with urllib.request.urlopen(urllib.request.Request(url, headers={"Accept": "application/vnd.github+json"}), timeout=20) as response:
            runs = json.loads(response.read())["workflow_runs"]
    except (urllib.error.URLError, TimeoutError, KeyError, ValueError) as error:
        print(f"ci_status: could not read runs ({error})")
        return 2
    lines, failed = summarize(runs, args.limit)
    print("\n".join(lines) if lines else "no runs")
    if failed:
        print("ci_status: latest run failed for " + ", ".join(failed) + "; fix it first")
        return 1
    print("ci_status: latest runs ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
