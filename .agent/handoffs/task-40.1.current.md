## Task

- Title: Footer Auto-Fit Line Layout (No Hardcoded Per-Line Bindings)
- Acceptance target: Task 40.1 plus Task 40's revised footer acceptance contract — footer command layout must be generated from structured entries instead of hardcoded per-line binding strings; every standard footer context (directory, file, archive, and equivalent context-switched variants) must order visible entries by key class (numeric, alphabetic, symbolic) and rendered keybinding token order rather than label text, including natural function-key sequencing (`F9` before `F10`) and `Esc` after the function-key run; the top two rows must wrap by whole full-label entries, choose wrap points that try to balance used width across both rows instead of greedily saturating row 1, and only then truncate the last visible entry with an ellipsis if needed; the bottom function-key band must follow the same ordering and final-entry ellipsis rule on one row; key tokens must stay independent from labels so keymap and localization changes do not require footer-line rewrites; layout behavior must be documented; and the footer architecture must stay compatible with Task 11.2 command label/key-token separation, stable action-ID resolution, and future gettext work.
- Completion objective: finish Task 40.1 across the structured footer-entry model, deterministic class-sorted key-token footer ordering, whole-entry wrapping with last-entry ellipsis truncation, explicit signpost-vs-command-strip render split, runtime label/key-token resolution through the Task 11.2 commands architecture and current binding state, focused regression coverage, and source-of-truth doc updates so no standard runtime footer path still depends on hardcoded per-line bindings.

## Selected work family

- Family: structured standard-footer entry packing and signpost-aware rendering, including runtime action/key/label resolution, key-token ordering, whole-entry wrapping, and last-entry ellipsis truncation plus the directly coupled footer/help regressions/docs.
- Why this family now: the revised §6.1 contract and the current red PR both converge on the same owner boundary across footer runtime assembly, archive/footer expectations, and source-contract tests proving the footer is genuinely structured rather than another handcrafted narrow variant.
Deferred families:
  - Task 40.2 prompt-/dialog-takeover footer-area reflow, because the revised roadmap now separates the shared standard-footer layout from prompt-owned footer-area surfaces with a materially broader validation path.
  - Task 43 footer/F1 parity wording audit, because parity wording/content review is a separate contract after the layout engine exists.

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 40 / Task 40.1 | addressed | Task 40.1 now names the shared standard-footer layout contract across directory/file/archive contexts and moves prompt-owned footer-area takeovers into Task 40.2. |
| `docs/SPECIFICATION.md` §6.1 footer contract and §7.5 `commands.conf` rendering contract | addressed | §6.1 now describes the shared three-row runtime footer across directory/file/archive contexts, key-token ordering, balanced two-row wrapping, natural function-key sorting, `Esc` trailing the bottom-band function-key run, and final-entry ellipsis truncation. |
| `src/ui/display.c` dir/file/archive/showall/ll footer assembly, nav-row signposts, and integrated-help reuse | addressed | Footer runtime now sorts by key class plus keybinding token order, chooses top-row wrap points that balance the two command rows, applies the same layout to archive/footer variants, and truncates only the final visible entry with an ellipsis when rows are exhausted. |
| `src/ui/display_utils.c` command-strip measurement/render helpers | addressed | Added a shared full-entry formatter so footer sorting and truncation use the same plain-text entry strings that rendering exposes. |
| `include/ytnova_ui.h` footer/command-strip public structs or helper declarations | addressed | Header now exports the shared entry-text formatter used by the structured footer packer. |
| `src/cmd/commands.c` commands presentation lookup | addressed | `shown`/`label` rows from `commands.conf` now populate runtime presentation overrides keyed by context plus action ID, and the loader exports default-key/token helpers. |
| `src/cmd/profile.c`, `include/ytnova_cmd.h`, and `include/ytnova_defs.h` runtime binding/snapshot compatibility seam | addressed | Added binding-resolution helpers plus snapshot/storage fields so footer label/key presentation follows current bindings and survives atomic reload/restore. |
| `src/ui/help_popup.c` / popup command-strip prefix rendering | intentionally unchanged | Popup-specific reflow stays out of scope for Task 40.1; the current popup rendering remains valid until Task 43 / Task 40.2 broaden that surface. |
| Preview/prompt/dialog footer-adjacent surfaces (`DisplayPreviewHelp`, `input_line.c`, picker/dialog strips, viewer strips, prompt takeover rows in `interactions.c` / `compare_request.c`) | intentionally unchanged | These broader footer-area takeover surfaces now belong to Task 40.2's follow-on reflow family rather than the shared standard-footer layout covered here. |
| Archive footer regressions in `tests/test_archive_exit_ui.py` and `tests/test_archive_ui.py` | addressed | Archive footer coverage now follows the same shared footer layout contract, including key-token ordering and wrapped overflow rows. |
| Focused footer regressions in `tests/test_command_strip_visibility.py`, `tests/test_display_layout.py`, `tests/test_archive_exit_ui.py`, and `tests/test_panel_isolation.py` | addressed | Focused coverage now proves the shared footer contract across regular and archive contexts: key-token ordering (`Write` before `eXecute`, `F9` before `F10`, `Esc` last on the wide bottom band), whole-entry wrapping, shared command-column alignment, and last-entry ellipsis truncation instead of tail omission. |
| Source-contract coverage in `tests/test_theme_ui_contract.py` and `tests/test_appstate_contract_guard.py` | addressed | Source-contract coverage now checks the structured footer helpers actually used by `DisplayDirHelp`/`DisplayFileHelp`. |
| Existing helper utilities such as `tests/helpers_ui.py` | intentionally unchanged | Footer row extraction stayed compatible because Task 40.1 kept the three-row footer shape. |

## Validation

- `source .venv/bin/activate && make`
- `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py tests/test_panel_isolation.py::test_tree_viewport_helper_uses_current_row_and_exact_labels`
- `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py tests/test_display_layout.py::test_backslash_to_dir_in_showall_and_global tests/test_archive_exit_ui.py::test_archive_file_footer_uses_full_labels_and_shows_compare tests/test_archive_exit_ui.py::test_archive_dir_footer_uses_compare_and_dirmode_before_global tests/test_panel_isolation.py::test_tree_viewport_helper_uses_current_row_and_exact_labels`
- `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py tests/test_display_layout.py::test_backslash_to_dir_in_showall_and_global tests/test_appstate_contract_guard.py::test_render_footer_focus_reads_project_from_panel_state tests/test_archive_ui.py::test_archive_dir_footer_pipe_action_visible tests/test_archive_ui.py::test_archive_pipe_return_restores_ui_surfaces tests/test_archive_exit_ui.py::test_archive_file_footer_uses_full_labels_and_shows_compare tests/test_archive_exit_ui.py::test_archive_dir_footer_uses_compare_and_dirmode_before_global tests/test_theme_ui_contract.py::test_task_sixty_touched_surfaces_use_structured_command_strips`
- `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt --suppress=unmatchedSuppression src/ui/display.c src/ui/display_utils.c include/ytnova_ui.h`
- `git diff --check`

## Closure

- All inventoried in-scope Task 40.1 surfaces are now either addressed or intentionally unchanged with a split reason.
- Carry-along maintainer-requested dirty docs (`docs/ROADMAP.md`, `docs/ai/TASK_PROMPT_TEMPLATE.md`) remain included for commit/push without expanding Task 40.1 scope.
