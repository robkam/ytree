---
title: YTREE
section: 1
header: "User Commands"
footer: "ytree 3.0.0"
date: "January 2026"
---

# NAME

ytree - A File Manager for UNIX-like systems

# SYNOPSIS

`ytree` [`--init`] [`-v`|`-V`|`--version`] [`-p` *config_file*] [`-h` *history_file*] [`-d` *depth*] [`-f` *filter*] [*directory*|*archive*]...

# DESCRIPTION

**ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is inspired by the DOS file manager **XTree&trade;**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

This version is a modern refactor of the original **ytree** by Werner Bregulla, incorporating features from XTree&trade; and ZTreeWin, using modern C standards (C99/POSIX), and integrating robust libraries like `libarchive`.

If no command line arguments are provided, the current directory will be logged.

# OPTIONS

*   **-d** *depth*: Override the default scan depth (TREEDEPTH). Supports numeric values or keywords: **min**/**root** (0), **max**/**all** (100).
*   **-f** *filter*: Specify an initial file filter (filespec) on startup. Supports patterns (e.g., `*.c`), exclusions (`-*.o`), and combinations (e.g., `*.c,*.h`). Use quotes to prevent shell expansion (e.g., `ytree -f "*.c"`).
*   **-h** *history_file*: Use *history_file* instead of the default `~/.ytree-hst`.
*   **--init**: Create a starter profile file and exit. By default this creates `~/.ytree` only if it does not already exist. Use `-p` to target a different file.
*   **-p** *config_file*: Use *config_file* instead of the default `~/.ytree`.
*   **-v**, **-V**, **--version**: Print ytree version information and exit.
*   *directory*|*archive*: One or more directories or archive files to log on startup. If multiple paths are provided, they are all loaded as separate volumes. The first path specified becomes the active view.

# CONCEPTS

### The Display
The screen is divided into three areas: The **Directory Tree** (left), the **File Window** (below), and the **Statistics/Info** pane (right).

### Logging
Unlike standard `ls`-based managers, ytree "logs" (scans) directory structures into memory. This allows for instant navigation and searching without disk lag. Use the **l** command to log new paths or archives.

### Auto-Refresh
ytree monitors the **currently selected directory** for changes (created/deleted/modified files) and updates the file list automatically.

**Note:** This monitoring is **active only for the current directory**. Changes occurring in parent or sibling directories while they are not selected will not be detected automatically. Use **^L** (Reload) or **F5** to refresh the view when navigating back to previously modified areas. Additionally, auto-refresh relies on kernel notifications. It may not function on network shares (NFS, SMB) or non-native mounts (e.g., WSL Windows drives) where the operating system does not propagate change events.

# MODES AND NAVIGATION

**ytree** operates in distinct modes. The active mode determines available commands.

**Directory Mode**
Focus is on the directory hierarchy tree. Navigation keys allow moving between folders. Typing alphanumeric characters triggers a directory mode command (key bindings).
*   **Action:** Press **Return** to switch focus to the **File Mode** (file list) for the selected directory.

**File Mode**
Focus is on the file list of the selected directory. Operations here affect specific files. Typing alphanumeric characters triggers a file mode command (key bindings).
*   **Action:** Press **Return** to toggle the file window to full-screen. Press **Return** again to restore the split view or switch back to **Directory Mode**.

**Archive Mode**
When entering a supported archive (ZIP, TAR, GZ), ytree treats it as a virtual filesystem with archive-aware write operations (copy/move/delete/rename/mkdir paths where supported). It behaves similarly to Directory/File modes with archive-specific navigation and safety semantics.

**Split Screen Mode**
Activated by **F8**. The screen is divided vertically into two independent file manager panels.
*   **Switch Focus:** Press **TAB** to switch active control between the Left and Right panels.
*   **Targeting:** Operations like **Copy** and **Move** automatically default to the path of the inactive (passive) panel as the destination.

