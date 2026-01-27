# Contributing to ytree

Thank you for your interest in contributing to ytree! This document provides guidelines for setting up your development environment, running tests, and submitting your changes.

**Note for AI-Assisted Development:**
For the AI-assisted development workflow, script usage, and System Persona prompts, please see **[AI_WORKFLOW.md](AI_WORKFLOW.md)**.

## Development Setup

This project uses a combination of C for the application and Python for the test suite and AI assistance tools.

### 1. Prerequisites

- A C compiler (like `gcc`) and `make`.
- `libncurses-dev`, `libtinfo-dev`, `libreadline-dev`, `libarchive-dev`.
- `python3` and `python3-venv`.
- `pandoc` (for generating the man page).

On a Debian/Ubuntu-based system, you can install these with:
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev libtinfo-dev libreadline-dev libarchive-dev python3-venv pandoc
```

### 2. Initial Python Environment Setup

A helper script is provided to create an isolated Python environment for testing and AI tools. From the root of the project, run:

```bash
scripts/setup_dev.sh
```

This script will create a `.venv` directory, activate it, and install the required Python libraries (`pytest`, `pexpect`, `google-generativeai`, `prompt_toolkit`).

After the first run, you must reactivate this environment for any new shell session:
```bash
source .venv/bin/activate
```

## Building the Project

The `Makefile` supports two build modes: Release (default) and Debug.

### Release Build
For standard testing and usage, perform a release build. This applies optimizations (`-O2`) and produces a binary with no runtime debug dependencies.

```bash
make clean
make
```

### Debug Build (AddressSanitizer)
When developing new features or debugging crashes (especially segmentation faults), you should compile with debug mode enabled. This links against **AddressSanitizer (ASan)** and disables optimizations (`-O1`), providing detailed stack traces on memory errors.

```bash
make clean
make DEBUG=1
```

**Note:** The debug binary is significantly slower. Do not use this for general performance testing.

## Running the Test Suite

The test suite uses `pytest` and `pexpect` to simulate user interaction. It relies on a robust controller (`tests/ytree_control.py`) to manage the ncurses application in a headless environment.

**Prerequisite:** Ensure your Python virtual environment is activated:
```bash
source .venv/bin/activate
```

### 1. The Standard Way: `make test`
This is the preferred method for general regression checks and CI.
*   **Automatic Rebuilds:** It ensures the binary is up-to-date by automatically recompiling the C code if necessary before running tests.
*   **Simplicity:** It provides a "One Button" experience without needing to remember pytest flags.

```bash
# Standard run (quiet, shows progress dots)
make test

# Verbose run (shows individual test names and stdout)
make test-v
```

### 2. The Advanced Way: Direct CLI (`pytest`)
Use the `pytest` command directly when actively debugging, developing new features, or iterating rapidly on specific test cases.

```bash
# Run only tests matching a keyword (e.g., "archive")
pytest -k archive

# Run a specific test file
pytest tests/test_core.py

