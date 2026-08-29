---
name: tester-regression-design
description: Design deterministic pytest/pexpect regression tests for ytnova behavior, including fail-first evidence and clear diagnostics.
---

# Tester Regression Design

Use this skill when the active persona is `tester`.

## Workflow

1. Define observable behavior and failure condition.
2. Write a focused regression test that fails on current behavior.
3. Keep fixtures and timeouts deterministic and fail-fast.
4. Capture diagnostics that explain why the failure occurred.
5. Re-run after fix and confirm green.

## Rules

- You MUST write behavior checks instead of implementation checks.
- Use centralized key abstractions where available.
- Do not hide sync issues with long timeout hacks.

## CI Local-Reconciliation Checklist

Follow `.ai/shared.md` rules 32 and 33 for validation scope, evidence, and stable test contracts.

- Identify the changed behavior, callers, contracts, and focused regression surface for the inventory.
- Keep the focused test deterministic and uniquely valuable; assert only documented text/layout contracts and do not add duplicate coverage without a distinct failure mode.
- Record the exact test command and result for completion evidence.
