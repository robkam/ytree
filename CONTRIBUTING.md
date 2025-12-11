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

### Part 1: The Consultant (`Google AI Studio Web Interface`)

For complex architectural changes (e.g., "Implement a new file system watcher" or "Fix a complex memory corruption"), the local Chat script may lack sufficient context. In these cases, use the **Google AI Studio Web Interface** to analyze the **entire project**.

1.  **Generate Context:** Run the provided script to bundle all relevant source code into a single file.
    ```bash
    ./gather_context.py > context.txt
    ```
    *(Note: On Mac/Linux, you can pipe directly to clipboard: `./gather_context.py | xclip -sel clip`)*

2.  **Upload:**
    *   Go to Google AI Studio.
    *   Create a new prompt.
    *   Upload or paste the content of `context.txt`.

3.  **Prompt:** Ask your architectural question. Use the response to draft a `task.txt` plan, then switch to Part 2 to execute it locally.

### Part 2: The Builder (`build-ytree.py`)

Use this script to execute architectural changes or large refactors. It reads a `task.txt` file and rewrites source code.

*   **Model:** Gemini 2.5 Flash.
*   **Role:** Senior C Developer.
*   **Mechanism:** Smart Filtering. To save tokens, the Builder tries to guess which files are relevant. You can override this manually.

**Usage:**

1.  **Create `task.txt`:** Describe the **Logical Goal**. (The "What").
    *   **Context Control (Important):** If you know which files need modification, list them explicitly using a `Context:` line. This prevents the tool from sending the entire project structure to the AI, saving quota.

    *Example `task.txt`:*
    ```text
    Context: dirwin.c, global.h

    Change the color of directories to RED when they are marked as read-only.
    Ensure this checks the st_mode bit correctly.
    ```

2.  **Run the Builder:**
    ```bash
    ./build-ytree.py --task task.txt --apply
    ```

3.  **Verify:**
    ```bash
    make clean && make DEBUG=1
    ```

4.  **Loop:**
    *   If it fails, revert (`git restore .`), refine `task.txt`, and try again.

### Part 3: The Junior Consultant (`chat-ytree.py`)

Use this script to query the codebase, debug issues, and refine ideas.

*   **Mechanism:** Lazy Loading. By default, the AI only sees the list of filenames (the tree), not the content. You must "load" files into the context when you want the AI to read them.

**Usage:**
```bash
# Standard start (Context is empty)
./chat-ytree.py

# Pre-load specific files if you know you need them
./chat-ytree.py --context "main.c, global.h"
```

**Commands:**
*   `/load <pattern>`: Reads the content of matching files and sends them to the AI.
    *   *Example:* `/load dir*.c` loads `dirwin.c` and `dirtree.c`.
    *   *Example:* `/load log.c` loads just `log.c`.
*   `/files`: Lists all available files in the project.
*   `/clear`: Clears conversation history (and wipes loaded file memory).
*   `/quit`: Exits.

**Workflow Example:**
1.  Run `./chat-ytree.py`.
2.  User: "Where is the main loop?"
3.  AI: "I can't see the code yet. Please load `main.c`."
4.  User: `/load main.c`
5.  AI: "I have loaded `main.c`. The main loop is located in function `HandleEvents`..."

### Best Practices for AI Interaction

To maximize effectiveness and minimize frustration when working with the AI agents, adhere to these rules:
1.  **Atomic Tasks (Granularity):**
    *   Do not ask the AI to perform multiple distinct tasks in a single prompt.
    *   Break complex features down into single, granular units of work (e.g., "Add the function prototype to the header" -> "Implement the function" -> "Update the makefile").
    *   **Why:** LLMs degrade in quality when context and complexity increase. Small, serial steps ensure higher accuracy.
2.  **Revert over Repair (The "Clean Slate" Rule):**
    *   If the AI generates code that fails to compile or introduces a bug, **do not** ask it to "fix the error" immediately. This often leads to "compounded errors," creating a tangled mess of bad patches on top of bad code.
    *   **Action:** Immediately run `git restore .` to revert to the last known good state. Then, analyze *why* the prompt failed, refine your `task.txt` instructions (make them more specific or provide better context), and run the Builder again.
3.  **Reframing:**
    *   If the AI gets stuck repeatedly on a specific problem, do not keep repeating the same instruction.
    *   **Action:** Reframe the request. Approach the problem from a different angle (e.g., instead of "Fix this loop," try "Rewrite this logic using a state machine approach").
4.  **Context Hygiene:**
    *   LLMs have a finite context window. As a chat session grows longer, the model "forgets" earlier instructions or gets confused by outdated code snippets in the history.
    *   **Local Script:** Use `/clear` to wipe history or `/reload` to refresh file context.
    *   **Google AI Studio:** Start a **New Chat** for every distinct task. Long threads accumulate "hallucination debt" where the model prioritizes its previous (incorrect) guesses over new facts. If the model starts looping or hallucinating, abandon the thread and start fresh.
5.  **Human Review:**
    *   **You are the Lead Architect.** The AI is a junior developer that types very fast.
    *   **Action:** Never assume the AI's code is correct. **Always diff everything** before accepting changes. Verify that it compiles and test it to find it behaves as expected.
### Instrumentation-Based Debugging
If the AI is struggling to fix a bug or is "guessing" solutions that don't work, use this specific prompt pattern to force a diagnostic approach. This strategy (often called "printf debugging") focuses on gathering runtime evidence before attempting a fix.
**The Prompt:**
> "I have a bug: [DESCRIBE SYMPTOM]. Don't fix it yet. Create a task to instrument the relevant code with debug prints so we can trace the root cause."
**Why this works:**
1.  **"Don't fix it yet":** This stops the LLM from hallucinating a "solution" based on probability. It forces it to switch from "Solver Mode" to "Debugger Mode."
2.  **"Instrument":** Tells it to add `fprintf(stderr...)` or logging to specific functions.
3.  **"Relevant code":** It forces the LLM (the Architect) to look at the file list and logic flow to decide *where* the logs go, relieving you of that responsibility.
---

## Architectural Decisions & Constraints

This section documents agreed-upon architectural constraints and non-goals. These decisions prevent scope creep and ensure the codebase remains maintainable given its specific goals (TUI file manager).

### 1. Concurrency Model: Single-Threaded Event Loop
**Decision:** `ytree` will remain **single-threaded**. We will **NOT** implement multi-threading for background scanning or operations.
**Rationale:**
*   **Ncurses Constraints:** The `ncurses` library is not thread-safe. Updating the UI from a background thread requires complex message passing or locking mechanisms that complicate the architecture significantly.
*   **Global State:** The legacy codebase relies heavily on global state (`statistic`, `dir_entry_list`). Making this thread-safe would require a complete rewrite of the core data structures (Mutexes/Locks everywhere), which is a high-risk, high-cost endeavor ("100% effort").
*   **The "80/20" Solution:** The user frustration with "frozen" screens is addressed via **Visual Feedback** (Animations/Spinners) and **Graceful Abort** (ESC key) mechanisms. This delivers 80% of the benefit (responsiveness) for 20% of the complexity.
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