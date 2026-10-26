#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
#
# Reverse-engineering and port progress tracker for the Magician's Saga native port.
#
# The snapshot format, the base/head comparison report and the treemap/badge
# rendering are adapted from tools/progress.py of AnyPS5
# (https://github.com/boykopovar/AnyPS5, GPL-2.0). AnyPS5 counts implemented
# system-library functions and shader opcodes inside its own source tree; this
# version instead counts the reverse-engineering status of functions and data
# formats recorded in CSV registries. Keep this notice and the GPL-2.0 license
# when redistributing.
"""Compute reverse-engineering progress from re/functions.csv and re/formats.csv."""

import argparse
import csv
import json
import sys
from datetime import datetime, timezone
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Ordered from least to most advanced. See docs/PROGRESS.md for the definitions.
STATUSES = ("unknown", "traced", "documented", "reimplemented", "verified")
RANK = {status: index for index, status in enumerate(STATUSES)}
COLORS = {"unknown": "#6e7681", "traced": "#8957e5", "documented": "#1f6feb",
          "reimplemented": "#2ea043", "verified": "#e3b341"}
DEFAULT_STAGE = "documented"
REPORT_ROWS = 100
PANEL_WIDTH, GAP, MAP_HEIGHT, HEADER, LEGEND = 495, 10, 280, 30, 30
BORDER, TEXT = "#0d1117", "#ffffff"
LOG_HEADER = "date,stage,functions_done,functions_total,formats_done,formats_total,function_bytes_percent"


# --------------------------------------------------------------------------- registry

def pct(done, total):
    return round(100 * done / total, 2) if total else 0


def parse_size(value, where):
    value = (value or "").strip()
    if not value:
        return 0
    try:
        size = int(value, 0)
    except ValueError:
        raise ValueError(f"{where}: size {value!r} is not a number (use decimal or 0x hex)") from None
    if size < 0:
        raise ValueError(f"{where}: size must not be negative")
    return size


def load_registry(path):
    """Read a registry CSV (id,name,group,status,size). A missing file is an empty registry."""
    if not path.is_file():
        return []
    rows, seen = [], {}
    with path.open(newline="", encoding="utf-8") as handle:
        lines = (line for line in handle if line.strip() and not line.lstrip().startswith("#"))
        reader = csv.DictReader(lines)
        missing = {"id", "name"} - set(reader.fieldnames or [])
        if missing:
            raise ValueError(f"{path.name}: missing column(s): {', '.join(sorted(missing))}")
        for number, raw in enumerate(reader, start=1):
            where = f"{path.name} row {number}"
            key = (raw.get("id") or "").strip().lower()
            name = (raw.get("name") or "").strip()
            if not key or not name:
                raise ValueError(f"{where}: id and name are required")
            if key in seen:
                raise ValueError(f"{where}: duplicate id {key} (first used at row {seen[key]})")
            seen[key] = number
            status = (raw.get("status") or "").strip().lower()
            if not status:
                # IL2CPP metadata gives every method its original name, so a name
                # says nothing about analysis: a blank status is always "unknown".
                status = "unknown"
            if status not in RANK:
                raise ValueError(f"{where}: unknown status {status!r}; use one of {', '.join(STATUSES)}")
            rows.append({"id": key, "name": name, "group": (raw.get("group") or "").strip() or "unassigned",
                         "status": status, "size": parse_size(raw.get("size"), where)})
    return rows


# --------------------------------------------------------------------------- snapshot

def group_entry(name, rows, stage):
    floor = RANK[stage]
    items = sorted(([r["id"], r["name"], RANK[r["status"]], r["size"]] for r in rows), key=lambda i: i[0])
    reached = [i for i in items if i[2] >= floor]
    return {"name": name, "label": name, "done": len(reached), "todo": len(items) - len(reached),
            "bytes_done": sum(i[3] for i in reached), "bytes_total": sum(i[3] for i in items), "items": items}


