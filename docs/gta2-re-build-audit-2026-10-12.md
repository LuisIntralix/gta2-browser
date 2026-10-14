# GTA2 reconstruction: build, reuse gaps and revised budget

Audit date: 12 October 2026. Target: `CriminalRETeam/gta2_re` revision
`02de8beeb766644417f195d360eba33af0ee491f`.

**Substantial game code is reusable in principle. A small adapted portion now
executes successfully in a browser. The full browser game remains unbuilt and
untested.** Existing upstream Windows/Wine CI passes its 3,400-function checks;
our browser probe passes 44,577 arithmetic/geometry checks. Neither establishes
complete single-player gameplay, co-op or 60 FPS. [DEPS-07, BUILD-MATH-01,
BUILD-BROWSER-01]

**Revised conditional planning allowance: 160–550 million cumulative model
tokens**, including reserve, versus the earlier 200–600M. At an illustrative
80% GPT-6.1 Sol / 20% GPT-6 Astra token mix, Standard API model usage is roughly
**US$920–$3,170**. All-Astra is **US$2,560–$8,800**. These are low-confidence
engineering scenarios, not measured completion forecasts or the bill for this
chat. Reuse permissions, engineering labor and operations are separate.
[EST-AUDIT-01, PRICE-01]

## What was built and tested

The requested [REA skill](../.claude/skills/reverse-engineer-anything/SKILL.md)
routes source-repository analysis to normal repository tools. Its instruction
to assign independent questions supported separate source and dependency/CI
audits. Following the user's model correction, inherited-model workers were
stopped; the two replacement audit workers used explicit GPT-6.1 Sol/high.
No further Luna research was assigned. No new native REA session was opened.

The checkout and selected submodules were pinned locally. Emscripten **6.0.12**
was installed inside ignored `research/toolchains/emsdk`, without global setup.
The original game installer/assets were not executed or modified. No original
executables or proprietary compiler package were fetched for this audit.
The upstream checkout remains clean. [BUILD-ENV-01]

| Probe | Observed result | What this establishes |
|---|---|---|
| Existing exact-revision upstream CI | Windows VC2022, Windows VC6, Linux/Wine VC6 and reccmp jobs report success; VC6 logs show **3,400/3,400 functions OK** | A real Windows/x86 build and matching pipeline exists. This was retrieved CI evidence, not locally reproduced Windows execution. |
| Local macOS CMake configuration | Fails on GTA2Hax's Microsoft-compiler conditional when `MSVC_VERSION` is empty | Current build files assume a Windows-oriented environment. |
| Local Emscripten configuration | Succeeds with `MSVC_VERSION=0` command-line override | Configuration only; not compilation. |
| Full `decomp_main` WebAssembly build | Fails. First on Microsoft export syntax; after enabling MS extensions, on Miles platform detection, `windows.h` and `objbase.h` | The existing full target cannot be converted by a compiler switch alone. |
| Adapted isolated math modules, native ARM64 | **44,577 checks, zero failures** | Selected arithmetic, angles, point geometry and bit-set behavior passes independent expected-value checks. |
| Same modules, WebAssembly in Node | **44,577 checks, zero failures**, same checksum `3539616277` | Selected compiled code behaves consistently across these two targets. |
| Same WebAssembly, actual Codex in-app browser | Page visibly reports **44,577 checks, zero failures**, same checksum | A bounded piece of the reconstruction actually executes in a browser. |
| Native UBSan, negative fixed-point division | Reproduces `left shift of negative value -16384` at copied `fix16.cpp:42` | Real C++ undefined behavior needs attention for portability/determinism. This is not an observed gameplay failure. |

Evidence: DEPS-07, BUILD-NATIVE-01, BUILD-WASM-01, BUILD-MATH-01,
BUILD-BROWSER-01 and BUILD-UB-01 below. The reccmp variable-comparison step is
allowed to fail and reports **13 issues**; overall CI success does not mean all
variables passed. Its other percentages use different denominators and are not
game-completion percentages. CI's `--check_vars_only` exits before gameplay.

The math probe compiles `fix16.cpp`, `ang16.cpp` and `Fix16_Point.cpp`, plus
headers. It uses **adapted copies**: const-reference/exception-signature fixes,
compiler annotations, an empty Windows header for these API-free modules, and
the original zero value of `gFix16_6777CC` supplied by the harness. Algorithms
were not rewritten. Link-time removal excludes unused functions. The 44,577
checks are repeated test vectors across a small operation set, not 44,577
different functions or comprehensive numeric-domain coverage. No original EXE
oracle was used. Both native and Wasm use four-byte `wchar_t`; Wasm uses four-
byte pointers while ARM64 uses eight. ABI assumptions still require review.

