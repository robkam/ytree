# Primary Action Flow Audit

This checklist is reusable internal guidance for auditing primary-action depth, prompt-surface correctness, and remediation planning.

## Source of truth

- `docs/ROADMAP.md` primary-action-depth roadmap family
- `docs/SPECIFICATION.md` §§ 4.5-4.6 and 6.4
- `docs/ARCHITECTURE.md` command-surface and runtime-help ownership boundaries

## Coverage inventory

| Coverage surface | Runtime/help contexts | Primary owner modules | Evidence anchors |
| --- | --- | --- | --- |
| Filesystem directory/file | `main.dir`, `main.file` | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/display.c` | `etc/help/f1.en.md`, `etc/commands/en.conf` |
| Archive directory/file | `main.archive-dir`, `main.archive-file` | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/display.c` | `etc/help/f1.en.md`, `tests/test_archive_ui.py` |
| Aggregate views | `main.showall`, `main.global` | `src/ui/ctrl_file.c`, `src/ui/display.c`, `src/ui/file_list.c` | `etc/help/f1.en.md` |
| Split surfaces | `overlay.f8-dir`, `overlay.f8-file` | `src/ui/split_transition.c`, `src/ui/display.c`, `src/ui/runtime_help.c` | `etc/help/f1.en.md`, `tests/test_panel_isolation.py` |
| Preview overlay | `overlay.f7-dir`, `overlay.f7-file` | `src/ui/view_preview.c`, `src/ui/display.c` | `etc/help/f1.en.md`, `tests/test_f7_preview.py` |
| Compare prompts | `prompt.compare-target` plus compare explainer topics for scope/basis/results | `src/ui/compare_request.c` | `tests/test_compare_actions.py` |
| Output prompts | `prompt.output-destination`, `prompt.output-separator`, plus the shared `output-format` explainer | `src/ui/print_controller.c` | `etc/help/f1.en.md`, `src/ui/print_controller.c` |
| Filter prompt | `prompt.filter`, `prompt.filter-tagged` | `src/ui/interactions.c`, `src/ui/ctrl_file_ops.c` | `tests/test_filtering.py` |
| History dialog | `dialog.history` | `src/ui/history_dialog.c`, `src/ui/display.c` | `etc/help/f1.en.md` |
| Volume menu | `dialog.volume-menu` | `src/ui/volume_menu.c` | `etc/help/f1.en.md` |
| Applications menu | `dialog.applications` | `src/ui/application_menu.c` | `etc/help/f1.en.md` |
| F2 picker | `dialog.f2-picker` | `src/ui/f2_picker.c` | `etc/help/f1.en.md`, `tests/test_f2_vols.py` |
| Syntax-bearing command prompts | execute, archive-create, copy/move target, date/mode edits | `src/ui/interactions.c`, `src/ui/input_line.c`, `src/ui/attr_actions.c` | `etc/help/f1.en.md`, `tests/test_destination_prompt.py` |

## Measurement rules used in this audit

- Primary-action depth and decision counting follow `docs/SPECIFICATION.md` §4.6.
- `F1`, prompt history, `F2` browse, and completion are treated as same-layer aids when they return to the same pending prompt.
- Manual repro keys name the common-path entry from the active surface; prompt-local aid rows use the owning prompt as the start state.

## Full keybinding -> flow matrix

