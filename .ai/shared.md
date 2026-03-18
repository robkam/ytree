# Shared AI Instructions

These instructions apply to all AI agents used in this repository.

## Project Context

- Repository: `ytree`
- Domain: terminal file manager for UNIX-like systems
- Codebase language: C (C89/C99, POSIX.1-2008)
- Testing: Python `pytest` and `pexpect` from the local `.venv`

## Core Engineering Rules

1. Prioritize architectural stability, memory safety, and maintainability.
2. Do not use unsafe string APIs such as `strcpy` or `sprintf`.
3. Preserve architectural invariants: explicit context passing, dual-panel isolation, and deterministic single-threaded behavior.
4. Avoid superficial patching for architectural issues; prefer root-cause fixes that keep invariants intact.
5. Do not guess behavior; consult repository docs before changing architecture.
6. Keep changes scoped to the requested task; do not anticipate future work.
7. Prefer MCP semantic/navigation tools (symbol search, outlines, references) over loading large files into context unnecessarily.
8. All commit messages must follow the Conventional Commits specification style (e.g., `feat(ui): ...`, `fix(tests): ...`, `docs(ai): ...`).
9. If the immediately preceding step only needs a minor correction and should remain the same logical history unit, amend it with `git commit --amend --no-edit` instead of creating a trivial follow-up fix commit. Use a new commit only when the correction is materially distinct or worth preserving separately.
10. Treat user instructions as authoritative on goals, not automatically on exact wording, labels, keybindings, menu structure, or UX details. If a requested detail does not follow convention, established Ytree patterns, or best practices, say so explicitly and recommend the better option before implementing it.
11. For every bug fix, follow strict red-green: write/adjust a regression test first and demonstrate it fails on current code before changing implementation; then implement the architectural fix and re-run to green. A test added only after the fix is not sufficient evidence.

## Required Validation

- Build with `make clean && make` after meaningful code changes.
- Always activate the venv before pytest: `source .venv/bin/activate`.
- Run relevant tests with `pytest ...`.
- Do not claim completion without terminal verification.

## Primary References

- `doc/ARCHITECTURE.md`
- `doc/SPECIFICATION.md`
- `doc/ROADMAP.md`
- `doc/ai/WORKFLOW.md`
- `doc/ai/PRE_RELEASE_REVIEW.md`
- `.agent/rules/architect.md`
- `.agent/rules/developer.md`
- `.agent/rules/code_auditor.md`
- `.agent/rules/tester.md`
