# YtreeNova canonical help source (English)

This file is the single authored help source for contextual `F1` help and the
long-form help projections that later regenerate the manpage and
`docs/USAGE.md`.

## Topic-block schema

Every topic block in this file follows the same parser-facing contract:

1. The block starts with a level-2 heading in the exact form
   `## topic:<topic-id>`.
2. The heading is followed immediately by a fenced metadata block labelled
   `ytnova-help-meta`.
3. The metadata block contains exactly these keys, in this order:
   * `title:` — plain-text topic title.
   * `contexts:` — comma-separated stable runtime context/prompt IDs, or the
     literal `none` for link-only explainer pages.
4. The block then contains these sections in order:
   * required `### Contextual F1`
   * optional `### Explainer links`
   * required `### Long form`
5. When `### Explainer links` is present, every item uses Markdown link syntax
   with a `topic:` target, for example `- [Navigation](topic:navigation)`.
6. `### Long form` contains one or more level-4 subsections (`#### ...`).
   Their order is preserved as-authored for later projection.

Multiple runtime contexts may reuse one topic block by sharing a
comma-separated `contexts:` line. Reusable explanations stay in their own
shared topics and are linked with `topic:` links instead of being copied into
multiple blocks. Linked help must stay shallow: one or two hops from the
contextual page is the maximum intended depth.

## topic:intro
```ytnova-help-meta
title: Help Contents
contexts: none
```
### Contextual F1
YtreeNova keeps `F1` short and task-local. Use the contextual page for the
active surface, then follow shared explainer links only when you need more
background.

### Explainer links
- [Navigation](topic:navigation)
- [Shared commands](topic:shared-commands)
- [F10 config](topic:f10)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Purpose
This link-only topic introduces the canonical help set and explains why the
help system is split into concise contextual pages plus shared explainers.

#### Contents
*   Start with **Navigation** for the shared movement baseline.
*   Use **Shared commands** for the cross-context function-key family,
    including **F10 config**.
*   Use **Directory mode**, **File mode**, **Archive directory**, **Archive
    file**, **Showall**, **Global**, **F7 preview**, and **F8 split** for
    context-specific command pages.
*   Use **Filter**, **Compare overview**, and **Output overview** when one
    command family needs more detail than a one-line definition can hold.
*   Use **Command-line editing**, **VI keys**, **Theming**, and **F10
    config** for shared operator/reference topics that apply across multiple
    prompts or configuration surfaces.

## topic:navigation
```ytnova-help-meta
title: Navigation
contexts: none
```
### Contextual F1
Arrow keys, paging keys, `Home`, `End`, and `Enter` keep their usual ownership.
Contextual pages explain only the extra keys or caveats that differ from the
normal navigation baseline.

### Explainer links
- [Directory mode](topic:dir)
- [File mode](topic:file)
- [Shared commands](topic:shared-commands)
- [F7 preview](topic:f7)
- [F8 split](topic:f8)

### Long form
#### Baseline movement
Navigation is shared vocabulary. Context-specific help should assume this
baseline and document only the keys, limits, and ownership changes that are
special to that surface.

#### Common keys
*   **Up/Down** move the active selection.
*   **Page Up/Page Down** scroll by pages in list-oriented surfaces.
*   **Home/End** jump to the start or end of the current list.
*   **Enter** accepts the current selection or toggles between paired views
    such as tree/file or preview on/off when that context owns Enter.
*   **Esc** backs out of temporary overlays and prompt/dialog flows without
    committing the pending action.

## topic:shared-commands
```ytnova-help-meta
title: Shared Commands
contexts: none
```
### Contextual F1
Shared Commands explains the cross-context help keys and overlays that can
appear from multiple main views. Use it for the shared function-key family
(`F1`, `F5`, `F6`, `F7`, `F8`, `F9`, `F10`) instead of repeating those hints
on every context page.