| Surface family | Key(s) | Current common path | Decisions | Layer count | Manual repro | Surface correctness notes | Classification | Likely owner files |
| --- | --- | --- | ---: | ---: | --- | --- | --- | --- |
| Directory-like filter (`main.dir`, `main.archive-dir`, `overlay.f8-dir`) | `F` | `F -> FILTER prompt -> Enter -> filtered list` | 1 | 1 | Directory/Archive-Dir/Split-Dir: `F` | `Tab` tagged-only toggle is correctly owned by the prompt; `Up` history and prompt `F1` are same-layer aids. | Compliant baseline | `src/ui/interactions.c`, `src/ui/ctrl_dir.c` |
| File-like filter (`main.file`, `main.archive-file`, `main.showall`, `main.global`, `overlay.f7-file`, `overlay.f8-file`) | `F` | `F -> FILTER prompt -> Enter -> filtered list` | 1 | 1 | File/Archive-File/Showall/Global/F7/Split-File: `F` | Same prompt surface owns tagged-only narrowing and prompt-local aids. | Compliant baseline | `src/ui/interactions.c`, `src/ui/ctrl_file_ops.c` |
| Directory compare | `J` | `J -> COMPARE TARGET [scope/basis/tag toggles] -> Enter -> result` | 1 | 1 | Directory/Archive-Dir/Split-Dir: `J` | `F3` cycles scope/external mode on the live target prompt, while `F4`/`F5` keep basis and tagged-result choices explicit without opening extra layers. | Compliant baseline (remediated) | `src/ui/compare_request.c`, `src/ui/ctrl_dir.c` |
| File compare | `J` | `J -> COMPARE TARGET -> Enter -> result` | 1 | 1 | File/Archive-File/Showall/Global/F7/Split-File: `J` | Split mode seeds the inactive-panel target; target prompt correctly advertises history, browse, and help aids. | Compliant baseline | `src/ui/compare_request.c`, `src/ui/ctrl_file.c` |
| Attributes / metadata edits | `A` | `A -> ATTRIBUTES chooser -> field-specific prompt -> result` | 2 | 2 | File/Dir/Showall/Global/F7/Split: `A`, then `M/O/G/D` | Date edits now keep the syntax-bearing value prompt on the first post-chooser layer, with `F3` cycling modified/accessed/both and prompt `F1` documenting the accepted format. | Compliant baseline (remediated) | `src/ui/attr_actions.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` |
| Directory copy / move target family | `C`, `V` | `key -> name prompt -> destination prompt -> result` | 2 | 2 | Directory/Archive-Dir/Split-Dir: `C` or `V` | Directory copy/move is an explicit multi-input exception: first choose the replacement name or pattern, then choose the destination directory. The flow stays split because those are different decisions; merging them would hide meaning instead of removing bureaucracy. Only real safety confirmations may follow, and routine directory copy/move no longer stops on a blanket post-target confirmation. | Compliant exception (documented) | `src/ui/interactions.c`, `src/ui/ctrl_dir.c`, `tests/test_destination_prompt.py` |
| File copy / move target family | `C`, `M`, `C-k`, `C-n`, `Y` | `key -> name prompt -> destination prompt -> result` | 2 | 2 | File-like surfaces: `C`/`M`/`Y` | Single-item, tagged, and pathcopy variants share the same explicit exception: replacement name or wildcard first, destination directory second. The flow stays split because name/pattern and destination carry different meanings, so compression there would be less intuitive. Only overwrite/replace or create-missing-destination confirmations may follow. | Compliant exception (documented) | `src/ui/interactions.c`, `src/ui/ctrl_file.c`, `tests/test_destination_prompt.py` |
| Tagged delete | `C-d`, `D` in VI file view | `key -> batch delete confirmation -> result` | 1 | 1 | File/Archive-File/Showall/Global/F7/Split-File with tagged files: `C-d`; VI file view: `D` | Bulk tagged delete keeps one batch confirmation on the common path. Runtime must not interpose a routine `confirm each file?` chooser; only true follow-up safety prompts such as read-only override may appear when needed. | Compliant baseline (remediated) | `src/ui/file_tags.c`, `src/ui/ctrl_file_ops.c`, `tests/test_vi_keys_mode.py` |
| Output export family | `O`, `C-o`, `C-w` | `O -> Output to chooser -> destination prompt [F3 format, separator if needed] -> result` | 2-3 | 2-3 | File/Dir/Archive/Showall/Global/F7/Split: `O` | File versus hardcopy stays explicit on the route chooser. File output keeps `Raw` / `Framed` / `Page break` visible as an in-prompt toggle, and selecting a framed/page-break format collects its separator before the final destination value. Hardcopy asks only for the printer command. | Compliant baseline (remediated) | `src/ui/print_controller.c` |
| Execute prompt family | `X`, `C-x` | `X -> command prompt -> Enter -> result` | 1 | 1 | Directory/File/Showall/Global/F7/Split: `X` | Syntax-bearing command prompt has prompt help and tagged rerun semantics. | Compliant baseline | `src/ui/interactions.c`, `src/ui/input_line.c` |
| Create-archive prompt family | `Z` | `Z -> archive target prompt -> Enter -> result` | 1 | 1 | Directory/File/Showall/Global/F7/Split: `Z` | Tagged-first behavior is explained in-context; prompt is already shallow. | Compliant baseline | `src/ui/interactions.c` |
| Jump prompt family | `/` | `/ -> jump prompt -> Enter -> result` | 1 | 1 | Any command surface with Jump: `/` | Direct list-jump path is shallow and already matches the target contract. | Compliant baseline | `src/ui/key_engine.c`, `src/ui/interactions.c` |
| Volume chooser | `K` | `K -> Select Volume dialog -> Enter/D/Esc -> result` | 1 | 1 | Directory/File/Archive/Showall/Global/Split: `K` | Interaction depth is acceptable; the chooser now uses the canonical help/release/switch/cancel footer order and keeps generic list navigation in F1 instead of the live strip. | Compliant chooser footer | `src/ui/volume_menu.c`, `src/ui/display.c` |
| Applications chooser | `F9` | `F9 -> Applications dialog -> Enter/Esc -> result` | 1 | 1 | Any main/overlay surface with F9: `F9` | Depth is acceptable; the chooser now labels select/edit/cancel truthfully and supports `Home`/`End`/`PgUp`/`PgDn` alongside arrow navigation. | Compliant chooser surface | `src/ui/application_menu.c` |
| History aid | `Up`, `C-p` | `prompt -> history dialog -> Enter/Esc -> same prompt` | 1 | 1 same-layer aid | While a history-backed prompt is open: `Up` | Same-layer aid by contract; dialog itself is shallow and help-complete. | Compliant aid | `src/ui/history_dialog.c`, `src/ui/input_line.c` |
| F2 browse aid | `F2`, `C-f` | `prompt -> F2 picker -> Enter/Esc -> same prompt` | 1 | 1 same-layer aid | Compare target / destination prompts: `F2` | The picker is shallow and now shows its local help/log/cycle/dotfiles/select/cancel aids while leaving standard tree navigation to the shared chooser convention and F1 help. | Compliant picker aid | `src/ui/f2_picker.c` |
| F7 preview overlay | `F7` | `F7 -> preview overlay` | 0 | 1 overlay | File-like surfaces: `F7` | Overlay entry is direct; preview keeps its own restricted command surface and blocks `F8`/panel switching as documented. | Compliant baseline | `src/ui/view_preview.c`, `tests/test_f7_preview.py` |
| F8 split toggle | `F8` | `F8 -> split layout` | 0 | 0 | Main dir/file/showall/global: `F8` | Entry is immediate; follow-up prompt defaults belong to later commands, not to the toggle itself. | Compliant baseline | `src/ui/split_transition.c` |
| Showall / Global aggregate toggles | `S`, `G` | `key -> aggregate list` | 0 | 0 | Directory/File/Archive-Dir: `S` or `G` | Direct mode switch; no prompt-chain defect on entry. | Compliant baseline | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` |

## Prompt-surface correctness findings

No open findings remain in the prompt-aid visibility / chooser-label family after the footer-order normalization and applications-chooser navigation pass.

## Ranked remaining offender families

No ranked primary-action depth offender families remain open after the output export remediation.

## Remaining remediation batches

No separate remediation batches remain in this audit snapshot.
