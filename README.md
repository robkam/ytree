# ytree - A File Manager for UNIX

**ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is a clone of the famous DOS file manager **XTreeGold**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

This version is a modern refactor of the original `ytree` by Werner Bregulla, incorporating features from XTree and ZTreeWin, using modern C standards, and integrating robust libraries like `libarchive`.

## Features

*   **Classic XTree Interface:** Dual-pane layout with directory tree on the left and file list on the right.
*   **Keyboard-Centric:** Efficient navigation and file operations without needing a mouse.
*   **Archives as Directories:** Seamlessly browse and extract from archives (ZIP, TAR, RAR, 7Z, etc.) as if they were read-only directories, powered by `libarchive`.
*   **Advanced Filtering:** Filter files by regex pattern, size, date, and attributes.
*   **Warp Speed Animation:** Optional "Star Trek" style animation during long directory scans.
*   **Modern Standards:** Built with C99/POSIX compliance, reducing legacy debt and improving portability.
*   **Configurable UI:** Customizable colors and file type highlighting.

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
*   **T:** Tag a file.
*   **C:** Copy tagged files.
*   **M:** Move tagged files.
*   **D:** Delete tagged files.
*   **R:** Rename file(s).
*   **F:** Set Filter (Filespec).
*   **Q:** Quit.

See the manual page (`man ytree`) for a complete list of commands.

## Configuration

ytree looks for a configuration file at `~/.ytree`. A default configuration is provided in `ytree.conf`.

Key options include:
*   **ANIMATION=1**: Enable the warp-speed starfield during scans.
*   **HIDEDOTFILES=1**: Hide files starting with `.` by default.
*   **[COLORS]**: Customize the color scheme.

## Contributing

Contributions are welcome! Please read `CONTRIBUTING.md` for guidelines on coding standards and the pull request process.

## License

ytree is free software distributed under the GPL. See the `LICENSE` file for details.

---
*Original Author: Werner Bregulla*