### Explainer links
- [Navigation](topic:navigation)
- [F7 preview](topic:f7)
- [F8 split](topic:f8)
- [F10 config](topic:f10)

### Long form
#### Shared commands
*   **F1** (help): Open contextual help for the active surface.
*   **F5** (refresh): Refresh the current view.
*   **F6** (stats): Switch the stats/details presentation for the active view.
*   **F7** (autoview): Toggle preview/autoview for the active file context.
*   **F8** (split): Toggle split-screen mode.
*   **F9** (apps): Open the applications menu shell.
*   **F10** (config): Open the configuration command surface.
*   **Esc** (cancel): Close the current help popup or cancel the active
    overlay/prompt.

## topic:command-line-editing
```ytnova-help-meta
title: Command-line Editing
contexts: none
```
### Contextual F1
Prompt editing is shared across filter, compare, output, shell-command, and
path-entry prompts. Learn it once here instead of re-reading it in every
prompt-local page.

### Explainer links
- [Navigation](topic:navigation)
- [VI keys](topic:vi-keys)

### Long form
#### Editing keys
*   **Left/Right** move within the current prompt buffer.
*   **Home/End** jump to the start or end of the current prompt buffer.
*   **Backspace/Delete** erase the character to the left/right of the cursor.
*   **Enter** accepts the current prompt value.
*   **Esc** cancels the prompt without committing it.

#### Shared helpers
*   **Up** opens or cycles prompt history when that prompt keeps history.
*   **F2** opens a browser/picker when the active prompt supports browsing a
    path or reusable choice list.
*   Prompt-local `F1` explains only syntax and scope that are specific to that
    prompt; it should not re-teach the shared editing baseline.

## topic:vi-keys
```ytnova-help-meta
title: VI Keys
contexts: none
```
### Contextual F1
`VI_KEYS=1` changes command ownership to preserve lowercase vi-style
navigation. This explainer keeps the mode shift separate from ordinary
navigation help so users do not mix the two models.

