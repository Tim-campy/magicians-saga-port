# Presentation text and text layout

Talk (dialogue) and event-message text flows from the `Talk.txt`/`EventMsg.txt` tables through a boot-time catalogue, argument substitution, TAB/`&` splitting and a `SubForm` factory into `kairo.unity.ui.TextLayout`, which translates the text again, parses inline markup into `TextBlock`s, breaks lines using font measurement, auto-fits the font and reveals characters over time. All of this is static ARM64 evidence; nothing has been observed at runtime. The port has tested C++ modules for splitting/trim, argument substitution, form-data preparation, tag numeric parameters, reveal timing, auto-fit and the initial font correction; the markup parser, line formation, rubies, real translation, font metrics and rendering are not ported, so no screen of text can be produced yet. Fonts are in [fonts.md](fonts.md); markup syntax in [../formats/text-markup.md](../formats/text-markup.md).

## Overview

Pipeline (each stage names its port module, if any):

| # | Stage | Original | Port |
|---|---|---|---|
| 1 | Table load | `TalkData.Load` (id, charaIndex, counted string array), `EventMsgData.Load` (id, type, one string) | `presentation_forms` TalkData reader; EventMsg remains audit-only |
| 2 | Talk catalogue | `BootForm.Load` → `Data.get_talkData` → `Select(<>c.<Load>b__40_2)` → `ToArray` → static `AppData.talks_` (+0x378); delegate = `String.Join(TAB, TalkData.text)` | `presentation_forms` join_talk_text; desktop mission catalog owner |
| 3 | Argument substitution | `StringUtil.Replace(text,args)`: `<n>` markers ← `Language.TranslateText(arg_n,false)`, optional honorific cleanup | `presentation_text` `substitute_form_arguments` |
| 4 | Split / trim | `StringUtil.Split` TAB with trim; event fields by `&` without trim | `presentation_text` `split_form_text`, `split_event_text_fields` |
| 5 | Form data | `SubForm.MakeTalkForm` (type 0), `SubForm.MakeEvent` (type 4) | `presentation_forms` `prepare_talk_form`, `prepare_event_form` |
| 6 | Layout object | `TextLayout..ctor()` → `..ctor(!Japanese,true)` → `TextLayout.Init`; size 196×60, `SetSpeed(1500)`, colour RGB(37,35,54), line space 4 | none (`text_fit`/`text_reveal` hold numeric slices) |
| 7 | SetText | reveal reset → initial font correction → `_makeTextBlocks` → auto-fit loop → reveal length → `_makeTextRubies` → restore `font_` | `text_fit` `prepare_text_font_size`, `fit_text_blocks`; `text_reveal` `restart_text_reveal_for_new_text`, `reveal_length` |
| 8 | Markup parse | `_makeTextBlocks`: `TranslateText(text,true)` → `UnicodeToChanakya` → `<br>`→LF → tag scan over `TextLayout.TAG` (19 entries) | `text_parameters` (numeric parameters only) |
| 9 | Line formation | width test via `Font.StringWidthF`, LF breaks, word look-ahead, alignment from `po` bits, y layout | none |
| 10 | Reveal / timing | `TextLayout.Frame` (clock+TotalFrames), `Flush`, `IsEnd` | `text_reveal` |
| 11 | Page advance | `SubForm.Update` type-0 branch: Frame → input → IsEnd → Flush or next page → close counter | none |
| 12 | Draw | `TextLayout.Draw`: rebuild blocks if `Graphics` scale changed; `TextBlock.Draw` per block | none |

### Talk forms (type 0)

`SubForm.MakeTalkForm` (`0x1204abc`, outer) reads `TalkData.charaIndex_` (+0x20) and `AppData.talks_[id]` and calls the overload at `0x1204bc4`, which:
1. Applies `StringUtil.Replace` only when `args` is non-null. `args` is `IL2CPP_TYPE_STRING`, a TAB-encoded field list, not an object array (not .NET `String.Format`).
2. Creates the `SubForm`: `talkArg_`+0x218 = args, `type_`(+0xe8)=0, `x_/y_`(+0x150/+0x154)=0, `select_`(+0x13c)=0, `humanCharaIndex_`/character(+0x200)=argument, `nextIndex_`(+0x220)=−1.
3. Splits by TAB (trim) into `textAry_`(+0x1f8); `textIdx_`(+0x210)=0.
4. Creates the `TextLayout` (+0x208), sets size 196×60, `SetSpeed(1500)`, colour, line space, `SetText(textAry_[0])`, `SetTextCheckSize(-1)`. Reading `textAry_[0]` fails when there is no element — no empty page is invented.

