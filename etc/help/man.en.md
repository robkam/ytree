# YtreeNova manpage and USAGE help source (English)

Edit this file to improve manpage and USAGE reference text. It is the authored source for the manpage and generated `docs/USAGE.md`, kept distinct from contextual `F1` help.

## Topic-block schema

Every topic block in this file follows the same parser-facing contract:

1. The block starts with a level-2 heading in the exact form `## topic:<topic-id>`.
2. The heading is followed immediately by a fenced metadata block labelled `ytnova-help-meta`.
3. The metadata block contains exactly these keys, in this order:
   * `title:` — plain-text topic title.
   * `contexts:` — comma-separated stable runtime context/prompt IDs, or the literal `none` for link-only explainer pages.
4. The block then contains these sections in order:
   * required `### Contextual F1`
   * optional `### Explainer links`
   * required `### Long form`
5. When `### Explainer links` is present, every item uses Markdown link syntax with a `topic:` target, for example `- [Navigation](topic:navigation)`.
6. `### Long form` contains one or more level-4 subsections (`#### ...`). Their order is preserved as-authored for later projection.

Keep cross-references sparse. This source is the fuller reference path; contextual `F1` help is the shorter in-task path.

## topic:intro
```ytnova-help-meta
title: Contents
contexts: none
```
### Contextual F1
This manual is the fuller reference path for ytnova modes, commands, prompts, and support topics.
The in-app `F1` popup provides the shorter contextual version for the active surface.
### Explainer links
- [Navigation](topic:navigation)
- [Tagged](topic:tagged)
- [Shared commands](topic:shared-commands)
- [F2 picker](topic:f2-picker)
- [F10 config](topic:f10)

### Long form
#### Purpose
This file is the fuller reference source for the manpage and generated `docs/USAGE.md`.
The in-app `F1` popup remains the shorter contextual path for the active screen, prompt, or dialog.

#### Contents
* **Navigation**: Reference for popup navigation plus shared list movement.
* **Tagged**: Reference for ytnova's tagged working-set model.
* **Shared commands**: Reference for the function-key family that spans multiple modes.
* **Directory Mode** and **File Mode**: Reference for the main logged filesystem views.
* **Archive-Dir Mode** and **Archive-File Mode**: Reference for archive-backed navigation and command limits.
* **Showall** and **Global**: Reference for single-volume and multi-volume aggregated file lists.
* **F7 Preview** and **F8 Split**: Reference for overlay-only controls and ownership rules.
* **List Jump** and **Copy/Move Targets**: Reference for shared `/` jump behavior plus destination and wildcard rename rules.
* **Filter**, **Compare**, **Execute**, **Archive**, and **Output**: Reference for option-heavy prompt families.
* **Command-line Editing**, **VI Keys**, **F2 picker**, **F10 config**, and **Theming**: Reference for shared operator rules and configuration surfaces.

## topic:navigation
```ytnova-help-meta
title: Navigation
contexts: none
```
### Contextual F1
The help popup uses list-style navigation.
`Up` and `Down` move, `Enter` or `Right` follow, `Left` goes back, and `Esc` or `Q` closes.
### Explainer links
- [Directory mode](topic:dir)
- [File mode](topic:file)
- [F7 preview](topic:f7)
- [F8 split](topic:f8)
- [F2 picker](topic:f2-picker)

### Long form
#### Help popup keys
* **Up/Down**: Move between selectable rows or links.
* **Page Up/Page Down**: Scroll longer help pages.
* **Home/End**: Jump to the top or bottom of the current help page.
* **Enter/Right**: Open the selected help item or linked topic.
* **Left**: Go back one step.
* **Esc/Quit**: Close the popup.

#### Scope boundary
This topic owns help-popup movement only.
Use `List Jump` for runtime `/` name-jump behavior, and use the local mode page for ordinary tree/file selection commands.

## topic:list-jump
```ytnova-help-meta
title: List Jump
contexts: none
```
### Contextual F1
`/` is ytnova's in-list name jump.
It is distinct from help-popup navigation and remains scoped to the current runtime list.
### Explainer links
- [Directory mode](topic:dir)
- [File mode](topic:file)
- [Showall](topic:showall)
- [Global](topic:global)

### Long form
#### Jump model
`/` opens an incremental jump prompt for the current visible list only.
Tree/directory views jump among visible directory names, while file-oriented views jump among the visible file rows for that surface.

#### Acceptance and cancel
* **Type text**: Move immediately to the best current match as you type.
* **Enter**: Keep the current match and stay there.
* **Esc**: Cancel the jump and restore the original selection.
* **Scope changes**: Filtering, Showall/Global projection, archives, and split mode all change which visible list `/` searches, but they do not change the jump keys themselves.