### Explainer links
- [Navigation](topic:navigation)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Navigation remap
With `VI_KEYS=1`, lowercase **h/j/k/l** become Left/Down/Up/Right and **^U**
/**^D** become page-up/page-down.

#### Command collisions
Commands that would collide with lowercase vi navigation move to uppercase or a
non-conflicting fallback. Examples include **J** for Compare, **K** for Volume
Menu, **D** for Delete Tagged, and **U** for Untag All where applicable.

## topic:f10
```ytnova-help-meta
title: F10 Config Help
contexts: none
```
### Contextual F1
F10 opens the configuration command surface for profile, commands, themes, and
other persistent setup changes. Use this page for the high-level map, then
follow Theming when the change is color/layout specific.

### Explainer links
- [Shared commands](topic:shared-commands)
- [Theming](topic:theming)

### Long form
#### Config surface
Use **F10** to reach configuration-oriented commands instead of treating them
as per-directory actions. Persistent changes belong here, not in the active
file or directory command pages.

#### Related areas
Theme selection, semantic colors, and presentation tweaks are covered by
**Theming**. Prompt-edit/history behavior that appears inside config flows is
still owned by **Command-line Editing**.

## topic:theming
```ytnova-help-meta
title: Theming
contexts: none
```
### Contextual F1
Themes control semantic UI roles such as footer, picker, help, selection, and
severity colors. This topic keeps color-system explanation separate from the
day-to-day command pages.

### Explainer links
- [F10 config](topic:f10)

### Long form
#### Theme model
Themes are role-based: users configure semantic roles rather than styling each
surface with ad-hoc colors. Help popups, pickers, and the footer command strip
each have their own dedicated roles.

#### Editing path
Use **F10** and the theme/config files to change theme selection or role
definitions. Keep contrasts readable for help, picker, and selection surfaces;
those are high-frequency navigation aids.

## topic:dir
```ytnova-help-meta
title: Directory Help
contexts: main.dir
```
### Contextual F1
Directory Help is the directory-specific command page. Use Navigation for the
shared movement keys; this page keeps the focus on directory actions,
tree/logging behavior, and directory-only caveats.

### Explainer links
- [Navigation](topic:navigation)
- [Shared commands](topic:shared-commands)

### Long form
#### Directory commands
*   **1..9 view**: Select the active panel's base directory/file view while
    tree-focused. `1` resets to Name, `2` shows Attributes, `3` shows Owner,
    `4` shows Times, `5`, `7`, `8`, and `9` change the file projection, `6`
    toggles panel-wide row size units, `0` is unused, and `9` is a silent
    no-op outside Git worktrees.
*   **A** (Attributes): Open attributes submenu for directory metadata changes:
    mode (chmod), owner, group, date.
*   **C** (Copy): Copy the selected directory branch.
*   **D** (Delete): Delete selected directory.
*   **F** (Filter): Set file filter. Supports regex patterns (e.g., `*.c`),
    exclusions (`-*.o`), attributes (`:r`, `:x`), dates (`>2023-01-01`), and
    sizes (`>1M`).
*   **G** (Global): Show all files across all logged volumes in one global
    list.
*   **I** (Invert Tags): Toggle tag state for files in the selected/current
    directory scope.
*   **J** (Compare): Open the compare submenu (directory, logged tree, or
    external viewer). With `VI_KEYS=1`, use uppercase `J` for this action.
*   **K** (volume): Open the volume picker.
*   **L** (Log): Log a new directory or archive file. Logging an already logged
    volume/path performs a fresh reload and reanchors selection at the volume
    root.
*   **M** (Makedir): Create a new directory.
*   **N** (New File): Create a new empty file.
*   **O** (Only tagged): Toggle tagged-only file-list view for the current
    directory scope.
*   **P** (Pipe, or **|**): Pipe the selected directory to a command (stdin).
*   **Q** (Quit): Quit ytnova.
*   **R** (Rename): Rename selected directory.
*   **S** (Showall): Show all files in all directories of the current volume.
*   **T** (Tag): Tag all files in the selected directory.
*   **U** (Untag): Untag all files in the selected directory.
*   **V** (MoveDir): Move the selected directory branch.
*   **W** (Write): Export files in the selected directory to a command or file
    using a formatting dialog (Raw, Framed, Page Break).
*   **X** (eXecute): Execute a shell command. Leave `{}` unquoted; ytnova
    replaces it with the current directory path and shell-quotes the expanded
    path. Prompt **F1** also explains the tagged-file `^X` repeat path.
*   **Z** (archive): Create an archive from the current selection. If one or
    more files are tagged, ytnova archives the tagged files. If nothing is
    tagged, ytnova archives the selected file or selected directory. Directory
    sources are archived recursively. Supported destination suffixes: `.tar`,
    `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`, `.tar.xz`/`.txz`, `.zip`.
*   **/** (jump): Jump to a file or directory by name within the current list.
*   **`** (Backtick): Toggle visibility of hidden dot-files and directories.

#### Tree navigation
*   **Enter**: On logged directories, switch to File Mode (focus the file
    window). On unlogged/not-yet-scanned directories, perform one-level
    log/reveal (same behavior as `+`) and stay in Directory Mode.
*   **-**: State-based collapse/release. First press collapses an expanded
    node. Second press on a collapsed logged node evicts the file list (sets
    `+` status) and marks the directory as Unlogged. At root, use `-` to
    release logged contents.
*   **Tree status marker**: Unlogged directories use `+` in the left status
    margin column. Directory names do not carry a `+` suffix; an unlogged
    directory may still show `/` when it has subdirectories.
*   **Left Arrow**: If the selected directory is expanded, collapse it.
    Otherwise move selection to its parent directory. Repeated `Left` keeps
    ascending (and collapsing where needed). At filesystem root, `Left` is a
    no-op.
*   **Right Arrow** (Drill Down): Progressive depth navigation. If collapsed:
    expand one level. If already expanded: move cursor to the first child. It
    does not jump to siblings.
*   **+** (or **=**): One-level log/reveal only (no cursor movement). `=` is a
    convenience alias (unshifted `+` on most keyboards).
*   **\*** (Asterisk): Recursively expand the current directory and all its
    subdirectories.

## topic:file
```ytnova-help-meta
title: File Help
contexts: main.file
```
### Contextual F1
File help explains the live file footer commands, file-view operations, and
file-specific caveats that are not obvious from the command strip alone.

### Explainer links
- [Navigation](topic:navigation)
- [Output](topic:output)
- [Shared commands](topic:shared-commands)

### Long form
#### File commands
*   **A** (Attributes): Open attributes submenu for selected file metadata:
    mode, owner, group, date.
*   **C** (Copy): Copy the selected file.
*   **^K**: Copy all tagged files.
*   **D** (Delete): Delete selected file. *(With `VI_KEYS=1`, use lowercase
    `d` for this action and uppercase `D` for Delete Tagged.)*
*   **E** (Edit): Edit selected file with `$EDITOR` (default: vi).
*   **F** (Filter): Set file filter.
*   **H** (Hex): View selected file in hex mode.
*   **I** (Invert Tags): Toggle the tag state of all visible files.
*   **J** (Compare): Compare the selected file with a target file.
*   **L** (Log): Log a new directory or archive file. Logging an already logged
    volume/path performs a fresh reload and reanchors selection at the volume
    root.
*   **M** (Move): Move the selected file.
*   **^N**: Move all tagged files.
*   **N** (New File): Create a new empty file.
*   **O** (Only tagged): Toggle tagged-only file-list view (show tagged files
    only).
*   **P** (Pipe, or **|**): Pipe content of file to a command (stdin).
*   **R** (Rename): Rename the selected file.
*   **S** (Sort): Sort filelist (Access time, Change time, Extension, Group,
    Modification time, Name, Owner, Size).
*   **^S** (Search): Execute grep on tagged files. The prompt expects search
    text, not a full grep command; ytnova builds `grep -i -- PATTERN {}`
    internally and untags files that do not match. Prompt **F1** summarizes
    the tagged-scope behavior.
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all displayed files.
*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`
    for this action.)*
