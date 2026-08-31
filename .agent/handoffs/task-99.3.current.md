# Remediate Geometry, Presentation, and Runtime Interaction Contracts

## Objective
Replace remaining brittle runtime geometry, presentation, help, footer, theme, layout, viewport, and modal assertions with semantic behavioral contracts, consuming the waiting/navigation helpers completed by the preceding remediation.

## Inventory
- Authority: `docs/ROADMAP.md` Task 99.3 and `tests/contract_resilience_baseline.json`.
- Help family: `tests/test_help_text_contract.py`, `tests/test_help_source_schema.py`, `tests/test_help_generator.py`, help portions of `tests/test_theme_ui_contract.py`; runtime surfaces include contextual help popup, generated help assets, locale/theme projections.
- Footer/command family: `tests/test_command_strip_visibility.py`, `tests/test_display_layout.py`, `tests/test_archive_exit_ui.py`, `tests/test_compare_actions.py`, `tests/test_f2_vols.py`, `tests/test_ui_display.py`, relevant `tests/test_panel_isolation.py` sections.
- Layout/modal family: `tests/test_display_layout.py`, `tests/test_stats_panel.py`, `tests/test_ui_layout.py`, `tests/test_f7_preview.py`, `tests/test_modal_message_layout.py`, `tests/test_modal_color_taxonomy.py`, `tests/test_modal_severity_contract.py`, `tests/test_small_window.py`, `tests/test_panels.py`, relevant `tests/test_panel_isolation.py` sections.
- Runtime/source-coupled review: `tests/test_dir_window_dispatch_regressions.py`, `tests/test_file_window_dispatch_regressions.py`, `tests/test_archive_ui.py`, `tests/test_color_config.py`, `tests/test_theme_ui_contract.py`, modal tests, `tests/test_commands_exhaustive.py`, `tests/test_tagged_action_regressions.py`, command-strip/help/security-shell tests.
- Excluded: waiting, polling, and fixed navigation are owned by the completed preceding remediation; documentation semantics and retained static-contract classification are owned by subsequent roadmap items.

## Selected family
Semantic help and modal interaction contracts, including reusable modal/style/action helpers. Footer-packing and geometry projection families are deferred because they have separate runtime assertions and validation paths.

## Closure
- Inventory: active.
- Prior-work handoffs: removed as stale after merged semantic synchronization work.
