# GTA2 browser game team execution specification

Version 1.1, 12 October 2026, Australia/Sydney. Owner: the user of this workspace.
Audience: one orchestrator directing bounded implementation and validation tasks.

Build a browser edition of GTA2 with complete single player, private campaign
co-op for two to four friends, modern widescreen presentation and sustained
60 FPS on declared baseline hardware. Reuse the audited reconstruction where
permitted. Preserve original assets as user-supplied local files. Ship no original
game files. This specification defines execution; creating it has not started
implementation or authorized purchases, external deployment or publication.

Read the binding [implementation decisions](decisions.md) and
[machine-readable configuration](decision-config.json) before dispatching work.
They close architecture/product choices; workers implement them without comparing
alternatives. This specification defines requirements and gates. If facts contradict
a decision, the orchestrator resolves the specific contradiction.

Read this with [tasks.json](tasks.json), [the launch prompt](launch-prompt.md),
[the worker assignment template](worker-prompt.md),
[the build audit](../gta2-re-build-audit-2026-10-12.md), the repository
[AGENTS.md](../../AGENTS.md), [README](../../README.md) and
[REA setup](../rea-addon.md). The newer pinned audit supersedes historical
progress figures in older research; it does not supersede workspace guardrails.

## Product requirements

| ID | Required behavior and release evidence |
|---|---|
| R01 | User selects a local extracted game directory or supported archive. Validate required files and versions, explain missing/invalid files, show progress, then boot. No game assets leave the device or enter the release bundle. No arbitrary EXE execution in the browser. |
| R02 | Full original single player across all three districts, every mission and bonus activity, progression, gang respect, vehicles, weapons, police, trains, pickups, death/arrest, menus, original English text/glyphs, sound/music and original video content where used. Every discovered content item has a test-matrix row. |
| R03 | Save, quit, reload and district transitions preserve state. Browser storage failures are visible; export/import supports recovery. Use browser save schema v1; original SVG import/export is outside v1. Native browser saves must always work. |
| R04 | Keyboard/mouse controls, remapping, pause, focus-loss handling, audio volume/mute, fullscreen/windowed use, resolution and HUD scale work without stuck input. Audio starts after a user gesture. Controller support is a later enhancement, not a release gate. |
| R05 | Private invite rooms support two to four players completing the entire campaign together, including mission failure/retry, saving, guest reconnect and a clear host-departure outcome. No account system or public matchmaking is required. |
| R06 | 1920×1080 and 2560×1440 render targets, aspect-correct world view, readable scalable HUD, no stretching or extra simulation speed. Camera/culling changes must not change mission triggers or offscreen simulation behavior. |
| R07 | Chrome, Edge, Firefox and Safari desktop pass the browser matrix and performance criteria below. Record exact tested versions; refresh stable-version selection at release. A build that only works in Chromium is incomplete. |
| R08 | Ship a reproducible asset-free client, source/notice manifest as permitted, local-run instructions, optional signaling/relay service, recovery guide and measured limits. No runtime AI calls are necessary. |

Excluded: mobile, consoles, new art, 4K/120 FPS, public matchmaking, anti-cheat
for public competitive play, six-player co-op, host migration and seamless
mid-mission late joining. Original competitive modes are a later enhancement;
their code may inform transport and player handling. Do not silently exchange
campaign co-op for deathmatch or omit missions to meet a date.

## Starting evidence and limits

The source baseline is `CriminalRETeam/gta2_re` at
`02de8beeb766644417f195d360eba33af0ee491f`, retained untouched at
`research/gta2_re`. Selected submodule pins and host tools are recorded in
[environment.json](../evidence/gta2-build-audit-20261012/environment.json).
Emscripten 6.0.12 is installed at `research/toolchains/emsdk`. REA is local 6.4.0.
Do not upgrade a baseline during an active packet without recording the reason
and re-running affected checks.