## topic:shared-commands
```ytnova-help-meta
title: Shared Commands
contexts: none
```
### Contextual F1
These function keys keep their high-level meaning across modes.
Surface-specific details still belong to the relevant mode or prompt topic.
### Explainer links
- [F7 preview](topic:f7)
- [F8 split](topic:f8)
- [Applications menu](topic:applications-menu)
- [F10 config](topic:f10)

### Long form
#### Shared function keys
* **F1**: Open contextual help for the active surface.
* **F5**: Refresh the current view.
* **F6**: Change the stats/details presentation for the active view.
* **F7**: Toggle preview for the active file context.
* **F8**: Toggle split-screen mode.
* **F9**: Open the Applications menu.
* **F10**: Open the configuration command surface.
* **Esc**: Back out of the current overlay, prompt, or popup.

## topic:tagged
```ytnova-help-meta
title: Tagged
contexts: none
```
### Contextual F1
Tagged files form a working set for bulk actions, narrowed views, searches, and archive/export flows.
Tag-driven behavior is central to ytnova command workflow.
### Explainer links
- [File mode](topic:file)
- [Showall](topic:showall)
- [Search tagged](topic:search-tagged)
- [Filter help](topic:filter)

### Long form
#### Tagged basics
Tags are a working set. They are not a second clipboard and not a saved search.
You can build a set, act on it, narrow it, then clear or invert it.

#### Common tagged flows
* **Tag** and **Untag**: Add or remove the current row from the working set.
* **Invert Tags**: Flip the tag state inside the current visible scope.
* **Filter**: Press `F`, then `Tab` to switch the current file-list scope between all rows and tagged-only rows without changing tag state.
* **Copy tagged** and **Move tagged**: Send the whole tagged set to one destination.
* **View tagged**: Open the tagged files one after another.
* **Search tagged**: Search only the tagged files, then untag non-matches.
* **Archive**: Archive the tagged set first. When nothing is tagged, archive falls back to the current selection.

## topic:command-line-editing
```ytnova-help-meta
title: Command-line Editing
contexts: none
```
### Contextual F1
Most prompts share the same editing keys.
Prompt-specific syntax and scope rules belong to the relevant command topic.
### Explainer links
- [VI keys](topic:vi-keys)
- [F2 picker](topic:f2-picker)

### Long form
#### Editing keys
* **Left/Right**: Move inside the current prompt text.
* **Home/End**: Jump to the start or end of the prompt text.
* **Backspace/Delete**: Delete the character to the left or right of the cursor.
* **Enter**: Accept the current value.
* **Esc**: Cancel without committing the prompt.

#### Shared helpers
* **Up**: Open or cycle prompt history when that prompt keeps history.
* **F2**: Open a browser or picker when the current prompt supports browsing.
* **F1**: Show syntax or scope rules that matter only to the current prompt.

## topic:copy-move-targets
```ytnova-help-meta
title: Copy/Move Targets
contexts: none
```
### Contextual F1
Copy, move, and pathcopy use two explicit prompts.
First choose the replacement name or wildcard rename pattern.
Then choose the destination directory.
The split stays intentional because name/pattern and destination are separate decisions.
Merging them would hide meaning instead of removing friction.
Overwrite conflicts compare size/time so you can judge newer/older and bigger/smaller.
### Explainer links
- [Directory mode](topic:dir)
- [File mode](topic:file)
- [Archive-File mode](topic:archive-file)
- [F8 split](topic:f8)

### Long form
#### Target forms
Use a directory path when you want the original names preserved under another directory.
Use one full replacement name when you want one selected item to land under a new explicit name.
Use a wildcard pattern such as `*.bak` or `copy-*` when you want ytnova to rewrite each selected basename by pattern.

#### Shared rules
Tagged copy/move uses the same target syntax as single-item copy/move.
Pathcopy uses the same two-prompt target flow while preserving the selected file's path relative to the current volume root.
Split mode may seed the inactive-panel directory as the default target, but you can still replace that default before the operation starts.
Archive-backed copy/move keeps the same destination model even when extraction or archive-aware paths are involved.
Only real safety prompts may follow the name and destination prompts, such as overwrite/replace conflicts or creating a missing destination directory.
Overwrite/replace conflicts show source and destination size/time facts when available so you can see whether the destination is newer/older or bigger/smaller before answering.
Directory copy/move starts after the destination is accepted; there is no extra copy-now or move-now confirmation.

