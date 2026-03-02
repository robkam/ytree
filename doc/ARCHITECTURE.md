# Architecture of ytree v3

## 1. Overview
This document outlines the architectural design of `ytree`. The codebase utilizes a modular, context-oriented C99 design.

The primary objective is to maintain a **predictable, high-integrity state machine**. Every component is designed to uphold the **Focus vs. Freeze** logic and the specific hierarchy of modal priorities inherited from the XTree lineage.

---

## 2. Execution & Concurrency Model
`ytree` is strictly **SINGLE-THREADED**. This ensures deterministic state transitions and prevents race conditions within the Ncurses rendering pipeline.

*   **Sequential Logic:** All application logic, filesystem I/O, and UI rendering execute sequentially in the main thread.
*   **Signal Handling:** Signals (e.g., `SIGINT`, `SIGWINCH`) set atomic flags. No complex logic, I/O, or Ncurses calls are permitted inside signal handlers.
*   **Context-Oriented Design:** Functions operate exclusively on passed Context pointers (`ViewContext`, `YtreePanel`, or `Volume`). Global state variables are not permitted.

---

## 3. Core Architectural Data Hierarchy

### 3.1 Data Ownership
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

### 3.2 Dual-Panel Context Isolation (F8 Logic)
The Split-Screen architecture treats each panel as an independent instance of a volume manager.

*   **Active Panel:** Owns keyboard focus and initiates all operations.
*   **Inactive Panel:** Strictly **DORMANT**. It does not update its display or change state in response to activity in the active panel.
*   **State Persistence (Tab-Switch):** The `Tab` key is the bridge. Switching panels restores the exact state held when that panel last had focus.

### 3.3 Inter-Panel Operations (The Directional Rule)
*   **Targeting:** Copy and Move operations occur directionally: **Source (Active Panel) to Destination (Inactive Panel)**.
*   **Read-Only Bridge:** The active panel reads the path of the inactive panel to set a default destination without altering the inactive panel's state.

---

## 4. Behavioral Protocols

### 4.1 Protocol A: Directory Entry and "No Files" Constraint
*   **Transition Invariant:** A directory can only be entered (Tree to File Mode) if it contains at least one file.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel restores the cursor to the last highlighted file.
*   **Navigation Stability:** Moving through the Tree never automatically triggers a transition into File Mode.

### 4.2 Protocol B: Archive and Volume Lifecycle
*   **Lifecycle Management:** The active panel handles Logging new volumes, Cycling through logged volumes, and Releasing (unlogging) volumes.
*   **Independent Rooting:** Changing the root or volume in the active panel has no impact on the inactive panel.

### 4.3 Protocol C: F7 Autoview
*   **Contextual Logic:** F7 displays content for files and shows a file list temporarily for directories.
*   **Dynamic Background Navigation:** While F7 is active, Up/Down keys move the cursor; the preview updates in real-time.
*   **Undo Protocol:** Pressing `F7` or `Esc` destroys the overlay and returns the user to the **exact** position and mode held before the preview. No state changes persist.

---

## 5. Visual and Rendering Standards
*   **Terminal Integrity:** UI updates are staged using `wnoutrefresh()` and committed atomically via `doupdate()` to prevent visual artifacts.
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) are used only for horizontal boundary lines. Vertical separators must remain clean.
*   **Micro-Consistency:** Mode flags must be synchronized with the layout before any redraw.

---

## 6. Directory Structure
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
    *   `src/util/`: Utilities and history management.
*   **tests/**: Behavioral TUI tests.