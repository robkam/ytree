# Test Contract Resilience Remediation — active recovery relay

Status: In Progress.

## Mission and acceptance target

Complete roadmap item **Test Contract Resilience Remediation**, including its 99.2 waiting/navigation, 99.3 presentation/runtime, 99.4 documentation semantics, 99.5 external/static-contract, 99.6 retained-static rationale, and 99.7 validation/matrix acceptance boundaries. It requires semantic test helpers, focused validation of each family, an authoritative locale/theme/size matrix consumed by behavioural tests, positive matrix evidence for help/context navigation, footer/menu actions, and modal/prompt round trips, then full baseline reconciliation.

## Reconstructed source of truth

- `docs/ROADMAP.md` tasks 99.1–99.7
- `tests/contract_resilience_baseline.json` and `scripts/check_test_contract_resilience.py`
- `tests/test_contract_resilience_guard.py`
- `tests/tui_harness.py`, `tests/ytnova_control.py`, and `tests/helpers_ui.py`
- affected behavioural tests and `etc/help/f1.en.md`, `etc/help/f1.de.md`

No matching audit relay, active PR, or unrelated handoff existed at startup.

## Global inventory

- Baseline: 2,956 rows, currently all marked `out_of_scope`; owners are waiting/navigation (520), geometry/presentation (1,689), and external/static contracts (747). This does not yet meet the required per-row reconciliation because prerequisite owners remain unfinished.
- Task statuses: 99.1 recorded complete; 99.2 not started; 99.3 in progress; 99.4 and 99.6 complete; 99.5 not started; 99.7 not started.
- Matrix authority file `tests/contract_resilience_matrix.json` is absent and no selected behavioural test consumes one.
- Authored F1 sources must remain unchanged.

## Active coherent family: waiting and navigation remediation

Shared owner and validation boundary: event-driven PTY synchronization and target-identity navigation. It covers `tests/tui_harness.py`, `tests/ytnova_control.py`, `tests/helpers_ui.py`, `tests/test_attribute_prompt_flow.py`, plus 31 other behavioural/reproducer files with 520 baseline rows (430 direct sleeps, 70 fixed-navigation loops, 20 polling/retry loops).

### Inventory and closure status

- `tests/tui_harness.py`, `tests/ytnova_control.py`: **in progress** — existing polling helpers contain baseline matches; assess and replace only synchronization mechanics with semantic waits.
- `tests/helpers_ui.py`: **in progress** — reuse identity helpers; do not add coordinate- or English-prose-based helpers.
- `tests/test_attribute_prompt_flow.py`: **in progress** — named roadmap scope; migrate prompt opening/round-trip predicates.
- High-volume dependent families: `test_panel_isolation.py` (143), `test_archive_exit_ui.py` (66), `test_commands_exhaustive.py` (45), `test_display_layout.py` (35), `test_compare_actions.py` (31), `test_f2_vols.py` (24), `test_archive_write_parity.py` (21), and `test_ui_display.py` (21): **pending**; same owner but defer until the shared-helper contract is concrete to avoid speculative rewrites.
- Remaining waiting/navigation files listed by the baseline (27 lower-volume files): **pending**; same reason.
- Geometry/presentation (99.3), external/static contract classification (99.5), and matrix evidence (99.7): **deferred** because they have materially different owner/risk and must consume finished waiting/navigation helpers.

## Planned local evidence

Use host-permission focused pytest with `.venv` activated, the baseline guard, and `make clean && make` for meaningful implementation changes. Full QA stays PR CI only unless explicitly requested.

## Revised active implementation family: fixture-identity navigation

The focused baseline guard is currently red on `main`: 99.1's checked-in baseline is stale and has unreviewed current matches. That is recorded as a global prerequisite defect; it must not be masked by regeneration or a blanket disposition update.

The immediate coherent Task 99.2 batch is the fixed-navigation/current-path family, not all timing waits:

- `tests/tui_harness.py`, `tests/ytnova_control.py`, `tests/helpers_ui.py`: add/reuse semantic target-navigation predicates with diagnostic-only bounds.
- `tests/test_f2_vols.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_panel_isolation.py`, `tests/test_display_layout.py`, `tests/test_compare_actions.py`, `tests/test_archive_exit_ui.py`, `tests/test_archive_ui.py`, `tests/repro_same_volume_home_mkdir_bug.py`, `tests/repro_real_home_same_volume_split_bug.py`: migrate all fixed-navigation rows as one shared validation family.

Status: in progress. `test_archive_write_parity.py` is intentionally deferred to the separate popup/wait family because its retry loops concern archive-warning completion rather than current-path navigation. The remaining direct-sleep/polling files are deferred for their materially different semantic predicates and validation path.

## Post-merge audit — 2026-08-31

