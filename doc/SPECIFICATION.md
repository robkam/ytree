# Functional & Behavioral Specification of Ytree

## 0. Introduction
`ytree` is a TUI File Manager following the **XTree/ZTree/UnixTree** lineage. It acts as a strict **State Machine**. This document serves as the "Contract of Truth" for behavior. Any deviation in UI logic or visual representation is a bug.

### 0.1 Technical Foundation
*   **Standard:** C99 (via `gcc` or `clang`).
*   **Flags:** `-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64`.
*   **Encapsulation:** Direct access to global memory is strictly prohibited. All functions must derive state from explicitly passed pointers (ViewContext, YtreePanel, or Volume).
*   **Isolation:** Usage of Global Variables is strictly prohibited. All logic must derive state from passed Context pointers.
---

## 1. The User Interface Architecture

### 1.1 The Layout Grid
The screen is divided into non-overlapping zones. Geometry is calculated dynamically, except for the Stats Panel.

| Zone | Geometry | Content | Behavior |
| :--- | :--- | :--- | :--- |
| **Header** | Row 0 | Volume, Path, Clock, Version | Updates on every navigation event. |
| **Tree View** | Top-Left | Visual directory hierarchy | Primary Navigation anchor. |
| **File View** | Bottom-Left | File list of selected directory | Shows "** No files **" if empty; "** Not logged **" if unread. |
| **Stats Panel** | Right Column (**Fixed 26**) | Metadata, Filters, Disk Stats | Context-aware. Always visible in Standard Mode. |
| **Command Area** | Bottom 3 Rows | Menu, Prompts, Messages | Handles all user interaction feedback. |

### 1.2 Visual Grammar (The "XTree Look")
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) must **only** be used for horizontal boundary lines. Vertical separators must remain clean, unbroken lines to avoid visual clutter.
*   **Empty State:** If a directory contains no files, the File View window must display the text: `** No files **`.
*   **Micro-Consistency:** UI state flags (e.g., `big_window`, `split_mode`) must be synchronized with the internal state machine before any call to `doupdate()`.

### 1.3 Tree Status Column
The first character column of the Tree View serves as the Memory State Indicator:
*   `+` : **Unlogged.** The directory entry is visible in the tree, but its file list is not in memory. The File View must display `** Not logged **`.
*   ` ` (Blank): **Logged.** The file list for this directory is resident in memory.
*   Trailing `/`: Visual indicator that the directory contains subdirectories on the disk.

---

## 2. Navigation & Focus Logic

### 2.1 Focus Flow (`NOSMALLWINDOW`)
The behavior of the `Enter` key on a directory node is governed by the configuration:

*   **Bypass Mode (`NOSMALLWINDOW=1`):**
    *   `Enter` on Tree -> **Instant Zoom**. File Window expands to full height.
    *   `Enter` or `Esc` on Zoomed Window -> Returns focus to the **Tree View**.
*   **Staged Navigation (`NOSMALLWINDOW=0`):**
    *   `Enter` on Tree -> **Focus Shift**. Focus moves to the File View (Small Window). Tree remains visible.
    *   `Enter` on Small Window -> **Zoom**. File Window expands to full height.
*   **Navigation Stability:** Moving the cursor through the Tree must **never** automatically trigger a transition into File Mode or Zoom.

### 2.2 Directory Protocols
*   **Logging vs. Entry:** "Logging" is the act of scanning a directory branch. Any directory can be logged and exist in the Tree. However, **Entry** (transitioning focus from Tree to File View) is strictly prohibited if the directory contains zero files.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel must restore the cursor to the **last highlighted file**.

### 2.3 Directory Memory Commands
*   **`+` (Plus):** Shallow Log. Scans the files of the current directory and the names of immediate subdirectories.
*   **`*` (Asterisk):** Deep Log. Recursively scans the entire branch.
*   **`-` (Minus):** Release. Evicts the file list of the current directory and all its subdirectories from memory. Sets the status indicator to `+`.

---

## 3. Split-Screen (F8) & Session Model

### 3.1 The Active-Inactive Rule
The Split-Screen architecture treats each panel as an independent instance of a volume manager.
*   **Active Panel:** Owns keyboard focus. Updates visuals immediately.
*   **Inactive Panel (The "Frozen" Rule):** Strictly **DORMANT**. It must not update its file list, scroll, or change selection while inactive.
*   **Lazy Refresh:** The inactive panel only validates or re-reads its directory content from the disk the moment it regains focus via `Tab`.

### 3.2 State Persistence
Switching panels via `Tab` must restore the exact state held when that panel last had focus, including:
*   **Volume Context:** Logged volume or archive.
*   **Cursor & Offset:** Highlighted entry and scroll position.
*   **Selection:** Tags are specific to the panel session.
*   **Filter (Filespec):** Independent search/filter strings.

### 3.3 Modal Search Behavior
*   **Persistence:** The search string is retained after the mode is exited.
*   **Sticky Cursor:** If a character is typed that produces no match, the cursor remains at the last successful match.
*   **Implicit Exit:** Pressing a key associated with a file operation (Copy, Delete, Move) confirms the current search match and immediately executes that command.

---

## 4. Notification & Messaging Tiers
`ytree` distinguishes between two primary locations for communication:

### 4.1 Footer Messages (Command Area)
*   **Transient:** Non-critical status (e.g., "File copied"). Appears in the Message row. Disappears on the next keystroke.
*   **Sticky/Warning:** Requires acknowledgment or input (e.g., "Delete file? Y/N" or "Path not found"). Stays in the footer until the user responds or hits a key to clear the warning.

### 4.2 Modal Messages (Centered Box)
A bordered pop-up box that overlays the center of the screen, used for:
*   **Info:** Detailed system information or multi-line status.
*   **Warning:** Significant operational warnings that require explicit dismissal.
*   **Error:** Critical failures (e.g., "Permission Denied" or "Archive Corrupt").
*   **Constraint:** Modals must be dismissed with `Esc` or `Enter` before any other navigation can occur.

---

## 5. The Virtual Filesystem (VFS)
*   **Archive Integration:** Archives are treated as directories. Entering an archive logs it as a Virtual Volume. `Left Arrow` at the root of an archive "Backs Out" to the parent physical volume.
*   **Stream Rewrite:** Modifications to archives use an atomic rewrite strategy to ensure data integrity.
*   **Live View:** Use `inotify` (where available) for automatic refreshes. If kernel limits are hit, the system falls back to manual refresh logic safely.

---

## 6. Filtering & Command Execution
*   **Filter Stack:** Cumulative logic applies: `Filespec AND Attribute Mask AND Date/Size AND Regex`.
*   **Grep Tagged (`^S`):** A non-destructive content filter applied to the currently tagged set.
*   **Targeting:** In Split-Screen, Copy/Move operations in the Active Panel use the Inactive Panel's current path as the default destination.
*   **User Menu (`F9`):** Supports macro expansion: `%f` (file), `%d` (dir), `%t` (tagged list), `%p` (inactive panel path).

---

## 7. Safety & Integrity
*   **Signal Handling:** `SIGINT` and `SIGTERM` are trapped for graceful terminal restoration and VFS cleanup.
*   **Memory Management:** Recursive scans for the Tree View respect the `TREEDEPTH` safety limit to prevent stack overflows or OOM (Out of Memory) conditions on massive filesystems.
*   **Encapsulation:** Global state pointers are strictly forbidden. All logic must utilize the `ViewContext` structure passed explicitly through the call stack.
