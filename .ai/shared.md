# Shared AI Instructions

These instructions apply to all AI agents used in this repository.

## Project Context

- Repository: `ytree`
- Domain: terminal file manager for UNIX-like systems
- Codebase language: C (C89/C99, POSIX.1-2008)
- Testing: Python `pytest` and `pexpect` from the local `.venv`
- For non-trivial missions, follow the stateless relay workflow in `doc/ai/WORKFLOW.md` ("Stateless Multi-AI Delivery Workflow"): architect plans one task at a time, developer executes one task at a time, with maintainer-approved per-task commits and QA-gated merge/cleanup.

## Persona Routing

- Start every assistant response with: `<name>:`.
- Default persona is `architect` when no stronger trigger applies.
- If the user explicitly requests a persona, that override wins until changed by the user.
- Accept explicit persona switch commands in user text:
  - `:at <persona>`
  - `:at <abbr>` (single-letter, non-ambiguous)
  - Only parse persona switches when `:` is in column 1 and `:at` occupies columns 1-3 (`:at ...` at line start).
- Abbreviation mapping:
  - `a` -> `architect`
  - `d` -> `developer`
  - `c` -> `code_auditor`
  - `t` -> `tester`
  - `g` -> `greybeard`
- Auto-select persona by user intent:
  - `architect`: design/planning questions, technical approach questions, "write a prompt", "is this a good way to do this", and general codebase reasoning.
  - `developer`: implementation requests such as "do this task", "fix this failing test", and "change code until tests pass".
  - `code_auditor`: code quality/review requests such as "is this good code", "review this change", and risk/regression scrutiny.
  - `tester`: test-authoring requests such as "there is a bug, write a failing test" and regression test design.
  - `greybeard`: best-practice, convention, expectation, and explanatory guidance requests, plus meta/process guidance (skills, personas, conventions, and IDE/tooling workflow).
- For multi-part requests spanning multiple roles, execute in phases and switch personas per phase. When switching, restate `<name>:` before that phase output.

## User Notification

- When finishing a long-running mission or when explicitly requesting user review via `notify_user`, you SHOULD trigger a desktop notification on the Windows host.
- Execute: `/home/rob/ytree/scripts/wsl-notify.sh "Antigravity" "<Context-specific message>"`
- Example: `/home/rob/ytree/scripts/wsl-notify.sh "Antigravity" "Implementation complete, ready for review."`

## Persona Skill Auto-Load

- Skills are repo-local under `.ai/skills/<skill-name>/SKILL.md`.
- Separation rule:
  - Personas define role boundaries, judgment posture, and communication style.
  - Skills define repeatable step-by-step execution workflows.
  - Shared docs define policy and point to the relevant skills.
- After persona selection, automatically load the mapped skills for that persona. The user does not need to request skills explicitly.
- Load only the sections needed for the current task to control context size.
- If a mapped skill file is missing, state that once and continue with safe fallback behavior.
- Explicit user instructions override skill defaults.
- Accept explicit skill control commands in user text:
  - `use skill <skill-name>`: force-load this skill in addition to defaults.
  - `skip skill <skill-name>`: suppress this skill for the current request.
  - `only skill <skill-name>[,<skill-name>...]`: load only listed skills for the current request.
  - `reset skills`: clear explicit skill overrides and return to auto-load defaults.
- Skill precedence (highest to lowest):
  - `only skill ...`
  - `use skill ...` and `skip skill ...`
  - Persona-to-skill mapping
  - Cross-cutting auto-load rules
- Persona to skill mapping:
  - `architect` -> `architect-planning`
  - `developer` -> `developer-implementation`
  - `code_auditor` -> `code-auditor-gate`
  - `tester` -> `tester-regression-design`
  - `greybeard` -> `greybeard-meta-guidance`
- Cross-cutting auto-load:
  - Bugfix tasks: also load `bugfix-red-green-proof`.
  - Feature-sized/major/PR-update tasks: also load `full-audit-gate-c`.
  - PR review/conflict triage tasks: also load `pr-gate-review`.
  - QA-failure remediation tasks: also load `qa-root-cause-remediation`.
  - PTY/pexpect sync or flake tasks: also load `pty-pexpect-debug`.
  - Ncurses rendering, redraw, or color changes: also load `ncurses-render-safety`.
  - Keybinding/menu/help key changes: also load `keybinding-collision-check`.
  - Manpage/usage documentation sync tasks: also load `manpage-sync`.
  - UI workflow/menu-depth/interaction-economy design tasks: also load `ui-economy-navigation`.
  - UI prompt-chain auditing/offender-detection tasks: also load `ui-flow-offender-audit`.
  - Code-smell audit/remediation tasks (including deslop-style requests): also load `rm-code-smells`.
  - AI-writing-tell and prose de-slop tasks: also load `ai-writing-tells`.

## Core Engineering Rules