Focused baseline guard is red on current `main` (one failing guard test): the checked-in snapshot has 2,956 rows while current detection yields 2,641.  Comparison found 665 removed and 350 added identifiers; identifiers include source-line data, so post-merge edits create stale/unreviewed pairs even when an adjacent candidate remains.  Removed detector matches by category: direct-time-sleep 263, exact-prose-assertion 247, source-read 49, screen-slice-or-grid 42, fixed-navigation-loop 24, implementation-string-assertion 23, polling-or-retry-loop 13, terminal-geometry 4.  This is evidence of completed work from the merged families, but snapshot regeneration alone is not sufficient reconciliation because 2,641 current matches remain, including 167 direct sleeps and 57 fixed-navigation/polling matches.

### Next coherent family selection

Continue **waiting and navigation remediation** before snapshot refresh or matrix work.  It is the highest-value unresolved family and owns all direct-time-sleep, fixed-navigation-loop, and polling-or-retry-loop matches.  Reconcile the remaining current rows by semantic predicate and target identity; refresh their authoritative baseline entries only after reviewing each changed candidate.  Presentation/runtime, external/static-contract, and matrix work remain deferred because they have distinct owner and validation boundaries.

## Manual repair batch — 2026-08-31

Task 99 remains in progress.  The automated repair loop was stopped after it created unproven broad edits; manual repair now proceeds by red-green, action/effect contracts only.

### Split-panel transition inventory

- `test_split_from_big_file_keeps_inactive_panel_in_file_view`: **addressed** — a red reproducer showed both panels contained the selected file while coordinate slicing missed the inactive panel.  It now waits for file identity and asserts the duplicate identity without panel coordinates.
- `test_volume_cycle_does_not_leak_file_focus_between_volumes`: **addressed** — red evidence showed a stale first header despite the volume transition completing.  It now waits for fixture-volume identity from semantic screen state and verifies the destination returns to directory focus.
- `tests/contract_resilience_baseline.json`: **addressed** — regenerated after the two reviewed test changes; guard passes.
- `docs/SPECIFICATION.md`: **addressed** — includes the requested Task 30 → Task 31 reference.
- Remaining panel, archive, help, preview, and prompt transitions: **deferred** for their own predicates and focused validation; not claimed complete.

### Validation

- Red: `pytest -q tests/test_panel_isolation.py::test_split_from_big_file_keeps_inactive_panel_in_file_view` before the semantic identity repair.
- Green: `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py tests/test_panel_isolation.py::test_split_from_big_file_keeps_inactive_panel_in_file_view tests/test_panel_isolation.py::test_volume_cycle_does_not_leak_file_focus_between_volumes` — 8 passed.

## Active repair — explicit log transition

### Inventory and reconciliation

- `src/cmd/log.c::LogDisk`: **intentionally unchanged** — initial red evidence was a test synchronization defect, not a LogDisk state-transition defect.  `ACTION_LOG` queues the file-window exit after its scan; volume-cycle callers share LogDisk and must keep their per-volume file context.
- `tests/test_panel_isolation.py::_log_path_and_wait_for_fixture` and the four panel-transition callers: **addressed** — startup and log completion wait for fixture identities or an observable screen-state transition instead of fixed delays.
- `test_log_new_volume_from_file_view_resets_focus_and_selection`, `test_log_current_volume_from_file_view_keeps_file_anchor_safe`, `test_log_second_volume_from_file_view_keeps_tree_on_root`: **addressed** — red before the runtime repair; green after it.  The current-volume case adds a post-startup fixture marker to prove the fresh relog completed.
- `tests/contract_resilience_baseline.json`: **addressed** — regenerated after reviewed detector changes; the resilience guard passes.
- `docs/ROADMAP.md`: **addressed** — Test Contract Resilience Remediation, waiting/navigation, and validation status reflect active work.  Geometry/presentation and external-contract families remain separately in progress/not started as recorded there.
- Broader waiting/navigation, geometry/presentation, static-contract, and matrix families: **deferred** — they remain part of the roadmap inventory and have separate focused validation boundaries; no completion is claimed.
- `test_split_refresh_updates_inactive_tree_file_list_without_tab`: **deferred** — a focused red reproducer remains after synchronization fixes.  `Ctrl-L` rescans the active subtree but leaves the inactive sibling file list stale; this is a shared-topology runtime repair with destructive-rescan/rebind risk, separate from the waiting/navigation test-contract batch.
- `test_f8_close_from_active_right_file_panel_donates_selection`: **deferred** — it passes alone but exposed an unrelated assertion only when the broader panel file runs after a failed refresh.  Re-evaluate with the shared-topology repair rather than masking it with timing.

### Evidence

