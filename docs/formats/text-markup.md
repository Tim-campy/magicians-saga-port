# Text markup

Inline markup used in presentation text (`Talk.txt`, `EventMsg.txt` and anything passed to `kairo.unity.ui.TextLayout.SetText`). Two layers: field separators and `<n>` argument markers consumed by `SubForm`/`StringUtil.Replace` before layout, and `<br>` plus the 19 layout tags consumed by `TextLayout._makeTextBlocks`. The tag list, scanner rules and the numeric parameter path are established statically; the visual effect of most tags is not reconstructed. Text content itself is not documented here.

## Syntax

### Table records (before markup)

- `Talk.txt` row: `id` TAB `charaIndex` TAB `count` TAB `text_0` … `text_{count-1}` (counted string array; negative count → null, 0 → empty array). The boot catalogue joins the array with TAB, so each fragment becomes one page.
- `EventMsg.txt` row: `id` TAB `type` TAB `text` (single token). `SubForm.MakeEvent` encodes `type&text`; a TAB inside text yields several `type&text` segments; each segment is split by `&` (field0 = int, field1 = text, further fields ignored, no trim).
- Page/segment separator: TAB, with trim of U+0000..U+0020 at both ends of each page; empty pages kept. No escape syntax.
- Continuation record (talk `nextIndex_` path): TAB fields `character`, `nextIndex`, `page…` (format seen in code only).

### Argument markers

`<n>` (decimal index, multi-digit allowed) is replaced by the translated n-th TAB field of the argument string. Seen in tables: `<0>`, `<1>`. Not part of the layout tag table.

### Line break

`<br>` → LF (U+000A), replaced literally after translation and `UnicodeToChanakya`, before the tag scan. A raw LF also breaks.

### Layout tags (`TextLayout.TAG`, index order = match order)

| Idx | Tag | Kind | Established effect |
|---:|---|---|---|
| 0 | `<co=…>` | colour | parameter → RGB (see below) |
| 1 | `</co>` | end | restore layout colour `TextLayout`+0x20 (not a stack pop) |
| 2 | `<si=…>` | param | not traced |
| 3 | `</si>` | end | not traced |
| 4 | `<st=…>` | param | not traced |
| 5 | `</st>` | end | not traced |
| 6 | `<bl=…>` | param | not traced |
| 7 | `</bl>` | end | not traced |
| 8 | `<po=…>` | position | alignment letters (see below) |
| 9 | `<pic=…>` | param | pictograph block (counts 1 in reveal length) — inferred from `isPictograph_` |
| 10 | `<bo=…>` | colour | same colour parsing as `co`; effect not traced |
| 11 | `</bo>` | end | not traced |
| 12 | `<btn=…>` | param | button block (counts 1) — inferred from `isButton_` |
| 13 | `<ff>` | flag | linked to an autoScale branch at line close; not traced |
| 14 | `<kr=…>` | param | not traced |
| 15 | `<em>` | flag | not traced |
| 16 | `</em>` | end | not traced |
| 17 | `<ruby=…>` | param | ruby text (`_makeTextRubies`); not traced |
| 18 | `</ruby>` | end | not traced |

`=` entries are prefix templates: the parameter is the text between `=` and `>`.

### Scanner rules (`0x1bbdad4`)

- Candidate = `<` up to the next `>`, distance ≤ 32 code units.
- Entries tested in index order: exact tags with `String.CompareTo`, parameterised with `String.StartsWith(prefix through '=')` (comparison culture/ordinal not proven).
- Behaviour for no match, unterminated `<` or distance > 32 (literal fallback) follows the original flow and is not yet reconstructed — do not assume unknown tags vanish.

### Numeric parameters (all parameterised tags)