| Evidence ID from audit | Established starting point | Execution implication |
|---|---|---|
| DEPS-07 | Exact-SHA upstream Windows/Wine CI passes 3,400 function checks; optional variable check has 13 issues | Reuse its build knowledge, but obtain an actual gameplay reference. Matching is not semantic proof. |
| BUILD-MATH-01 / BUILD-BROWSER-01 | Adapted math subset passes 44,577 checks in native, Node Wasm and the browser | Keep the probe as a bounded regression; it is not the game or a performance benchmark. |
| BUILD-WASM-01 / DEPS-03–06 | Full Wasm target fails Windows bindings; renderer is D3D9; Miles/Bink sources are shims; networking is DirectPlay | Build separate browser/platform targets with real services. Do not hide missing functionality behind no-op implementations. |
| BUILD-UB-01 / SG-EXTERNAL-01 | Signed-negative left shift detected; pointer-shaped data and CRT RNG assumptions exist | Audit arithmetic, ABI/data layout and RNG before trusting replay or synchronization. |
| SG-COOP-01 / SG-TIMING-01 | Mission paths assume local player; scripts use simulation ticks | Add explicit co-op semantics and separate simulation from presentation. |
| DEPS-04 / DEPS-08 | Patched mode falls back to original code; reuse permissions unresolved | Standalone tests cannot use original-function fallback. Track permission status per component before distribution. |

`downloads/GTA2INSTALLER.exe` is pristine. Verify MD5
`764848b25aed87b69da472f18c19580d` before analysis/extraction. Extract into `game/`.
The workspace installer is the 9.6F research target; the reconstruction targets
10.5. Establish an explicit compatible data/version profile. Do not solve a
mismatch by silently downloading a different original EXE.

## Orchestrator operating rules

Use **GPT-6.1 Sol** for implementation and prescribed verification. Use medium
reasoning for bounded implementation and high for difficult implementation, debugging
and review. Workers read source to implement the assigned contract; they do not
receive broad research or architecture-selection tasks. Use **GPT-6 Astra/high**
only for disputed evidence, genuinely unresolved reverse engineering and review
of high-risk changes. Architecture and gameplay decisions are already fixed in
[decisions.md](decisions.md). This routing is a project policy, not a measured capability guarantee.
**Do not assign Luna any research, reverse engineering, architecture or semantic
review. This plan assigns no work to Luna.** If a named model is unavailable,
record the substitution and use an available model of adequate capability;
never silently downgrade research to Luna.

Use a strong orchestrator to define small tasks and arbitrate findings. Keep
most implementation on Sol. Start with at most **three workers plus the
orchestrator**, reduced to the host's actual slot limit. Increasing concurrency
requires independent file ownership, a passed integration build and evidence
that review capacity is keeping up. Never spawn a task per function or hand an
entire subsystem to a worker with only this specification as context.

Each work assignment must include an exact baseline revision, one bounded
objective, non-goals, owned paths, interface version, dependencies, acceptance
checks and a token allowance. Workers cannot change neighboring interfaces or
acceptance thresholds without a concrete change request to the orchestrator.
Stop a packet after two unsuccessful implementation/verification cycles with
the same root blocker; return the minimal reproduction and hypotheses. The
orchestrator then splits, supplies missing evidence or escalates it. It does not
launch identical retries. A worker's failure is not grounds to mark the product
impossible or to ask the user to solve an ordinary debugging problem.

Before dispatch, check dependency and milestone gates, current rights/runtime
constraints and path collisions. A dependency is satisfied only when accepted,
not when a worker says it has finished. Every candidate needs review and
integration verification. The writer cannot be the only reviewer. Sol/high may
review routine packets; Astra/high independently reviews arithmetic/ABI/RNG,
mission authority, serialization/protocol and changes to the reference oracle.
The orchestrator may fill that reviewer role if it did not write the change.

Keep plans and evidence in the repository so an interrupted session can resume.
No goal, automation, external chat or recurring run is created by this spec.
Subagents stay within the orchestrator's task; do not create user-visible chats
as a substitute for worker delegation.

## Workspace and integration contract

At execution kickoff, inspect existing Git state. If there is no root repository,
initialize a local one only after verifying ignore rules and staged contents.
Keep `downloads/`, `game/`, `node_modules/`, `.npm-cache/`, `research/`, `build/`
and local capture/data directories excluded. Do not commit copied game data,
original executables, proprietary SDKs, generated asset conversions or dumps.
Do not publish the repository during implementation.

Use this layout, created by the bootstrap packet:

```text
port/core/                portability adapters and permitted core patch set
port/platform/           clock, input, storage and browser lifecycle
port/render/             WebGL2 backend and presentation
port/audio/              browser audio and video adapters
port/net/                transport, protocol and client/host integration
port/coop/               cooperative mission policies
web/                     asset importer and browser shell
services/signaling/      minimal room/signaling service
tests/unit/              generated or synthetic fixtures only
tests/replay/            replay runner and test definitions
tests/browser/           browser and two/four-client scenarios
tests/perf/              repeatable scene definitions and metric collectors
docs/contracts/          reviewed interface and schema definitions
docs/decisions/          architecture decisions with alternatives and evidence
docs/orchestration/runs/ assignment, usage, acceptance and integration ledgers
research/port-work/      local upstream copies/build inputs while rights unresolved
research/runtime-data/   ignored game-derived captures and local fixtures
```

