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
| Compare prompts | `prompt.compare-scope`, `prompt.compare-target`, `prompt.compare-basis`, `prompt.compare-results` | `src/ui/compare_request.c` | `tests/test_compare_actions.py` |
| Output prompts | `prompt.output-format`, `prompt.output-separator`, `prompt.output-destination` | `src/ui/print_controller.c` | `etc/help/f1.en.md`, `src/ui/print_controller.c` |
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
| Directory compare | `J` | `J -> COMPARE SCOPE -> COMPARE TARGET -> COMPARE BASIS -> TAG FILE LIST -> result` | 4 | 4 | Directory/Archive-Dir/Split-Dir: `J`, then `D` | External branch is `J -> COMPARE SCOPE -> EXTERNAL VIEWER -> COMPARE TARGET -> result`; all prompts are visible, but the chain is deep. | Offender: compare prompt chain | `src/ui/compare_request.c`, `src/ui/ctrl_dir.c` |
| File compare | `J` | `J -> COMPARE TARGET -> Enter -> result` | 1 | 1 | File/Archive-File/Showall/Global/F7/Split-File: `J` | Split mode seeds the inactive-panel target; target prompt correctly advertises history, browse, and help aids. | Compliant baseline | `src/ui/compare_request.c`, `src/ui/ctrl_file.c` |
| Attributes / metadata edits | `A` | `A -> ATTRIBUTES chooser -> field-specific prompt -> result` | 2 | 2 | File/Dir/Showall/Global/F7/Split: `A`, then `M/O/G` | Date edits add `DATE FIELD -> DATE input`, raising the chain to 3 layers; the date input is syntax-bearing. | Offender: attribute chooser depth | `src/ui/attr_actions.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` |
| Directory copy / move target family | `C`, `V` | `key -> target/name prompt(s) -> optional destination confirmation -> result` | 2-3 | 2-3 | Directory/Archive-Dir/Split-Dir: `C` or `V` | Multi-input workflow; missing-destination and directory-copy confirmations are legitimate exception candidates but the current chain still needs explicit fast-path review. | Review family: multi-input target prompts | `src/ui/interactions.c`, `src/ui/ctrl_dir.c`, `tests/test_destination_prompt.py` |
| File copy / move target family | `C`, `M`, `Ctrl-K`, `Ctrl-N` | `key -> name prompt -> To Directory prompt -> optional destination confirmation -> result` | 2-3 | 2-3 | File-like surfaces: `C`/`M` | Tagged and single-item variants share the same target syntax; current chain is deeper than the one-layer budget unless justified as a real exception. | Review family: multi-input target prompts | `src/ui/interactions.c`, `src/ui/ctrl_file.c`, `tests/test_destination_prompt.py` |
| Output export family | `O`, `Ctrl-W` | `O -> Format chooser -> [separator prompt] -> Output to chooser -> destination prompt -> result` | 3-4 | 3-4 | File/Dir/Archive/Showall/Global/F7/Split: `O` | Prompt-local help exists, but the common path still crosses multiple chooser layers before the destination prompt. | Offender: output prompt chain | `src/ui/print_controller.c` |
| Execute prompt family | `X`, `Ctrl-X` | `X -> command prompt -> Enter -> result` | 1 | 1 | Directory/File/Showall/Global/F7/Split: `X` | Syntax-bearing command prompt has prompt help and tagged rerun semantics. | Compliant baseline | `src/ui/interactions.c`, `src/ui/input_line.c` |
| Create-archive prompt family | `Z` | `Z -> archive target prompt -> Enter -> result` | 1 | 1 | Directory/File/Showall/Global/F7/Split: `Z` | Tagged-first behavior is explained in-context; prompt is already shallow. | Compliant baseline | `src/ui/interactions.c` |
| Jump prompt family | `/` | `/ -> jump prompt -> Enter -> result` | 1 | 1 | Any command surface with Jump: `/` | Direct list-jump path is shallow and already matches the target contract. | Compliant baseline | `src/ui/key_engine.c`, `src/ui/interactions.c` |
| Volume chooser | `K` | `K -> Select Volume dialog -> Enter/D/Esc -> result` | 1 | 1 | Directory/File/Archive/Showall/Global/Split: `K` | Interaction depth is acceptable; wording still mixes `Delete` in-strip with `Release` in help/reference prose. | Visibility/label defect, not depth defect | `src/ui/volume_menu.c`, `src/ui/display.c` |
| Applications chooser | `F9` | `F9 -> Applications dialog -> Enter/Esc -> result` | 1 | 1 | Any main/overlay surface with F9: `F9` | Depth is acceptable, but the command strip currently labels `Enter` and `Esc` together as `Close` even though `Enter` accepts a row. | Visibility/label defect, not depth defect | `src/ui/application_menu.c` |
| History aid | `Up`, `Ctrl-P` | `prompt -> history dialog -> Enter/Esc -> same prompt` | 1 | 1 same-layer aid | While a history-backed prompt is open: `Up` | Same-layer aid by contract; dialog itself is shallow and help-complete. | Compliant aid | `src/ui/history_dialog.c`, `src/ui/input_line.c` |
| F2 browse aid | `F2`, `Ctrl-F` | `prompt -> F2 picker -> Enter/Esc -> same prompt` | 1 | 1 same-layer aid | Compare target / destination prompts: `F2` | The picker is shallow, but its live strip currently hides `Up/Down/Left/Right/Enter/Esc`, which are all usable there. | Visibility defect on picker surface | `src/ui/f2_picker.c` |
| F7 preview overlay | `F7` | `F7 -> preview overlay` | 0 | 1 overlay | File-like surfaces: `F7` | Overlay entry is direct; preview keeps its own restricted command surface and blocks `F8`/panel switching as documented. | Compliant baseline | `src/ui/view_preview.c`, `tests/test_f7_preview.py` |
| F8 split toggle | `F8` | `F8 -> split layout` | 0 | 0 | Main dir/file/showall/global: `F8` | Entry is immediate; follow-up prompt defaults belong to later commands, not to the toggle itself. | Compliant baseline | `src/ui/split_transition.c` |
| Showall / Global aggregate toggles | `S`, `G` | `key -> aggregate list` | 0 | 0 | Directory/File/Archive-Dir: `S` or `G` | Direct mode switch; no prompt-chain defect on entry. | Compliant baseline | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` |

## Prompt-surface correctness findings

| Surface | Finding | Risk |
| --- | --- | --- |
| `dialog.f2-picker` | The live picker strip shows only `Log` and `cycle < >`, while `Up/Down/Left/Right/Enter/Esc` are the primary usable controls documented in help and tests. | Hidden usable commands on the active surface. |
| `dialog.applications` | The strip renders `Close` for both `Enter` and `Esc`, but `Enter` actually accepts the highlighted row. | Misleading shown command label. |
| `dialog.volume-menu` | The strip uses `Delete` for `D`, while help/reference prose describe the action as release/unload. | Action wording is harsher and less precise than the actual behavior. |
| Attribute date editing | The date prompt is syntax-bearing (`YYYY-MM-DD [HH:MM[:SS]]`) and sits behind two chooser stages. | Deep chain plus syntax prompt deserving dedicated compression/review. |

## Ranked offender families

| Rank | Family | Why it ranks here | Current chain | Proposed compression target | Likely docs/tests |
| --- | --- | --- | --- | --- | --- |
| 1 | Compare chooser and tagged-result flow | Highest routine operator cost; directory compare requires 4 layers on the internal path and 4 on the external branch. | `J -> scope -> target -> basis -> result` or `J -> scope -> external -> target` | Collapse directory compare into one chooser + one target prompt, keeping advanced compare options as in-prompt toggles/defaults. | `src/ui/compare_request.c`, `tests/test_compare_actions.py`, help topics `compare*` |
| 2 | Output export prompt chain | Export is frequent and currently spans 3-4 layers before the final destination value. | `O -> format -> [separator] -> destination type -> destination` | One chooser plus one destination prompt, with format/destination extras folded into prompt-local toggles/defaults where safe. | `src/ui/print_controller.c`, `tests/test_print_feature.py`, help topics `output*` |
| 3 | Attribute editing and date flow | Attribute edits require a chooser before the real prompt, and date edits add another chooser before a syntax-bearing value prompt. | `A -> attributes -> field -> [date field] -> value` | One attribute chooser layer maximum, with date-scope selection folded into the same input surface. | `src/ui/attr_actions.c` |
| 4 | Multi-input target prompts (copy/move variants) | Common commands still traverse multiple target/name/destination surfaces and then add confirmation branches. | `copy/move -> name -> destination -> [confirm]` | Reconcile which steps are true exceptions and which can be combined or defaulted safely. | `src/ui/interactions.c`, `tests/test_destination_prompt.py`, `copy-move-targets` help |
| 5 | Prompt-local aid visibility and chooser wording | Lower depth risk than compare/output, but current active surfaces hide usable keys or name accepted actions imprecisely. | `F2/history/volume/apps` aid and chooser surfaces | Keep the existing shallow layers, but make the live surface advertise the real commands and outcome verbs. | `src/ui/f2_picker.c`, `src/ui/application_menu.c`, `src/ui/volume_menu.c` |

## Remediation batches

| Planned roadmap batch | Included family | Why it stays separate |
| --- | --- | --- |
| Compare flow compression | Rank 1 | Distinct compare owner module and focused validation path. |
| Attribute flow compression | Rank 3 | Distinct owner/risk surface; date syntax rules need separate prompt treatment. |
| Prompt-aid visibility and chooser labels | Rank 5 | Mostly menu/picker/help-surface work with lighter runtime risk than compare/output. |
| Multi-input target prompt review | Rank 4 | Needs exception-rule decisions across copy/move/archive-style target prompts. |
| Output prompt compression | Rank 2 | Shared print/export owner and its own prompt chain. |
