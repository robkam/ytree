---
name: developer-implementation
description: Implement approved ytnova tasks in C with minimal coherent edits, architectural safety, and terminal-verified proof.
---

# Developer Implementation

Use this skill when the active persona is `developer`.

## Workflow

1. Confirm task boundary, expected behavior, and the inventory that defines in-scope surfaces.
2. Locate affected symbols and implement the smallest coherent change set that still closes the whole inventoried slice.
3. Preserve explicit context passing and panel/state isolation.
4. Apply the source comment contract: document invariants/rationale where needed, and remove or update stale comments touched by the change.
5. Reconcile the inventory before completion: every item must be addressed, intentionally unchanged with reason, or deferred/blocked with reason.
6. Build and run relevant tests before completion.
7. Report evidence, not claims.

## Guardrails

- No unsafe string APIs (`strcpy`, `sprintf`, `strcat`).
- No unrelated refactors.
- If instruction conflicts with invariants, state the conflict and choose a safe path.
- Silent omission is forbidden: do not stop at the first green helper cluster, obvious call site, or reproducer if adjacent inventoried surfaces remain unresolved.
- Do not add "change diary" comments; keep historical narrative in commits, not source.
- Documentation signal rule: when updating docs, you MUST add or edit guidance only where contextually relevant to that section's audience; you MUST NOT duplicate AI/tooling notes in multiple unrelated sections.

## Completion Evidence

- What changed (file-level)
- Why it fixes the issue
- Inventory reconciliation summary
- Build/test commands executed
- Observed pass/fail results
