# GTA2 browser port: REA pilot and cost estimate

> **Superseded estimate:** the subsequent [pinned build-and-gap audit](gta2-re-build-audit-2026-10-12.md)
> verifies upstream CI, tests an adapted math subset in WebAssembly and the
> browser, and revises the full-scope planning allowance to **160–550M tokens**
> (approximately **US$922–$3,168** at the same 80% Sol / 20% Astra Standard API
> assumptions). The pilot results and earlier scenario below are retained as
> historical evidence; they are not the current build status or estimate.

Assessment date: **12 October 2026**. Scope: a feasibility test and planning
estimate, not an implementation of the game. Dollar figures are **USD**, exclude
tax, and use rates retrieved on this date.

The promising route is to validate and, where permitted, reuse the community's
GTA2 reconstruction, then build the browser platform layer and co-op. REA helps
answer binary questions; it does not automatically supply a portable renderer,
networked campaign, or proof that every mission works. This is an engineering
inference from the project and platform evidence below [S1, S2, S4, S5].

**Planning allowance:** approximately **200–600 million cumulative model tokens**
including reserve for the complete browser game with co-op. Under the explicit
mixed-model pricing assumptions below, that is roughly **$1,150–$3,460 in model
API usage**. Using GPT-6 Astra for everything gives **$3,200–$9,600** at Standard
speed. These are conditional scenarios, not a quote or a measured completion
forecast. Human engineering, testing, hosting and unresolved reuse permissions
are separate [EST1, S7, S8].

## Assumed meaning of “finished”

These are proposed acceptance criteria, not observed capabilities:

- Full original single player: all three districts, missions and bonus content,
  progression, menus, saves, controls, audio/music and original behavior where
  observable. The original game is the regression reference.
- Desktop Chrome, Edge, Firefox and Safari; 1920×1080 and 2560×1440 display
  modes, appropriate widescreen camera and scalable HUD, retaining the art style.
  Mobile, 4K/120 FPS, new art, public matchmaking and console support are excluded
  from this estimate.
- A 60 FPS target on agreed baseline hardware, assessed in busy scenes and
  extended play. Baseline hardware is still to be selected. Resolution and a
  frame-rate counter alone do not establish acceptable frame pacing.
- Two to four friends in private rooms, with shared campaign missions,
  synchronized vehicles/pedestrians/combat, explicit progression and respawn
  rules, reconnection and reasonable network-latency behavior.
- Each player imports their own game files locally. Work products do not bundle
  or publish GTA2 assets, following this workspace's guardrails [L1].

Original multiplayer documentation describes Deathmatch and Tag. Shared story
missions therefore require additional design and implementation; they are not
established by the original multiplayer feature [S3, inference].

## What the local test actually measured

Used the project-scoped
[`reverse-engineer-anything` skill](../.claude/skills/reverse-engineer-anything/SKILL.md)
and installed REA 6.4.0. The reproducible runner is
[`scripts/rea_probe.py`](../scripts/rea_probe.py). It verifies the installer,
runs the native CLI route and portable PE-resource route, records complete JSON,
and checks the installer again. It executes no GTA2 code and installs nothing.

Raw outputs and timing records are in
[`docs/evidence/rea-pilot-20261012T084447Z/`](evidence/rea-pilot-20261012T084447Z/).

| Test | Observed result | Wall time | Evidence |
|---|---|---:|---|
| Installer identity | 362,331,961 bytes; expected MD5 matched before and after | Not separately benchmarked | L2 |
| Native overview, via `rea inspect` | `provider_unavailable`; no native function analysis produced | 1.2473 s | L3 |
| Portable `inspect-pe-resources` | PE32, machine 332 (`0x14c`), 44 resource records, 64 directory entries examined; complete within that parser's scope | 1.3071 s | REA1 |

The MD5 is `764848b25aed87b69da472f18c19580d`. SHA-256 is
`0cd380498dfbe62448d56aaddd3e36593cfda324aa8a3972b06a02d015b083a0` [L2, REA1].

