# **Functional Specification**
> **Purpose:** This document defines the behavioral "Contract of Truth" for `ytree`. It specifies how the UI should respond to input, how the filesystem is represented, and the design philosophy that governs the user experience.
> This specification defines behavior contracts only; detailed regression-test inventories and case matrices belong in roadmap/planning artifacts and the test suite, not in this file.

## **1. Design Philosophy**
The `ytree` interface is built to make the power of the Unix filesystem accessible through a high-speed, intuitive terminal interface.

*   **Unix-First Design:** Prioritize a user experience tailored for Unix power users, emphasizing shell integration, standard POSIX conventions, and scriptability.
*   **Ytree makes filesystem work self-evident:** Users should not need command-line fluency or Unix jargon to succeed; core actions must be visible, named plainly, and understandable from the interface itself.
*   **Interaction Economy (Minimize Friction):** `ytree` is designed to minimize the distance between user intent and execution. Avoid unnecessary confirmations for safe operations and ensure the common path is always `key -> Enter -> result`.
*   **Direct Access (No Menu Diving):** High-speed keyboard access is superior to hierarchical navigation. Core functionality must be accessible via single-key or simple combinations; UI depth should never exceed one level for primary actions.
*   **No Hidden Features:** All functionality, especially syntax like the `{}` placeholder, should be explained in context within the UI (e.g., in help lines or prompts).

## **2. The User Interface Architecture**

### 2.1 Input Semantics

ytree separates **view-state toggles** from **one-shot actions**:

*   **`Enter` toggles Tree/File focus states** in normal navigation flow.
*   **`F7`/`F8` are toggle view modes:** Preview and Split Screen are stateful layout modes toggled by repeating the same key.
*   **`s`/`g` are mode-entry keys, not same-key toggles:** they enter Showall/Global file-list states; behavior of repeated `s`/`g` follows that state's local keymap.
*   **Actions are one-way:** repeating the same key may run another action, insert input into an active prompt, or do nothing; it never means undo.
*   **Esc** is universal cancel/return. It does **not** undo completed filesystem mutations.

### 2.2 The Layout Grid
The screen is divided into non-overlapping zones. Geometry is calculated dynamically, except for the Stats Panel.

| Zone | Geometry | Content | Behavior |
| :--- | :--- | :--- | :--- |
| **Header** | Row 0 | Volume, Path, Clock, Version | Updates on every navigation event. |
| **Tree View** | Top-Left | Visual directory hierarchy | Primary Navigation anchor. |
| **File View** | Bottom-Left | File list of selected directory | Shows "** No files **" if empty; "** Unlogged **" if unread. |
| **Stats Panel** | Right Column (**Fixed 26**) | Metadata, Filters, Disk Stats | Context-aware. Always visible in Standard Mode. |
| **Command Area** | Bottom 3 Rows | Menu, Prompts, Messages | Handles all user interaction feedback. |

### 2.3 Visual Grammar (The "XTree&trade; Look")
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) must **only** be used for horizontal boundary lines. Vertical separators must remain clean, unbroken lines to avoid visual clutter.
*   **Empty State:** If a directory contains no files, the File View window must display the text: `** No files **`.
*   **Small-Window Name Column Alignment:** In the small File View, reserve the first post-border cell for tag (`*` or space), the second as a spacer, and start all row text at the same column. This applies to regular names, symlink labels, and placeholders (e.g., `No files`, `Unlogged`).  
    Untagged: `│  check_xml_integrit`, `│  @current`, `│  No files`, `│  Unlogged`  
    Tagged: `│* check_xml_integrit`, `│* @current`, `│  No files`, `│  Unlogged`