**File Preview Mode**
Activated by **F7**. The screen layout changes to show the file list on the left (or active panel) and the file contents on the right.
*   **Navigate File List:** Use **Up/Down**, **Page Up/Down**, **Home/End** to move the selection in the file list. The preview pane updates immediately.
*   **Scroll Preview:** Use **Shift+Up/Down** (or **^P** / **^N**) to scroll the content of the preview window line by line. Use **Shift+Page Up/Down** to scroll by pages. **Shift+Home/End** jumps to the beginning or end of the file.

# KEY BINDINGS

**Note:** All keys are case insensitive unless otherwise noted. The symbol `^` denotes the **CTRL** key.

### Global Commands
These commands work in most modes:

*   **F1**: Help (context-sensitive in prompts/dialogs).
*   **F5**: Refresh (same as **^L**).
*   **F6**: Toggle Statistics Panel (Wide Mode).
*   **F7**: Toggle File Preview Panel.
*   **F8**: Toggle Split Screen Mode.
*   **F9**: Application Menu (**reserved**, not implemented yet).
*   **F10**: Config (**reserved**, not implemented yet).
*   **/** (or **F12**): **Incremental Jump** (List Jump). Start typing to jump to the first matching entry in the current list (directory names in the Directory Window, filenames in the File Window). The selection updates immediately as you type. Press **Enter** to accept the current match, or **Esc** to cancel and restore the original selection.
*   **\\**: In **Showall**/**Global** file lists, exit that mode and jump to the selected file in its owner directory. In Archive-Dir mode, `\\` jumps to archive root when used below root, and exits to the parent physical directory when used at archive root. In normal filesystem dir/file windows and Archive-File mode, `\\` is a no-op.
*   **B**: Toggle Brief (Compact) filename view in the File Window.
*   **^L**: **Reload**. Re-read the contents of the current directory from disk and refresh the view.
*   **K** (Shift+K): **Volume Menu**. Show a list of all currently logged volumes (drives/paths). Select a volume to switch context instantly. Press `Delete` (or `D`) in the menu to release (unlog) a volume.
*   **<** / **>** (or **,** / **.**): **Cycle Volumes**. Switch to the previous or next logged volume instantly.
*   **^Q**: **Quit to Directory**. If you exit ytree with ^Q, the last selected directory becomes your current working directory. *Note: This requires a shell wrapper function.*
*   **Q**: **Quit**. Exit ytree.

### VI Keys Mode (Profile Option)
When `VI_KEYS=1` in `[GLOBAL]`, ytree reserves lowercase vi navigation keys:
`h/j/k/l` and `^D/^U` (page down/up). To avoid collisions:

*   Use **H/L/K** for **Hex/Log/Volume Menu**.
*   In file-view contexts, use **D** for **Delete Tagged** and **U** for
    **Untag All**.
*   Lowercase **d/u** keep the regular context action (single item / current
    scope untag).

### Directory Mode
Active when browsing the directory tree window.

*   **A** (Attributes): Open attributes submenu for directory metadata changes:
    mode (chmod), owner, group, date.
