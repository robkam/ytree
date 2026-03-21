# CODEX.md

Discovery stub for Codex tooling.

Canonical Codex instructions: `./.ai/codex.md`
Shared instructions for all agents: `./.ai/shared.md`

Docs note: `doc/USAGE.md` is generated from `etc/ytree.1.md`; edit `etc/ytree.1.md` as source.

Testing quick reference (Codex):
- Run from repo root: `/home/rob/ytree`
- Activate venv: `source .venv/bin/activate`
- Full suite command: `pytest`
- Always run pytest with host permissions from the start (no sandbox-first run), because PTY-based tests require unrestricted PTY allocation.

Persona quick switch:
- Full names: `use architect`, `use developer`, `use code_auditor`, `use tester`, `use greybeard`
- Abbreviations: `use a`, `use d`, `use c`, `use t`, `use g`
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
  - PTY/pexpect flake debugging -> `pty-pexpect-debug`
