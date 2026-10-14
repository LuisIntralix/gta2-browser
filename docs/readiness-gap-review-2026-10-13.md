# Readiness and gap review: can we reach a fully playable browser GTA2?

## Status update, later on 13 October 2026

User decisions:

- unshield approved.
- A quick Windows environment approved.
- **Scope: single player first, with co-op seams wired in so co-op doesn't
  need a rewrite.**
- **Purpose: own play with friends, education, and benchmarking AI
  capability.** No public hosting is planned.

| Item | Status |
|---|---|
| B1 game data | **Done.** 344 files extracted into `game/` with a hashed manifest. `gta2.exe` is byte-identical to upstream's `9.6f.exe`. Reproduce with `scripts/extract/extract_game.sh`. |
| A1 9.6F data vs reconstructed loaders | **Header-level pass.** STY v700 and GXT v100 match the versions the loaders verify. Content-level parity is still untested. |
| B3 reference host | **Done, via Wine instead of a VM.** The original 9.6F game runs on this Mac with scripted input. It has no audio, and startup needs retries. See [reference-runtime.md](reference-runtime.md). |
| B2 standalone playability | **Partly answered.** Upstream's CI-built standalone boots to the menu and takes input with the 9.6F renderer DLLs. Getting into gameplay is unconfirmed under Wine (focus handling). Our own browser build gives better visibility than Wine here. |
| S1 scope | Decided: single player first, co-op seams. |
| S5 legal | Decided: private use. The D16 gate stays for any wider distribution. |

Review date: 13 October 2026. Scope: the workspace as it stands at commit
`62f6ecc` plus uncommitted `port/` headers and the T02 inventory. Static review
only. Nothing was executed against game data, because no game data has been
extracted yet.

## Verdict

**The plan and the code base are strong. The missing pieces are inputs and one
key piece of evidence.** We don't need more architecture research. Three
things stop real progress:

1. **No game files.** `game/` is empty, and T02 is blocked because nothing on
   this host can unpack the InstallShield 7 cabinet `data2.cab`.
2. **Nobody has seen the reconstruction run.** The plan assumes `gta2_re` can
   run on its own. Upstream ships a `--run_standalone` mode, but no one in this
   workspace has watched it boot and play.
3. **No Windows machine** to run the original 9.6F reference or the upstream
   standalone build.

All three can be resolved quickly. Until they are, every task after T02 is
working blind.

## Why Vice City and Eaglercraft are the right comparison, and where GTA2 differs

| | reVC (Vice City in the browser) | Eaglercraft | GTA2 here |
|---|---|---|---|
| Starting point | re3/reVC were **complete, standalone, cross-platform** engines (OpenGL via librw) that had run natively on Linux/macOS for years | Decompiled Java client with a mature build | Matching decompilation for MSVC6/x86. Standalone mode exists, but its playability is unverified here |
| Main browser work | Emscripten port of an engine that already worked | Rewrote the Java runtime layer (TeaVM) | Write a new platform layer (renderer, frontend video, audio, input, loop, files), then check behaviour |
| Multiplayer | Not added | Built into Minecraft already | **Spec adds 2–4 player campaign co-op that the original game never had** |
| Legal outcome | Take-Two sent DOS.Zone a DMCA notice in Dec 2025, even though players had to supply their own files | Takedowns | Same rights holder. No `gta2_re` licence |

The successful ports worked because a native, standalone engine existed first,
so the browser step was "only" porting. For GTA2 the equivalent first step is
proving the `gta2_re` standalone exe plays. That is the most important piece of
evidence the project doesn't have.

## What we have

- Pristine, checksum-verified 9.6F installer. A full static map of its
  container, with 376 manifest names (T02 inventory, `research/runtime-data/inventory/`).
- Pinned `gta2_re` (`02de8be…`): 3,400 MATCH, 21 WIP and 9 CRT-wrapper STUB
  functions. That covers the mission VM, physics, AI, police, traffic, map/style
  loaders and saves. Upstream CI builds and verifies it.
- An upstream standalone path. `CMakeLists.txt` links `decomp_main` against the
  GTA2Hax reimplementations of the two renderer DLLs (`d3ddll.dll`,
  `DmaVideo.dll`). Source comments such as "global crashing standalone" show
  upstream does run it.
- Emscripten 6.0.12. A math probe that passes in a real browser.
- A complete spec, binding decisions D01–D16, 33 tasks, gates G0–G6 and a run
  ledger. T00 and T01 are accepted.

## Size of the platform layer

Measured from the pinned source:

| Windows dependency | Surface to replace | Reference available |
|---|---|---|
| Renderer + frontend DLLs (`d3ddll`, `DmaVideo`) | **64 exports** in total | GTA2Hax D3D9 reimplementation, ~3,300 lines |
| Miles Sound System | **35** distinct `AIL_*` calls | Empty shims only. SDT/RAW audio is raw PCM, so Web Audio is simple |
| Bink | **21** distinct `Bink*` calls | One file (`intro.bik`). Convert with FFmpeg or skip |
| DirectInput | Used in 5 source files | Map to DOM keyboard/mouse |
| DirectPlay | `NetPlay.*` | Only needed for co-op |
| Registry / Win32 loop / `timeGetTime` | 5 registry calls, `winmain.cpp` loop | `emscripten_set_main_loop` plus a config file |

This is a bounded amount of work, small enough to finish. It isn't a hidden
mountain.

## Gaps, ranked

### Blockers (resolve before any further implementation work)

