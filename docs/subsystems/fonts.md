# Fonts

`kairo.unity.ui.Font` wraps a Unity font + `GUIStyle` and keeps a numeric size/scale state; text width is measured either through Unity `GUIStyle.CalcSize` or, when `useFontTexture_` is set, through a Java `Paint` call on Android. The default Unity font is chosen per language code (`Fonts/default-<lang>-<region>` → `-<lang>` → `-ja` → `default`), and talk layouts get their fonts from the default `TextLayout` constructor. The numeric state is ported and tested. Fresh talk construction uses the GUIStyle route; the shipped default GameSkin supplies LegacyRuntime when English resource lookup misses. Equivalent desktop metrics remain unverified, and the port has no font backend yet. All static evidence; nothing observed at runtime.

## Overview

### Objects and fields

`Font` (type 2144) instance fields: `size_`+0x10 (requested size), `usedSize_`+0x1c (effective size; earlier reports called this "size_"), `use preset` flag +0x20, `guiStyleId_`+0x24, `guiFontSize_`+0x2c (adjusted size), `scale_`+0x40 (float), `fontScale_`+0x44 (int), `useFontTexture_`+0x48 (byte), +0xa8 (float handed to the AdjustFontSize delegate). Static fields: `HEIGHT`+0, `WIDTH`+8, `defaultGUIFont_`+0x18, `defaultGUIFontLanguage_`+0x20, `localizeSettingLock_`+0x30, `localizeSettingStack_`+0x38, `localizeSettingCount_`+0x40, `requestFontScale_`+0x44, `requestFontOffset_`+0x48, `LT_DISABLE_TEXTLAYOUT`+0x4c (field 9805). Instance +0x44/+0x48 and static +0x44/+0x48 are different fields with coincident offsets — do not confuse them.

`TextLayout` font fields: `font_`+0x18 (main font, from `TextLayout.Init`), `fontBuf_`+0xa0 (separate Font from the 2-arg ctor), `autoScale_`+0x78, `scaleRatio_`+0x7c (float, read from the application in Init), `localizeFontEnable_`+0x85, `adjustFontSize_`+0x86.

`Graphics` static: +0x58 (cctor writes false), `<DefaultUseFontTexture>k__BackingField`+0x59 (field 9987), +0x5a (cctor writes true); instance +0x5c float scale (compared at draw), fonts at +0x1c8/+0x1d0.

### Startup chain for a talk font

1. `IApplication.Start` (`0x1b80c70`) calls `Language.Init` at `0x1b80d20`, early and unconditionally (only immediate B/BL site to `0x1b146fc`; delegates/reflection not covered).
2. `Language.Init` reads `PlayerPrefs.GetString(PREF_LANG_PACK` static +0xe8`)` → `SetLanguagePack` (`0x1b16594`); retry with null args after false. Under a guard on config field +0x60 it stores 100 → `Font.requestFontScale_` (`0x1b14c28`) and 0 → `requestFontOffset_` (`0x1b14c5c`) inline.
3. `SubForm.MakeTalkForm` → `TextLayout..ctor()` (`0x1bbca54`, at `0x1204ca8`) → `..ctor(!Language.Japanese(), true)` (`0x1bbcabc`): new Font → `fontBuf_` (ctor `0x1b77698` at `0x1bbcb98`), `TextLayout.Init` (`0x1bbcbb0`), `autoScale_ = !Japanese`, `localizeFontEnable_ = true` (`0x1bbcbb4/b8`). First argument is **not** `format_` (+0x6c).
4. `TextLayout.Init` (`0x1bbcbd4`) creates a second Font (`0x1bbcc50`) → `font_` (`0x1bbcc54`); reads application scale → `scaleRatio_`.
5. `Font..ctor` → `Font.Init` (`0x1b77fbc`, may dispatch to UI thread) → `Font._init`: `GetDefaultGUIFont`, `GetGUIStyleId` → +0x24, face 2, `SetSize(0,true)`, `SetStyle(0)`. No external TTF is chosen by the talk factory.

### Default font selection (`Font._getDefaultGUIFont` `0x1b7acf4`)

