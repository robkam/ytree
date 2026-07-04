---
name: code-auditor-gate
description: Perform adversarial ytnova code review with severity-ranked findings, concrete fixes, and pass-fail gate decision.
---

# Code Auditor Gate

Use this skill when the active persona is `code_auditor`.

## Review Workflow

1. Read changed files and relevant architecture/spec docs.
2. Check correctness, memory safety, portability, maintainability, and performance regressions in interactive flows.
3. Validate architecture invariants and conceptual integrity (ownership, isolation, redraw control, architecture conformance).
4. Run the mandatory Security Checklist before finalizing findings.
5. Validate source comment quality: invariants/rationale are documented where needed, and no stale or change-diary comments remain.
6. Validate documentation signal quality: new guidance appears in the most relevant canonical location, with no redundant AI/process broadcast across unrelated sections.
7. Validate UX economy for interactive flows (common-path depth, prompt chaining, fast path).
8. Produce findings first, ordered by severity.
9. End with explicit gate status and residual risks.

## Conceptual Integrity / Architecture Conformance Lens

When reviewing, explicitly look for canonical whole-system failure modes:

- conceptual integrity loss: the codebase no longer feels like one coherent design
- architectural drift: implementation no longer matches the documented design
- architectural erosion: architectural constraints are actively violated
- shotgun surgery: one logical change forces scattered edits across many files
- duplicate or parallel implementations: old and new paths survive side by side
- divergent sibling semantics: similar features implement the same concept differently without a documented reason
- half-migrated replacements: old code remains reachable, inert, or only partially retired
- event-loop nondeterminism: user-visible behavior changes under resize, watcher, or bursty input conditions
- signal-safety violations: complex logic, I/O, or ncurses calls in signal handlers

Required review questions:

- Is there exactly one authoritative implementation for each durable behavior?
- Does the implementation still conform to the intended architecture and documented invariants?
- Are any surviving alternatives intentional transition paths, or accidental leftovers?
- Do similar code paths still express the same concept in the same way?
- Would a maintainer understand the system as a coherent whole, or as stitched-together fragments?
- Does the change avoid introducing avoidable blocking, redraw lag, or input-latency regressions?
- Does the change preserve single-threaded event-loop determinism and signal-safety boundaries?

When this lens finds a problem, identify:

- the surviving duplicate, drift, or erosion point,
- whether it is dead, transitional, or still user-reachable,
- the architectural or conceptual-integrity breakage it causes,
- and the single durable owner or consolidation path.

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