*   **^U**: Untag all displayed files. *(With `VI_KEYS=1`, `^U` is page-up
    navigation and uppercase `U` becomes Untag All.)*
*   **V** (View): View file with the pager defined in the main config (default:
    less).
*   **^V**: **View Tagged**. View all tagged files sequentially.
*   **W** (Write): Export the selected file to a command or file using a
    formatting dialog (Raw, Framed, Page Break).
*   **X** (eXecute): Execute a shell command. Leave `{}` unquoted; ytnova
    replaces it with the selected file path and shell-quotes the expanded path.
    Prompt **F1** also explains the tagged-file `^X` repeat path.
*   **Y**: (Pathcopy): Copy selected file, replicating its directory structure
    relative to the current volume root.
*   **Z** (archive): Create an archive from tagged files, or from the selected
    file/directory when nothing is tagged. Directory sources are archived
    recursively.

#### File-window navigation
*   **1 .. 4** (Base View): Select the file or directory base view for the
    active panel: `1` Name, `2` Attributes, `3` Owner, `4` Times. Press `2`,
    `3`, or `4` again to return to `1`.
*   **5**: Toggle the compact Name/full-width file rendering variant when the
    current base view is `1` / Name.
*   **6**: Toggle binary vs human-readable size units for directory/file rows
    on the active panel.
*   **7**: Toggle Mini preview detail in the file window.
*   **8**: Toggle File detail in the file window.
*   **9**: Toggle the Git status band for filesystem file lists when the
    current directory is inside a Git worktree.
