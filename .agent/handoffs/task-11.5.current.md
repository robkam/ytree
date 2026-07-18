## Task

- Title: Locale/Layout-Aware Command Presets
- Acceptance target: shipped command presets exist as separate packaged source files keyed by stable preset ID; `commands.conf` remains the single canonical user-editable command surface and can select zero or one preset plus local overrides; no user-editable surface requires multilingual block toggling; packagers can ship a localized default preset choice without forking command-dispatch code and users can override it later through `commands.conf` / `F10`; validation catches collisions and unresolved actions after preset + override resolution; preset files carry concise comment headers; and `docs/SPECIFICATION.md`, `docs/ARCHITECTURE.md`, and F10/help docs describe presets as read-only packaged data layered under one active commands file.
- Completion objective: finish Task 11.5 across packaged preset sources/install/runtime discovery, `commands.conf` preset selection plus override layering, fail-closed preset validation, stable command-surface coverage for directory/file and archive-directory/archive-file variants, focused regression coverage, and the directly coupled docs/manpage/generated-usage surfaces.

## Selected work family

- Family: packaged command-preset layering from source/install/runtime load through `commands.conf` selection/override semantics, including the directly coupled command-surface IDs, starter/catalog docs, and focused loader/runtime regressions.
- Why this family now: Task 11.5 is a single owner-boundary change across command config data, load/validation logic, command-surface identity, and packaging/docs; splitting preset files from loader/runtime/docs would create false completion risk because selection, archive-surface ownership, install paths, and validation all prove the same contract.

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 11.1 / 11.4 / 11.5 source-of-truth text | addressed | Roadmap now defines the packaged-preset architecture, updates adjacent Task 11.1/11.4 wording to the commands-surface model, and marks Locale/Layout-Aware Command Presets complete. |
| `docs/SPECIFICATION.md` command/config/F10 contract | addressed | Spec now documents preset selector semantics, packaged preset install/runtime ownership, archive command-surface IDs, fail-closed resolution, and `F10` ownership of `commands.conf`. |
| `docs/ARCHITECTURE.md` config-family boundaries | addressed | Architecture now records the preset boundary, stable command-surface identity, and editable-vs-read-only ownership rules. |
| `etc/ytnova.1.md` and generated `docs/USAGE.md` | addressed | Manpage/usage now describe `--init` creating `commands.conf`, `F10` preset ownership, and packaged preset install/file paths. |
| `etc/ytnova.commands` and `src/core/default_commands_catalog.h` | addressed | Starter `commands.conf` now advertises `preset = <id>`, canonical archive sections, and stable row grammar; generated catalog remains in sync. |
| New packaged preset sources under `etc/commands/*.conf` | addressed | Shipped `en` and `de` preset files include concise headers, stable preset IDs, and localized row data. |
| `Makefile` install/packaging path definitions | addressed | Build/install now generate/check the preset fallback catalog, expose packaged preset paths, and install/uninstall preset files under shared app-data. |
| `include/ytnova_defs.h` config/path/runtime storage | addressed | Added packaged preset path constants plus archive dir/file runtime storage for presentation overrides and command lists. |
| `include/ytnova_cmd.h` public commands-loader APIs | addressed | Added context-aware runtime binding/reset APIs required for preset layering and archive surfaces. |
| `src/core/config_paths.c` and related path resolution seams | intentionally unchanged | Existing config-surface path resolution already covers `commands.conf`; preset catalogs are shared app-data, not a new user config family, so no new config-path helper was needed. |
| `src/cmd/commands.c` loader/validation core | addressed | Commands loader now parses optional preset selectors, loads packaged presets before overrides, validates archive-aware surfaces, and fails closed on invalid or missing preset IDs. |
| `src/cmd/profile.c` runtime binding/presentation storage and snapshot/restore | addressed | Runtime command state now tracks archive dir/file surfaces, preserves preset-layered bindings through reload rollback, and resolves bindings by context. |
| `src/ui/display.c` footer/help resolved command presentation | addressed | Footer/help rows now resolve from the active runtime command state for directory/file and archive directory/file surfaces. |
| `src/ui/ui_edit_config.c` commands editing/reload path | addressed | `F10` still edits only `commands.conf`, and reload now relies on the same fail-closed layered command loader as startup. |
| Legacy compatibility seam: old six-column / legacy profile command sections | addressed | Legacy six-column `commands.conf` rows still load, and targeted startup coverage confirms preset support did not break the compatibility seam. |
| Focused config/runtime tests (`tests/test_cli_version_flags.py`, `tests/test_profile_template_sync.py`, `tests/test_archive_exit_ui.py`) | addressed | Updated to cover starter output, generated preset catalog sync, `--init`, F10 commands buffers, and legacy compatibility. |
| New/updated preset-layering tests (likely command loader / docs contract / archive surface regressions) | addressed | Added focused runtime proof for packaged preset selection plus local overrides and for invalid preset startup aborts; archive/help surfaces also validate resolved archive command ownership. |
| Docs/source guards (`tests/test_color_config.py`, `tests/test_theme_ui_contract.py`, or adjacent contract tests as needed) | addressed | Doc/source guard suites now cover the new commands/preset contract and continued footer/help wiring expectations. |


## CI remediation

- Family: command-preset dispatch regressions plus archive-footer expectation drift discovered on PR full-QA CI.
- addressed: `src/cmd/usermode.c` preserves vi-mode uppercase `D` / `U` semantics when preset remaps would otherwise collapse them to lowercase file commands.
- addressed: `src/ui/ctrl_file_ops.c` and `src/ui/dir_ops.c` allow `ACTION_CMD_MKFILE` in `USER_MODE` as well as `DISK_MODE`, restoring make-file prompts in file and directory views after preset-layered command routing.
- addressed: `tests/test_archive_exit_ui.py` now expects archive `\ root` / `\ exit` on footer line 2, matching the shipped footer layout.

## Closure

- Validation:
  - `source .venv/bin/activate && make && pytest -q tests/test_help_text_contract.py`
  - `source .venv/bin/activate && pytest -q tests/test_profile_template_sync.py`
  - `source .venv/bin/activate && pytest -q tests/test_cli_version_flags.py`
  - `source .venv/bin/activate && pytest -q tests/test_archive_exit_ui.py::test_missing_commands_f10_unchanged_edit_keeps_starter_file tests/test_archive_exit_ui.py::test_legacy_six_column_commands_file_does_not_abort_startup`
  - `source .venv/bin/activate && pytest -q tests/test_color_config.py`
  - `source .venv/bin/activate && pytest -q tests/test_theme_ui_contract.py`
  - `source .venv/bin/activate && pytest -q tests/test_command_strip_visibility.py`
  - `source .venv/bin/activate && pytest -q tests/test_archive_ui.py`
  - `source .venv/bin/activate && pytest -q tests/test_archive_ui.py::test_archive_footer_keeps_root_on_command_row_and_omits_ctrl_r_rename`
  - `source .venv/bin/activate && pytest -q tests/test_help_text_contract.py::test_archive_f1_help_uses_archive_specific_context_titles`
  - `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src/ui/ui_edit_config.c`
  - `source .venv/bin/activate && make clean && make && git diff --check`
  - `source .venv/bin/activate && make && pytest -q tests/test_commands_exhaustive.py::test_mkfile_command`
- Deferred/blocked: none.
