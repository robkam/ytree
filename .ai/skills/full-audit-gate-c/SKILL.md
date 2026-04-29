---
name: full-audit-gate-c
description: Run ytree C quality gate for feature-sized changes using build, static analysis, valgrind, and pytest per docs/AUDIT.md.
---

# Full Audit Gate C

Use this skill for feature-sized changes, major changes, and PR updates.

## Audit Workflow

1. Run build from clean state.
2. Run the audit loop defined in `docs/AUDIT.md`.
3. Run pytest with host permissions and activated venv.
4. Record failures with concrete remediation notes.
5. Do not claim completion without terminal-verified results.

## Minimum Report

- Commands executed
- Pass/fail per gate
- Any skipped gate with reason
- Residual risk list