*   **0**: Currently unused; silent no-op.
*   **Enter**: Switch to Full Screen File Mode / Directory Mode.
*   **Left Arrow**: Move to the previous visible file column; in one-column
    layouts this performs page-up navigation.
*   **Right Arrow**: Move to the next visible file column; in one-column
    layouts this performs page-down navigation.
*   **Date Changes:** Date actions change Accessed time, Modified time, or both
    (POSIX does not allow setting creation/birth time here).

## topic:archive-dir
```ytnova-help-meta
title: Archive Directory Help
contexts: main.archive-dir
```
### Contextual F1
Archive directory help mirrors the live archive-directory footer, then adds the
archive-specific caveats that differ from normal filesystem directory behavior.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [Output](topic:output)

### Long form
#### Archive directory commands
*   **J** (Compare): Open compare flow. With `VI_KEYS=1`, use uppercase `J`
    for this action.
*   **D** (Delete): Delete selected archive directory entry.
*   **F** (Filter): Set file filter.
*   **G** (Global): Show all files across all logged volumes in one global
    list.
*   **I** (Invert Tags): Toggle tag state for files in the selected/current
    archive directory scope.
*   **L** (Log): Log a new directory or archive. Logging an already logged
    volume/path performs a fresh reload and reanchors selection at the volume
    root.
*   **M** (Makedir): Create directory in archive context where supported.
*   **O** (Only tagged): Toggle tagged-only file-list view for the current
    archive directory scope.
*   **R** (Rename): Rename selected archive directory entry.
*   **S** (Showall): Show all files in the archive.
*   **T** (Tag): Tag all files in current virtual directory.
*   **U** (Untag): Untag all files in current virtual directory.
*   **1 .. 4** (Dir Mode): Select the active panel's base archive-directory/
    file view while tree-focused: `1` Name/reset, `2` Attributes, `3` Owner,
    `4` Times. `5`, `7`, `8`, and `9` update the panel's file projection; `6`
    toggles panel-wide row size units; `0` is unused; `9` is a silent no-op in
    archives.

#### Archive directory navigation
*   **Enter**: Switch to Archive-File Mode.
*   **-**: State-based collapse/release. Expanded nodes collapse; collapsed
    logged nodes (or logged leaves) unlog/release.
*   **Left Arrow**: Collapse the current archive directory when expanded;
    otherwise move selection to its parent directory.
*   **Right Arrow** (Drill Down): Progressive depth navigation. If collapsed:
    expand one level. If already expanded: move cursor to the first child.
*   **+** (or **=**): Expand the current archive directory by one level.
*   **\\**: At archive non-root, jump to archive root. At archive root, exit
    to parent physical directory.

## topic:archive-file
```ytnova-help-meta
title: Archive File Help
contexts: main.archive-file
```
### Contextual F1
Archive file help mirrors the live archive-file footer and documents the
differences between archive file actions and normal filesystem file actions.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Output](topic:output)

### Long form
#### Archive file commands
*   **C** (Copy): Copy selected file (including extract/copy paths).
*   **^K** (Copy Tagged): Copy all tagged files.
*   **D** (Delete): Delete selected archive file entry.
*   **F** (Filter): Set file filter.
*   **H** (Hex): View file in hex mode.
*   **I** (Invert Tags): Toggle the tag state of all visible files.
*   **M** (Move): Move selected file using archive-aware semantics.
*   **O** (Only tagged): Toggle tagged-only file-list view (show tagged files
    only).
