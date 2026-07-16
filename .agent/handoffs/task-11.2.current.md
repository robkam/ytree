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

## Recovery notes

- Rewrote the stale same-task relay after `docs/ROADMAP.md` changed Task 11.2 from split bindings/labels files to the current single `commands.conf` contract.
- GitHub PR `feat(config): add commands surface bootstrap` stayed draft because CI went red on July 16, 2026: the repo-side failure was the static-analyzer null-dereference report in `src/core/main.c:GetDefaultSurfacePath()`, and the unrelated Fedora checkout failure was a transient GitHub-side archive-download error.