## topic:vi-keys
```ytnova-help-meta
title: VI Keys
contexts: none
```
### Contextual F1
When `VI_KEYS=1`, lowercase vi navigation is reserved.
Conflicting commands move to uppercase or another safe key.
### Explainer links
- [Navigation](topic:navigation)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Navigation remap
With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.
`Ctrl-U` and `Ctrl-D` become page up and page down.

#### Command collisions
Commands that would steal those lowercase keys move out of the way.
Examples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist.

## topic:f10
```ytnova-help-meta
title: F10 Config Help
contexts: none
```
### Contextual F1
F10 owns configuration-related actions, including profile editing, command editing, theme editing, and reload.
It is the setup surface rather than an ordinary file-management command.
### Explainer links
- [Theming](topic:theming)
- [Shared commands](topic:shared-commands)

### Long form
#### Config surface
Use `F10` when you want to change persistent behavior instead of doing one one-off file or directory action.
Profile settings, command labels, themes, and reload all live here.

#### Related files
`ytnova.conf` owns profile settings.
`commands.conf` owns user command labels and bindings.
`themes.conf` owns theme selection and theme-role overrides.

## topic:theming
```ytnova-help-meta
title: Theming
contexts: none
```
### Contextual F1
Themes style semantic UI roles and file-type palettes.
Theme edits belong in the config/theme files, not in per-screen hard-coded colors.
### Explainer links
- [F10 config](topic:f10)

### Long form
#### Theme model
Themes set semantic roles such as `footer`, `help`, `help_link`, `selection`, `picker`, and `warning`.
This keeps one theme change consistent across the whole UI.

#### Editing path
Use `F10` to open the theme or config editing path.
Keep high-frequency navigation surfaces readable first: selection, picker, footer, and help.

## topic:dir
```ytnova-help-meta
title: Directory Help
contexts: main.dir
```
### Contextual F1
Directory mode is the logged tree view.
It owns directory navigation, tree expansion, and directory-scoped commands.
### Explainer links
- [Navigation](topic:navigation)
- [Shared commands](topic:shared-commands)

### Long form
#### Directory navigation
* **Enter**: Open the file window. If the selected directory is still unlogged, `Enter` logs or reveals one level first.
* **Collapse**: Collapse the current branch. Press `-` again on a collapsed logged node to release it and mark it unlogged.
* **Left Arrow**: Collapse the current node or move to its parent.
* **Right Arrow**: Expand one level first, then move to the first child.
* **Plus**: Log or reveal one level without moving the cursor.
* **Asterisk**: Recursively expand the selected directory and its subdirectories.