Reproduce the local native/Wasm/sanitizer probe, with the installed tools and
pinned checkout present:

```sh
python3 scripts/run_gta2_math_probe.py
python3 -m http.server 8765 --bind 127.0.0.1 --directory research/probes
```

Open `http://127.0.0.1:8765/` for the browser probe. The runner downloads nothing
and does not launch the game. It writes into ignored `research/probes` and the
audit's `build/reproduction` evidence directory. The audit's temporary server
was stopped after verification.

## How much can be reused

The reproducible source scan found **3,400 MATCH, 21 WIP and 9 STUB** markers,
with no duplicate addresses. All nine STUBs are functioning CRT wrappers, and
all 21 WIPs have bodies. There are extensive implementations of the mission
VM, campaign loading/saving, players, vehicles, collision/physics, traffic,
police, and GMP/STY readers. The old two-thirds figure is stale for this pin.
[SG-COUNT-01, SG-GAPS-01, SG-SYSTEMS-01]

The repository's v10.5 inventory contains 4,453 function addresses. MATCH-marked
entries represent **76.35% of those addresses and 85.97% of their listed byte
sizes**. Most of the 1,023 unmarked addresses are library/import/null-function
or helper candidates. This classification uses names, address ranges and
upstream documentation; original reachability was not independently proved.
The inventory excludes external DLL implementations and is not an independent
complete binary inventory. **No defensible percentage of the requested browser
product is “done” follows from these counts.** [SG-COUNT-01, SG-UNMARKED-01]

| Area | Reuse supported by evidence | Remaining work / confidence boundary |
|---|---|---|
| Core world, vehicles, AI, formats and mission VM | Substantial source plus upstream matching results | Resolve semantic discrepancies, 32-bit layouts, arithmetic and RNG assumptions; validate all districts/missions against the original. Only the math subset ran locally. |
| Single-player completeness | Campaign and persistence paths are implemented | Standalone boot/playthrough remains unknown. WIP vehicle AI, source-reported train/global-layout trouble and texture-bound guards need runtime checks. |
| Graphics | Reconstructed rendering front-end, formats, API boundary; GTA2Hax Direct3D9 reference backend | Build browser backend, batching, texture/palette handling, camera/culling/HUD scaling. D3D9 window resizing does not prove modern-resolution quality or FPS. |
| Audio/video | Sound scheduling/policy and interface descriptions | Miles/Bink `.cpp` files are empty linking shims. Browser playback/decoding/import paths need real implementations. |
| Multiplayer | Player arrays, input/session/packet code, six-player capacity | Replace DirectPlay/COM, implement room/session lifecycle, synchronization, disconnect/reconnect and latency handling. |
| Campaign co-op | Multiple-player entities provide a useful base | Define mission authority, triggers, rewards, death/arrest, progression and saves; fix local-player assumptions. |
| Modern display and 60 FPS | Existing draw/update separation is useful | Preserve validated simulation ticks; render/interpolate independently; profile representative scenes and browser/GPU combinations. |
| Packaging | Known original formats can support user file import | Asset validation, local storage/save durability and version compatibility remain untested. Assets stay user supplied. |

Sources: SG-SYSTEMS-01, SG-GAPS-01, SG-EXTERNAL-01, SG-COOP-01,
SG-TIMING-01, DEPS-03–06. This is a reuse assessment, not a claim these systems
were observed running.

Two findings prevent treating the remaining work as “finish 21 functions.”
First, patched mode sends WIP/STUB calls **back into the original executable**;
successful patched gameplay would not prove standalone completion. Second,
`SCRCMD_SET_ENTER_STATUS_50FB60` resolves a target player but modifies the local
player instead; the adjacent upstream comment associates this with multiplayer
desync. Save/failure/reward paths also assume a local player. Existing modes are
FRAG, POINTS and TAG. Campaign co-op needs behavior changes as well as transport.
[DEPS-04, SG-COOP-01]

