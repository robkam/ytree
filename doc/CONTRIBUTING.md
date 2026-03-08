# Contributing to ytree

Thank you for your interest in contributing to ytree! This document provides guidelines for setting up your development environment, running tests, and submitting your changes.

**Note for AI-Assisted Development:**
For the AI-assisted development workflow, script usage, System Persona prompts, and semantic tool integration (**Serena**, **jCodeMunch**), please see **[ai/WORKFLOW.md](ai/WORKFLOW.md)**.

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

**Prerequisite:** Ensure your Python virtual environment is activated (`source .venv/bin/activate`).

```bash
# Standard run (quiet, shows progress dots) — auto-rebuilds the binary
make test

# Verbose run (shows individual test names and stdout)
make test-v
```

For direct `pytest` CLI usage, test naming conventions, infrastructure details, and harness rules, see **[ai/TESTING.md](ai/TESTING.md)**.

---

## Architectural Decisions & Constraints

The project enforces strict architectural constraints (single-threaded event loop, hard ncurses dependency, context-passing with no global mutable state). For the full specification of these rules, permitted exceptions, and rationale, see **[ARCHITECTURE.md](ARCHITECTURE.md)**.

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