*   **Single-Row List Invariant:** Tree/File/Showall/archive list rows must never wrap to a second terminal line.
*   **Informative Truncation Policy:** When width is insufficient, content must be truncated (not wrapped) using a deterministic strategy that preserves the most useful identity cues. Prefer `prefix…suffix` elision when both ends carry meaning (for example filename stem + extension or path tail); use one-sided clipping only when the omitted side is low-value in that context.
*   **Static Text Rule:** Truncated UI labels are static and stable while focused; marquee/auto-scrolling text is not permitted for core list and attribute surfaces.
*   **Motion-Only-When-Informative Rule:** UI animation is avoided by default. Motion is permitted only when it conveys live operational state (for example scanning/copying progress, spinner/ETA/progress counter) rather than decorative movement.
*   **Progress Indicator Selection Rule:** Use a spinner when duration is unknown (default). Use a progress bar/percent/ETA when total work is measurable.
*   **Progress Surface Ownership Rule:** Progress/spinner updates MUST NOT overwrite or hide footer help, active prompt text, or F1 help surfaces. If layout is constrained, degrade to a compact indicator instead of replacing those surfaces.
*   **Regression Guard:** No-wrap/truncate behavior is a required regression-test contract across normal and archive view modes.
*   **Micro-Consistency:** UI state flags (e.g., `big_window`, `split_mode`) must be synchronized with the internal state machine before any call to `doupdate()`.

### 2.4 Tree Status Column
The first character column of the Tree View serves as the Memory State Indicator:
*   `+` : **Unlogged.** The directory entry is visible in the tree, but its file list is not in memory. The File View must display `**Unlogged**`.
*   ` ` (Blank): **Logged.** The file list for this directory is resident in memory.
*   Directory-name suffix contract: `+` is a status-margin marker (not a name suffix). A trailing `/` may still be shown on a directory name to indicate subdirectory presence.

---

## 3. Navigation & Focus Logic

### 3.1 Focus Flow (`SMALLWINDOWSKIP`)
The behavior of the `Enter` key on a directory node is governed by the configuration:

*   **State gate (all configs):** If the selected directory is unlogged/not-yet-scanned, `Enter` performs one-level log/reveal (same as `+`) and keeps focus in Tree View.
*   **Bypass Mode (`SMALLWINDOWSKIP=1`):**
    *   `Enter` on a logged directory in Tree -> **Instant Zoom**. File Window expands to full height.
    *   `Enter` or `Esc` on Zoomed Window -> Returns focus to the **Tree View**.
*   **Staged Navigation (`SMALLWINDOWSKIP=0`):**
    *   `Enter` on a logged directory in Tree -> **Focus Shift**. Focus moves to the File View (Small Window). Tree remains visible.
    *   `Enter` on Small Window -> **Zoom**. File Window expands to full height.
*   **Navigation Stability:** Moving the cursor through the Tree must **never** automatically trigger a transition into File Mode or Zoom.

### 3.2 Directory Protocols
*   **Logging vs. Entry:** "Logging" is the act of scanning a directory branch. Any directory can be logged and exist in the Tree. However, **Entry** (transitioning focus from Tree to File View) is strictly prohibited if the directory contains zero files.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel must restore the cursor to the **last highlighted file**.

### 3.3 Directory Memory Commands (Structural Controls)
*   **`+` or `=` (Expand):** Shallow Log. Scans the files of the current directory and the names of immediate subdirectories. `=` is a convenience alias (unshifted `+` on most keyboard layouts).
*   **`*` (Asterisk):** Deep Log. Recursively scans the entire branch.
*   **`-` (Minus / Collapse):** State-based memory release. First press collapses an expanded node. Second press on a collapsed (but logged) node evicts the file list, sets the status indicator to `+`, and marks the directory as Unlogged. At root, use `-` to release logged contents.

### 3.4 Arrow Key Navigation (Spatial Controls)
Arrow keys provide spatial, cursor-oriented navigation through the tree. They are distinct from the structural `+`/`-`/`*` controls:
*   **`→` (Right Arrow / Drill Down):** Progressive depth navigation. If the node is collapsed: expand one level. If already expanded: move cursor to the first child.
*   **`←` (Left Arrow):** If the selected directory is expanded, collapse it. Otherwise, move selection to its parent directory. At the filesystem/archive root, `Left` follows progressive retreat: first collapse visible children, then release/unlog, then no-op.