*   **C** (Compare): Open the compare submenu (directory, logged tree, or external viewer).
*   **D** (Delete): Delete selected directory.
*   **F** (Filter): Set file filter. Supports regex patterns (e.g., `*.c`), exclusions (`-*.o`), attributes (`:r`, `:x`), dates (`>2023-01-01`), and sizes (`>1M`).
*   **G** (Global): Show all files across all logged volumes in one global list.
*   **L** (Log): Log a new directory or archive file.
*   **M** (Makedir): Create a new directory.
*   **N** (New File): Create a new empty file.
*   **O** (Archive): Create an archive from the current selection. If one or more files are tagged, ytree archives the tagged files. If nothing is tagged, ytree archives the selected file or selected directory. Directory sources are archived recursively. Supported destination suffixes: `.tar`, `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`, `.tar.xz`/`.txz`, `.zip`.
*   **R** (Rename): Rename selected directory.
*   **S** (Showall): Show all files in all directories of the current volume.
*   **T** (Tag): Tag all files in the selected directory.
*   **U** (Untag): Untag all files in the selected directory.
*   **X** (eXecute): Execute a shell command. The `{}` placeholder is replaced by the current directory path.
*   **`** (Backtick): Toggle visibility of hidden dot-files and directories.
*   **^F** (Dir Mode): Cycle directory display modes (Filenames only -> Attributes -> Inode/Owner -> Times).
*   **Return**: Switch to File Mode (focus the file window).
*   **\*** (Asterisk): Expand the current directory and all its subdirectories.

### File Mode
Active when the file window is focused.

*   **A** (Attributes): Open attributes submenu for selected file metadata:
    mode, owner, group, date.
*   **^A**: Open attributes submenu for all tagged files:
    mode, owner, group, date.
*   **C** (Copy): Copy the selected file.
*   **^K**: Copy all tagged files.
*   **D** (Delete): Delete selected file. *(With `VI_KEYS=1`, use lowercase `d`
    for this action.)*
*   **^D**: Delete all tagged files. *(With `VI_KEYS=1`, `^D` is page-down
    navigation and uppercase `D` becomes Delete Tagged.)*
*   **E** (Edit): Edit selected file with `$EDITOR` (default: vi).
*   **F** (Filter): Set file filter.
*   **H** (Hex): View selected file in hex mode.
*   **J** (Compare): Compare the selected file with a target file.
*   **L** (Log): Log a new directory or archive file.
*   **M** (Move): Move the selected file.
*   **N** (New File): Create a new empty file.
*   **^N**: Move all tagged files.
*   **O** (Archive): Create an archive from tagged files, or from the selected file/directory when nothing is tagged. Directory sources are archived recursively. Supported destination suffixes: `.tar`, `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`, `.tar.xz`/`.txz`, `.zip`.
*   **P** (Pipe): Pipe content of file to a command (stdin).
*   **^P**: Pipe content of all tagged files to a command.
*   **R** (Rename): Rename the selected file.
*   **^R**: Rename all tagged files.
*   **S** (Sort): Sort filelist (Access time, Change time, Extension, Group, Modification time, Name, Owner, Size).
*   **^S** (Search): Execute grep on tagged files. Untags files that do not match the command.
*   **Date Changes:** Date actions change Accessed time, Modified time, or both (POSIX does not allow setting creation/birth time here).
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all displayed files.
*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`
    for this action.)*
*   **^U**: Untag all displayed files. *(With `VI_KEYS=1`, `^U` is page-up
    navigation and uppercase `U` becomes Untag All.)*
*   **V** (View): View file with the pager defined in `~/.ytree` (default: less).
*   **^V**: **View Tagged**. View all tagged files sequentially. Mode is controlled by `TAGGEDVIEWER`: `external` (default, uses `$PAGER`) or `internal` (built-in viewer with in-app navigation).
*   **X** (eXecute): Execute a shell command. `{}` is replaced by the filename.
*   **^X**: Execute shell command for all tagged files. `{}` is replaced by the full path.
*   **Y**: (Pathcopy): Copy selected file, replicating its directory structure relative to the current volume root.
*   **^Y**: Copy all tagged files including path.
*   **^F** (File Mode): Cycle file display modes.
*   **Return**: Switch to Full Screen File Mode / Directory Mode.
*   **\*** (Asterisk) / **Shift-8**: **Invert Tags**. Toggle the tag state of all currently visible files.

### Archive Mode
When browsing an archive (ZIP, TAR, etc.), ytree behaves like a virtual file system with archive-aware operations and distinct root/non-root navigation rules.

**Archive-Dir Mode**