REA reported Ghidra unconfigured (`GHIDRA_INSTALL_DIR` unset), Hopper's launcher
missing and IDA unconfigured. Its diagnostic found an acceptable full JDK
21.0.12 already present [L3]. This establishes the available configuration; it
does not prove no other Ghidra copy exists elsewhere on the machine.

`rea inspect` uses the CLI's native session-open route and closes the session
through `withSessionCleanup`, including on failure. This was checked in the
installed `DirectAnalysis.js` and `DirectAnalysisCleanup.js` [L4]. No persistent
native session was opened or left behind by this pilot.

The resource result is **installer metadata**, not a game-file inventory or
gameplay reconstruction. REA leaves most resource payloads opaque. This pilot
did not extract `gta2.exe`, establish its packing/version, recover a function,
execute the game, run a browser build, benchmark FPS, or test multiplayer.
Consequently **1.3 seconds cannot be extrapolated into a game-completion time**
[REA1, L3; unknowns U1–U4].

The workspace reserves installing a native engine for a user decision. The
estimate can be completed without that installation [L1]. A deeper native pilot
would first need a selected Ghidra installation and an extracted, identified
executable. Existing notes describe a 9.6F target, whereas the community source
targets 10.5; compatibility must be checked, not assumed [L1, S2, U1].

### Observable text size, not the bill for this chat

The saved
[`token-estimates.json`](evidence/rea-pilot-20261012T084447Z/token-estimates.json)
uses the locally installed `tokenx` estimator, also used by REA's CLI dependency
for `--token-count`. It is a heuristic, not the model tokenizer or API usage
counter [L5].

| Text | Characters | Estimated tokens |
|---|---:|---:|
| REA skill instructions alone | 13,580 | 3,494 |
| Native failure JSON | 2,411 | 731 |
| Complete PE-resource evidence JSON | 24,405 | 8,179 |
| Selected summary of the same retained evidence | 965 | 276 |

The two REA outputs total about **8,910 estimated text tokens**. Reading the
selected resource summary instead of its full JSON reduces this particular
response's estimated text size by **96.6%**. The summary retains the evidence ID,
artifact digest, counts, coverage and limitations; detailed follow-ups still
need the full record. This is a measurement of one output, not a claim of 96.6%
savings for the whole project [L5].

The CLI operations were local analysis, not paid LLM inference. Consuming their
outputs in an agent contributes input tokens; the skill, references, source
reads, conversation history, repeated context, agent/subagent output and
reasoning also contribute. **Exact charged tokens for this conversation are
unavailable in this test.** Neither the JSON sizes nor the figures above are the
total usage of this chat.

## Updated external evidence

**Observation of a maintainer report, not independently verified gameplay:**
on 9 October 2026, Valps reported fewer than 50 functions remaining to match,
compared with about 730 before the team's recent AI-assisted work [S1]. This
supersedes the older two-thirds progress figure for planning purposes. It does
not prove all missions, peripheral libraries or bugs are resolved.

**Source observation:** the current published build documentation targets GTA2
10.5 and Windows executables/Wine. The inspected CMake file links `mss32`,
`binkw32`, `Winmm` and `DPLAYX.lib` and declares a `WIN32` executable [S2]. A
browser build needs replacements/adapters for those platform dependencies; it
is not established by a high percentage of matched native functions
[S2, S4, S5; inference].

**Bounded license check:** no explicit reuse license was established from the
repository root listing and README inspected in this review. Do not assume this
source is MIT because some related projects are. The reuse-based estimates below
are conditional on establishing appropriate permissions; an independent rewrite
needs a fresh estimate [S2, U5].

The existing `gta2-webgl` README documents a browser/server prototype requiring
one map and one style file supplied by the user. That README does not establish
complete campaign or co-op support, so it is a reference for exploration rather
than a verified finished base [S6].

## Proposed technical route

This is a design recommendation, not an implementation result [S2–S6, EST1].

