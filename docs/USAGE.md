<!-- Auto-generated from etc/help/help.en.md by scripts/generate_help_assets.py; do not edit directly. -->

# NAME

ytnova - a file manager for Unix-like systems

# SYNOPSIS

`ytnova` [`--init`] [`-v`|`-V`|`--version`] [`-p` *config_file*] [`-h` *history_file*] [`-d` *depth*] [`-f` *filter*] [*directory*|*archive*]...

# DESCRIPTION

**ytnova** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is inspired by the DOS file manager **XTree**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

It began from Ytree v2.10 but now continues as a separate Unix-like XTreeGold tribute, using contemporary POSIX/C99 code and libraries such as `libarchive`.

If no command line arguments are provided, the current directory will be logged.

# OPTIONS

*   **-d** *depth*: Override the default scan depth (TREEDEPTH). Supports numeric values or keywords: **min**/**root** (0), **max**/**all** (100).
*   **-f** *filter*: Specify an initial file filter (filespec) on startup. Supports patterns (e.g., `*.c`), exclusions (`-*.o`), and combinations (e.g., `*.c,*.h`). Use quotes to prevent shell expansion (e.g., `ytnova -f "*.c"`).
*   **-h** *history_file*: Use *history_file* instead of the default `~/.ytnova-hst`.
*   **--init**: Create missing starter profile, commands, and theme files and exit. By default this creates `~/.config/ytnova/ytnova.conf`, `~/.config/ytnova/commands.conf`, and `~/.config/ytnova/themes.conf` only if they do not already exist, and falls back to the home-dotfile paths only when the XDG target cannot be used. Use `-p` to target a different profile file.
*   **-p** *config_file*: Use *config_file* instead of the default `~/.config/ytnova/ytnova.conf`.
*   **-v**, **-V**, **--version**: Print ytnova version information and exit.
*   *directory*|*archive*: One or more directories or archive files to log on startup. If multiple paths are provided, they are all loaded as separate volumes. The first path specified becomes the active view.

# CONCEPTS

### The Display
The screen is divided into three panes plus a footer keybinding line: the **Directory Tree** (upper-left), the **File Window** (below the tree), and the **Statistics/Info** pane (right, spanning both left panes). The footer shows context-sensitive keybinding hints.

### Logging
Unlike file managers that rescan directories on demand, ytnova "logs" (scans) directory structures into memory. This allows instant navigation and searching without disk lag. Use the **l** command to log new paths or archives.

### Auto-Refresh
ytnova monitors the **currently selected directory** for changes (created/deleted/modified files) and updates the file list automatically.

**Note:** This monitoring is **active only for the current directory**. Changes occurring in parent or sibling directories while they are not selected will not be detected automatically. Use **^L** (Reload) or **F5** to refresh the view when navigating back to previously modified areas. Additionally, auto-refresh relies on kernel notifications. It may not function on network shares (NFS, SMB) or non-native mounts (e.g., WSL Windows drives) where the operating system does not propagate change events.

# MODES AND NAVIGATION

### Help System

YtreeNova keeps `F1` short and task-local. Use the contextual page for the
active surface, then follow shared explainer links only when you need more
background.

See also: Navigation, Shared commands, F10 config, Command-line editing.
### Navigation

This page keeps help-popup navigation distinct from ordinary YtreeNova
navigation. Learn the shared popup keys once here, then return to the active
context page for tree-only or file-only movement.
Arrow keys, paging keys, `Home`, `End`, and `Enter` keep their usual
ownership.

See also: Directory mode, File mode, Shared commands, F7 preview, F8 split.
### Directory Mode

Directory Help keeps the focus on directory commands plus tree-only navigation.
Use the shared Navigation page for popup controls and generic movement you only
need to learn once.

See also: Navigation, Shared commands.
### File Mode

File Help keeps the focus on file commands plus file-window navigation. Use the
shared Navigation page for popup controls and generic movement you only need to
learn once.

See also: Navigation, Output, Shared commands.
### Archive-Dir Mode

Archive directory help mirrors the live archive-directory footer, then adds the
archive-specific caveats that differ from normal filesystem directory behavior.

See also: Navigation, Directory mode, Output.
### Archive-File Mode