### Event forms (type 4)

`SubForm.MakeEvent` (`0x1205028`) builds `decimal(EventMsgData.type)` + `&` + `EventMsgData.text` (+0x20/+0x28) and calls `0x1205120`, which: Replace → TAB split → allocates `eventIds_`(+0xf0) and `eventTexts_`(+0x100) of equal length → per segment splits by `&` (flags 0/0/0), `ParseInt(field0)` **before** reading `field1` (missing field1 faults), ignores fields 2+. Form gets `type_`=4, x/y=0, select=0, `eventAnim_`(+0xf8) = four zero ints. Update/draw of type 4 is not traced (it is a separate jump-table target, `0x11f80cc`).

### Page advance and close (type 0)

`SubForm.Update` (`0x11f7a44`) first runs `MyFormBase.Update`, the close counter (+0x244: if ≥0 decrement; ≤0 → `FormBase.Pop`) and `IsDrawWindowEnd`, then dispatches on type via a 12-entry uint16 table at `0x645ddc` (target = `0x11f7dec` + entry×4; type 0 → `0x11f829c`, type 4 → `0x11f80cc`). Type 0:
1. `TextLayout.Frame` on +0x208, before input.
2. Advance input: `Canvas.CheckKeyPulse(0x100000)`, or static flag +0xbb of the type at GOT `0x272f5c0` plus `Canvas.CheckKeyState(0x40)` (physical keys / flag meaning open).
3. On advance: `IsEnd` false → `Flush` (`0x11f85e8`), page index unchanged. True → `textIdx_++` (32-bit), compare with `textAry_` length; next page → `SetText`.
4. After the last page: `nextIndex_ == -1` → write 8 to the close counter (`0x11f8a40`), no immediate Pop. The 8 is Update passes, not rendered frames.
5. `nextIndex_ != -1` (`0x11f8854..0x11f89bc`): load `AppData.talks_[nextIndex]`, Replace with `talkArg_` if non-null, TAB split; field0 = character via `JInteger.ParseInt` (−1 → slot 7 object of `AppData+0x198→+0x20`; still −1 → 0; bounds-checked against vector +0x1b0); field1 = new `nextIndex_`; fields 2+ → `textAry_`; `textIdx_`=0; first page required. This continuation format differs from the initial factory (which gets the character separately); producers of a non-(−1) `nextIndex_` are not mapped.

### Mission talk integration and layout boundary

`presentation_forms` supplies TalkDefinition/TalkTable, load_talk_definition/load_talk_table and join_talk_text. The reader assigns id/charaIndex immediately and assigns the counted UTF16 string array only after every element is read; negative count gives null, zero gives a distinct empty array. Catalog indexing reuses the shared table reader: id gaps, last duplicate, BOM and explicit CRLF policy. BootForm's delegate joins the fragments with TAB, preserving empty fragments and rejecting a null array. UTF8 decoding is injected into the portable reader; the desktop adapter uses the public C++17 [codecvt UTF8/UTF16 facet](https://learn.microsoft.com/en-us/cpp/standard-library/codecvt?view=msvc-170), without reconstructing runtime Unicode code.

`update_talk_page` implements the reached type0 branch after the caller's opening/closing guards: text Frame, portrait Seb.Frame, pulse short-circuit or Config/held query, IsEnd, Flush or wrapped page increment, SetText, and final close8. Continuation remains an explicit host callback; the factory's nextIndex=-1 does not reach it. Failed SetText retains the incremented page index. Tests cover query order, held/pulse gates, overflow, multiple pages and partial failure. This is a branch port, not a complete SubForm.Update implementation.