#### Directory commands
* **1..9 view**: Change the active panel's directory and file presentation. `1` resets to the default Name view, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5`/`7`/`8`/`9` affect the file projection, and `6` toggles size units.
* **Attributes**: Open the attributes submenu. Change mode, owner, group, or date.
* **Copy**: Copy the selected directory branch.
* **Delete**: Delete the selected directory.
* **Filter**: Filter the current file-list scope. Use globs such as `*.c`, comma lists such as `*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as `:r`, `>2023-01-01`, or `>1M`.
* **Global**: Show files from every logged volume in one list.
* **Invert Tags**: Flip the tag state inside the current visible scope.
* **Compare**: Compare the current directory, the current logged tree, or an external viewer target.
* **Volume**: Open the volume picker.
* **Log**: Log a new directory or archive file. Logging an already logged path reloads it from the top.
* **Makedir**: Create a new directory.
* **New File**: Create a new empty file.
* **Filter**: The filter prompt also owns the tagged-only scope toggle; press `Tab` there to switch between all files and tagged-only for the current scope.
* **Pipe**: Send the selected directory to a command on standard input.
* **Quit**: Quit ytnova.
* **Rename**: Rename the selected directory.
* **Showall**: Show every file inside the current logged volume.
* **Tag**: Tag the files in the selected directory scope.
* **Untag**: Untag the files in the selected directory scope.
* **MoveDir**: Move the selected directory branch.
* **Output**: Export the current selection to a file or command through the output prompts.
* **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.
* **Archive**: Create an archive from the tagged set first, or from the current selection when nothing is tagged.
* **Jump**: Jump to a matching name in the current list.
* **Dotfiles**: Toggle hidden dot-files and dot-directories.

## topic:file
```ytnova-help-meta
title: File Help
contexts: main.file
```
### Contextual F1
File mode is the main file-list view.
It owns file navigation, file-scoped commands, tagged actions, and export entry points.
### Explainer links
- [Navigation](topic:navigation)
- [Tagged](topic:tagged)
- [Output](topic:output)

### Long form
#### File navigation
* **1..9 view**: Change the active panel's file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.
* **Enter**: Switch between the embedded file window and full-screen file mode.
* **Left Arrow**: Move to the previous visible file column. In one-column layouts it behaves like page up.
* **Right Arrow**: Move to the next visible file column. In one-column layouts it behaves like page down.

#### File commands
* **Attributes**: Open the attributes submenu for the selected file.
* **Copy**: Copy the selected file.
* **Copy tagged**: Copy the tagged set to one destination.
* **Delete**: Delete the selected file.
* **Edit**: Open the selected file in the configured editor.
* **Filter**: Filter the current file-list scope with globs, exclusions, and extended selectors.
* **Hex**: View the selected file in hex mode.
* **Invert Tags**: Flip the tag state inside the current visible scope.
* **Compare**: Compare the selected file against another file.
* **Volume**: Open the volume picker.
* **Log**: Log a new directory or archive file without leaving file mode.
* **Move**: Move the selected file.
* **Move tagged**: Move the tagged set to one destination.
* **New File**: Create a new empty file.
* **Filter**: The filter prompt also owns the tagged-only scope toggle; press `Tab` there to switch between all files and tagged-only for the current scope.
* **Pipe**: Send the selected file to a command on standard input.
* **Quit**: Quit ytnova.
* **Rename**: Rename the selected file.
* **Sort**: Change the current file-list sort order.
* **Tag**: Tag the selected file.
* **Tag all**: Tag every visible file in the current scope.
* **Untag**: Remove the tag from the selected file.
* **Untag all**: Remove every tag in the current scope.
* **View**: View the selected file with the configured pager.
* **View tagged**: View the tagged files one after another.
* **Output**: Export the selected file or tagged set through the output prompts.
* **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely. `Ctrl-X` reruns the command for each tagged file.
* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.
* **Search tagged**: Search only the tagged files, then untag files that do not match.
* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.
* **Jump**: Jump to a matching name in the current list.
* **Dotfiles**: Toggle hidden dot-files in the current scope.

## topic:archive-dir
```ytnova-help-meta
title: Archive Directory Help
contexts: main.archive-dir
```
### Contextual F1
Archive-Dir mode is the tree-style view inside a logged archive.
It mirrors directory work where the archive format permits it.
### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)

### Long form
#### Archive directory navigation
* **Enter**: Switch to Archive File Mode for the selected archive directory.
* **Left Arrow**: Collapse the current archive node or move to its parent.
* **Right Arrow**: Expand one level first, then move to the first child.
* **Root**: `\` jumps to archive root when you are below it.
* **Exit archive**: `\` leaves the archive when you are already at archive root.

#### Archive directory commands
* **1..9 view**: `1..4` choose the base archive directory/file view. `5`, `7`, and `8` still affect the paired file projection, `6` toggles row-size units, and `9` stays a no-op inside archives.
* **Delete**: Delete the selected archive directory entry.
* **Filter**: Filter the current archive-backed file-list scope.
* **Global**: Show archive-backed results together with other logged volumes.
* **Compare**: Compare the current archive directory or logged tree view.
* **Volume**: Open the volume picker.
* **Log**: Log another directory or archive file.
* **Makedir**: Create a directory where the archive format supports it.
* **Pipe**: Send the selected archive path to a command on standard input.
* **Output**: Export the current archive-backed selection through the output prompts.
* **Quit**: Quit ytnova.
* **Rename**: Rename the selected archive directory entry.
* **Showall**: Show every file in the current archive.
* **Tag**: Tag the files in the current virtual directory scope.
* **Untag**: Untag the files in the current virtual directory scope.
* **Jump**: Jump to a matching name in the current list.
* **Dotfiles**: Toggle hidden entries when the archive view exposes them.

## topic:archive-file
```ytnova-help-meta
title: Archive File Help
contexts: main.archive-file
```
### Contextual F1
Archive-File mode is the file-list view for archive-backed content.
Some filesystem commands are unavailable or become archive-aware here.
### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Tagged](topic:tagged)

### Long form
#### Archive file navigation
* **1..9 view**: `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, and `8` toggles File detail. `9` stays a no-op inside archives.
* **Enter**: Switch back to Archive Directory Mode.
* **Jump**: Jump to a matching name in the current list.

