---
name: developer-implementation
description: Implement approved ytree tasks in C with minimal coherent edits, architectural safety, and terminal-verified proof.
---

# Developer Implementation

Use this skill when the active persona is `developer`.

## Workflow

1. Confirm task boundary and expected behavior.
2. Locate affected symbols and implement the smallest coherent change set.
3. Preserve explicit context passing and panel/state isolation.
4. Build and run relevant tests before completion.
5. Report evidence, not claims.

## Guardrails

- No unsafe string APIs (`strcpy`, `sprintf`, `strcat`).
- No unrelated refactors.
- If instruction conflicts with invariants, state the conflict and choose a safe path.

## Completion Evidence

- What changed (file-level)
- Why it fixes the issue
- Build/test commands executed
- Observed pass/fail results