# Run and stop on the first failure (useful for debugging loops)
pytest -x
```

**Test Infrastructure:**
*   `tests/test_core.py`: Main regression tests for core features (Copy, Move, Rename).
*   `tests/ytree_control.py`: The PTY controller that drives `ytree`, handling input clearing and timeouts.
*   `tests/ytree_keys.py`: Centralized definitions for key commands.
*   `tests/conftest.py`: Pytest fixtures for setup and teardown.

---

## Architectural Decisions & Constraints
This section documents agreed-upon architectural constraints and non-goals. These decisions prevent scope creep and ensure the codebase remains maintainable given its specific goals (TUI file manager).

### 1. Concurrency Model: Single-Threaded Event Loop
**Decision:** `ytree` will remain **single-threaded**. We will **NOT** implement multi-threading for background scanning or operations.
**Rationale:**
*   **Ncurses Constraints:** The `ncurses` library is not thread-safe. Updating the UI from a background thread requires complex message passing or locking mechanisms that complicate the architecture significantly.
*   **Global State:** The legacy codebase relies heavily on global state (`statistic`, `dir_entry_list`). Making this thread-safe would require a complete rewrite of the core data structures (Mutexes/Locks everywhere), which is a high-risk, high-cost endeavor ("100% effort").
*   **The "80/20" Solution:** Use **Visual Feedback** (Animations/Spinners) and **Graceful Abort** (ESC key) mechanisms. This delivers 80% of the benefit (responsiveness) for 20% of the complexity.

### 2. UI Toolkit: Hard Ncurses Dependency
**Decision:** `ytree` is and will remain a curses-based TUI application.
*   **Non-Goal:** Implementing a "headless" mode or porting to a different UI toolkit (GTK, Qt) is out of scope.
*   **Non-Goal:** Removing `ncurses` to run on raw serial lines without termcap capabilities is out of scope.

### 3. Global State vs. Future Split Screen (F8)
**Context:** Currently, `ytree` uses a "Single Active Volume" model where global macros (like `statistic`) map to the `CurrentVolume`.
*   **Constraint:** Do not attempt to prematurely refactor these globals into "Window Contexts" until Phase 5 (Split Screen Implementation).
*   **Future Impact:** When F8 (Split Screen) is implemented, it will require a major architectural refactor to move filtering state and directory pointers out of the global scope and into per-pane structures. Until then, the codebase assumes a single active view to maintain stability.

---

## Submitting Changes

1.  **Fork the repository** on GitHub.
2.  **Create a new branch** for your feature or bugfix (`git checkout -b feature/my-new-feature`).
3.  **Make your changes** and commit them with a clear, declarative commit message.
4.  **Submit small, incremental pull requests** rather than a single large one, to keep reviews straightforward.
5.  **Push your branch** to your fork (`git push origin feature/my-new-feature`).
6.  **Open a Pull Request** against the `main` branch of the upstream ytree repository.

## Coding Style

Please adhere to the existing coding style found throughout the project. The codebase follows C89/C99 standards with a focus on readability, consistency, and proper resource management.

## Ncurses Guidelines

### Key Bindings

When mapping keys in a terminal environment, many Control combinations generate the same ASCII codes as physical keys. Binding these "Taboo" keys will break standard navigation or cause unexpected behavior.

**Taboo Bindings (Do Not Use)**

| Control Key | ASCII Value | Physical Key Collision | Impact |
| :--- | :--- | :--- | :--- |
| **`^I`** | `0x09` | **Tab** | Navigation failure. |
| **`^M`** | `0x0D` | **Enter** (CR) | Inability to confirm actions. |
| **`^J`** | `0x0A` | **Enter** (LF) | Inability to confirm actions. |
| **`^[`** | `0x1B` | **Escape** | Breaks cancel/exit logic. |

**Safe Control Keys**

Preferred bindings that generally avoid conflicts:
`^A` (Home), `^B`, `^E` (End), `^F`, `^G`, `^K`, `^L` (Redraw), `^N`, `^O`, `^P`, `^R`, `^T`, `^U`, `^V`, `^W`, `^X`, `^Y`.

### Rendering

When working with the ncurses UI, it is critical to handle window drawing and coloring correctly to avoid visual artifacts and performance issues.

**Cautionary Note on `WbkgdSet()`**

The function `WbkgdSet()` sets the background property for an **entire window**. It should not be used within rendering loops to style individual lines of text. Misusing it in this way will cause the entire window's background to change on each call, leading to flickering, incorrect colors (e.g., black or same-as-text backgrounds), and inefficient rendering. Avoid calling this function repeatedly during a single screen refresh.

**The Correct Approach**

The idiomatic approach for rendering a window is a two-step process that separates the window's background from the text's attributes:
1.  **Set the Window Background Once:** In the main display function (e.g., `DisplayTree`, `DisplayFiles`), make a single call to `WbkgdSet()` to establish the desired background for the entire window. Immediately follow this with `werase()` to apply this background, creating a clean canvas for drawing.
2.  **Style Text with Attributes:** In functions that render individual lines (e.g., `PrintDirEntry`), use `wattron()` to enable specific attributes (like a color pair for highlighting or bold text) before drawing the text, and `wattroff()` to disable them afterward. These functions only affect the characters being drawn, not the window's persistent background property.

This separation ensures stable, efficient, and artifact-free rendering.