### 3.5 Preview Mode (`F7`) Contract
`F7` is the primary inspect mode for viewing file contents while retaining list-oriented navigation.
*   **Activation/Toggle:** `F7` enters preview mode and `F7` again exits preview mode.
*   **Layout Contract:** Preview mode presents a file-list pane and a content-preview pane simultaneously.
*   **List-First Navigation:** Standard list navigation changes selection in the list pane; preview content updates to the selected file.
*   **Preview-Scroll Modifiers:** Shift-modified navigation keys (and configured equivalents such as `^P`/`^N`) scroll preview content without changing list selection.
*   **Mode Safety Rule:** Split-navigation controls are not active while preview mode is active (for example `F8` and `Tab` must not perform split/layout switching from inside preview mode).
*   **Command Availability Rule:** Preview mode may expose a defined in-context command subset; unavailable commands must not trigger unintended mode/layout transitions.

---

## 4. Keyboard Interaction Taxonomy

The `ytree` input system follows a layered model designed for high-speed interaction and contextual efficiency.

### 4.1 Input Principles
*   **Case-Sensitivity:** Keys are **case-insensitive** by default. Lowercase notation is used for letter-based commands (e.g., `c` for copy). The Ctrl key is shown by the `^` symbol.
*   **Standard Conventions**: Function keys use the `F1`-`F12` (uppercase prefix) notation. Control keys use the `^key` (e.g., `^l`) lowercase notation.
*   **Alt-Key Portability Rule:** `Alt`/Meta key sequences are terminal-dependent and are not part of supported key contracts. Core workflows must use non-`Alt` bindings.
*   **Contextual Logic:** The effect of a key depends on whether focus is on the Tree View or File View.

### 4.2 Interaction Layers

