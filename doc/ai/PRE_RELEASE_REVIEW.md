# Pre-Release Review Workflow

This document defines the default quality gate before a change is considered release-ready.

Goal: minimize defects, regressions, and architectural drift before external review or release.

---

## 1. Role Mapping

This workflow uses the personas in `.agent/rules/`:

- Architect: defines scope, constraints, and acceptance criteria.
- Developer: implements fixes within the approved scope.
- Code Auditor: performs adversarial, evidence-based review and gate decisions.
- Tester: validates behavior and regression safety.

This is not a single-persona activity. It is a strict loop across all four roles.

---

## 2. Default Loop

### Phase A: Architecture Check (Architect)

Confirm scope, invariants, and acceptance criteria for the change.

Output:
- scope
- affected modules/symbols
- acceptance criteria
- known risk areas

### Phase B: Audit (Code Auditor)

Run a strict review of the target diff and affected code paths.

For every finding provide:
- severity: blocker | high | medium | low
- file:line
- evidence
- impact
- concrete fix

Rules:
- no generic praise
- no vague comments
- mark uncertainty explicitly

### Phase C: Fix (Developer)

Implement fixes for all blocker/high findings and medium findings that are safe to address now.

Rules:
- preserve architectural invariants
- prefer root-cause fixes over symptom patches
- keep changes scoped and testable

### Phase D: Verify (Tester)

Run build and tests relevant to the changed area, then broader regressions.

Minimum checks:
- build succeeds (`make clean && make` or project equivalent)
- relevant `pytest` suites pass
- no new failures in critical previously-stable suites

### Phase E: Final Gate (Code Auditor)

Re-audit the final diff.

Exit criteria:
- blocker = 0
- high = 0
- verification green
- any remaining medium/low items listed as residual risk

Repeat A-B-C-D-E until gate passes.

---

## 3. Release Report

Before release, produce a concise gate report:

- scope reviewed
- findings summary by severity
- tests run and results
- residual risks
- final status: PASS or FAIL

Gate policy:
- PASS only when blocker/high are zero and verification is green.

---

## 4. ytree-Specific Priorities

- Stability over novelty.
- Portability across SSH/tmux terminal environments.
- Deterministic behavior and clear state ownership.
- Architectural fixes for redraw/state bugs before cosmetic changes.