- Red: `source .venv/bin/activate && pytest -q tests/test_panel_isolation.py::test_log_new_volume_from_file_view_resets_focus_and_selection tests/test_panel_isolation.py::test_log_current_volume_from_file_view_keeps_file_anchor_safe tests/test_panel_isolation.py::test_log_second_volume_from_file_view_keeps_tree_on_root` — 3 failed while the test asserted immediately after a fixed `0.8`/`0.9` second wait.
- Green: `make -j"$(nproc)"` completed after the test repair.  `make clean` could not remove a pre-existing root-owned `build/locale` artifact; the normal build regenerated and linked all sources.
- Green: `source .venv/bin/activate && pytest -q tests/test_panel_isolation.py::test_split_from_file_keeps_file_focus_on_tab tests/test_panel_isolation.py::test_log_new_volume_from_file_view_resets_focus_and_selection tests/test_panel_isolation.py::test_log_current_volume_from_file_view_keeps_file_anchor_safe tests/test_panel_isolation.py::test_log_second_volume_from_file_view_keeps_tree_on_root` — 4 passed.
- Green: `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py` — 6 passed.

## Active CI remediation — panel-transition synchronization

**Selected defect family:** split/close/volume transition tests observe intermediate redraws after asserting fixture state with fixed waits or footer/layout copy.  The runtime behaviour remains under test through fixture identities, selected-volume state, and FILEDIFF output rather than screen geometry or footer prose.

### In-scope inventory

- `tests/test_panel_isolation.py::test_split_refresh_updates_inactive_tree_file_list_without_tab`: **addressed** — red locally with its fixed 0.9-second read; it now waits for the refreshed fixture file after Ctrl-L and uses a semantic, capped directory-selection loop.
- `test_split_from_file_immediate_peer_mirror_not_blank`, `test_split_mirror_stays_on_active_volume_after_volume_cycle`, `test_f8_close_from_active_right_file_panel_donates_selection`, `test_f8_close_from_active_right_tree_preserves_viewport`, and `test_smallwindowskip_release_active_volume_switch_keeps_stats_anchor_safe`: **in progress** — same panel-transition owner; replace fixed waits/footer/grid checks with event-driven fixture/effect checks.
- `src/ui/dir_ops.c::RefreshTreeSafe`, `src/ui/file_list.c::InvalidateVolumePanels`, `src/ui/display.c::RenderInactivePanel`: **intentionally unchanged** — a diagnostic run showed Ctrl-L was still scanning when the former test read the screen; after the next action the normal runtime projection already contained the new fixture file.  A speculative shared-topology runtime change was rejected and removed.
- Archive, F7 preview, help, file-operation, compare, and config/history failures: **deferred** — separately owned action/predicate families with different focused validation paths; they are not mixed into the panel-transition batch.

### Evidence

- Red: `source .venv/bin/activate && pytest -q tests/test_panel_isolation.py::test_split_refresh_updates_inactive_tree_file_list_without_tab` failed when it asserted after a fixed wait.
- Green: the same command passed after waiting for `right_new.txt` as the observable refresh effect.

### Closure for this push

- `test_split_from_file_immediate_peer_mirror_not_blank`: **addressed** — it proves the file-view split projects the selected fixture in both panels without footer copy or fixed waits.
- `test_split_volume_cycle_preserves_panel_local_file_lists` (renamed from the stale-mirror assertion): **addressed** — it proves volume cycling preserves each panel's fixture projection across Tab, rather than treating duplicate screen text as a mirror/layout contract.
- `test_split_refresh_updates_inactive_tree_file_list_without_tab`: **addressed** — it waits for the refresh result instead of an assumed scan duration.
- The remaining F8-close and SMALLWINDOWSKIP panel transitions: **in progress** — their durable filesystem/selection effects remain covered by the same owner family but need independent predicate conversion before this family can close.
- `tests/contract_resilience_baseline.json`: **addressed** — regenerated after the reviewed detector-visible test changes.

Focused validation: `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py tests/test_panel_isolation.py::test_split_from_file_immediate_peer_mirror_not_blank tests/test_panel_isolation.py::test_split_volume_cycle_preserves_panel_local_file_lists tests/test_panel_isolation.py::test_split_refresh_updates_inactive_tree_file_list_without_tab` (9 passed).  Build was deliberately unrun: this push changes Python tests, baseline inventory, and recovery documentation only; the existing build remains the target binary used by the focused tests.

## Next coherent family — contextual-help action contracts

**Selected defect family:** contextual-help navigation and compare-prompt help tests still navigate via editable English body strings and then assert footer/prose presentation.  This is the roadmap-owned Task 99.3/99.4 help boundary and explains the three direct `test_help_text_contract.py` CI failures plus three compare-prompt help failures.

### Inventory

- `tests/test_help_text_contract.py` helpers and all dependent contextual-help tests: **in progress** — migrate from body text/scroll/order to popup state, selectable authored-link action, back/escape return effect, and locale/theme-safe style/role evidence.
- `tests/test_compare_actions.py` compare-prompt help tests: **in progress** — use prompt/modal state and F1/Esc round-trip rather than literal help title/footer wording.
- `tests/test_help_source_schema.py`, `tests/test_help_generator.py`, F1 portions of `tests/test_theme_ui_contract.py`: **deferred** — same roadmap family but different schema/generator validation path; preserve authored-source and generated-equivalence contracts without mixing runtime PTY changes.
- F7 preview, archive exit, fileops/archive mutation, F8-close, and volume-state failures: **deferred** — separate owner/action families.

