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
4. Apply the source comment contract: document invariants/rationale where needed, and remove or update stale comments touched by the change.
5. Build and run relevant tests before completion.
6. Report evidence, not claims.

## Guardrails

- No unsafe string APIs (`strcpy`, `sprintf`, `strcat`).
- No unrelated refactors.
- If instruction conflicts with invariants, state the conflict and choose a safe path.
- Do not add "change diary" comments; keep historical narrative in commits, not source.
- Documentation signal rule: when updating docs, add or edit guidance only where contextually relevant to that section's audience; avoid duplicating AI/tooling notes in multiple unrelated sections.

## Completion Evidence

- What changed (file-level)
- Why it fixes the issue
- Build/test commands executed
- Observed pass/fail results
