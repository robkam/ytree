---
name: tester-regression-design
description: Design deterministic pytest/pexpect regression tests for ytree behavior, including fail-first evidence and clear diagnostics.
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