*   **P** (Pipe, or **|**): Pipe content to command.
*   **R** (Rename): Rename selected archive file entry.
*   **S** (Sort): Sort file list.
*   **^S** (Search): Search tagged files for a string. The prompt expects
    search text, not a full grep command; ytnova builds `grep -i -- PATTERN {}`
    internally and untags files that do not match. Prompt **F1** summarizes
    the tagged-scope behavior.
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all files.
*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`
    for this action.)*
*   **^U**: Untag all files. *(With `VI_KEYS=1`, `^U` is page-up navigation and
    uppercase `U` becomes Untag All.)*
*   **V** (View): View file.
*   **^V**: **View Tagged**. View all tagged files sequentially.
*   **W** (Write): Export file content to a command or file.
*   **Y** (Pathcopy): Copy selected file with relative path preservation.

#### Archive file navigation
*   **1 .. 4** (Base View): Select the archive-file base view for the active
    panel: `1` Name, `2` Attributes, `3` Owner, `4` Times. Press `2`, `3`, or
    `4` again to return to `1`.
*   **5**: Toggle the compact Name/full-width file rendering variant when the
    current base view is `1` / Name.
*   **6**: Toggle binary vs human-readable size units for archive rows.
*   **7**: Toggle Mini preview detail in the file window.
*   **8**: Toggle File detail in the file window.
*   **9**: Silent no-op in archive file lists.
*   **0**: Currently unused; silent no-op.
*   **Enter**: Switch to Archive-Dir Mode.
*   **\\**: No-op.
*   Archive file-window status text uses `Unlogged` when the selected directory
    is unlogged and `No files` when the selected directory is logged and empty.

## topic:filter
```ytnova-help-meta
title: Filter Help
contexts: prompt.filter,prompt.filter-tagged
```
### Contextual F1
Use normal glob-like patterns such as `*.c`, comma-separated unions such as
`*.c,*.h`, and exclusions such as `-*.o`.
Extended selectors such as `:r`, `:x`, `>2023-01-01`, and `>1M` stay valid in
the runtime prompt, and an empty entry falls back to `*`.
Filter prompts stay scoped to the active file-list family, including tagged
aggregates when the current prompt came from a tagged-only view.

### Explainer links
- [Navigation](topic:navigation)
- [Showall](topic:showall)
- [Global](topic:global)
- [Command-line editing](topic:command-line-editing)

### Long form
#### Filter syntax
Use normal glob-like patterns such as `*.c`, comma-separated unions such as
`*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as
attributes (`:r`, `:x`), dates (`>2023-01-01`), or sizes (`>1M`). If the
shell would expand the pattern, quote it before launching ytnova.

#### Scope rules
Filter prompts stay scoped to the active file-list family. Directory/File,
archive, Showall, and Global contexts may share syntax while still applying the
result to their own current scope and tagged/untagged conventions.

## topic:compare
```ytnova-help-meta
title: Compare Help
contexts: none
```
### Contextual F1
Compare help is split into prompt-local runtime topics plus this shared long-form
explainer. Runtime `F1` pages stay focused on the active compare step, then link
back here for the broader compare model.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [File mode](topic:file)

### Long form
#### Compare flows
*   **File compare (`J` in File Mode):** Compare the selected file against a
    target file. ytnova can use an external file-diff helper if configured.
    *   `FILEDIFF` may use `%1` (source) and `%2` (target) placeholders; when
        omitted, ytnova appends source and target paths to the helper command.
*   **Directory compare (`J` in Directory Mode):**
    *   `D`: compare the current directory.
    *   `T`: compare the current logged tree.
    *   `X`: launch an external directory/tree compare viewer.
    *(With `VI_KEYS=1`, use uppercase `J` for this action.)*

#### Compare rules
*   Internal compare tags matches on the active/source side only.
*   Logged-tree compare uses logged content only; it does not auto-log unopened
    subdirectories.
*   There is no separate "compare tagged files" mode.

## topic:compare-target
```ytnova-help-meta
title: Compare Target Help
contexts: prompt.compare-target
```
### Contextual F1
The current file, directory, or logged tree is the compare source.
Enter the target path directly, or use `F2` to browse and `Up` for prompt
history.
In split view, the inactive panel seeds the default compare target.

### Explainer links
- [Navigation](topic:navigation)
- [Compare overview](topic:compare)

### Long form
#### Runtime scope
This runtime-only topic keeps the compare-target popup concise while the shared
compare explainer continues to own the broader compare documentation bundle.