Do not mutate `research/gta2_re`. Develop against a pinned local working copy
and a reproducible patch/adaptation procedure. A future source vendor/import
decision must record provenance and permission status. Calling decompiled code
“clean-room” or adding an MIT header does not establish rights. Exclude the
Windows injection/hook target from the browser dependency closure.

Assign one branch/worktree per active implementation worker if Git is available.
Do not concurrently edit a shared checkout. Outside Git, dispatch only disjoint
path ownership and integrate serially. The orchestrator alone edits shared
build manifests, dependency locks and reviewed contracts, or explicitly leases
them to one worker. Build/cache directories must be worker-specific.

Workers submit a commit or patch, changed-path list and evidence bundle. The
orchestrator checks ownership, reviews the diff, runs affected checks, merges
serially and runs the milestone smoke test on the combined result. Preserve the
last passing revision. A conflict or regression returns the packet to rework;
never resolve it by deleting another worker's behavior without review.

## Fixed architecture and gameplay contracts

The complete choices are in [decisions.md](decisions.md), D01–D16. Workers receive
only the decision sections relevant to their packet, together with its acceptance
checks. The machine-readable constants are in [decision-config.json](decision-config.json).
Do not treat them as options or run another architecture survey.

| Boundary | Binding choice |
|---|---|
| Version and oracle | 9.6F local asset profile and original gameplay reference; pinned 10.5 source adapted to that behavior. No 10.5 EXE acquisition or required native reconstruction build. |
| Stack | C++17/wasm32, Emscripten 6.0.12; plain JS modules, HTML/CSS; WebGL2. Single main-thread instance, no Asyncify/pthreads/framework rewrite. |
| Timing and arithmetic | 33 ms simulation ticks, interpolated display, four-tick catch-up cap; explicit 14-bit fixed-point rules and selected MSVC-style RNG with reference validation. |
| Data and storage | User-supplied directory or specified .gta2pack; current-district MEMFS; IndexedDB Blob cache and transactional logical saves. |
| Presentation | Pure WebGL2 renderer, RGBA atlases, original vertical world span, wider horizontal presentation and separate canonical gameplay cameras. |
| Media | Web Audio effects, streamed browser media, local FFmpeg preparation for Bink/unsupported formats. No browser codec reverse engineering. |
| Multiplayer | Host-only simulation, presentation-only guests, no physics prediction/rollback; WebRTC host-star, Node/ws signaling and coturn relay. |
| Replication | Three named channels, tick snapshots, acknowledged delta baselines, stable entity IDs, bounded chunks/resync and explicit backpressure. |
| Campaign | One mission actor, one shared score/respect/progression ledger, individual health/inventory/wanted state; explicit targeting/respawn/failure rules. |
| Session | Join between missions; 60-second slot reconnect, host-owned saves, no host migration. |

T04 transcribes the fixed boundaries into minimal headers/schemas and conformance
tests. It does not choose an engine, language, transport or authority model.
Contracts still enumerate concrete source fields, widths, units, ownership,
errors and tests; unsupported source assumptions go to a focused specialist
question. No raw pointer or C++ memory serialization is permitted.

## Reference and test requirements

Reference order: observed original runtime with recorded executable/data hashes;
documented format semantics; reconstructed behavior; explicit product deviation.
Always distinguish observation, inference and unknowns. Source annotations,
upstream CI and hybrid patched gameplay cannot substitute for standalone tests.
Preserve original quirks unless a reviewed modern-display/co-op policy changes
them. Never change expected test outputs solely to make the port pass.

Create a catalog of every district, mission, bonus and important state branch
from local assets. Each row records profile/hash, prerequisites, steps, expected
result, SP/co-op mode, status, build, evidence and defects. Missing coverage is
`untested`, not `pass`. Generate small synthetic fixtures for public automated
tests; keep game-derived captures/replays/screenshots under ignored local paths.
Source-control only permitted test definitions, metadata and expected summaries.