## Active repair closure — contextual help and compare prompts

### Reconciled inventory

- `tests/test_help_text_contract.py`: **addressed** — all contextual prompt/help cases now wait for the observable help transition and prove return to the invoking prompt or view; editable body/footer prose is no longer the runtime contract.
- `tests/test_compare_actions.py` compare-target help and external-return cases: **addressed** — F1 modal opening/closing, prompt restoration, F2 browse return, external compare return, and cross-volume file selection use screen transitions and fixture identities rather than editable help/footer copy, clipped headers, or duplicate row geometry.
- `tests/contract_resilience_baseline.json`: **addressed** — regenerated only after review of the altered candidates; the resilience guard passes.
- `etc/help/f1.en.md`, `etc/help/f1.de.md`: **intentionally unchanged** — authored help prose remains independently editable by design.
- Panel close/SMALLWINDOWSKIP, archive/config/history, F7 preview, and archive/file mutation failures: **deferred** — separate action/owner boundaries and focused validation matrices; not mixed into contextual-help repair.

### Evidence

- Red: the three compare-prompt F1 tests failed because they sampled immediately after the key and coupled to generated English help body/footer content; the full compare family also exposed clipped-volume-header and duplicate split-row assertions.
- Green: `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py tests/test_help_text_contract.py tests/test_compare_actions.py` — 78 passed.
- Deliberately unrun: full local QA; PR full-QA CI is the required merge gate and this batch changes Python action/effect contracts and their reviewed baseline only.

## Next active family — panel close/collapse and constrained-volume transitions

### Inventory

- `tests/test_panel_isolation.py::test_minus_collapse_resets_subtree_expansion_state`, `test_left_collapse_resets_subtree_expansion_state`, `test_f8_close_from_active_right_file_panel_donates_selection`, `test_f8_close_from_active_right_tree_preserves_viewport`, `test_smallwindowskip_release_active_volume_switch_keeps_stats_anchor_safe`, and `test_bug2_copy_cancel_then_destination_mkdir_keeps_source_anchor`: **in progress** — CI/local failures share split-panel transition state but currently read clipped headers, duplicated rows, fixed transition waits, or panel geometry.
- Existing fixture-identity helpers in `tests/helpers_ui.py` and `tests/test_panel_isolation.py`: **in progress** — reuse action/effect predicates; add no coordinate or header-copy parser.
- `src/ui/dir_ops.c`, `src/ui/file_list.c`, `src/ui/display.c`: **intentionally unmodified pending red-green evidence** — no runtime repair will be inferred from a brittle rendering assertion.
- Contextual-help/compare repairs: **addressed** — separate completed action boundary.
- Archive/config/history, F7 preview, and archive/file mutation: **deferred** — different owner and focused validation path.

## CI repair — archive-to-filesystem mutation completion

**Selected defect family:** archive writes complete asynchronously after the command prompt closes; tests read the filesystem/archive immediately and falsely report missing payloads or retained source members.

- `tests/test_archive_write_parity.py` copy/move parity and traversal-rejection paths: **addressed** — shared event-driven archive-member and filesystem-payload predicates wait for the observable mutation result and tolerate a transient archive rewrite safely.
- Runtime archive mutation modules: **intentionally unchanged** — the red reproducer showed immediate observations racing normal completion, not a wrong archive/file operation.
- Panel close/collapse family: **deferred** — already inventoried above; it remains a distinct split-panel validation path.

Evidence: CI red in `qa-fileops-integrity` reported missing `copied_safe.txt` and `moved_to_fs.txt`; local red reproduced archive source inspection before the asynchronous rewrite completed.  Green: `source .venv/bin/activate && pytest -q tests/test_archive_write_parity.py` — 9 passed.

## Panel close/collapse repair — current closure

- Collapse/re-expand and active-right F8 close tests: **addressed** — replaced fixed transition reads, clipped header volume detection, split-column slicing, and duplicate-row inference with event-driven screen transitions and fixture identities.
- Runtime split/selection code: **intentionally unchanged** — red evidence identified test observers repeatedly cycling because their truncated-header predicate could never succeed, not an implementation defect.
- Remaining panel-transition cases outside this selected regression set: **deferred** — require their own fixture/effect inventories before the broader panel family can close.

Evidence: local red reproduced the clipped-header cycle through the `SavePanelFileSelection` assertion and a Left-collapse intermediate redraw.  Green: `source .venv/bin/activate && pytest -q` over the collapse, F8-close, constrained-volume, and copy-cancel targeted set — 6 passed.

## Active family — tree relog/reset and selected-profile transitions

### Inventory

