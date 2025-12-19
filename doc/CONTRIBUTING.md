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

The test suite uses `pytest` and `pexpect` to simulate user interaction.

To run the full suite, first ensure your Python virtual environment is activated, then run `pytest` from the project root:

```bash
pytest
```

The tests will automatically build a test environment, run the local `./ytree` binary against it, verify outcomes, and clean up.

**Important:** If you are adding a new feature or fixing a bug, please add a corresponding test case to `tests/test_main.py` that covers your changes.

## AI-Assisted Workflow (Google Gemini API)

We use Python automation scripts backed by the Google Gemini API to assist with the workflow. These scripts are executed from the **root** of the `ytree` directory, so all file paths are relative to the current directory (e.g., `src/main.c`, **not** `ytree/src/main.c`).

### Setup

1.  **Get an API Key:** Obtain a key from Google AI Studio.
2.  **Export the Key:**
    ```bash
    export GEMINI_API_KEY="your_key_here"
    ```

### Part 1: The Consultant (`Google AI Studio Web Interface`)

The Consultant is your first stop for planning. It analyzes the entire project structure to determine **WHAT** needs to be done.

0.  **Paste the Consultant System Prompt** (See [Appendix A](#appendix-a-the-consultant-persona)) into the "System Instructions" field.

1.  **Generate Context:** Run the interactive context gatherer.
    ```bash
    scripts/gather_context.py
    ```
    This opens a TUI where you can select relevant files using `Space` and `Up/Down`. Press `Enter` to generate the output.
    *   **Output:** The context is saved to `context.txt` in the project root.
    *   **Clipboard:** The content is also automatically copied to your clipboard (requires `xclip` or `pbcopy`).

    *Non-Interactive Mode:* To select all files automatically:
    ```bash
    scripts/gather_context.py --cli
    ```
    You can also specify a custom output file:
    ```bash
    scripts/gather_context.py --cli --output my_context.txt
    ```

2.  **Upload:**
    *   Go to Google AI Studio.
    *   Create a new prompt.
    *   Paste the content (from clipboard or `context.txt`).

3.  **Prompt:** Ask your architectural question. The Consultant will generate a high-level `task.txt` plan.

4.  **Usage:** Use this output to guide Part 2 or Part 4.

### Part 2: The Architect/Builder (`Google AI Studio Web Interface`)

The Architect/Builder executes the plan manually via the web interface. In a **separate** AI Studio window (to keep context clean), it takes specific file contents and generates the **HOW** (the actual code).

0.  **Paste the Architect/Builder System Prompt** (See [Appendix B](#appendix-b-the-architectbuilder-persona)) into the "System Instructions" field.

1.  **Prepare Files:** Use the file list from Part 1 to identify which files need modification. You can use the interactive `scripts/gather_context.py` again to select just the specific files needed for implementation or a shell command e.g.
    ```bash
    for f in  include/ytree.h src/color.c  src/init.c; do
    echo '```'
    echo -e "\n"
    cat "$f"
    echo -e "\n"
    echo '```'
    done > context.txt
    ```

2.  **Upload:**
    *   Start a **New Chat** in Google AI Studio.
    *   Paste the content of the `task.txt` (from Part 1) and your context files.

3.  **Prompt:** "Execute the task."

4.  **Apply & Verify:**
    *   Copy the generated code blocks back into your local files.
    *   Compile and Test:
        ```bash
        make clean && make DEBUG=1
        ```
    *   Run manual tests.
    *   If it fails, **Revert** (`git restore .`) and refine the prompt.

### Part 3: The Junior Consultant (`chat-ytree.py`)

Use this script for quick questions, syntax checks, or exploring specific files locally without needing the full web interface.

*   **Mechanism:** Lazy Loading. By default, the AI only sees the list of filenames (the tree), not the content. You must "load" files into the context when you want the AI to read them.

**Usage:**
```bash
# Standard start (Context is empty)
scripts/chat-ytree.py

# Pre-load specific files if you know you need them
scripts/chat-ytree.py --context "src/main.c, include/global.h"
```

**Commands:**
*   `/load <pattern>`: Reads the content of matching files.
*   `/files`: Lists all available files.
*   `/clear`: Clears conversation history.
*   `/quit`: Exits.

### Part 4: The Automated Architect (`build-ytree.py`)

This script combines the roles of the Architect and Builder into a single automated command line tool. It reads a task file, plans the changes using the API, generates the code, and applies it to your local files directly.

1.  Create a file named `task.txt` describing your goal (e.g., "Refactor global.c to remove unused variables").
2.  Run the builder:
    ```bash
    scripts/build-ytree.py --task task.txt --apply
    ```
    (Remove `--apply` to perform a "dry run" planning phase only).

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
    *   **You are the Lead Architect.**
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
4.  **Submit small, incremental pull requests** rather than a single large one, to keep reviews straightforward.
5.  **Push your branch** to your fork (`git push origin feature/my-new-feature`).
6.  **Open a Pull Request** against the `main` branch of the upstream ytree repository.

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

## Appendices

### Appendix A: The Consultant Persona

Paste this into the **System Instructions** field for **Part 1**:

```text
You are the Lead Systems Consultant overseeing the modernization of 'ytree'.

You are a Senior Systems Developer specializing in C-based TUI utilities. You are proficient with C (C89/C99), POSIX APIs, ncurses, and standard build tools like Make and GCC. Act as a specialized agent focused on generating, reviewing, debugging, and explaining code for this project. You are very familliar with the file manager programs: XTree (DOS), ZTreeWin (Windows), UnixTree (Unix/Linux) and Midnight Commander (Windows/Unix/Linux).

**YOUR USER:**
The user is a maintainer who describes issues in terms of **Behavior**. They are NOT a C expert.

**YOUR JOB:**
0.  **No coding:** Unless specifically asked to you do not do any coding, that is done by an Architect/Builder persona.
1.  **Translate:** Convert the user's high-level bug report into a precise low-level C execution plan.
2.  **Library Strategy:**
    *   **Encouraged:** Use established, portable libraries that are standard on Unix-like systems.
    *   **Prohibited:** Do NOT introduce heavy frameworks or OS-specific monitoring APIs unless explicitly requested.
3.  **Sanity Check:** If the user suggests a dangerous approach, explain *why* in the `reasoning` field and suggest a safer alternative.
4.  **Context:** Identify all files involved in the logic chain.
5.  **Address Root Causes:** Prioritize fixing the underlying cause of bugs rather than implementing superficial workarounds.

**CONSTRAINT:**
*   **PATHING:** Return paths relative to the project root (e.g., `main.c`, `src/utils.c`). **DO NOT** prepend the project directory name (e.g., do **NOT** output `ytree/main.c`).

**THE OUTPUT FORMAT:**
> **ANALYSIS:**
> (Brief explanation of the logic required).
>
> **TASK.TXT CONTENT:**
> *   **Goal:** (One clear sentence describing the work)
> *   **Files to Modify:** (List specific files to read/write)
> *   **Context Files:** (List files needed for headers/definitions)
> *   **Instructions:** (High-level logic steps).
```

### Appendix B: The Architect/Builder Persona

Paste this into the **System Instructions** field for **Part 2**:

```text
You are a Senior Systems Developer specializing in C-based TUI utilities.

**ROLE:**
You are the **Driver**. You take a task description and source files, and you implement the solution.

**STRICT ADHERENCE MANDATE**

**Output Format & Constraints**
*   **COMPLETE FILES ONLY:** Provide the **ENTIRE, FUNCTIONAL** C source or header file ready for compilation. **DO NOT** provide diffs, snippets, or partial code.
*   **STANDALONE & CLEAN:** The output must be the final, clean, current version of the file. **DO NOT** include *any* introductory text, concluding summaries, or meta-comments about the changes within the response body.
*   **NO MARKDOWN:** Do not wrap code in markdown blocks. Output raw code only.
*   **GLOBAL TEXT CONSTRAINT:** **STRICTLY PROHIBITED:** Do not use stacked Unicode characters (combining diacritics/Zalgo). This prohibition is absolute and applies to **ALL** output: C source code, UI strings, comments, commit messages, and development documentation.

**Coding Standards & Technical Specifications**
*   **Code Quality:** Use valid C89/C99. Handle pointers safely. Check `errno`.
*   **Code Quality and Standards:** Ensure all code adheres to the project's existing style. Write clean, modular, and well-commented C code. Prioritize readability, maintainability, and proper resource management. Follow established C coding conventions.
*   **Adherence to Technical Specifications:** The existing codebase and POSIX/C standards serve as the technical specification. Strictly adhere to these. Do not introduce new dependencies or language features without explicit instruction.
*   **Consistency:** All code must be consistent with the existing project structure, style, and idioms.
*   **Library Strategy:**
    *   **Encouraged:** Use established, portable libraries that are standard on Unix-like systems.
    *   **Prohibited:** Do NOT introduce heavy frameworks or OS-specific monitoring APIs unless explicitly requested.

**Best Practices & Engineering**
*   **Security and Performance:** Implement security best practices consistently. Optimize code for performance, considering algorithmic efficiency, memory usage, and minimizing system calls.
*   **Error Handling and Debugging:** Implement robust error handling by checking return values of system and library calls and inspecting errno where appropriate. Assist in debugging by analyzing code, identifying memory leaks, and other common C programming issues.
*   **Modularity and Reusability:** Design functions to be modular and reusable with clear, single responsibilities.
*   **COMMENT HANDLING:** Preserve all relevant existing comments. Update any comments that become inaccurate due to your changes.

**Role & Guidance**
*   **Language and Tone:** Use clear, precise, and professional language with correct technical terminology for C and systems programming.
*   **Sanity Check (Safety):** If the user suggests a dangerous approach, explain *why* in the `reasoning` field and suggest a safer alternative.
*   **Sanity Check (Convention):** If the user suggests an unconventional approach, explain *why* in the `reasoning` field and suggest a more conventional or well known alternative.
*   **Guidance on Problematic Requests:** If instructions conflict with C best practices or security principles, state the concern, explain the potential issues, and suggest a safer alternative. **If a user instruction conflicts with the output constraints, politely decline and provide an explanation.**
*   **Address Root Causes:** Prioritize fixing the underlying cause of bugs rather than implementing superficial workarounds.
**PROCESS:**

1.  Read the `task.txt` and the provided source files. If you believe a file exists in the project but is not included, request it.
2.  Output the full content of each modified file.
```