## topic:compare-scope
```ytnova-help-meta
title: Compare Scope Help
contexts: prompt.compare-scope
```
### Contextual F1
Directory only compares the current directory.
Logged tree compares the current logged tree recursively and never auto-logs
unopened `+` subdirectories.
External viewer launches `DIRDIFF` or `TREEDIFF` helpers instead of tagging
runtime compare results.

### Explainer links
- [Navigation](topic:navigation)
- [Compare overview](topic:compare)

### Long form
#### Runtime scope
This runtime-only topic explains the compare-scope chooser without duplicating
the full compare documentation into prompt-local code.

## topic:compare-basis
```ytnova-help-meta
title: Compare Basis Help
contexts: prompt.compare-basis
```
### Contextual F1
Size checks file length and Date checks the last-modified time.
`siZe+date` marks a difference when either size or modification time differs.
Hash opens both files and compares their content exactly, so it is slower than
metadata-only checks.

### Explainer links
- [Navigation](topic:navigation)
- [Compare overview](topic:compare)

### Long form
#### Runtime scope
This runtime-only topic keeps the compare-basis chooser generated-content
driven without widening the long-form command reference.

## topic:compare-results
```ytnova-help-meta
title: Compare Result Help
contexts: prompt.compare-results
```
### Contextual F1
Choose which compare result to tag in the active/source-side file list.
`diFferent` tags basis mismatches and `Unique` tags source-only entries.
Match, Newer, Older, Type-mismatch, and Error each tag only that one outcome.

### Explainer links
- [Navigation](topic:navigation)
- [Compare overview](topic:compare)

### Long form
#### Runtime scope
This runtime-only topic keeps the compare-result chooser generated-content
driven while the shared compare explainer owns the durable long-form docs.

## topic:output
```ytnova-help-meta
title: Output Help
contexts: none
```
### Contextual F1
Output help is split into runtime prompt topics plus this shared long-form
explainer. Runtime `F1` pages stay focused on the active output step, then link
back here for the durable write/export model.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Archive file](topic:archive-file)

### Long form
#### Output destinations
Write/output flows may send content to a file path or to an external command.
The canonical prompt sequence explains the distinction between ordinary file
output and hardcopy-oriented command entry so the same authored text can serve
filesystem, archive, and prompt-local help.

#### Output formats
The output dialog owns the format choices used by write/export flows, including
Raw, Framed, and Page Break variants plus any separator prompt that follows.
If the runtime later narrows a contextual slice, the generated long-form docs
must still come from this one authored topic.

## topic:output-format
```ytnova-help-meta
title: Output Format Help
contexts: prompt.output-format
```
### Contextual F1
Raw writes content without frame headings.
Framed adds per-file heading/footer framing, and Page break inserts a separator
between successive files without leaving a trailing separator at the end.
Choose the format first; later prompts gather separators and destinations.

### Explainer links
- [Navigation](topic:navigation)
- [Output overview](topic:output)

### Long form
#### Runtime scope
This runtime-only topic keeps the format chooser generated-content driven while
the shared output explainer owns the durable long-form docs.

## topic:output-destination
```ytnova-help-meta
title: Output Destination Help
contexts: prompt.output-destination
```
### Contextual F1
Destination chooses whether write/output goes to a file path or to an external
command.
When the prompt asks for the final target, enter either the destination file or
the command line exactly as you want it run.
Leave the destination blank only to cancel and return without writing.

### Explainer links
- [Navigation](topic:navigation)
- [Output overview](topic:output)

### Long form
#### Runtime scope
This runtime-only topic keeps the destination chooser generated-content driven
without duplicating prompt prose in print/output controllers.

## topic:output-separator
```ytnova-help-meta
title: Output Separator Help
contexts: prompt.output-separator
```
### Contextual F1
Framed and Page break modes prompt for a separator string before the
destination step.
Leave the separator blank to accept the default triple-backtick fence.
The separator text is reused between files only for the selected framed/page
format; Raw output skips this prompt entirely.