- Cache: if `Language.GetLanguageCode()` equals `defaultGUIFontLanguage_`, return `defaultGUIFont_`. On change: invalidate previous font, clear invalid-font cache, zero `defaultGUIFont_`; after the search store the code, update invalid-name records, call `TextLayout.ClearCache` (`0x1bbc8f4`).
- For a non-null, non-empty code: `ToLower` → `Trim` → `Split('-')` (culture-dependent .NET calls; do not replace with ASCII normalisation unproven). `Resources.Load` + `UnityEngine.Object` null-equality:
  1. ≥2 components: `Fonts/default-<c0>-<c1>` (`0x1b7b098`; later components ignored)
  2. `Fonts/default-<c0>` (`0x1b7b16c`)
  3. if `Language.Japanese()`: `Fonts/default-ja` (`0x1b7b250`)
  4. `Fonts/default` (`0x1b7b300`)
  Result cast to `UnityEngine.Font` (a type exception is not "missing"). Literals: `Fonts/default` GOT `0x273d7a8`, `-` `0x272fb60`, `-ja` `0x273d7a0`. No system-font creation when all fail; `GetGUIStyleId` finds a GUIStyle with the same font or creates one and `set_font(arg)`. Null uses the current-skin fallback described below, rather than Android SANS_SERIF.
- APK `ResourceManager` (pathID 13 of `globalgamemanagers`) has exactly four font keys: `fonts/default-en-switch`, `fonts/default-hi`, `fonts/default-ja`, `fonts/default-th` (FileIDs 1114–1117, Font pathID 3). No `fonts/default`, `-en`, `-en-us`, `-ko`, `-zh`, `-zh-cn`. Nothing maps `en-US` → `en-switch`. Unity default resources also hold Font `LegacyRuntime` (pathID 10102). All five fonts are extracted (see assets docs, report 14).

### Shipped default-skin font

The shipped GameSkin (MonoBehaviour pathID11000 in `assets/bin/Data/unity default resources`, byte start1099048, size8488) has its first GUISkin-specific field at raw offset0x28: m_Font PPtr(fileID0,pathID10102). It resolves to that file's LegacyRuntime font,50788 bytes, whose metadata lists Liberation Sans/Arial. The aligned common prefix consumes40 bytes; the generic MonoBehaviour typetree reader reads only that prefix and fails its8488-byte size check. This finding is a bounded aligned field read, not a complete typetree parse.

