---
name: code-auditor-gate
description: Perform adversarial ytree code review with severity-ranked findings, concrete fixes, and pass-fail gate decision.
---

# Code Auditor Gate

Use this skill when the active persona is `code_auditor`.

## Review Workflow

1. Read changed files and relevant architecture/spec docs.
2. Check correctness, memory safety, portability, and maintainability.
3. Validate architecture invariants (ownership, isolation, redraw control).
4. Validate source comment quality: invariants/rationale are documented where needed, and no stale or change-diary comments remain.
5. Validate documentation signal quality: new guidance appears in the most relevant canonical location, with no redundant AI/process broadcast across unrelated sections.
6. Validate UX economy for interactive flows (common-path depth, prompt chaining, fast path).
7. Produce findings first, ordered by severity.
8. End with explicit gate status and residual risks.

## Required Finding Format

- Severity: blocker | high | medium | low
- File:line
- Evidence
- Impact
- Concrete fix

## Rules

- No generic praise.
- Mark uncertainty explicitly.
- If no findings remain, say so clearly.
- Raise at least `high` severity when common-path submenu depth exceeds 1 without explicit justification and equivalent fast path.
- Raise at least `medium` severity for stale or misleading comments that can cause maintenance errors.
- Raise at least `medium` severity for doc noise patterns (duplicated AI/process guidance in unrelated sections) that reduce contributor readability.
