## Task

- Title: Implement Integrated Help System
- Acceptance target: a pop-up, scrollable `F1` help system that stays contextual to the active runtime surface rather than showing one generic dump, covering directory/tree view, file view, archive view, Showall/Global view, split/preview layout, and prompt/dialog state.
- Completion objective: finish the integrated-help runtime/docs/test batch so `F1` resolves the current surface correctly, reuses the portable low-noise help contract from Task 42, and reconciles the Task 41 roadmap/spec/runtime/test surfaces without unaccounted gaps.

## Selected work family

- Family: contextual `F1` popup/help dispatch across main footer contexts plus active dialog/prompt help surfaces
- Why this family now: Task 42 landed the portable footer/prompt wording contract, so the next coherent Task 41 batch is wiring the actual integrated help surface to the existing command-strip and prompt-help sources instead of leaving `F1` absent outside the compare/prompt subflows.
- PR: https://github.com/robkam/ytreenova/pull/403
- Deferred families: none yet.

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 41 entry | addressed | Task 41 and the already-landed Task 42 wording batch are both marked completed. |
| `docs/SPECIFICATION.md` §§6.4-6.5 help/modal contracts | addressed | Coverage rule and modal audit now describe picker/dialog/help coverage through the shared popup renderer. |
| `etc/ytnova.1.md` / generated `docs/USAGE.md` F1/help docs | addressed | Global `F1` docs now describe contextual runtime, picker, and prompt help coverage. |
| `include/ytnova_defs.h` action enum | addressed | Added `ACTION_HELP` for main-surface `F1` dispatch. |
| `src/ui/key_engine.c` key decode | addressed | `KEY_F(1)` now resolves to the new help action. |
| `src/core/main.c` action-name registry | addressed | Action registry includes `ACTION_HELP`. |
| `src/core/appstate_actions.c` action/dispatch coverage registry | addressed | Appstate transition/coverage metadata includes `ACTION_HELP` as a keybinding action. |
| `registry/appstate/appstate_action_coverage.json` action coverage source-of-truth | addressed | Action coverage registry now documents `ACTION_HELP` in enum/runtime order so the appstate contract guard matches the new main-surface keybinding action. |
| `include/ytnova_ui.h` public help-popup/help-surface declarations | addressed | Shared popup row types plus integrated-help APIs are declared here. |
| shared popup runtime module (new helper if needed) | addressed | Added `src/ui/help_popup.c` for scrollable help rendering with command-strip/text rows. |
| `src/ui/display.c` directory/file/preview/history help data and integrated help surface | addressed | Main `F1` help now resolves directory/file/showall/archive/preview/split/history contexts from the canonical footer data. |
| `src/ui/ctrl_dir.c` main tree/directory dispatch | addressed | Directory controller opens contextual integrated help. |
| `src/ui/ctrl_file.c` preview-mode filtering | addressed | Preview mode now allows `ACTION_HELP`. |
| `src/ui/ctrl_file_ops.c` file-surface misc dispatch | addressed | File controller opens contextual integrated help. |
| `src/ui/history_dialog.c` history dialog F1/help restoration path | addressed | History picker opens `F1` help and restores its dialog surface without changing the existing low-noise picker strip. |
| `src/ui/volume_menu.c` volume dialog F1/help restoration path | addressed | Volume picker opens `F1` help while preserving the existing explicit narrow-strip action labels. |
| `src/ui/application_menu.c` applications dialog F1/help restoration path | addressed | Applications picker opens `F1` help while preserving the existing explicit narrow-strip action labels. |
| `src/ui/input_line.c` prompt help callback seam | intentionally unchanged | Existing prompt help callback path already satisfied Task 41 once the shared popup renderer was reusable. |
| `src/ui/compare_request.c` compare prompt help popup path | addressed | Compare help now reuses the shared popup renderer while preserving dismiss-on-any-key behavior. |
| `src/ui/interactions.c` execute/search/archive prompt help popup path | addressed | Prompt help now reuses the shared popup renderer. |
| `tests/test_compare_actions.py` | addressed | Focused compare help regression suite rerun green. |
| `tests/test_modal_color_taxonomy.py` | addressed | Source-level palette coverage now checks the shared help popup and its callers. |
| focused integrated-help regression tests (new or existing help test file) | addressed | `tests/test_help_text_contract.py` now covers main surfaces, picker dialogs, archive titles, and source-level context branches. |
| `tests/test_command_strip_visibility.py` | addressed | Narrow picker-strip regression now proves explicit action labels still survive after picker `F1` support landed. |
| `tests/test_theme_ui_contract.py` | addressed | Theme/source contract coverage now tracks the shared help-popup module and preserved picker-strip wording instead of the pre-refactor compare-local helper. |
| appstate contract guard path (`scripts/check_appstate_contract.py` and focused guard tests if implicated) | addressed | Local red-green remediation used the existing guard unchanged; the fix was syncing the missing `ACTION_HELP` registry doc entry instead of changing the guard. |
| Existing footer command-strip rendering (`src/ui/display_utils.c`) | intentionally unchanged | Shared help popups reuse the existing command-strip renderer directly; no renderer changes were required. |

## Validation

- Red: `python3 scripts/check_appstate_contract.py`
- Green: `make clean && make`
- Green: `source .venv/bin/activate && pytest -q tests/test_modal_color_taxonomy.py tests/test_help_text_contract.py tests/test_compare_actions.py`
- Green: `make qa-code-quality`
- Red: `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py::test_picker_menus_show_explicit_close_and_action_keys tests/test_theme_ui_contract.py::test_volume_menu_uses_required_theme_command_strip tests/test_theme_ui_contract.py::test_help_surfaces_use_help_role tests/test_theme_ui_contract.py::test_task_sixty_touched_surfaces_use_structured_command_strips`
- Green: `make`
- Green: `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py::test_picker_menus_show_explicit_close_and_action_keys tests/test_theme_ui_contract.py::test_volume_menu_uses_required_theme_command_strip tests/test_theme_ui_contract.py::test_help_surfaces_use_help_role tests/test_theme_ui_contract.py::test_task_sixty_touched_surfaces_use_structured_command_strips`
- Green: `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py tests/test_theme_ui_contract.py`
- Green: `source .venv/bin/activate && pytest -q tests/test_modal_color_taxonomy.py tests/test_help_text_contract.py tests/test_compare_actions.py tests/test_command_strip_visibility.py tests/test_theme_ui_contract.py`
- Red: `make qa-scan`
- Green: `make qa-scan`