#### Archive file commands
* **Copy**: Copy the selected archive entry through archive-aware extract/copy paths.
* **Copy tagged**: Copy the tagged archive entries to one destination.
* **Delete**: Delete the selected archive entry.
* **Filter**: Filter the current archive-backed file-list scope.
* **Hex**: View the selected archive entry in hex mode.
* **Invert Tags**: Flip the tag state inside the current visible scope.
* **Compare**: Compare the selected archive entry against another file.
* **Volume**: Open the volume picker.
* **Log**: Log another directory or archive file.
* **Move**: Move the selected archive entry through archive-aware paths.
* **Move tagged**: Move the tagged archive entries to one destination.
* **Pipe**: Send the selected archive entry to a command on standard input.
* **Output**: Export the selected archive entry through the output prompts.
* **Quit**: Quit ytnova.
* **Rename**: Rename the selected archive entry.
* **Sort**: Change the current file-list sort order.
* **Tag**: Tag the selected archive entry.
* **Untag**: Remove the tag from the selected archive entry.
* **View**: View the selected archive entry.
* **View tagged**: View the tagged archive entries one after another.
* **Pathcopy**: Copy the selected archive entry while keeping its relative path.
* **Search tagged**: Search only the tagged archive entries, then untag non-matches.
* **Execute**: Not available in archive file mode.
* **Dotfiles**: Toggle hidden entries when the archive view exposes them.

## topic:filter
```ytnova-help-meta
title: Filter Help
contexts: prompt.filter,prompt.filter-tagged
```
### Contextual F1
Filters apply glob, exclusion, attribute, date, and size selectors to the current file-list family.
The prompt starts with `*`, which means all files.
Terms can be stacked by separating them with commas.
### Explainer links
- [Tagged](topic:tagged)
- [Showall](topic:showall)
- [Global](topic:global)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Syntax
Current filter terms:
* `*` — show all files
* `*.c` — glob match
* `*.c,*.h` — stack multiple glob terms
* `-*.o` — exclude matches
* `:r` — attribute test
* `:x` — attribute test
* `>2023-01-01` — date test
* `>1M` — size test

You can combine terms in one filter:
* `*.c,-*.tmp`
* `*.c,*.h,>1M`
* `:r,*.sh`
* `*.log,>2024-01-01,-debug*`