`saga_mission` composes the actual Event/Talk tables, indexed execution and opcode8 factory with these kernels. Quest1 event212 selects Talk4 (character71, two pages); events213/214/215 select Talk5/6/7 (two/three/two pages). The native type0 window style is2 from SubForm.configData_[0][3] (`0x1206694..0x12066bc`); IsDrawWindowEnd (`0x11f8cd8`) tests membership in0/1/2 and gates opening until frame>5. Fresh finishFrame=-1 comes from the constructor; the final page assigns8 and eight later Updates request Pop. See [modal lifecycle](ui.md#mission-talk-modal-lifecycle) and [integration](integration.md#mission-event-dialogues-s1-a1a4).

`talk_layout` provides provisional, explicit per-page TextLayout lengths. Native SetText sums the actual parsed/wrapped TextBlock strings, including font-dependent space decisions, so raw or tag-stripped source length is not a replacement. The host runs the existing timing/Flush/IsEnd kernels with these supplied lengths and an explicit IsShortWords input. Text/portrait/window/softlabel drawing is a listed presentation stub; the title exposes page controls only. Full markup, font measurement and rendering remain required.

### Markup parser (`_makeTextBlocks`, `0x1bbd4c4`, 10,344 bytes)

SetText calls TranslateConsoleText before reveal reset and has an extra split/rejoin branch conditional on `<ff>`/`<FF>` (GOT0x273e830/0x273e868). Talks4..7 do not reach that branch; this does not authorize dropping it for other text. The existing SetText export also has an outgoing branch at0x1bbd004 to0x25fe2f0 whose exceptional/cold role remains unproven.

Entry: saves `font_`; font localization scope (see [fonts.md](fonts.md)); creates a work font copy and `AdjustFontSize(scaleRatio_)`; then at `0x1bbd9a4..0x1bbda18`: `Language.TranslateText(text, true)` → `Language.UnicodeToChanakya` → literal `<br>` → LF (`StringUtil.Replace` overload). So table text does not reach the parser unchanged, and the whole text is translated a second time (independent of the argument translation in stage 3).

Tag scan (`0x1bbdad4`): find `<` and next `>`, require distance ≤ 32, walk the 19-entry static `TAG` array (built by `TextLayout..cctor`, static +0x18) in order: `String.CompareTo` for exact tags, `String.StartsWith` with the prefix up to `=` for parameterised tags. Handling dispatch: uint16 jump table at `0x68dd0e`, target = `0x1bbe854` + entry×4 (co → `0x1bbeaac`, /co → `0x1bbec9c`, po → `0x1bbee5c`). Tag semantics and parameter parsing: [../formats/text-markup.md](../formats/text-markup.md). Tags actually used by the shipped tables: `br`, `co`, `/co`, `po`, plus `<0>`/`<1>` substitution markers.

`TextBlock` (type 2268): `text_`+0x28, `isButton_`+0x44, `isPictograph_`+0x45. `TextBlock..ctor` (`0x1bc09e8`) takes parent/font/colour/x/y/blink, **copies** the Font, starts with empty text, `start_`=`length_`=−1, `borderColor`=−1. `TextLayout.textBlocks_` is assigned the new array at `0x1bbf528` before it is fully populated (no transactional rollback).

### Line formation

An unterminated `<` or an exhausted TAG search takes ordinary-character processing (`0x1bbdaf8..0x1bbdb70` -> `0x1bbdcc4`). A closed candidate with closing-minus-opening distance>32 advances past the opener without emitting that `<` (`0x1bbdb0c..0x1bbdb14` -> `0x1bbe854`). Preserve this quirk instead of stripping all unknown markup. Fresh talk Init has format=false, excluding the U+3001/U+3002 format-overflow branch. Actual wrapping still needs accumulated-run measurements and native lookahead, not a conventional replacement word-wrap algorithm.

- `0x1bbde50..0x1bbdec4`: LF (U+000A) forces a break. Otherwise compare accumulated width + increment with `width_`(+0x38); `width_ == -1`, processing flags and `multiLine_`(+0x84) change the path. With `format_`(+0x6c) set, U+3001/U+3002 get special overflow handling. Not "N characters per line".
- Increment may be computed by measuring the block text with the new character appended minus the previous width (`0x1bbdee4..0x1bbdf34`) — summing per-character widths is not equivalent.
- Word/run look-ahead `0x1bbdf40..0x1bbe318` stops at space/LF and inspects tags; `Language.IsJapaneseStyleTextLayout` at `0x1bbe338` changes space handling at the break.
- Line close `0x1bbe508..0x1bbe668`: x of each block = alignment offset + running sum of widths; y = vertical accumulator + (line max height − block font height)/2 (integer half); reset width; y += max height + `lineSpace_`. An extra branch tied to `ff`/autoScale exists (do not drop it when rebuilding pagination). Alignment: horizontal bit 2 = centre, 4 = right; vertical 0x20 = middle, 0x40 = bottom; dimension −1 has separate formulas.

### Auto-fit and font preparation in SetText (`0x1bbcfdc`)

Order: reveal reset (`0x1bbd28c`) → initial font correction (`0x1bbd298..0x1bbd300`) → first `_makeTextBlocks` + font snapshot → auto-fit (`0x1bbd32c..0x1bbd420`) → reveal length (`0x1bbd420..0x1bbd48c`) → `_makeTextRubies` → restore `font_` from snapshot (`0x1bbd498`).

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `data.TalkData.Load` | `game:0x06001213` | id, charaIndex, counted string array | high |
| `data.EventMsgData.Load` | `game:0x06001135` | id, type, one string token | high |
| `<>c.<Load>b__40_2` | `game:0x0600071e` | Talk catalogue delegate: `String.Join(TAB, text)` | high |
| `form.SubForm.MakeTalkForm` (outer) | `game:0x060008d2` | Reads charaIndex and `talks_[id]`, forwards | medium |
| `form.SubForm.MakeTalkForm` | `game:0x060008d4` | Type-0 form factory | high |
| `form.SubForm.MakeEvent` (outer) | `game:0x060008d5` | `type&text` encoding | high |
| `form.SubForm.MakeEvent` | `game:0x060008d6` | Type-4 form factory | high |
| `form.SubForm.Update` | `game:0x060008b8` | Close counter, type dispatch, talk page advance | high (type 0 only) |
| `kairo.unity.util.StringUtil.Replace` | `kairo:0x060005fe` | `<n>` argument substitution + honorific cleanup | high |
| `kairo.unity.util.StringUtil.Replace` (literal) | `kairo:0x060005ff` | Literal replace, search resumes after insertion | high |
| `kairo.unity.util.StringUtil.Split` (14533) | `kairo:0x06000600` | Forwards with flags (1,0,0) | high |
| `kairo.unity.util.StringUtil.Split` (impl 14535) | `kairo:0x06000602` | trim/exclude-empty/escape flags | high (flags used here) |
| `kairo.unity.util.StringUtil.set_SanSanRemove` | `kairo:0x060005fc` | Honorific cleanup switch; no direct callers | medium |
| `kairo.unity.util.Language.TranslateText` | `kairo:0x060003f2` | Cached translation entry | medium |
| `kairo.unity.util.Language.CallTranslateText` | `kairo:0x060003f3` | Translation core (regex, trim, case, fallbacks) | low |
| `kairo.unity.util.Language.GetTranslateCache` / `SetTranslateCache` | `kairo:0x0600040e` / `kairo:0x0600040f` | Translation cache | low |
| `kairo.unity.util.Language._translateText` | `kairo:0x060003f4` | Lookup called by CallTranslateText | low |
| `kairo.unity.util.Language.UnicodeToChanakya` | `kairo:0x0600041b` | Applied to whole text before tag scan | medium |
| `kairo.unity.util.Language.IsShortWords` | `kairo:0x06000428` | Selects reveal speed factor | high |
| `kairo.unity.util.Language.IsJapaneseStyleTextLayout` | `kairo:0x060003e7` | Space handling at line break | medium |
| `kairo.unity.ui.TextLayout..cctor` | `kairo:0x06001832` | Builds 19-entry `TAG` array | high |
| `kairo.unity.ui.TextLayout..ctor()` | `kairo:0x06001835` | Forwards `(!Japanese, true)` | high |
| `kairo.unity.ui.TextLayout..ctor(bool,bool)` | `kairo:0x06001836` | `fontBuf_`, Init, autoScale_, localizeFontEnable_ | high |
| `kairo.unity.ui.TextLayout.Init` | `kairo:0x06001837` | `font_`, scaleRatio_, pos=0, speed=1000 | high |
| `kairo.unity.ui.TextLayout.SetText` (19198) | `kairo:0x06001839` | Rebuild pipeline (see order above) | high |
| `kairo.unity.ui.TextLayout._makeTextBlocks` | `kairo:0x0600183b` | Translate, markup parse, measure, lines | medium |
| `kairo.unity.ui.TextLayout._makeTextRubies` | `kairo:0x0600183c` | Ruby pass after blocks | low |
| `kairo.unity.ui.TextLayout.GetHeight` | `kairo:0x0600183e` | Measured layout height (auto-fit) | medium |
| `kairo.unity.ui.TextLayout.Draw(Graphics)` | `kairo:0x06001841` | Scale-change rebuild, per-block draw | medium |
| `kairo.unity.ui.TextLayout.Frame` | `kairo:0x06001843` | Reveal timing (+ blink/draw counters) | high (timing part) |
| `kairo.unity.ui.TextLayout.Reset` | `kairo:0x06001844` | Zero pos/beginTime/lastAppFrame/timeOffset | high |
| `kairo.unity.ui.TextLayout.SetSpeed` | `kairo:0x06001845` | Speed ×3/2 unless short words | high |
| `kairo.unity.ui.TextLayout.Flush` | `kairo:0x0600184e` | Reveal all, recompute beginTime | high |
| `kairo.unity.ui.TextLayout.IsEnd` | `kairo:0x0600184f` | Reveal finished predicate | high |
| `TextBlock..ctor` | `kairo:0x06001882` | Block with copied Font | high |
| `kairo.unity.ui.Graphics.PopAutoTextLayout` | `kairo:0x060013bc` | Called by parser cleanup | low |

## Behaviour

### Split (`StringUtil.Split` impl `0x1c19098`)
- Split(14533, `0x1c195ec`) forwards with flags (trim=1, excludeEmpty=0, escape=0). Table/integer parsers (data report 76) use (0,0,0).
- Null or empty input → array of length 0. Non-empty input keeps empty segments, including after a trailing delimiter; TAB alone → two empty segments; only-trimmable input → one empty segment.
- Trim (`0x1c192cc..0x1c193a4`) works on UTF-16 units: advance start while unit < 0x21, retreat end while unit ≤ 0x20 → strips **U+0000..U+0020** only. NBSP, U+3000, surrogates are kept; interior untouched. `String.Trim`/`Char.IsWhiteSpace` are **not** used.
- CRLF normalisation exists only on the LF-separator branch; not for TAB/`&`/`,`.

### Argument substitution (`StringUtil.Replace` `0x1c18df4`)
- Returns text unchanged if text null, args null or args empty (no host call).
- args split by TAB with flags (0,0,0): no trim, empty fields kept.
- For field i: `TranslateText(field,false)`, then read `SanSanRemove`; build marker `<i>` (delimiters at `0x2739e50`/`0x2739da0`); replace all occurrences via literal Replace (`0x1c19534`), resuming the search after the inserted text — the current marker inside an inserted argument is not reprocessed, but later indices' markers inserted by an argument are reached in later passes. Multi-digit indices work.
- Honorific cleanup, pattern `(さん(</co>)?)さん` → `$1` (pattern `0x273fed0`, replacement `0x2739fe0`): branch at `0x1c18f60` is `cbz w8` — **false skips, true enables** (report 85 had this inverted; 88 corrected it). When enabled, applies one `Regex.Replace` pass after the substitution only if neither the pre-substitution text nor the translated argument already matched. Translation returning null leaves text unchanged on the no-check path; with cleanup enabled and no pre-existing match, the null argument faults (Regex.IsMatch). Left-to-right, non-overlapping, single pass.
- `set_SanSanRemove` (`0x1c18d9c`): no immediate B/BL callers found (`build/research/sansan-setter-callers.json`); reflection/direct field writes not excluded. Initial value unknown.
- Literals: TAB GOT `0x272feb8`, `&` `0x2732430`.
- Composition: substitute first, then split — a TAB produced by translation creates a page boundary.

### Translation (`Language.TranslateText` `0x1b1e6f0`)
Null → null; empty → same string; else `GetTranslateCache` hit returns; miss → `CallTranslateText(text, flag)` (`0x1b1ebf0`: special chars, substitutions, regex, trim, case, `_translateText` incl. retry after regex), guarded fallbacks to `TranslateBuiltInMultiLangText`/`TranslateConsoleText`, then `SetTranslateCache`. Flags, literals and fallback order are not closed. Called with `false` for arguments and `true` for whole layout text. The catalogue delegate does not translate.

### Reveal timing (TextLayout type 2272)
Fields: `length_`+0x34, `pos_`+0x40, `speed_`+0x44, `beginTime_`+0x48 (64-bit), `timeOffset_`+0x50, `lastAppFrame_`+0x58.
- `Init`: 64-bit literal at `0x626ca0` → pos=0, speed=1000.
- `SetSpeed` (`0x1bc3568`): store arg; if `!IsShortWords()` → `trunc32(wrap32(speed*3)/2)` (multiply first, may overflow; not ×1.5f). `IsShortWords` (`0x1b2f410`) = `Japanese()` or (`langPackScale_` static +0xb0 ≠ −1 and ≥ 100). So the talk's 1500 becomes 1500 or 2250.
- Length (`SetText` `0x1bbd420..0x1bbd48c`): zero, then per block +1 if `isButton_` or `isPictograph_` (text not read), else `StringEx.Length(text_)` = managed length int32 at +0x10 (UTF-16 units; surrogate pair = 2); null faults; 32-bit sum. Not `text.size()` of the prepared text.
- `Frame` (`0x1bc33ac`): if beginTime==0 → beginTime = now−50; if lastAppFrame == TotalFrames → timeOffset += 50 (32-bit); lastAppFrame = sign-extended TotalFrames (second query); now again; product = (now + timeOffset − beginTime) × speed (64-bit); pos = low32 of: 0 if product ≤ −50, else min(product/50 truncated, wrap32(length×1000)) (`0x1bc3468..0x1bc34ac`; high-mul/shift sequence equals truncated division on 10,014 test values, seed 91). Query order: clock (if needed), TotalFrames twice, clock.
- Clock: `JSystem.CurrentTimeMillis` = `(DateTime.UtcNow − static ref).TotalMilliseconds` → int64; not proven monotonic. TotalFrames: `IApplication.get_TotalFrames` = int32 at app+0x58 (see [boot.md](boot.md)).
- `Reset` (`0x1bc3554`): zeroes pos, beginTime, lastAppFrame, timeOffset. SetText rebuild path (`0x1bbd28c`) zeroes pos, beginTime, timeOffset but **not** lastAppFrame.
- `Flush` (`0x1bc36b0`): pos = wrap32(length×1000); query clock; beginTime recomputed with 32→64-bit multiply, `SDIV` by speed (no zero guard: quotient 0) and division by −20. Example: length 1, speed 1500, now 2000 → pos 1000, beginTime 1967; a following Frame at now 2000 gives pos 990 (no sticky "done" flag).
- `IsEnd` (`0x1bc370c`): pos == −1 or pos ≥ wrap32(length×1000) (signed 32).
- Frame also updates block blink counters and a conditional draw counter (not ported).

### Initial font correction (SetText `0x1bbd298..0x1bbd300`)
Only if `adjustFontSize_`(+0x86) false and `multiLine_`(+0x84) true; independent of `autoScale_` (so applies to Japanese talks). size = `Font.usedSize_`(+0x1c), f = static `Font.requestFontScale_`(+0x44, captured once): `r = ((wrap32(size*100) / f) * 100 wrapped) / f` (signed truncating, two separate divisions; f=0 → 0; INT_MIN/−1 → INT_MIN), `SetSize(r,false)`, then `adjustFontSize_ = true` only after a normal return. 10/150 → 4; −10/150 → −4. The result is a request to the setter, not necessarily the drawn size. `TextLayout.SetFont` clears +0x86.

### Auto-fit (SetText `0x1bbd32c..0x1bbd420`)

TextLayout.GetHeight (`0x1bbfd2c`,188 bytes) starts a binary32 maximum at0 and traverses textBlocks_ in order, requiring the array and each block. Null text fails before the font query. Empty text contributes no height and does not require its font; nonempty text requires block.font and computes binary32 `block.y + int32(font.usedSize)` from offsets+0x34/+0x1c. Only ordered greater values replace the maximum; negative/NaN candidates do not increase it. It returns ARM FCVTZS of binary32 `maximum +0.9` (literal0x3f666666 at0x6278bc), with explicit +infinity ->INT32_MIN. This numeric font size is not a FreeType ascent/descent/line-spacing result. Evidence: complete ignored mission-text-metrics/TextLayout_GetHeight.asm, project base0x100000 subtracted; confidence high for the static getter.
Requires `autoScale_`(+0x78) and height(+0x3c) > 0.
- Multiline: while `GetHeight()` > height+1: size = usedSize−1 (wrap32), `SetSize(size,false)`, rebuild blocks; exit when the **requested** size was 1 (even if the setter produced another size); else re-measure. After the loop: if height < `GetHeight()` again, call `GetHeight()` once more and store it into height (1-px tolerance can still grow the layout). Repeated queries are not merged.
- Single line: also requires width > 0; reduce while width < `textWidth_`(+0x68), rebuilding each time; no GetHeight; same requested-size-1 stop. No iteration cap.
- Ordered float comparisons (FCMP/B.PL, B.LE); NaN never enters via the positive guards.

### Draw-time scale refresh (`TextLayout.Draw` `0x1bc1450..0x1bc1478`)
Compare float `Graphics`+0x5c with `scaleRatio_`(+0x7c) (FCMP/B.EQ: NaN ≠, ±0 equal); if different, store the new value first, then `_makeTextBlocks`, then `_makeTextRubies`. A failure in block construction skips rubies; no rollback. Draw uses the existing `font_` (no `Graphics.GetFont`) and calls `TextBlock.Draw` (`0x1bc1838`) per block at `0x1bc1624`.

### Parser exceptions and cleanup
- LSDA `0x935e34` for `_makeTextBlocks`: 255 call-site ranges, LPStart omitted, type encoding 0x9c, ULEB128 call-sites.
- Local catches only around the decimal/hex parameter parses (see [../formats/text-markup.md](../formats/text-markup.md)).
- Preparation calls before the cleanup context (`0x1bbd84c` Push, `0x1bbd860` IsNeedLocalizeScale, `0x1bbd88c` copy ctor, `0x1bbd8a4` GetLocalizeScale, `0x1bbd8b8` SetSize, `0x1bbd8d4` Push) land at `0x1bbfa30` (pads `0x1bbfa30/9dc/9d8/a08`) and jump to `0x1bbfc88` **without** the cleanup helper. The context is built at `0x1bbd8fc..0x1bbd914`. Later calls (`0x1bbd958` work-font ctor, `0x1bbd974` AdjustFontSize, `0x1bbd9b0` TranslateText; pads `0x1bbfa28/a24/9d0`) go to `0x1bbfb9c` → `0x1bbfbbc` (selector 1 catches, object to sp+0x70) or `0x1bbfc80`, both calling helper `0xf875bc`. All have action_index 3.
- Helper `0xf875bc`: clears `Font.LT_DISABLE_TEXTLAYOUT` (static +0x4c, `0xf875ec`); if localization was applied → `PopLocalizeSetting` (`0xf87610`) and restore `font_` (`0xf87620`); if `localizeFontEnable_` false → Pop (`0xf8764c`); `Graphics.PopAutoTextLayout` (`0xf8766c`); block-vs-area checks; at `0xf87710` if context field 0 non-null → `0xfd3d20` → `0x104f9c4` (allocate + `__cxa_throw`; PLT `0x25fe500` begin_catch, `…510` end_catch, `…520` allocate_exception, `…530` throw).
- Outer routes `0x1bbfc88`/`0x1bbfcbc` write **true** (not the saved value) to `Language.UnicodeToChanakyaEnable` (+0x184, field 7828) and rethrow. RGB-component parse errors propagate; they are not turned into 0.
- Port consequence: preparation and protected body must be separate scopes; a single RAII guard from function entry would change behaviour; mutations before a failure are kept.

## Port

| Module (`src/core/gameplay/`) | Implements | Test (CTest) |
|---|---|---|
| `presentation_text.{hpp,cpp}` | `split_form_text` (TAB, trim), `split_event_text_fields` (`&`), `split_text_color_fields` (`,` trim); `substitute_form_arguments` with `PresentationTextHost` (translate_argument, remove_duplicate_honorific read after each translation). UTF-16 `std::u16string`, null vs empty kept via `std::optional`. | `presentation_text_contract` (`tests/test_presentation_text.cpp`), `presentation_arguments_contract` (`tests/test_presentation_arguments.cpp`) |
| `presentation_forms.{hpp,cpp}` | `prepare_talk_form` (type 0 data, `.at(0)` fault on no page), `prepare_event_form` (type 4, `parse_event_type` host before field1 access). TalkData/table join and type0 page advance are also host-bound; no complete TextLayout. | `presentation_forms_contract` (also executor → opcode 8 → wait 10 → opcode 12 integration with fixtures) |
| `text_parameters.{hpp,cpp}` | `parse_text_tag_numbers`, `text_tag_argb`; `ManagedTextParseError` = catchable managed exception category | `text_parameters_contract` |
| `text_reveal.{hpp,cpp}` | `reveal_length`, `configured_reveal_speed`, `TextRevealState`, reset/restart, `advance_text_reveal_timing`, `flush_text_reveal`, `text_reveal_ended`; host `TextRevealClock`; unsigned arithmetic for wrap | `text_reveal_contract` |
| `text_fit.{hpp,cpp}` | `prepare_text_font_size`, `fit_text_blocks` with `TextFitHost` (font size/setter/rebuild/height) | `text_fit_contract` |

Tools: `tools/inspect_presentation_table.py` (record layout audit + lexical `<...>` inventory; ASCII-decimal Int32 subset; keeps CR, extra tokens, null vs empty; no translation) with `tests/test_presentation_table.py` (7 tests); `tools/inspect_exception_callsites.py` (LSDA call-site ranges + landing pads with ELF SHA-256, pyelftools from `.tools/python-libs`; does not decode catch types/action chains) with `tests/test_exception_callsites.py` (2 tests). Last recorded Windows build: 20/20 CTest (MSVC 14.51, `tools\build-desktop.cmd`). No Linux run, no comparison with the original.

Missing: real `TranslateText`/`UnicodeToChanakya`, SanSanRemove state, the markup parser and block generation, line formation and measurement, `_makeTextRubies`, parser cleanup, Frame blink/draw counters, non-factory talk continuation, type-4 update/draw, TextLayout/TextBlock rendering.

## Evidence

Mission table/page contracts use ignored presentation-loaders/M38544.asm, talk-text-array/M36583.asm, talk-delegate/M35739.asm and form-update/M36149.asm. Opening/close defaults use mission-talk-modal/, membership/static config use mission-talk-lifecycle/. All bounded exports report full syntactic coverage; original addresses subtract project image base0x100000. Confidence high for those static branches, provisional for supplied layout lengths; no Android comparison.

- [../archive/reports/85-presentation-text-preparation.md](../archive/reports/85-presentation-text-preparation.md), [86-talk-catalog-producer.md](../archive/reports/86-talk-catalog-producer.md), [87-native-presentation-split.md](../archive/reports/87-native-presentation-split.md), [88-native-presentation-arguments.md](../archive/reports/88-native-presentation-arguments.md), [89-native-presentation-forms.md](../archive/reports/89-native-presentation-forms.md), [90-talk-page-advance.md](../archive/reports/90-talk-page-advance.md), [91-text-layout-timing.md](../archive/reports/91-text-layout-timing.md), [92-native-text-reveal.md](../archive/reports/92-native-text-reveal.md), [93-presentation-markup-inventory.md](../archive/reports/93-presentation-markup-inventory.md), [94-text-parameter-exceptions.md](../archive/reports/94-text-parameter-exceptions.md), [95-native-text-parameters.md](../archive/reports/95-native-text-parameters.md), [96-text-lines-and-font-metrics.md](../archive/reports/96-text-lines-and-font-metrics.md), [102-native-text-autoscale.md](../archive/reports/102-native-text-autoscale.md), [103-initial-text-font-correction.md](../archive/reports/103-initial-text-font-correction.md), [105-font-adjustment-and-preparation-exceptions.md](../archive/reports/105-font-adjustment-and-preparation-exceptions.md), [108-font-backend-access-and-draw-scale.md](../archive/reports/108-font-backend-access-and-draw-scale.md)
- Local (`build/research/`): `form-data-preparation/`, `delay-attach-more/M36179.asm`, `talk-writers/M35730.asm`, `talk-delegate/`, `translation-lookup/`, `form-update/M36149.asm`, `text-progress/`, `text-time/` (incl. `division-check.txt`), `text-block-length/M19200.asm`, `text-tags/M19191.asm`, `text-integer/`, `text-integer-exceptions.json`, `text-parser-cleanup/`, `font-adjust-wrapper/preparation-exceptions.json`, `presentation-tables/`, `presentation-markup/*.json`, `sansan-setter-callers.json`, build logs `presentation-*-build.log`, `text-*-build.log`.

## Open questions

1. (blocking) Real translation path: `CallTranslateText` flags, literals, fallback order and the effective language; `UnicodeToChanakya` behaviour (Hindi path).
2. (blocking) Full `_makeTextBlocks` block generation for the tags in use (`br`, `co`, `/co`, `po`) plus measurement composition; `_makeTextRubies`; `ff`/autoScale branch in line close.
3. (blocking) Font metrics backend for line breaking (see [fonts.md](fonts.md)).
4. Effective `StringUtil.SanSanRemove` value and its writers.
5. Producers of `SubForm.nextIndex_ != -1`, the continuation record format in real data, and the advance-input flag (static +0xbb) / physical keys.
6. `TextLayout.Frame` blink and draw counters; full draw path through `TextBlock.Draw`.
7. Type-4 (event) form update/draw and `eventAnim_` semantics.
8. Static reference of `JSystem.CurrentTimeMillis` and monotonicity.
9. Exception boundaries of `TextLayout.Draw`; whether external callers restore state after parser failures.
10. Units/meaning of 196×60 and the drawn coordinates of the talk window.