*   **C** (Compare): Open compare flow.
*   **D** (Delete): Delete selected archive directory entry.
*   **F** (Filter): Set file filter.
*   **G** (Global): Show all files across all logged volumes in one global list.
*   **L** (Log): Log a new directory or archive.
*   **M** (Makedir): Create directory in archive context where supported.
*   **R** (Rename): Rename selected archive directory entry.
*   **S** (Showall): Show all files in the archive.
*   **T** (Tag): Tag all files in current virtual directory.
*   **U** (Untag): Untag all files in current virtual directory.
*   **^F** (Dir Mode): Cycle display modes.
*   **Return**: Switch to Archive-File Mode.
*   **-**: State-based collapse/release. Expanded nodes collapse; collapsed logged nodes (or logged leaves) unlog/release.
*   **Left Arrow**: Collapse/navigation behavior only; does not perform archive-exit.
*   **\\**: At archive non-root, jump to archive root. At archive root, exit to parent physical directory.

**Archive-File Mode**

*   **C** (Copy): Copy selected file (including extract/copy paths).
*   **^K** (Copy Tagged): Copy all tagged files.
*   **D** (Delete): Delete selected archive file entry.
*   **F** (Filter): Set file filter.
*   **H** (Hex): View file in hex mode.
*   **M** (Move): Move selected file using archive-aware semantics.
*   **P** (Pipe): Pipe content to command.
*   **R** (Rename): Rename selected archive file entry.
*   **S** (Sort): Sort file list.
*   **^S** (Search): Search tagged files for a string. Untags files that do not match.
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all files.
*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`
    for this action.)*
*   **^U**: Untag all files. *(With `VI_KEYS=1`, `^U` is page-up navigation and
    uppercase `U` becomes Untag All.)*
*   **V** (View): View file.
*   **^V**: **View Tagged**. View all tagged files sequentially. Mode is controlled by `TAGGEDVIEWER`: `external` (default, uses `$PAGER`) or `internal` (built-in viewer with in-app navigation).
*   **Y** (Pathcopy): Copy selected file with relative path preservation.
*   **^F** (File Mode): Cycle display modes.
*   **Return**: Switch to Archive-Dir Mode.
*   **\\**: No-op.
*   **\*** (Asterisk): Invert tag selection.

Archive file-window status text:

*   `Unlogged`: selected directory is unlogged.
*   `No files`: selected directory is logged and empty.

# COMPARE

*   **File compare (`J` in File Mode):** Compare the selected file against a target file. ytree can use an external file-diff helper if configured.
*   **Directory compare (`C` in Directory Mode):**
    *   `D`: compare the current directory.
    *   `T`: compare the current logged tree.
    *   `X`: launch an external directory/tree compare viewer.
*   Internal compare tags matches on the active/source side only.
*   Logged-tree compare uses logged content only; it does not auto-log unopened subdirectories.
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

# CONFIGURATION

ytree reads configuration from `~/.ytree` by default, or from `-p` *config_file*
when provided.

Use `ytree --init` to create `~/.ytree` when it is missing. Existing files are
never overwritten by `--init`.
Example: `ytree --init`

The file created by `--init` is a fully annotated profile template.

# QUIT TO DIRECTORY

To allow `^Q` to change your shell's working directory, add this function to your `~/.bashrc`:

```bash
yt() {
    ytree "$@"
    local tmpfile="$HOME/.ytree-$$.chdir"
    if [ -f "$tmpfile" ]; then
        source "$tmpfile"
        rm "$tmpfile"
    fi
}
```

# FILES

*   `~/.ytree`: Configuration file.
*   `~/.ytree-hst`: Command line history.

# BUGS

To avoid problems with escape sequences on RS/6000 machines (telnet/rlogin), set `ESCDELAY`:
```bash
export ESCDELAY=1000
```

### Reporting problems

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytree/issues).

It will help us to address the issue if you include the following:

*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **Ytree Version:**
*   **Steps to Reproduce:**
*   **Expected Behavior:**
*   **Actual Behavior:**

# AUTHORS

Authors and contributors are listed in the [AUTHORS.md](AUTHORS.md) file.

# SEE ALSO

**glob**(7), **grep**(1), **less**(1), **regex**(7), **vi**(1)