Tick-dependent scripts include 30-times-seconds timer conversion; scheduling
uses 33/11 ms single-player and 50/40/33 ms network settings. Simply increasing
simulation updates to 60 per second would risk altered game behavior. Rendering
at 60 FPS with a validated simulation cadence is the proposed route, not yet
a benchmarked result. [SG-TIMING-01, inference]

Reuse permissions are unresolved: no root license file was found in the pinned
gta2_re or GTA2Hax tracked trees; the pinned hook loader is GPLv3 and pinned
Detours is the older non-commercial shared-source edition. A browser port can
exclude the injection/hook subsystem, but that does not establish permission
for the remaining source. This is a bounded dependency finding, not legal
clearance. The estimate below assumes permitted core reuse; an independent
rewrite requires re-estimation. [DEPS-08]

## Revised token and cost scenario

**EST-AUDIT-01: low-confidence planning judgment.** The audit improves knowledge
of the reuse base. It does not measure tokens per completed gameplay feature:
the math experiment is too small and unrepresentative to extrapolate. Exact
billed input, cached input and reasoning usage for this chat and its workers
was not available, so no measured audit-token total is claimed.

The previous estimate already assumed community reuse. This evidence supports
a **modest reduction in recovery work**, not a from-scratch-to-nearly-free
discount. Browser bring-up, co-op and release validation allowances are retained.

| Incremental work from the audited starting point | Previous | Revised |
|---|---:|---:|
| Representative browser gameplay slice, platform bring-up and native baseline | 5–15M | 5–15M |
| Remaining core fixes, platform services and full single-player mission/save QA | 60–150M | 35–110M |
| Private 2–4-player campaign co-op and network QA | 60–200M | 60–200M |
| Modern display, performance, browser compatibility and soak tests | 20–80M | 20–80M |
| **Full scope before reserve** | **145–445M** | **120–405M** |
| **Rounded allowance with roughly one-third reserve** | **200–600M** | **160–550M** |

Single player alone is **60–205M before reserve**, including the slice and full
display/performance allowance. Rows are incremental; cumulative totals must
not be added again. Scope remains desktop Chrome/Edge/Firefox/Safari,
1080p/1440p and 60 FPS on an agreed hardware baseline, all original single-player
content, and private 2–4-person cooperative campaign. Mobile, 4K/120 FPS,
public matchmaking and new art are excluded. Baseline hardware and co-op
policies are still to be selected; these are acceptance assumptions.

Tokens mean cumulative input plus output/reasoning across **every model call
and worker**, including replayed context and tool results. Cached inputs count
as tokens but have different prices. For continuity, one illustrative packet
is 30 calls × (20,000 input + 3,500 output/reasoning) = 705,000 tokens. The revised
base represents approximately 170–575 such debugging/verification packets.
Packet counts and call sizes are assumptions, not observed productivity.

