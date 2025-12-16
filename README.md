
# ytree - A File Manager for UNIX

**ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is inspired by the famous DOS file manager **XTreeGold**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

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

For detailed usage, configuration, and keybindings, see [USAGE.md](doc/USAGE.md) or run `man ./ytree.1`.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](doc/CONTRIBUTING.md) for guidelines on coding standards and the pull request process.

## Reporting problems

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytree/issues).

It will help us to address the issue if you include the following:
*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **Version:** (ytree version)
*   **Steps to Reproduce:** (What I did)
*   **Expected Behavior:** (What I expected)
*   **Actual Behavior:** (What actually happened)

## License

ytree is free software distributed under the GPL. See the [LICENSE.md](LICENSE.md) file for details.

## Contributors

For detailed authorship, see [AUTHORS.md](doc/AUTHORS.md).
