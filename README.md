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

For detailed usage, configuration, and keybindings, see [USAGE.md](USAGE.md) or run `man ./ytree.1`.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on coding standards and the pull request process.

## License

ytree is free software distributed under the GPL. See the [LICENSE](LICENSE) file for details.

## Contributors

**Original Development**
Originally developed by [Werner Bregulla](https://www.han.de/~werner/ytree.html). Many thanks to the people who helped during the classic era:

*   **Andrew Cottrell** - DJGPP port
*   **Andrey Zakhvatov / Peter Brevik** - FreeBSD port
*   **Axel F. Zinser** - NeXT port
*   **C. R. Bryan III** - QuitTo, hexdump
*   **Carlos Barros** - Color, history & keyF2, configurable viewer, Spanish man-page, list all tagged files, hex viewer/editor, readline
*   **Jens-Uwe Mager** - RS/6000 port, Configuration
*   **Marcus Brinkmann** - Port to GNU Hurd
*   **Mark Hessling** - QNX port, PDCurses, configuration addons
*   **Martynas Venckus** - OpenBSD/NetBSD port
*   **Norman Fahlbusch** - Testing and "marketing" :-)
*   **Roger Knobbe** - Secondary key for sort by extension
*   **S.Addams (SJA)** - OpenBSD port
*   **Scott Wagner** - Command/menu configurable
*   **Siegfried Salomon** - SCO OpenServer 5 (5.0.5)
*   **Tina Hoeltig** - Man-page, tips
*   **Valere Monseur aka Dobedo** - NetBSD
*   **Victor Vislobokov** - UTF-8 support

**v3.0 Modernization**
After decades of dormancy, the project was radically refactored and modernized by [Rob Kam](https://github.com/robkam) using Google AI Studio's **Gemini 3 Pro**.