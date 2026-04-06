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
4. Run the mandatory Security Checklist before finalizing findings.
5. Validate source comment quality: invariants/rationale are documented where needed, and no stale or change-diary comments remain.
6. Validate documentation signal quality: new guidance appears in the most relevant canonical location, with no redundant AI/process broadcast across unrelated sections.
7. Validate UX economy for interactive flows (common-path depth, prompt chaining, fast path).
8. Produce findings first, ordered by severity.
9. End with explicit gate status and residual risks.

## Security Checklist (Mandatory)

- Check for newly introduced vulnerability classes: buffer/integer overflow, format-string misuse, use-after-free/double-free, path traversal, symlink TOCTOU races, command injection.
- Verify untrusted inputs are validated and explicitly bounded before use in memory/file/process operations.
- Verify security-sensitive behavior fails closed on invalid or unexpected states.
- Verify file/process operations use least privilege and avoid broad permissions/escalation by default.
- Prefer standard/POSIX or well-maintained existing primitives over custom security-sensitive implementations; flag custom replacements unless strongly justified.

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