1. **Validate a native reference.** Extract locally into `game/`, fingerprint
   relevant executables/data, select the game version, establish reuse terms,
   build/run the community reconstruction, and reproduce representative mission
   and vehicle behavior. Use REA only for unresolved binary questions.
2. **Build one browser gameplay slice.** A C/C++ core compiled with Emscripten to
   WebAssembly, a WebGL2 renderer, browser input/audio, local file import and
   persistent saves. Adapt the main loop to the browser. Emscripten documents
   the platform changes around loop scheduling, files, input/audio and networking
   [S4, S5]. This is a candidate architecture, not a tested selection.
3. **Preserve gameplay while modernizing display.** Keep the reference simulation
   cadence and separate presentation/interpolation. Test larger camera views,
   culling and HUD scaling rather than stretching the original frame. Profile
   traffic, police, explosions and long sessions on the baseline hardware.
4. **Complete single-player parity.** Track each mission and feature with an
   observed pass/failure/unknown; check saving, district changes, audio and rare
   script branches. Matching function counts are not the acceptance metric.
5. **Add co-op in stages.** First a small custom mission with two clients, then
   four players and latency/reconnect tests, then the original campaign. A host
   or server should own mission execution and authoritative world state. Choose
   WebRTC data channels or a server transport after a measured networking spike;
   ordinary native sockets/DirectPlay are not a drop-in browser solution [S5].
6. **Validate a release candidate.** Full campaign playthroughs, automated
   regression replays where practical, private-session soak tests, browser/GPU
   coverage, save durability and frame-time measurements. No FPS guarantee is
   justified before this work.

This architecture uses AI during development. It does not require model calls
to run gameplay. Recurring operational costs would be hosting, signaling,
servers/relays and bandwidth; they need traffic and concurrency measurements.

## Token model and assumptions

**EST1 — low-confidence engineering judgment.** The local test calibrates
evidence overhead only. No representative implementation task was completed, so
there is no empirical tokens-per-feature rate. The following is a transparent
scenario for deciding whether to fund a better pilot, not a statistical
confidence interval or guaranteed upper bound.

“Total tokens” means cumulative input plus output across all model calls and
workers. Input includes replayed history, relevant source, skill instructions
and tool results. The output allowance includes generated text/code/tool
arguments **and reasoning tokens**, where billed. Reasoning is not added a
second time. Cached input remains part of the token count, with a different
price. File size and unique lines of code do not measure this total.

One illustrative work packet is a scoped change followed by debugging and
verification:

```
30 model calls × (20,000 input + 3,500 output/reasoning tokens)
≈ 705,000 cumulative tokens per packet
```

These call sizes are assumptions. A mission correction, renderer behavior, save
path or synchronization defect can be one packet; packets are not functions and
do not all take equal time. Roughly 200–630 such packets gives the base range
below. Longer contexts, more reasoning, unsuccessful iterations or substantially
less reusable code can push costs well beyond it.

| Incremental work | Assumed cumulative tokens |
|---|---:|
| Browser slice, including source/version audit and initial platform bring-up | 5M–15M |
| Remaining recovery, platform gaps and full single-player behavior/mission QA | 60M–150M |
| Private 2–4-player campaign co-op and network QA | 60M–200M |
| Display/performance work, compatibility, soak tests and release polish | 20M–80M |
| **Complete scope before reserve** | **145M–445M** |
| **Rounded planning allowance with reserve** | **200M–600M** |

Single player without co-op totals **85M–245M** using the same generous final
polish allowance. The slice is included in both cumulative totals. These rows
must not be added to the cumulative totals again.

## Dollar conversion

Use the dated official API rate card [S7]. The simplified table assumes
Standard service, input contexts no larger than 272K, **85% input / 15% output**
(including reasoning), no cached reads/writes, no regional uplift and no paid
tool charges. The 85/15 mix approximates the packet example, rather than being
measured usage. All dollars below are USD.

