# GTA2 orchestrator launch prompt

Paste the following into an orchestrator when you want implementation to begin.
The packet defines local execution; it does not pre-authorize purchases,
publication, external messages or installing Ghidra.

```text
Execute the GTA2 browser project using docs/orchestration/gta2-browser-spec.md
and docs/orchestration/tasks.json in this workspace. Read the binding
docs/orchestration/decisions.md and decision-config.json first. Read AGENTS.md, README.md,
docs/rea-addon.md and docs/gta2-re-build-audit-2026-10-12.md first. Treat the
pinned audit as the current technical baseline. Preserve original assets and
downloads; do not publish or upload them.

Act as orchestrator. Use GPT-6.1 Sol workers for bounded implementation and
Sol/high for difficult implementation, prescribed verification and review.
The hard architecture/gameplay decisions have already been made in decisions.md.
Do not delegate broad research, framework comparisons or redesign. Use GPT-6
Astra/high only for high-risk review and a concrete unresolved contradiction. Do not use Luna for research or reverse engineering; this
specification routes no tasks to Luna. Check actual model/tool availability
and record substitutions. Start with at most three workers, or fewer if slots
are unavailable. Follow the repository's REA skill for binary investigations.

Create a run ledger from the manifest, inspect Git/tool/runtime state and
execute T00. Then dispatch ready, independent packets only. Implement the fixed contracts
before parallel implementation, give workers explicit owned paths and tests,
and review and integrate serially. Expand epics into small child packets rather
than assigning entire subsystems. Do not repeat the broad build audit unless
the baseline changed or a task needs a specific unanswered fact.

The first delivery is G2: real browser asset import, a district, walking,
driving, shooting, working audio, an original mission and save/reload. Preserve
the full single-player/co-op objective, but use G2 as a checkpoint and reforecast
the remaining scope with measured usage before expanding later phases. Report
the checkpoint and continue authorized ready work unless I set a stopping or
spending limit; do not request approval for routine local fixes.

Use packet caps and escalation rules in the specification. The project-wide
160–550M estimate is not permission to buy credits or exceed configured limits.
Track actual usage where available; mark unknowns honestly. Do not claim full
gameplay, parity, co-op, performance or permission clearance without evidence.
If a runtime, rights or hardware dependency blocks a gate, state its exact
impact and continue useful independent work. Ask only for the missing decision
or access that cannot be obtained within the authorized scope.
```

Optional launch constraints can be appended: total token/spend ceiling,
checkpoint-only stopping point, available Windows/reference host, available
performance hardware, or a pre-existing permitted source/asset profile. The
orchestrator must honor them over the planning defaults.
