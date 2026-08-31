# Classify and Preserve Justified External Contracts

- Status: In Progress.
- Acceptance target: external CLI/config/template contracts retain only published machine- or user-consumed exactness; security/static guards retain non-observable safeguards with explicit rationale; filesystem/archive tests preserve end-state integrity while replacing only brittle setup.
- Completion objective: reconcile every Task 99.5 test family with stable contracts and focused evidence, then update the roadmap status.

## In-scope inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 99.5 and Task 99.1 disposition authority | pending | Source of truth and closure status. |
| External contracts: `test_cli_version_flags.py`, `test_profile_template_sync.py`, `test_theme_catalog_sync.py`, `test_theme_config_paths.py`, `test_mcp_doctor.py`, `test_pre_push_guard.py`, `test_ci_repair_loop.py`, `test_install_shadow_guard.py` | pending | Preserve published CLI/config/generated/workflow interfaces; remove only incidental prose/whitespace coupling. |
| Security/static contracts: `test_c_unsafe_apis_guard.py`, `test_security_gate_contract.py`, `test_security_tempfiles.py`, `test_security_shell_paths.py`, `test_fuzz_harness_sync_guard.py`, `test_appstate_contract_guard.py` | pending | Identify invariant and why safe runtime proof is unavailable for retained inspection. |
| Filesystem/archive effects: `test_core.py`, `test_destination_prompt.py`, `test_archive_backend.py`, `test_archive_write_parity.py`, `test_fileops_integrity.py` | pending | Preserve end-state integrity; change only brittle setup. |
| Referenced scripts/config/template/guard/runtime paths and focused tests | pending | Determined by family audits before edits. |

## Family split

One coherent testing-contract family shares a Python-test validation path. Parallel audits collect the complete inventory; implementation will batch all adjacent findings that do not require materially different runtime ownership or validation.

## Validation plan

Run `make clean && make`, then the smallest complete focused pytest set for every touched family with `source .venv/bin/activate`; PR full QA CI is deliberately unrun locally because the repository requires it as the merge gate.

## Reconciliation for published contract batch

- `test_cli_version_flags.py`, `test_mcp_doctor.py`, `test_pre_push_guard.py`, `test_install_shadow_guard.py`: addressed; remove incidental diagnostic wording while preserving return codes and effects.
- `test_theme_catalog_sync.py`: addressed; retain stale-header rejection without private rendering spellings.
- `test_profile_template_sync.py`: addressed; retain generated-source equality with explicit static-invariant rationale.
- `test_destination_prompt.py`: addressed; replace fixed read delay with semantic screen transition.
- `test_theme_config_paths.py`, `test_ci_repair_loop.py`: deferred; require a separate runtime/workflow test-design boundary to replace private source/narrative coupling safely.
- Security/static contract family: deferred; distinct security-audit risk class requires complete static-invariant review.
- Remaining filesystem/archive end-state family: intentionally unchanged; audit found no remaining brittle setup.

Validation: `make -j2`; `source .venv/bin/activate && pytest -q tests/test_theme_catalog_sync.py tests/test_cli_version_flags.py tests/test_mcp_doctor.py tests/test_pre_push_guard.py tests/test_install_shadow_guard.py tests/test_profile_template_sync.py tests/test_destination_prompt.py` (27 passed). Deliberately unrun: full QA locally; PR CI supplies the required full gate.