1. Decimal: `JInteger.ParseInt(s)` → `Int32.Parse(s, NumberStyles 0x7, current NumberFormatInfo)` (leading/trailing white, leading sign). A caught `System.Exception` → value 0 (local handler, `[0x1bbe414,0x1bbe41c)` → pad `0x1bbf300`, zeroes w27).
2. Hexadecimal, always attempted independently: `JInteger.ParseInt(s,16)` → `Int32.Parse(s, 0x200 AllowHexSpecifier)` (any radix ≠ 16 takes the decimal path). Caught exception → 0 (`[0x1bbe420,0x1bbe430)` → `0x1bbf2bc`, zeroes w25).
3. `co`/`bo` only (indices 0/10), when hex == 0: if the parameter contains `,`, split with trim (U+0000..U+0020), decimal-parse the first three fields (further fields ignored) and combine `(r<<16) | (g<<8) | b` without clamping (`1,256,0` → `0xFF010000` after alpha; `1,2,-1` → `0xFFFFFFFF`). Errors here are **not** caught locally (pad `0x1bbf930` → outer cleanup and rethrow). No comma → 0. Hex ≠ 0 → hex value used.
4. Colour = low three bytes as R,G,B → `Graphics.GetColorOfRGB` (clamps 0..255, alpha 0xFF). So `co=4a86e8` fails decimal, succeeds hex.

### `po` parameter

Letters searched in fixed order: horizontal `l`,`c`,`r` → bits 1/2/4; vertical `t`,`m`,`b` → bits 0x10/0x20/0x40. Later letters win within a dimension; a dimension with no letter keeps the previous setting. Also zeroes the local vertical accumulator. `cm` → 2 | 0x20 (centre/middle). Line layout: h 2 = centred, 4 = right; v 0x20 = middle, 0x40 = bottom.

## Reading it

- `tools/inspect_presentation_table.py talk|event <file> --output <json>`: parses the record layouts (ASCII-decimal Int32 subset, CR and extra tokens preserved, null vs empty distinct) and emits a **lexical** inventory of `<…>` candidates with counts and record ordinal/ID/fragment locations; flags `<digits>` as argument candidates. No translation, no tag semantics. Tests: `tests/test_presentation_table.py` (7).
- Port: `src/core/gameplay/presentation_text.cpp` (TAB/`&`/`,` splits, `<n>` substitution), `src/core/gameplay/text_parameters.cpp` (`parse_text_tag_numbers`, `text_tag_argb`; host supplies decimal/hex parsing). No tag scanner or block builder exists yet.
- Validation: both languages audited. English Talk (119 fragments): `br` 136, `co=4a86e8` 4, `/co` 4, `<0>`/`<1>` 1 each. Japanese Talk: `br` 182, `<0>`/`<1>` 1 each. English EventMsg (24): `br` 26, `po=cm` 23, `<0>` 2. Japanese EventMsg: `br` 27, `<0>` 2. Talk: 46 records/language, IDs 0–45, 40 with more than one fragment, none empty; EventMsg: 24 records, IDs 0–23; no duplicates, extra tokens or null arrays. Script opcode 8 (57 calls, 45 IDs) and opcode 12 (22 calls, 22 IDs) reference only existing IDs. Translation, argument values and code can introduce tags not seen in tables.

## Evidence

- [../archive/reports/85-presentation-text-preparation.md](../archive/reports/85-presentation-text-preparation.md), [86-talk-catalog-producer.md](../archive/reports/86-talk-catalog-producer.md), [87-native-presentation-split.md](../archive/reports/87-native-presentation-split.md), [93-presentation-markup-inventory.md](../archive/reports/93-presentation-markup-inventory.md), [94-text-parameter-exceptions.md](../archive/reports/94-text-parameter-exceptions.md), [95-native-text-parameters.md](../archive/reports/95-native-text-parameters.md), [96-text-lines-and-font-metrics.md](../archive/reports/96-text-lines-and-font-metrics.md)
- Local: `build/research/presentation-markup/*.json`, `presentation-tables/`, `text-tags/M19191.asm`, `text-block-length/M19200.asm`, `text-integer/`, `text-integer-exceptions.json`.

## Open questions

1. (blocking) Semantics of `si`, `st`, `bl`, `pic`, `bo`, `btn`, `ff`, `kr`, `em`, `ruby` and their end tags.
2. (blocking) Scanner fallback for unmatched/overlong/unterminated `<…>`.
3. Culture of `CompareTo`/`StartsWith` and `Int32.Parse` (current NumberFormatInfo).
4. Whether translated text introduces other tags (Hindi/Thai packs, `UnicodeToChanakya`).
