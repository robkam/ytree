# Remediate Waiting and Navigation Mechanisms

## Objective
Replace all baseline-matched direct sleeps, time-driven polling/retry loops, fixed navigation counts, and ambiguous popup detection with event-driven semantic state helpers and fixture-identity navigation. Acceptance target: `tests/tui_harness.py`, `tests/ytnova_control.py`, and every matching baseline row, including `tests/test_attribute_prompt_flow.py`.

## Inventory
- Authority: `docs/ROADMAP.md` Task 99.2 and `tests/contract_resilience_baseline.json` (520 rows: 430 direct sleeps, 20 polling/retry loops, 70 fixed-navigation loops). All are presently classified `out_of_scope` with owner `waiting and navigation remediation`; this work reconciles them to `remediated` or a justified retained contract.
- Shared synchronization/runtime helpers and call paths: `tests/tui_harness.py` (`YtreeNovaTUI`), `tests/ytnova_control.py` (`YtreeNovaController`), `tests/helpers_files.py`, `tests/ytnova_keys.py`.
- Directly in scope test modules: `tests/debug_screen.py`, `tests/reproduce_sort_bug.py`, `tests/repro_real_home_same_volume_split_bug.py`, `tests/repro_same_volume_home_mkdir_bug.py`, `tests/test_archive_exit_ui.py`, `tests/test_archive_ui.py`, `tests/test_archive_write_parity.py`, `tests/test_commands_exhaustive.py`, `tests/test_compare_actions.py`, `tests/test_core.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_display_layout.py`, `tests/test_exit_empty_dir.py`, `tests/test_f2_vols.py`, `tests/test_f7_preview.py`, `tests/test_file_window_dispatch_regressions.py`, `tests/test_fileops_integrity.py`, `tests/test_filtering.py`, `tests/test_ghost_bugs.py`, `tests/test_help_text_contract.py`, `tests/test_panel_isolation.py`, `tests/test_panels.py`, `tests/test_refresh_race.py`, `tests/test_security_shell_paths.py`, `tests/test_small_window.py`, `tests/test_state_collision.py`, `tests/test_stats_panel.py`, `tests/test_tagged_action_regressions.py`, `tests/test_ui_display.py`, `tests/test_ui_layout.py`, `tests/test_vi_keys_mode.py`, `tests/test_viewer_return_ui.py`, `tests/test_attribute_prompt_flow.py`.
- Compatibility seams: legacy direct `pexpect` waits/screen dumps; existing helpers must retain their public behaviour while gaining semantic timeout diagnostics.
- Excluded boundary: geometry, presentation, F1 prose and source-inspection remediation belongs to Tasks 99.3–99.6; only their waiting/navigation setup is in scope.

## Coherent family selected
Shared event-driven PTY synchronization and semantic fixture-navigation primitives, then all dependent waiting/navigation baseline rows. This shares one owner boundary and validation path; affected files will be migrated in coherent test-area batches within the same roadmap mission.

## Closure
- Shared helpers: pending.
- Dependent baseline rows: pending.
- Tracker status: In Progress (working-tree tracker change retained from mission setup).
- Deferred families: geometry/presentation and help/source contracts are excluded by their separate roadmap validation boundaries.

## Progress reconciliation
- Addressed: `tests/tui_harness.py`, `tests/ytnova_control.py`, `tests/test_attribute_prompt_flow.py`; semantic PTY reads and condition waits no longer use direct sleeps, with timeout diagnostics; attribute prompt detection no longer depends on bottom-row slices.
- Addressed direct-wait migrations: `tests/test_archive_exit_ui.py`, `tests/test_archive_write_parity.py`, `tests/test_compare_actions.py`, `tests/test_f2_vols.py`, `tests/test_ghost_bugs.py`, `tests/test_panels.py`, `tests/test_small_window.py`, `tests/test_ui_display.py`, `tests/test_file_window_dispatch_regressions.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_core.py`, `tests/test_fileops_integrity.py`, `tests/test_filtering.py`, `tests/test_commands_exhaustive.py`.
- Addressed fixture/current-path navigation in `tests/test_f2_vols.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_compare_actions.py`, `tests/test_archive_exit_ui.py`, `tests/test_help_text_contract.py`, `tests/repro_same_volume_home_mkdir_bug.py`, and `tests/repro_real_home_same_volume_split_bug.py`.
- Pending: all remaining baseline rows in `tests/debug_screen.py`, `tests/helpers_files.py`, `tests/reproduce_sort_bug.py`, `tests/test_display_layout.py`, `tests/test_exit_empty_dir.py`, `tests/test_panel_isolation.py`, `tests/test_refresh_race.py`, `tests/test_security_shell_paths.py`, `tests/test_state_collision.py`, `tests/test_tagged_action_regressions.py`, `tests/test_ui_layout.py`, `tests/test_vi_keys_mode.py`, and `tests/test_viewer_return_ui.py`; remaining fixed-navigation loops in `tests/test_display_layout.py` and `tests/test_panel_isolation.py`.
- Blocked design decision: `tests/helpers_files.py::wait_for_file` has no PTY event source. Replace its clock polling only with a filesystem event watcher or a caller-provided process/PTY predicate; do not substitute busy polling.
- Baseline reconciliation: pending. The authoritative baseline must be regenerated only after the remaining rows are remediated or retained with a durable reason; no row may retain the current `out_of_scope` ownership once this item closes.

## Evidence
- `make` passed after `make clean` could not remove a root-owned locale artifact; existing compiler warnings only.
- `source .venv/bin/activate && pytest -q tests/test_core.py tests/test_fileops_integrity.py tests/test_attribute_prompt_flow.py` passed (19).
