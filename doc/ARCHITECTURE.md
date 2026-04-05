# **System Architecture**
> **Purpose:** This document defines the internal design of `ytree`. It serves as the authoritative guide for maintaining the codebase's structural integrity.

## **1. Core Quality Principles**
To maintain architectural stability throughout the modernization, all changes must adhere to these foundational rules:

*   **Code Quality (DRY):** All development should adhere to the "Don't Repeat Yourself" principle. Code should be modular, reusable, and free of redundancy.
*   **Architectural Integrity (Anti-Patching):** Do not apply superficial fixes for deep architectural problems. If a bug is caused by fragmented state or logic, **STOP**. Refactor the architecture to unify the logic before fixing the specific bug. It is better to break one thing to fix the system than to patch the system and break everything.
*   **Single Responsibility (SRP):** Enforce strict modularity. Each file (module) must serve exactly one purpose. Maintain a hard separation between the **UI** (View), **File System** (Model), and **Commands** (Controller) to ensure the codebase remains testable and maintainable.
*   **Use Established Libraries:** Prefer mature, well-supported libraries (e.g., `libarchive`) instead of creating custom replacements.
*   **Module Ownership (Feature Containment):** A feature that can be self-contained **MUST** be self-contained in its own module. It is **FORBIDDEN** to implement a new feature as a sub-function within an existing controller (`ctrl_*.c`) unless that logic is exclusively and inseparably part of that controller's input/event loop. The canonical test: *"Could this function be called from a different context without modification?"* If yes, it does not belong in a controller. Before adding any new function to `ctrl_dir.c` or `ctrl_file.c`, you MUST first ask: which module owns this logic? If no suitable module exists, create one. Controllers are terminal sinks - they dispatch to modules; they do not house modules.

### 1.1 Module Boundary Contract (Enforced)

The project uses a structural QA guard (`make qa-module-boundaries`) to catch architectural drift early. This guard is intentionally concrete and deterministic:

*   **No implementation includes:** `#include "*.c"` is forbidden.
*   **Per-directory dependency policy:** each source directory may depend only on approved layer(s). The target policy is:
    *   `core -> core`
    *   `util -> core, util`
    *   `fs -> core, fs, util`
    *   `cmd -> core, cmd, fs, util`
    *   `ui -> core, cmd, fs, ui, util`
*   **Legacy exceptions are explicit:** pre-existing violations are enumerated in `scripts/check_module_boundaries.py` as a temporary debt list. The guard fails on any new violation and also fails if an exception becomes stale and is not removed.
*   **Controller growth budget:** `src/ui/ctrl_dir.c` and `src/ui/ctrl_file.c` have line-count budgets as anti-regression tripwires to prevent feature creep back into controller modules.
*   **Controller top-level allowlists:** `src/ui/ctrl_dir.c` and `src/ui/ctrl_file.c` are pinned to approved top-level function sets. Any newly introduced top-level helper fails the guard unless architecture explicitly approves an allowlist update.
*   **God-function anti-growth budgets:** `HandleDirWindow` and `HandleFileWindow` have explicit line budgets; growth beyond baseline fails the guard and must be addressed by moving separable logic into dedicated modules.

### 1.2 Controller Ownership Rule (Dispatch-Only)

Controllers (`ctrl_*.c`) are orchestration boundaries, not ownership modules.

Acceptable controller contents:
*   Input/event dispatch loops.
*   Prompt/confirmation wiring that is inseparable from event flow.
*   UI-state coordination that cannot be reused outside the controller loop.

Unacceptable controller contents:
*   Reusable business logic (copy/move/compare/path transforms).
*   Generic helper utilities that could be called from non-controller modules.
*   Feature logic that can live in `src/cmd`, `src/fs`, `src/ui` helper modules, or a new dedicated module.

Canonical ownership test:
*   If a function could be called from another context without modification, it must not be introduced as a top-level controller function.

This does **not** replace architecture review. It is a fitness function: mechanical checks that fail fast when structure drifts.

## **2. Architectural Overview**
This document outlines the architectural design of `ytree`. The codebase utilizes a modular, context-oriented C99 design.

The primary objective is to maintain a **predictable, high-integrity state machine**. Every component is designed to uphold the **Focus vs. Freeze** logic and the specific hierarchy of modal priorities inherited from the XTree&trade; lineage.

---

## 2. Execution & Concurrency Model
`ytree` is strictly **SINGLE-THREADED**. This ensures deterministic state transitions and prevents race conditions within the Ncurses rendering pipeline.

*   **Sequential Logic:** All application logic, filesystem I/O, and UI rendering execute sequentially in the main thread.
*   **Signal Handling:** Signals (e.g., `SIGINT`, `SIGWINCH`) set atomic flags. No complex logic, I/O, or Ncurses calls are permitted inside signal handlers.
*   **Context-Passing Design:** All functions receive explicit context pointers (`ViewContext *ctx`, `YtreePanel *`, or `Volume *`) as their first argument. Global mutable state is prohibited. See **Section 3** for detailed rules and exemptions.

---

## 3. Context-Passing Architecture

`ytree` follows a strict **context-passing** (also called "context-oriented") architecture. This is the most important structural property of the codebase — it governs how every function accesses application state.

### 3.1 The Rule

> **Every function receives the state it operates on as an explicit parameter. No function may read or write application state through global variables.**

In practice, this means every function signature begins with `ViewContext *ctx` (or a more specific context like `YtreePanel *` or `Volume *`). The `ViewContext` is the root session object; it is allocated once in `main()`, passed by pointer into every call chain, and owns all application state through its member hierarchy.