For gameplay comparisons, record inputs, seed, tick numbers, observable state,
version and allowed deviations. Use the selected scripted-input/video/event capture route; original replay
format integration is outside v1. Compare discrete mission events exactly; approve any numeric
tolerance per field with rationale. Preserve a minimal failing reproduction for
every corrected semantic bug. Original/campaign reference execution remains a
blocking acceptance dependency if the required Windows/Wine environment is absent.
Continue independent synthetic, platform and rendering work while it is resolved.

Release gates require:

| Gate | Measurable pass condition |
|---|---|
| G0 Bootstrap | Baselines/ignores verified; task state/usage ledger active; source permission status and local runtime availability recorded; no asset files staged. Missing reference tools may remain tracked blockers, but cannot be called installed. |
| G1 Contracts and reference | Implemented fixed platform/state/render contracts; working 9.6F original-game smoke/capture route; precise compatibility profile. A Windows reconstruction build is not a prerequisite; standalone browser execution is tested at G2. |
| G2 Playable browser slice | Import local files; draw a real district with correct assets; walk, enter/drive/exit a car, shoot, complete one original mission, save/quit/reload; working audio; 15-minute play without crash or original-code fallback. Chrome plus one non-Chromium browser. |
| G3 Single player | Entire catalog passed in SP, including failure/retry/save paths; all three districts traversed; full campaign playthrough; zero known progression blockers/data-loss defects. Original media works. |
| G4 Cooperative slice | Two then four real clients pass a custom mission with the co-op rules; input ownership, snapshots/resync, save/reload and disconnect behavior pass under impaired networking. |
| G5 Cooperative campaign | Every campaign catalog row passed in co-op with two clients; representative branch/system coverage repeated with four, plus a complete four-player campaign playthrough; actor/non-actor disconnect and shared-progression cases covered. |
| G6 Release candidate | G3 and G5, full browser/performance matrix, soak/recovery tests, no bundled original files, resolved distribution permissions, reproducible clean build and reviewer acceptance. Publishing is a separate authorized action. |

Use these two binding baseline classes: an Apple M1 Mac with 8 GB RAM
and a Windows PC with Core i5-10400, GTX 1650 and 16 GB RAM, both at 60 Hz.
These are selected acceptance targets, not claims about available hardware. At
bootstrap record actual devices, OS, GPU/driver, power mode and versions.
Equivalent or slower replacements require a documented comparison. Missing
baseline access leaves performance unverified; it does not license weaker
thresholds. Safari runs on macOS; Chrome/Firefox on both; Edge on Windows.

At both requested render resolutions, use ten-minute runs after a 60-second
warm-up for traversal, dense traffic/police, explosions and four-player scenes.
Require mean presented FPS at least 59 on a 60 Hz display, p95 frame interval
at most 18 ms, p99 at most 33.4 ms and no steady-play stall over 100 ms. Report
loading/GC stalls separately and investigate outliers; do not hide loading
behind a perfect in-game average. Measure CPU simulation/render submission and
GPU time when available, actual render resolution, dropped frames and memory.
Instrumented timing must be validated against browser profiling for at least
one scene. No claim of GPU timing when the API is unavailable.

Use 30-minute single-player and two-hour four-player soak tests. After warm-up
and equivalent scene return, retained memory must not show unbounded growth;
set an explicit baseline and investigate more than 10% growth. Test storage-full,
refresh during save, corrupted save, context loss/recovery, audio resume and
background/focus transitions. Saves must be transactional or recoverable from
the previous valid version. Avoid using coarse memory readings as proof of no leak.

Network matrix: loopback; two physical machines; two internet locations with
TURN exercised; 50/100/150 ms RTT, up to 30 ms jitter and 1% packet loss; a
five-second interruption and reconnect. Record the impairment method and actual
measurements. Require no permanent state divergence, duplicate rewards, lost
ownership or deadlocked mission. Target p95 host-confirmed actions below 250 ms
at 100 ms RTT. A worst-case 250 ms/5% loss case may degrade but must fail/recover
clearly. Hash authoritative snapshots; compare clients only at the same applied
authoritative tick, not their interpolated presentation state.

## Task dispatch and evidence contract

`tasks.json` is a dependency plan, not executable code or a live scheduler.
Validate it with `python3 scripts/validate_gta2_plan.py` from the workspace root.
Copy it to a new run directory before updating status. Requirements/gates here,
locked decisions/configuration, and manifest task metadata are normative in their
respective scopes. Resolve contradictions before dispatch and version them together. `epic` entries must be expanded
into bounded child packets with acceptance criteria; they cannot be delegated
wholesale. A child's context should normally contain only the packet, contracts,
relevant source and evidence summary, not all conversation history.

