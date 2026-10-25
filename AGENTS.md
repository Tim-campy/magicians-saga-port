# AGENTS.md — Magician's Saga Native Port

## Mission
Reverse engineer a legitimately obtained Android APK of **Magician's Saga** (Kairosoft; package commonly identified as `net.kairosoft.android.wizardquest`) and build a **native**, behaviorally faithful port for **Windows and Linux**, with an architecture suitable for a later **Nintendo 3DS** port.

The APK is the source of truth for this specific version. Do not assume its engine, implementation language, native libraries, asset formats, network requirements, or complete offline functionality before inspecting it.

## Operating principles
1. **Inspect before implementing.** Inventory files, ABIs, DEX, native libraries, assets, metadata, permissions, and dynamic dependencies. Record APK version and SHA-256.
2. **Evidence over guesses.** Every important claim about game behavior must point to a class/method, native function/address, resource, controlled experiment, or external reference. Mark unknowns explicitly.
3. **Preserve original behavior.** Prioritize accurate rules, timing, UI flow, progression, saves, RNG behavior, and asset presentation. Never silently substitute approximations.
4. **Iterate in small verified steps.** Plan, investigate, implement, test, document. Keep builds reproducible and changes reviewable.
5. **No Android runtime in the final PC build.** Target native Windows/Linux executables; Android emulation is permitted only as a research oracle.
6. **Prepare for 3DS from day one.** Keep gameplay code independent from platform APIs, minimize heap churn, avoid unnecessary dependencies, and budget for memory, texture sizes, CPU, screen layout, and input limitations. Do not claim 3DS feasibility until measured.
7. **Use legal copies and authorized environments.** Do not redistribute the proprietary APK, original assets, credentials, or decompiled proprietary source. Keep game data user-supplied and local. Do not attack services or circumvent payment/licensing controls.

## Available tools and REA skills
- At session start, discover installed **REA (Reverse Engineer Anything)** skills, agents, MCP tools, commands, and their documentation. Read the relevant skill instructions before using them. Do not invent tool names or syntax.
- Prefer REA-supported workflows for binary triage, static analysis, symbol/function discovery, cross-references, strings, decompilation, call graphs, and structured notes when available.
- Use **Ghidra** for native `.so` analysis; **JADX** for DEX/Java; **apktool** for resources/manifest; `unzip`, `file`, `readelf`, `nm`, `strings`, `sha256sum` for triage; **ADB/logcat** and an Android emulator or test device for controlled runtime observation when available.
- Check tool availability first; explain missing prerequisites and propose installation steps without silently changing the host system.
- When REA and another tool disagree, record both observations and investigate rather than guessing.

## First milestone — APK reconnaissance (mandatory)
1. Locate the user-provided APK. Do not download a random replacement without asking.
2. Record file hash, package ID, version, min/target SDK, supported ABIs, entry activities, permissions, DEX count, `.so` list, and asset directory summary.
3. Determine whether gameplay is in Java/Kotlin, native ARM code, embedded scripts, or a mixture. Identify frameworks only with evidence.
4. Map startup and game loop, rendering, audio, input, persistence, resource loading, and any service dependencies.
5. Identify whether all gameplay resources are embedded in the APK or require expansion files, downloads, or other packages.
6. Produce `docs/01-apk-inventory.md`, `docs/02-architecture.md`, and `docs/03-open-questions.md` before substantial porting work.

## Reverse-engineering workflow
- Build a subsystem map: boot, UI, scene management, rendering, input, audio, combat, entities, AI, progression, inventory, economy, saves, and asset formats.
- For each subsystem, document original symbols/addresses or DEX classes, evidence, dependencies, confidence level (`confirmed`, `probable`, `unknown`), and a test plan.
- Preserve original identifiers in research notes, but use clean, maintainable naming in the new implementation.
- Investigate resource file headers, compression, serialization, sprite atlases, animation data, and fonts. Write deterministic extractors; never modify the original APK.
- Use the original game in a local, authorized test environment as a behavioral reference. Capture reproducible inputs and observable outputs; redact sensitive information.
- Prefer focused experiments and differential testing over broad speculative reconstruction.
- Maintain `docs/research-log.md` with date, question, method, findings, evidence, confidence, and next action.

## Port architecture
Suggested layout (adjust after reconnaissance):

```text
AGENTS.md
README.md
docs/
tools/
tests/
src/
  core/          # platform-independent game logic
  assets/        # resource loaders, not proprietary asset dumps
  platform/
    desktop/     # Windows/Linux
    n3ds/        # later milestone
```

- Prefer **C++17** for the portable core unless APK evidence strongly favors a different strategy; document the tradeoff before committing.
- Keep rendering, audio, input, filesystem, timing, networking (if any), and save storage behind narrow interfaces.
- Consider SDL2 for desktop and devkitPro/libctru plus suitable graphics/audio libraries for 3DS, subject to compatibility and measurements.
- Use fixed or explicitly controlled timestep where appropriate; isolate RNG and serialization for deterministic tests.
- Keep game content separate from engine code; load assets from an extraction output supplied by the user.
- Use CMake for desktop builds where practical. Never make the desktop implementation depend on Android APIs.

## Validation and milestones
- **M0:** APK inventory and feasibility report.
- **M1:** Asset extraction and rendering prototype, with verified sprites/fonts/audio.
- **M2:** Boot/menu navigation and input fidelity.
- **M3:** First playable gameplay loop with deterministic tests.
- **M4:** Save/load, progression, economy, and content coverage.
- **M5:** Windows/Linux release candidate with performance and regression testing.
- **M6:** 3DS feasibility benchmark (RAM, VRAM, CPU, textures, controls, display); then incremental 3DS port if viable.

Each milestone requires: documented evidence, a reproducible build/run command, tests or comparison captures, known limitations, and a clear acceptance checklist. Never declare a feature complete merely because it compiles.

## Agent execution rules
- Before major changes, state the goal, files to inspect, risks, and a short plan.
- Use specialized REA skills or subagents for bounded tasks (DEX mapping, ELF analysis, asset formats, test harness) when actually available. Do not assume multiagent features exist.
- Keep a concise `docs/progress.md` with completed, active, blocked, and next tasks.
- Prefer incremental patches; do not mass-generate speculative code or refactor unrelated areas.
- Run relevant tests after edits. If tests cannot run, state why and give exact commands for the user.
- Ask for user input only when a missing APK, required tool, or material design decision blocks progress.
- Never fabricate measurements, extracted files, reverse-engineering results, or successful tests.

## Repository maintenance (established after initial reconnaissance)

- Follow `CONTRIBUTING.md` for local Git commits and review staged files before committing. Never commit proprietary inputs, extracted data, local research dumps or credentials.
- Use `docs/README.md` as the report index, grouped by M0–M6 chapters. Preserve existing report paths; add new reports to the relevant chapter.
- Record implementation and organization changes in `CHANGELOG.md`; keep `docs/progress.md` concise and preserve detailed findings in `docs/research-log.md`.
- Place portable code in the corresponding `src/core/` module and maintain its CMake target. Current user priority is gameplay.

## Initial instruction to Codex (historical starting point)
Start at **M0**. Discover and read available REA skills and inspect the workspace. Locate the supplied Magician's Saga APK, perform non-destructive reconnaissance, create the three mandatory reports and `docs/progress.md`, and recommend the next narrowly scoped task. **Do not begin a wholesale rewrite or port until the architecture is evidenced.**
