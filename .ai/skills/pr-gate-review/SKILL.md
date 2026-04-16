---
name: pr-gate-review
description: Enforce structured PR scrutiny with risk-focused findings, plain-language summaries, and conflict triage outputs.
---

# PR Gate Review

Use this skill when reviewing pull requests or triaging merge conflicts.

## Objectives

1. Surface regressions, security risks, and missing tests before merge.
2. Produce plain-language explanations for maintainers who do not review raw diffs.
3. Provide actionable conflict resolution guidance.

## Review Flow

1. Restate PR intent in plain language.
2. Identify behavior changes and blast radius.
3. Run a security checklist:
   - input validation and bounds checks
   - memory safety and ownership
   - path/file/process safety
   - privilege and fail-closed behavior
4. Validate tests:
   - regression test coverage for changed behavior
   - negative path coverage for risky code paths
5. Classify findings by severity (`blocker`, `high`, `medium`, `low`).
6. End with explicit merge recommendation (`approve` or `do-not-merge`).

## Conflict Triage Flow

1. List conflicting files.
2. For each file, state likely intent of both sides.
3. Propose a merge strategy:
   - keep ours
   - keep theirs
   - combine with ordered edits
4. Mark follow-up tests required after conflict resolution.
5. State residual risk after resolution.

## Required Output Shape

- `Summary`
- `Findings (severity-ranked)`
- `Required fixes before merge`
- `Conflict guidance` (only when conflicts exist)
- `Final recommendation`