Public [Unity2022.3 GUISkin source](https://raw.githubusercontent.com/Unity-Technologies/UnityCsReference/2022.3/Modules/IMGUI/GUISkin.cs) declares m_Font first and assigns it as GUIStyle's default in MakeCurrent. [GUIStyle.font](https://docs.unity3d.com/2022.3/Documentation/ScriptReference/GUIStyle-font.html) specifies the current-skin fallback for null fonts. Thus the shipped default-skin English path selects the extracted `unity/Font/110_LegacyRuntime.ttf`; a different live skin/language remains an explicit runtime input.

### Language code

- `Language.GetLanguageCode` (`0x1b18a94`): `langPackCode_` (static +0xa0) if non-null/non-empty, else `Japanese()` ? `ja-JP` (GOT `0x2739b00`) : `en-US` (`0x27349c0`). `Japanese` compares `Language.Get()` with 0.
- `SetLanguagePack` (`0x1b17be4`): for `KAIRO_JAPANESE`/`KAIRO_ENGLISH` (static +0x8/+0x10) writes `langPackFile_`+0xc0 and `ja-JP`/`en-US` into `langPackCode_` (`0x1b18084`), then `LoadLanguagePack(null…)` (`0x1b181a4`) and `ClearCache`; conditional `PlayerPrefs.SetString/Save`.
- `LoadLanguagePack` (`0x1b20554`): initialises code/title/author/scale (STP at `0x1b20c8c` writes author +0x98 and `en-US` +0xa0; `ja-JP` for `KAIRO_JAPANESE` at `0x1b20ce4`); key `@language` (GOT `0x2739bc8`) → `Regex.Replace(value, "[^a-zA-Z\-]", "")` → `langPackCode_` (`0x1b211bc`); then if exactly two `-` components: `ToLower(c0) + "-" + ToUpper(c1)` (`0x1b22594`); one component (e.g. `ZH`) kept. Also stores 100 into `requestFontScale_` at `0x1b23ffc` (guard open). Saved values are restored at `0x1b2582c` in an exception region (scope not closed).
- Local packs (`29_language`, UTF-8, SC/TC with BOM): `@language,ko`, `@language,zh-CN`, `@language,ZH` on line 2. None has a matching font key.

### Width measurement

`Font.StringWidthF(text)` (`0x1b798ec`) → overload (`0x1b79778`, null extra arg): `IApplication.CheckUiThread` → direct `_stringWidthF` (`0x1b798f8`) or UI dispatch. `_stringWidthF`: `guiStyleId_ == -1` or null text → 0.
- `useFontTexture_ == false`: `Font.GetGUIStyle` (`0x1b7a310`); set `GUIStyle.fontSize/fontStyle`, `GUIContent.text`; `GUIStyle.CalcSize` (`0x23e9e28`). A contains-space check selects cached space-to-dot replacement before measurement, as detailed below. Result ×100 / the captured application factor.
- `useFontTexture_ == true`: scaled size → `KairoPlugin.GetFontTextureSize(text,size)` (`0x1c2b724`) → delegate `<>c__DisplayClass151_0.<GetFontTextureSize>b__0` (`0x1c3683c`, captures text +0x10, size +0x18):
  - `Config.UNITY_EDITOR` (static +0x30): reflection into `WinLib` / `kairo.windows.plugin.Utility.GetFontTextureSize` (not present; not a port dependency).
  - else `Config.APPLI_TYPE` (static 0) == 2: `AndroidJavaObject.CallStatic<byte[]>("getFontTextureSize", text, size)` at `0x1c36b3c` on static `KairoPlugin`+0x10; otherwise returns null.
  - Non-null result → `ByteArrayInputStream` → two `StreamUtil.ReadFloat` (each: read 4 bytes, `EncodeUtil.ByteAryToFloat(…, true)` → `ConvertEndian` byte swap → `BitConverter.ToSingle` bit reinterpretation) → `SizeF(width, second)`.
- `KairoPlugin.Init` (`0x1c22a6c`, if APPLI_TYPE==2 and not editor): `kairo.android.plugin.Utility` → static +0x10, `kairo.android.plugin.Config` → +0x8, `android.os.Build$VERSION` → +0x18.
- Java `kairo.android.plugin.Utility.getFontTextureSize(Ljava/lang/String;I)[B`: new `Paint`, linearText on, `setTextSize((float)size)`, `Typeface.SANS_SERIF`, antiAlias on; `measureText(text)`; second value = float return of `getFontMetrics(null)` (line spacing, not pixel bounds); `DataOutputStream.writeFloat` both (big-endian) → byte[]. Uses no APK font.

### GUIStyle measurement and scale contract

The GUIStyle branch of Font._stringWidthF captures IApplication.GetScaleRatio(false), gets its GUIStyle, then configures fontSize using binary32 `(applicationRatio * guiFontSize /100) +0.001` (literal bits0x3a83126f at0x6279d0). ARM FCVTZS saturates finite overflow and maps NaN to0; the explicit positive-infinity branch returns INT32_MIN. It assigns fontStyle from instance+0x28 and normal.textColor from instance floats+0x30..+0x3c. These are colour writes, not padding or glyph metrics. After measurement it returns ordered binary32 `width*100/applicationRatio`; the captured ratio is not re-read for that division.

The transformed measurement text is tested for U+0020. If present, a cache keyed by that text supplies a copy with every space replaced by U+002E; GUIContent receives that replacement. No-space text is assigned directly. GUIStyle.CalcSize therefore measures dots instead of spaces on this route. The alternate Android texture call receives the original transformed text. Font.LT_DISABLE_TEXTLAYOUT suppresses recursive translation inside block construction; outside that scope, TranslateText, UnicodeToChanakya and optional automatic layout still matter.

IApplication.GetScaleRatio (`0x1b7a270`,160 bytes; bool force) returns binary32 `instance+0xfc * instance+0x124 /100` when force=false and cached+0xfc is not NaN. Its raw-bit guard admits infinities as well as finite values: absolute bits below0x7f800001. Force=true or NaN calls KairoPlugin.GetScaleRatio(240,240). Initial/live field values remain explicit scenario/runtime inputs; do not replace this contract with a universal100.

Font.GetGUIStyleId (`0x1b784ac`,520 bytes) traverses the shared style list in order, reads each style.font and tests Unity Object equality with the supplied font. It returns the first match. Otherwise construct a fresh GUIStyle, assign that font, append, then return the current count minus1. A null list/style fails; destroyed-object/null equality remains the external Unity object-lifetime contract. Font.GetGUIStyle (`0x1b7a310`,112 bytes) returns null for id=-1; otherwise delegates indexed access to the shared style provider. Public containers suffice for the list operations.

Public [Unity2022.3 bindings](https://raw.githubusercontent.com/Unity-Technologies/UnityCsReference/2022.3/Modules/IMGUI/GUIStyle.bindings.cs) expose CalcSize as a native extern; the reference-only C# source does not implement its metrics and must not be copied into this GPL repository. [FreeType](https://freetype.org/license) is a public implementation candidate under its GPLv2 option, but identical font bytes do not establish matching hinting, kerning, bounds, scaling or rounding. Compare the desktop provider against original GUIStyle measurements before claiming equivalent line breaks; do not reconstruct Unity's native font engine.

### Who sets `useFontTexture_`

Stores to Font+0x48: `Font._set` (`0x1b78c3c`, copies from source font), `Font._copy` (`0x1b7a650`, copies), `Graphics.Init` (`0x1b5c978..0x1b5c988`: first Graphics font ← `Graphics.DefaultUseFontTexture`). `Font..ctor`, `_init`, `_setSize`, `_updateGUIFont` write nothing to it. `DefaultUseFontTexture`: getter `0x1b5b494` reads static +0x59; setter `0x1b5b4ec` stores `arg & 1` (`0x1b5b540`); no immediate callers of the setter; `Graphics..cctor` does not store +0x59 (zero-initialised → false at start). `KairoBase.Start` STRB +0x59 at `0x1c6c004` is on its own instance (discarded). Talk TextLayout fonts are created by `Font..ctor`, not copied from Graphics: no `Graphics.GetFont` → `TextLayout.SetFont` link found (no immediate callers of `SetFont`). For the reached fresh constructor -> _init -> localization/work-copy chain, useFontTexture_ stays at its CLR-zero false: neither constructor/_init writes +0x48, and copies preserve it. This establishes the GUIStyle route for this fresh chain, without a global claim about aliases/reflection or restored/custom fonts.

### Localization scope in `_makeTextBlocks`

1. Save `font_` (`0x1bbd824`). If `localizeFontEnable_` false → `PushLocalizeSetting(false)` (`0x1bbd84c`).
2. `IsNeedLocalizeScale` (`0x1b79fe0`): permission = top of stack (`stack[count-1]`) if count ≥ 1, else `Config.LOCALIZE_FONT_SCALE` (static byte +0x50); false if `localizeSettingLock_` or `Language.Japanese()`; then require `usedSize_` > 5 (checked last).
3. If true: copy font → `font_`, `GetLocalizeScale` on the copy → `SetSize(n, usePreset=true)` (`0x1bbd8b8`), then `PushLocalizeSetting(false)` (`0x1bbd8d4`) so it is not reapplied in scope.
4. Separate work font copy + `AdjustFontSize(scaleRatio_)` (`0x1bbd974`) — a different operation; do not fold the factors.
5. Cleanup helper restores (see [text.md](text.md) exceptions).

### Draw scale refresh

`TextLayout.Draw` rebuilds blocks and rubies when `Graphics`+0x5c ≠ `scaleRatio_` (see [text.md](text.md)). `Graphics.GetFont` (`0x1b681c8`) returns a **copy** of Graphics+0x1c8 (`Font.Copy` into a non-null destination, else copy ctor `0x1b78924`), never the internal reference. `TextLayout.SetFont` (`0x1bc362c`): clears +0x86 (`0x1bc3668`), copy-constructs, then replaces `font_` (`0x1bc3684`); no block rebuild; a constructor failure leaves the flag cleared and the old font.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `kairo.unity.ui.Font..ctor` | `kairo:0x0600129a` | Default construction → Init | high |
| `kairo.unity.ui.Font..ctor(Font)` | `kairo:0x0600129d` | Copy constructor | medium |
| `kairo.unity.ui.Font.Init` | `kairo:0x0600129b` | UI-thread dispatch → `_init` | medium |
| `kairo.unity.ui.Font._init` | `kairo:0x0600129c` | Default font, GUIStyle id, face 2, size 0, style 0 | high |
| `kairo.unity.ui.Font._set` | `kairo:0x0600129f` | Copies flag + GUIStyle/size/style | medium |
| `kairo.unity.ui.Font._setSize` | `kairo:0x060012a5` | Size request, presets, ×scale/100 | high |
| `kairo.unity.ui.Font.StringWidthF` (1-arg) | `kairo:0x060012b1` | Forwards with null | high |
| `kairo.unity.ui.Font.StringWidthF` (2-arg) | `kairo:0x060012b2` | UI-thread check and dispatch | high |
| `kairo.unity.ui.Font._stringWidthF` | `kairo:0x060012b3` | GUIStyle.CalcSize or font-texture route | high |
| `kairo.unity.ui.Font.Copy` | `kairo:0x060012bb` | Copy into existing font | medium |
| `kairo.unity.ui.Font._copy` | `kairo:0x060012bc` | Copies attributes incl. +0x48 | medium |
| `kairo.unity.ui.Font.AdjustFontSize` | `kairo:0x060012be` | UpdateFontScale + adjust arithmetic, UI dispatch | high |
| `kairo.unity.ui.Font._adjustFontSize` | `kairo:0x060012bf` | Adjust arithmetic → +0x2c | high |
| `kairo.unity.ui.Font.<AdjustFontSize>b__86_0` | `kairo:0x060012e0` | Delegate reading +0xa8 | medium |
| `kairo.unity.ui.Font.IsNeedLocalizeScale` | `kairo:0x060012c0` | Localization predicate | high |
| `kairo.unity.ui.Font.GetLocalizeScale` | `kairo:0x060012c1` | Localized integer size | high |
| `kairo.unity.ui.Font.PushLocalizeSetting` | `kairo:0x060012c4` | Push bool | high |
| `kairo.unity.ui.Font.PopLocalizeSetting` | `kairo:0x060012c5` | count-- if ≥1 | high |
| `kairo.unity.ui.Font.SetScale` | `kairo:0x060012c7` | Stores `requestFontScale_` | high |
| `kairo.unity.ui.Font.UpdateFontScale` | `kairo:0x060012c9` | Apply requested scale, resize with presets | high |
| `kairo.unity.ui.Font.GetDefaultGUIFont` | `kairo:0x060012ca` | Dispatch to `_getDefaultGUIFont` | medium |
| `kairo.unity.ui.Font._getDefaultGUIFont` | `kairo:0x060012cb` | Per-language resource search + cache | high |
| `kairo.unity.ui.Font._updateGUIFont` | `kairo:0x060012cd` | May swap GUIStyle; no flag write | low |
| `kairo.unity.ui.Font.GetGUIStyleId` | `kairo:0x060012d3` | Find/create GUIStyle for a font | medium |
| `kairo.unity.ui.Font.GetGUIStyle` | `kairo:0x060012d6` | GUIStyle for measurement | medium |
| `kairo.unity.ui.Font..cctor` | `kairo:0x060012d7` | HEIGHT/WIDTH presets, requestFontScale_=100 | high |
| `kairo.unity.ui.Graphics.get_DefaultUseFontTexture` | `kairo:0x060012ed` | Reads static +0x59 | high |
| `kairo.unity.ui.Graphics.set_DefaultUseFontTexture` | `kairo:0x060012ee` | Stores `arg&1`; no direct callers | high |
| `kairo.unity.ui.Graphics.GetFont` | `kairo:0x06001361` | Returns a copy of Graphics font | high |
| `kairo.unity.ui.TextLayout.SetFont` | `kairo:0x0600184a` | Clear +0x86, copy, replace font_ | high |
| `kairo.unity.ui.TextLayout.ClearCache` | `kairo:0x06001834` | Called on language change | low |
| `kairo.unity.native.KairoPlugin.GetFontTextureSize` | `kairo:0x06000730` | Font-texture measurement wrapper | high |
| `<>c__DisplayClass151_0.<GetFontTextureSize>b__0` | `kairo:0x060007e0` | Editor reflection / Android CallStatic, SizeF | high |
| `kairo.unity.util.Language.Init` | `kairo:0x060003dd` | Prefs → SetLanguagePack; inline font-scale stores | medium |
| `kairo.unity.util.Language.SetLanguagePack` | `kairo:0x060003f8` | Sets langPackFile_/langPackCode_ | medium |
| `kairo.unity.util.Language.LoadLanguagePack` | `kairo:0x060003fa` | `@language` directive, code normalisation | medium |
| `kairo.unity.util.Language.GetLanguageCode` | `kairo:0x060003fe` | langPackCode_ or ja-JP/en-US | high |
| `kairo.unity.util.Language.Japanese` | `kairo:0x060003e1` | `Language.Get() == 0` | high |
| `kairo.unity.util.Language.Get` | `kairo:0x060003df` | Current language index | low |

## Behaviour

### Size request (`Font._setSize` `0x1b78f84`)
Stores `usedSize_`+0x1c, `size_`+0x10 and the preset flag +0x20 **before** loading the preset table; the table is null-checked at `0x1b78ff0..0x1b78ff8` before the `usePreset` decision (`0x1b79008`), so a missing table faults even with presets off, leaving +0x2c unchanged. With presets: negative request passes the signed length compare but fails the unsigned bound at `0x1b79030`; request ≥ length is used as a literal size; valid index → preset value. Effective size = `wrap32(value × fontScale_) / 100` (signed). Which table (`HEIGHT`/`WIDTH`) is used per call is in the method body, not restated in reports.

### Presets (`Font..cctor` `0x1b7c070`)
Two 3-element int32 arrays via `RuntimeHelpers.InitializeArray` (`0x1b7c19c`, `0x1b7c1c4`) → `HEIGHT` (static +0) = {12, 24, 30}, `WIDTH` (static +8) = {6, 12, 15} (metadata data offsets `0x448428` / `0x448770`, `<PrivateImplementationDetails>` fields; SHA-256 of the 12 bytes matches the field names). Initial values only; later writers not excluded. cctor also sets `requestFontScale_` = 100; instance ctor copies it into `fontScale_`.

### Scale
- `SetScale` (`0x1b7ac34`): stores int into `requestFontScale_`, no clamp; no direct callers, but inline stores of 100 exist in `Language.Init` and `LoadLanguagePack`. Do not assume 100 is universal.
- `UpdateFontScale` (`0x1b7a8ac`): if `fontScale_ != requestFontScale_` → copy and `SetSize(size_, true)`. A setter failure keeps the new scale (no rollback).

### Adjust (`Font.AdjustFontSize` `0x1b7a6cc`, `_adjustFontSize` `0x1b7a844`)
`CheckUiThread`; direct path calls `UpdateFontScale` then the arithmetic; delegated path stores the float arg in +0xa8 and schedules `<AdjustFontSize>b__86_0` (`0x1b7c460`), which reads +0xa8 (mutable instance field, not a captured value). Arithmetic (`0x1b7a858..0x1b7a89c`), binary32 each step: `q = arg/100.0f`; `t = (float)usedSize_ * q / q` (do not cancel); `scale_ = q`; `t + 0.01f` (bits `0x3c23d70a` at `0x62798c`); FCVTZS → `guiFontSize_`+0x2c, with exact +∞ → INT_MIN. NaN → 0 (e.g. arg 0 → 0/0), finite out-of-range saturates. `usedSize_` is not written.

### Localized size (`GetLocalizeScale` `0x1b7a128`, returns int)
Factor `0x3f547ae1` (≈0.83); if `Config.ASIA_FONT_SCALE` (byte +0x6d) and language code `StartsWith("ko"|"zh", 1)` → `0x3f7ae148` (≈0.98). `(float)usedSize_ * f + 0x3dcccccd` (≈0.1), separate ops; FCVTZS with explicit +∞ handling; if original size ≤ 9 clamp below to `usedSize_ − 1` (wrap32). Constants at `0x62782c`, `0x627900`, `0x6278b8`; keep exact bits.

### Localize stack
`PushLocalizeSetting` (`0x1b7a9fc`): append a slot if needed, write at index count, count++. `PopLocalizeSetting` (`0x1b7ab5c`): count-- only if count ≥ 1 (no removal, no underflow error); stale values remain above the top.

### Backend the port needs
A measurement service returning float widths for a run of text at a given Font state (size/style/scale), consistent with whichever route the original uses: Unity IMGUI `GUIStyle.CalcSize` on the selected default font (likely talk path), or Android `Paint.measureText` + `getFontMetrics` with SANS_SERIF (font-texture path). Neither is reproduced by fixed per-character widths or by an arbitrary desktop font; any desktop substitute must be validated against reference measurements.

## Port

- `src/core/gameplay/text_font.{hpp,cpp}`: `TextFontState` (requested_size +0x10, size +0x1c, use_preset +0x20, adjusted_size +0x2c, scale +0x40, font_scale +0x44); `set_text_font_size` (`_setSize`, preset table passed explicitly, `nullptr` = missing table), `update_text_font_scale` (`UpdateFontScale`), `adjust_text_font_size` (post-UpdateFontScale arithmetic of `_adjustFontSize`; caller owns dispatch). Strict binary32 intermediates; FCVTZS modelled without out-of-range casts. Test `text_font_contract` (`tests/test_text_font.cpp`): presets/literals, negative bounds, null table without presets, scale change, partial effects on exceptions, wrap32, ±0/∞/NaN, conversion limits. Last build 20/20 CTest on Windows.
- `src/core/gameplay/text_fit.{hpp,cpp}` (`prepare_text_font_size`, `fit_text_blocks`) uses a host font setter; see [text.md](text.md).
- Missing: font resource selection, language code/pack state, localization predicate/scale/stack, GUIStyle or Paint measurement, Font copy/dispatch semantics, any glyph rendering backend.

## Evidence

The current-slice font blocker uses complete ignored font-backend/M17759.asm, font-init-body/M17761.asm, font-metrics-core/F1b798f8.asm and mission-text-metrics/ exports. Space literal GOT0x272f400 ->cell0x27eb9d0 ->encoded0xa00000f9 resolves stringLiteral124 to U+0020. GameSkin bytes remain local in research/exports/mission-talk-font/GameSkin.bin; research/unity-assets.json records its object identity/offset. Confidence high for the fresh/default-skin route and wrapper arithmetic; desktop metric equivalence needs an original runtime oracle.

- [../archive/reports/96-text-lines-and-font-metrics.md](../archive/reports/96-text-lines-and-font-metrics.md), [97-font-texture-delegate.md](../archive/reports/97-font-texture-delegate.md), [98-android-font-measurement.md](../archive/reports/98-android-font-measurement.md), [99-default-font-resource-selection.md](../archive/reports/99-default-font-resource-selection.md), [100-language-pack-font-code.md](../archive/reports/100-language-pack-font-code.md), [101-talk-font-construction-and-startup.md](../archive/reports/101-talk-font-construction-and-startup.md), [103-initial-text-font-correction.md](../archive/reports/103-initial-text-font-correction.md), [104-font-localization-scope.md](../archive/reports/104-font-localization-scope.md), [105-font-adjustment-and-preparation-exceptions.md](../archive/reports/105-font-adjustment-and-preparation-exceptions.md), [106-native-font-numeric-state.md](../archive/reports/106-native-font-numeric-state.md), [107-font-presets-and-graphics-initialization.md](../archive/reports/107-font-presets-and-graphics-initialization.md), [108-font-backend-access-and-draw-scale.md](../archive/reports/108-font-backend-access-and-draw-scale.md)
- Local (`build/research/`): `font-width/`, `font-metrics-core/`, `font-texture-size/`, `font-backend/`, `font-backend-start/`, `font-selection/`, `font-selection-default/` (incl. M17770, M17820), `font-init-body/M17761.asm`, `font-language-selection/`, `language-font-origin/`, `pack-directives.json`, `language-init-callers.json`, `talk-font-boot/`, `talk-layout-constructor/`, `font-scale-correction/`, `font-scale-callers.json`, `font-localize-scope/`, `font-adjust-wrapper/`, `font-presets/initial-tables.json`, `font-type-reference-scan.json` (candidates only, has false positives), `font-store-candidates.json`, `font-java-classes.json`, `font-java-smali/`, `font-java-binding/`, `font-float-decoder/`, `font-float-bits/`; `research/local/unity-assets.json`.
- APK SHA-256 `d58943e6…6eead`, ELF SHA-256 `17a6cf3a…526f` (see [overview.md](overview.md)).

## Open questions

1. (blocking, runtime oracle) GUIStyle run widths/heights for the selected LegacyRuntime font and sizes/scales; compare any public desktop backend before claiming equivalent line breaks.
2. Effective live skin/restored-font selection outside the established fresh/default-skin English projection.
3. (blocking) Effective `requestFontScale_`, `Config.LOCALIZE_FONT_SCALE`, `Config.ASIA_FONT_SCALE`, application scale feeding `scaleRatio_`, and `Graphics`+0x5c at draw.
4. Effective language at boot: `Language.Get`, PlayerPrefs value, guard on config +0x60, `LoadLanguagePack` rollback scope.
5. `Font._setSize` preset table choice per caller; later writers of `HEIGHT`/`WIDTH`.
6. `_stringWidthF` space→dot cache condition and the application division factor.
7. Culture semantics of `ToLower`/`ToUpper`/`Trim`/`StartsWith(…,1)` used on language codes.
8. `TextBlock.Draw` (`0x1bc1838`) glyph route per block.
