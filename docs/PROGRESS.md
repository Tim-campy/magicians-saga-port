# Progress measurement

`tools/progress.py` turns two CSV registries into numbers, an SVG treemap, an HTML page and badges. CI publishes them to GitHub Pages on every push to `main`.

## Origin and licence

`tools/progress.py` is the tool of the Yu-Gi-Oh! GX Tag Force 3 reverse-engineering project, which adapts the snapshot format, comparison report and treemap/badge rendering of [AnyPS5](https://github.com/boykopovar/AnyPS5)'s `tools/progress.py` (GPL-2.0). Files under `tools/` are GPL-2.0-only (SPDX header), and so is the repository (`LICENSE`).

## Registries

| File | One row per | Maintained by |
| --- | --- | --- |
| `re/functions.csv` | game method with native code in `libil2cpp.so` | `tools/registry.py` (never edit by hand) |
| `re/formats.csv` | file format, container or data structure | by hand |

Shared columns: `id,name,group,status,size` (`functions.csv` adds `address`).

| Column | Meaning |
| --- | --- |
| `id` | Methods: `game:<token>` (Assembly-CSharp.dll) or `kairo:<token>` (KairoLibrary.dll), the metadata token in lower-case hex. Formats: a short stable key. Unique. |
| `name` | Methods: `namespace.Type.Method` from the IL2CPP metadata, shown only once the method has a status (see below). Formats: a descriptive name, no commas. |
| `group` | Methods: broad code area derived from the namespace (`ui`, `ecs`, `data`, `boot`, ...), overridden by `re/groups.csv`; the treemap uses it. Per-subsystem bookkeeping is the status file (`re/status/<subsystem>.tsv`). Formats: the subsystem. |
| `status` | `unknown`, `traced`, `documented`, `reimplemented` or `verified`. Blank means `unknown`. |
| `size` | Bytes. Methods: distance to the next mapped method. Enables the byte-weighted figure. |
| `address` | ELF virtual address in `libil2cpp.so` (load bias 0), the address Ghidra shows. |

### Scope of the function registry

Only `Assembly-CSharp.dll` (4,704 methods) and `KairoLibrary.dll` (6,771, Kairosoft's in-house engine) count: 10,959 methods have code. Unity, mscorlib, Newtonsoft, Firebase, Tapjoy and other third-party assemblies are excluded, like a console SDK; the port replaces them instead of reproducing them. Methods whose pointer slot is null (abstract methods, unresolved generic instantiations: 516 in the two game assemblies) are excluded as well; this is a known limitation of `tools/map_il2cpp_methods.py`.

### Original names stay local

The full method list with original names is the game's own metadata, so it stays out of Git (`research/functions-full.csv`). The committed `re/functions.csv` gives the original name only for methods that have a status, which are already discussed in the docs; other rows show their id as name. `tools/registry.py apply` writes both copies; `check` rejects a committed row that leaks a name without a status.

## Stages

| Stage | Methods | Formats and structures |
| --- | --- | --- |
| `unknown` | Not analysed. | Seen but not identified. |
| `traced` | Role identified with a concrete basis (call site, data flow, string, runtime trace) recorded in `docs/`. IL2CPP gives every method its original name, so a name alone is not progress. | Located and named. |
| `documented` | Parameters, return value, side effects and callers recorded in `docs/subsystems/` or `docs/formats/`. **Headline bar.** | Layout written down in `docs/formats/`. |
| `reimplemented` | A clean C++ port of its behaviour exists in `src/` with a test in `tests/`. Evidence points to the source file. | A parser, extractor or codec in `tools/` or `src/` handles it. |
| `verified` | Behaviour compared against the original game running on Android (device or emulator), results in `docs/verification/`. | Round-trips byte-exactly or matches the original on real data. |

Each stage includes the ones before it: a `reimplemented` method must also be documented. Never raise a status without meeting the definition.

## Updating method statuses

1. Find the id: `python -I tools/registry.py lookup <name-or-0xaddress>`.
2. Add or edit a row in `re/status/<subsystem>.tsv`: `id<TAB>status<TAB>evidence<TAB>note`. `evidence` is a repo-relative path (`docs/subsystems/combat.md`, `src/core/gameplay/combat_rules.cpp`). A method appears in one status file only.
3. If a game type in the global namespace belongs to a subsystem, add `type_prefix,group` to `re/groups.csv` (`QuestResultForm,rewards`).
4. `python -I tools/registry.py apply`, then `python -I tools/registry.py check`.

`apply` needs the local `research/functions-full.csv`. If it is missing, rebuild it: `python -I tools/registry.py build research/il2cpp-code-map.json` (the code map comes from `tools/map_il2cpp_methods.py`, see `docs/SETUP.md`).

## Counting rules

- An entry is **done** when its stage is at or above `--stage` (default `documented`).
- The headline percentage counts entries; with sizes, a second figure weights by bytes.
- `reached` lists the cumulative percentage at every stage from `traced` upward.
- Totals include only entries in the registries. The function registry is complete for the two game assemblies; the format registry grows as formats are found.

## Commands

```
python -I tools/registry.py check
python -I tools/progress.py --check
python -I tools/progress.py
python -I tools/progress.py --stage reimplemented
python -I tools/progress.py --group ecs
python -I tools/progress.py build/progress --log
python -I tools/progress.py --compare old/progress.json new/progress.json
python -I tools/run_tests.py
```

- With an output directory, it writes `progress.json`, `progress.svg`, `progress.html`, `badge-functions.svg` and `badge-formats.svg`.
- `--log` keeps one row per UTC day in `re/progress-log.csv` (replacing that day's row).
- `--compare` prints a markdown report (advanced, added, regressed, removed); both files must use the same `--stage`.