def summarize(groups, stage):
    done = sum(g["done"] for g in groups)
    total = done + sum(g["todo"] for g in groups)
    bytes_done = sum(g["bytes_done"] for g in groups)
    bytes_total = sum(g["bytes_total"] for g in groups)
    ranks = [i[2] for g in groups for i in g["items"]]
    reached = {s: pct(sum(1 for r in ranks if r >= RANK[s]), total) for s in STATUSES[1:]}
    return {"stage": stage, "done": done, "total": total, "percent": pct(done, total),
            "bytes_done": bytes_done, "bytes_total": bytes_total, "bytes_percent": pct(bytes_done, bytes_total),
            "reached": reached, "groups": groups}


def collect(path, stage):
    groups = {}
    for row in load_registry(path):
        groups.setdefault(row["group"], []).append(row)
    return summarize([group_entry(name, rows, stage) for name, rows in sorted(groups.items())], stage)


def select_groups(data, names):
    known = {g["name"] for g in data["groups"]}
    missing = []
    for name in names:
        if name not in known and name not in missing:
            missing.append(name)
    if missing:
        label = "group" if len(missing) == 1 else "groups"
        raise ValueError(f"unknown {label}: {', '.join(missing)}")
    return summarize([g for g in data["groups"] if g["name"] in set(names)], data["stage"])


def validate_snapshot(snapshot):
    if not isinstance(snapshot, dict):
        raise ValueError("snapshot must be an object")
    for section in ("functions", "formats"):
        data = snapshot.get(section)
        if not isinstance(data, dict):
            raise ValueError(f"{section} must be an object")
        if data.get("stage") not in RANK:
            raise ValueError(f"{section}.stage must be one of {', '.join(STATUSES)}")
        percent = data.get("percent")
        if isinstance(percent, bool) or not isinstance(percent, (int, float)) or not 0 <= percent <= 100:
            raise ValueError(f"{section}.percent must be a number between 0 and 100")
        groups = data.get("groups")
        if not isinstance(groups, list):
            raise ValueError(f"{section}.groups must be a list")
        for index, group in enumerate(groups):
            location = f"{section}.groups[{index}]"
            if not isinstance(group, dict) or not isinstance(group.get("name"), str):
                raise ValueError(f"{location} must be an object with a string name")
            items = group.get("items")
            if not isinstance(items, list):
                raise ValueError(f"{location}.items must be a list")
            for item in items:
                valid = (isinstance(item, list) and len(item) == 4 and isinstance(item[0], str)
                         and isinstance(item[1], str) and isinstance(item[2], int) and not isinstance(item[2], bool)
                         and 0 <= item[2] < len(STATUSES) and isinstance(item[3], int) and item[3] >= 0)
                if not valid:
                    raise ValueError(f"{location}.items must be [id, name, rank, size] entries")


# --------------------------------------------------------------------------- treemap layout
# squarify/place/worst_ratio/cells follow AnyPS5's tools/progress.py.

def worst_ratio(row, side):
    area = sum(row)
    return max(max(side * side * r / area ** 2, area ** 2 / (side * side * r)) for r in row)


def place(row, x, y, w, h, rects):
    thickness = sum(row) / min(w, h)
    offset = 0
    for area in row:
        length = area / thickness
        if w >= h:
            rects.append((x, y + offset, thickness, length))
        else:
            rects.append((x + offset, y, length, thickness))
        offset += length
    return (x + thickness, y, w - thickness, h) if w >= h else (x, y + thickness, w, h - thickness)


def squarify(values, x, y, w, h):
    total = sum(values)
    areas = [v * w * h / total for v in values]
    rects, row = [], []
    while areas:
        side = min(w, h)
        if not row or worst_ratio(row + [areas[0]], side) <= worst_ratio(row, side):
            row.append(areas.pop(0))
            continue
        x, y, w, h = place(row, x, y, w, h, rects)
        row = []
    if row:
        place(row, x, y, w, h, rects)
    return rects


def cells(count, x, y, w, h):
    if not count or w <= 0 or h <= 0:
        return []
    rows = max(1, min(count, round((count * h / w) ** 0.5)))
    result = []
    for r in range(rows):
        in_row = count // rows + (r < count % rows)
        cw = w / in_row
        result += [(x + c * cw, y + r * h / rows, cw, h / rows) for c in range(in_row)]
    return result


# --------------------------------------------------------------------------- rendering

def rect(x, y, w, h, color, stroke=0.5):
    return (f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" '
            f'fill="{color}" stroke="{BORDER}" stroke-width="{stroke}"/>')