Use normal glob-like patterns such as `*.c`, comma-separated unions such as `*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as `:r`, `:x`, `>2023-01-01`, or `>1M`.
If your shell would expand the pattern before ytnova sees it, quote it at the shell prompt.

#### Scope
The filter always applies to the current file-list family.
That may be a normal file list, archive file list, Showall, or Global.
Press `Tab` to switch the filter scope between all files and tagged files.
This is enabled only when tagged files exist in the current scope.
When tagged scope is enabled, the prompt changes to `FILTER [tagged only]:`.

## topic:compare
```ytnova-help-meta
title: Compare Help
contexts: none
```
### Contextual F1
Compare covers diff-style viewing, target selection, scope selection, basis selection, and result handling.
Use the related compare topics for the prompt-by-prompt details.
### Explainer links
- [File mode](topic:file)
- [Directory mode](topic:dir)
- [Navigation](topic:navigation)

### Long form
#### Compare flow
Choose the target first.
Then choose the compare scope when the source is a directory.
Then choose the compare basis when the runtime offers more than one basis.
Finally choose which result class to tag on the source side.

#### Compare rules
* Logged-tree compare uses logged content only. It does not auto-log unopened `+` subdirectories.
* `FILEDIFF` may use `%1` and `%2`. When those placeholders are missing, ytnova appends source and target paths to the helper command.
* External directory/tree compare launches `DIRDIFF` or `TREEDIFF` instead of tagging runtime results.
* There is no separate compare-tagged-files mode.

## topic:compare-target
```ytnova-help-meta
title: Compare Target Help
contexts: prompt.compare-target
```
### Contextual F1
The compare target prompt selects the other file, directory, panel, or external viewer target.
Available choices depend on the active compare mode.
### Explainer links
- [Compare Help](topic:compare)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Target rules
Enter one path.
The compare scope decides whether that one path is treated as a file target, a directory target, or a logged-tree target.

## topic:change-date
```ytnova-help-meta
title: Date Change Help
contexts: prompt.change-date
```
### Contextual F1
The date prompt accepts `YYYY-MM-DD` and optional `HH:MM[:SS]` time input for attribute edits.
`F3` cycles whether the entered value updates the modified time, accessed time, or both.
### Explainer links
- [File mode](topic:file)
- [Directory mode](topic:dir)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Scope choices
Use `modified` to change only the last-modified timestamp.
Use `accessed` to change only the access timestamp.
Use `both` to write the entered value to both timestamps.

#### Format rules
If you omit the time portion, ytnova keeps the current hour, minute, and second.
Tagged date edits use the same prompt and scope cycle.

## topic:compare-scope
```ytnova-help-meta
title: Compare Scope Help
contexts: prompt.compare-scope
```
### Contextual F1
The compare scope prompt chooses single-item, tagged-set, current-directory, or wider list-family comparison scope.
The exact options depend on the active surface.
### Explainer links
- [Compare Help](topic:compare)

### Long form
#### Scope choices
Use `Directory` for one level.
Use `Logged tree` for the currently logged recursive tree.
Use `External viewer` when you want an external diff tool instead of tagged compare results inside ytnova.

## topic:compare-basis
```ytnova-help-meta
title: Compare Basis Help
contexts: prompt.compare-basis
```
### Contextual F1
The compare basis prompt chooses the matching criteria used for the current compare run.
Typical bases include name, size, time, and content-oriented comparisons.
### Explainer links
- [Compare Help](topic:compare)

### Long form
#### Basis choices
Choose the cheapest basis that answers the question you actually have.
Use `Hash` only when metadata is not trustworthy enough.

## topic:compare-results
```ytnova-help-meta
title: Compare Result Help
contexts: prompt.compare-results
```
### Contextual F1
Compare results can be displayed, filtered, and converted into a tagged working set for follow-up commands.
This topic covers the result-handling side of compare.
### Explainer links
- [Compare Help](topic:compare)

### Long form
#### Result tagging
The compare command never rewrites files.
It marks the chosen result class on the active/source side so you can inspect, copy, move, or archive that subset next.

## topic:execute-file
```ytnova-help-meta
title: Execute File Help
contexts: prompt.execute-file
```
### Contextual F1
The file execute prompt runs a shell command against the current file or reruns it once per tagged file.
`{}` expands to one selected file path.
### Explainer links
- [File mode](topic:file)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Placeholder rules
`{}` stands for one selected file path.
When you use the tagged rerun path, the same command is repeated once per tagged file.

## topic:execute-dir
```ytnova-help-meta
title: Execute Directory Help
contexts: prompt.execute-dir
```
### Contextual F1
The directory execute prompt runs a shell command against the current directory path or reruns it once per tagged scope item.
`{}` expands to one selected path.
### Explainer links
- [Directory mode](topic:dir)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Placeholder rules
`{}` stands for the current directory path.
The tagged rerun path still walks tagged files from the active list, not tagged directories from somewhere else.

## topic:search-tagged
```ytnova-help-meta
title: Search Tagged Help
contexts: prompt.search-tagged
```
### Contextual F1
Search tagged runs a text search over the tagged set and removes tags from non-matching files.
It is a narrowing operation on an existing working set.
### Explainer links
- [Tagged](topic:tagged)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Tagged search rules
Start by tagging a working set.
Then search only that set. The result is another, narrower tagged set because files that do not match lose their tags.

## topic:create-archive
```ytnova-help-meta
title: Create Archive Help
contexts: prompt.create-archive
```
### Contextual F1
Create archive builds a new archive from the tagged set first, or from the current selection when nothing is tagged.
Archive format support depends on the chosen suffix.
### Explainer links
- [Tagged](topic:tagged)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Archive creation rules
Directory selections are archived recursively.
Archive creation picks the tagged set first because tagging is the normal way to build a custom archive batch.

## topic:output
```ytnova-help-meta
title: Output Help
contexts: none
```
### Contextual F1
Output exports one or more files to a destination using raw, framed, or page-break formats.
The related output topics cover format, separator, and destination prompts.
### Explainer links
- [File mode](topic:file)
- [Archive file](topic:archive-file)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Output model
`Output` is an export flow.
It can write plain content, framed content, or page-break-separated content.
It can also send that output to a printer command instead of a file path.

#### Output order
Choose file or hardcopy first.
On the file destination prompt, `F3` cycles `Raw`, `Framed`, and `Page break`.
Choose the separator as soon as `F3` selects framed or page-break output, before entering the final file path.
Hardcopy asks only for the printer command.

## topic:output-format
```ytnova-help-meta
title: Output Format Help
contexts: prompt.output-format
```
### Contextual F1
Output format chooses how each exported file is framed in the batch.
Raw, framed, and page-break output serve different downstream readers.
### Explainer links
- [Output Help](topic:output)

### Long form
#### Format choices
Use `Raw` when another tool will parse the output.
Use `Framed` or `Page break` when a human will read the exported batch.

## topic:output-destination
```ytnova-help-meta
title: Output Destination Help
contexts: prompt.output-destination
```
### Contextual F1
Output destination chooses file output versus Hardcopy first, then collects the final destination value.
For file output, `CWD` is the current working directory for bare filenames.
Press `F3` only on the file destination prompt to cycle `Raw`, `Framed`, and `Page break`.
Framed and page-break output ask for the separator before returning to the file destination prompt.
Hardcopy sends raw output to a shell printer command such as `lpr`, `lp`, or `cat > /dev/lp1`.
### Explainer links
- [Output Help](topic:output)
- [Output Format Help](topic:output-format)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Destination choices
File output writes exported text to a path.
Hardcopy sends raw exported text to the chosen printer command.

## topic:output-separator
```ytnova-help-meta
title: Output Separator Help
contexts: prompt.output-separator
```
### Contextual F1
Output separator appears only when `F3` selects framed or page-break output.
Raw output bypasses this prompt.
### Explainer links
- [Output Help](topic:output)

### Long form
#### Separator rules
The separator is reused between files for the current framed or page-break export.
It is not appended after the last file.

## topic:showall
```ytnova-help-meta
title: Showall Help
contexts: main.showall
```
### Contextual F1
Showall lists every file inside the current logged volume in one aggregated file list.
It keeps single-volume scope while flattening directory boundaries.
### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Tagged](topic:tagged)

### Long form
#### Showall behavior
* **Scope**: Showall lists every file inside the current logged volume only.
* **Esc**: Return to the previously selected directory.
* **Open owner**: `\` jumps to the owner directory of the selected file inside the current logged volume.
* **Sort**: Repeating `S` changes sort; it does not leave Showall.
* **Filter**: Filter still applies only to the current Showall result set.
* **Filter**: Filter still applies only to the current Showall result set, and `Tab` inside the filter prompt narrows that same result set to tagged-only rows.

## topic:global
```ytnova-help-meta
title: Global Help
contexts: main.global
```
### Contextual F1
Global lists files from every logged volume in one aggregated file list.
It keeps multi-volume scope while flattening directory boundaries.
### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Tagged](topic:tagged)

### Long form
#### Global behavior
* **Scope**: Global lists files from every logged volume.
* **Esc**: Return to the previously selected directory.
* **Open owner**: `\` jumps to the owner directory of the selected file even when it lives under another logged volume root.
* **Global**: Repeating `G` is a no-op while you are already in Global.
* **Filter**: Filter still applies only to the current Global result set.
* **Filter**: Filter still applies only to the current Global result set, and `Tab` inside the filter prompt narrows that same result set to tagged-only rows.

## topic:f7
```ytnova-help-meta
title: F7 Preview Help
contexts: overlay.f7-dir,overlay.f7-file
```
### Contextual F1
F7 preview overlays file preview controls on top of the underlying file-selection context.
The preview owns scrolling while the underlying selection still owns the file target.
### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Applications menu](topic:applications-menu)

### Long form
#### Preview navigation
* **Select file**: `Up`, `Down`, `PgUp`, `PgDn`, `Home`, and `End` move the live file selection.
* **Preview lines**: `Shift-Up` and `Shift-Down`, and `Ctrl-P` and `Ctrl-N`, scroll the preview.
* **Preview pages**: `Shift-PgUp` and `Shift-PgDn` page the preview. `Shift-Home` and `Shift-End` jump to its ends.
* **Leave preview**: `F7` returns to the underlying file or directory view.
* **Applications**: `F9` opens the applications menu without leaving preview.
* **Blocked keys**: `F8` split does nothing while preview is active, and `Tab` does not switch panels.
* **Cancel**: `Esc` leaves preview immediately.

#### Preview commands
* **Copy**: Copy the selected file; `Ctrl-K` copies the tagged set.
* **Filter**: Filter the list without leaving preview; `Ctrl-S` searches only the tagged set.
* **File commands**: Attributes, Delete, Edit, Invert, `J compare`, `M/^N move`, Newfile, Rename, Tag/Untag, View, Output, `eXecute`, `pathcopY`, `Z archive`, `/ jump`, and `` ` dotfiles `` still work.