| Category | Definition | Behavioral Persistence |
| :--- | :--- | :--- |
| **Linguistic Mnemonics** | Keys bound to command strings (e.g., `c`=copy, `m`=move). | Primary candidates for l10n/i18n re-mapping. |
| **Structural Controls** | Positional keys (`+`/`=`, `-`, `*`) that manipulate the tree. | Static; universal regardless of locale. |
| **Spatial Navigation** | Arrow keys (`←`, `→`) for cursor-oriented tree traversal. | Fixed; directional drill-down / retreat. |
| **TUI Conventions** | Universal terminal muscle memory (`/`, `^l`, `^v`, `^q`). | Fixed; standard Unix utility behavior. |
| **State Toggles** | Binary or stateful switches (`` ` ``, `F6`, `F7`, `F8`). | Stateful; toggles UI display modes. |
| **Control Aliases** | ASCII Control characters as functional aliases. | Fixed at the terminal protocol level (e.g., `^m` = Enter). |
| **Prompt Interactions** | Contextual shortcuts active only during text prompts. | Specialized editing and browsing tools. |

### 4.3 Key Behavioral Rules
*   **The Minus Rule (`-`):** State-based memory release. First press collapses an expanded node; second press on a collapsed logged node evicts the file list (sets `+` status) and marks the directory as Unlogged.
*   **The Right Arrow Rule (`→`):** Progressive drill-down. Expand collapsed → move to child. Always takes the user one step deeper into the tree.
*   **The Root-Left Rule (`←` at root):** First `Left` collapses visible root children. Second `Left` releases/unlogs root contents. Further `Left` on already-unlogged root is a no-op.
*   **The Plus/Equals Rule (`+`/`=`):** Explicit one-level expand only. No cursor movement. `=` is the unshifted alias for `+`.
*   **The Tree Marker Rule (`+` status):** Unlogged state is rendered only in the dedicated tree status margin column; directory names do not carry a `+` suffix.
*   **The Volume Menu Rule (`K` menu):** Selecting the already-active volume preserves its current in-memory state (no implicit relog/reload).
*   **The Explicit Relog Rule (`L` on current path):** Logging an already logged volume/path performs a fresh relog/reload of that volume state and reanchors selection at volume root.
*   **The Invert Rule (`i`/`I`):** In both tree and file windows, invert tags applies to the active panel's current file-list scope (filesystem and archive contexts).
*   **The Only-Tagged Rule (`o`/`O`):** In both tree and file windows, toggle tagged-only file-list view for the active panel's current scope; toggling never changes tag state.
*   **The Archive/Global Jump (`\`):** In Archive Mode, jumps to the archive root. in Global/Showall views, jumps to the highlighted file's directory.
*   **Numeric FileInfo Band (`1..9`, `0`):** Number keys are the canonical file-display controls in normal list contexts (not active in `F7` preview). These controls apply to file-display rendering for the active panel whether focus is currently in the tree/dir window or file window.
*   **Vi-Key Collision Policy:** When `VI_KEYS=1`, lowercase `h/j/k/l` are reserved for navigation. Uppercase `H/K/L/J` are used for commands (Hex, Volume, Log, Compare).
*   **Tagged Actions**: `^u` (Untag All) and `^d` (Delete All Tagged) provide batch operations across the visible scope.
*   **Quit to Directory (`^q`):** Exits `ytree` to the currently highlighted directory (requires shell-level support to finalize the shell path).

### 4.4 Function Key Blueprint (F1-F12)
*   **`F1`**: help.
*   **`F2`**: directory picker (Prompts Only).
*   **`F5`**: refresh.
*   **`F6`**: stats toggle.
*   **`F7`**: autoview toggle.
*   **`F8`**: split-screen toggle.
*   **`F9`**: user menu (Macros).
*   **`F10`**: configuration.
*   **`F12`**: incremental jump (Legacy alias for `/`).

### 4.5 Prompt Interaction Standards
When a text prompt is active, specialized conventions ensure a refined editing experience:
*   **Line Editing**: `^a` / `^e` (start/end), `^k` / `^u` (kill to end/start), `^w` (kill word).
*   **History**: `^p` or `Up` arrow recalls contextual history (e.g., previous filters).
*   **Browsing**: `F2` or `^f` opens the directory selection browser.

---

## 5. Split-Screen (F8) & Session Model

### 5.1 The Active-Inactive Rule
Split mode is a two-panel session entered/exited by `F8`.
*   **Activation:** `F8` toggles split mode on/off.
*   **Focus Switch:** `Tab` switches the active panel.
*   **Active Panel:** Owns keyboard focus and receives command/navigation input.
*   **Inactive Panel:** Does not process direct input while inactive; its own cursor/selection context is retained until focus is switched back.
*   **Freeze/Resume Rule:** When a panel loses focus, its panel-local state is frozen; when it becomes active again (via `Tab`), it must resume exactly where it left off.
*   **Active-Only Mutation Rule:** Commands mutate only the active panel's panel-local state. Cross-panel updates are limited to shared topology mirroring defined in §5.3.

### 5.2 State Persistence
Switching panels via `Tab` must restore the exact state held when that panel last had focus for panel-local state:
*   **Volume Context:** Logged volume or archive.
*   **Cursor & Offset:** Highlighted entry and scroll position.
*   **Selection:** Tags are specific to the panel session. Collapsing a directory (`Left` or `-`) or explicitly unreading/releasing it (`--`) discards saved tags beneath that directory; reloading it must not resurrect stale tags.
*   **Compare Tagging Rule:** Compare workflows may report difference counts, but they do not implicitly mutate per-file tag state; tags change only through explicit tagging actions.
*   **Filter (Filespec):** Independent search/filter strings.
*   **Window/Mode Context:** Directory/File/Showall-style panel-local focus context.

### 5.3 Shared Tree Topology Contract
The split panels share one logged tree topology contract for a given logged volume:
*   **Shared Logging State:** Logged/unlogged directory-memory state is shared.
*   **Shared Structural State:** Expand/collapse/release tree-shape changes are shared.
*   **Mirror Rule:** Structural tree changes triggered in the active panel must be reflected in the inactive panel immediately.
*   **Selection Retention Rule:** Mirrored structural updates must not move the inactive panel's cursor/selection when its selected node remains visible/valid.
*   **Non-Invalidating Changes Rule:** Adding siblings/ancestors, or changing sibling structure that does not invalidate the inactive selected node, must not move the inactive selection.
*   **Frozen File-View Anchor Rule:** A panel left in file view is restored from its saved directory path and selected filename, not from the current flattened tree index. If shared-tree rebuilding leaves that directory as a visible but unloaded placeholder, the directory payload must be reloaded before the panel is rendered or resumed so the file window cannot degrade to an empty or unrelated listing.
*   **Render Is Not Authority Rule:** Rendering may display the saved panel state, but it must not decide a new panel selection from whatever entry currently occupies the saved numeric index after a shared tree rebuild.
*   **Deterministic Fallback Rule (When Selected Node Becomes Invalid):**
    *   Keep exact node if still visible/valid after mirror update.
    *   Else move to nearest visible ancestor of the previously selected node.
    *   Else move to next visible sibling in display order.
    *   Else move to previous visible sibling.
    *   Else move to the root visible node.
*   **No Surprise Parent-Jump Rule:** Collapsing a parent/grandparent in the active panel must not force the inactive selection to jump to parent unless the previously selected node is no longer visible/valid.

### 5.4 Modal Search Behavior
*   **Persistence:** The search string is retained after the mode is exited.
*   **Sticky Cursor:** If a character is typed that produces no match, the cursor remains at the last successful match.
*   **Implicit Exit:** Pressing a key associated with a file operation (Copy, Delete, Move) confirms the current search match and immediately executes that command.

---

## 6. Notification & Messaging Tiers
`ytree` distinguishes between three primary locations for communication:

### 6.1 Footer Messages (Command Area)
*   **Transient:** Non-critical status (e.g., "File copied"). Appears in the Message row. Disappears on the next keystroke.
*   **Sticky/Warning:** Requires acknowledgment or input (e.g., "Delete file? Y/N" or "Path not found"). Stays in the footer until the user responds or hits a key to clear the warning.
*   **Outcome Clarity Rule:** Successful commands may remain quiet, but ytree MUST NOT appear successful while doing nothing. No-op/skip/error outcomes must be explicit and user-visible.
*   **Modifier-Held Shortcut Footer:** While `Ctrl` is physically held, the footer MUST switch to the `Ctrl` shortcut help set and remain visible for the full hold duration. On `Ctrl` release, the footer MUST immediately return to the normal context help. This is transient key-state behavior, not a toggle.

### 6.2 Modal Messages (Centered Box)
A bordered pop-up box that overlays the center of the screen, used for:
*   **Info:** Detailed system information or multi-line status.
*   **Warning:** Significant operational warnings that require explicit dismissal.
*   **Error:** Critical failures (e.g., "Permission Denied" or "Archive Corrupt").
*   **Constraint:** Modals must be dismissed with `Esc` or `Enter` before any other navigation can occur.

### 6.3 Audible Feedback Policy
`ytree` interaction is completely silent. Navigation boundaries, unsupported keys, and input validation must remain silent. If an event is expected during ordinary workflow, it must not trigger an audible cue.

### 6.4 Context Help Contract (Footer <-> F1)
*   **Parity Rule:** For any active context, commands shown in footer help MUST appear in that context's F1 help set. Missing footer commands in F1 are defects.
*   **Concision Rule:** F1 content is concise and contextual. Detailed semantics and examples belong in `etc/ytree.1.md` and generated `docs/USAGE.md`.
*   **Coverage Rule (Required):** Contract coverage includes filesystem and archive contexts (directory/file), `F7`, `F8`, `Showall`, `Global`, and tagged workflows.
*   **Variant Rule:** Help rendering must stay correct for `VI_KEYS=1` variants and Ctrl-held footer variants.
*   **i18n Readiness Rule:** Footer/F1 text must be structured for gettext extraction and reuse to avoid duplicated, drifting message strings across contexts.
*   **Progress Coexistence Rule:** Long-operation progress rendering must coexist with footer/prompt/F1 guidance and must not seize ownership of those help surfaces.

---

## 7. The Virtual Filesystem (VFS)
*   **Archive Integration:** Archives are treated as directories. Entering an archive logs it as a Virtual Volume. `Left Arrow` at the root of an archive "Backs Out" to the parent physical volume.
*   **Stream Rewrite:** Modifications to archives use an atomic rewrite strategy to ensure data integrity.
*   **Live View:** Use `inotify` (where available) for automatic refreshes. If kernel limits are hit, the system falls back to manual refresh logic safely.

---

## 8. Filtering & Command Execution
*   **Filter Stack:** Cumulative logic applies: `Filespec AND Attribute Mask AND Date/Size AND Regex`.
*   **Grep Tagged (`^s`):** A non-destructive content filter applied to the currently tagged set.
*   **Targeting:** In Split-Screen, Copy/Move operations in the Active Panel use the Inactive Panel's current path as the default destination.
*   **Copy Contract:** `Copy` uses source-type semantics: file/tagged-file sources copy non-recursively; directory/tagged-directory sources copy recursively.
*   **Preserve Ancestor Paths Option:** `Copy` may preserve ancestor-relative path from source into destination when enabled. Base root for preserved segments is the operation base root (logged/selected source root), never filesystem `/`.
*   **Source Scope Rule:** Unlogged directories are excluded from copy source scope by default unless explicitly selected/logged.
*   **User Menu (`f9`):** Supports macro expansion: `%f` (file), `%d` (dir), `%t` (tagged list), `%p` (inactive panel path).

---

## 9. Safety & Integrity
*   **Signal Handling:** `SIGINT` and `SIGTERM` are trapped for graceful terminal restoration and VFS cleanup.
*   **Memory Management:** Recursive scans for the Tree View respect the `TREEDEPTH` safety limit to prevent stack overflows or OOM (Out of Memory) conditions on massive filesystems.
*   **Encapsulation:** Global state pointers are strictly forbidden. All logic must utilize the `ViewContext` structure passed explicitly through the call stack.
*   **Destructive-Action Confirmation Rule:** Before destructive mutations (delete, overwrite, replace), ytree MUST show explicit confirmation with clear source/target context and a default-safe choice. Safe/non-destructive operations should remain confirmation-free.

---

## 10. Module Organization & Architecture

### 10.1 Directory Ownership
Every module (`.c`/`.h` pair) must reside in the directory corresponding to its architectural layer:
- **`src/core/`**: Application lifecycle, global state management (`ViewContext`, `Volume`), and session-level logic.
- **`src/fs/`**: Filesystem and archive I/O, VFS drivers, and low-level disk operations.
- **`src/cmd/`**: User command implementations (business logic). These modules coordinate between the FS model and the UI.
- **`src/ui/`**: Presentation layer, input loops (`ctrl_*.c`), rendering (`render_*.c`), and interaction widgets.
- **`src/util/`**: Stateless, non-business helpers (strings, memory_utils, path_utils, completion_utils).

### 10.2 Module Sizing & Cohesion
- **Target Size:** 100-800 Lines of Code (LOC).
- **Bloat Threshold:** Modules exceeding 1,000 LOC are candidates for decomposition.
- **Fragmentation Threshold:** Modules under 50 LOC should be merged into cohesive units.
- **Single Responsibility:** Each module must have one clear purpose.

### 10.3 Naming Conventions
- **`ctrl_` Prefix:** Reserved for modules containing the primary input/event loops for a view (Controller).
- **`render_` Prefix:** Reserved for modules dedicated to visual output via ncurses (View).
- **Generic Plural:** Use for stateless utility collections (e.g., `path_utils.c`).

### 10.4 Header Hygiene
- **Layered Access:** Communication between layers must occur through designated layer headers (`ytree_fs.h`, `ytree_ui.h`).
- **Decoupling:** Minimize cross-layer `#include` directives.
- **Encapsulation:** Internal module state and helper functions should remain `static`. Only the necessary API should be exposed in the header.
