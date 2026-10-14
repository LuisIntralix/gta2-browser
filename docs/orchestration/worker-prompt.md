# GTA2 worker assignment template

The orchestrator fills every bracketed field before dispatch. Attach only the
relevant contract sections, source excerpts and evidence; workers may retrieve
more when needed. Do not dispatch this template with unresolved placeholders.

```text
You own GTA2 task [task ID / child ID] on baseline [commit].
Objective: [one independently verifiable outcome].
Non-goals: [explicit exclusions].
Model and reasoning: [approved route].
Worktree: [absolute path].
Read first: [AGENTS.md, relevant spec sections, contract versions, source paths].
You may write only: [exact files or bounded directories].
Dependencies already accepted: [IDs and evidence].
Binding decisions: [Dxx IDs and relevant decisions.md excerpts].
Existing evidence to reuse: [IDs and artifact paths].
Required checks: [commands or precise runtime steps and expected outcomes].
Token ceiling: [number]; fallback call ceiling if unavailable: [number].
Independent reviewer: [role/agent]; return bundle: [path].

Implement the selected design; do not compare alternative frameworks, engines,
transports, codecs or authority models. Source reading and the prescribed tests
are allowed; a contradictory fact goes to the orchestrator with its decision ID.
Implement this bounded change, run meaningful checks and return a reviewable
patch/commit with evidence. Keep source observations, runtime observations,
inferences and unknowns separate. Do not alter expectations, remove features,
add a fake-success service, update upstream/toolchain versions, or modify files
outside your lease to make the test pass. Propose any necessary contract change
with its impact and wait for orchestrator coordination on that shared change.

Preserve downloads and game assets. No global installs, rea setup, original
EXE downloads, public uploads, external messages or new paid infrastructure.
Use ordinary source tools for repository questions. For genuine native binary
questions, follow the local REA skill and close the session. Do not install
Ghidra without the user's decision. Treat upstream build scripts as code to
inspect; their side effects are not authorized by their documentation.

If two attempts fail on the same root blocker, or the packet reaches its
ceiling, stop that packet and return the minimal reproduction plus the next
discriminating experiment. Do not keep re-reading the entire repository.
At 80% allowance, record a reviewable checkpoint and updated forecast.

Return: changed paths, commit/patch, observed behavior, checks with actual exit
results and artifact paths, remaining defects, unknowns, measured usage or
explicit unknown, and the smallest next action. A candidate is not accepted
until independent review and integration checks pass.
```

Result record example shape; replace values with evidence, never guessed usage:

```json
{
  "task_id": "T06.01",
  "status": "review",
  "baseline_revision": "<commit>",
  "commit_or_patch": "<commit or local patch path>",
  "changed_paths": [],
  "observations": [],
  "inferences": [],
  "unknowns": [],
  "checks": [
    {"command_or_steps": "<actual check>", "exit_status": 0,
     "artifact_path": "<saved log>", "evidence_id": "<unique ID>"}
  ],
  "usage": {
    "source": "unavailable",
    "model": "gpt-6.1-sol",
    "input_tokens": null,
    "cached_read_tokens": null,
    "cache_write_tokens": null,
    "output_including_reasoning_tokens": null,
    "calls": null,
    "cost_usd": null
  },
  "remaining_defects": [],
  "next_action": "Independent review"
}
```

Reviewer checklist: owned scope respected; dependencies/contract version current;
no forbidden files; implementation matches actual requirement; checks detect the
failure being fixed; oracle/thresholds unchanged without a reviewed reason;
runtime claims match evidence; known defects preserved; integration smoke passes.
The reviewer returns `accept` or `rework` with concrete findings. Only the
orchestrator updates the task to `accepted` after integration.
