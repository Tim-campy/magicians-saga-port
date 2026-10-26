# Unity serialized files and media (textures, FSB5 audio, fonts)

The APK's Unity data are standard serialized-file v22 assets (Unity 2022.3.62f2) read with UnityPy 1.25.4. Besides the 38 TextAssets (see [jar-container](jar-container.md)) they contain 73 Texture2D, 11 Sprite, 5 Font and 23 AudioClip objects. All are catalogued; `extract_assets.py --unity-media` exports 68 textures, 23 FSB/WAV pairs and 5 TTF fonts with structural validation. No in-game rendering or playback comparison exists.

## Layout

**Serialized files.** 1,182 logical files under `assets/` (1,149 v22). `globalgamemanagers.assets.split0/1/2` (1,048,576 + 1,048,576 + 855,944) concatenate numerically to 2,953,096 bytes (header: metadata size 25,694, data offset 25,744; SHA-256 `c00ef7e9…cd427`). 2,389 objects: MonoScript 1,071, Shader 1,101, ComputeShader 6, TextAsset 38, Texture2D 73, Sprite 11, Font 5, AudioClip 23, GameObject 3, MonoBehaviour 7. BuildSettings path ID 11 lists only `Assets/Scenes/main.unity`; `level0` GameObject `Main` (path ID 1): MonoBehaviours 11/12/13 -> MonoScripts 883/492/881 (`main.Main`, `system.tapjoy.MyTapjoy`, `system.notification.FCMReceiver`), plus `TapjoyUnity` 14/15. ResourceManager (Unity) has 1,161 named entries. TextAssets live in hashed files under `assets/bin/Data/`, path ID 1 (e.g. `xls` 419,822 bytes, `enemy` 1,393,315, `quest` 1,193,989, `connect` 60).

**AudioClip.** `m_Resource.m_Source/m_Offset/m_Size` point into `.resource` members; all 23 ranges in bounds. Each range is FSB5 version 1, one subsound, codec 0x0f (Vorbis), 60-byte header; no loop chunk (type 3). 3,784,096 bytes FSB -> 74,347,948 bytes WAV, all PCM s16le stereo 44,100 Hz; frame counts equal FSB header counts; max duration delta vs Unity float 0.0000032393 s. Example: `se_button` 31,353 frames.

**Font.** `m_FontData` holds raw TrueType (`00 01 00 00`): `default-th` 33,872 (Noto Sans Thai), `default-en-switch` 2,072,612 (M+ 1p medium), `default-hi` 86,736 (Kairo Hindi Thin), `default-ja` 1,920,716 (m_plus), `LegacyRuntime` 50,788 (Liberation Sans; path ID 10102 in `unity default resources`, original `OS/2`/`hhea` checksum mismatches, global 0xb249af98, preserved). The four content fonts have global checksum 0xb1b0afba and valid table checksums.

**Texture2D.** 68 decode via `texture.image.convert('RGBA')` with pixel-identical PNG round trip; 5 font textures have zero dimensions/empty payload and are recorded, not invented. Names are not sizes (`1080x2160` is 1024x2048).

## Reading it

- `tools/inspect_unity_assets.py`: catalogue only (no media decoding/writes), split-file validation (gaps, duplicates, size mismatch, out-of-range objects rejected), Shannon entropy per TextAsset, hostnames only for URL strings. Deterministic JSON `research/local/unity-assets.json`.
- `tools/extract_assets.py --unity-media`: 187 extra outputs (68 PNG + 68 original texture buffers, 23 FSB + 23 WAV, 5 fonts). Audio via `fmod_toolkit.raw_to_wav(..., convert_pcm_float=False)` (fmod_toolkit 0.1.3 / pyfmodex 0.7.2, FMOD Windows x64), exactly one output required, WAV validated with `wave` and ffprobe 7.1.1 (FFmpeg cannot read the FSB directly). Fonts validated per OpenType spec (incl. `head.checkSumAdjustment`), opened with FreeType via Pillow. Only Windows tested.

## Evidence

- [10-unity-assets](../archive/reports/10-unity-assets.md), [14-unity-media](../archive/reports/14-unity-media.md), [12-asset-extraction](../archive/reports/12-asset-extraction.md)
- Local: `research/local/unity-assets.json`, `research/local/unity-texture-export.json`, `build/research/media/manifest.json`, `build/research/unity-textures/`, `build/research/media/fonts/*.png`

## Open questions

1. Loop/volume/pitch/streaming policy requested by game code (no FSB loop chunks).
2. Font selection, glyph coverage, Thai/Hindi shaping and metrics vs Unity.
3. Role of the 11 Unity Sprites (`108f/b`…`432f/b`) and the custom MonoBehaviour fields (undecoded).
4. Memory/streaming budget for 74 MB of PCM on 3DS.
