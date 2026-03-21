---
name: pty-pexpect-debug
description: Debug flaky PTY/pexpect tests in ytree with deterministic synchronization and minimal timeout use.
---

# PTY Pexpect Debug

Use this skill when tests involve PTY timing or synchronization issues.

## Workflow

1. Create minimal reproducible failing test path.
2. Identify race boundary (input, redraw, prompt, filesystem state).
3. Replace blind sleeps with explicit synchronization checks.
4. Keep timeouts short and intentional.
5. Verify stability across multiple reruns.

## Rules

- Avoid long timeout inflation as a fix.
- Prefer deterministic wait-for-state assertions.
- Report exact flaky trigger and stabilization method.
