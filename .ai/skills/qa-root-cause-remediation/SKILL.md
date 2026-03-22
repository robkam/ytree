---
name: qa-root-cause-remediation
description: Enforce root-cause remediation for QA failures; no pass-by-patch, no test-to-green hacks, and no local suppressions without explicit approval.
---

# QA Root Cause Remediation

Use this skill whenever a QA gate fails (build, static analysis, sanitizer, valgrind, or pytest).

## Mandatory Rules

1. Fix the underlying defect that caused the failure.
2. Do not patch around the check by weakening behavior or bypassing failing paths.
3. Do not change tests only to force green unless the test is demonstrably wrong against spec.
4. Do not add local suppressions (`NOLINT`, `xfail`, `skip`, ignore lists, disable flags) as a shortcut.
5. If temporary suppression is the only safe short-term option, stop and get explicit user approval first, with rollback plan.

## Required Evidence

- Failing gate command and concise failure summary (before fix)
- Root-cause statement tied to code path or invariant
- Minimal changes made and why they address the root cause
- Passing rerun of the same failing gate
- If suppression was approved, include scope and removal plan

## Escalation

- If remediation requires broad refactoring, present options and tradeoffs before changing architecture.
- If failure indicates a spec mismatch, align spec, test, and code explicitly; do not silently alter tests.
