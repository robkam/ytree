# CODEX.md

Discovery stub for Codex tooling.

**MANDATORY AI INITIALIZATION**:
Before doing any codebase research or making tool calls, you **MUST** read `./.ai/codex.md` and `./.ai/shared.md`.
You **MUST** use the MCP semantic tools (`serena` and `jcodemunch`) for all searching and codebase exploration instead of fallback system tools (`grep`/`find`).

Canonical Codex instructions: `./.ai/codex.md`
Shared instructions for all agents: `./.ai/shared.md`

Docs note: `docs/USAGE.md` is generated from `etc/ytree.1.md`; edit `etc/ytree.1.md` as source.

Commit messages: see `./.ai/shared.md` rule 8 (enforced by `.githooks/commit-msg`).

Testing quick reference (Codex):
- Run from repo root: `~/ytree`
- Activate venv: `source .venv/bin/activate`
- Full suite command: `pytest`
- Always run pytest with host permissions from the start (no sandbox-first run), because PTY-based tests require unrestricted PTY allocation.
- Run audit targets (`make qa-all`, `make qa-*`) with host permissions from the start (no sandbox-first run), because toolchain steps can require unrestricted environment access.
- Pre-merge quality gate is PR full-QA CI (`make qa-all` equivalent). Local `make qa-all` is optional unless maintainer-requested.

MCP health check (Codex):
- Diagnose MCP startup/config drift: `make mcp-doctor`
- Bootstrap missing `~/.codex/config.toml` from repo `.codex/config.toml` and add safe `UV_CACHE_DIR`/`UV_TOOL_DIR` env blocks: `make mcp-doctor FIX=1`

Persona quick switch:
- Full names: `:at architect`, `:at developer`, `:at code_auditor`, `:at tester`, `:at greybeard`
- Abbreviations: `:at a`, `:at d`, `:at c`, `:at t`, `:at g`
- Parsing rule: persona switching only triggers when `:` is in column 1 and `:at` occupies columns 1-3 (`:at ...` at line start).
- Default when no explicit override is `architect` (see `./.ai/shared.md`)

Persona skill auto-load:
- Skills live in `./.ai/skills/*/SKILL.md`.
- After persona selection, mapped skills are loaded automatically (no extra user command required).
- Optional explicit skill controls: `use skill <name>`, `skip skill <name>`, `only skill <name>[,<name>...]`, `reset skills`
- Core mapping:
  - `architect` -> `architect-planning`
  - `developer` -> `developer-implementation`
  - `code_auditor` -> `code-auditor-gate`
  - `tester` -> `tester-regression-design`
  - `greybeard` -> `greybeard-meta-guidance`
- Cross-cutting mapping:
  - bugfix -> `bugfix-red-green-proof`
  - feature-sized/major/PR update -> `full-audit-gate-c`
  - QA-failure remediation -> `qa-root-cause-remediation`
  - PTY/pexpect flake debugging -> `pty-pexpect-debug`
  - ncurses rendering/redraw/color work -> `ncurses-render-safety`
  - keybinding/menu/help key changes -> `keybinding-collision-check`
  - manpage/usage doc sync tasks -> `manpage-sync`
  - UI workflow/menu-depth/interaction-economy design -> `ui-economy-navigation`
  - UI prompt-chain offender detection/audit -> `ui-flow-offender-audit`
  - code-quality audit/remediation (clean-code/deslop-style) -> `code-quality`
  - AI-writing-tell/prose de-slop tasks -> `ai-writing-tells`

UX economy gate:
- Common path MUST be `key -> Enter -> result` with at most one submenu.

QA remediation gate:
- Fix root causes for failing QA checks; do not patch around failures or add local suppressions without explicit user approval.
