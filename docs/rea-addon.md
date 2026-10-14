# REA addon — project-scoped install

**REA (Reverse Engineer Anything)** — <https://github.com/morluto/rea> — is an
MCP addon that gives coding agents tools to inspect binaries, installers and
applications and explain what they do. In this workspace it is the tool for
analysing `downloads/GTA2INSTALLER.exe` and the extracted game files.

This install is deliberately **project-scoped**: everything lives inside this
workspace, and no user-level/global agent configuration was modified. Opening
any other project will not see REA.

## What was installed

| Path | Purpose |
| --- | --- |
| `package.json`, `package-lock.json` | Pins `rea-agents` at exactly `6.4.0` (2026-10-11 release). |
| `node_modules/` | The REA CLI + MCP server, local to this project. No global npm install. |
| `.mcp.json` | Project-scope MCP registration for Claude Code (only loads when Claude Code is opened in this folder). |
| `.cursor/mcp.json` | Project-scope MCP registration for Cursor (same). |
| `.claude/skills/reverse-engineer-anything/` | REA's bundled agent workflow instructions, copied from the installed package (skill version 37). |

Both `mcp.json` files launch the local copy directly:

```
node  "./node_modules/rea-agents/scripts/rea.mjs"  mcp
```

If Node moves, update both files. Both registrations point at this exact
installed copy, so the served version is pinned by `package-lock.json`.

## Using it

- **In an agent (Claude Code / Cursor):** open *this* folder as the workspace,
  approve the project MCP server when prompted, and restart the agent. REA's
  ~170 tools appear as `rea` MCP tools, and the skill loads from
  `.claude/skills/`.
- **In a terminal (works with no agent at all):**

  ```sh
  ./node_modules/.bin/rea --help
  # e.g. static inspection of an Electron/JS tree:
  ./node_modules/.bin/rea analyze-javascript-application /absolute/path --json
  ```

## What is intentionally NOT installed

- No global npm package (`rea-agents` is absent from `npm root -g`).
- No user-scope registrations: `~/.claude.json`, `~/.gwh/cordis.patch.yml`
  (DeepSeek Harness), Codex, etc. are untouched. `rea setup` was not run — it
  only writes user-scope registrations, which would leak REA into every other
  workspace.
- **DeepSeek Harness (this app):** its only MCP mechanism is the *home* patch
  (`$DSH_HOME/cordis.patch.yml`), which applies to every Harness workspace —
  conflicting with the project-only requirement. To use REA in a Harness
  session here, run the CLI from this workspace instead
  (`./node_modules/.bin/rea …`).
- Native analysis engines (Hopper, Ghidra, IDA) — REA treats these as
  bring-your-own and installs nothing. Static JavaScript/.NET inspection needs
  none. Decompiling the 32-bit GTA2 PE binaries needs one of them later; see
  below.

## `rea doctor` notes (expected, not faults)

`./node_modules/.bin/rea doctor` audits *user-scope* locations, so it reports:

- `hopper` / `ghidra` / `ida-registration` — `missing_analysis_engine`
  (optional; install one only when you want native decompilation; Ghidra route:
  `GHIDRA_INSTALL_DIR` pointing at a Ghidra 12.1.x release + JDK 21+).
- `skill:identity` / `registration:*` — "missing" because ours are
  project-scoped, which the audit does not read.

Verified on install (2026-10-12): `rea --version` → 6.4.0; MCP `initialize`
handshake against the local server returned `serverInfo: rea 6.4.0`; no global
files were created or modified.

## Updating

```sh
npm install --save-exact rea-agents@latest
rm -rf .claude/skills/reverse-engineer-anything
cp -R node_modules/rea-agents/skills/reverse-engineer-anything .claude/skills/
```

Then restart the agent. Re-run the handshake smoke test if unsure.

## Uninstalling

Delete the rows in the table above: `package.json`/`package-lock.json`
(REA entries), `node_modules/`, `.mcp.json`, `.cursor/mcp.json`,
`.claude/skills/reverse-engineer-anything/`. Nothing outside the workspace
needs cleaning.
