# Enforce Stable Behavioural Test Contracts

## Acceptance target
CI rejects unreviewed volatile TUI-test patterns or requires a narrowly documented exception. The guard covers static sleeps, fixed terminal geometry/grids, full-screen snapshots, padding/wrapping/footer-order assertions, and editable copy/translations. Exceptions name the test, rule, rationale, owner, and expiry/removal condition.

## Inventory reconciliation
- `docs/ROADMAP.md` Task 91 — addressed: status records the CI guard and accountable exceptions.
- `scripts/check_test_contract_resilience.py` — addressed: baseline schema requires a removal condition and calls the records an exception allowlist.
- `tests/contract_resilience_baseline.json` — addressed: regenerated schema-two allowlist includes rule/path/rationale/owner/removal condition per scanned row.
- `tests/test_contract_resilience_guard.py` — addressed: proves missing accountability fields are rejected and every checked-in row identifies its accountable exception data.
- `Makefile` — addressed: `qa-test-contract-resilience` runs the guard and `qa-code-quality` includes it, so baseline and full QA CI enforce it.
- `.github/workflows/ci.yml` and `.github/workflows/full-qa.yml` — intentionally unchanged: both already invoke `qa-code-quality` or the full pytest suite; the new Make dependency reaches both CI paths.
- `docs/AUDIT.md` — addressed: PR full-QA inventory names the test-contract resilience guard.
- Existing `tests/test_contract_resilience_matrix.py` and `tests/tui_harness.py` — intentionally unchanged: they prove current behavioural resilience/event-driven waiting, not guard-policy enforcement.

## Validation
- Red proof: `python3 scripts/check_test_contract_resilience.py` failed against the prior schema-one allowlist after schema-two accountability requirements were introduced.
- `make` — passed (a clean build could not begin because a pre-existing root-owned locale artifact blocked `make clean`; normal build regenerated the binary successfully).
- `make qa-test-contract-resilience` — passed.
- `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py` — 8 passed.
- `make qa-code-quality` — passed.
- `git diff --check` — passed.
- Deliberately unrun locally: `make qa-all`; PR full-QA CI is the required pre-merge gate for this focused scripts/tests/Makefile change.

## Residual risk
The AST guard is intentionally first-line static detection; semantic review remains required by the roadmap policy. Every detected current match must remain individually reconciled in the checked-in allowlist.