def text(x, y, value, size):
    return (f'<text x="{x:.2f}" y="{y:.2f}" font-family="sans-serif" font-size="{size}" fill="{TEXT}" '
            f'stroke="{BORDER}" stroke-width="3" paint-order="stroke">{escape(value)}</text>')


def treemap(title, data, left):
    groups = sorted((g for g in data["groups"] if g["items"]), key=lambda g: -len(g["items"]))
    parts = [text(left + 4, 21, f'{title} (>= {data["stage"]}): {data["percent"]}% ({data["done"]}/{data["total"]})', 16)]
    if not groups:
        return parts
    layout = squarify([len(g["items"]) for g in groups], left, HEADER, PANEL_WIDTH, MAP_HEIGHT)
    for group, (x, y, w, h) in zip(groups, layout):
        total = len(group["items"])
        parts.append(f'<g><title>{escape(group["name"])}: {group["done"]}/{total} '
                     f'({pct(group["done"], total):.0f}% at {data["stage"]})</title>')
        ranks = sorted((i[2] for i in group["items"]), reverse=True)
        for index, cell in enumerate(cells(total, x + 1, y + 1, w - 2, h - 2)):
            parts.append(rect(*cell, COLORS[STATUSES[ranks[index]]]))
        parts.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" fill="none" '
                     f'stroke="{BORDER}" stroke-width="2"/>')
        label_length = int((w - 10) / 7)
        label = group["label"][:label_length] if label_length > 0 else ""
        if (label == group["label"] or len(label) >= 3) and h > 22:
            parts.append(text(x + 5, y + 16, label, 12))
        parts.append("</g>")
    return parts


def legend(y):
    parts, x = [], 4
    for status in STATUSES:
        parts.append(rect(x, y, 12, 12, COLORS[status]))
        parts.append(text(x + 17, y + 11, status, 12))
        x += 17 + 8 * len(status) + 18
    return parts


def render(functions, formats):
    width, height = 2 * PANEL_WIDTH + GAP, HEADER + MAP_HEIGHT + LEGEND
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}">',
             rect(0, 0, width, height, BORDER, 0)]
    parts += treemap("Functions", functions, 0)
    parts += treemap("Formats", formats, PANEL_WIDTH + GAP)
    parts += legend(HEADER + MAP_HEIGHT + 8)
    parts.append("</svg>")
    return "\n".join(parts)


def badge(label, data):
    percent = data["percent"]
    color = "#4c1" if percent >= 90 else "#97ca00" if percent >= 60 else "#dfb317" if percent >= 30 else "#fe7d37"
    value = f"{percent}%"
    left, right = 10 + 7 * len(label), 10 + 7 * len(value)
    width = left + right
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="20" role="img" aria-label="{escape(label)}: {value}">'
            f'<rect width="{left}" height="20" fill="#555"/><rect x="{left}" width="{right}" height="20" fill="{color}"/>'
            f'<g fill="#fff" font-family="Verdana,DejaVu Sans,sans-serif" font-size="11" text-anchor="middle">'
            f'<text x="{left / 2}" y="14">{escape(label)}</text><text x="{left + right / 2}" y="14">{value}</text></g></svg>')


def table(heading, data):
    head = "".join(f"<th>{status}</th>" for status in STATUSES)
    rows = [f"<h2>{escape(heading)}</h2>", "<table>",
            f'<tr><th>Group</th>{head}<th>&ge; {data["stage"]}</th><th>Total</th><th>%</th></tr>']
    totals = [0] * len(STATUSES)
    for group in sorted(data["groups"], key=lambda g: g["name"].lower()):
        counts = [sum(1 for i in group["items"] if i[2] == rank) for rank in range(len(STATUSES))]
        totals = [a + b for a, b in zip(totals, counts)]
        total = len(group["items"])
        cols = "".join(f"<td>{c}</td>" for c in counts)
        rows.append(f'<tr><td>{escape(group["name"])}</td>{cols}<td>{group["done"]}</td><td>{total}</td>'
                    f'<td>{pct(group["done"], total):.0f}%</td></tr>')
    cols = "".join(f"<th>{c}</th>" for c in totals)
    rows.append(f'<tr><th>Total</th>{cols}<th>{data["done"]}</th><th>{data["total"]}</th><th>{data["percent"]}%</th></tr>')
    rows.append("</table>")
    return "\n".join(rows)


