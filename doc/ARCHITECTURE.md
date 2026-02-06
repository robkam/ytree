# Architecture of ytree v3

## 1. Overview
This document outlines the architectural modernization of `ytree`. The codebase has transitioned from a 1990s-style monolithic structure to a modular, standards-compliant C99 design.

The primary objective is to maintain a **predictable, high-integrity state machine**. Every component is designed to uphold the **Focus vs. Freeze** logic and the specific hierarchy of modal priorities inherited from the XTree lineage.

---

## 2. Execution & Concurrency Model
`ytree` is strictly **SINGLE-THREADED**. This ensures deterministic state transitions and prevents race conditions within the Ncurses rendering pipeline.

*   **Sequential Logic:** All application logic, filesystem I/O, and UI rendering execute sequentially in the main thread.
*   **Signal Handling:** Signals (e.g., `SIGINT`, `SIGWINCH`) are used only to set atomic flags. No complex logic, I/O, or Ncurses calls are permitted inside signal handlers.
*   **Global Synchronization:** Before rendering or processing commands, the controller must ensure the Global Volume pointer (`CurrentVolume`) is explicitly synchronized with the **Active Panel**.

---

## 3. Core Architectural Invariants

### 3.1 Dual-Panel Context Isolation (F8 Logic)
The Split-Screen architecture treats each panel as an independent instance of a volume manager.

*   **Active vs. Inactive (Focus vs. Freeze):**
    *   **Active Panel:** Owns keyboard focus and initiates all operations.
    *   **Inactive Panel:** Strictly **DORMANT**. It must never update its display, refresh pointers, or change state in response to activity in the active panel.
*   **Independent State:** Panels maintain completely isolated state variables, even if both panels are viewing the same path:
    *   **Volume Context:** The specific logged volume or archive.
    *   **Cursor Position:** The highlighted entry.
    *   **View Offset:** The scroll position (display-begin).
    *   **Tagging Selection:** File selections/tags are panel-specific.
    *   **Filespec:** Filter strings (e.g., `*.c`).
*   **State Persistence (Tab-Switch):** The `Tab` key is the *exclusive* bridge. Switching panels must restore the **exact** state held when that panel last had focus.
*   **The Lazy Refresh Rule:** An inactive panel remains in its frozen state upon being "Tabbed" into. It validates/refreshes its view only when a subsequent user action forces a re-read.

### 3.2 Inter-Panel Operations (The Directional Rule)
*   **Targeting:** Operations such as Copy and Move occur directionally: **Source (Active Panel) to Destination (Inactive Panel)**.
*   **Read-Only Bridge:** The active panel may read the path of the inactive panel to set a default destination, but this operation must not trigger a refresh or state change in the inactive panel.

### 3.3 The Modal Hierarchy (Command Priority)
1.  **Level 1: F7 Autoview (Preview Mode) [PREEMPTIVE]:** Creates a modal lock. `Tab` is disabled; panel switching is prohibited.
2.  **Level 2: F8 Split-Window [STRUCTURAL]:** Defines the dual independent volume environments.
3.  **Level 3: The Log/Volume [DATA]:** All navigation and filtering are relative to the currently logged volume.

---

## 4. Behavioral Protocols

### 4.1 Protocol A: Directory Entry and "No Files" Constraint
*   **Transition Invariant:** A directory can only be "entered" (Tree to File Mode) if it contains at least one file.
*   **Empty Directory Behavior:** If empty, the program blocks the transition. The file list shows "No files," the cursor remains in the Tree, and context does not change.
*   **Selection Memory (Breadcrumbs):** When escaping File Mode to Tree Mode and later re-entering the same directory, the panel must restore the cursor to the **last highlighted file**.
*   **Navigation Stability:** Moving through the Tree must never automatically trigger a transition into File Mode.

### 4.2 Protocol B: Archive and Volume Lifecycle
*   **Active Logging:** Only the active panel can "Log" a new volume or open an archive as a virtual volume.
*   **Lifecycle Management:** The active panel supports **Logging** new volumes, **Cycling** through logged volumes, and **Releasing** (unlogging) a volume.
*   **Independent Rooting:** Changing the root/volume in the active panel has zero impact on the inactive panel.

### 4.3 Protocol C: F7 Autoview (The "No Surprise" Preview)
*   **Contextual Logic:** F7 displays content for files and a **'Peek'** (temporary file list) for directories.
*   **Dynamic Background Navigation:** While F7 is active, Up/Down keys move the cursor; the preview updates in real-time.
*   **Undo Protocol:** Pressing `F7` or `Esc` destroys the overlay and returns the user to the **exact** position and mode held before the preview. No state changes persist.

---

## 5. Visual and Rendering Standards
*   **Terminal Integrity:** UI updates must be atomic to prevent "ghost" characters.
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) should **only** be used for horizontal boundary lines (separating Tree and File areas). Vertical separators must remain clean.
*   **Micro-Consistency:** Mode flags (`big_window`, `preview_mode`) must be synchronized with the layout before any redraw.

---

## 6. Operational Principles (Architectural SDD)
*   **Spec-First Fixes:** Before implementing a fix, the developer must verify if the current behavior violates the Invariants or Protocols defined in the Specification.
*   **Side-Effect Analysis:** Every change must be checked against the "Dual-Panel Isolation" rule.
*   **Predictability:** Do not introduce "smart" behaviors that move the cursor or change modes automatically. Stay within manual boundaries.

---

## 7. Directory Structure
*   **build/**: Compiled binary outputs and distribution packages.
*   **doc/**: Project documentation, specifications, and screenshots.
*   **etc/**: Default configuration files and sample `ytree.conf`.
*   **include/**: C header files defining the internal API and data structures.
*   **obj/**: Intermediate object files generated during compilation.
*   **scripts/**: Python utility scripts and development environment setup.
*   **src/**: Source code partitioned by functional responsibility:
    *   `src/core/`: Initializers, global state, and session management.
    *   `src/fs/`: File system abstractions, archive handling, and watchers.
    *   `src/ui/`: Ncurses rendering logic and window management.
    *   `src/cmd/`: Implementation of specific user commands (Copy, Move, etc.).
    *   `src/util/`: String manipulation, path normalization, and history.
*   **tests/**: Behavioral TUI tests using Python and `pexpect`.

