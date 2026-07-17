## Task

- Title: Split Command Customization into `commands.conf` (i18n/l10n-Safe Layout)
- Acceptance target: `commands.conf` exists as a first-class XDG-first command-customization surface with packaged defaults and deterministic starter generation; `ytnova.conf` is no longer the canonical editable home for labels, shown key tokens, key/custom-command mappings, or rendered footer/menu text; structured command resolution stays keyed by stable action identity and independent from bound key tokens; discovery/bootstrap/edit/reload behavior is consistent across config, themes, and commands; missing user `commands.conf` falls back to built-in defaults without file creation; `--init` and `F10` can create/edit all three surfaces independently; and legacy `ytnova.conf` command sections still load only as compatibility inputs that lose deterministically to `commands.conf`.
- Completion objective: finish Task 11.2 across packaged/default command-surface assets, deterministic source/generated starter artifacts, shared config-family discovery/bootstrap/edit/reload policy, explicit `commands.conf` loading and validation with legacy precedence, structured command-data plumbing needed by runtime consumers, focused regression coverage, and matching canonical docs so no command-customization surface still depends on monolithic `ytnova.conf` ownership or raw rendered footer/menu-line storage.

## Clarification gate

Task 11.2 now locks down its own concrete decisions in `docs/ROADMAP.md` lines 250-256, so there is no remaining maintainer ambiguity to pause on:
1. Canonical home: `commands.conf` is the editable home of command customization.
2. Precedence: `commands.conf` -> legacy `ytnova.conf` section -> built-in default.
3. Runtime assembly model: visible command entries resolve from `(action_id, label, key_token, availability_state)`, not whole rendered line overrides.
4. Starter-file model: canonical columns are `context | binding | shown | label | action | command`, with tightly constrained alias usage.
5. `--init` contract: bootstrap config, themes, and commands through one shared discovery policy and deterministic source/generated pipeline.
6. `F10` contract: resolve/create/edit the active config, themes, and commands files independently under that same policy.

## Selected work family

- Family: commands-surface discovery/bootstrap/loader plumbing for `commands.conf`, including shared path resolution, starter generation, compatibility precedence, `--init`, `F10`, and atomic reload hooks.
- Why this family now: it is the largest coherent owner boundary spanning config/runtime bootstrap and parser ownership. It unlocks the rest of Task 11.2 by making `commands.conf` a real surface instead of a roadmap-only contract.
- Deferred families:
  - structured footer/help/menu/prompt command assembly from stable action metadata, because it has a materially different runtime/UI validation path from config-surface discovery and parser/bootstrap plumbing.
  - final manpage/generated-usage sync, because it should land against the settled runtime/config behavior rather than a moving partial contract.

## Inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 11.2 | intentionally unchanged | Source of truth already matches the selected family; no tracker text change was needed in this batch. |
| `docs/SPECIFICATION.md` §§7.1 and 7.5 | intentionally unchanged | Spec already described the commands surface and did not need wording changes for this plumbing batch. |
| `etc/ytnova.conf` legacy command sections | addressed | Starter config now points command customization at `commands.conf` instead of embedding canonical `[MENU]`/`[DIRMAP]`/`[FILEMAP]`/`[DIRCMD]`/`[FILECMD]` sections. |
| new `etc/ytnova.commands` | addressed | Added packaged default command-customization starter content. |
| `src/core/default_profile_template.h` | addressed | Regenerated after removing canonical command-section ownership from the packaged config starter. |
| new generated compiled command starter/header | addressed | Added generated compiled starter content for `commands.conf`. |
| `scripts/generate_default_profile_template.py` / new command starter generator / `Makefile` QA targets | addressed | Added a dedicated commands generator and QA/utility targets; existing profile generator stayed intact. |
| `tests/test_profile_template_sync.py` and adjacent generator sync coverage | addressed | Added commands starter drift coverage alongside the existing profile template check. |
| `include/ytnova_defs.h` config-family path bookkeeping | addressed | Added command-surface path constants, active-path storage, and init-hook plumbing. |
| shared config-family path-resolution helper surface | addressed | Added a shared helper module and routed startup/`--init`/F10/theme discovery through it. |
| `src/core/init.c` startup discovery | addressed | Startup now loads config + commands + themes through the shared XDG-first/fallback policy. |
| `src/core/main.c` `--init` flow | addressed | `--init` now bootstraps config, commands, and themes together, reports the commands file outcome, and caches `HOME` before the bootstrap-path guard so static analysis no longer flags a null-dereference path in `GetDefaultSurfacePath()`. |
| `src/cmd/profile.c` | addressed | Added override helpers and snapshot coverage so commands reload remains atomic while legacy profile parsing stays compatibility-only. |
| new explicit `commands.conf` loader/validator module | addressed | Added a dedicated commands loader/validator rather than extending the monolithic profile parser again. |
| `src/cmd/theme.c` / existing theme-path discovery | addressed | Updated theme discovery to use the shared path helper rather than leaving duplicate policy logic behind. |
| `src/ui/ui_edit_config.c` | addressed | F10 now exposes Commands and reloads config + commands + themes atomically. |
| runtime command consumers (`src/ui/display.c`, `src/ui/display_utils.c`, help/footer/menu/prompt command-strip sources, keybinding/runtime assembly call paths) | deferred | Separate runtime/UI family: converting visible command assembly to stable action/label/key-token data has a materially different validation path from config/bootstrap plumbing. |
| focused runtime regression coverage in `tests/test_archive_exit_ui.py` | addressed | Added/updated focused F10 starter-file assertions for config and commands. |
| docs/manpage surfaces (`etc/ytnova.1.md`, generated `docs/USAGE.md`, contributor guidance) | deferred | Final documentation reconciliation stays split because it depends on the later runtime/UI command-assembly family. |