def summary_page(functions, formats):
    return "\n".join([
        '<!DOCTYPE html>',
        '<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">',
        '<title>Magician\'s Saga port progress</title>',
        '<style>body{font-family:sans-serif;max-width:1000px;margin:auto;padding:16px}img{max-width:100%}'
        'table{border-collapse:collapse}th,td{border:1px solid #ccc;padding:2px 8px}td+td,th+th{text-align:right}</style>',
        '</head><body>',
        "<h1>Reverse-engineering progress</h1>",
        '<img src="progress.svg" alt="progress">',
        "<p>Generated by <code>tools/progress.py</code> from <code>re/functions.csv</code> and <code>re/formats.csv</code>. "
        'Raw numbers: <a href="progress.json">progress.json</a>. '
        f'Headline stage: <code>{functions["stage"]}</code>. '
        f'Function bytes at that stage: {functions["bytes_percent"]}% '
        f'({functions["bytes_done"]}/{functions["bytes_total"]}).</p>',
        table("Functions", functions),
        table("Formats and data structures", formats),
        "<p>Stages, in order: <code>unknown</code> (default tool name, not analysed), <code>named</code> (name backed by evidence), "
        "<code>documented</code> (signature, purpose and side effects written down), "
        "<code>reimplemented</code> (clean reimplementation exists), "
        "<code>verified</code> (checked against the original's behaviour). See docs/PROGRESS.md.</p>",
        "</body></html>",
    ]) + "\n"


def write_site(output, functions, formats):
    output.mkdir(parents=True, exist_ok=True)
    (output / "progress.json").write_text(json.dumps({"functions": functions, "formats": formats}, indent=2), encoding="utf-8")
    (output / "badge-functions.svg").write_text(badge("functions", functions), encoding="utf-8")
    (output / "badge-formats.svg").write_text(badge("formats", formats), encoding="utf-8")
    (output / "progress.svg").write_text(render(functions, formats), encoding="utf-8")
    (output / "progress.html").write_text(summary_page(functions, formats), encoding="utf-8")


# --------------------------------------------------------------------------- comparison report

def md(value):
    return value.replace("|", "\\|").replace("`", "'")


def flatten(data):
    return {item[0]: (item[1], item[2], group["name"]) for group in data["groups"] for item in group["items"]}


def details(icon, title, rows):
    if not rows:
        return []
    lines = [f"<details>\n<summary>{icon} {len(rows)} {title}</summary>\n", "| Group | Name | Change |", "| - | - | - |"]
    lines += [f"| {md(group)} | `{md(name)}` | {change} |" for group, name, change in sorted(rows)[:REPORT_ROWS]]
    if len(rows) > REPORT_ROWS:
        lines.append(f"| ... | {len(rows) - REPORT_ROWS} more | |")
    return lines + ["\n</details>"]


def compare(title, unit, base, head):
    old, new = flatten(base), flatten(head)
    both = old.keys() & new.keys()
    advanced = [(new[k][2], new[k][0], f"{STATUSES[old[k][1]]} -> {STATUSES[new[k][1]]}") for k in both if new[k][1] > old[k][1]]
    regressed = [(new[k][2], new[k][0], f"{STATUSES[old[k][1]]} -> {STATUSES[new[k][1]]}") for k in both if new[k][1] < old[k][1]]
    added = [(new[k][2], new[k][0], f"new, {STATUSES[new[k][1]]}") for k in new.keys() - old.keys()]
    removed = [(old[k][2], old[k][0], f"was {STATUSES[old[k][1]]}") for k in old.keys() - new.keys()]
    if not (advanced or regressed or added or removed):
        return []
    delta = round(head["percent"] - base["percent"], 2)
    icon = "📈" if delta > 0 else "📉" if delta < 0 else "➖"
    counts = [f"{n:+} {label}" for n, label in ((len(advanced), "advanced"), (len(added), "added"),
                                                (-len(regressed), "regressed"), (-len(removed), "removed")) if n]
    lines = [f'{icon} **{title}** (>= {head["stage"]}): {head["percent"]}% ({delta:+}%, {", ".join(counts)} {unit})', ""]
    lines += details("✅", "advanced", advanced)
    lines += details("🆕", "added to the registry", added)
    lines += details("⚠️", "went back a stage", regressed)
    lines += details("🗑️", "removed", removed)
    return lines + [""]


