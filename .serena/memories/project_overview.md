# Project Overview

ytree is a modern C99 refactor of the classic ytree file manager for UNIX-like systems.
Unlike traditional file browsers, ytree is a "Logger" that scans the entire drive hierarchy into memory, allowing for flat views, instant filtering across subdirectories, and bulk operations.
It features Multi-Volume support, Split-Screen workflows, and transparent archive browsing.

## Tech Stack
- **Languages:** C (C99 Standard) for main application, Python for automation/testing.
- **Dependencies:** ncurses, readline, libarchive.
- **Build System:** Make (`Makefile`), built with GCC or Clang.
- **Environment:** WSL2 Ubuntu, relying on POSIX paths exclusively.
- **Testing:** `pytest` (using `pexpect` for TUI behavioral testing) within a local virtual environment (`.venv`).