### Explainer links
- [Navigation](topic:navigation)
- [Output overview](topic:output)

### Long form
#### Runtime scope
This runtime-only topic keeps the separator prompt generated-content driven
while the shared output explainer remains the canonical long-form reference.

## topic:showall
```ytnova-help-meta
title: Showall Help
contexts: main.showall
```
### Contextual F1
Showall help explains the single-volume aggregated file view and the commands
or caveats that differ from ordinary file mode.
Press `Esc` to return to the previously selected directory.
Press `\\` to jump to the owner directory of the selected file inside the
current logged volume.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Global](topic:global)

### Long form
#### Showall behavior
Showall toggles file-list mode for all files in the current logged volume.
Press **Esc** to return to the previously selected directory. Press **\\** to
jump to the owner directory of the selected file.

#### Scope notes
Shared file-view commands still behave like ordinary file mode unless the
aggregated single-volume scope changes the ownership of the current result set.

## topic:global
```ytnova-help-meta
title: Global Help
contexts: main.global
```
### Contextual F1
Global help explains the multi-volume aggregated file view, including how it
returns to owner directories and how its scope differs from ordinary file mode.
Press `Esc` to return to the previously selected directory.
Press `\\` to jump to the owner directory of the selected file even when that
owner lives under a different logged volume root.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)
- [Showall](topic:showall)

### Long form
#### Global behavior
Global toggles file-list mode for all files across all logged volumes. Press
**Esc** to return to the previously selected directory. Press **\\** to jump
to the owner directory of the selected file.

#### Multi-volume scope
Global shares the aggregated-file mental model with Showall but keeps room for
multi-volume caveats such as owner-directory jumps that cross volume roots.

## topic:f7
```ytnova-help-meta
title: F7 Preview Help
contexts: overlay.f7-dir,overlay.f7-file
```
### Contextual F1
F7 help explains preview ownership and how the preview overlay interacts with
the underlying directory or file context.
Use `Shift+Up/Down` or `^P/^N` to scroll preview contents line by line.
Use `Shift+PgUp/PgDn` for pages and `Shift+Home/End` to jump to the top or
bottom of the current preview.

### Explainer links
- [Navigation](topic:navigation)
- [File mode](topic:file)

### Long form
#### Preview behavior
File Preview Mode is activated by **F7**. The screen layout changes to show
the file list on the left (or active pane) and the file contents on the right.
Press **F7** again to leave preview mode.

#### Preview controls
*   Use **Up/Down**, **Page Up/Down**, and **Home/End** to move the selection
    in the file list. The preview pane updates immediately.
*   Use **Shift+Up/Down** (or **^P** / **^N**) to scroll the preview contents
    line by line.
*   Use **Shift+Page Up/Down** to scroll by pages.
*   Use **Shift+Home/End** to jump to the beginning or end of the file.

## topic:f8
```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-dir,overlay.f8-file
```
### Contextual F1
F8 help explains split-view ownership, inactive-panel defaults, and the keys
or caveats that only appear while split mode is active.
Press `Tab` to switch the active panel while leaving the passive panel's state
intact.
Copy, move, and compare prompts default to the inactive panel as the
destination/target while split mode is active.

### Explainer links
- [Navigation](topic:navigation)
- [Directory mode](topic:dir)
- [File mode](topic:file)

### Long form
#### Split behavior
Split Screen Mode is activated by **F8**. The screen is divided vertically into
two independent file manager panels. Press **F8** again to return to
single-panel mode.

#### Split controls
*   Press **Tab** to switch active control between the Left and Right panels.
*   Copy, move, and compare prompts default to the inactive (passive) panel as
    the destination/target when split mode is active.
*   Split mode keeps panel-local selection, view, tag, and volume state
    isolated so the passive panel remains a real target rather than a mirror.