## Active follow-up family

- Family: F10 command-strip mnemonic capitalization consistency across the runtime strip, canonical docs, generated usage text, tests, and AI guidance.
- Why this family now: Task 11.2 made the commands surface first-class under `F10`; this follow-up keeps that surface aligned with the established YtreeNova/XTree convention that only the bound mnemonic letter is capitalized.
- Deferred families:
  - broader footer/help/menu command-strip assembly outside the `F10` surface, because those runtime consumers have a materially different validation path.

## Follow-up inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `src/ui/ui_edit_config.c` F10 strip labels | addressed | Lowercased non-leading-mnemonic labels so runtime renders `co(M)mands` and keeps the same inline-only mnemonic convention for the rest of the strip. |
| `docs/SPECIFICATION.md` mnemonic rule + F10 entry order | addressed | Clarifies that only the mnemonic letter is capitalized when it appears later in the word and lists the F10 entries as lowercase labels. |
| `etc/ytnova.1.md` / generated `docs/USAGE.md` | addressed | Synced the user-facing F10 strip text and reload wording with the commands surface. |
| `docs/ROADMAP.md` Task 11.2 direction text | addressed | Synced the locked F10 strip string and expert-path wording with the shipped surface. |
| `tests/test_theme_ui_contract.py` | addressed | Asserts the rendered F10 strip and lowercase label model. |
| `tests/test_color_config.py` | addressed | Asserts the synced manpage/usage F10 strip text. |
| `.ai/shared.md` | addressed | Added a concise durable reminder so future AI edits preserve the mnemonic-only capitalization convention. |

## Follow-up closure

- `src/ui/ui_edit_config.c`: addressed.
- `docs/SPECIFICATION.md`: addressed.
- `etc/ytnova.1.md`: addressed.
- `docs/USAGE.md`: addressed.
- `docs/ROADMAP.md`: addressed.
- `tests/test_theme_ui_contract.py`: addressed.
- `tests/test_color_config.py`: addressed.
- `.ai/shared.md`: addressed.
- Broader footer/help/menu runtime consumers outside `F10`: intentionally unchanged for this PR because they belong to the separate deferred command-assembly family already tracked above.

## Active format follow-up family

- Family: sectioned `commands.conf` contract with per-context `[DIR]` / `[FILE]` blocks instead of repeating a `context` column on every starter row.
- Why this family now: the current starter format is readable but awkward; converting Task 11.2 itself now avoids locking a needlessly repetitive canonical file shape into docs, starter artifacts, and parser behavior.
- Deferred families:
  - broader footer/help/menu command-strip assembly outside the `F10` surface, because those runtime consumers still have a materially different validation path.

## Format follow-up inventory

