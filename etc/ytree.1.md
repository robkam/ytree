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

`ytree` [`-p` *config_file*] [`-h` *history_file*] [`-d` *depth*] [*directory*|*archive*]...

# DESCRIPTION

**ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is inspired by the DOS file manager **XTree&trade;**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

This version is a modern refactor of the original **ytree** by Werner Bregulla, incorporating features from XTree&trade; and ZTreeWin, using modern C standards (C99/POSIX), and integrating robust libraries like `libarchive`.

If no command line arguments are provided, the current directory will be logged.

# OPTIONS

*   **-d** *depth*: Override the default scan depth (TREEDEPTH). Supports numeric values or keywords: **min**/**root** (0), **max**/**all** (100).
*   **-f** *filter*: Specify an initial file filter (filespec) on startup. Supports patterns (e.g., `*.c`), exclusions (`-*.o`), and combinations (e.g., `*.c,*.h`). Use quotes to prevent shell expansion (e.g., `ytree -f "*.c"`).
*   **-h** *history_file*: Use *history_file* instead of the default `~/.ytree-hst`.
*   **-p** *config_file*: Use *config_file* instead of the default `~/.ytree`.
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
When entering a supported archive (ZIP, TAR, GZ), ytree treats it as a read-only virtual filesystem. It behaves similarly to Directory/File modes but with a restricted command set. Typing alphanumeric characters triggers an archive mode command.

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

*   **F1**: Help (**reserved**, not implemented yet).
*   **F3**: Config (**reserved**, not implemented yet).
*   **F5**: Refresh (same as **^L**).
*   **F6**: Toggle Statistics Panel (Wide Mode).
*   **F7**: Toggle File Preview Panel.
*   **F8**: Toggle Split Screen Mode.
*   **F9**: Application Menu (**reserved**, not implemented yet).
*   **/** (or **F12**): **Incremental Jump** (List Jump). Start typing to jump to the first matching entry in the current list (directory names in the Directory Window, filenames in the File Window). The selection updates immediately as you type. Press **Enter** to accept the current match, or **Esc** to cancel and restore the original selection.
*   **\\**: In **Showall**/**Global** file lists, exit that mode and jump to the selected file in its owner directory.
*   **B**: Toggle Brief (Compact) filename view in the File Window.
*   **^L**: **Reload**. Re-read the contents of the current directory from disk and refresh the view.
*   **K** (Shift+K): **Volume Menu**. Show a list of all currently logged volumes (drives/paths). Select a volume to switch context instantly. Press `Delete` (or `D`) in the menu to release (unlog) a volume.
*   **<** / **>** (or **,** / **.**): **Cycle Volumes**. Switch to the previous or next logged volume instantly.
*   **^Q**: **Quit to Directory**. If you exit ytree with ^Q, the last selected directory becomes your current working directory. *Note: This requires a shell wrapper function.*
*   **Q**: **Quit**. Exit ytree.
*   **Up Arrow**: History (with `P` to Pin, `D` to Delete).

### Directory Mode
Active when browsing the directory tree window.

*   **A** (Attributes): Open attributes submenu for directory metadata changes:
    mode (chmod), owner, group, date.
*   **D** (Delete): Delete selected directory.
*   **F** (Filter): Set file filter. Supports regex patterns (e.g., `*.c`), exclusions (`-*.o`), attributes (`:r`, `:x`), dates (`>2023-01-01`), and sizes (`>1M`).
*   **G** (Global): Show all files across all logged volumes in one global list.
*   **L** (Log): Log a new directory or archive file.
*   **M** (Makedir): Create a new directory.
*   **N** (New File): Create a new empty file.
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
*   **D** (Delete): Delete selected file.
*   **^D**: Delete all tagged files.
*   **E** (Edit): Edit selected file with `$EDITOR` (default: vi).
*   **F** (Filter): Set file filter.
*   **H** (Hex): View selected file in hex mode.
*   **L** (Log): Log a new directory or archive file.
*   **M** (Move): Move the selected file.
*   **N** (New File): Create a new empty file.
*   **^N**: Move all tagged files.
*   **P** (Pipe): Pipe content of file to a command (stdin).
*   **^P**: Pipe content of all tagged files to a command.
*   **R** (Rename): Rename the selected file.
*   **^R**: Rename all tagged files.
*   **S** (Sort): Sort filelist (Access time, Change time, Extension, Group, Modification time, Name, Owner, Size).
*   **^S** (Search): Execute grep on tagged files. Untags files that do not match the command.
*   **Date Changes:** Date actions change Accessed time, Modified time, or both (POSIX does not allow setting creation/birth time here).
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all displayed files.
*   **U** (Untag): Untag selected file.
*   **^U**: Untag all displayed files.
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
When browsing an archive (ZIP, TAR, etc.), ytree behaves similarly to a read-only file system.