Using the [official API rate card](https://developers.openai.com/api/docs/pricing)
retrieved on 12 October 2026, Standard short-context input/output rates per
million are $2/$10 for GPT-6.1 Sol and $10/$50 for GPT-6 Astra. Assume 85% input,
15% output including reasoning, input contexts at most 272K, no cache effects,
regional uplift or separately billed tools. Effective rates are $3.20/M for
Sol, $16/M for Astra, and $5.76/M for an 80/20 token mix. No Luna is budgeted.
[PRICE-01]

| Scope | Tokens | All Sol | 80% Sol / 20% Astra | All Astra |
|---|---:|---:|---:|---:|
| Browser gameplay slice | 5–15M | $16–$48 | $29–$86 | $80–$240 |
| Full single player, before reserve | 60–205M | $192–$656 | $346–$1,181 | $960–$3,280 |
| Single player + co-op, before reserve | 120–405M | $384–$1,296 | $691–$2,333 | $1,920–$6,480 |
| **Full scope with reserve** | **160–550M** | **$512–$1,760** | **$922–$3,168** | **$2,560–$8,800** |

USD model API costs only. The mix is illustrative, not a promise that Sol
can handle a fixed fraction of the project. Fast mode doubles these API rates;
long contexts, a greater reasoning/output fraction or unsuccessful iteration
can increase spend. Cache reads may reduce it; cache writes are separate.
For example, a 30% output fraction raises the blend to $7.92/M, or about
$1,267–$4,356 for the same token range. Higher reasoning can also increase
the token range itself. [PRICE-01, arithmetic in EST-AUDIT-01]

These API figures are **not Codex subscription allowances or purchased-credit
prices**, and are not a total project quote. The previous 9–18 person-month
staffing scenario remains low confidence; this audit supplies no grounds to
calibrate calendar time. Human campaign testing, specialist engineering,
hardware/tooling, permission resolution, hosting/relay/server costs and tax are
excluded. Runtime gameplay need not call an AI model. If permitted reuse or
standalone compatibility fails, the stated upper bound is not a guarantee.

## Remaining validation and next useful investment

| Audit question | Status |
|---|---|
| Does this revision build in its supported environment? | Answered by exact-SHA upstream CI; local Windows/Wine reproduction not performed. |
| Is meaningful implementation available to reuse? | Answered statically; a small math subset also passed runtime checks. |
| Does the full source build/run in a browser today? | Local full compile fails; isolated Wasm probe runs. Full game unknown. |
| Is every single-player mission playable standalone? | Unresolved; no game runtime or assets loaded. |
| Can existing multiplayer supply cooperative campaign behavior? | Transport/entity pieces are reusable; concrete additional mission semantics identified. |
| Is 1080p/1440p 60 FPS attainable? | Unresolved; no renderer/gameplay benchmark. |
| Is there an empirically calibrated full-project token forecast? | No; revised conditional scenario only. |

The next useful milestone is one **real browser gameplay slice**: import local
assets, render a map, walk/drive/shoot, execute a short mission and save/reload.
Establish a supported native reference in parallel; do not use patched fallback
as proof of standalone completeness. Record tokens by model and work packet,
bugs/time spent, and frame-time distributions on selected hardware. That
milestone should re-estimate the remaining project before campaign co-op expands.
The 5–15M allowance above is for this gameplay milestone, not the tiny completed
math experiment. This audit stops at the authorized build/gap assessment.

## Evidence index

All SG/DEPS/BUILD/EST identifiers below are local audit IDs, not REA-generated
IDs. The earlier binary-metadata REA pilot remains in the
[previous report](browser-port-feasibility-2026-10-12.md).

- **SG-COUNT-01, SG-UNMARKED-01, SG-GAPS-01, SG-SYSTEMS-01, SG-EXTERNAL-01,
  SG-COOP-01, SG-TIMING-01:** [source findings](evidence/gta2-build-audit-20261012/source-gaps/findings.md),
  [counts and file hashes](evidence/gta2-build-audit-20261012/source-gaps/counts.json),
  [counter](evidence/gta2-build-audit-20261012/source-gaps/count_source.py).
- **DEPS-01–09:** [dependency, license and CI findings](evidence/gta2-build-audit-20261012/dependencies/findings.md),
  including original log line references. Primary [exact-SHA CI run](https://github.com/CriminalRETeam/gta2_re/actions/runs/38216592513)
  and [pinned source](https://github.com/CriminalRETeam/gta2_re/tree/02de8beeb766644417f195d360eba33af0ee491f).
- **BUILD-ENV-01:** [host, tools, pins, clean status and artifact hashes](evidence/gta2-build-audit-20261012/environment.json).
- **BUILD-NATIVE-01:** [native configuration log](evidence/gta2-build-audit-20261012/build/native-configure.json).
- **BUILD-WASM-01:** [initial configuration](evidence/gta2-build-audit-20261012/build/wasm-configure.json),
  [initial compile failure](evidence/gta2-build-audit-20261012/build/wasm-game-build.json),
  [compile after MS-extension override](evidence/gta2-build-audit-20261012/build/wasm-game-build-ms-extensions.json).
- **BUILD-MATH-01:** [reproduced native/Wasm results](evidence/gta2-build-audit-20261012/build/reproduction/summary.json),
  [compatibility patch](evidence/gta2-build-audit-20261012/build/reproduction/math-compatibility.patch),
  [test harness](../scripts/gta2_math_probe.cpp), [runner](../scripts/run_gta2_math_probe.py).
- **BUILD-BROWSER-01:** [rendered browser observation](evidence/gta2-build-audit-20261012/build/browser-observation.json).
- **BUILD-UB-01:** [negative-division sanitizer result](evidence/gta2-build-audit-20261012/build/reproduction/ubsan-negative-divide.json).
- **EST-AUDIT-01:** [machine-readable assumptions and dollar arithmetic](evidence/gta2-build-audit-20261012/cost-scenarios.json).
- **PRICE-01:** [official API pricing](https://developers.openai.com/api/docs/pricing), fetched 2026-10-12.