def report(base, head):
    lines = compare("Functions", "functions", base["functions"], head["functions"])
    lines += compare("Formats", "entries", base["formats"], head["formats"])
    return "\n".join(lines)


# --------------------------------------------------------------------------- command line

def summary_lines(label, data):
    reached = " | ".join(f'{status} {data["reached"][status]}%' for status in STATUSES[1:])
    extra = f'; bytes {data["bytes_percent"]}%' if data["bytes_total"] else ""
    return [f'{label} {data["done"]}/{data["total"]} ({data["percent"]}%) at stage "{data["stage"]}"{extra}',
            f"  reached: {reached}"]


def append_log(root, stage, functions, formats):
    path = root / "re" / "progress-log.csv"
    today = datetime.now(timezone.utc).date().isoformat()
    row = ",".join(str(v) for v in (today, stage, functions["done"], functions["total"],
                                    formats["done"], formats["total"], functions["bytes_percent"]))
    old = path.read_text(encoding="utf-8").splitlines() if path.is_file() else []
    kept = [line for line in old[1:] if line and not line.startswith(today + ",")]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join([LOG_HEADER, *kept, row]) + "\n", encoding="utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Reverse-engineering and port progress for the Magician's Saga native port.")
    parser.add_argument("output", type=Path, nargs="?",
                        help="directory to write progress.json, progress.svg, progress.html and badges into")
    parser.add_argument("--root", type=Path, default=ROOT, help="project root containing re/ (default: this checkout)")
    parser.add_argument("--stage", choices=STATUSES[1:], default=DEFAULT_STAGE,
                        help=f"status a function or format must reach to count as done (default: {DEFAULT_STAGE})")
    parser.add_argument("--group", action="append", metavar="NAME",
                        help="limit function counts to these groups; repeat for several. Format counts stay complete")
    parser.add_argument("--json", action="store_true", help="print the progress.json object to stdout")
    parser.add_argument("--check", action="store_true", help="only validate the registries and print their sizes")
    parser.add_argument("--log", action="store_true", help="record today's counts in re/progress-log.csv")
    parser.add_argument("--compare", type=Path, nargs=2, metavar=("BASE", "HEAD"),
                        help="print a markdown report of the changes between two progress.json files")
    args = parser.parse_args(argv)

    if args.compare:
        if args.output or args.group or args.json or args.check or args.log:
            parser.error("--compare does not combine with other options")
        snapshots = []
        for path in args.compare:
            try:
                snapshot = json.loads(path.read_text(encoding="utf-8"))
                validate_snapshot(snapshot)
            except (OSError, ValueError) as error:
                parser.error(f"{path}: {error}")
            snapshots.append(snapshot)
        base, head = snapshots
        if base["functions"]["stage"] != head["functions"]["stage"] or base["formats"]["stage"] != head["formats"]["stage"]:
            parser.error("both snapshots must be generated with the same --stage")
        print(report(base, head), end="")
        return 0

    if args.log and args.group:
        parser.error("--log records whole-project numbers and does not combine with --group")
    try:
        functions = collect(args.root / "re" / "functions.csv", args.stage)
        formats = collect(args.root / "re" / "formats.csv", args.stage)
        if args.check:
            print(f'ok: {functions["total"]} functions, {formats["total"]} formats')
            return 0
        if args.group:
            functions = select_groups(functions, args.group)
    except ValueError as error:
        parser.error(str(error))
    if args.output:
        try:
            write_site(args.output, functions, formats)
        except OSError as error:
            parser.error(f"{args.output}: {error.strerror or error}")
    if args.log:
        try:
            append_log(args.root, args.stage, functions, formats)
        except OSError as error:
            parser.error(f"re/progress-log.csv: {error.strerror or error}")
    if args.json:
        print(json.dumps({"functions": functions, "formats": formats}, indent=2))
    else:
        print("\n".join(summary_lines("functions", functions) + summary_lines("formats", formats)))
    return 0


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