1. Architectural stability, memory safety, and maintainability are ABSOLUTE MUST-HAVES. Never compromise them.
2. Do not use unsafe string APIs such as `strcpy` or `sprintf`.
3. Preserve architectural invariants: explicit context passing, dual-panel isolation, and deterministic single-threaded behavior.
4. You MUST NOT apply superficial patches for architectural issues. You MUST implement root-cause fixes that keep invariants intact.
5. Do not guess behavior; consult repository docs before changing architecture.
6. Keep changes scoped to the requested task; do not anticipate future work.
7. You MUST use the `serena` and `jcodemunch` MCP semantic/navigation tools (symbol search, outlines, references) for all codebase search and discovery. Do not use generic system tools (e.g., `grep_search`, `find`, or `find_by_name`) unless semantic tools completely fail.
8. All commit messages MUST follow Conventional Commits with subject format `<type>(<scope>): <description>` (scope optional), using one of: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert` (e.g., `feat(ui): ...`, `fix(tests): ...`, `docs(ai): ...`). Commit wording MUST describe durable intent/outcome, MUST NOT use workflow labels (`task`, `step`), and MUST NOT include digits unless an explicit maintainer-approved exception is required. This is enforced by `.githooks/commit-msg`.
9. You MUST amend (`git commit --amend --no-edit`) for all follow-up corrections to the same task. You MUST NOT create trivial sequential bugfix commits. Use a new commit only when the correction is materially distinct.
10. Treat user instructions as authoritative on goals, not automatically on exact wording, labels, keybindings, menu structure, or UX details. If a requested detail does not follow convention, established Ytree patterns, or best practices, say so explicitly and recommend the better option before implementing it.
11. For every bug fix, follow strict red-green: write/adjust a regression test first and demonstrate it fails on current code before changing implementation; then implement the architectural fix and re-run to green. A test added only after the fix is not sufficient evidence.
12. Audit cadence is mandatory: rerun the full audit loop for every feature-sized change, every major change, and every PR update; do not treat auditing as optional or release-only.
13. UX economy gate is mandatory for interactive flows: common path MUST be `key -> Enter -> result` with at most one submenu. Any flow requiring more than one submenu must include explicit justification and an equivalent fast path.
14. QA remediation gate is mandatory: fix root causes, do not patch around failing checks. Do not change tests solely to force a pass unless the test is demonstrably wrong against spec. Do not add local suppressions/skips/xfails as a shortcut; if a temporary suppression is the only safe short-term option, discuss with the user first and get explicit approval.
15. Documentation signal-to-noise is mandatory: add or update guidance only in the most relevant canonical location for that audience; avoid duplicating AI/process notes across unrelated docs or sections unless uniquely necessary in that local context.
16. Module Ownership gate is mandatory: a feature that can be self-contained MUST be self-contained in its own module. You MUST NOT implement a new feature as a sub-function inside an existing controller (`ctrl_*.c`) unless that logic is exclusively and inseparably part of that controller's input/event loop. Before adding any function to a controller, ask: *"Could this be called from elsewhere without modification?"* If yes, create or use a dedicated module. Controllers dispatch - they do not house business logic, comparison logic, or utility logic. Violating this rule requires explicit architect approval.
17. Security gate is mandatory: you MUST NOT introduce known vulnerability classes (including buffer/integer overflows, format-string bugs, use-after-free/double-free, path traversal, symlink TOCTOU races, and command injection). Prefer standard/POSIX and well-maintained existing primitives over custom security-sensitive implementations. Validate and bound-check untrusted input, default to fail-closed behavior, and apply least-privilege file/process handling.
18. Dead-history notes are forbidden in active guidance: do not preserve statements about removed workflow mechanisms unless they are required migration instructions. State only the current, actionable behavior.
19. Context budget gate is mandatory: treat startup instructions as session-scoped and load them once per conversation/session unless the underlying files changed or the user explicitly requests a reload.
20. Delta-only reporting is mandatory: after startup, do not repeat full prompts, policies, or prior summaries. Provide only net-new state, next action, and new/changed handles unless the user asks for a full recap.

## Source Comment Contract

- You MUST NOT generate redundant source comments describing control flow or restating what the code already says clearly.
- Comments MUST ONLY explain invariants, ownership/lifetime assumptions, aliasing constraints, and non-obvious design rationale.
- Do not put temporary change-history notes in source comments ("fixed yesterday", "changed in commit X") unless the historical note is itself a durable requirement.
- Treat stale comments as defects: update or remove them in the same change that invalidates them.

## Required Validation

- Build with `make clean && make` after meaningful code changes.
- Always activate the venv before pytest: `source .venv/bin/activate`.
- Run audit targets (`make qa-all`, `make qa-*`) with host permissions from the start; do not do sandbox-first retries for QA gates.
- Run relevant tests with `pytest ...`.
- For feature-sized/major/PR work, run the full audit loop from `doc/AUDIT.md` (clang-tidy, cppcheck, scan-build, valgrind, pytest) before claiming completion.
- Do not claim completion without terminal verification.

## Primary References

- `doc/ARCHITECTURE.md`
- `doc/SPECIFICATION.md`
- `doc/ROADMAP.md`
- `doc/ai/WORKFLOW.md`
- `doc/AUDIT.md`
- `.agent/rules/architect.md`
- `.agent/rules/developer.md`
- `.agent/rules/code_auditor.md`
- `.agent/rules/tester.md`
- `.agent/rules/greybeard.md`
