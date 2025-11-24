# Contributing to ytree

Thank you for your interest in contributing to ytree! This document provides guidelines for setting up your development environment, running tests, and submitting your changes.

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
./setup_dev.sh
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

The test suite uses `pytest` and `pexpect` to simulate user interaction.

To run the full suite, first ensure your Python virtual environment is activated, then run `pytest` from the project root:

```bash
pytest
```

The tests will automatically build a test environment, run the local `./ytree` binary against it, verify outcomes, and clean up.

**Important:** If you are adding a new feature or fixing a bug, please add a corresponding test case to `tests/test_main.py` that covers your changes.

## AI-Assisted Workflow (Google Gemini API)

We use Python automation scripts backed by the Google Gemini API to assist with the workflow.

### Setup

1.  **Get an API Key:** Obtain a key from Google AI Studio.
2.  **Export the Key:**
    ```bash
    export GEMINI_API_KEY="your_key_here"
    ```

### Part 1: The Consultant (`Google AI Studio Web Interface (Gemini 3.0 Pro)`)

For complex architectural changes (e.g., "Implement a new file system watcher"), the local Chat script is too limited. In these cases, use the **Google AI Studio Web Interface (Gemini 3.0 Pro)** to draft your `task.txt` plan, then paste it locally.

### Part 2: The Builder (`build-ytree.py`)

Use this script to execute changes. It reads a `task.txt` file and rewrites the source code.

*   **Model:** Gemini 2.5 Flash. Gemini 2.5 Pro is smarter, but has too low a quota.
*   **Role:** Senior C Developer. It handles implementation details, headers, and Makefiles.
*   **Capabilities:** Uses an infinite continuation loop to write large files without truncation.

**Usage:**

1.  **Create `task.txt`:** Describe the **Logical Goal** (The "What").
    *   *Good:* "Implement a directory polling mechanism that checks `st_mtime` every 500ms. Do not use `inotify`."
    *   *Bad:* "Add `int x` to line 40 of dirwin.c." (Do not micro-manage the builder).

2.  **Run the Builder:**
    ```bash
    # Standard run
    ./build-ytree.py --task task.txt --apply
    ```

3.  **Verify:**
    ```bash
    make clean && make DEBUG=1
    ```

4.  **Loop:**
    *   If it fails, revert (`git restore .`), refine `task.txt`, and try again.
    *   If it succeeds, go back to the Chat window and type `/reload` so the Consultant sees the new code.

### Part 3: The Consultant (`chat-ytree.py`)

Use this script to query the codebase, debug issues, and refine ideas. It loads the entire source tree into context but **cannot write files**.

*   **Model:** Gemini 2.5 Flash (Fast, Cheap, High Quota).
*   **Default Role:** Advisor. Good for "Explain this function," "Find where variable X is defined," or "Draft a basic plan."
*   **Instruction Mode:** To generate a `task.txt` plan locally, start your prompt with **"Give me a task.txt"**. The AI will switch modes and output a structured, numbered list of requirements suitable for copying into `task.txt`.

**Usage:**
```bash
./chat-ytree.py
```

**Commands:**
*   `/reload`: Re-reads all files from disk (run this after every Build!).
*   `/clear`: Clears conversation history to save tokens.
*   `/quit`: Exits.

---

## Submitting Changes

1.  **Fork the repository** on GitHub.
2.  **Create a new branch** for your feature or bugfix (`git checkout -b feature/my-new-feature`).
3.  **Make your changes** and commit them with a clear, declarative commit message.
4.  **Push your branch** to your fork (`git push origin feature/my-new-feature`).
5.  **Open a Pull Request** against the `main` branch of the upstream ytree repository.

## Coding Style

Please adhere to the existing coding style found throughout the project. The codebase follows C89/C99 standards with a focus on readability, consistency, and proper resource management.

### Ncurses Rendering Guidelines

When working with the ncurses UI, it is critical to handle window drawing and coloring correctly to avoid visual artifacts and performance issues.

**Cautionary Note on `WbkgdSet()`**

The function `WbkgdSet()` sets the background property for an **entire window**. It should not be used within rendering loops to style individual lines of text. Misusing it in this way will cause the entire window's background to change on each call, leading to flickering, incorrect colors (e.g., black or same-as-text backgrounds), and inefficient rendering. Avoid calling this function repeatedly during a single screen refresh.

**The Correct Approach**

The idiomatic approach for rendering a window is a two-step process that separates the window's background from the text's attributes:
1.  **Set the Window Background Once:** In the main display function (e.g., `DisplayTree`, `DisplayFiles`), make a single call to `WbkgdSet()` to establish the desired background for the entire window. Immediately follow this with `werase()` to apply this background, creating a clean canvas for drawing.
2.  **Style Text with Attributes:** In functions that render individual lines (e.g., `PrintDirEntry`), use `wattron()` to enable specific attributes (like a color pair for highlighting or bold text) before drawing the text, and `wattroff()` to disable them afterward. These functions only affect the characters being drawn, not the window's persistent background property.

This separation ensures stable, efficient, and artifact-free rendering.