#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build, update, check and search the function registry (re/functions.csv).

The registry lists every game method that has native code in libil2cpp.so:
Assembly-CSharp.dll (id prefix `game:`) and KairoLibrary.dll (`kairo:`).
Unity, mscorlib and third-party assemblies are left out, like a console
SDK. Ids are `<prefix>:<metadata token>`, which stay stable when addresses
are shared by several methods.

Two copies exist. research/functions-full.csv (local, gitignored) carries
every original method name from the game's metadata. re/functions.csv
(committed, read by the progress meter) carries the original name only for
methods that have a status, i.e. that the docs already discuss; the others
show their id as name. Always edit statuses, never the CSVs.

Statuses live in re/status/*.tsv, one file per subsystem
(`id<TAB>status<TAB>evidence<TAB>note`), and `apply` copies them into both
CSVs. Groups come from namespace rules below, overridden by re/groups.csv
(`type_prefix,group`; the longest matching prefix wins).

  python -I tools/registry.py build research/il2cpp-code-map.json
  python -I tools/registry.py apply
  python -I tools/registry.py check
  python -I tools/registry.py lookup Combat
  python -I tools/registry.py lookup 0x117c88c
"""

import argparse
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FULL = Path("research") / "functions-full.csv"
PUBLIC = Path("re") / "functions.csv"
IMAGES = {"Assembly-CSharp.dll": "game", "KairoLibrary.dll": "kairo"}
STATUSES = ("unknown", "traced", "documented", "reimplemented", "verified")
COLUMNS = ("id", "name", "group", "status", "size", "address")

# Namespace prefix -> group. The longest matching prefix wins; the global namespace is "unassigned".
NAMESPACE_GROUPS = {
    "main": "boot",
    "form": "ui", "system.form": "ui", "surface": "ui", "system.surface": "ui", "panel": "ui",
    "system.panel": "ui", "game.popupBar": "ui", "game.slideBar": "ui", "kairo.unity.ui": "ui",
    "kairo.unity.form": "ui", "kairo.unity.surface": "ui", "kairo.unity.panel": "ui", "kfw.panel": "ui",
    "ecs": "ecs", "kairo.unity.ecs": "ecs", "kairo.unity.component": "ecs",
    "game": "gameplay", "parameter": "gameplay",
    "ai": "ai", "kairo.unity.ai": "ai",
    "kairo.unity.map": "world",
    "data": "data", "resource": "data", "cfg": "data", "kairo.common.cfg": "data",
    "kairo.unity.io": "io",
    "kairo.unity.graphics": "graphics", "kairo.unity.graph": "graphics", "kairo.unity.m3d4": "graphics",
    "kfw.bsp": "graphics",
    "native": "platform", "java": "platform", "kairo.unity.native": "platform", "kairo.unity.mono": "platform",
    "kairo.unity.plugin": "platform", "kairo.unity.iOSsupport": "platform", "kairo.unity.system": "platform",
    "system": "platform",
    "system.billing": "services", "system.tapjoy": "services", "system.install": "services",
    "system.friend": "services", "system.lobi": "services", "system.notification": "services",
    "system.lineup": "services", "news": "services", "analytics": "services", "TapjoyUnity": "services",
    "kairo.unity.ad": "services", "kairo.unity.analytics": "services", "kairo.unity.licensing": "services",
    "kairo.unity.system.payment": "services", "kairo.unity.system.social": "services",
    "kairo.unity.system.lobi": "services", "kairo.unity.system.notification": "services",
    "kairo.unity.system.lineup": "services",
    "ext.util": "util", "util": "util", "kairo.unity.util": "util", "kairo.unity.math": "util",
    "kairo.unity.collections": "util", "kairo.unity.type": "util", "kairo.unity.secure": "util",
    "kairo.unity.task": "util", "kairo.unity.reflection": "util", "kairo.unity.attribute": "util",
    "kairo.common.ver": "util", "BuildInfos": "util", "R.Library": "util",
}


def fail(message):
    sys.exit(f"registry: {message}")


def prefix_match(name, table):
    """Longest key of table that equals name or is a dotted prefix of it."""
    best = None
    for key in table:
        if (name == key or name.startswith(key + ".")) and (best is None or len(key) > len(best)):
            best = key
    return best


def namespace_group(namespace):
    key = prefix_match(namespace, NAMESPACE_GROUPS) if namespace else None
    return NAMESPACE_GROUPS[key] if key else "unassigned"


def load_overrides(root):
    path = root / "re" / "groups.csv"
    if not path.is_file():
        return {}
    table = {}
    with path.open(newline="", encoding="utf-8") as handle:
        for number, row in enumerate(csv.DictReader(l for l in handle if not l.startswith("#")), start=2):
            prefix, group = (row.get("type_prefix") or "").strip(), (row.get("group") or "").strip()
            if not prefix or not group:
                fail(f"re/groups.csv line {number}: type_prefix and group are required")
            if prefix in table:
                fail(f"re/groups.csv line {number}: duplicate prefix {prefix}")
            table[prefix] = group
    return table


def load_status(root):
    """Merge re/status/*.tsv into {id: (status, evidence, note, where)}."""
    merged = {}
    for path in sorted((root / "re" / "status").glob("*.tsv")):
        with path.open(newline="", encoding="utf-8") as handle:
            for number, line in enumerate(handle, start=1):
                if not line.strip() or line.startswith("#"):
                    continue
                cells = line.rstrip("\r\n").split("\t")
                if cells[0] == "id":
                    continue
                where = f"{path.relative_to(root).as_posix()} line {number}"
                if len(cells) < 3:
                    fail(f"{where}: need id, status and evidence separated by tabs")
                key, status, evidence = cells[0].strip().lower(), cells[1].strip(), cells[2].strip()
                if status not in STATUSES[1:]:
                    fail(f"{where}: status {status!r} is not one of {', '.join(STATUSES[1:])}")
                if key in merged:
                    fail(f"{where}: {key} is already set in {merged[key][3]}")
                merged[key] = (status, evidence, cells[3].strip() if len(cells) > 3 else "", where)
    return merged


def read_csv(path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def read_full(root):
    path = root / FULL
    if not path.is_file():
        fail(f"{FULL.as_posix()} is missing; run `build` with research/il2cpp-code-map.json first")
    return read_csv(path)


def publish(root, rows):
    """Write both copies; the public one names only the methods that have a status."""
    write_csv(root / FULL, rows)
    write_csv(root / PUBLIC, [dict(row, name=row["name"] if row["status"] else row["id"]) for row in rows])


def assign_groups(rows, overrides):
    for row in rows:
        key = prefix_match(row["name"], overrides)
        if key:
            row["group"] = overrides[key]


def apply_status(root, rows):
    status = load_status(root)
    known = {row["id"] for row in rows}
    missing = [f"{where}: unknown id {key}" for key, (_, _, _, where) in status.items() if key not in known]
    if missing:
        fail("\n  ".join(["status rows that are not in the registry:"] + missing))
    for row in rows:
        row["status"] = status[row["id"]][0] if row["id"] in status else ""
    return status


def method_name(entry):
    owner = f'{entry["namespace"]}.{entry["type_name"]}' if entry["namespace"] else entry["type_name"]
    return f'{owner}.{entry["method_name"]}'


def build(root, code_map):
    data = json.loads(Path(code_map).read_text(encoding="utf-8"))
    rows = []
    for entry in data["mapped_methods"]:
        prefix = IMAGES.get(entry["image_name"])
        if not prefix or entry["status"] != "structurally_mapped":
            continue
        start, end = entry["elf_va"], entry.get("next_distinct_mapped_method_va")
        rows.append({"id": f'{prefix}:{entry["token"].lower()}', "name": method_name(entry),
                     "group": namespace_group(entry["namespace"]), "status": "",
                     "size": str(end - start) if end and end > start else "", "address": hex(start)})
    rows.sort(key=lambda r: (r["id"].split(":")[0] != "game", r["id"]))
    assign_groups(rows, load_overrides(root))
    apply_status(root, rows)
    publish(root, rows)
    print(f"registry: wrote {len(rows)} methods to {FULL.as_posix()} and {PUBLIC.as_posix()}")


def apply(root):
    rows = read_full(root)
    overrides = load_overrides(root)
    unused = [p for p in overrides if not any(r["name"] == p or r["name"].startswith(p + ".") for r in rows)]
    if unused:
        fail("re/groups.csv prefixes that match no method: " + ", ".join(unused))
    assign_groups(rows, overrides)
    status = apply_status(root, rows)
    publish(root, rows)
    print(f"registry: {len(status)} statuses applied to {len(rows)} methods")


def check(root):
    """Validate the committed files only, so it also runs where the local copy is absent (CI)."""
    path = root / PUBLIC
    if not path.is_file():
        fail(f"{PUBLIC.as_posix()} is missing")
    rows = read_csv(path)
    status = load_status(root)
    by_id = {row["id"]: row for row in rows}
    errors = []
    for key, (value, evidence, _, where) in status.items():
        if key not in by_id:
            errors.append(f"{where}: unknown id {key}")
            continue
        if by_id[key]["status"] != value:
            errors.append(f"{where}: {PUBLIC.as_posix()} has {by_id[key]['status'] or 'blank'} for {key}; run `apply`")
        target = evidence.split("#")[0]
        if not target or not (root / target).is_file():
            errors.append(f"{where}: evidence {evidence!r} is not a file in the repository")
        elif value in ("reimplemented", "verified") and not target.startswith(("src/", "tests/", "docs/verification/")):
            errors.append(f"{where}: {value} needs evidence in src/, tests/ or docs/verification/")
    for row in rows:
        if row["status"] and row["id"] not in status:
            errors.append(f"{PUBLIC.as_posix()}: {row['id']} has status {row['status']} but no row in re/status/")
        if not row["status"] and row["name"] != row["id"]:
            errors.append(f"{PUBLIC.as_posix()}: {row['id']} has no status but shows an original name; run `apply`")
    if errors:
        fail("\n  ".join(["check failed:"] + errors[:50] + ([f"... {len(errors) - 50} more"] if len(errors) > 50 else [])))
    print(f"registry: ok ({len(rows)} methods, {len(status)} with a status)")


def lookup(root, query, limit):
    full = root / FULL
    rows = read_csv(full if full.is_file() else root / PUBLIC)
    q = query.lower()
    if q.startswith("0x"):
        target = int(q, 16)
        hits = [r for r in rows if int(r["address"], 16) <= target < int(r["address"], 16) + int(r["size"] or 1)]
    else:
        hits = [r for r in rows if q == r["id"] or q in r["name"].lower()]
    for row in hits[:limit]:
        print("\t".join(row[c] or "-" for c in COLUMNS))
    if len(hits) > limit:
        print(f"... {len(hits) - limit} more (use --limit)")
    if not hits:
        print("no match")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=ROOT, help="repository root")
    sub = parser.add_subparsers(dest="command", required=True)
    b = sub.add_parser("build", help="regenerate both registry copies from the IL2CPP code map (local file)")
    b.add_argument("code_map", help="research/il2cpp-code-map.json written by tools/map_il2cpp_methods.py")
    sub.add_parser("apply", help="copy re/groups.csv and re/status/*.tsv into both registry copies")
    sub.add_parser("check", help="validate re/status/*.tsv against the committed registry")
    l = sub.add_parser("lookup", help="find methods by name substring, id or address (0x...)")
    l.add_argument("query")
    l.add_argument("--limit", type=int, default=40)
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if args.command == "build":
        build(root, args.code_map)
    elif args.command == "apply":
        apply(root)
    elif args.command == "check":
        check(root)
    else:
        lookup(root, args.query, args.limit)


if __name__ == "__main__":
    main()
