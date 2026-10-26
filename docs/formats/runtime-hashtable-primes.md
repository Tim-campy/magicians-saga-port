# Runtime Hashtable prime table

The shipped mscorlib HashHelpers uses an ordered 72-entry capacity table. The port reads it from the maintainer's APK instead of embedding it.

## Layout

The extracted `runtime/hashtable-primes.bin` is exactly 288 bytes: 72 consecutive little-endian int32 values, with no header or padding. Entries are positive, strictly increasing primes. Their stored order is significant to GetPrime. No original table contents are distributed.

## Reading it

Run `tools/extract_assets.py` with `--runtime-tables` using the commands in [setup](../SETUP.md). The extractor verifies the APK SHA-256, metadata signature/version 31, default-data section bounds and the 288-byte initializer range. It records the output size and SHA-256 in `manifest.json` under `runtime_tables`. `--check --runtime-tables` requires exactly this inventory and checks its content alongside the other extracted files. Legacy manifests remain accepted when this option is not requested. Differing existing files are never overwritten.

[runtime_data.cpp](../../src/platform/desktop/runtime_data.cpp) reads exactly 288 bytes and rejects trailing bytes, nonpositive/out-of-int32 entries, non-primes and non-increasing entries. It returns shared immutable data to the concrete call-context host. These structural checks do not authenticate the table; run the manifest check first. Synthetic tests deliberately use another valid prime sequence.

## Evidence

HashHelpers..cctor at ELF `0x1e2fe44` allocates 72 int32 elements and initializes them through GOT `0x274c8f0`. Metadata field-ref 479 resolves field 7152, whose initializer starts at v31 default_data +50056. Bounded exports are local under `research/exports/hashtable-primes/`. Confidence high.

Local extraction to `extracted/runtime` matched the independently read 288-byte initializer exactly. The manifest check and desktop integration test passed using that output. Synthetic tests cover bounds, version, manifest inventory/integrity, structural decoding and copies of contexts with actual bucket growth. No Android comparison is claimed.

## Open questions

None for this fixed-version initializer. Cross-version compatibility requires new evidence and an updated APK/layout identity.