| Surface | Status | Notes |
| --- | --- | --- |
| `docs/ROADMAP.md` Task 11.2 mechanism/decisions/acceptance text | addressed | Canonical format contract now uses sectioned per-context blocks with five-column rows. |
| `docs/SPECIFICATION.md` §7.5 `commands.conf` contract | addressed | Defines `[DIR]` / `[FILE]` sections and five-column rows, and drops the old repeated-context shape from the canonical contract. |
| `etc/ytnova.commands` packaged starter | addressed | The starter now uses `[DIR]` / `[FILE]` blocks with five-column rows and no duplicated context cells. |
| `src/core/default_commands_catalog.h` generated starter header | addressed | Regenerated from the new sectioned starter source. |
| `src/cmd/commands.c` parser/validator | addressed | Loads sectioned five-column `[DIR]` / `[FILE]` blocks only for commands.conf. |
| `tests/test_profile_template_sync.py` | intentionally unchanged | Header sync coverage should stay valid automatically once the starter source changes. |
| `tests/test_cli_version_flags.py` | addressed | `--init` assertions now expect section headers and five-column rows, and no stale commands.conf note in the profile starter. |
| `tests/test_archive_exit_ui.py` | addressed | Missing-commands F10 assertions now expect section headers and five-column rows; stale profile-note assertions were removed. |
| active PR / durable handoff text | pending | The handoff now records the broadened command-format scope; the PR summary/title still need to be updated before push. |

## Format follow-up closure

- `docs/ROADMAP.md`: addressed.
- `docs/SPECIFICATION.md`: addressed.
- `etc/ytnova.commands`: addressed.
- `src/core/default_commands_catalog.h`: addressed.
- `src/cmd/commands.c`: addressed.
- `tests/test_profile_template_sync.py`: intentionally unchanged.
- `tests/test_cli_version_flags.py`: addressed.
- `tests/test_archive_exit_ui.py`: addressed.
- active PR summary/title: pending until the broadened commit/PR wording is updated.

## Startup compatibility follow-up

- Defect family: existing six-column user `commands.conf` files from the earlier Task 11.2 starter caused `LoadCommands failed*ABORT` on startup after the canonical sectioned five-column switch.
- Reproducer path: launch `build/ytnova` with `/home/rob/.config/ytnova/commands.conf` in the previous six-column format; startup aborted before the UI painted.
- Selected fix: keep `[DIR]` / `[FILE]` five-column blocks as the canonical starter and docs, but accept existing six-column user rows during load so upgrades do not abort.
- Surfaces: `src/cmd/commands.c` loader, `tests/test_archive_exit_ui.py` startup regression coverage.


## FileInfo footer wording follow-up

- Defect family: footer/help/docs/tests advertise the FileInfo band as `1..0` even though only `1..9` are meaningful user-visible view controls and `0` is a silent no-op.
- Why this family now: the current wording overstates the active band and conflicts with user expectations at runtime.
- Inventory:
  - `src/ui/display.c` FileInfo footer/help command-strip tokens — addressed.
  - `etc/ytnova.conf` starter comments for the numeric view keys — addressed.
  - `etc/ytnova.1.md` and generated `docs/USAGE.md` numeric band wording — addressed.
  - `docs/SPECIFICATION.md` numeric FileInfo band contract text — addressed.
  - `docs/ROADMAP.md` references to `1..0` / `0` unused in the current UX direction — addressed.
  - `docs/BUGS.md` Task 44 cross-reference wording — addressed.
  - focused footer/help regression tests: `tests/test_stats_panel.py`, `tests/test_command_strip_visibility.py`, `tests/test_display_layout.py`, `tests/test_archive_exit_ui.py`, `tests/test_panel_isolation.py`, `tests/test_compare_actions.py`, and adjacent sync checks in `tests/test_profile_template_sync.py` / `tests/test_color_config.py` — addressed.

## FileInfo footer wording closure

- Advertised band wording now uses `1..9` consistently across runtime footer/help text, starter comments, spec/roadmap/manpage docs, generated usage, and exact-string regressions.
- Hidden `0` behavior is intentionally unchanged: runtime still accepts it as a silent no-op, but user-facing footer/help text no longer advertises it as part of the active FileInfo band.

## Recovery notes

- Rewrote the stale same-task relay after `docs/ROADMAP.md` changed Task 11.2 from split bindings/labels files to the current single `commands.conf` contract.
- GitHub PR `feat(config): add commands surface bootstrap` stayed draft because CI went red on July 16, 2026: the repo-side failure was the static-analyzer null-dereference report in `src/core/main.c:GetDefaultSurfacePath()`, and the unrelated Fedora checkout failure was a transient GitHub-side archive-download error.
