<!-- Auto-generated from etc/help/man.en.md by scripts/generate_help_assets.py; do not edit directly. -->

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

This manual is the fuller reference path for ytnova modes, commands, prompts, and support topics.
The in-app `F1` popup provides the shorter contextual version for the active surface.

See also: Navigation, Tagged, Shared commands, F2 picker, F10 config.
### Navigation

The help popup uses list-style navigation.
`Up` and `Down` move, `Enter` or `Right` follow, `Left` goes back, and `Esc` or `Q` closes.

See also: Directory mode, File mode, F7 preview, F8 split, F2 picker.
### Directory Mode

Directory mode is the logged tree view.
It owns directory navigation, tree expansion, and directory-scoped commands.

See also: Navigation, Shared commands.
### File Mode

File mode is the main file-list view.
It owns file navigation, file-scoped commands, tagged actions, and export entry points.

See also: Navigation, Tagged, Output.
### Archive-Dir Mode

Archive-Dir mode is the tree-style view inside a logged archive.
It mirrors directory work where the archive format permits it.

See also: Navigation, Directory mode.
### Archive-File Mode

Archive-File mode is the file-list view for archive-backed content.
Some filesystem commands are unavailable or become archive-aware here.

See also: Navigation, File mode, Tagged.
### Showall Mode

Showall lists every file inside the current logged volume in one aggregated file list.
It keeps single-volume scope while flattening directory boundaries.

See also: Navigation, File mode, Tagged.
### Global Mode

Global lists files from every logged volume in one aggregated file list.
It keeps multi-volume scope while flattening directory boundaries.

See also: Navigation, File mode, Tagged.
### File Preview Mode

F7 preview overlays file preview controls on top of the underlying file-selection context.
The preview owns scrolling while the underlying selection still owns the file target.

See also: Navigation, File mode, Applications menu.
### Split Screen Mode

Split mode keeps two panels active at once, and runtime F1 opens the directory or file split page for the active panel.
Use the split page for the live footer command list and this page for the shared split model.

See also: Navigation, Directory split page, File split page.
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

#### Shared function keys
* **F1**: Open contextual help for the active surface.
* **F5**: Refresh the current view.
* **F6**: Change the stats/details presentation for the active view.
* **F7**: Toggle preview for the active file context.
* **F8**: Toggle split-screen mode.
* **F9**: Open the Applications menu.
* **F10**: Open the configuration command surface.
* **Esc**: Back out of the current overlay, prompt, or popup.
### Directory Mode

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
### File Mode

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
### Archive-Dir Mode

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
### Archive-File Mode

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
# COMPARE

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
### Output Help

#### Output model
`Output` is an export flow.
It can write plain content, framed content, or page-break-separated content.
It can also send that output to a printer command instead of a file path.

#### Output order
Choose the format first.
Choose the separator only when the format asks for one.
Then choose the final destination.
# SUPPORT TOPICS

### Command-line Editing

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
### Copy/Move Targets

#### Target forms
Use one full replacement name when you want one selected item to land under a new explicit name.
Use a wildcard rename pattern such as `*.bak` or `copy-*` when you want ytnova to rewrite each selected basename by pattern.
Use the second `To Directory:` prompt when you want to choose where the result lands, whether by typing a path directly or by using `F2` browse/history there.

#### Shared rules
Tagged copy/move and pathcopy use the same two-step target flow as single-item copy/move.
Split mode may seed the inactive-panel directory as the default `To Directory:` value, but you can still replace that default before the operation starts.
Archive-backed copy/move keeps the same destination prompt model even when extraction or archive-aware paths are involved.
### List Jump

#### Jump model
`/` opens an incremental jump prompt for the current visible list only.
Tree/directory views jump among visible directory names, while file-oriented views jump among the visible file rows for that surface.

#### Acceptance and cancel
* **Type text**: Move immediately to the best current match as you type.
* **Enter**: Keep the current match and stay there.
* **Esc**: Cancel the jump and restore the original selection.
* **Scope changes**: Filtering, Showall/Global projection, archives, and split mode all change which visible list `/` searches, but they do not change the jump keys themselves.
### VI Keys

#### Navigation remap
With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.
`Ctrl-U` and `Ctrl-D` become page up and page down.

#### Command collisions
Commands that would steal those lowercase keys move out of the way.
Examples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist.
### F10 Config

#### Config surface
Use `F10` when you want to change persistent behavior instead of doing one one-off file or directory action.
Profile settings, command labels, themes, and reload all live here.

#### Related files
`ytnova.conf` owns profile settings.
`commands.conf` owns user command labels and bindings.
`themes.conf` owns theme selection and theme-role overrides.
### Theming

#### Theme model
Themes set semantic roles such as `footer`, `help`, `help_link`, `selection`, `picker`, and `warning`.
This keeps one theme change consistent across the whole UI.

#### Editing path
Use `F10` to open the theme or config editing path.
Keep high-frequency navigation surfaces readable first: selection, picker, footer, and help.
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
`help_keybind`, `help_link`, `help_link_selection`, `help_box_lines`,
`warning`, `error`, and `search_hit`. `footer` owns the always-visible
keybinding strip, while `help` owns the F1 reading surface. `help_keybind`
owns help-popup mnemonic emphasis; when it is omitted, runtime falls back to
`keybind` on the `help` background. `help_box_lines` owns the F1 popup frame;
when it is omitted, runtime inherits the `help` foreground and background. When
`picker_selection` is omitted it falls back to
`selection`, so existing themes keep the same picker highlight behavior. The
bundled starter themes keep `picker` on a different background so F2,
history, volume, and applications menus stand out from the main content
background. Color values accept names or numbers, `grey`/`gray`, bright
prefixes such as `+white`, and optional backgrounds such as `+white on
blue`. `+grey`/`+gray` is accepted syntax but currently renders as
`white`, so prefer `white` when you mean the rendered color.

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