This pattern provides:
*   **Testability** — Functions can be called with synthetic contexts.
*   **Panel independence** — Two panels cannot accidentally share state through hidden globals.
*   **Auditability** — Every data dependency is visible in the function signature.
*   **LLM/tooling clarity** — Static analysis and AI tools can trace data flow without resolving global symbol tables.

### 3.2 `ViewContext` — The Session Root

The `ViewContext` struct (defined in `include/ytree_defs.h`) is the root of all application state:

```
ViewContext (The Session)
├── left   → YtreePanel (View: cursor, scroll, window state)
├── right  → YtreePanel (View: cursor, scroll, window state)
├── active → points to left or right
├── volumes_head → Volume linked list (Model: DirEntry trees, statistics)
└── viewer, layout, mode flags, etc.
```

No component reaches "upward" or "sideways" through globals to find its siblings. All cross-component access goes through `ctx`.

### 3.3 Permitted Global Exceptions

Exactly **three** global variables exist in the codebase. Each has a specific technical justification and must not be extended:

| Variable | Type | File | Justification |
|---|---|---|---|
| `ui_colors[]` | `UIColor[]` | `src/ui/color.c` | Color palette table — written once during config parsing at startup, read-only thereafter. Shared across all UI code as immutable configuration. |
| `NUM_UI_COLORS` | `int` | `src/ui/color.c` | Derived from `sizeof(ui_colors)` — effectively a compile-time constant. |
| `ytree_shutdown_flag` | `volatile sig_atomic_t` | `src/core/main.c` | Set by the `SIGTERM`/`SIGINT` signal handler. POSIX signal handlers cannot receive context pointers; an atomic global flag is the mandated pattern for signal-to-mainloop communication. |

> **For contributors:** Adding new global variables is not permitted. If you need shared state, add a member to `ViewContext` (or `YtreePanel` / `Volume` as appropriate) and pass it through the call chain.

### 3.4 Historical Note

The original `ytree` codebase used pervasive global state (`CurrentVolume`, `statistic`, `dir_entry_list`, etc.) and functions with `void` parameter lists that silently operated on globals. Between 2024–2025, all 228+ function signatures were refactored to receive explicit context pointers, all global state was migrated into `ViewContext`, and the compatibility bridge (`GlobalView` pointer) was subsequently removed.

## 4. Core Architectural Data Hierarchy

### 4.1 Data Ownership
The application state is strictly hierarchical:

1.  **`ViewContext` (The Session):**
    *   The root object representing the application instance.
    *   Owns pointers to `left` and `right` panels and the `active` panel focus pointer.
    *   Owns the `volumes_head` registry of all loaded volumes.

2.  **`YtreePanel` (The View):**
    *   Represents a single UI pane.
    *   Owns **View State**: cursor position, scroll offset, and window-specific dimensions.
    *   Holds a reference to a `Volume`.
    *   Independent Panels may point to the same `Volume` while maintaining unique cursors.

3.  **`Volume` (The Model):**
    *   Represents a filesystem (Physical Disk or Archive).
    *   Owns **Data**: `DirEntry` tree structure, `Statistic` metadata, and path info.
    *   Contains no UI state.

### 4.2 Dual-Panel Context Isolation (F8 Logic)
The Split-Screen architecture treats each panel as an independent instance of a volume manager.

*   **Active Panel:** Owns keyboard focus and initiates all operations.
*   **Inactive Panel:** Strictly **DORMANT**. It does not update its display or change state in response to activity in the active panel.
*   **State Persistence (Tab-Switch):** The `Tab` key is the bridge. Switching panels restores the exact state held when that panel last had focus.

### 4.3 Inter-Panel Operations (The Directional Rule)
*   **Targeting:** Copy and Move operations occur directionally: **Source (Active Panel) to Destination (Inactive Panel)**.
*   **Read-Only Bridge:** The active panel reads the path of the inactive panel to set a default destination without altering the inactive panel's state.

---

## 5. Behavioral Protocols

### 5.1 Protocol A: Directory Entry and "No Files" Constraint
*   **Transition Invariant:** A directory can only be entered (Tree to File Mode) if it contains at least one file.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel restores the cursor to the last highlighted file.
*   **Navigation Stability:** Moving through the Tree never automatically triggers a transition into File Mode.

### 5.2 Protocol B: Archive and Volume Lifecycle
*   **Lifecycle Management:** The active panel handles Logging new volumes, Cycling through logged volumes, and Releasing (unlogging) volumes.
*   **Independent Rooting:** Changing the root or volume in the active panel has no impact on the inactive panel.

### 5.3 Protocol C: F7 Autoview
*   **Contextual Logic:** F7 displays content for files and shows a file list temporarily for directories.
*   **Dynamic Background Navigation:** While F7 is active, Up/Down keys move the cursor; the preview updates in real-time.
*   **Undo Protocol:** Pressing `F7` or `Esc` destroys the overlay and returns the user to the **exact** position and mode held before the preview. No state changes persist.

---

## 6. Visual and Rendering Standards
*   **Terminal Integrity:** UI updates are staged using `wnoutrefresh()` and committed atomically via `doupdate()` to prevent visual artifacts.
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) are used only for horizontal boundary lines. Vertical separators must remain clean.
*   **Micro-Consistency:** Mode flags must be synchronized with the layout before any redraw.

---

## 7. Directory Structure
*   **build/**: Compiled binary outputs.
*   **doc/**: Project documentation and specifications.
*   **etc/**: Default configuration files.
*   **include/**: C header files.
*   **obj/**: Intermediate object files.
*   **src/**: Source code:
    *   `src/core/`: Initializers and session management.
    *   `src/fs/`: File system and archive handling.
    *   `src/ui/`: Ncurses rendering and window management.
    *   `src/cmd/`: User command implementations.
    *   `src/util/`: Utilities and history_utils management.
*   **tests/**: Behavioral TUI tests.
