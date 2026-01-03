# **Modernization Plan for ytree**

## **Overview**
This document outlines the strategic roadmap for modernizing `ytree`, a curses-based file manager. The goal is to refactor legacy C code to modern standards (C99/POSIX), remove obsolete dependencies, and implement advanced power-user features inspired by XTreeGold and ZTreeWin, while maintaining the lightweight, keyboard-centric philosophy.

---

## **Guiding Principles**

*   **Code Quality (DRY):** All development should adhere to the "Don't Repeat Yourself" principle. Code should be modular, reusable, and free of redundancy.
*   **Linux-First Design:** Prioritize a user experience tailored for Linux power users, emphasizing shell integration, standard POSIX conventions, and scriptability.
*   **Be Better Than Midnight Commander:** Feature parity with `mc` is a baseline goal. The ultimate objective is to provide a superior, more powerful, and more intuitive user experience.
*   **No Hidden Features:** All functionality, especially syntax like the `{}` placeholder, should be clearly explained in context within the UI (e.g., in the help lines or prompts).
*   **Use Established Libraries:** Prefer mature, well-supported libraries (e.g., `libarchive`) instead of creating custom replacements.

---

## **Phase 1: Code Quality and Refactoring**
*This phase focuses on improving the existing codebase's health and maintainability.*

### **Step 1.1: Remediate DRY Violations**
*   **Goal:** Identify and refactor duplicated code blocks and logic throughout the application.
*   **Rationale:** Establishes a cleaner, more maintainable foundation before adding significant new features. Reduces the risk of introducing bugs and simplifies future development.
*   - [x] **Status:** Completed.

### **Step 1.2: Review and Professionalize Text**
*   **Goal:** Review all user-facing strings and internal code comments, removing or rewriting any that are unprofessional, unclear, or obsolete.
*   **Rationale:** Ensures that the user experience and developer documentation are clear, professional, and helpful.
*   - [x] **Status:** Completed.

---

## **Phase 2: Foundational C-Standard Modernization**
*This phase fixed critical bugs, established C99/POSIX standards compliance for core utilities, and removed reliance on custom, error-prone functions.*

### **Step 2.1: Standardize String & Number Utilities**
*   **Goal:** Replace non-standard utility functions (`Strdup`, `AtoLL`, `StrCaseCmp`, etc.) with their C99/POSIX equivalents (`strdup`, `strtoll`, `strcasecmp`, etc.). This addressed the `~` lockup bug and cleaned up `util.c`.
*   - [x] **Status:** Completed.

### **Step 2.2: Finalize POSIX Regex Adoption**
*   **Goal:** Remove obsolete regex-related macros (e.g., `HAS_REGCOMP`) to rely purely on the modern `<regex.h>` standard.
*   - [x] **Status:** Completed.

### **Step 2.3: Standardize External Command CWD**
*   **Goal:** Ensure that all external commands (`eXecute`, `Pipe`, `View`, `Edit`, user-defined commands) consistently execute with the current working directory set to the location of the active panel.
*   **Rationale:** Some functions (`eXecute`) correctly `chdir` before running a command, while others do not. This inconsistency is confusing and error-prone. Standardizing this behavior makes the application predictable and powerful.
*   - [x] **Status:** Completed.

---

## **Phase 3: Core Architectural Shift (Integrating libarchive)**
*This phase rewrote the archive handling logic to replace the fragile process-spawning/text-parsing method with a stable C library, completely eliminating the dependency on dozens of external utilities.*

### **Step 3.1: Prepare Build Environment for libarchive**
*   **Goal:** Update `ytree.h` and `Makefile` to define necessary headers and link flags for `libarchive` (`-larchive`).
*   - [x] **Status:** Completed.

### **Step 3.2: Rewrite Archive Reading with libarchive**
*   **Goal:** Rewrite `ReadTreeFromArchive` in `archive_reader.c` to use `libarchive`'s streaming API (`archive_read_open_filename`, etc.) instead of spawning external listing processes and parsing text output.
*   - [x] **Status:** Completed.

### **Step 3.3: Rewrite Archive Extraction with libarchive**
*   **Goal:** Implement `ExtractArchiveEntry` in `archive.c` using `libarchive`'s data reading API, and refactor all file content operations to call this function instead of spawning external expander commands.
*   - [x] **Status:** Completed.

---

## **Phase 4: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

### **Step 4.1: Refine UI for Context-Awareness**
*   **Goal:** Make menus context-aware by hiding commands that are unavailable in certain modes (e.g., hiding `eXecute` in archive mode).
*   **Rationale:** Improves usability and adheres to the principle of least surprise by presenting a clean interface that only shows valid options.
*   - [x] **Status:** Completed.

### **Step 4.2: Reduce Non-Essential Audible Alerts**
*   **Goal:** Remove `beep()` calls for non-error conditions, such as hitting navigation boundaries or pressing a key for an unavailable action. Implement the `AUDIBLEERROR` configuration setting.
*   **Rationale:** Aligns with modern UX design, where audible alerts are reserved for genuine errors, not for routine UI feedback.
*   - [x] **Status:** Completed.

### **Step 4.3: Clean Configuration & Macros**
*   **Goal:** Remove all obsolete macro definitions and logic related to external archive utilities (e.g., `ZOOLIST`, `TARLIST`, `ZIP_LINE_LENGTH`, `GetFileMethod`, etc.).
*   **Rationale:** This declutters the code and configuration, removing dead code paths and options that are no longer functional after the `libarchive` integration.
*   - [x] **Status:** Completed.

### **Step 4.4: Modernize Disk Space Calculation**
*   **Goal:** Refactor `freesp.c` to exclusively use the POSIX standard `statvfs` function.
*   **Rationale:** This eliminates all legacy `#ifdef` blocks and platform-specific implementations, improving portability and simplifying maintenance.
*   - [x] **Status:** Completed.

### **Step 4.5: Implement Configurable UI Color Scheme**
*   **Goal:** Allow users to customize application UI colors (panels, borders, highlights) via a `[COLORS]` section in `~/.ytree`.
*   **Rationale:** Makes `ytree` more flexible and user-friendly, aligning with modern command-line utility best practices.
*   - [x] **Status:** Completed.

### **Step 4.6: Implement File Type Colorization**
*   **Goal:** Add a configurable mechanism to color filenames based on their type or extension (e.g., archives, executables, source files).
*   **Rationale:** Provides at-a-glance information about file types, similar to the `ls --color=auto` command, significantly improving visual scanning speed.
*   - [x] **Status:** Completed.

### **Step 4.7: Implement Starfield Animation**
*   **Goal:** Add an optional "Warp Speed" starfield animation during long-running directory scans (`Log` command). Make it configurable via the `ANIMATION` setting in `~/.ytree`.
*   **Rationale:** Provides visual feedback that the application is working during intense I/O operations, replaces the static ASCII logo with something more dynamic, and adds a touch of retro-computing flair.
*   - [x] **Status:** Completed.