## topic:f8
```ytnova-help-meta
title: F8 Split Help
contexts: none
```
### Contextual F1
Split mode keeps two panels active at once, and runtime F1 opens the directory or file split page for the active panel.
Use the split page for the live footer command list and this page for the shared split model.
### Explainer links
- [Navigation](topic:navigation)
- [Directory split page](topic:f8-dir)
- [File split page](topic:f8-file)

### Long form
#### Split controls
* **Leave split**: Press `F8` again to return to single-panel mode.
* **Tab**: Switch the active panel and keep the passive panel's state intact.
* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.
* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state.

## topic:f8-dir
```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-dir
```
### Contextual F1
The split-directory page combines split-only rules with the active directory-footer command list.
It is the runtime F1 page when the split focus is on the tree panel.
### Explainer links
- [Navigation](topic:navigation)
- [Split overview](topic:f8)
- [Directory mode](topic:dir)

### Long form
#### Split controls
* **Leave split**: Press `F8` again to return to single-panel mode.
* **Tab**: Switch the active panel and keep the passive panel's state intact.
* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.
* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state.

#### Split directory commands
* **Command list**: The split-directory page reuses the directory-footer command family (`1..9 view`, Attributes, Copy, Delete, Filter, Global, Invert, `J compare`, `K volume`, Log, Makedir, Newfile, Pipe, Quit, Rename, Showall, Tag, Untag, MoveDir, Output, `eXecute`, `Z archive`, `/ jump`, and `` ` dotfiles ``) for the active panel. Tagged-only scope now lives under `Filter` via `Tab` inside the prompt.

## topic:f8-file
```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-file
```
### Contextual F1
The split-file page combines split-only rules with the active file-footer command list.
It is the runtime F1 page when the split focus is on the file panel.
### Explainer links
- [Navigation](topic:navigation)
- [Split overview](topic:f8)
- [File mode](topic:file)

### Long form
#### Split controls
* **Leave split**: Press `F8` again to return to single-panel mode.
* **Tab**: Switch the active panel and keep the passive panel's state intact.
* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.
* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state.

#### Split file commands
* **Command list**: The split-file page reuses the file-footer command family (`1..9 view`, Attributes, `C/^K copy`, Delete, Edit, Filter, Hex, Invert, `J compare`, `K volume`, Log, `M/^N move`, Newfile, Pipe, Quit, Rename, Sort, Tag, Untag, View, Output, `eXecute`, `pathcopY`, `Z archive`, `/ jump`, and `` ` dotfiles ``) for the active panel. Tagged-only scope now lives under `Filter` via `Tab` inside the prompt.