| Model | Input / 1M | Cached input / 1M | Cache writes / 1M | Output / 1M | Effective $ / 1M total at 85/15, no cache |
|---|---:|---:|---:|---:|---:|
| GPT-6.1 Sol | $2.00 | $0.10 | $2.50 | $10.00 | $3.20 |
| GPT-6 Astra | $10.00 | $1.00 | $12.50 | $50.00 | $16.00 |
| 80% Sol / 20% Astra, by tokens | — | — | — | — | $5.76 |

The mix is a budgeting example, not a claim the cheaper model can complete a
fixed proportion of this project. Quality failures can erase nominal savings.

| Cumulative scope | Tokens | All Sol | 80/20 blend | All Astra |
|---|---:|---:|---:|---:|
| One browser gameplay slice | 5M–15M | $16–$48 | $29–$86 | $80–$240 |
| Complete single player, before reserve | 85M–245M | $272–$784 | $490–$1,411 | $1,360–$3,920 |
| Single player + co-op, before reserve | 145M–445M | $464–$1,424 | $835–$2,563 | $2,320–$7,120 |
| **Complete scope with planning reserve** | **200M–600M** | **$640–$1,920** | **$1,152–$3,456** | **$3,200–$9,600** |

For a real bill, calculate each model/tier separately, using mutually exclusive
token categories (counts expressed in millions):

```
cost = ordinary_input × input_rate
     + cached_reads   × cached_input_rate
     + cache_writes   × cache_write_rate
     + output_including_reasoning × output_rate
     + separately_billed_tools
```

Cached reads may reduce cost; new cache writes have their own charge. Fast mode
doubles these two models' Standard API rates; longer input contexts also change
the rate. The no-cache table is neither the cheapest nor the most expensive
possible case [S7]. For example, the reserve scenario on all-Astra Fast would be
about $6,400–$19,200 under the same token mix. Heavy reasoning could further
increase both token count and the fraction priced as output.

**Codex subscriptions are different.** These API figures are not an estimate of
how many tasks a Plus/Pro subscription includes, nor the cost of this chat. The
official Codex page says limits depend on task complexity/context and gives
separate purchased-credit rates. At the same illustrative mix, Standard Sol is
80 credits per million total tokens and Astra is 400; the 80/20 blend is 144.
The 200M–600M reserve scenario is therefore **28,800–86,400 purchased credits**
before cache effects at that blend, if all usage were billed through those
credit rates. Included allowance, plan eligibility, credit purchase price and
account-specific limits were not inspected. Credit billing has no separate
cache-write charge [S8].

Model spend excludes engineering labor, manually verifying missions, test
hardware, native tooling/OS availability, infrastructure, bandwidth and any
permissions work. As a separate low-confidence staffing judgment, allow roughly
**9–18 person-months** for an experienced engineer/team across the whole scope.
This is not derived from token throughput. Parallel work can shorten elapsed
time, but integration and playtesting remain [EST1]. A loaded labor budget is
that effort multiplied by the actual monthly staffing cost.

## Recommended next decision

Fund a **bounded playable slice**, then re-estimate. The initial go/no-go should
require: an identified usable source/data baseline, one district in the browser,
walking/driving/shooting, one working mission, save/reload, browser audio, and
frame-time captures at 1080p on named hardware. Add a separate two-client shared
mission test before committing to campaign-wide co-op [EST1].

Start with a 5M–15M token planning envelope for that slice, approximately $30–$90
at the illustrative blend. This is a proposed budget, not an authorization to
spend or a promise that the slice can be completed within it. If the native
baseline cannot run or source reuse cannot be established, stop that assumption
and re-estimate before porting.

Record actual API `input_tokens`, cache details, `output_tokens` and reasoning
details per completed work packet. Reasoning details are a breakdown of output,
not an extra amount to add again. Record retries, accepted behavior and human
review time too. After 3–5 representative packets, replace the assumed packet
size with observed values and revise the remaining work count. Keep complete
REA evidence on disk and use focused views to control context overhead [L5,
EST1].

## Evidence ledger and unresolved questions

