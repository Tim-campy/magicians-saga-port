# IL2CPP metadata v31 and method code map

`assets/bin/Data/Managed/Metadata/global-metadata.dat` (7,792,604 bytes, SHA-256 `8f79c53ce22e97269f7ac7dd932cd4d80ed83169a5a61f9b9a85fc26d08a0cbd`) is a Unity IL2CPP v31 metadata file: names, types, methods, fields and parameters of all 64 managed images. Method bodies live in `lib/arm64-v8a/libil2cpp.so` (42,888,064 bytes, SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`); the map from method token to code address goes through `Il2CppCodeRegistration` → `Il2CppCodeGenModule[64]` → per-image method pointer tables. Both are parsed by project tools and feed the function registry `re/functions.csv`.

## Layout

### global-metadata.dat (little-endian)

Header 256 bytes: magic `0xFAB11BAF` (u32), version `31` (s32), then offset/size pairs of the sections. Record sizes used (v31, per Il2CppDumper `MetadataClass.cs` @ `4741d46`):

| Record | Bytes | Notes |
|---|---:|---|
| Header | 256 | |
| Type definition | 88 | includes vtableStart, byvalTypeIndex |
| Method definition | 36 | includes `returnParameterToken` (v31) |
| Image definition | 40 | |
| Assembly definition | 64 | |
| Field / Parameter definition | 12 | |

Observed sections:

| Table | Records | Offset (dec) | Bytes |
|---|---:|---:|---:|
| Images | 64 | 7,253,524 | 2,560 |
| Assemblies | 64 | 7,256,084 | 4,096 |
| Types | 8,619 | 6,495,052 | 758,472 |
| Methods | 63,273 | 1,924,904 | 2,277,828 |
| Fields | 39,053 | 5,478,308 | 468,636 |
| Parameters | 71,337 | 4,622,264 | 856,044 |

Game images: `KairoLibrary.dll` (image 1, 645 types 1705–2349, 6,771 methods), `Assembly-CSharp.dll` (image 4, 440 types 4111–4550, 4,704 methods), `Assembly-CSharp-firstpass.dll` (image 15, 75 types 7747–7821, 215 methods). 64 images ≠ the name count of `ScriptingAssemblies.json` (different sets; reason not established).

Identifiers:
- **Method index**: global row in the Methods table (e.g. `main.Main.OnCreate` = 34537). Metadata file offsets of types (e.g. `main.Main` type 4166 at `0x68b35c`) are not code addresses.
- **Token**: `0x06000000 | RID`; RID is 1-based within the image. Registry ids are `game:<token>` / `kairo:<token>`.
- **Virtual slots**: `vtableStart` indexes a vtable-entry array of encoded method references (e.g. `0x60009a37`, decoded with the v27+ rule of Il2CppDumper `Metadata.cs`). At runtime the native class places slot n at `class + 0x138 + 16n` (method pointer + MethodInfo pair; corroborated on several classes).
- **Encoded type references** in metadata-usage slots: e.g. `0x20004fef` → native index 10231 → type via `byvalTypeIndex` (= type 1778 `kairo.common.cfg.Config`). Used to identify GOT-referenced static blocks (`class+0xb8` = static fields).

### libil2cpp.so code map (ELF64 LE, ET_DYN, AArch64, load bias 0)

| Element | ELF VA |
|---|---|
| `Il2CppCodeRegistration` (candidate base, probable) | `0x2604920` |
| `codeGenModulesCount` (= 64) | `0x2604998` |
| `codeGenModules` | `0x26049a0` |
| module pointer array | `0x27a0798` |
| `Assembly-CSharp.dll` module / name / method table (4,704) | `0x2625400` / `0x6375a9` / `0x27a89e8` |
| `KairoLibrary.dll` module / method table (6,771) | `0x2708df8` / `0x28740c8` |

`Il2CppCodeGenModule` ARM64 v31 start/size per Il2CppDumper `Il2CppClass.cs`. Method pointer for token t is entry `(t & 0x00ffffff) - 1` of the module's table (not the global method index). Every slot is filled by a dynamic relocation: `R_AARCH64_RELATIVE` (1027, value = addend) or `R_AARCH64_ABS64` (257, symbol + addend). Example: `main.Main.OnCreate` token `0x0600026c` → slot `0x27a9d40`, RELA record at file `0x45a868`, addend `0x117c78c` (file offset `0x117878c`).

ELF relocation inventory: 257,114 RELATIVE, 1,631 ABS64, 565 JUMP_SLOT, 153 GLOB_DAT.

Function extents: `.eh_frame` FDEs (137,201) give unwind ranges, used as method bounds (e.g. OnCreate `0x117c78c..0x117cdc8`, 1596 bytes). The next mapped address is only a search bound, not a function end.

Coverage: 63,273 methods; 56,955 non-null pointers to distinct executable VAs; 6,318 null slots (abstract/generic causes not classified). Game modules: 11,690 records, 11,174 non-null. Tables entirely in BSS are zero memory (e.g. 6-slot table of `UnityEngine.Purchasing.AppleCore.dll` at `0x28fce50`).

## Reading it

- `tools/inspect_il2cpp_metadata.py <apk> --output research/local/il2cpp-metadata-inventory.json` reads the ZIP member directly. Rejects other versions, out-of-file or overlapping sections, short records, bad strings, token mismatches and inconsistent ownership. Checks: every method points back to its owner type; every type is covered by exactly one image. Two runs identical; nine malformed variants rejected.
- `tools/map_il2cpp_methods.py <libil2cpp.so> <inventory.json> --output research/local/il2cpp-code-map.json`. Validation chain: ELF header/PT_LOAD/RELA checks; find the 64 image name strings, follow RELATIVE relocations to them; accept module candidates whose name, method count, pointer table (inside PT_LOAD) and unique RIDs `1..count` match; every non-null pointer must be 4-byte aligned in an executable segment; corroborated by the 64-entry module array referenced by a pointer preceded by count 64 in the CodeRegistration tail. Two runs identical; six invalid inputs rejected (three truncations, wrong machine, callback relocation redirected to a non-executable string, duplicate RID).
- `tools/inspect_unwind_ranges.py` (FDE ranges), `tools/inspect_arm64_calls.py` (BL/B/BLR/BR inside a range, direct targets resolved through the code map).
- `tools/registry.py build research/il2cpp-code-map.json` generates `re/functions.csv` (game + kairo methods only).
- Output JSON contains proprietary names; kept local (gitignored).

## Evidence

- [../archive/reports/05-il2cpp-metadata.md](../archive/reports/05-il2cpp-metadata.md), [07-il2cpp-code-map.md](../archive/reports/07-il2cpp-code-map.md), [09-native-callbacks.md](../archive/reports/09-native-callbacks.md) (FDE bounds, vtable slots), [13-animation-formats.md](../archive/reports/13-animation-formats.md) (vtable origin `0x138` corroboration), [27-frame-policy.md](../archive/reports/27-frame-policy.md) (encoded type refs)
- External schema: Il2CppDumper `MetadataClass.cs`, `Il2CppClass.cs`, `Il2Cpp.cs` (`GetMethodPointer`), `Metadata.cs` at commit `4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a`; Arm AAELF64 dynamic relocations.

## Open questions

1. Other `Il2CppCodeRegistration` fields and `Il2CppMetadataRegistration` (generic instances, type registration) are not validated.
2. Causes of the 6,318 null slots (abstract, generic sharing, stripped) are unclassified.
3. Runtime confirmation of the registration (no process capture yet).
4. Schema was taken from a third-party tool; not checked against the game's own runtime reader.
5. Field offsets per type (instance/static layout) are not extracted systematically; docs use raw offsets.