Task states: `planned → ready → running → review → accepted`.
`running/review → rework` returns through `ready`; `blocked` records dependency,
owner and unblocking evidence; `cancelled` records an approved scope decision.
Only the orchestrator accepts tasks. A blocked predecessor does not block
unrelated work. A gate is accepted only when all its listed tasks and gate tests
pass. Parent epics are accepted only when children and integration tests pass.

Assignment fields: `task_id`, `parent_id`, `objective`, `baseline_revision`,
`read_paths`, `write_paths`, `dependencies`, `decision_ids`, `contract_versions`, `non_goals`,
`required_checks`, `model`, `reasoning`, `token_limit`, `reviewer`, `return_path`.
Worker result fields: `task_id`, `status`, `commit_or_patch`, `changed_paths`,
`observations`, `inferences`, `unknowns`, `checks` (command/runtime, exit status,
artifact path), `evidence_ids`, `usage`, `remaining_defects`, `next_action`.
Failed commands and negative findings remain in the bundle. Do not paste huge
logs into orchestrator context; preserve files and return the relevant excerpt.

An evidence record has a stable ID, UTC timestamp, host/browser/compiler,
source/build/data hashes, command or interaction steps, actual result, evidence
type and limitations. Avoid personal paths/secrets in shareable logs. Only
redacted, asset-free summaries may enter external CI approved for this workspace.
External communication, publishing and use of new third-party destinations need
their applicable authorization; no worker may infer it from an upstream README.

## Budgets and escalation

The audit's **160–550M total-token scenario** is a planning range, not a spend
authorization or hard upper guarantee. Retain phase envelopes: 5–15M slice,
35–110M additional SP, 60–200M co-op, 20–80M performance/compatibility, plus
roughly one-third reserve. Gate G2 requires recalibration using measured work.
Do not count already-completed audit work again as future implementation.

For a bounded packet, set a default 250,000 cumulative-token ceiling, raised
by the orchestrator to at most 750,000 only with scope justification. Decompose
larger tasks. At 80% of the packet allowance, finish a reviewable checkpoint
and report the forecast; at the ceiling stop that packet and return evidence.
An explicit user/project spend limit always overrides these defaults. If usage
is unavailable, record `unknown`, cap calls at 20 per packet as an imperfect
fallback, and do not claim a measured spend. Calls are not equivalent tokens.

Record input, cached read, cache write, output/reasoning, model, tier, provider
and rates separately where exposed. Never double-count reasoning. Include
orchestration, reviews, retries, abandoned approaches and worker context in cost.
Do not replace actual usage with source-file token counts. API cost projections
must use current official rates and the configured billing route; Codex included
allowances/credits are separate. The audit's illustrative 80% Sol/20% Astra mix
is a starting budget distribution, not a quota to meet by using an unsuitable
model. Use cheaper models by narrowing packets and avoiding repeated context.

Reforecast at each gate and after two packet overruns in the same subsystem.
Escalate with: minimal reproduction; what was attempted; observed failure;
competing explanations; evidence needed; smallest next experiment; revised
scope/token forecast. A stronger model reviews the question before another
broad implementation attempt. Ask the user only for unavailable runtime/tool
installation decisions, materially changed product scope, spending beyond their
limit or required rights/deployment authorization. Routine fixes and reversible
local work should proceed within an already-authorized execution task.

Ghidra installation specifically remains the user's decision under AGENTS.md.
Do not run `rea setup` or global installs. Use the REA skill for genuine binary
questions, summary-first, and close native sessions. Audit upstream build scripts
before execution: current `build.py` may download originals and copy into an
installation even when a run flag is absent. Use an explicitly selected isolated
runtime for any Windows registry/game setup.

## Completion and handoff

After every gate, deliver a playable build if one exists, a compact pass/fail/
unknown matrix, accepted revisions, measured usage and forecast, unresolved
defects and the next independent packets. No recurring notification is implied.
Stop work only at a user limit or real dependency impasse after independent work
is exhausted; never call a mockup, map viewer or working math probe the full game.

Release completion requires all product requirements and G6 evidence. Keep a
separate distribution-ready status and publication status: publishing is not
implied by completing a build. The final handoff includes launch instructions,
asset import guide, exact compatibility profile, save/room recovery behavior,
known limitations, source/licenses, reproducible build commands and test reports.