Local labels beginning **L** are ordinary filesystem/CLI observations, not REA
Evidence IDs. **S** labels are public primary sources. **EST1** denotes the
planning assumptions above.

- **L1:** [README](../README.md), [AGENTS](../AGENTS.md),
  [REA setup notes](rea-addon.md), and
  [prior research](reverse-engineering-report.md). Existing packing/version
  statements were not independently verified against an extracted executable.
- **L2:** [Pilot measurements](evidence/rea-pilot-20261012T084447Z/measurements.json).
- **L3:** [Native CLI diagnostic](evidence/rea-pilot-20261012T084447Z/native-overview.json).
  Failure output has no REA evidence ID; none has been invented.
- **L4:** Installed package
  `node_modules/rea-agents/dist/application/DirectAnalysis.js` and
  `DirectAnalysisCleanup.js`; version pinned in `package.json`/lockfile.
- **L5:** [Text-size estimates](evidence/rea-pilot-20261012T084447Z/token-estimates.json)
  and [selected evidence summary](evidence/rea-pilot-20261012T084447Z/pe-resource-summary.json).
- **REA1:** `ev_6c4b21628b7528da74dd4dc1f5ff79239f0bbe01774c7e80e5df487bd0fde8c0`;
  [complete PE-resource Evidence](evidence/rea-pilot-20261012T084447Z/pe-resources.json),
  provider `rea-pe-resources`, operation `inspect_pe_resources`.
- **S1:** [Maintainer progress, 9 October 2026](https://gtamp.com/forum/viewtopic.php?t=1424#p14149).
  Maintainer claim; no local campaign validation. The broader thread also
  distinguishes reconstruction from proposed future multiplayer improvements.
- **S2:** [gta2_re repository and build instructions](https://github.com/CriminalRETeam/gta2_re),
  [CMake configuration](https://github.com/CriminalRETeam/gta2_re/blob/master/CMakeLists.txt).
  Published default-branch views inspected; no immutable revision or exhaustive
  license/dependency audit was established by this review.
- **S3:** [Original GTA2 manual, multiplayer section](https://gtamp.com/GTA2/gta2manual.pdf), PDF page 6.
- **S4:** [Emscripten runtime environment](https://emscripten.org/docs/porting/emscripten-runtime-environment.html).
- **S5:** [Emscripten networking](https://emscripten.org/docs/porting/networking.html).
- **S6:** [Existing GTA2 WebGL prototype README](https://github.com/Kajakklubben/gta2-webgl).
- **S7:** [Official OpenAI API pricing](https://developers.openai.com/api/docs/pricing), retrieved 12 October 2026.
- **S8:** [Official Codex pricing and token rates](https://learn.chatgpt.com/docs/pricing#token-rates), retrieved 12 October 2026.

| Question | Status | Evidence / remaining gap |
|---|---|---|
| Does the installed skill/tool work on this artifact? | Partly answered | Resource parser succeeds; native provider unavailable [REA1, L3] |
| Is the installer original and unchanged? | Answered within recorded identity checks | Matching MD5 before/after and recorded SHA-256 [L2] |
| U1: Exact extracted version, packing and compatibility with 10.5 source? | Unresolved | `game/` was empty; executable not extracted in this pilot |
| U2: Native/campaign correctness and remaining recovery? | Unresolved | Current maintainer claim [S1]; no local build or playthrough |
| U3: 1080p/1440p browser frame rate and memory behavior? | Unresolved | No browser game implementation or baseline hardware benchmark |
| U4: Shared campaign rules, state replication and latency behavior? | Unresolved | Original competitive modes documented [S3]; co-op design/test needed |
| U5: Permissions for source reuse and a publishable asset boundary? | Partly answered | Local asset rule clear [L1]; upstream reuse terms not established [S2] |
| Is the requested token/cost estimate available? | Answered as scenarios only | Explicit packet, token-mix and dated rate assumptions [EST1, S7, S8] |

The feasibility assessment is complete at this bounded level. Reverse engineering,
full playability and performance are not claimed complete.