## topic:history-dialog
```ytnova-help-meta
title: History Help
contexts: dialog.history
```
### Contextual F1
The history dialog reuses earlier prompt entries and supports pinning or deletion.
It is a shared helper surface for prompts that keep history.
### Explainer links
- [Navigation](topic:navigation)

### Long form
#### History actions
* **Select entry**: `Up` and `Down` move through the current history list.
* **Scroll long entry**: `Left` and `Right` shift a long history line horizontally.
* **Pin**: `P` keeps an important entry at the top of the current history list.
* **Delete**: `D` removes the selected entry from the current history list.
* **Accept**: `Enter` reuses the selected entry.
* **Cancel**: `Esc` closes the dialog without reusing an entry.

## topic:volume-menu
```ytnova-help-meta
title: Volume Help
contexts: dialog.volume-menu
```
### Contextual F1
The volume menu lists loaded volumes, lets you switch to one, and can release a volume.
Loaded volumes keep independent in-memory state until released or reloaded.
### Explainer links
- [Navigation](topic:navigation)

### Long form
#### Volume actions
* **Select volume**: `Up` and `Down` move through the loaded-volume list.
* **Switch volume**: `Enter` activates the selected volume.
* **Keep state**: Selecting the already active volume keeps its current in-memory state.
* **Release volume**: `D` unloads the selected volume unless it is the last remaining one.
* **Cancel**: `Esc` closes the menu.

## topic:applications-menu
```ytnova-help-meta
title: Applications Help
contexts: dialog.applications
```
### Contextual F1
The applications menu lists configured application presets.
Use `Enter` to select the highlighted preset, `E` to edit the commands catalog that backs application presets, and `Esc` to cancel.
### Explainer links
- [Navigation](topic:navigation)

### Long form
#### Applications actions
* **Select preset**: `Up` and `Down` move through the preset list.
* **Edit presets**: `E` opens the commands catalog so application presets can be changed without leaving the chooser family.
* **Cancel menu**: `Esc` closes the placeholder chooser without selecting a preset.
* **Current state**: The shipped Applications menu is still a lightweight placeholder surface, so treat it as a shell for future app-launch behavior.

## topic:f2-picker
```ytnova-help-meta
title: F2 Picker Help
contexts: dialog.f2-picker
```
### Contextual F1
The F2 picker browses for a path or preset supported by the active prompt.
It is a prompt helper, not a standalone mode, and it also exposes local volume cycling, path logging, and dotfile toggles without leaving the prompt.
### Explainer links
- [Navigation](topic:navigation)
- [Command-line editing](topic:command-line-editing)

### Long form
#### F2 picker actions
* **Move in the tree**: `Up`/`Down` move the selection, while `Left` and `Right` collapse, expand, or enter subtrees.
* **Cycle loaded volumes**: `<` and `>` rotate through logged volumes in the picker.
* **Log a new path**: `L` logs a new directory or volume without leaving the picker.
* **Toggle dotfiles**: `` ` `` reuses the invoking view's dotfile visibility control inside the picker.
* **Select or cancel**: `Enter` selects the highlighted directory and `Esc` cancels the picker.