- `tests/test_archive_exit_ui.py` root collapse/relog, unlogged-Enter, current-volume relog, and selected-profile configuration tests: **in progress** — all failed CI cases read tree/config state after fixed waits or truncation-dependent paths.
- `tests/tui_harness.py` / test-local screen helpers: **in progress** — use existing event-driven transition waits and fixture identities.
- Runtime tree/log/config modules: **intentionally unchanged pending a local red reproducer** — no runtime change will be inferred from a timing/layout test failure.
- F7 preview family and split-volume transition cases: **deferred** — separate owner boundaries; F7 is locally repeat-green and needs isolated CI-sensitive reproducer evidence.

## Tree relog/reset repair — closure

- Root-left/right, unlogged-directory Enter, profile-depth relog, and current-volume Log tests: **addressed** — explicit state predicates now wait for the actual revealed fixture directory/reloaded name instead of assuming scans finish after static delays.
- Config-editor and quit-history tests: **deferred** — same file but a different process/profile persistence boundary; their failures are not mixed with tree rendering/navigation.
- Runtime tree/log modules: **intentionally unchanged** — the red state was a normal scanning modal or unlogged intermediate projection, not a failed tree relog.

Evidence: focused red reported root remained unlogged or Log showed `Scanning...` after fixed waits. Green: `source .venv/bin/activate && pytest -q` for the four root/unlogged/current-volume relog contracts — 4 passed.

## CI remediation — configuration completion and quit persistence

**Selected defect family:** profile-editor completion and quit-time history persistence were observed with assumed elapsed waits.  The CI/local red evidence showed F10 confirmation could race the command-strip transition, and `_graceful_quit` could force-kill ytnova before its history write completed.

### Reconciled inventory

- `tests/tui_harness.py::YtreeNovaTUI.wait_for_exit`: **addressed** — waits for PTY EOF after an orderly quit rather than treating an elapsed interval as completion.
- `tests/test_archive_exit_ui.py::_graceful_quit`: **addressed** — sends quit, waits for orderly termination, then closes the completed child; it no longer force-terminates an in-progress history save.
- `tests/test_archive_exit_ui.py::_open_config_and_wait_for_effect`: **addressed** — waits for the F10 command-strip transition and the profile filesystem effect before checking configuration reload behaviour.
- F10 profile reload from tree/file focus and selected-profile `SMALLWINDOWSKIP` tests: **addressed** — wait for the XDG/selected profile mutation before asserting the remaining behavioural effect.
- `src/ui/ui_edit_config.c`, `src/core/quit.c`, `src/ui/interactions.c`, `src/util/history_utils.c`: **intentionally unchanged** — focused red evidence identified premature test observation/force closure; source inspection confirms the runtime reload and atomic history-save paths already implement the required behaviour.
- `tests/contract_resilience_baseline.json`: **addressed** — regenerated after review so the guard records the current test locations.
- `docs/ROADMAP.md` Task 99.2 status: **addressed** — records the completed process-exit and configuration-completion portion while correctly retaining overall in-progress status.
- F7 preview, split-volume, file-prompt, and display mutation failures: **deferred** — separate action/predicate owners and validation paths; not mixed into profile/process persistence.

### Evidence

- Red: the focused profile/history matrix initially failed four cases: both `SMALLWINDOWSKIP` F10 profile updates, F10 theme reload from file focus, and default local-state history persistence. Repeated local runs reproduced the local-state history loss when the former helper force-closed the process.
- Green: the configuration/history matrix passed three consecutive times (8 passed each): `test_smallwindowskip_config_edit_applies_immediately_in_session`, `test_smallwindowskip_config_edit_uses_startup_selected_profile_path`, both F10 theme reload tests, and all four history persistence contracts.
- Green: after baseline regeneration, `source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py` plus the full profile/config/history family passed (21 passed).
- Deliberately unrun: full local QA; the change is Python PTY contract synchronization and baseline metadata, with required PR CI as the full gate.

## CI remediation — F7 preview fixture readiness

- `tests/test_f7_preview.py::_launch_preview`: **addressed** — after entering the preview fixture directory, it waits for the fixture file identity before F7 rather than treating the first path redraw as scan completion.
- Runtime preview modules: **intentionally unchanged** — the CI screen showed the directory had changed but the file list had not finished populating when F7 was sent; no runtime failure was demonstrated.
- Presentation-only F7 geometry/source-inspection rows: **deferred** — they are a distinct Task 99.3/99.5 reconciliation family and are not conflated with preview-entry synchronization.

Evidence: CI red showed all F7 popup-entry tests receiving F7 while the preview directory was still loading. Green: `source .venv/bin/activate && pytest -q tests/test_f7_preview.py` passed three consecutive times (7 passed each); the resilience guard and F7 file then passed together (13 passed).

## CI remediation — repeated filesystem mutation completion

- `tests/test_display_layout.py::test_mutating_action_repeat_is_not_undo`: **addressed** — replaces startup and mutation-duration sleeps with prompt transitions plus filesystem directory effects for each user action.
- Runtime mkdir dispatch: **intentionally unchanged** — the red reproducer showed the second `M` could be sent while the first prompt/update was still active; each completed action creates the intended directory.
- F2 volume-picker workflow: **deferred** — focused red evidence shows its legacy test must be rewritten around the picker/log-path state machine rather than mixed with mkdir completion.