**Archive-Dir Mode**
*   **F** (Filter): Set file filter.
*   **G** (Global): Show all files across all logged volumes in one global list.
*   **L** (Log): Log a new directory or archive.
*   **S** (Showall): Show all files in the archive.
*   **T** (Tag): Tag all files in current virtual directory.
*   **U** (Untag): Untag all files in current virtual directory.
*   **^F** (Dir Mode): Cycle display modes.
*   **Return**: Switch to Archive-File Mode.
*   **Left Arrow**: At the archive root, this closes the archive and returns to the parent directory.

**Archive-File Mode**
*   **C** (Copy): Copy (Extract) selected file.
*   **^K** (Copy Tagged): Copy (Extract) all tagged files.
*   **F** (Filter): Set file filter.
*   **H** (Hex): View file in hex mode.
*   **P** (Pipe): Pipe content to command.
*   **S** (Sort): Sort file list.
*   **^S** (Search): Search tagged files for a string. Untags files that do not match.
*   **T** (Tag): Tag selected file.
*   **^T**: Tag all files.
*   **U** (Untag): Untag selected file.
*   **^U**: Untag all files.
*   **V** (View): View file.
*   **^V**: **View Tagged**. View all tagged files sequentially. Mode is controlled by `TAGGEDVIEWER`: `external` (default, uses `$PAGER`) or `internal` (built-in viewer with in-app navigation).
*   **^F** (File Mode): Cycle display modes.
*   **Return**: Switch to Archive-Dir Mode.
*   **\*** (Asterisk): Invert tag selection.

# COMMAND LINE EDITING

Input prompts support standard shortcuts:

*   **^A / Home**: Start of line.
*   **^E / End**: End of line.
*   **^K**: Delete to end of line.
*   **^U**: Delete to start of line.
*   **^W**: Delete word left.
*   **^D / Del**: Delete character.
*   **^H / Backspace**: Backspace.
*   **Up Arrow**: History (with `P` to Pin, `D` to Delete).
*   **F2**: Directory picker for path-entry prompts (for example: Log, Copy, Move destination fields).

# CONFIGURATION

ytree looks for a configuration file at `~/.ytree`. A default is provided in `ytree.conf`.

### Key Options

*   **ANIMATION=1**: Enable the warp-speed starfield during scans.
*   **AUTO_REFRESH=3**: Control automatic refresh behavior (Bitmask). Add values to combine modes.
    *   **1**: Enable File System Watcher (Inotify).
    *   **2**: Refresh on Directory Navigation (Cursor move).
    *   **4**: Refresh when entering the File Window.
    *   *Default*: `3` (1 + 2) enables both Watcher and Navigation Refresh.
*   **HIDEDOTFILES=1**: Hide files starting with `.` by default.
*   **TAGGEDVIEWER=external|internal**: Controls `^V` (View Tagged). `external` uses `$PAGER` (default). `internal` uses the built-in tagged viewer.
*   **HIGLOBAL_COLOR=fg,bg**: Search-hit highlight color used in preview and internal tagged viewer only.
*   **[COLORS]**: Customize the color scheme.

### External Viewers

The `[VIEWER]` section maps file extensions to external programs.

```ini
[VIEWER]
.jpg,.gif,.png=xv
.1,.md=nroff -man | less
.pdf=okular
.mp3=mpg123
```
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