Archive file help mirrors the live archive-file footer and documents the
differences between archive file actions and normal filesystem file actions.

See also: Navigation, File mode, Output.
### Showall Mode

Showall help explains the single-volume aggregated file view and the commands
or caveats that differ from ordinary file mode.
Press `Esc` to return to the previously selected directory.
Press `\\` to jump to the owner directory of the selected file inside the
current logged volume.

See also: Navigation, File mode, Global.
### Global Mode

Global help explains the multi-volume aggregated file view, including how it
returns to owner directories and how its scope differs from ordinary file mode.
Press `Esc` to return to the previously selected directory.
Press `\\` to jump to the owner directory of the selected file even when that
owner lives under a different logged volume root.

See also: Navigation, File mode, Showall.
### File Preview Mode

F7 help explains preview ownership and how the preview overlay interacts with
the underlying directory or file context.
Use `Shift+Up/Down` or `^P/^N` to scroll preview contents line by line.
Use `Shift+PgUp/PgDn` for pages and `Shift+Home/End` to jump to the top or
bottom of the current preview.

See also: Navigation, File mode.
### Split Screen Mode

F8 help explains split-view ownership, inactive-panel defaults, and the keys
or caveats that only appear while split mode is active.
Press `Tab` to switch the active panel while leaving the passive panel's state
intact.
Copy, move, and compare prompts default to the inactive panel as the
destination/target while split mode is active.

See also: Navigation, Directory mode, File mode.
# KEY BINDINGS

