---
name: bugfix-red-green-proof
description: Enforce strict ytree bugfix loop: failing regression first, implementation second, passing proof third.
---

# Bugfix Red Green Proof

Use this skill for any bugfix work, regardless of persona.

## Mandatory Sequence

1. Reproduce bug with a targeted regression test (RED).
2. Run and show that the test fails before code fix.
3. Implement minimal root-cause fix.
4. Re-run targeted tests to green.
5. Run broader relevant tests to guard against regressions.

## Evidence

- Command list in order
- Failing result before fix
- Passing result after fix
- Notes on why fix addresses root cause