| # | Gap | Fix |
|---|---|---|
| B1 | **Game data not extracted.** No 7z, unshield, innoextract, Homebrew or Wine on this host. | Build [`unshield`](https://github.com/twogood/unshield) (C, CMake, zlib) from source into ignored `research/toolchains/`. That needs no global install. It supports InstallShield 5–7-era cabinets. Alternatively, run the installer once in a Windows VM and copy the installed tree into `game/`. **Needs user approval** (new tool download). |
| B2 | **Standalone `gta2_re` playability unknown.** | On a Windows VM, build upstream (its CI recipe) and run `--run_standalone` against the extracted data. Record: boots? frontend? drives? completes a mission? crash list? Alternatively, ask on the `gta2_re` Discord. This one result decides whether the project is "port a working game" or "finish the decompilation, then port". |
| B3 | **No Windows host** for D01 reference capture and B2. Host is Apple Silicon (arm64, macOS 26.7.1). | Windows 11 ARM in UTM, Parallels or VMware Fusion runs x86 games through Prism emulation. GTA2 is light enough. **User decision** (software/licence). D01 rules out Wine. That was a choice made for cost reasons. CrossOver/Wine would be a faster way to get an oracle and could be reconsidered. |

### Unverified assumptions (need evidence right after B1)

| # | Assumption | How to check |
|---|---|---|
| A1 | 9.6F data files work with the 10.5 code. The two exes differ: 10.5 is the "freeloader" build, according to upstream's `gta2_data_setup.py`. | After extraction, check GBST/GBMP versions, the `.scr` compiled-script format and `.gxt` headers against the parsers in `gtx_0x106C.cpp`, `map_0x370.cpp` and the script loader. A mismatch would hit every mission. |
| A2 | The 32-bit layout code works on wasm32. It contains pointer-shaped data fields (`gtx_0x106C.cpp:470`, `map_0x370.cpp:4658`). | wasm32 has 4-byte pointers, which helps. Compile `gtx`/`map` loaders under Emscripten and load real `.sty`/`.gmp` as the first core milestone. |
| A3 | WIP/"fake match" functions behave correctly (`CarAI_78` state machine, trains, `TextureCache` bounds guard). | Only runtime play exposes these. Covered by B2 and G2. |

### Scope and process risks (decisions for the user)

| # | Risk | Recommendation |
|---|---|---|
| S1 | **Co-op campaign more than doubles the scope.** The audit puts co-op at 60–200M of the 160–550M tokens. The original game never had campaign co-op. Every mission needs authority rules (SG-COOP-01). | Define "fully playable" as **single player first (G3)**. Treat co-op as phase 2, re-estimated after G2. Vice City's port shipped without new multiplayer. |
| S2 | **D02 makes the renderer harder than it needs to be.** It bans SDL and requires a pure-JS WebGL2 renderer that consumes a C++ render view. The reVC browser build compiled its GL backend straight through Emscripten. | A C++ GLES3 implementation of the 64 `gbh`/`DmaVideo` exports, compiled to WebGL2 by Emscripten, is shorter. It can reuse GTA2Hax's structure, and it keeps draw calls out of the JS↔Wasm boundary. Consider reopening D07 under its own rules. |
| S3 | **Ceremony comes before the first pixel.** T04 is writing contract headers (`port/include/gta2/*.h`, ~1,300 lines) while no code has touched real data. | After B1, run a time-boxed **spike**: compile the map/style loaders and draw one district in the browser. Then fit the contracts to what actually works. |
| S4 | **The model routing in the spec doesn't match reality.** The spec names GPT-6.1 Sol/Astra. The run actually uses DeepSeek Flash/V4 Pro (`environment.md`). The cost tables assume the spec models. | Re-baseline the cost forecast at G2 using measured usage, as the spec already requires. |
| S5 | **Publishing is legally exposed.** `gta2_re` has no licence (T01). Take-Two used DMCA against the reVC browser port even though players supplied their own files. Rockstar's 10 Sept 2026 mod guidelines reportedly forbid running its games on unofficial platforms. | A private local build is achievable. Public hosting carries a high takedown risk whatever the asset model. Keep D16's gate. Decide the end goal (personal play vs. public site) before investing in R07/R08 release work. |

## Research no longer needed

Formats (GMP/STY/SDT/GXT/SCR), mission VM behaviour, physics and AI, browser
API choices, save design and timing model are all covered by the audit,
decisions and upstream source. More surveying would cost tokens without
reducing risk. The one external project found, [Kajakklubben/gta2-webgl](https://github.com/Kajakklubben/gta2-webgl),
is an abandoned 2021 three.js map viewer. It's useful only as a sanity
reference for the map format.

## Recommended next steps, in order

1. Approve B1 (local unshield build, or a VM install). Extract into `game/` and
   finish T02 with real hashes.
2. Check A1 (do the 9.6F data headers match the 10.5 parsers?).
3. Approve B3 (Windows 11 ARM VM). Run upstream standalone (B2) and a short 9.6F
   reference capture (T03).
4. Run the spike from S3. Then decide on S1, S2 and S5 and re-estimate.

## Sources

- Workspace: `docs/gta2-re-build-audit-2026-10-12.md`, `docs/decisions/permissions.md`,
  `docs/orchestration/{gta2-browser-spec,decisions}.md`, run ledger, T02 inventory README.
- Upstream: `research/gta2_re/{README.md,CLAUDE.md,CMakeLists.txt,build.py,Scripts/gta2_data_setup.py}`.
- [Take-Two DMCA against DOS.Zone Vice City port (80.lv)](https://80.lv/articles/take-two-takes-down-browser-version-of-gta-vice-city-with-dmca-request)
- [Quenq Vice City port and Rockstar 2026 mod guidelines (Dexerto)](https://www.dexerto.com/gta/gta-vice-city-is-playable-in-your-browser-again-after-take-two-takedown-3406096/)
- [gta2-webgl listing (LibertyCity)](https://libertycity.net/files/gta-2/154261-gta2-on-webgl.html)