**Note:** All keys are case insensitive unless otherwise noted. The symbol `^` denotes the **CTRL** key. For most commands, pressing **^key** (indicated in footer menus only where different) applies the action to all **tagged** files in the current scope. The live footer stays low-noise: there is no held-`Ctrl` footer variant, and Ctrl-only tagged/search semantics are explained in the active prompt/**F1** help instead of being shown all the time.

### Global Commands
These commands work in most modes:

*   **F1**: Help. Opens a context-sensitive popup for the active runtime surface: directory/file/archive views, Showall/Global lists, `F7` preview, split-panel targeting notes, picker dialogs, and prompt-specific syntax such as `{}` placeholders or tagged-flow semantics.
*   **F5**: Refresh (same as **^L**).
*   **F6**: Toggle the stats panel itself on and off. This does not change the current file or directory view selection.
*   **F7**: Toggle File Preview Pane.
*   **F8**: Toggle Split Screen Mode.
*   **F9**: Open the Applications menu shell. This picker-themed surface is the home for user-configurable external app presets/commands; in the current alpha it is primarily a visible shell while preset execution/reporting is still in progress.
*   **F10**: Open the configuration command surface: `(C)onfig  co(M)mands  (T)hemes  (R)eload  (Esc)/(Q)uit`. Press **Enter** or **C** to edit the main config, **M** to edit `commands.conf`, **T** to edit themes, or **R** to reload the current config/theme/commands set. The commands path owns preset selection plus local command overrides; packaged command presets stay read-only shared data. A successful reload silently repaints; a failed reload keeps the previous working config/theme/commands state and reports the error in the status/footer area.
*   **/**: **Incremental Jump** (List Jump). Start typing to jump to the first matching entry in the current list (directory names in the Directory Window, filenames in the File Window). The selection updates immediately as you type. Press **Enter** to accept the current match, or **Esc** to cancel and restore the original selection.
*   **\**: In **Showall**/**Global** file lists, exit that mode and jump to the selected file in its owner directory. In Archive-Dir mode, `\` jumps to archive root when used below root, and exits to the parent physical directory when used at archive root. In normal filesystem dir/file windows and Archive-File mode, `\` is a no-op.
*   **1 .. 9**: File or directory info band for the active panel (disabled in `F7` preview).
    *   In tree/directory focus the footer shows `1..9 dir view`.
    *   In file focus the footer shows `1..9 file view`.
    *   The current file/directory stats section shows the active view by name (for example `View: Name`, `View: Compact`, or `View: Git`).
    *   `1`: Name only. This is the plain default/baseline view, startup always begins here, and pressing `1` also resets temporary compact/overlay state back to Name.
    *   `2`: Attributes, including `name -> target` symlink rows in file projections.
    *   `3`: Owner.
    *   `4`: Times.
    *   By default, `1..4` are shared per panel, so changing the tree/directory view also changes the file-window view for that panel.
    *   Selecting `1..4` returns that file projection to its named base view and clears temporary extra view state there.
    *   Pressing the already-active `2`, `3`, or `4` again resets that context back to `1` / Name.
    *   Set `SEPARATE_DIR_FILE_VIEWS=1` to make tree/directory and file-window `1..4` views independent again.
    *   `5`: Toggle the compact Name/full-width file rendering variant when the current `1` / Name base view is active.
    *   `6`: Toggle binary vs human-readable size units for directory/file rows only. Stats stay human-readable.
    *   `7`: Toggle Mini preview detail (start of readable file contents on every visible file row). This leaves Compact so the detail is visible.
    *   `8`: Toggle File detail (`file`-style type-summary text on every visible file row). This leaves Compact so the detail is visible.
    *   `9`: Toggle the Git status band in filesystem file lists when the current directory is inside a Git worktree.
    *   `5` only works from the current `1` / Name base view; it always uses the Name file projection and is a silent no-op from `2`, `3`, or `4`.
    *   `5`, `7`, `8`, and `9` do not change tree rows; they change the panel's file projection instead, so in tree focus they update the small file window and in file focus they update the file window.
    *   Extra view states do not stack in the stats label; it names the one visible active state (`Compact`, `Mini preview`, `File`, or `Git`).
    *   `0`: Currently unused; silent no-op.
*   **^L**: **Reload**. Re-read the contents of the current directory from disk and refresh the view.
*   **K**: **Volume Menu**. Show a list of all currently logged volumes (drives/paths). Select a volume to switch context instantly. Selecting the already-active volume preserves its current in-memory state (no implicit relog). Press `Delete` (or `D`) in the menu to release (unlog) a volume. *(With `VI_KEYS=1`, use uppercase `K`; lowercase `k` is navigation.)*
*   **<** / **>** (or **,** / **.**): **Cycle Volumes**. Switch to the previous or next logged volume instantly.
*   **^Q**: **Quit to Directory**. If you exit ytnova with ^Q, the last selected directory becomes your current working directory. See shell wrapper function below.
*   **Q**: **Quit**. Exit ytnova.

### VI Keys Mode (Profile Option)
When `VI_KEYS=1` in `[GLOBAL]`, ytnova reserves lowercase vi navigation keys:
`h/j/k/l` and `^D/^U` (page down/up). To avoid collisions:

*   Use **H/L/K/J** for **Hex/Log/Volume Menu/Compare**.
*   In file-view contexts, use **D** for **Delete Tagged** and **U** for
    **Untag All**.
*   Lowercase **d/u** keep the regular context action (single item / current
    scope untag).

### Shared Commands

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
### Directory Mode

#### Directory navigation
*   **Enter**: Open the file window, or log/reveal one level when the selected
    directory is still unlogged. Logged directories switch to File Mode.
*   **Collapse**: Collapse or release the selected directory. `-` first
    collapses an expanded node; pressing it again on a collapsed logged node
    evicts the file list and marks that node unlogged.
*   **Tree marker**: Show logged state in the left margin. Unlogged
    directories use `+`; directory names themselves do not gain a `+` suffix.
*   **Left Arrow**: Collapse the current node or move selection to its parent.
    At filesystem root, Left is a no-op.
*   **Right Arrow**: Expand one level or move to the first child. It does not
    jump across siblings.
*   **Plus**: Log or reveal one level without moving the cursor. `=` is the
    unshifted alias on many keyboards.
*   **Asterisk**: Recursively expand the selected directory and its
    subdirectories.

#### Directory commands
*   **1..9 view**: Select the active panel's base directory and file view. `1`
    resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times,
    `5`, `7`, `8`, and `9` change the file projection, `6` toggles panel-wide
    row size units, and `9` is a silent no-op outside Git worktrees.
*   **Attributes**: Open the attributes submenu. Change mode (chmod), owner,
    group, or date.
*   **Copy**: Copy the selected directory branch.
*   **Delete**: Delete the selected directory.
*   **Filter**: Set file filter. Supports patterns such as `*.c`, exclusions
    such as `-*.o`, attributes such as `:r` and `:x`, dates such as
    `>2023-01-01`, and sizes such as `>1M`.
*   **Global**: Show all files across all logged volumes in one list.
*   **Invert Tags**: Toggle tag state for the current directory scope.
*   **Compare**: Open the compare submenu. Choose directory, logged-tree, or
    external-viewer compare modes. With `VI_KEYS=1`, use uppercase `J`.
*   **Volume**: Open the volume picker.
*   **Log**: Log a new directory or archive file. Logging an already logged
    path performs a fresh reload and reanchors selection at the volume root.
*   **Makedir**: Create a new directory.
*   **New File**: Create a new empty file.
*   **Only tagged**: Toggle tagged-only view for the current directory scope.
*   **Pipe**: Pipe the selected directory to a command on stdin. `|` is the
    alternate key.
*   **Quit**: Quit ytnova.
*   **Rename**: Rename the selected directory.
*   **Showall**: Show all files in all directories of the current volume.
*   **Tag**: Tag all files in the selected directory.
*   **Untag**: Untag all files in the selected directory.
*   **MoveDir**: Move the selected directory branch.
*   **Write**: Export files in the selected directory to a command or file.
    The formatter dialog offers Raw, Framed, and Page Break output.
*   **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand
    it to the current directory path and shell-quote the result. Prompt `F1`
    also explains the tagged-file `^X` repeat path.
*   **Archive**: Create an archive from the current selection. Tagged files win;
    otherwise ytnova archives the selected file or directory recursively.
    Supported suffixes are `.tar`, `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`,
    `.tar.xz`/`.txz`, and `.zip`.
*   **Jump**: Jump to a file or directory name in the current list.
*   **Dotfiles**: Toggle hidden dot-files and dot-directories.
### File Mode

#### File-window navigation
*   **1..9 view**: Select the active panel's file view. `1` resets to Name,
    `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles
    Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles
    File detail, and `9` toggles the Git band inside Git worktrees.
*   **Enter**: Switch between the file window and full-screen file mode.
*   **Left Arrow**: Move to the previous visible file column. In one-column
    layouts, Left performs page-up navigation.
*   **Right Arrow**: Move to the next visible file column. In one-column
    layouts, Right performs page-down navigation.
*   **Date changes**: Date actions change Accessed time, Modified time, or
    both. POSIX does not offer creation/birth time updates here.

#### File commands
*   **Attributes**: Open the file attributes submenu. Change mode, owner,
    group, or date.
*   **Copy**: Copy the selected file.
*   **Pathcopy**: Copy the selected file while preserving its path relative to
    the current volume root.
*   **Copy tagged**: Copy all tagged files.
*   **Delete**: Delete the selected file. With `VI_KEYS=1`, lowercase `d`
    keeps this action and uppercase `D` becomes Delete Tagged.
*   **Edit**: Edit the selected file with `$EDITOR`. The default editor is
    `vi`.
*   **Filter**: Set file filter.
*   **Hex**: View the selected file in hex mode.
*   **Invert Tags**: Toggle the tag state of all visible files.
*   **Compare**: Compare the selected file with a target file.
*   **Volume**: Open the volume picker.
*   **Log**: Log a new directory or archive file. Logging an already logged
    path performs a fresh reload and reanchors selection at the volume root.
*   **Move**: Move the selected file.
*   **Move tagged**: Move all tagged files.
*   **New File**: Create a new empty file.
*   **Only tagged**: Toggle tagged-only file view.
*   **Pipe**: Pipe the selected file to a command on stdin. `|` is the
    alternate key.
*   **Rename**: Rename the selected file.
*   **Sort**: Sort the file list. Choose Access time, Change time, Extension,
    Group, Modification time, Name, Owner, or Size.
*   **Search tagged**: Search tagged files with grep. The prompt expects plain
    search text, builds `grep -i -- PATTERN {}` internally, and untags files
    that do not match.
*   **Tag**: Tag the selected file.
*   **Tag all**: Tag all displayed files.
*   **Untag**: Untag the selected file. With `VI_KEYS=1`, lowercase `u` keeps
    this action.
*   **Untag all**: Untag all displayed files. With `VI_KEYS=1`, `^U` stays
    page-up navigation and uppercase `U` becomes Untag All.
*   **View**: View the selected file with the configured pager. The default is
    View file with the pager defined in the main config. The default is
    `less`.
*   **View tagged**: View all tagged files sequentially.
*   **Write**: Export the selected file to a command or file. The formatter
    dialog offers Raw, Framed, and Page Break output.
*   **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand
    it to the selected file path and shell-quote the result. Prompt `F1` also
    explains the tagged-file `^X` repeat path.
*   **Archive**: Create an archive from tagged files, or from the selected
    file/directory when nothing is tagged. Directory sources are archived
    recursively.
### Archive-Dir Mode

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
### Archive-File Mode

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
# COMPARE

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

# COMMAND LINE EDITING

### Line Editing Keys

Input prompts support standard text-editing shortcuts:

*   **^A / Home**: Start of line.
*   **^E / End**: End of line.
*   **^K**: Delete to end of line.
*   **^U**: Delete to start of line.
*   **^W**: Delete word left.
*   **^D / Del**: Delete character.
*   **^H / Backspace**: Backspace.

### Prompt Navigation Keys

These keys apply while prompt dialogs are active (for example: Log, Copy, Move).

*   **Up Arrow**: History (with `P` to Pin, `D` to Delete).
*   **F2**: Directory picker for path-entry prompts.
*   **Missing copy/move destinations**: If the resolved destination directory does not exist, ytnova shows `Create missing directory? (y/N)` with the full target path. `y` creates it before the operation continues; `N`/`Esc` returns to the destination prompt without mutating the filesystem.

### Filter Help

#### Filter syntax
Use normal glob-like patterns such as `*.c`, comma-separated unions such as
`*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as
attributes (`:r`, `:x`), dates (`>2023-01-01`), or sizes (`>1M`). If the
shell would expand the pattern, quote it before launching ytnova.

#### Scope rules
Filter prompts stay scoped to the active file-list family. Directory/File,
archive, Showall, and Global contexts may share syntax while still applying the
result to their own current scope and tagged/untagged conventions.
### Output Help

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
# SUPPORT TOPICS

### Command-line Editing

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
### VI Keys

#### Navigation remap
With `VI_KEYS=1`, lowercase **h/j/k/l** become Left/Down/Up/Right and **^U**
/**^D** become page-up/page-down.

#### Command collisions
Commands that would collide with lowercase vi navigation move to uppercase or a
non-conflicting fallback. Examples include **J** for Compare, **K** for Volume
Menu, **D** for Delete Tagged, and **U** for Untag All where applicable.
### F10 Config

#### Config surface
Use **F10** to reach configuration-oriented commands instead of treating them
as per-directory actions. Persistent changes belong here, not in the active
file or directory command pages.

#### Related areas
Theme selection, semantic colors, and presentation tweaks are covered by
**Theming**. Prompt-edit/history behavior that appears inside config flows is
still owned by **Command-line Editing**.
### Theming

#### Theme model
Themes are role-based: users configure semantic roles rather than styling each
surface with ad-hoc colors. Help popups, pickers, and the footer command strip
each have their own dedicated roles.

#### Editing path
Use **F10** and the theme/config files to change theme selection or role
definitions. Keep contrasts readable for help, picker, and selection surfaces;
those are high-frequency navigation aids.
# CONFIGURATION

ytnova reads its main configuration from `~/.config/ytnova/ytnova.conf` by
default. The home-directory fallback path is `~/.ytnova` when the XDG target
cannot be used. Passing `-p` *config_file*
uses that explicit main config path instead.

Use `ytnova --init` to create the preferred main config when it is missing.
Existing files are never overwritten by `--init`.
Example: `ytnova --init`

The file created by `--init` is a fully annotated profile template. It selects
the default semantic theme; role definitions and file-type palette rules live
in theme files, not in the main config.

Theme catalogs are plain text. ytnova loads user themes from
`$XDG_CONFIG_HOME/ytnova/themes.conf` or `~/.config/ytnova/themes.conf`, falls
back to `~/.ytnova.themes` only when the XDG-style target cannot be used, then
uses the installed packaged catalog or compiled-in defaults without creating a
user theme file. Run `ytnova --init` to bootstrap an editable starter catalog.

Theme roles use semantic names such as `dynamic_text`, `static_text`, `keybind`,
`footer`, `selection`, `dialog`, `picker`, `picker_selection`, `help`,
`help_link`, `help_link_selection`, `warning`, `error`, and `search_hit`.
`footer` owns the always-visible keybinding strip, while `help` owns the F1
reading surface. When `picker_selection` is omitted it falls back to
`selection`, so existing themes keep the same picker highlight behavior. The
bundled starter themes keep `picker` on a different background so F2,
history, volume, and applications menus stand out from the main content
background. Color values accept names or numbers, `grey`/`gray`, bright
prefixes such as `+white` or `+grey`, and optional backgrounds such as
`+white on blue`.

Theme-local file-type palettes use compact grouped rules, for example
`archives = red: tar,tgz,zip` or `scripts = +cyan: sh,bash,py`. Rules are
evaluated top to bottom; the first matching extension or special selector wins.
Special selectors may include `LINK` and `EXEC`; directory tree rows use theme
roles rather than file-type palette rules. When a rule omits a background, it
inherits the active filename/window background. Starter themes should also omit
redundant backgrounds on ordinary content roles when they are meant to follow
the theme background, so changing `background = ...` repaints the shared
surface intuitively.

Command customization lives in `commands.conf`. ytnova loads user command
overrides from `$XDG_CONFIG_HOME/ytnova/commands.conf` or
`~/.config/ytnova/commands.conf`, falls back to `~/.ytnova.commands` only when
the XDG-style target cannot be used, then uses the installed packaged active
command map or compiled-in defaults without creating a user command file.
`commands.conf` may optionally start with `preset = <id>` to select one
packaged read-only command preset before local per-action overrides are
applied. Packaged preset catalogs live under the shared app-data commands
directory (for example `/usr/share/ytnova/commands/<preset>.conf`); `F10`
edits only `commands.conf`, not the packaged preset files.

# QUIT TO DIRECTORY

To allow `^Q` to change your shell's working directory, add this shell wrapper function to your `~/.bashrc`. It also gives you a short `yt` command:

```bash
yt() {
    ytnova "$@"
    local tmpfile="$HOME/.ytnova-$$.chdir"
    if [ -f "$tmpfile" ]; then
        source "$tmpfile"
        rm "$tmpfile"
    fi
}
```

# FILES

*   `$XDG_CONFIG_HOME/ytnova/ytnova.conf` or `~/.config/ytnova/ytnova.conf`: Preferred main configuration file.
*   `$XDG_CONFIG_HOME/ytnova/commands.conf` or `~/.config/ytnova/commands.conf`: Preferred user command map and preset-selection file.
*   `$XDG_CONFIG_HOME/ytnova/themes.conf` or `~/.config/ytnova/themes.conf`: Preferred user theme catalog.
*   `$XDG_STATE_HOME/ytnova/ytnova.hst` or `~/.local/state/ytnova/ytnova.hst`: Preferred command history path.
*   `~/.ytnova`: Legacy fallback main configuration file.
*   `~/.ytnova.commands`: Legacy fallback user command map file.
*   `~/.ytnova.themes`: Legacy fallback user theme catalog.
*   `~/.ytnova-hst`: Legacy fallback command history path.
*   `/usr/share/ytnova/ytnova.commands`: Installed packaged active command map.
*   `/usr/share/ytnova/commands/*.conf`: Installed packaged read-only command presets.

### Reporting problems

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytreenova/issues).

It will help us to address the issue if you include the following:

*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **YtreeNova Version:**
*   **Steps to Reproduce:**
*   **Expected Behavior:**
*   **Actual Behavior:**

# AUTHORS

Authors and contributors are listed in the [AUTHORS.md](AUTHORS.md) file.

# SEE ALSO

**bash**(1), **glob**(7), **grep**(1), **less**(1), **regex**(7), **vi**(1)