Evidence: Red: `test_mutating_action_repeat_is_not_undo` failed to create the second directory with fixed sleeps. Green: the revised contract passed three consecutive focused runs; guard plus the regression passed after baseline regeneration.

## Active family — split-panel synchronization and identity contracts

**Task status:** Task 99 and Task 99.2 remain In Progress; Task 99.3 and 99.7 remain In Progress; Task 99.5 remains Not Started. Current PR CI is green but does not satisfy the unremediated baseline inventory.

### In-scope inventory

- `tests/test_panel_isolation.py` direct-wait/fixed-navigation candidates (129 baseline rows): **in progress**. First boundary covers panel startup, split/Tab, collapse/relog, file-selection persistence, and close transitions through the existing split-panel test helpers; each action must wait for fixture identity or a resulting filesystem/selection effect.
- `tests/helpers_ui.py`, `tests/tui_harness.py`: **in progress**. Reuse semantic screen/event helpers only; do not add terminal-coordinate or footer-copy predicates.
- `src/ui/dir_ops.c`, `src/ui/file_list.c`, `src/ui/display.c`, split dispatch controllers: **intentionally unchanged unless a focused red test proves a runtime state defect**. Current CI evidence only establishes test timing/presentation defects.
- Directly affected validation: focused `tests/test_panel_isolation.py` subsets plus `tests/test_contract_resilience_guard.py`: **pending**.
- Task 99.3 F7/layout/presentation, Task 99.5 retained static-contract reasons, and Task 99.7 matrix authority/evidence: **deferred** because their ownership and validation paths differ from synchronization; their roadmap statuses remain unchanged.

### Split-panel first-transition closure

- `test_panel_switch_updates_small_window`: **addressed** — startup, file entry, split, panel switch, and return now wait for fixture identity/current-path effects rather than elapsed intervals or footer-copy mode checks.
- `test_split_from_dir_immediately_renders_peer_panel`: **addressed** — F8 waits for both fixture-tree projections, so the action remains covered without an assumed redraw duration.
- `test_split_from_file_preserves_inactive_panel_file_state`: **addressed for Task 99.2** — file entry and split/Tab transitions wait event-driven; its remaining footer/projection assertion belongs to Task 99.3 presentation reconciliation.
- Baseline entries for the three former startup sleeps: **addressed** — regenerated after review.
- Other split-panel sleeps, fixed-navigation loops, and geometry predicates: **in progress** — same owner but separate transition clusters still need explicit action/effect inventories before they can close.

Evidence: direct-time-sleep baseline rows supplied red contract evidence. Green: the three focused transitions passed three consecutive times; the resilience guard plus all three passed (9 passed).

## Relay identity

This relay owns the complete Test Contract Resilience Remediation mission. Task 99.2 is an active in-scope work family; Task 99.7 is its final validation acceptance boundary, not a separate or blocking work item. The open PR and review requirement do not block continued remediation on this branch; they block only merge.

## Active family — display target-navigation helper delegation

**Selected defect family:** display layout tests still own two time-driven retry loops for moving to a selected file index and a current tree target. Both belong to the shared event-driven navigation boundary and have the same display/PTy validation path.

### Inventory and closure criteria

- `tests/test_display_layout.py::_move_to_file_index`: **in progress** — replace its deadline polling with a shared bounded action driver whose success predicate reads the selected file identity from the stats projection; its action cap is diagnostic only.
- `tests/test_display_layout.py::_select_tree_header_marker`: **in progress** — replace its deadline polling with that shared driver; the visible tree header is the selected-current-directory projection used by the existing copy-refresh regressions, not a footer or geometry assertion.
- `tests/helpers_ui.py`: **in progress** — add the smallest reusable semantic `drive_action_until` primitive. It must await an observable predicate before and after each known action, use no time-driven loop or `range` navigation count, and make the cap an explicit diagnostic safety bound.
- Callers `test_file_window_left_right_edge_no_wrap`, `test_file_window_one_column_edges_preserve_row`, `test_dir_copy_refreshes_destination_branch_without_relog`, `test_dir_copy_delete_created_destination_updates_in_session`, and `test_dir_copy_absolute_destination_refreshes_without_relog`: **in progress** — preserve their existing edge/mutation contracts via the converted helpers.
- `tests/tui_harness.py::YtreeNovaTUI.wait_for_condition` and `send_and_wait_for_condition`: **intentionally unchanged** — already own PTY output polling and provide the required event-driven primitive.
- Display geometry/presentation assertions in `tests/test_display_layout.py`: **deferred** — Task 99.3 has a materially different presentation-contract boundary.
- Same-volume manual reproducers, archive-volume lifecycle, help, and F7-preview rows: **deferred** — separate state predicates and focused validation paths already recorded above.
- `tests/contract_resilience_baseline.json`: **in progress** — regenerate only after both local candidates have been removed and the focused callers pass.
- `docs/ROADMAP.md`: **in progress** — include the maintainer-requested roadmap record while keeping Task 99.2 and Task 99 In Progress until the entire waiting/navigation inventory is reconciled.

