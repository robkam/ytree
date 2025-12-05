# ytree - A File Manager for UNIX

**ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is a clone of the famous DOS file manager **XTreeGold**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

This version is a modern refactor of the original `ytree` by Werner Bregulla, incorporating features from XTree and ZTreeWin, using modern C standards, and integrating robust libraries like `libarchive`.

## Features

*   **Classic XTree Interface:** Directory Tree window coupled with a Small File Window for previews, expandable to a full-screen File Window.
*   **Multi-Volume Support:** Log multiple drives, directories, or archives simultaneously and switch between them instantly.
*   **Keyboard-Centric:** Efficient navigation and file operations without needing a mouse.
*   **Archives as Directories:** Seamlessly browse and extract from archives (ZIP, TAR, RAR, 7Z, etc.) as if they were read-only directories.
*   **Advanced Filtering:** Filter files by regex pattern, size, date, and attributes.
*   **Modern Standards:** Built with C99/POSIX compliance, reducing legacy debt and improving portability.
*   **Configurable UI:** Customizable colors, file type highlighting, and modernized statistics panel.
*   **External Viewers:** Associate specific file extensions with external programs (images, PDFs, etc.).
*   **User Commands:** Bind keys to custom shell commands/scripts for infinite extensibility.

## Installation

### Prerequisites

*   **C Compiler** (GCC or Clang)
*   **ncurses** (libncurses-dev / ncurses-devel)
*   **readline** (libreadline-dev / readline-devel)
*   **libarchive** (libarchive-dev / libarchive-devel)

### Build from Source

```bash
# Clone the repository
git clone https://github.com/robkam/ytree.git
cd ytree

# Compile (Optimized Release Build)
make

# Install
sudo make install

# Uninstall
sudo make uninstall
```

*Note: Developers can compile with AddressSanitizer enabled by running `make DEBUG=1`.*

## Usage

Start ytree from the command line:

```bash
ytree [directory] [archive_file]
```

If no argument is provided, it starts in the current directory.

### Key Bindings (Brief)

*   **Arrows / Vi Keys:** Navigate the tree and file lists.
*   **Enter:** Enter a directory or view a file.
*   **` (Backtick):** Toggle visibility of hidden dot-files.
*   **L:** Log (scan) a new directory or archive.
*   **K (Shift+K):** Open Volume Menu (Switch/Release volumes).
*   **< / >:** Cycle through loaded volumes.
*   **T:** Tag a file.
*   **C:** Copy tagged files.
*   **M:** Move tagged files.
*   **D:** Delete tagged files.
*   **R:** Rename file(s).
*   **F:** Set Filter (Filespec).
*   **^Q:** Quit to directory (requires shell wrapper).
*   **Q:** Quit.

See the manual page (`man ytree`) for a complete list of commands.

## Quit to Directory (^Q)

To allow `ytree` to change your shell's working directory upon exit (using `^Q`), add this function to your `~/.bashrc` or `~/.zshrc`:

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

Then use `yt` instead of `ytree` to start the program.

## Configuration

ytree looks for a configuration file at `~/.ytree`. A default configuration is provided in `ytree.conf`.

Key options include:
*   **ANIMATION=1**: Enable the warp-speed starfield during scans.
*   **HIDEDOTFILES=1**: Hide files starting with `.` by default.
*   **[COLORS]**: Customize the color scheme.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on coding standards and the pull request process.

## License

ytree is free software distributed under the GPL. See the [LICENSE](LICENSE) file for details.

---
*Original Author: Werner Bregulla*