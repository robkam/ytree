## Task

- Title: Split User Label and Binding Surfaces (i18n/l10n-Safe Config Layout)
- Acceptance target: `bindings.conf` and `labels.conf` exist as first-class XDG-first config surfaces with packaged defaults, deterministic starter generation, dedicated precedence over legacy `ytnova.conf` sections, independent label/key resolution, shared discovery/bootstrap/edit flows, atomic reload across config/themes/bindings/labels, and matching spec/manpage/contributor guidance.
- Completion objective: finish Task 11.2 across runtime discovery, parser/loading, `--init`, `F10`, compatibility precedence, structured label/key assembly, docs, and focused regression coverage so no canonical user-editable surface still depends on raw rendered footer/menu lines or monolithic `ytnova.conf` ownership.

## Clarification gate

Proposed answers taken from `docs/ROADMAP.md` Task 11.2:
1. Canonical home of label overrides: `~/.config/ytnova/labels.conf` (fallback `~/.ytnova.labels`), sourced from packaged `etc/ytnova.labels`.
2. Canonical home of bindings/key-action customization: `~/.config/ytnova/bindings.conf` (fallback `~/.ytnova.bindings`), sourced from packaged `etc/ytnova.bindings`.
3. Precedence: dedicated split file -> legacy section in `ytnova.conf` -> built-in defaults.
4. Runtime assembly model: visible command entries render from `(action_id, label, key_token, availability_state)`, not pre-rendered footer/menu lines.
5. `--init` contract: bootstrap config, themes, bindings, and labels with one deterministic source/generated pipeline and one shared discovery policy.
6. `F10` contract: resolve/create/edit the active config, themes, bindings, and labels files independently under the same XDG-first/fallback policy.

## Selected work family

- Family: config-surface split plumbing for dedicated bindings/labels files plus shared discovery/bootstrap/edit/reload contracts.
- Why this family now: it is the largest coherent owner boundary that unblocks the rest of Task 11.2; label-runtime assembly depends on first making bindings/labels real config surfaces with shared discovery, precedence, `--init`, and `F10` flows.
- Deferred families:
  - structured label/runtime command assembly across footer/help/menu surfaces, because it has a materially different runtime/UI validation path from config file discovery/parser/bootstrap plumbing.
  - final docs/status reconciliation, because it depends on the runtime/config surfaces that land first.

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 11.2 source of truth | pending | Title, contracts, acceptance criteria, and split-family rationale source. |
| `docs/SPECIFICATION.md` §§7.1 and 7.5 | pending | Current discovery/F10 contract still covers only config/themes. |
| `etc/ytnova.conf` | pending | Legacy canonical file still documents `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, `[FILECMD]`. User already has local modifications; preserve and fold them in. |
| new `etc/ytnova.bindings` | pending | Packaged default binding surface. |
| new `etc/ytnova.labels` | pending | Packaged default label surface. |
| `src/core/default_profile_template.h` + generator path | pending | Existing generated config template likely must drop canonical bindings/labels ownership text. |
| new generated starter/header surfaces for bindings/labels | pending | Needed for deterministic `--init`/starter-file bootstrap unless current architecture can safely reuse plain text assets with equal verification guarantees. |
| `scripts/generate_default_profile_template.py` / new generator(s) / `Makefile` QA targets | pending | Deterministic generation + drift verification for new surfaces. |
| `include/ytnova_defs.h` path constants / `ViewContext` path bookkeeping | pending | Needs shared config-family paths for bindings/labels plus active-path tracking if runtime edits them independently. |
| `src/core/main.c` startup discovery and `--init` | pending | Currently initializes config + themes only. |
| `src/cmd/profile.c` profile parser/runtime snapshot/writeback | pending | Still owns legacy `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, `[FILECMD]` parsing and profile rendering. |
| `src/cmd/theme.c` / shared discovery helpers | intentionally unchanged? | Reuse expected, but verify shared path-resolution extraction instead of duplicating rules again. |
| `src/ui/ui_edit_config.c` F10 command surface, starter creation, reload | pending | Currently exposes `(C)onfig (T)hemes (R)eload` only and reloads config+theme only. |
| structured label/runtime assembly surfaces (`src/ui/display.c`, `src/ui/display_utils.c`, command/help/footer callers) | deferred | Same roadmap item, separate family after dedicated surfaces exist. |
| focused regression tests in `tests/test_archive_exit_ui.py` | pending | Existing F10 config/themes tests are the direct extension point for bindings/labels/edit/reload bootstrap behavior. |
| `tests/test_profile_template_sync.py` + new starter sync tests | pending | Generation/verifier coverage for config plus new bindings/labels starters. |
| docs/manpage surfaces (`etc/ytnova.1.md`, generated `docs/USAGE.md`, contributor guidance) | deferred | Update after runtime/docs contract settles for the new surfaces. |

## Recovery notes

- Removed stale unrelated relay `.agent/handoffs/task-41.current.md` after confirming PR #403 merged on 2026-07-16 and no open PR remained.
