# Architecture of ytree v3

## Overview
This document outlines the architectural modernization of `ytree`. The codebase has moved from a 1990s-style monolithic structure (K&R/C89) to a modular, standards-compliant C99 design.

The primary goal of this refactoring was to separate **Model** (Filesystem), **View** (Ncurses Rendering), and **Controller** (Input Handling) concerns, enabling advanced features like Split Screen (F8) and Multi-Volume support.

## Directory Structure

The source tree is organized semantically to enforce separation of concerns:

*   **`src/core/`**: Application lifecycle and state management.
    *   `main.c`: Entry point and argument parsing.
    *   `init.c`: Ncurses initialization and layout calculation.
    *   `volume.c`: Management of loaded directory trees (`Volume` structs).
*   **`src/ui/`**: User Interface logic.
    *   `ctrl_*.c`: **Controllers**. Input loops and event dispatching (e.g., `ctrl_dir.c`, `ctrl_file.c`).
    *   `render_*.c`: **Views**. Drawing logic decoupled from input (e.g., `render_dir.c`).
    *   `ui_nav.c`: Generic list navigation logic.
    *   `stats.c`: The right-hand information panel.
    *   `vol_menu.c`: The Volume Selection UI.
    *   `view_internal.c`: The built-in Hex/Text viewer logic.
    *   `view_preview.c`: The F7 side-panel preview renderer.
*   **`src/fs/`**: Filesystem Model (VFS).
    *   `readtree.c`: Directory scanning logic.
    *   `archive.c`: Libarchive integration for virtual filesystems.
    *   `vfs.c`: Virtual File System abstraction layer.
*   **`src/cmd/`**: Action Implementations.
    *   Atomic implementations of user commands (`copy.c`, `move.c`, `delete.c`, `execute.c`, `view.c`).
*   **`src/util/`**: Helpers.
    *   Safe string manipulation, path normalization, and history management.

## Key Architectural Decisions

### 1. Elimination of "God Objects"
Legacy `ytree` relied on massive files which handled file I/O, user input, and screen drawing simultaneously. In v3.0, these have been split. For example, the `View` functionality was split into:
*   `src/cmd/view.c`: External command execution logic.
*   `src/ui/view_internal.c`: Interactive Hex/Text editor.
*   `src/ui/view_preview.c`: Passive panel rendering.

### 2. Multi-Volume State (`Volume` vs `Global`)
Global variables for directory lists were replaced with a `Volume` structure managed by a Hash Table (`uthash`). This allows `ytree` to hold multiple filesystem trees (Volumes) in memory simultaneously, enabling instant context switching and the "Log" concept found in XTree.

### 3. Split-Screen Capabilities (`ViewContext`)
UI references are encapsulated in `ViewContext` structures rather than hardcoded global window pointers. This prepares the rendering pipeline to support dual independent panels (Left/Right) sharing the same underlying logic.

### 4. Modern C Standards
*   **Strings:** Unsafe `strcpy`/`sprintf` replaced with `snprintf` and bounds-checked helpers.
*   **System:** Transitioned to POSIX standard `statvfs` and `regex.h`.
*   **Memory:** Strict ownership models for dynamic lists (`FileEntry`, `DirEntry`) to prevent leaks.

## Future Roadmap

While the architecture is stable, this is a work in progress. See `[ROADMAP.md](ROADMAP.md)` for the roadmap.
