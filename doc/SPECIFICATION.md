# Functional & Behavioral Specification of Ytree

## 0. Introduction
`ytree` is a TUI File Manager following the **XTree/ZTree/UnixTree** lineage. It acts as a strict State Machine. This document is the "Contract of Truth" for behavior. Any deviation in the UI or logic is a bug.

---

## 1. The User Interface Strategy (XTree Standard)
Unlike orthodox dual-pane managers (e.g., Midnight Commander), `ytree` follows the asymmetric **XTree Layout Strategy**. The screen is divided into functional zones separated by line borders.

### 1.1 The Standard Layout (The "View")
The default screen is composed of distinct, border-separated zones:

1.  **The Header (Top Line):**
    *   Left: Full absolute path of the current directory.
    *   Right: Date, Time, and Version info.
2.  **The Directory Window ("Big Window"):**
    *   Location: Top-Left (~80% width).
    *   Content: Visual tree hierarchy (Root -> Branch -> Leaf).
    *   Behavior: Primary navigation. Scrolling here updates the File Window immediately.
3.  **The File Window ("Small Window"):**
    *   Location: Bottom-Left (~80% width), located strictly **underneath** the Big Window.
    *   Content: The files contained within the currently highlighted Directory node.
    *   **Constraint:** If the directory is empty, this window displays the text "**No files**".
4.  **The Stats Panel:**
    *   Location: Vertical column on the far Right (~20% width), full height between Header and Footer.
    *   Content: Active Filter (Filespec), Volume Info, Disk Stats, Selected File Attributes.
5.  **The Command Footer:**
    *   Location: Bottom 3 lines of the screen.
    *   States: Passive (Menu), Input (Prompt), Message (Status).

### 1.2 Layout Modes & Transitions
*   **Standard Mode:** The Tree (Big Window) dominates. The File Window (Small Window) shows a few lines of context.
*   **File Mode (The "Zoom"):**
    *   **Default Behavior:** Pressing `Enter` on a directory expands the File Window to consume the Tree Window's space.
    *   **Configurable:** Users may disable Zoom to navigate the Small Window without obscuring the Tree.
*   **Split Mode (F8):**
    *   The screen splits vertically into two independent panels.
    *   Each panel contains its own compressed version of the Standard Layout (Tree + File + Stats).
    *   **The Golden Rule:** Both panels **NEVER** update simultaneously. Only the panel with the cursor (Active) updates its view. The Inactive panel is a static snapshot (Frozen) until focus is switched via `Tab`.

---

## 2. Core Logic Invariants

### 2.1 Focus vs. Freeze (The F8 Rule)
The Split-Screen architecture treats each panel as an independent instance of a volume manager.

*   **Active Panel:** Owns keyboard focus. Updates visuals immediately.
*   **Inactive Panel:** Strictly **DORMANT**.
    *   Must NOT update its file list, scroll, or change selection while inactive.
    *   **Lazy Refresh:** The inactive panel only validates/re-reads its directory content when it regains focus (via `Tab`).
*   **State Persistence:** Switching panels via `Tab` must restore the **exact** state held when that panel last had focus. This includes:
    *   **Volume Context:** The specific logged volume or archive.
    *   **Cursor Position:** The highlighted entry.
    *   **View Offset:** The scroll position (display-begin).
    *   **Tagging Selection:** File selections/tags are panel-specific.
    *   **Filespec:** Filter strings (e.g., `*.c`).

### 2.2 The Modal Hierarchy
Priority order of UI states (Highest to Lowest):
1.  **Level 1: F7 Autoview (Preemptive):** Overlays everything. Input is locked to preview navigation. `Tab` is disabled.
2.  **Level 2: F8 Split (Structural):** Two independent panels exist.
3.  **Level 3: Standard Navigation:** Moving within a logged volume.

---

## 3. Behavioral Protocols

### 3.1 Protocol A: Directory Entry
*   **Transition Invariant:** A directory can only be "entered" (Tree to File Mode) if it contains at least one file.
*   **Empty Directory Behavior:** If empty, the program blocks the transition. The file list shows "No files," the cursor remains in the Tree, and context does not change.
*   **Selection Memory (Breadcrumbs):** When escaping File Mode to Tree Mode and later re-entering the same directory, the panel must restore the cursor to the **last highlighted file**.
*   **Navigation Stability:** Moving through the Tree must never automatically trigger a transition into File Mode.

### 3.2 Protocol B: Panel Context & Lifecycle
*   **Isolation:** Changing the volume/drive in the Active Panel must NEVER change the volume/drive in the Inactive Panel.
*   **Targeting:** Copy/Move operations in the Active Panel use the Inactive Panel's current path as the default destination.
*   **Active Logging:** Only the active panel can "Log" a new volume or open an archive.

### 3.3 Protocol C: F7 Autoview
*   **Contextual Logic:** F7 displays content for files and a **'Peek'** (temporary file list) for directories.
*   **Dynamic Background Navigation:** While F7 is active, Up/Down keys move the cursor; the preview updates in real-time.
*   **Undo Protocol:** Pressing `F7` or `Esc` destroys the overlay and returns the user to the **exact** position and mode held before the preview.

---

## 4. Visual Standards
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) should **only** be used for horizontal boundary lines. Vertical separators must remain clean lines to avoid visual clutter.
*   **Micro-Consistency:** Mode flags (`big_window`, `preview_mode`) must be synchronized with the layout before any redraw.