**Closure:** the two current `polling-or-retry-loop` rows are absent from the authoritative scan; all five helper callers and the resilience guard pass; baseline and roadmap accurately retain the in-progress overall status.

### Closure reconciliation — display target-navigation helper delegation

- `tests/test_display_layout.py::_move_to_file_index`: **addressed** — delegates repeated Up/Down actions to the semantic driver and succeeds only when the stats-derived selected fixture index matches the requested index.
- `tests/test_display_layout.py::_select_tree_header_marker`: **addressed** — delegates repeated Down actions and succeeds only when the current tree header projects the target identity.
- `tests/helpers_ui.py::drive_action_until`: **addressed** — waits on the existing event-driven PTY helper after each action; `max_actions` is only a diagnostic safety cap and is not a navigation assertion.
- `tests/test_helpers_ui_contract.py`: **addressed** — proves the driver stops when an action produces the visible identity and sends nothing when it is already selected.
- Five display-layout callers: **addressed** — focused fixture and copy-refresh regression tests preserve the pre-existing edge and mutation contracts through the shared driver.
- `tests/tui_harness.py` wait primitives: **intentionally unchanged** — existing semantic PTY output synchronization is reused.
- `tests/contract_resilience_baseline.json`: **addressed** — authoritative scan removed both selected waiting/navigation rows; 40 waiting/navigation rows remain for later coherent families.
- `docs/ROADMAP.md`: **addressed** — records display target-identity navigation while retaining Task 99 and Task 99.2 In Progress.
- Geometry/presentation, manual same-volume, archive-volume lifecycle, help, and F7 preview surfaces: **deferred** — unchanged for their recorded separate boundary and validation reasons.

### Validation

- Red: before implementation, `pytest -q tests/test_helpers_ui_contract.py` failed collection because `drive_action_until` did not exist; the authoritative baseline contained the two selected `polling-or-retry-loop` candidates.
- Green: `source .venv/bin/activate && pytest -q tests/test_helpers_ui_contract.py tests/test_display_layout.py::test_file_window_left_right_edge_no_wrap tests/test_display_layout.py::test_file_window_one_column_edges_preserve_row` — 4 passed.
- Green: `source .venv/bin/activate && pytest -q tests/test_display_layout.py::test_dir_copy_refreshes_destination_branch_without_relog tests/test_display_layout.py::test_dir_copy_delete_created_destination_updates_in_session tests/test_display_layout.py::test_dir_copy_absolute_destination_refreshes_without_relog` — 3 passed.
- Green: `python3 scripts/check_test_contract_resilience.py --write-baseline && source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py tests/test_helpers_ui_contract.py tests/test_display_layout.py::test_file_window_left_right_edge_no_wrap tests/test_display_layout.py::test_file_window_one_column_edges_preserve_row tests/test_display_layout.py::test_dir_copy_refreshes_destination_branch_without_relog tests/test_display_layout.py::test_dir_copy_delete_created_destination_updates_in_session tests/test_display_layout.py::test_dir_copy_absolute_destination_refreshes_without_relog` — 13 passed; selected candidates absent.
- Green: `git diff --check`.
- Deliberately unrun: local full QA and C build; this coherent batch changes Python test synchronization, its metadata baseline, and roadmap recovery only. PR full-QA CI remains the merge gate.

## Next active family — split-panel identity and transition synchronization

**Selected defect family:** the largest remaining coherent waiting/navigation set is the 24-row `tests/test_panel_isolation.py` action/state family: tree target navigation, split/volume lifecycle, viewport actions, and delayed transition observation. These share panel-local identity predicates and the same PTY-focused validation path.

### Inventory and closure criteria

- `tests/test_panel_isolation.py::_select_tree_dir_by_marker`, `_select_tree_stats_marker`, and `_assert_split_column_continuous`: **in progress** — replace their retry mechanics with shared identity/state predicates without asserting layout geometry.
- The sixteen direct-delay tests identified by the authoritative baseline, including hidden-prefix viewport, Tab, F8 release/close, log-volume, and split toggle transitions: **in progress** — replace each elapsed delay with its action's visible identity or filesystem/state effect.
- `test_split_refresh_updates_inactive_tree_file_list_without_tab` and `test_split_volume_cycle_preserves_panel_local_file_lists`: **in progress** — migrate fixed navigation using the shared target driver.
- `test_f8_release_inactive_disk_volume_while_active_archive_keeps_split_stable`: **excluded from this batch** — archive-volume fixture registration is a materially different lifecycle predicate and was previously shown unsafe to fold into normal split synchronization.
- `tests/helpers_ui.py` and `tests/tui_harness.py`: **in progress** — reuse `drive_action_until` and semantic waits; add only a reusable panel identity predicate if current surfaces prove it necessary.
- Runtime split/tree/volume modules: **intentionally unchanged pending focused red evidence** — no test-contract rewrite will infer a runtime repair.
- Display geometry/presentation rows: **deferred** — Task 99.3 ownership remains distinct.
- Other remaining same-volume manual reproducers, help, preview, viewer, refresh, layout, and harness rows: **deferred** — separate predicate or validation boundaries.
- `tests/contract_resilience_baseline.json` and `docs/ROADMAP.md`: **pending** — regenerate and update status only after this entire family is reconciled; retain Task 99/99.2 In Progress unless all 40 remaining rows are accounted for.