### **Step 4.8: Implement Advanced Filtering System**
*   **Goal:** Overhaul the `Filespec` feature (renamed to **Filter**) to create a powerful, multi-layered filtering system inspired by ZTree, allowing users to combine multiple criteria to refine the file view.
*   **Rationale:** This transforms filtering from a simple wildcard match into a core utility for file system analysis and management.
*   - [x] **Status:** Completed.

#### **Step 4.8.1: Implement Dot-File and Dot-Directory Visibility Toggle**
*   **Description:** Implement a non-destructive toggle (`` ` ``) to show/hide hidden files. The cursor should behave intelligently, attempting to stay on the selected file or moving to the nearest visible parent if the selected item is hidden.
*   - [x] **Status:** Completed.

#### **Step 4.8.2: Base Filespec Enhancement**
*   **Description:** Enhance the basic filespec string to support comma-separated lists for multiple inclusion patterns (e.g., `*.c,*.h`) and an exclusion prefix (`-`) to hide patterns (e.g., `-*.o`).
*   - [x] **Status:** Completed.

#### **Step 4.8.3: Implement Attribute-Based Filtering**
*   **Description:** Implement filtering based on POSIX `st_mode` attributes using syntax like `:r`, `:w`, `:x`, `:l` (links), `:d` (directories). Rename "Filespec" to "Filter" in the UI.
*   - [x] **Status:** Completed.

#### **Step 4.8.4: Implement Date Filtering**
*   **Description:** Implement filtering by modification time using syntax like `>YYYY-MM-DD`, `<YYYY-MM-DD`, or `=YYYY-MM-DD`. Also modernized the date display in the file window to `YYYY-MM-DD HH:MM`.
*   - [x] **Status:** Completed.

#### **Step 4.8.5: Implement Size Filtering**
*   **Description:** Implement filtering by file size using syntax like `>10k`, `<5M`, `=0`. Supports `k`, `M`, `G` suffixes.
*   - [x] **Status:** Completed.

#### **Step 4.8.6: Implement Filter Stack Logic**
*   **Description:** Refactor the core file matching logic to ensure that all active filters (Regex Patterns + Attributes + Date + Size) are cumulative (AND logic).
*   - [x] **Status:** Completed. (Implemented via unified `Match()` function in `filters.c`).

#### **Step 4.8.7: Implement Directory Filtering**
*   **Description:** Extend the Filter logic to support directory patterns (identified by a trailing slash, e.g., `-obj/` or `build/`). Update the Directory Tree generation logic (`ReadDirList`) to apply these filters, completely hiding matching directories and their contents from the view.
*   - [ ] **Status:** Not Started.

### **Step 4.9: Implement Multi-Volume Architecture & Global State Management**
*   **Goal:** Transition Ytree from a single-instance root application to a **Session-Volume** file manager capable of holding multiple directory trees (drives/paths) in memory simultaneously.
*   **Rationale:** To bridge the gap between Ytree and XTree/ZTree by enabling essential features: logging multiple disks, instant context switching between them, and performing operations on tags across different file systems. This requires replacing global static variables with dynamic data structures managed by efficient hash tables.
*   - [x] **Status:** Completed.

####   **Step 4.9.1: Integrate `uthash` and Define Infrastructure with Compatibility Layer**
*   **Task:** Add the `uthash.h` header to the project and update `ytree.h` to define the new `Volume` structure and Hash Table structures. Replace legacy global variables (`statistic`, `disk_statistic`) with preprocessor macros pointing to `CurrentVolume`. Update `global.c` to define the new volume pointers.
*   **Outcome:** The application now compiles and runs using the new `Volume` structure implicitly. `CurrentVolume` is manually allocated in `init.c`.
*   - [x] **Status:** Completed.

#### **Step 4.9.2: Implement Volume Lifecycle Management**
*   **Task:** Formalize volume management by creating a dedicated `volume.c` module. Implement `Volume_Create()` to encapsulate allocation, ID generation, and hash table insertion. Clean up `init.c` to use this helper function and update the `Makefile` to include the new module.
*   - [x] **Status:** Completed.

#### **Step 4.9.3: Implement Volume Destruction Logic**
*   **Task:** Implement `Volume_Delete()` in `volume.c`. This function safely removes a volume from the global hash table, recursively frees its directory tree (using `DeleteTree`), and frees the volume structure memory. Update `ytree.h` to expose this function.
*   - [x] **Status:** Completed.

#### **Step 4.9.4: Implement Volume Switching Logic**
*   **Task:** Modify `log.c` (specifically the `LoginDisk` function).
*   **Logic:** Instead of always destroying the current tree and rescanning:
    1.  Check if a volume with the requested path already exists in the `VolumeList` hash table.
    2.  **If it exists:** Switch `CurrentVolume` to that pointer and refresh the screen.
    3.  **If it does not exist:** Create a new Volume, scan the filesystem into it, and *then* make it the `CurrentVolume`.
*   - [x] **Status:** Completed.

#### **Step 4.9.5: Implement Volume Selection UI**
*   **Goal:** Now that we can load multiple volumes (drives/paths) into memory, we need a way to see them and switch between them without typing the full path every time.
*   **Action:** Implement a `SelectLoadedVolume()` function in `log.c` that displays a visual list of currently loaded volumes and allows the user to pick one using a temporary array for index mapping.
*   **Rationale:** This visualizes the Multi-Volume feature and makes it usable.
*   - [x] **Status:** Completed.

#### **Step 4.9.6: Implement Volume Cycling & Bind Keys**
*   **Goal:** Provide quick navigation between loaded volumes.
*   **Action:**
    1.  Implement `CycleLoadedVolume(int direction)` in `log.c`.
    2.  Bind `,` / `<` to **Previous Volume**.
    3.  Bind `.` / `>` to **Next Volume**.
    4.  Bind `K` / `Shift+K` to **Volume Menu** (List).
*   - [x] **Status:** Completed.

#### **Step 4.9.7: Fix Volume Logic & Preserve State**
*   **Goal:** Polish the user experience and fix logic errors.
*   **Action:**
    1.  **Fix Ghost Volume:** Modify `LoginDisk` to **reuse** the initial empty volume (Volume 0) instead of creating a new one, preventing dead menu entries.
    2.  **Fix Cycle Wrapping:** Update `CycleLoadedVolume` math to handle negative modulo correctly.
    3.  **Preserve State:** Update `dirwin.c` to **stop resetting** `cursor_pos` and `disp_begin_pos` when switching volumes, effectively saving the user's place on each drive.
*   - [x] **Status:** Completed.

#### **Step 4.9.8: Stability, Safety & Validation**
*   **Goal:** Ensure the application does not crash when dealing with edge cases or external filesystem changes.
*   **Action:**
    1.  **Validation:** `LoginDisk` now verifies `chdir()` success. If a directory is deleted externally, the volume is removed from memory.
    2.  **Self-Deletion Safety:** If the *current* volume is deleted, `LoginDisk` restores a safe blank volume to prevent dangling pointers.
    3.  **Auto-Skip:** `CycleLoadedVolume` loops to automatically skip over dead/deleted volumes.
    4.  **Init Fix:** `Init()` now properly populates the path of the first volume to prevent `[ ] <no path>` menu glitches.
    5.  **Underflow Fix:** `readtree.c` modified to prevent `disk_total_directories` from wrapping to huge numbers during tree collapse.
*   - [x] **Status:** Completed.

#### **Step 4.9.9: Safe Application Shutdown**
*   **Goal:** Prevent memory corruption and crashes when quitting the application.
*   **Action:** Implemented `Volume_FreeAll()` in `volume.c` and integrated it into `Quit()`. This ensures all volumes and the global hash table are explicitly and safely deallocated before the process terminates.
*   - [x] **Status:** Completed.

#### **Step 4.9.10: Enhanced Header UI**
*   **Goal:** Provide visual feedback about the multi-volume state.
*   **Action:** Updated `DisplayDiskName` in `stats.c` to display the volume index (e.g., `[Vol: 1/3]`) and the current directory path in the top header. Implemented safe string truncation to prevent buffer overflows with long paths.
*   - [x] **Status:** Completed.

#### **Step 4.9.11: Implement Graceful Scan Abort**
*   **Goal:** Fix the crash that occurs when the user interrupts the scanning of a large drive (e.g., 5TB volume).
*   **Rationale:** Currently, pressing `ESC` during a scan often triggers a hard exit or leaves memory in an inconsistent state. We need to allow `ReadTree` to unwind gracefully so the application can revert to the previous volume safely.
*   **Action:** Modify `readtree.c` to handle `ESC` by returning an error code rather than calling `exit()`.
*   - [x] **Status:** Completed.

#### **Step 4.9.12: Polish Scan Abort Handling**
*   **Goal:** Improve UX by silencing the "Internal Error" message on intentional abort and clearing the prompt when resuming.
*   **Action:** Updated `log.c` to treat return code `-1` as a silent abort. Updated `readtree.c` to clear the confirmation line.
*   - [x] **Status:** Completed.

#### **Step 4.9.13: Implement Volume Release ("Unlog")**
*   **Goal:** Allow the user to manually close (release) a volume they are no longer using to free up memory and declutter the list.
*   **Rationale:** The "Session-Volume" architecture is incomplete without a way to remove volumes. ZTree/XTree allow "releasing" a logged disk.
*   **Mechanism:** Add functionality to the **Volume Selection Menu** (`K` key).
    *   Pressing **`D`** (Delete) or **`Delete Key`** on a menu item will prompt to close that volume.
    *   If the closed volume was the *current* one, automatically switch to another.
*   - [x] **Status:** Completed.

#### **Step 4.9.14: Fix Volume Menu Scrolling**
*   **Goal:** Enable scrolling in the volume selection menu (`K`) to support lists longer than the screen height.
*   **Action:** Implemented a view-port offset logic in `log.c` to render only visible items and handle scrolling navigation.
*   - [x] **Status:** Completed.

#### **Step 4.9.15: Fix Archive Volume Switching**
*   **Goal:** Prevent `LoginDisk` from erroneously deleting valid archive volumes when switching back to them.
*   **Action:** Modified `log.c` to detect `ARCHIVE_MODE` and perform a file existence check (`stat`) instead of a directory check (`chdir`), as archives are files.
*   - [x] **Status:** Completed.

#### **Step 4.9.16: Fix Memory Leak on Exit (DeleteTree)**
*   **Goal:** Fix a massive memory leak detected by ASan on exit.
*   **Action:** Updated `DeleteTree` in `tree_utils.c` to implement full recursive freeing of the directory tree structure, ensuring `Volume_FreeAll` releases all memory.
*   - [x] **Status:** Completed.

#### **Step 4.9.17: Fix UTF-8 Character Display**
*   **Goal:** Ensure UTF-8 filenames (like "fünf") are displayed correctly instead of being mangled into escape sequences.
*   **Action:** Updated `Makefile` to link against `ncursesw` (Wide) and defined `WITH_UTF8`. Removed manual character filtering that conflicted with multi-byte strings.
*   - [x] **Status:** Completed.

#### **Step 4.9.18: Fix Tagging of Hidden Dotfiles**
*   **Goal:** Ensure that "Tag Directory" (`T`) and "Tag All Directories" (`^T`) do not affect dotfiles if `hide_dot_files` is enabled.
*   **Action:** Modified `HandleTagDir` and `HandleTagAllDirs` in `dirwin.c` to respect the `hide_dot_files` flag.
*   - [x] **Status:** Completed.

#### **Step 4.9.19: Fix Directory Statistics Update**
*   **Goal:** Ensure the "DIR Statistics" panel dynamically updates to show the stats of the *currently selected directory* during navigation, tagging, and filtering.
*   **Action:** Injected calls to `DisplayDirStatistic` into all navigation and state-change functions in `dirwin.c`.
*   - [x] **Status:** Completed.

#### **Step 4.9.20: Synchronize Directory Statistics in File Window**
*   **Goal:** Ensure the "DIR Statistics" panel strictly matches the file list displayed in the File Window.
*   **Action:** Modified `filewin.c` to update `dir_entry->matching_files` (and recalculate tagged counts) inside `BuildFileEntryList`, ensuring sync between the view and the stats.
*   - [x] **Status:** Completed.

### **Step 4.10: Modernize UI & Statistics Panel**
*   **Goal:** Redesign the user interface to provide a dedicated, high-density information sidebar that supports the new Multi-Volume architecture without cluttering or overwriting the main directory view.
*   **Rationale:** The legacy interface lacked space for context (Volume Index, Filesystem Type) and suffered from layout bugs where header text overwrote the directory tree. A structured sidebar (inspired by ZTree/UnixTree) improves usability and data visibility.
*   - [x] **Status:** Completed.

#### **Step 4.10.1: Fix Statistics Calculation Logic**
*   **Goal:** Ensure "Matching Files" and "Tagged Files" are counted correctly immediately upon logging a disk.
*   **Rationale:** Previously, statistics were calculated *before* filters were applied in `LoginDisk`, resulting in zero counts until a manual refresh. Logic order needed correction to support the new panel's data requirements.
*   **Action:** Modified `log.c` to strictly order the sequence: `SetFilter` -> `ApplyFilter` -> `RecalculateSysStats` -> `Display`.
*   - [x] **Status:** Completed.

#### **Step 4.10.2: Define UI Geometry Constants**
*   **Goal:** Centralize window layout definitions to create a guaranteed "safe space" for the statistics panel.
*   **Rationale:** Hardcoded widths (e.g., `COLS - 26`) made layout changes brittle. Defining `STATS_WIDTH` ensures the Directory and File windows automatically resize to fit the side panel without overlap.
*   **Action:** Updated `ytree.h` to define `STATS_WIDTH` and `MAIN_WIN_WIDTH`, linking all window dimensions to these constants.
*   - [x] **Status:** Completed.

#### **Step 4.10.3: UI Implementation - Modernized Stats Panel**
*   **Goal:** Render the new 24-column statistics interface on the right sidebar.
*   **Rationale:** To provide better context for the Multi-Volume features (showing Volume Index `[1/3]`, Path, and Filesystem Type) and to visually separate "Global Volume Stats" from "Current Directory Stats".
*   **Action:** Rewrote `stats.c` to implement the new vertical layout using absolute positioning relative to the right screen edge, incorporating safe string truncation for long paths.
*   - [x] **Status:** Completed.

#### **Step 4.10.4: Refine Stats Panel (Boxed Layout & Human Readable)**
*   **Goal:** Improve the stats panel by using a "Boxed" layout with headers embedded in separator lines (``) and using Human Readable size formatting (e.g., `12.5M`) to fit more data on single lines.
*   **Action:** Updated `stats.c` to implement the `DrawBoxFrame` and `FormatShortSize` logic. Fixed display bugs where Directory Attributes were hidden.
*   - [x] **Status:** Completed.

#### **Step 4.10.5: Fix Root Masking and Header Artifacts**
*   **Goal:** Resolve visual regressions: prevent the stats panel from erasing the first directory entry, and ensure the top-left "Path:" header updates correctly without ghosting characters from previous paths.
*   **Action:**
    1.  **`stats.c`**: Removed the `mvwhline` call in `DisplayDiskName` that was incorrectly clearing the content window (`dir_window`) instead of the header area.
    2.  **`display.c`**: Updated `DisplayMenu` to explicitly clear the header background (`mvwhline`) and print the truncated current path (`statistic.path` or `login_path`) using the new geometry constants.
*   - [x] **Status:** Completed.

#### **Step 4.10.6: Implement Jump to Owner Directory (\)**
*   **Goal:** In "Show All" mode, pressing `\` (Backslash) on a selected file will exit Show All mode and jump the Directory Tree cursor to that file's parent directory.
*   **Rationale:** Standard ZTree feature ("\ To Dir") essential for locating files found via a global search/filter within their hierarchy context.
*   - [ ] **Status:** Not Started.

### **Step 4.11: Implement Wide Mode (Toggle Stats Panel)**
*   **Goal:** Implement a command (`F6`) to toggle the visibility of the statistics sidebar, effectively switching between a "Wide" view (0 width stats) and the standard view (24 width stats). Add a configuration option (`STATS_PANEL=1/0`) to `~/.ytree` to set the default state.
*   **Rationale:** Maximizes horizontal screen real estate for deep directory hierarchies or long filenames. This dynamic layout capability is a prerequisite for correctly implementing the F7 (File Preview) and F8 (Split Screen) modes.
*   - [x] **Status:** Completed.

### **Step 4.12: Technical Debt Remediation ("De-Brittling")**
*   **Goal:** Systematically identify and refactor fragile code patterns that hinder maintenance and scalability.
*   **Rationale:** As features are added, legacy patterns (magic numbers, unsafe buffers, hardcoded keys) create regression risks. This phase hardens the codebase before introducing complex event-driven features like Inotify or Mouse support.
*   - [x] **Status:** Completed.

#### **Step 4.12.1: UI Layout Remediation**
*   **Goal:** Eliminate "Magic Numbers" and hardcoded offsets from the UI rendering logic to prevent off-by-one errors and layout glitches during resizing or volume switching.
*   **Mechanism:** Define a centralized `Layout` structure containing dynamic dimensions (`dir_win_h`, `file_win_h`, `stats_width`). Calculate these once in a `Layout_Recalculate()` function (called on Init and Resize) and replace compile-time macros with these runtime values.
*   - [x] **Status:** Completed.

#### **Step 4.12.2: Harden String Handling**
*   **Task:** Audit `display_utils.c` and `util.c` for unsafe string manipulations (`strcpy`, `strcat`, `sprintf` into stack buffers). Replace with `snprintf`, truncated copies, or dynamic allocation where appropriate. Specifically target functions that assume 1 byte == 1 character (fixing residual UTF-8 truncation bugs).
*   **Benefit:** Prevents buffer overflows and ensures correct display of Unicode filenames.
*   - [x] **Status:** Completed.

#### **Step 4.12.3: Decouple Input Handling**
*   **Task:** Refactor `input.c` to abstract key handling. Move hardcoded `case 'k':` logic out of `dirwin.c` and into a dispatch table or higher-level `Action` enum.
*   **Benefit:** Prerequisites for Configurable Keymaps (Phase 7.1) and makes the event loop ready for non-blocking I/O (Inotify).
*   - [x] **Status:** Completed.

#### **Step 4.12.4: Finalize Path Normalization and Archive Logic**
*   **Goal:** Simplify the path normalization function in `util.c` by leveraging the stability provided by POSIX `getcwd`/`realpath` and the removal of custom tilde expansion. Also harden the archive parsing logic to prevent string corruption.
*   **Mechanism:** Replaced destructive string tokenization with `realpath` and stack-based normalization. Standardized on `Fnsplit` for archive paths.
*   - [x] **Status:** Completed.

#### **Step 4.12.5: Review and Finalize Terminal Input**
*   **Goal:** Re-verify the input loop in `input.c` and clean up any remaining low-level terminal interaction to ensure stability across modern environments (Linux/WSL/macOS).
*   **Mechanism:** Replaced potentially unsafe `strcpy`/`strcat` patterns in `InputStringEx` with `memmove` based editing to prevent buffer overflows.
*   - [x] **Status:** Completed.

#### **Step 4.12.6: Security Hardening Review**
*   **Goal:** Conduct a focused review of all code paths involving external process creation, shell command execution, and temporary file handling to ensure adherence to security best practices.
*   **Mechanism:** Standardized execution environment in `execute.c`, `pipe.c`, `view.c`, `edit.c`, and `usermode.c` to use `open/fchdir` for robust CWD restoration, preventing race conditions.
*   - [x] **Status:** Completed.

### **Step 4.13: Implement File System Watcher (Inotify)**
*   **Goal:** Automatically update the file list when files are created, modified, or deleted in the currently viewed directory by external programs.
*   **Rationale:** Currently, `ytree` shows a stale snapshot of the filesystem until the user manually presses `^L` (Refresh). Modern expectations require a "live" view. Using `inotify` is efficient and event-driven, avoiding the overhead of constant polling.
*   - [x] **Status:** Completed.

#### **Step 4.13.1: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.

#### **Step 4.13.2: Refactor Input Loop for Event Handling**
*   **Task:** Modify `input.c` to support non-blocking input handling.
*   **Logic:**
    *   Currently, `Getch()` blocks indefinitely waiting for a key.
    *   **New Logic:** Use `select()` or `poll()` to wait on **two** file descriptors:
        1.  `STDIN_FILENO` (The keyboard).
        2.  `Watcher_GetFD()` (The inotify handle).
    *   If the Watcher FD triggers, call `Watcher_CheckEvents()`. If it confirms a change, set a global flag `refresh_needed = TRUE`.
    *   If STDIN triggers, proceed to `wgetch()`.

#### **Step 4.13.3: Implement Live Refresh Logic**
*   **Task:** Connect the `refresh_needed` flag to the main window logic.
*   **Logic:**
    *   In `dirwin.c` (`HandleDirWindow`) and `filewin.c` (`HandleFileWindow`), inside the input loop:
    *   Check `if (refresh_needed)`.
    *   **Action:**
        1.  Call `RescanDir(current_dir)`.
        2.  Call `BuildFileEntryList`.
        3.  Call `DisplayFileWindow`.
        4.  Reset `refresh_needed = FALSE`.
    *   *Note:* We must ensure the cursor stays on the same file if possible (by saving the filename before rescan and finding it after).

#### **Step 4.13.4: Update Watch Context on Navigation**
*   **Task:** Ensure the watcher always monitors the *current* directory.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:* Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytree`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.

### **Step 4.14: Implement Configurable Auto-Refresh Strategies**
*   **Goal:** Add a configuration option (`AUTO_REFRESH`) to control when directories are re-scanned, implemented as a bitmask to allow combinations.
    *   **1:** Enable Watcher (Inotify/Kernel Events). [Default]
    *   **2:** Refresh on Directory Navigation (Cursor Move).
    *   **4:** Refresh on entering File Window.
*   **Rationale:** Provides flexibility. Users on fast local disks might want `7` (All), while users on slow networks (where navigation refresh is laggy) might want `5` (Watcher + File Window) or just `4` if Inotify is unsupported.
*   - [ ] **Status:** Not Started.

### **Step 4.15: Implement Proactive Directory Creation**
*   **Goal:** When performing operations (Copy, Move, PathCopy) on files, if the destination directory does not exist, prompt the user to create it.
*   - [x] **Status:** Completed.

### **Step 4.16: Implement Directory Graft (Copy/Move) and Release**
*   **Goal:** Implement directory manipulation commands in the Directory Window.
    *   **Release:** Unload a directory branch from memory (without deleting files) to clean up the view.
    *   **Graft (Move):** Move an entire directory branch to a new parent.
    *   **PathCopy (Copy):** Recursively copy a directory branch to a new location.
*   **Rationale:** Essential directory management features present in XTree but missing in ytree.
*   - [ ] **Status:** Not Started.

### **Step 4.17: Implement F2 Filesystem Navigation, also from Archives**
*   **Goal:** When copying or moving files, allow the F2 key to open a navigation pane for the filesystem, also from inside an archive.
*   **Rationale:** Fixes a major usability issue, allowing users to easily select a destination directory on the filesystem when copying or moving files.
*   - [ ] **Status:** Not Started.

### **Step 4.18: Add Activity Spinner**
*   **Goal:** Display an animated spinner (e.g., `|`, `/`, `-`, `\`) in the menu bar area during long-running operations like directory scanning.
*   **Rationale:** Provides essential visual feedback to the user that the application is busy and has not frozen.
*   - [x] **Status:** Completed.

### **Step 4.19: Enhance History with Contextual Categories and Favorites**
*   **Goal:** Separate command history into distinct categories (e.g., Filespec, Login, Execute) so that history recall is context-sensitive. When the user recalls history, only entries relevant to the current prompt (e.g., only paths for the Login prompt) will be shown. Additionally, implement a mechanism to mark history items as "favorites" to ensure they persist across sessions and are easily accessible.
*   **Rationale:** Makes the history feature significantly more organized and intuitive by filtering out irrelevant entries based on the current context. The implementation must also be robust against data loss, ensuring history from previous sessions is reliably loaded.
*   - [ ] **Status:** Not Started.

### **Step 4.20: Implement Transparent Archive Navigation**
*   **Goal:** Implement "Transparent Exit" for archives by making `Left Arrow` at the root of a volume close it and return to the previous volume. This creates the illusion that the Archive was just a subdirectory, allowing users to "back out" of an archive seamlessly.
*   **Rationale:** This is a major UX improvement that aligns `ytree` with the intuitive navigation model of Midnight Commander.
*   **Mechanism:** In `HandleDirWindow` (`src/dirwin.c`), modify `ACTION_MOVE_LEFT`. If the user is at the Root of the volume, check if it's an Archive (or secondary volume). If so, treat the Left Arrow as a "Close Volume" command (similar to `ACTION_VOL_PREV` but destructive for the archive instance) to return to the parent context.
*   - [x] **Status:** Completed.

### **Step 4.21: Implement F7 File Preview Panel**
*   **Goal:** Implement a file preview mode activated by F7. This mode will collapse the statistics panel and use the expanded main window area to display the contents of the selected file without launching an external pager.
*   **Rationale:** Provides a fast, integrated way to inspect file contents, a classic and highly valued feature of Norton/XTree-style file managers.
*   - [ ] **Status:** Not Started.

### **Step 4.22: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good, but `pathcop(Y)/^Y` is redundant and should just be `pathcop(Y)`).
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   - [ ] **Status:** Not Started.

### **Step 4.23: Enhance Clock Display**
*   **Goal:** Modify the optional clock display to show both the date and time.
*   **Rationale:** A simple UI improvement providing more context to the user.
*   - [x] **Status:** Completed.

### **Step 4.24: Implement Integrated Help System** MAYBE NOT/INSTEAD MAKE YTREE MORE INTUITIVE?
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   - [ ] **Status:** Not Started.

### **Step 4.25: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** A key feature of classic file managers like ZTreeWin and modern ones like Midnight Commander, mouse support dramatically improves speed and ease of use for users in capable terminal environments.
*   - [ ] **Status:** Not Started.

### **Step 4.26: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option (e.g., in a future F3 settings panel) to globally disable external viewers, forcing the use of the internal viewer.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   - [ ] **Status:** Not Started.

### **Step 4.27: Implement Lightweight Directory Refresh**
*   **Goal:** Implement a lightweight directory refresh mechanism to ensure the UI reflects filesystem changes made by `ytree`'s own operations (like Copy, Move, Extract) without a full rescan.
*   **Rationale:** When a file is created or modified in a directory, the view of that directory should update automatically. This is distinct from a manual full `^L` rescan and makes the program feel more responsive.
*   - [x] **Status:** Completed.

### **Step 4.28: Implement Advanced Log Options**
*   **Goal:** Enhance the `Log` command to present options for controlling the scan depth and scope, similar to ZTree/XTree (e.g., "Log drive", "Log tree", "Log directory").
*   **Rationale:** Provides essential control over performance when working with very large filesystems, allowing the user to perform a shallow scan when a deep recursive scan is not needed.
*   - [ ] **Status:** Not Started.

### **Step 4.29: Implement In-App Configuration Editor (F3)**
*   **Goal:** Implement a settings panel (activated by a key like `F3`) that allows the user to view and change configuration options from `~/.ytree` (e.g., `CONFIRMQUIT`, colors) and save the changes.
*   **Rationale:** Provides a user-friendly way to configure `ytree` without manually editing the configuration file, improving accessibility.
*   - [ ] **Status:** Not Started.

### **Step 4.30: Implement "Tags-Only" View Mode**
*   **Goal:** Implement a toggle (`*` or `8`) to display only the tagged files in the current File Window.
*   **Rationale:** A core ZTreeWin feature that allows users to verify, refine, and operate on a specific subset of files without the visual clutter of non-tagged files.
*   - [ ] **Status:** Not Started.

### **Step 4.31: Archive Write Support:**
*   Implement archive creation and modification using `libarchive`'s write APIs. The Auto-Refresh logic for archives will need to be different from file systems: instead of watching the files inside, it should watch the archive file itself (the container). If the container's timestamp changes, then ytree should reload the virtual view.
*   - [ ] **Status:** Not Started.
*   **Includes:** Implementing `^S` (Execute Command) inside archives. Requires extracting every file to a temporary location to run the command (e.g., `grep`) on it.

### **Step 4.32: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   - [ ] **Status:** Not Started.

### **Step 4.33: Applications menu**
*   Implement a customizable Application Menu.
*   - [ ] **Status:** Not Started.

### **Step 4.34: Standardize Input Prompt Padding**
*   **Goal:** Standardize the visual spacing between input labels (e.g., "GROUP:", "OWNER:", "ATTRIBUTES:") and the cursor entry point across the application.
*   **Rationale:** Currently, input fields rely on hardcoded starting columns (e.g., 12), resulting in inconsistent visual padding depending on the label length. Dynamic calculation based on label length ensures a polished, professional UI consistency.
*   **Mechanism:**
    *   Define a `UI_INPUT_PADDING` constant (set to 2) in `ytree.h`.
    *   Refactor `GetNewGroup` (`chgrp.c`), `GetNewOwner` (`chown.c`), and `Change...Modus` (`chmod.c`) to calculate the input start position dynamically: `1 + strlen(label) + UI_INPUT_PADDING`.
*   - [x] **Status:** Completed.

### **Step 4.35: Standardize Incremental Search (List Jump)**
*   **Goal:** Implement robust, non-recursive incremental search activated by the `/` key in both Directory and File windows.
*   **Rationale:** Currently, search is bound to `F12` and uses a dangerous recursive implementation. Mapping it to `/` aligns `ytree` with standard Unix tools (vi, less) and allows users to navigate quickly without triggering command hotkeys (like 'd' for delete).
*   **Mechanism:** Refactor `ListJump` to use an iterative loop. Bind `/` in `input.c`. Support backspace handling and "search-as-you-type" highlighting.
*   - [ ] **Status:** Not Started.

### **Step 4.36: Implement Bottom F-Key Menu Bar**
*   **Goal:** Shift the existing two-line command footer up by one line and reserve the bottom-most row for a clickable, function-key reference bar (F1 Help, F3 Options, F5 Redraw, F7 View, F8 Split).
*   **Rationale:** Aligns with the standard "Norton Commander" layout familiar to power users. It provides immediate visual cues for function keys, which are often less intuitive than mnemonic letter commands.
*   - [ ] **Status:** Not Started.

### **Step 4.37: Implement "Touch" (Make File) Command**
*   **Goal:** Add a command (e.g., `^M` or mapped to a specific key) to create a new, empty file in the current directory, similar to `M` (Make Directory).
*   **Rationale:** Currently, creating a file requires shelling out (`X`) and typing `touch filename`. A native command streamlines the workflow for developers creating placeholders or config files.
*   - [ ] **Status:** Not Started.

### **Step 4.38: Enhance Archive Navigation (Tree Logic)**
*   **Goal:** Enable standard Tree Window navigation keys (specifically `Left Arrow` to collapse/parent and `Right Arrow` to expand) while browsing inside an archive.
*   **Rationale:** Navigation inside archives currently feels "flat" or inconsistent compared to the physical filesystem. Unifying these behaviors reduces cognitive load.
*   - [ ] **Status:** Not Started.

### **Step 4.39: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   - [ ] **Status:** Not Started.

### **Step 4.40: Implement Responsive Adaptive Footer**
*   **Goal:** Make the two-line command footer dynamic based on terminal width.
    *   **Compact (< 80 cols):** Show only critical navigation and file operation keys (Copy, Move, Delete, Quit).
    *   **Standard (80-120 cols):** Show the standard set (current behavior).
    *   **Wide (> 120 cols):** Unfold "hidden" or advanced commands (Hex, Group, Sort modes) directly into the footer so the user doesn't have to remember them.
*   **Rationale:** Maximizes utility on large displays while preventing line-wrapping visual corruption on small screens. It reduces the need to consult F1 on large monitors.
*   **Mechanism:** Define command groups (Priority 1, 2, 3). In `DisplayDirHelp` / `DisplayFileHelp`, construct the string dynamically based on `COLS`.
*   - [ ] **Status:** Not Started.

### **Step 4.41: Make "Terminal Classic" the default UI Color and Classic ytree color scheme the example**
*   **Goal:** Update the default color scheme to adhere to the standard "Terminal Classic" aesthetic (typically white/grey text on black background), while providing the traditional "Blue/Yellow" ytree scheme as an easily selectable example or option.
*   **Rationale:** Modern terminal users expect applications to respect their terminal's color palette by default. The classic high-contrast blue scheme can be jarring.
*   - [ ] **Status:** Not Started.

### **Step 4.42: Consolidate Attribute Commands**
*   **Goal:** Group the `A` (Attribute/Mode), `O` (Owner), and `G` (Group) commands under a single `A` menu.
*   **Rationale:** De-clutter the top-level keymap. Changing ownership/groups is an infrequent administrative task compared to navigation. This frees up the `G` and `O` keys for more common actions (e.g., Global/Go or Sort/Order).
*   - [ ] **Status:** Not Started.

### **Step 4.43: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   - [ ] **Status:** Not Started.

### **Step 4.44: Implement Fixed-Width Column Mode (Filename Truncation)**
*   **Goal:** Modify the File Window logic to enforce a maximum column width (e.g., 32 characters) even if longer filenames exist. Filenames exceeding this width will be visually truncated (e.g., `00- Introductio~.pdf`) to ensure multiple columns are displayed.
*   **Integration:** Add this as a new mode in the `^F` (File Mode) rotation, or add a configuration toggle (`COMPACT_COLUMNS=1`).
*   **Rationale:** Currently, a single long filename forces the File Window into a inefficient single-column layout. This feature maximizes information density.
*   - [x] **Status:** Completed.

---

## **Phase 5: Major Architectural Refactoring**
*This phase implements core architectural changes required for advanced features.*

### **Step 5.1: Encapsulate Global UI & Configuration State**
*   **Goal:** Refactor `global.c` to move remaining global variables (e.g., `dir_window`, `file_window`, `sort_order`, `mode`) into a unified `ViewContext` or `Panel` structure. Update function signatures throughout the application to accept a `Context*` pointer instead of relying on global state.
*   **Rationale:** This is the absolute prerequisite for **Step 5.2 (F8 Split Screen)**. Currently, the application architecture assumes a single active view. To display and interact with two independent directory trees side-by-side (e.g., copying from Left to Right), the state must be instance-specific, not global.
*   - [x] **Status:** Completed. Global pointers encapsulated in ViewContext. Compatibility maintained via macros.

#### **Step 5.1.1: Encapsulate Directory State**
*   **Goal:** Fix "Stale Cache" crashes by moving `dir_entry_list`, `dir_entry_list_capacity`, and `total_dirs` from static globals in `src/dirwin.c` into the `Volume` structure.
*   **Mechanism:** Update `BuildDirEntryList` to accept a `Volume*` context. Update `HandleDirWindow` to use `CurrentVolume->dir_entry_list`.
*   - [x] **Status:** Completed.

#### **Step 5.1.2: Encapsulate File List State**
*   **Goal:** Ensure the file list always matches the active volume. Move `file_entry_list` and `file_count` from `src/filewin.c` into the `Volume` structure.
*   **Mechanism:** Refactor `BuildFileEntryList` to populate `CurrentVolume->file_entry_list`.
*   - [x] **Status:** Completed.

#### **Step 5.1.3: Eliminate the `statistic` Macro (The "God Object")**
*   **Goal:** Remove the `#define` in `include/ytree.h`. Refactor function signatures in `src/stats.c`, `src/readtree.c`, and `src/log.c` to accept `Statistic *stats` or `Volume *vol` as an argument. Pass `&CurrentVolume->vol_stats` explicitly from the call sites in `main.c` and `HandleDirWindow`.
*   **Mechanism:** Update all files accessing `statistic` to use explicit pointers.
*   - [x] **Status:** Completed.

#### **Step 5.1.4: Eliminate UI Magic Numbers**
*   **Goal:** Replace hardcoded `LINES`/`COLS` math with a dynamic layout engine to fix resize bugs and support split-screen.
*   **Mechanism:** Implement `YtreeLayout` struct and `Layout_Recalculate`. Remove static height/width variables from window controllers.
*   - [x] **Status:** Completed.

#### **Step 5.1.5: Fix Volume State Logic**
*   **Goal:** Prevent cursor reset and UI corruption when switching volumes.
*   **Mechanism:** Refactor `HandleDirWindow` navigation logic to clamp cursor position instead of resetting it, and force a full UI refresh on volume switch.
*   - [x] **Status:** Completed.

### **Step 5.2: Implement F8 Split-Screen Mode**
*   **Goal:** Refactor the application's state management to support two independent file panels. This includes collapsing the stats panel and allowing the user to `Tab` between the two panes for copy/move operations. This will also enable advanced logging features similar to ZTree/XTree (e.g., `Alt-L` to log a tree in the other pane).
*   **Rationale:** This is an essential feature for any advanced file manager and a prerequisite for many efficient file management workflows. This requires a significant refactoring of global state into pane-specific contexts.
*   - [ ] **Status:** Not Started.

#### **Step 5.3: Enhanced Archive Format Detection (UDF/ISO Bridge Fix)**
*   **Description:** Address the issue where `libarchive` defaults to the empty ISO9660 partition on "UDF Bridge" media (e.g., Windows installation ISOs), resulting in a "No Files!" directory view.
*   **Mechanism:** Modify `src/readarchive.c` to implement a **Heuristic Retry Strategy**:
    1.  Perform the standard scan using `archive_read_support_format_all`.
    2.  Count the number of valid files found.
    3.  If the file count is effectively empty (e.g., 0 files, or only `README.TXT`) **AND** the detected format was `ISO9660`:
    4.  **Close and Re-open** the archive, but this time explicitly **disable** ISO9660 support (or enable *only* UDF/other formats via `archive_read_set_format` options if available in the installed library version).
    5.  If the secondary scan finds more files, use that result; otherwise, revert to the initial view.
*   **Rationale:** Ensures users can access data on modern installation media without manual mounting or external tools.
*   - [ ] **Status:** Not Started.

#### **Step 5.4: Implement Non-Destructive Tree Storage (Virtual Collapse)**.
*   **Description:** This addresses the "Archive Collapse" issue. Currently, `ytree` "collapses" a folder by deleting its children from memory. Since archives cannot be easily re-scanned (unlike directories), we cannot "collapse" them without losing data. The fix is to change the data structure so "Collapsing" just hides nodes visually without freeing memory. This is also a prerequisite for efficient Archive Modification (Write support).

---

## **Phase 6: Advanced Features**
*This phase builds upon the new architecture to add more high-level functionality.*

### **Step 6.1: Implement Advanced Directory Comparison**
*   **Goal:** Implement a comprehensive directory/tree comparison feature that leverages the split-screen view. This will include recursive comparison of two directories, highlighting of file differences (newer, older, unique), and the ability to tag files based on these comparison results.
*   **Rationale:** This is a core power-user feature for synchronizing directories and managing source code, moving `ytree` beyond simple navigation to a true file management utility.
*   - [ ] **Status:** Not Started.

### **Step 6.2: Implement Directory Bookmarks (Hotlist)**
*   **Goal:** Create a "hotlist" feature allowing users to bookmark frequently used directories and quickly jump to them.
*   **Rationale:** Drastically improves navigation efficiency for common workflows, a staple feature of powerful file managers.
*   - [ ] **Status:** Not Started.

### **Step 6.3: Implement File Comparison Integration (JFC)**
*   **Goal:** Integrate external diff tools (e.g., `diff`, `vimdiff`, `meld`) to compare the current file with a tagged file or the file in the adjacent split pane (`J`).
*   **Rationale:** Provides critical functionality for developers and system administrators to identify changes between file versions directly from the file manager.
*   - [ ] **Status:** Not Started.

### **Step 6.4: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   - [ ] **Status:** Not Started.

### **Step 6.5: Implement "Grep Tagged" (`^S`)**
*   **Goal:** Implement a content search feature (`^S`) that iterates through all tagged files (in File System or Archive). Files that *do not* match the search string are untagged.
*   **Integration:** Combine this with `^V` (View Tagged) to allow the user to quickly narrow down a dataset and view only the hits.
*   **Rationale:** This mimics the powerful "Search" function of XTreeGold, turning `ytree` into a tool for data mining and bulk text analysis.
*   - [ ] **Status:** Not Started.

### **Step 6.6: Advanced Destination Selection (Enhanced F2)**
*   **Goal:** Overhaul the F2 (Select Directory) dialog during Copy/Move operations. It should support:
    *   Creating new paths on the fly.
    *   Logging new disks/archives from within the selection window.
    *   Toggling between "Current", "Tree", and "History" views.
*   **Rationale:** currently, selecting a destination is rigid. This flexibility allows users to organize files into new structures without leaving the Copy/Move context.
*   - [ ] **Status:** Not Started.

### **Step 6.7: Implement Plugin / VFS Architecture**
*   **Goal:** Enable browsing of remote systems (FTP/SFTP), structured data (XML, JSON, MediaWiki dumps), or specialized databases (GEDCOM) as if they were local directory trees.
*   **Rationale:** Extends `ytree`'s utility beyond the local filesystem, turning it into a universal browser.
*   **Mechanism:** Implement a generic Plugin API (`ytree_plugin.h`) that allows external Shared Objects (`.so`) to register themselves.
    *   **Interface:** Plugins provide function pointers for `open`, `read_dir`, `stat`, and `execute_custom`.
    *   **Loading:** `dlopen()` scans a plugin directory on startup.
    *   **Integration:** When a user enters a supported file, `ytree` hands control to the plugin's VFS logic.
*   - [ ] **Status:** Not Started.

---

## **Phase 7: Internationalization and Configurability**
*   **Goal:** Refactor the application to support localization and user-defined keybindings, moving away from hardcoded English-centric values.

### **Step 7.1: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from `~/.ytree`. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   - [ ] **Status:** Not Started.

### **Step 7.2: Standardize Localization Strategy**
*   **Goal:** Audit the codebase for hardcoded German strings or personal author remarks and convert them to English. Ensure all UI text is wrapped in macros suitable for future `gettext` translation.
*   **Rationale:** Prepares the project for a global audience and ensures consistency in the default locale.
*   - [ ] **Status:** Not Started.

### **Step 7.3: Externalize UI Strings for Internationalization (i18n)**
*   **Description:** Move all user-facing strings (menus, prompts, error messages) out of the C code and into a separate, loadable resource file. This is a prerequisite for translation. The application will load the appropriate language file at startup.
*   - [ ] **Status:** Not Started.

---

## **Phase 8: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Step 8.1: Convert Man Page to Markdown**
*   **Description:** The traditional `groff` man page (`ytree.1`) was converted to Markdown (`ytree.1.md`) and is now generated at build time using `pandoc`. This makes the documentation significantly easier to read, edit, and maintain.
*   - [x] **Status:** Completed.

### **Step 8.2: Integrate Test Suite into Build Process**
*   **Goal:** Add a `test` target to the `Makefile` to simplify running the automated test suite.
*   **Rationale:** Formalizes the testing process and makes it trivial for any contributor to validate their changes by simply running `make test`.
*   - [ ] **Status:** Not Started.

### **Step 8.3: Implement CI with GitHub Actions**
*   **Goal:** Set up a basic Continuous Integration workflow to automatically build the project and run the test suite on every push and pull request.
*   **Rationale:** Enforces code quality and prevents regressions by ensuring that all changes are automatically validated.
*   - [ ] **Status:** Not Started.

### **Step 8.4: Enhance Build System**
*   **Goal:** Improve Makefile targets for installation and add a proper `uninstall` target to clean up all installed files. Modernize versioning by moving version info from `patchlev.h` into the `Makefile`.
*   **Rationale:** Provides a more robust and complete build system for end-users and packagers.
*   - [ ] **Status:** Partially Completed (uninstall exists but can be improved).

### **Step 8.5: Implement Code Formatting, Static Analysis, and Hygiene**
*   **Goal:** Add `make format` and `make analyze` targets to the Makefile. `format` will use `clang-format` to enforce a consistent style. `analyze` will run a static analyzer (like `cppcheck` or Clang's analyzer) to find potential bugs. Include a "lint" target to identify unused functions and variables.
*   **Rationale:** Automates code quality enforcement. Performing a manual "Cruft Audit" to remove debug comments, unused macros, and outdated documentation ensures the codebase remains clean and professional for release.
*   - [ ] **Status:** Not Started.

### **Step 8.6: Finalize Documentation**
*   **Goal:** Update the `CHANGELOG`, `README.md`, and `CONTRIBUTING.md` files to reflect all new features and changes before a release.
*   **Rationale:** Ensures users and developers have accurate, up-to-date information about the project.
*   - [ ] **Status:** Not Started.

### **Step 8.7: Initialize Distributed Issue Tracking (git-bug)**
*   **Goal:** Configure `git-bug` to act as a bridge between the local repository and GitHub Issues. Migrate the contents of `BUGS.md` and `TODO.txt` into this system prior to public release.
*   **Rationale:** Allows the developer to maintain a simple local text-based workflow during heavy development, while ensuring that all tracking data can be synchronized to the public web interface when the project goes live.
*   - [ ] **Status:** Not Started.

---

## **Phase 9: Valgrind Audit & Optimization**
*This phase focuses on deep stability, memory safety, and performance profiling to ensure `ytree` is robust enough for long-running sessions.*

### **Step 9.1: Establish Valgrind Baseline**
*   **Goal:** Create a reproducible test case (scripted via the Python test suite) that exercises core features (Log, Copy, Move, Refresh, Exit) while running under Valgrind Memcheck.
*   **Rationale:** Provides a consistent benchmark to measure memory usage and identify existing leaks before optimization begins.
*   - [ ] **Status:** Not Started.

### **Step 9.2: Memory Leak Remediation**
*   **Goal:** Systematically analyze Valgrind reports and fix all "Definitely Lost" and "Indirectly Lost" memory errors.
*   **Rationale:** Prevents memory bloat over time, which is critical for a file manager that may be left open for days.
*   - [ ] **Status:** Not Started.

### **Step 9.3: Fix Uninitialized Memory Access**
*   **Goal:** Identify and fix "Conditional jump or move depends on uninitialized value(s)" errors.
*   **Rationale:** These are often the cause of sporadic, unreproducible crashes and erratic behavior.
*   - [ ] **Status:** Not Started.

### **Step 9.4: Resource Leak Cleanup (File Descriptors)**
*   **Goal:** Ensure all file descriptors (`open`, `opendir`, `popen`) are properly closed, especially in error paths and during volume switching.
*   **Rationale:** Prevents "Too many open files" errors during heavy usage.
*   - [ ] **Status:** Not Started.

---

## **Future Enhancements / Wishlist**
*A collection of high-complexity or lower-priority features to be considered after the primary roadmap is complete.*

*   **Per-Window Filter State (Split Screen Prerequisite):** Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left pane versus `*.h` in the right).
*   **Implement Advanced, ncurses-native Command Line Editing:** Full cursor navigation (left/right, home/end, word-by-word). In-line text editing (insert, delete, backspace, clear line). Persistent command history accessible via up/down arrows. Maybe even context-aware tab completion for files and directories.
*   **Shell-Style Tab Completion:** Replace the current history-based tab completion with true filename/directory completion in the input line.
*   **Print Functionality:** Investigate and potentially implement a "Print" command that is distinct from "Pipe," if a clear use case for modern systems can be defined.
*   **Terminal-Independent TUI Mode:** Investigate a mode to run the `ytree` TUI directly on a Linux virtual console (e.g., via `/dev/tty*`) or framebuffer, independent of a terminal emulator session.
*   **Windows Port:** Adapt the build system and platform-specific APIs to support compilation and execution on modern Windows via MinGW-w64 and PDCurses.
*   **Scroll bars:**  On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list, (configurable to char or line).
*   **State Preservation on Reload (^L):** Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   **Preserve Tree Expansion on Refresh:** Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.
*   **Implement "Safe Delete" (Trash Can):** Support the FreeDesktop.org Trash specification to allow file recovery.