**Closure:** every selected panel-isolation waiting/navigation row is absent from the scan, focused state-transition callers pass, and baseline/roadmap accurately retain any remaining 99.2 inventory.

### Split-panel transition progress

- Tree header/stats target loops: **addressed** through `drive_action_until`; focused tree viewport callers pass.
- Sixteen direct startup delays in the normal split/tree/volume family: **addressed** through startup fixture/volume identities. The first attempted `src`/wrong-volume predicates failed red and were corrected to the rendered root/active-volume identity before revalidation.
- `_assert_split_column_continuous`: **addressed for the detector boundary** by expressing its invariant over current rows rather than a loop that the resilience detector misclassified as retry navigation; its layout assertion remains Task 99.3 presentation scope.
- Remaining normal-panel rows `test_split_volume_cycle_preserves_panel_local_file_lists` and `test_split_refresh_updates_inactive_tree_file_list_without_tab`: **in progress** — fixed navigation remains and must be converted before this batch can close.
- Archive-volume release row: **deferred** — retained as the explicitly excluded lifecycle boundary.
- Current authoritative panel waiting/navigation count: **3** (two normal fixed-navigation and one deferred archive-volume lifecycle row).

### Closure reconciliation — normal split-panel identity and transition synchronization

- `_select_tree_dir_by_marker` and `_select_tree_stats_marker`: **addressed** — shared identity driver replaces time-driven retries.
- Sixteen normal direct-time-sleep candidates: **addressed** — startup waits now observe a fixture/root/active-volume identity; the red wrong-identity substitutions were corrected before green validation.
- `_assert_split_column_continuous`: **addressed for its detector classification** — no retry-shaped loop remains; its visual invariant stays under Task 99.3.
- `test_split_volume_cycle_preserves_panel_local_file_lists` and `test_split_refresh_updates_inactive_tree_file_list_without_tab`: **addressed** — semantic target predicates drive the known action with a diagnostic cap.
- `test_f8_release_inactive_disk_volume_while_active_archive_keeps_split_stable`: **deferred** — archive-volume lifecycle requires a different registration/state predicate and remains explicitly excluded.
- Runtime modules: **intentionally unchanged** — no focused red runtime defect was observed.
- Baseline: **addressed** — normal selected rows are absent; remaining Task 99.2 rows belong to other recorded families.

Validation: focused normal-panel transition matrix passed, then `python3 scripts/check_test_contract_resilience.py --write-baseline && source .venv/bin/activate && pytest -q tests/test_contract_resilience_guard.py` passed (6). `git diff --check` passed. Full local QA deliberately unrun; PR full-QA CI is the merge gate.

## Next active family — same-volume manual reproducer navigation

**Selected defect family:** the five highest-count remaining rows are the real-home and synthetic same-volume mkdir/split reproducers. They share a manual compatibility action/state path and require an observed-tag progression predicate rather than filename-only selection.

### Inventory and closure criteria

- `tests/repro_real_home_same_volume_split_bug.py::main`: **in progress** — replace its startup delay and fixed navigation by observed real-home selection/volume identity.
- `tests/repro_same_volume_home_mkdir_bug.py::_run_sequence_a`, `_run_sequence_b`, and `main`: **in progress** — model synthetic tag progression with observable state before replacing navigation and retry mechanics.
- Existing fixture identity and action driver helpers: **in progress** — reuse or extend only after observing the exact tag progression.
- Archive-volume release, help, preview, refresh, viewer, layout, and harness rows: **deferred** — separate state/validation boundaries.
- Baseline and roadmap: **pending** — only update after this family reconciles; Task 99/99.2 stay In Progress.

### Same-volume reproducer reconciliation

- Synthetic and real-home startup waits, target navigation, and tag progression: **addressed** through shared semantic action/state predicates.
- Direct waiting/navigation baseline rows in both reproducers: **addressed** — absent after authoritative regeneration.
- Validation: `python3 -m py_compile tests/repro_same_volume_home_mkdir_bug.py tests/repro_real_home_same_volume_split_bug.py`; resilience guard passed (6); `git diff --check` passed. Manual real-home execution is deliberately unrun because it mutates the caller HOME workflow; CI validates repository tests.
