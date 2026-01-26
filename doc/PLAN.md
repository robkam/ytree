# **Modernization Plan for ytree**

## **Overview**
This document outlines the strategic roadmap for modernizing `ytree`, a curses-based file manager. The goal is to refactor legacy C code to modern standards (C99/POSIX), remove obsolete dependencies, and implement advanced power-user features inspired by XTreeGold and ZTreeWin, while maintaining the lightweight, keyboard-centric philosophy.

---

## **Guiding Principles**

*   **Code Quality (DRY):** All development should adhere to the "Don't Repeat Yourself" principle. Code should be modular, reusable, and free of redundancy.
*   **Architectural Integrity (Anti-Patching):** Do not apply superficial fixes for deep architectural problems. If a bug is caused by fragmented state or logic, **STOP**. Refactor the architecture to unify the logic before fixing the specific bug. It is better to break one thing to fix the system than to patch the system and break everything.
*   **Single Responsibility (SRP):** Enforce strict modularity. Each file (module) must serve exactly one purpose. Maintain a hard separation between the **UI** (View), **File System** (Model), and **Commands** (Controller) to ensure the codebase remains testable and maintainable.
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
*   **Files to Modify:** `src/dirwin.c`, `src/filewin.c`, `src/display.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 1.2: Review and Professionalize Text**
*   **Goal:** Review all user-facing strings and internal code comments, removing or rewriting any that are unprofessional, unclear, or obsolete.
*   **Rationale:** Ensures that the user experience and developer documentation are clear, professional, and helpful.
*   **Files to Modify:** `src/*.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

---

## **Phase 2: Foundational C-Standard Modernization**
*This phase fixed critical bugs, established C99/POSIX standards compliance for core utilities, and removed reliance on custom, error-prone functions.*

### **Step 2.1: Standardize String & Number Utilities** (Use the Auditor Persona here)
*   **Goal:** Replace non-standard utility functions (`Strdup`, `AtoLL`, `StrCaseCmp`, etc.) with their C99/POSIX equivalents (`strdup`, `strtoll`, `strcasecmp`, etc.). This addressed the `~` lockup bug and cleaned up `util.c`.
*   **Files to Modify:** `src/string_utils.c`, `include/ytree.h`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 2.2: Finalize POSIX Regex Adoption**
*   **Goal:** Remove obsolete regex-related macros (e.g., `HAS_REGCOMP`) to rely purely on the modern `<regex.h>` standard.
*   **Files to Modify:** `src/filter.c`, `include/ytree.h`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 2.3: Standardize External Command CWD**
*   **Goal:** Ensure that all external commands (`eXecute`, `Pipe`, `View`, `Edit`, user-defined commands) consistently execute with the current working directory set to the location of the active panel.
*   **Rationale:** Some functions (`eXecute`) correctly `chdir` before running a command, while others do not. This inconsistency is confusing and error-prone. Standardizing this behavior makes the application predictable and powerful.
*   **Files to Modify:** `src/execute.c`, `src/pipe.c`, `src/view.c`, `src/edit.c`, `src/usermode.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

---

## **Phase 3: Core Architectural Shift (Integrating libarchive)**
*This phase rewrote the archive handling logic to replace the fragile process-spawning/text-parsing method with a stable C library, completely eliminating the dependency on dozens of external utilities.*

### **Step 3.1: Prepare Build Environment for libarchive**
*   **Goal:** Update `ytree.h` and `Makefile` to define necessary headers and link flags for `libarchive` (`-larchive`).
*   **Files to Modify:** `Makefile`, `include/ytree.h`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 3.2: Rewrite Archive Reading with libarchive**
*   **Goal:** Rewrite `ReadTreeFromArchive` in `archive_reader.c` to use `libarchive`'s streaming API (`archive_read_open_filename`, etc.) instead of spawning external listing processes and parsing text output.
*   **Files to Modify:** `src/readarchive.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 3.3: Rewrite Archive Extraction with libarchive**
*   **Goal:** Implement `ExtractArchiveEntry` in `archive.c` using `libarchive`'s data reading API, and refactor all file content operations to call this function instead of spawning external expander commands.
*   **Files to Modify:** `src/archive.c`
*   **Context Files:** `include/ytree.h`, `src/copy.c`, `src/view.c`
*   - [x] **Status:** Completed.

---

## **Phase 4: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

### **Step 4.1: Refine UI for Context-Awareness**
*   **Goal:** Make menus context-aware by hiding commands that are unavailable in certain modes (e.g., hiding `eXecute` in archive mode).
*   **Rationale:** Improves usability and adheres to the principle of least surprise by presenting a clean interface that only shows valid options.
*   **Files to Modify:** `src/display.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.2: Reduce Non-Essential Audible Alerts**
*   **Goal:** Remove `beep()` calls for non-error conditions, such as hitting navigation boundaries or pressing a key for an unavailable action. Implement the `AUDIBLEERROR` configuration setting.
*   **Rationale:** Aligns with modern UX design, where audible alerts are reserved for genuine errors, not for routine UI feedback.
*   **Files to Modify:** `src/dirwin.c`, `src/filewin.c`, `src/input.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.3: Clean Configuration & Macros**
*   **Goal:** Remove all obsolete macro definitions and logic related to external archive utilities (e.g., `ZOOLIST`, `TARLIST`, `ZIP_LINE_LENGTH`, `GetFileMethod`, etc.).
*   **Rationale:** This declutters the code and configuration, removing dead code paths and options that are no longer functional after the `libarchive` integration.
*   **Files to Modify:** `include/config.h`, `src/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.4: Modernize Disk Space Calculation**
*   **Goal:** Refactor `freesp.c` to exclusively use the POSIX standard `statvfs` function.
*   **Rationale:** This eliminates all legacy `#ifdef` blocks and platform-specific implementations, improving portability and simplifying maintenance.
*   **Files to Modify:** `src/freesp.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.5: Implement Configurable UI Color Scheme**
*   **Goal:** Allow users to customize application UI colors (panels, borders, highlights) via a `[COLORS]` section in `~/.ytree`.
*   **Rationale:** Makes `ytree` more flexible and user-friendly, aligning with modern command-line utility best practices.
*   **Files to Modify:** `src/color.c`, `src/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.6: Implement File Type Colorization**
*   **Goal:** Add a configurable mechanism to color filenames based on their type or extension (e.g., archives, executables, source files).
*   **Rationale:** Provides at-a-glance information about file types, similar to the `ls --color=auto` command, significantly improving visual scanning speed.
*   **Files to Modify:** `src/color.c`, `src/profile.c`, `src/filewin.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.7: Implement Starfield Animation**
*   **Goal:** Add an optional "Warp Speed" starfield animation during long-running directory scans (`Log` command). Make it configurable via the `ANIMATION` setting in `~/.ytree`.
*   **Rationale:** Provides visual feedback that the application is working during intense I/O operations, replaces the static ASCII logo with something more dynamic, and adds a touch of retro-computing flair.
*   **Files to Modify:** `src/animate.c`, `src/readtree.c`, `src/log.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.8: Implement Advanced Filtering System**
*   **Goal:** Overhaul the `Filespec` feature (renamed to **Filter**) to create a powerful, multi-layered filtering system inspired by ZTree, allowing users to combine multiple criteria to refine the file view.
*   **Rationale:** This transforms filtering from a simple wildcard match into a core utility for file system analysis and management.
*   **Files to Modify:** `src/filter.c`
*   **Context Files:** `include/ytree.h`, `src/filewin.c`, `src/dirwin.c`
*   - [x] **Status:** Completed.

#### **Step 4.8.1: Implement Dot-File and Dot-Directory Visibility Toggle**
*   **Description:** Implement a non-destructive toggle (`` ` ``) to show/hide hidden files. The cursor should behave intelligently, attempting to stay on the selected file or moving to the nearest visible parent if the selected item is hidden.
*   **Files to Modify:** `src/dirwin.c`, `src/filewin.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.8.2: Base Filespec Enhancement**
*   **Description:** Enhance the basic filespec string to support comma-separated lists for multiple inclusion patterns (e.g., `*.c,*.h`) and an exclusion prefix (`-`) to hide patterns (e.g., `-*.o`).
*   **Files to Modify:** `src/filter.c`
*   **Context Files:** `src/string_utils.c`
*   - [x] **Status:** Completed.

#### **Step 4.8.3: Implement Attribute-Based Filtering**
*   **Description:** Implement filtering based on POSIX `st_mode` attributes using syntax like `:r`, `:w`, `:x`, `:l` (links), `:d` (directories). Rename "Filespec" to "Filter" in the UI.
*   **Files to Modify:** `src/filter.c`, `src/display.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.8.4: Implement Date Filtering**
*   **Description:** Implement filtering by modification time using syntax like `>YYYY-MM-DD`, `<YYYY-MM-DD`, or `=YYYY-MM-DD`. Also modernized the date display in the file window to `YYYY-MM-DD HH:MM`.
*   **Files to Modify:** `src/filter.c`, `src/display_utils.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.8.5: Implement Size Filtering**
*   **Description:** Implement filtering by file size using syntax like `>10k`, `<5M`, `=0`. Supports `k`, `M`, `G` suffixes.
*   **Files to Modify:** `src/filter.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.8.6: Implement Filter Stack Logic**
*   **Description:** Refactor the core file matching logic to ensure that all active filters (Regex Patterns + Attributes + Date + Size) are cumulative (AND logic).
*   **Files to Modify:** `src/filter.c`
*   **Context Files:** `src/filewin.c`
*   - [x] **Status:** Completed. (Implemented via unified `Match()` function in `filters.c`).

#### **Step 4.8.7: Implement Directory Filtering**
*   **Description:** Extend the Filter logic to support directory patterns (identified by a trailing slash, e.g., `-obj/` or `build/`). Update the Directory Tree generation logic (`ReadDirList`) to apply these filters, completely hiding matching directories and their contents from the view.
*   **Files to Modify:** `src/cmd/filter.c`, `src/ui/ctrl_dir.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [ ] **Status:** Not Started.

### **Step 4.9: Implement Multi-Volume Architecture & Global State Management**
*   **Goal:** Transition Ytree from a single-instance root application to a **Session-Volume** file manager capable of holding multiple directory trees (drives/paths) in memory simultaneously.
*   **Rationale:** To bridge the gap between Ytree and XTree/ZTree by enabling essential features: logging multiple disks, instant context switching between them, and performing operations on tags across different file systems. This requires replacing global static variables with dynamic data structures managed by efficient hash tables.
*   **Files to Modify:** `src/global.c`, `src/volume.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

####   **Step 4.9.1: Integrate `uthash` and Define Infrastructure with Compatibility Layer**
*   **Task:** Add the `uthash.h` header to the project and update `ytree.h` to define the new `Volume` structure and Hash Table structures. Replace legacy global variables (`statistic`, `disk_statistic`) with preprocessor macros pointing to `CurrentVolume`. Update `global.c` to define the new volume pointers.
*   **Outcome:** The application now compiles and runs using the new `Volume` structure implicitly. `CurrentVolume` is manually allocated in `init.c`.
*   **Files to Modify:** `include/ytree.h`, `src/global.c`, `src/init.c`
*   **Context Files:** `include/uthash.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.2: Implement Volume Lifecycle Management**
*   **Task:** Formalize volume management by creating a dedicated `volume.c` module. Implement `Volume_Create()` to encapsulate allocation, ID generation, and hash table insertion. Clean up `init.c` to use this helper function and update the `Makefile` to include the new module.
*   **Files to Modify:** `src/volume.c`, `src/init.c`, `Makefile`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.3: Implement Volume Destruction Logic**
*   **Task:** Implement `Volume_Delete()` in `volume.c`. This function safely removes a volume from the global hash table, recursively frees its directory tree (using `DeleteTree`), and frees the volume structure memory. Update `ytree.h` to expose this function.
*   **Files to Modify:** `src/volume.c`, `src/tree_utils.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.4: Implement Volume Switching Logic**
*   **Task:** Modify `log.c` (specifically the `LoginDisk` function).
*   **Logic:** Instead of always destroying the current tree and rescanning:
    1.  Check if a volume with the requested path already exists in the `VolumeList` hash table.
    2.  **If it exists:** Switch `CurrentVolume` to that pointer and refresh the screen.
    3.  **If it does not exist:** Create a new Volume, scan the filesystem into it, and *then* make it the `CurrentVolume`.
*   **Files to Modify:** `src/log.c`
*   **Context Files:** `src/volume.c`, `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.5: Implement Volume Selection UI**
*   **Goal:** Now that we can load multiple volumes (drives/paths) into memory, we need a way to see them and switch between them without typing the full path every time.
*   **Action:** Implement a `SelectLoadedVolume()` function in `log.c` that displays a visual list of currently loaded volumes and allows the user to pick one using a temporary array for index mapping.
*   **Rationale:** This visualizes the Multi-Volume feature and makes it usable.
*   **Files to Modify:** `src/log.c`, `src/input.c`
*   **Context Files:** `src/volume.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.6: Implement Volume Cycling & Bind Keys**
*   **Goal:** Provide quick navigation between loaded volumes.
*   **Action:**
    1.  Implement `CycleLoadedVolume(int direction)` in `log.c`.
    2.  Bind `,` / `<` to **Previous Volume**.
    3.  Bind `.` / `>` to **Next Volume**.
    4.  Bind `K` / `Shift+K` to **Volume Menu** (List).
*   **Files to Modify:** `src/log.c`, `src/input.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.7: Fix Volume Logic & Preserve State**
*   **Goal:** Polish the user experience and fix logic errors.
*   **Action:**
    1.  **Fix Ghost Volume:** Modify `LoginDisk` to **reuse** the initial empty volume (Volume 0) instead of creating a new one, preventing dead menu entries.
    2.  **Fix Cycle Wrapping:** Update `CycleLoadedVolume` math to handle negative modulo correctly.
    3.  **Preserve State:** Update `dirwin.c` to **stop resetting** `cursor_pos` and `disp_begin_pos` when switching volumes, effectively saving the user's place on each drive.
*   **Files to Modify:** `src/log.c`, `src/dirwin.c`
*   **Context Files:** `src/volume.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.8: Stability, Safety & Validation**
*   **Goal:** Ensure the application does not crash when dealing with edge cases or external filesystem changes.
*   **Action:**
    1.  **Validation:** `LoginDisk` now verifies `chdir()` success. If a directory is deleted externally, the volume is removed from memory.
    2.  **Self-Deletion Safety:** If the *current* volume is deleted, `LoginDisk` restores a safe blank volume to prevent dangling pointers.
    3.  **Auto-Skip:** `CycleLoadedVolume` loops to automatically skip over dead/deleted volumes.
    4.  **Init Fix:** `Init()` now properly populates the path of the first volume to prevent `[ ] <no path>` menu glitches.
    5.  **Underflow Fix:** `readtree.c` modified to prevent `disk_total_directories` from wrapping to huge numbers during tree collapse.
*   **Files to Modify:** `src/log.c`, `src/readtree.c`, `src/init.c`
*   **Context Files:** `src/volume.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.9: Safe Application Shutdown**
*   **Goal:** Prevent memory corruption and crashes when quitting the application.
*   **Action:** Implemented `Volume_FreeAll()` in `volume.c` and integrated it into `Quit()`. This ensures all volumes and the global hash table are explicitly and safely deallocated before the process terminates.
*   **Files to Modify:** `src/quit.c`, `src/volume.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.10: Enhanced Header UI**
*   **Goal:** Provide visual feedback about the multi-volume state.
*   **Action:** Updated `DisplayDiskName` in `stats.c` to display the volume index (e.g., `[Vol: 1/3]`) and the current directory path in the top header. Implemented safe string truncation to prevent buffer overflows with long paths.
*   **Files to Modify:** `src/stats.c`, `src/display.c`
*   **Context Files:** `src/volume.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.11: Implement Graceful Scan Abort**
*   **Goal:** Fix the crash that occurs when the user interrupts the scanning of a large drive (e.g., 5TB volume).
*   **Rationale:** Currently, pressing `ESC` during a scan often triggers a hard exit or leaves memory in an inconsistent state. We need to allow `ReadTree` to unwind gracefully so the application can revert to the previous volume safely.
*   **Action:** Modify `readtree.c` to handle `ESC` by returning an error code rather than calling `exit()`.
*   **Files to Modify:** `src/readtree.c`
*   **Context Files:** `src/log.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.12: Polish Scan Abort Handling**
*   **Goal:** Improve UX by silencing the "Internal Error" message on intentional abort and clearing the prompt when resuming.
*   **Action:** Updated `log.c` to treat return code `-1` as a silent abort. Updated `readtree.c` to clear the confirmation line.
*   **Files to Modify:** `src/log.c`, `src/readtree.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.9.13: Implement Volume Release ("Unlog")**
*   **Goal:** Allow the user to manually close (release) a volume they are no longer using to free up memory and declutter the list.
*   **Rationale:** The "Session-Volume" architecture is incomplete without a way to remove volumes. ZTree/XTree allow "releasing" a logged disk.
*   **Mechanism:** Add functionality to the **Volume Selection Menu** (`K` key).
    *   Pressing **`D`** (Delete) or **`Delete Key`** on a menu item will prompt to close that volume.
    *   If the closed volume was the *current* one, automatically switch to another.
*   **Files to Modify:** `src/log.c`
*   **Context Files:** `src/volume.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.14: Fix Volume Menu Scrolling**
*   **Goal:** Enable scrolling in the volume selection menu (`K`) to support lists longer than the screen height.
*   **Action:** Implemented a view-port offset logic in `log.c` to render only visible items and handle scrolling navigation.
*   **Files to Modify:** `src/log.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.9.15: Fix Archive Volume Switching**
*   **Goal:** Prevent `LoginDisk` from erroneously deleting valid archive volumes when switching back to them.
*   **Action:** Modified `log.c` to detect `ARCHIVE_MODE` and perform a file existence check (`stat`) instead of a directory check (`chdir`), as archives are files.
*   **Files to Modify:** `src/log.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.16: Fix Memory Leak on Exit (DeleteTree)**
*   **Goal:** Fix a massive memory leak detected by ASan on exit.
*   **Action:** Updated `DeleteTree` in `tree_utils.c` to implement full recursive freeing of the directory tree structure, ensuring `Volume_FreeAll` releases all memory.
*   **Files to Modify:** `src/tree_utils.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.9.17: Fix UTF-8 Character Display**
*   **Goal:** Ensure UTF-8 filenames (like "fünf") are displayed correctly instead of being mangled into escape sequences.
*   **Action:** Updated `Makefile` to link against `ncursesw` (Wide) and defined `WITH_UTF8`. Removed manual character filtering that conflicted with multi-byte strings.
*   **Files to Modify:** `Makefile`, `include/ytree.h`, `src/display_utils.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.9.18: Fix Tagging of Hidden Dotfiles**
*   **Goal:** Ensure that "Tag Directory" (`T`) and "Tag All Directories" (`^T`) do not affect dotfiles if `hide_dot_files` is enabled.
*   **Action:** Modified `HandleTagDir` and `HandleTagAllDirs` in `dirwin.c` to respect the `hide_dot_files` flag.
*   **Files to Modify:** `src/dirwin.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.9.19: Fix Directory Statistics Update**
*   **Goal:** Ensure the "DIR Statistics" panel dynamically updates to show the stats of the *currently selected directory* during navigation, tagging, and filtering.
*   **Action:** Injected calls to `DisplayDirStatistic` into all navigation and state-change functions in `dirwin.c`.
*   **Files to Modify:** `src/dirwin.c`
*   **Context Files:** `src/stats.c`
*   - [x] **Status:** Completed.

#### **Step 4.9.20: Synchronize Directory Statistics in File Window**
*   **Goal:** Ensure the "DIR Statistics" panel strictly matches the file list displayed in the File Window.
*   **Action:** Modified `filewin.c` to update `dir_entry->matching_files` (and recalculate tagged counts) inside `BuildFileEntryList`, ensuring sync between the view and the stats.
*   **Files to Modify:** `src/filewin.c`
*   **Context Files:** `src/stats.c`
*   - [x] **Status:** Completed.

### **Step 4.10: Modernize UI & Statistics Panel**
*   **Goal:** Redesign the user interface to provide a dedicated, high-density information sidebar that supports the new Multi-Volume architecture without cluttering or overwriting the main directory view.
*   **Rationale:** The legacy interface lacked space for context (Volume Index, Filesystem Type) and suffered from layout bugs where header text overwrote the directory tree. A structured sidebar (inspired by ZTree/UnixTree) improves usability and data visibility.
*   **Files to Modify:** `src/stats.c`, `src/display.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.10.1: Fix Statistics Calculation Logic**
*   **Goal:** Ensure "Matching Files" and "Tagged Files" are counted correctly immediately upon logging a disk.
*   **Rationale:** Previously, statistics were calculated *before* filters were applied in `LoginDisk`, resulting in zero counts until a manual refresh. Logic order needed correction to support the new panel's data requirements.
*   **Action:** Modified `log.c` to strictly order the sequence: `SetFilter` -> `ApplyFilter` -> `RecalculateSysStats` -> `Display`.
*   **Files to Modify:** `src/log.c`
*   **Context Files:** `src/filter.c`, `src/stats.c`
*   - [x] **Status:** Completed.

#### **Step 4.10.2: Define UI Geometry Constants**
*   **Goal:** Centralize window layout definitions to create a guaranteed "safe space" for the statistics panel.
*   **Rationale:** Hardcoded widths (e.g., `COLS - 26`) made layout changes brittle. Defining `STATS_WIDTH` ensures the Directory and File windows automatically resize to fit the side panel without overlap.
*   **Action:** Updated `ytree.h` to define `STATS_WIDTH` and `MAIN_WIN_WIDTH`, linking all window dimensions to these constants.
*   **Files to Modify:** `include/ytree.h`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.10.3: UI Implementation - Modernized Stats Panel**
*   **Goal:** Render the new 24-column statistics interface on the right sidebar.
*   **Rationale:** To provide better context for the Multi-Volume features (showing Volume Index `[1/3]`, Path, and Filesystem Type) and to visually separate "Global Volume Stats" from "Current Directory Stats".
*   **Action:** Rewrote `stats.c` to implement the new vertical layout using absolute positioning relative to the right screen edge, incorporating safe string truncation for long paths.
*   **Files to Modify:** `src/stats.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.10.4: Refine Stats Panel (Boxed Layout & Human Readable)**
*   **Goal:** Improve the stats panel by using a "Boxed" layout with headers embedded in separator lines and using Human Readable size formatting (e.g., `12.5M`) to fit more data on single lines.
*   **Action:** Updated `stats.c` to implement the `DrawBoxFrame` and `FormatShortSize` logic. Fixed display bugs where Directory Attributes were hidden.
*   **Files to Modify:** `src/stats.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.10.5: Fix Root Masking and Header Artifacts**
*   **Goal:** Resolve visual regressions: prevent the stats panel from erasing the first directory entry, and ensure the top-left "Path:" header updates correctly without ghosting characters from previous paths.
*   **Action:**
    1.  **`stats.c`**: Removed the `mvwhline` call in `DisplayDiskName` that was incorrectly clearing the content window (`dir_window`) instead of the header area.
    2.  **`display.c`**: Updated `DisplayMenu` to explicitly clear the header background (`mvwhline`) and print the truncated current path (`statistic.path` or `login_path`) using the new geometry constants.
*   **Files to Modify:** `src/stats.c`, `src/display.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.10.6: Implement Jump to Owner Directory (\)**
*   **Goal:** In "Show All" mode, pressing `\` (Backslash) on a selected file will exit Show All mode and jump the Directory Tree cursor to that file's parent directory.
*   **Rationale:** Standard ZTree feature ("\ To Dir") essential for locating files found via a global search/filter within their hierarchy context.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/ui/input.c`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   - [ ] **Status:** Not Started.

### **Step 4.11: Implement Wide Mode (Toggle Stats Panel)**
*   **Goal:** Implement a command (`F6`) to toggle the visibility of the statistics sidebar, effectively switching between a "Wide" view (0 width stats) and the standard view (24 width stats). Add a configuration option (`STATS_PANEL=1/0`) to `~/.ytree` to set the default state.
*   **Rationale:** Maximizes horizontal screen real estate for deep directory hierarchies or long filenames. This dynamic layout capability is a prerequisite for correctly implementing the F7 (File Preview) and F8 (Split Screen) modes.
*   **Files to Modify:** `src/ui/input.c`, `src/core/init.c`
*   **Context Files:** `src/core/global.c`
*   - [x] **Status:** Completed.

### **Step 4.12: Technical Debt Remediation ("De-Brittling")** (Use the Auditor Persona here)
*   **Goal:** Systematically identify and refactor fragile code patterns that hinder maintenance and scalability.
*   **Rationale:** As features are added, legacy patterns (magic numbers, unsafe buffers, hardcoded keys) create regression risks. This phase hardens the codebase before introducing complex event-driven features like Inotify or Mouse support.
*   **Files to Modify:** `src/ui/display_utils.c`, `src/util/string_utils.c`, `src/ui/input.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.12.1: UI Layout Remediation**
*   **Goal:** Eliminate "Magic Numbers" and hardcoded offsets from the UI rendering logic to prevent off-by-one errors and layout glitches during resizing or volume switching.
*   **Mechanism:** Define a centralized `Layout` structure containing dynamic dimensions (`dir_win_h`, `file_win_h`, `stats_width`). Calculate these once in a `Layout_Recalculate()` function (called on Init and Resize) and replace compile-time macros with these runtime values.
*   **Files to Modify:** `src/core/init.c`, `src/core/global.c`, `include/ytree.h`
*   **Context Files:** `src/ui/display.c`
*   - [x] **Status:** Completed.

#### **Step 4.12.2: Harden String Handling**
*   **Task:** Audit `display_utils.c` and `util.c` for unsafe string manipulations (`strcpy`, `strcat`, `sprintf` into stack buffers). Replace with `snprintf`, truncated copies, or dynamic allocation where appropriate. Specifically target functions that assume 1 byte == 1 character (fixing residual UTF-8 truncation bugs).
*   **Benefit:** Prevents buffer overflows and ensures correct display of Unicode filenames.
*   **Files to Modify:** `src/ui/display_utils.c`, `src/util/string_utils.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.12.3: Decouple Input Handling**
*   **Task:** Refactor `input.c` to abstract key handling. Move hardcoded `case 'k':` logic out of `dirwin.c` and into a dispatch table or higher-level `Action` enum.
*   **Benefit:** Prerequisites for Configurable Keymaps (Phase 7.1) and makes the event loop ready for non-blocking I/O (Inotify).
*   **Files to Modify:** `src/ui/input.c`, `include/ytree.h`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   - [x] **Status:** Completed.

#### **Step 4.12.4: Finalize Path Normalization and Archive Logic**
*   **Goal:** Simplify the path normalization function in `util.c` by leveraging the stability provided by POSIX `getcwd`/`realpath` and the removal of custom tilde expansion. Also harden the archive parsing logic to prevent string corruption.
*   **Mechanism:** Replaced destructive string tokenization with `realpath` and stack-based normalization. Standardized on `Fnsplit` for archive paths.
*   **Files to Modify:** `src/util/path_utils.c`
*   **Context Files:** `src/fs/archive.c`
*   - [x] **Status:** Completed.

#### **Step 4.12.5: Review and Finalize Terminal Input**
*   **Goal:** Re-verify the input loop in `input.c` and clean up any remaining low-level terminal interaction to ensure stability across modern environments (Linux/WSL/macOS).
*   **Mechanism:** Replaced potentially unsafe `strcpy`/`strcat` patterns in `InputStringEx` with `memmove` based editing to prevent buffer overflows.
*   **Files to Modify:** `src/ui/input.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

#### **Step 4.12.6: Security Hardening Review**
*   **Goal:** Conduct a focused review of all code paths involving external process creation, shell command execution, and temporary file handling to ensure adherence to security best practices.
*   **Mechanism:** Standardized execution environment in `execute.c`, `pipe.c`, `view.c`, `edit.c`, and `usermode.c` to use `open/fchdir` for robust CWD restoration, preventing race conditions.
*   **Files to Modify:** `src/cmd/execute.c`, `src/cmd/pipe.c`, `src/cmd/view.c`, `src/ui/edit.c`, `src/cmd/usermode.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 4.13: Implement File System Watcher (Inotify)**
*   **Goal:** Automatically update the file list when files are created, modified, or deleted in the currently viewed directory by external programs.
*   **Rationale:** Currently, `ytree` shows a stale snapshot of the filesystem until the user manually presses `^L` (Refresh). Modern expectations require a "live" view. Using `inotify` is efficient and event-driven, avoiding the overhead of constant polling.
*   **Files to Modify:** `src/fs/watcher.c`, `src/ui/input.c`
*   **Context Files:** `include/watcher.h`
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
    *   **4:** Refresh when entering File Window.
*   **Rationale:** Provides flexibility. Users on fast local disks might want `7` (All), while users on slow networks (where navigation refresh is laggy) might want `5` (Watcher + File Window) or just `4` if Inotify is unsupported.
*   **Files to Modify:** `include/config.h`, `src/core/init.c`
*   **Context Files:** `src/ui/input.c`, `src/ui/ctrl_dir.c`
*   - [x] **Status:** Completed.

### **Step 4.15: Implement Proactive Directory Creation**
*   **Goal:** When performing operations (Copy, Move, PathCopy) on files, if the destination directory does not exist, prompt the user to create it.
*   **Files to Modify:** `src/cmd/mkdir.c`, `src/cmd/copy.c`, `src/cmd/move.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 4.16: Consolidate Attribute Commands**
*   **Goal:** Group the `A` (Attribute/Mode), `O` (Owner), and `G` (Group) commands under a single `A` menu.
*   **Rationale:** De-clutter the top-level keymap. Changing ownership/groups is an infrequent administrative task compared to navigation. This frees up the `G` and `O` keys for more common actions (e.g., Global/Go or Sort/Order).
*   **Files to Modify:** `src/ui/input.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`
*   **Context Files:** `src/cmd/chmod.c`, `src/cmd/chown.c`, `src/cmd/group.c`
*   - [ ] **Status:** Not Started.

### **Step 4.17: Implement Directory Graft (Copy/Move) and Release**
*   **Goal:** Implement directory manipulation commands in the Directory Window.
    *   **Release:** Unload a directory branch from memory (without deleting files) to clean up the view.
    *   **Graft (Move):** Move an entire directory branch to a new parent.
    *   **PathCopy (Copy):** Recursively copy a directory branch to a new location.
*   **Rationale:** Essential directory management features present in XTree but missing in ytree.
*   **Files to Modify:** `src/ui/ctrl_dir.c`
*   **Context Files:** `src/cmd/copy.c`, `src/cmd/move.c`
*   - [ ] **Status:** Not Started.

### **Step 4.18: Implement F2 Filesystem Navigation**
*   **Goal:** Enhance the F2 Directory Selection window to support volume management.
    *   Enable logging new drives/paths via `(L)og`.
    *   Enable quick switching between loaded volumes via `(<)` / `(>)` cycling.
    *   Display available commands `[ (L)og (< >) Cycle ]` on the bottom border of the F2 window to prevent footer corruption.
*   **Rationale:** Fixes a major usability issue, allowing users to easily select a destination directory on the filesystem (even from inside an archive) when copying or moving files.
*   **Files to Modify:** `src/ui/ctrl_dir.c`
*   **Context Files:** `src/cmd/log.c`, `src/core/volume.c`
*   - [ ] **Status:** Not Started.

### **Step 4.19: Add Activity Spinner**
*   **Goal:** Display an animated spinner (e.g., `|`, `/`, `-`, `\`) in the menu bar area during long-running operations like directory scanning.
*   **Rationale:** Provides essential visual feedback to the user that the application is busy and has not frozen.
*   **Files to Modify:** `src/ui/animate.c`
*   **Context Files:** `src/fs/readtree.c`, `src/cmd/log.c`
*   - [x] **Status:** Completed.

### **Step 4.20: Enhance History with Contextual Categories and Favorites**
*   **Goal:** Separate command history into distinct categories (e.g., Filespec, Login, Execute) so that history recall is context-sensitive. When the user recalls history, only entries relevant to the current prompt (e.g., only paths for the Login prompt) will be shown. Additionally, implement a mechanism to mark history items as "favorites" to ensure they persist across sessions and are easily accessible.
*   **Rationale:** Makes the history feature significantly more organized and intuitive by filtering out irrelevant entries based on the current context. The implementation must also be robust against data loss, ensuring history from previous sessions is reliably loaded.
*   **Files to Modify:** `src/util/history.c`
*   **Context Files:** `src/ui/input.c`
*   - [x] **Status:** Completed.

### **Step 4.21: Implement "Transparent Exit" for archives**
*   **Goal:** Make `Left Arrow` at the root of a volume close it and return to the previous volume. This creates the illusion that the Archive was just a subdirectory, allowing users to "back out" of an archive seamlessly.
*   **Rationale:** This is a major UX improvement that aligns `ytree` with the intuitive navigation model of Midnight Commander.
*   **Mechanism:** In `HandleDirWindow` (`src/ui/ctrl_dir.c`), modify `ACTION_MOVE_LEFT`. If the user is at the Root of the volume, check if it's an Archive (or secondary volume). If so, treat the Left Arrow as a "Close Volume" command (similar to `ACTION_VOL_PREV` but destructive for the archive instance) to return to the parent context.
*   **Files to Modify:** `src/ui/ctrl_dir.c`
*   **Context Files:** `src/cmd/log.c`, `src/core/volume.c`
*   - [x] **Status:** Completed.

### **Step 4.22: Implement F7 File Preview Panel**
*   **Goal:** Implement a file preview mode activated by F7. This mode will collapse the statistics panel and use the expanded main window area to display the contents of the selected file without launching an external pager.
*   **Rationale:** Provides a fast, integrated way to inspect file contents, a classic and highly valued feature of Norton/XTree-style file managers.
*   **Files to Modify:** `src/ui/input.c`, `src/cmd/view.c`, `src/core/init.c`
*   **Context Files:** `src/core/global.c`
*   - [x] **Status:** Completed.

### **Step 4.23: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good, but `pathcop(Y)/^Y` is redundant and should just be `pathcop(Y)`).
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.24: Enhance Clock Display**
*   **Goal:** Modify the optional clock display to show both the date and time.
*   **Rationale:** A simple UI improvement providing more context to the user.
*   **Files to Modify:** `src/core/clock.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 4.25: Implement Integrated Help System**
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** `src/ui/input.c`
*   - [ ] **Status:** Not Started.

### **Step 4.26: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** A key feature of classic file managers like ZTreeWin and modern ones like Midnight Commander, mouse support dramatically improves speed and ease of use for users in capable terminal environments.
*   **Files to Modify:** `src/ui/input.c`
*   **Context Files:** `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

### **Step 4.27: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option (e.g., in a future F3 settings panel) to globally disable external viewers, forcing the use of the internal viewer.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Files to Modify:** `src/cmd/view.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 4.28: Implement Lightweight Directory Refresh**
*   **Goal:** Implement a lightweight directory refresh mechanism to ensure the UI reflects filesystem changes made by `ytree`'s own operations (like Copy, Move, Extract) without a full rescan.
*   **Rationale:** When a file is created or modified in a directory, the view of that directory should update automatically. This is distinct from a manual full `^L` rescan and makes the program feel more responsive.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/ui/ctrl_dir.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [x] **Status:** Completed.

### **Step 4.29: Implement Advanced Log Options**
*   **Goal:** Enhance the `Log` command to present options for controlling the scan depth and scope, similar to ZTree/XTree (e.g., "Log drive", "Log tree", "Log directory").
*   **Rationale:** Provides essential control over performance when working with very large filesystems, allowing the user to perform a shallow scan when a deep recursive scan is not needed.
*   **Files to Modify:** `src/cmd/log.c`, `src/ui/input.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [ ] **Status:** Not Started.

### **Step 4.30: Implement In-App Configuration Editor (F3)**
*   **Goal:** Implement a settings panel (activated by a key like `F3`) that allows the user to view and change configuration options from `~/.ytree` (e.g., `CONFIRMQUIT`, colors) and save the changes.
*   **Rationale:** Provides a user-friendly way to configure `ytree` without manually editing the configuration file, improving accessibility.
*   **Files to Modify:** `src/cmd/profile.c`, `src/ui/input.c`
*   **Context Files:** `include/config.h`
*   - [ ] **Status:** Not Started.

### **Step 4.31: Implement View Tagged Files (`^V`)**
*   **Goal:** Allow sequential viewing of all tagged files.
*   **Features:**
    *   Iterate through tagged list.
    *   Internal Viewer integration: Add commands to jump to Next (`n`/`Space`) / Previous (`p`) tagged file directly from the viewer.
    *   Untag (`u`): Allow untagging the current file while viewing.
    *   Search integration: If `^S` was used, highlight the search term in the viewer.
*   **Files:** `src/cmd/view.c`, `src/ui/ctrl_file.c`, `src/ui/input.c`.
*   - [ ] **Status:** Not Started.

### **Step 4.32: Archive Write Support (Atomic Breakdown)**
*   **Goal:** Enable modification of archives (ZIP, TAR, ISO, etc.) directly within ytree. Since `libarchive` does not support random-access modification, this requires a "Stream Rewrite" engine: open original, open temp destination, stream entries from old to new (skipping deleted ones, injecting new ones), close, and swap. The Auto-Refresh logic for archives will need to watch the archive file itself (the container) to trigger reloads.

#### **Step 4.32.1: Implement Archive Rewrite Infrastructure**
*   **Description:** Implement the core `Archive_Rewrite(char *archive_path, RewriteCallback cb, void *user_data)` function. This generic engine will handle the `Read Old -> Write New` loop, temporary file creation, atomicity, and error recovery.
*   **Files to Modify:** `src/fs/archive.c`, `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 4.32.2: Implement Archive Deletion**
*   **Description:** Hook the `D` (Delete) command when in Archive Mode to use the Rewrite Engine. The callback will simply skip the entries marked for deletion during the copy stream.
*   **Files to Modify:** `src/cmd/delete.c`, `src/fs/archive.c`
*   - [x] **Status:** Completed.

#### **Step 4.32.3: Implement Archive Addition (Copy-In) & Mkdir**
*   **Description:** Hook `C` (Copy) and `M` (Makedir) to inject new headers and data blocks into the Rewrite stream.
*   **Files to Modify:** `src/cmd/copy.c`, `src/cmd/mkdir.c`, `src/fs/archive.c`
*   - [x] **Status:** Completed.

#### **Step 4.32.4: Implement Archive Rename**
*   **Description:** Hook `R` (Rename) to modify the `pathname` field of headers on the fly during the Rewrite stream.
*   **Files to Modify:** `src/cmd/rename.c`, `src/fs/archive.c`
*   - [x] **Status:** Completed.

#### **Step 4.32.5: Implement Archive Execution & Search (`^S`/`X`)**
*   **Description:** Implement `Execute` and `Grep` for archives by extracting files to a temporary directory (`/tmp/ytree_...`), running the command, and cleaning up. Unlike rewrite operations, this is a read-only extraction task.
*   **Files to Modify:** `src/cmd/execute.c`, `src/ui/ctrl_file.c`
*   - [x] **Status:** Completed.

#### **Step 4.32.6: Implement Archive Move (`M`) Support**
*   **Description:** Implement `M` (Move) for archives. Intra-archive moves use the Rewrite Engine to rename paths. Cross-volume moves use Copy-Extract + Delete.
*   **Files to Modify:** `src/cmd/move.c`, `src/fs/archive.c`.
*   - [ ] **Status:** Not Started.

### **Step 4.33: Implement View Tagged Files (`^V`)**
*   **Goal:** Allow sequential viewing of all tagged files with search term highlighting.
*   **Description:** Uses the system pager (`less`) to view multiple files. Integrates with `^S` (Search) to pass the search pattern (e.g., `less -p "term"`) for automatic highlighting.

#### **Step 4.33.1: Disk Mode View Tagged**
*   **Implementation:** Constructs a batch command line `less file1 file2 ...`. Enables standard pager navigation (`:n` Next, `:p` Previous).
*   **Files to Modify:** `src/cmd/view.c`, `src/ui/ctrl_file.c`, `src/cmd/execute.c`.
*   - [x] **Status:** Completed.

#### **Step 4.33.2: Archive Mode View Tagged**
*   **Implementation:** Extracts all tagged files from the archive to a temporary directory (`/tmp/ytree_view_XYZ/`). Passes these temporary paths to `less`. Cleans up the temporary directory upon exit. Handles large file counts by batching or warning if command line limit exceeded.
*   **Files to Modify:** `src/cmd/view.c`, `src/fs/archive.c`.
*   - [x] **Status:** Completed.

### **Step 4.34: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   **Files to Modify:** `src/fs/readarchive.c`, `src/cmd/log.c`
*   **Context Files:** `src/fs/archive.c`
*   - [ ] **Status:** Not Started.

### **Step 4.35: Applications menu**
*   Implement a customizable Application Menu.
*   **Files to Modify:** `src/cmd/usermode.c`, `src/cmd/profile.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.36: Standardize Input Prompt Padding**
*   **Goal:** Standardize the visual spacing between input labels (e.g., "GROUP:", "OWNER:", "ATTRIBUTES:") and the cursor entry point across the application.
*   **Rationale:** Currently, input fields rely on hardcoded starting columns (e.g., 12), resulting in inconsistent visual padding depending on the label length. Dynamic calculation based on label length ensures a polished, professional UI consistency.
*   **Mechanism:**
    *   Define a `UI_INPUT_PADDING` constant (set to 2) in `ytree.h`.
    *   Refactor `GetNewGroup` (`chgrp.c`), `GetNewOwner` (`chown.c`), and `Change...Modus` (`chmod.c`) to calculate the input start position dynamically: `1 + strlen(label) + UI_INPUT_PADDING`.
*   **Files to Modify:** `src/cmd/chgrp.c`, `src/cmd/chown.c`, `src/cmd/chmod.c`, `include/ytree.h`
*   **Context Files:** `src/ui/input.c`
*   - [x] **Status:** Completed.

### **Step 4.37: Standardize Incremental Search (List Jump)**
*   **Goal:** Implement robust, non-recursive incremental search activated by the `/` key in both Directory and File windows.
*   **Rationale:** Currently, search is bound to `F12` and uses a dangerous recursive implementation. Mapping it to `/` aligns `ytree` with standard Unix tools (vi, less) and allows users to navigate quickly without triggering command hotkeys (like 'd' for delete).
*   **Mechanism:** Refactor `ListJump` to use an iterative loop. Bind `/` in `input.c`. Support backspace handling and "search-as-you-type" highlighting.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/ui/ctrl_dir.c`, `src/ui/input.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 4.38: Implement Bottom F-Key Menu Bar**
*   **Goal:** Shift the existing two-line command footer up by one line and reserve the bottom-most row for a clickable, function-key reference bar (F1 Help, F3 Options, F5 Redraw, F7 View, F8 Split).
*   **Rationale:** Aligns with the standard "Norton Commander" layout familiar to power users. It provides immediate visual cues for function keys, which are often less intuitive than mnemonic letter commands.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 4.39: Implement "Touch" (Make File) Command**
*   **Goal:** Add a command (e.g., `^M` or mapped to a specific key) to create a new, empty file in the current directory, similar to `M` (Make Directory).
*   **Rationale:** Currently, creating a file requires shelling out (`X`) and typing `touch filename`. A native command streamlines the workflow for developers creating placeholders or config files.
*   **Files to Modify:** `src/cmd/mkdir.c`, `src/ui/input.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.40: Enhance Archive Navigation (Tree Logic)**
*   **Goal:** Enable standard Tree Window navigation keys (specifically `Left Arrow` to collapse/parent and `Right Arrow` to expand) while browsing inside an archive.
*   **Rationale:** Navigation inside archives currently feels "flat" or inconsistent compared to the physical filesystem. Unifying these behaviors reduces cognitive load.
*   **Files to Modify:** `src/ui/ctrl_dir.c`
*   **Context Files:** `src/fs/archive.c`
*   - [ ] **Status:** Not Started.

### **Step 4.41: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   **Files to Modify:** `src/ui/input.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.42: Implement Responsive Adaptive Footer**
*   **Goal:** Make the two-line command footer dynamic based on terminal width.
    *   **Compact (< 80 cols):** Show only critical navigation and file operation keys (Copy, Move, Delete, Quit).
    *   **Standard (80-120 cols):** Show the standard set (current behavior).
    *   **Wide (> 120 cols):** Unfold "hidden" or advanced commands (Hex, Group, Sort modes) directly into the footer so the user doesn't have to remember them.
*   **Rationale:** Maximizes utility on large displays while preventing line-wrapping visual corruption on small screens. It reduces the need to consult F1 on large monitors.
*   **Mechanism:** Define command groups (Priority 1, 2, 3). In `DisplayDirHelp` / `DisplayFileHelp`, construct the string dynamically based on `COLS`.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.43: Make "Terminal Classic" the default UI Color and Classic ytree color scheme the example**
*   **Goal:** Update the default color scheme to adhere to the standard "Terminal Classic" aesthetic (typically white/grey text on black background), while providing the traditional "Blue/Yellow" ytree scheme as an easily selectable example or option.
*   **Rationale:** Modern terminal users expect applications to respect their terminal's color palette by default. The classic high-contrast blue scheme can be jarring.
*   **Files to Modify:** `etc/ytree.conf`, `src/ui/color.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.44: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/util/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   **Files to Modify:** `src/util/tabcompl.c`, `src/ui/input.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.45: Implement Fixed-Width Column Mode (Filename Truncation)**
*   **Goal:** Modify the File Window logic to enforce a maximum column width (e.g., 32 characters) even if longer filenames exist. Filenames exceeding this width will be visually truncated (e.g., `00- Introductio~.pdf`) to ensure multiple columns are displayed.
*   **Integration:** Add this as a new mode in the `^F` (File Mode) rotation, or add a configuration toggle (`COMPACT_COLUMNS=1`).
*   **Rationale:** Currently, a single long filename forces the File Window into a inefficient single-column layout. This feature maximizes information density.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/ui/display.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

### **Step 4.46: Implement "Tags-Only" View Mode**
*   **Goal:** Implement a toggle (`*` or `8`) to display only the tagged files in the current File Window.
*   **Rationale:** A core ZTreeWin feature that allows users to verify, refine, and operate on a specific subset of files without the visual clutter of non-tagged files.
*   **Files to Modify:** `src/ui/ctrl_file.c`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   - [ ] **Status:** Not Started.

### **Step 4.47: Command Line File Filter (`-f`)**
*   **Goal:** Allow users to specify an initial file filter (filespec) via command line argument, e.g., `ytree -f "*.c,*.h"`.
*   **Rationale:** Improves startup efficiency for focused tasks and enables better shell aliases (e.g., `alias cview='ytree -f "*.c"'`).
*   **Files to Modify:** `src/core/main.c`, `src/core/init.c`.
*   - [x] **Status:** Completed.

### **Step 4.48: Implement Archive Creation (Pack)**
*   **Goal:** Enable creating new archives implicitly via Copy (`C`/`^K`) and Move (`M`/`^N`).
*   **Description:** If the destination path does not exist and ends with a supported archive extension (e.g., `.zip`, `.tar.gz`, `.tar`), prompt the user to create a new archive.
    *   **Copy:** Create archive and add files.
    *   **Move:** Create archive, add files, and delete originals (effectively "Archiving" them).
*   **Files to Modify:** `src/cmd/copy.c`, `src/cmd/move.c`, `src/fs/archive.c`.
*   - [ ] **Status:** Not Started.

---

## **Phase 5: Major Architectural Refactoring**
*This phase implements core architectural changes required for advanced features.*

### **Step 5.1: Encapsulate Global UI & Configuration State** (Use the Auditor Persona here)
*   **Goal:** Refactor `global.c` to move remaining global variables (e.g., `dir_window`, `file_window`, `sort_order`, `mode`) into a unified `ViewContext` or `Panel` structure. Update function signatures throughout the application to accept a `Context*` pointer instead of relying on global state.
*   **Rationale:** This is the absolute prerequisite for **Step 5.2 (F8 Split Screen)**. Currently, the application architecture assumes a single active view. To display and interact with two independent directory trees side-by-side (e.g., copying from Left to Right), the state must be instance-specific, not global.
*   **Files to Modify:** `src/core/global.c`, `include/ytree.h`
*   **Context Files:** `src/core/init.c`
*   - [x] **Status:** Completed. Global pointers encapsulated in ViewContext. Compatibility maintained via macros.

#### **Step 5.1.1: Encapsulate Directory State**
*   **Goal:** Fix "Stale Cache" crashes by moving `dir_entry_list`, `dir_entry_list_capacity`, and `total_dirs` from static globals in `src/dirwin.c` into the `Volume` structure.
*   **Mechanism:** Update `BuildDirEntryList` to accept a `Volume*` context. Update `HandleDirWindow` to use `CurrentVolume->dir_entry_list`.
*   **Files to Modify:** `src/ui/ctrl_dir.c`, `include/ytree.h`
*   **Context Files:** `src/core/volume.c`
*   - [x] **Status:** Completed.

#### **Step 5.1.2: Encapsulate File List State**
*   **Goal:** Ensure the file list always matches the active volume. Move `file_entry_list` and `file_count` from `src/filewin.c` into the `Volume` structure.
*   **Mechanism:** Refactor `BuildFileEntryList` to populate `CurrentVolume->file_entry_list`.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `include/ytree.h`
*   **Context Files:** `src/core/volume.c`
*   - [x] **Status:** Completed.

#### **Step 5.1.3: Eliminate the `statistic` Macro (The "God Object")**
*   **Goal:** Remove the `#define` in `include/ytree.h`. Refactor function signatures in `src/stats.c`, `src/readtree.c`, and `src/log.c` to accept `Statistic *stats` or `Volume *vol` as an argument. Pass `&CurrentVolume->vol_stats` explicitly from the call sites in `main.c` and `HandleDirWindow`.
*   **Mechanism:** Update all files accessing `statistic` to use explicit pointers.
*   **Files to Modify:** `src/ui/stats.c`, `src/fs/readtree.c`, `src/cmd/log.c`, `include/ytree.h`
*   **Context Files:** `src/core/main.c`, `src/ui/ctrl_dir.c`
*   - [x] **Status:** Completed.

#### **Step 5.1.4: Eliminate UI Magic Numbers**
*   **Goal:** Replace hardcoded `LINES`/`COLS` math with a dynamic layout engine to fix resize bugs and support split-screen.
*   **Mechanism:** Implement `YtreeLayout` struct and `Layout_Recalculate`. Remove static height/width variables from window controllers.
*   **Files to Modify:** `src/core/init.c`, `include/ytree.h`
*   **Context Files:** `src/ui/display.c`
*   - [x] **Status:** Completed.

#### **Step 5.1.5: Fix Volume State Logic**
*   **Goal:** Prevent cursor reset and UI corruption when switching volumes.
*   **Mechanism:** Refactor `HandleDirWindow` navigation logic to clamp cursor position instead of resetting it, and force a full UI refresh on volume switch.
*   **Files to Modify:** `src/ui/ctrl_dir.c`
*   **Context Files:** `src/cmd/log.c`
*   - [x] **Status:** Completed.

### **Step 5.2: Implement F8 Split-Screen Mode**
*   **Goal:** Refactor the application's state management to support two independent file panels. This includes collapsing the stats panel and allowing the user to `Tab` between the two panes for copy/move operations. This will also enable advanced logging features similar to ZTree/XTree (e.g., `Alt-L` to log a tree in the other pane).
*   **Rationale:** This is an essential feature for any advanced file manager and a prerequisite for many efficient file management workflows. This requires a significant refactoring of global state into pane-specific contexts.
*   **Files to Modify:** `src/core/init.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/input.c`
*   **Context Files:** `include/ytree.h`
*   - [x] **Status:** Completed.

#### **Step 5.3: Enhanced Archive Format Detection (UDF/ISO Bridge Fix)**
*   **Description:** Address the issue where `libarchive` defaults to the empty ISO9660 partition on "UDF Bridge" media (e.g., Windows installation ISOs), resulting in a "No Files!" directory view.
*   **Mechanism:** Modify `src/fs/readarchive.c` to implement a **Heuristic Retry Strategy**:
    1.  Perform the standard scan using `archive_read_support_format_all`.
    2.  Count the number of valid files found.
    3.  If the file count is effectively empty (e.g., 0 files, or only `README.TXT`) **AND** the detected format was `ISO9660`:
    4.  **Close and Re-open** the archive, but this time explicitly **disable** ISO9660 support (or enable *only* UDF/other formats via `archive_read_set_format` options if available in the installed library version).
    5.  If the secondary scan finds more files, use that result; otherwise, revert to the initial view.
*   **Rationale:** Ensures users can access data on modern installation media without manual mounting or external tools.
*   **Files to Modify:** `src/fs/readarchive.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

#### **Step 5.4: Implement Non-Destructive Tree Storage (Virtual Collapse)**.
*   **Description:** This addresses the "Archive Collapse" issue. Currently, `ytree` "collapses" a folder by deleting its children from memory. Since archives cannot be easily re-scanned (unlike directories), we cannot "collapse" them without losing data. The fix is to change the data structure so "Collapsing" just hides nodes visually without freeing memory. This is also a prerequisite for efficient Archive Modification (Write support).
*   **Files to Modify:** `src/fs/tree_utils.c`, `src/ui/ctrl_dir.c`, `src/fs/readtree.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 5.5: Physical Source Reorganization**
*   **Goal:** Reorganize the source tree into semantic subdirectories (`src/core`, `src/ui`, `src/fs`, `src/cmd`, `src/util`) to improve project structure.
*   **Rationale:** Decouples modules physically, making the architecture explicit and easier to navigate.
*   **Files to Modify:** All source files (moved), `Makefile`.
*   - [x] **Status:** Completed.

### **Step 5.6: Header Decomposition**
*   **Goal:** Break the monolithic `ytree.h` into modular headers (`ytree_defs.h`, `ytree_fs.h`, `ytree_cmd.h`, `ytree_ui.h`).
*   **Rationale:** Reduces compilation dependency chains and enforces architectural boundaries between layers (Model vs View vs Controller).
*   **Files to Modify:** `include/ytree.h` and new headers.
*   - [x] **Status:** Completed.

### **Step 5.7: Component Extraction**
*This phase separates Logic from UI Controller.*

#### **Step 5.7.1: Extract ui_nav.c (Navigation Logic)**
*   **Goal:** Extract common scrolling logic (Up, Down, PageUp, PageDown) from `dirwin.c` into a reusable module `src/ui/ui_nav.c`.
*   **Rationale:** Removes duplication between directory and file window navigation, enforcing the "Don't Repeat Yourself" (DRY) principle.
*   **Files to Modify:** `src/ui/ui_nav.c` (New), `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `include/ytree_ui.h`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   **Action:** Move `Movedown`, `Moveup`, `Movenpage` logic (generic parts) from `dirwin.c` to `ui_nav.c`, creating generic signatures.
*   - [x] **Status:** Completed.

#### **Step 5.7.2: Extract render_dir.c**
*   **Goal:** Move directory rendering functions from `dirwin.c` and `stats.c` to `src/ui/render_dir.c`.
*   **Rationale:** Separates the "View" (rendering) from the "Controller" (input loop) in the directory context.
*   **Files to Modify:** `src/ui/render_dir.c` (New), `src/ui/ctrl_dir.c`, `src/ui/stats.c`, `include/ytree_ui.h`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   **Action:** Move `PrintDirEntry`, `DisplayTree` from `dirwin.c` and `DisplayDirStatistic` from `stats.c` to `render_dir.c`.
*   - [x] **Status:** Completed.

#### **Step 5.7.3: Extract render_file.c**
*   **Goal:** Move file list rendering functions from `filewin.c` to `src/ui/render_file.c`.
*   **Rationale:** Separates the "View" (rendering) from the "Controller" (input loop) in the file list context.
*   **Files to Modify:** `src/ui/render_file.c` (New), `src/ui/ctrl_file.c`, `include/ytree_ui.h`
*   **Context Files:** `src/ui/ctrl_file.c`
*   **Action:** Move `PrintFileEntry`, `DisplayFiles` from `filewin.c` to `render_file.c`.
*   - [x] **Status:** Completed.

#### **Step 5.7.4: Rename and Purify ctrl_dir.c**
*   **Goal:** Rename `dirwin.c` to `src/ui/ctrl_dir.c` and clean it up to contain *only* the input loop and event dispatching.
*   **Rationale:** Formalizes the Controller role. This file now orchestrates the application but contains no low-level logic or drawing code.
*   **Files to Modify:** `src/ui/ctrl_dir.c`, `Makefile`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   **Action:** Rename file. Verify it calls functions in `render_dir.c` and `ui_nav.c` instead of defining them locally.
*   - [x] **Status:** Completed.

#### **Step 5.7.5: Rename and Purify ctrl_file.c**
*   **Goal:** Rename `filewin.c` to `src/ui/ctrl_file.c` and clean it up to contain *only* the input loop and event dispatching.
*   **Rationale:** Formalizes the Controller role for the file window.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `Makefile`
*   **Context Files:** `src/ui/ctrl_file.c`
*   **Action:** Rename file. Verify it calls functions in `render_file.c` and `ui_nav.c` instead of defining them locally.
*   - [x] **Status:** Completed.

#### **Step 5.8: Extract Volume Menu UI**
*   **Goal:** Separate UI logic from volume management logic in `log.c`.
*   **Action:** Move `SelectLoadedVolume` to `src/ui/vol_menu.c`.
*   - [x] **Status:** Completed.

#### **Step 5.9: Refactor View Module**
*   **Goal:** Split the monolithic `view.c` into semantic modules: internal hex viewer, preview renderer, and external view command.
*   **Action:** Created `src/cmd/view.c` (External), `src/ui/view_internal.c` (Hex/Text Editor), `src/ui/view_preview.c` (F7 Panel). Moved `src/ui/hex.c` to `src/cmd/hex.c`.
*   - [x] **Status:** Completed.

---

## **Phase 6: Advanced Features**
*This phase builds upon the new architecture to add more high-level functionality.*

### **Step 6.1: Implement Advanced Directory Comparison**
*   **Goal:** Implement a comprehensive directory/tree comparison feature that leverages the split-screen view. This will include recursive comparison of two directories, highlighting of file differences (newer, older, unique), and the ability to tag files based on these comparison results.
*   **Rationale:** This is a core power-user feature for synchronizing directories and managing source code, moving `ytree` beyond simple navigation to a true file management utility.
*   **Files to Modify:** `src/cmd/compare.c` (New)
*   **Context Files:** `src/ui/ctrl_file.c`, `src/ui/ctrl_dir.c`
*   - [ ] **Status:** Not Started.

### **Step 6.2: Implement Directory Bookmarks (Hotlist)**
*   **Goal:** Create a "hotlist" feature allowing users to bookmark frequently used directories and quickly jump to them.
*   **Rationale:** Drastically improves navigation efficiency for common workflows, a staple feature of powerful file managers.
*   **Files to Modify:** `src/ui/hotlist.c` (New), `src/ui/input.c`
*   **Context Files:** `src/cmd/log.c`
*   - [ ] **Status:** Not Started.

### **Step 6.3: Implement File Comparison Integration**
*   **Goal:** Integrate external diff tools (e.g., `diff`, `vimdiff`, `meld`) to compare the current file with a tagged file or the file in the adjacent split pane (`J`).
*   **Rationale:** Provides critical functionality for developers and system administrators to identify changes between file versions directly from the file manager.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/cmd/system.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 6.4: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   **Files to Modify:** `src/cmd/batch.c` (New)
*   **Context Files:** `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

### **Step 6.5: Implement "Grep Tagged" (`^S`)**
*   **Goal:** Implement a content search feature (`^S`) that iterates through all tagged files (in File System or Archive). Files that *do not* match the search string are untagged.
*   **Integration:** Combine this with `^V` (View Tagged) to allow the user to quickly narrow down a dataset and view only the hits.
*   **Rationale:** This mimics the powerful "Search" function of XTreeGold, turning `ytree` into a tool for data mining and bulk text analysis.
*   **Files to Modify:** `src/ui/ctrl_file.c`
*   **Context Files:** `src/cmd/execute.c`
*   - [x] **Status:** Completed.

### **Step 6.6: Advanced Destination Selection (Enhanced F2)**
*   **Goal:** Overhaul the F2 (Select Directory) dialog during Copy/Move operations. It should support:
    *   Creating new paths on the fly.
    *   Logging new disks/archives from within the selection window.
    *   Toggling between "Current", "Tree", and "History" views.
*   **Rationale:** currently, selecting a destination is rigid. This flexibility allows users to organize files into new structures without leaving the Copy/Move context.
*   **Files to Modify:** `src/ui/ctrl_dir.c`, `src/ui/input.c`
*   **Context Files:** `src/cmd/log.c`
*   - [ ] **Status:** Not Started.

### **Step 6.7: Implement VFS Abstraction Layer**
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytree` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"

#### **Step 6.7.1: Define VFS Interface & Volume Integration**
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytree_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytree_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.
*   **Files:** `include/ytree_vfs.h`, `include/ytree_defs.h`.

#### **Step 6.7.2: Implement VFS Registry**
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.
*   **Files:** `src/fs/vfs.c`, `include/ytree_vfs.h`.

#### **Step 6.7.3: Implement "Local" VFS Driver**
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/readtree.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.
*   **Files:** `src/fs/drv_local.c`, `src/fs/readtree.c` (cleanup).

#### **Step 6.7.4: Implement "Archive" VFS Driver**
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/readarchive.c` and `src/fs/archive.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.
*   **Files:** `src/fs/drv_archive.c`, `src/fs/readarchive.c` (delete).

#### **Step 6.7.5: Switch `LoginDisk` to VFS**
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.
*   **Files:** `src/cmd/log.c`.

#### **Step 6.7.6: Refactor Consumers (Polymorphism)**
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.
*   **Files:** `src/cmd/*.c`.

---

### **Step 6.8: Implement Recursive Directory Watching** (Use the Auditor Persona here)
*   **Goal:** Extend the file system watcher to monitor all *currently expanded* directories, allowing changes in visible sibling or child directories to appear immediately without manual refresh.
*   **Rationale:** Current `inotify` logic only watches the directory under the cursor. If a user modifies a subdirectory visible in the tree but not currently selected, the UI becomes stale.
*   **Mechanism:**
    *   Refactor `watcher.c` to maintain a **Hash Table** (using `uthash`) mapping Kernel Watch Descriptors (`wd`) to `DirEntry` pointers.
    *   Hook into `ReadTree` (Expansion): Automatically add a watch when a directory is read into memory.
    *   Hook into `UnReadTree`/`DeleteTree` (Collapse): Automatically remove the watch when a directory is freed.
    *   **Safety:** Handle `ENOSPC` (System Watch Limit) gracefully by falling back to "Active-Dir Only" mode if the user expands too many directories.
    *   **Archive Note:** This recursive logic applies to physical filesystems. For Archives, the Auto-Refresh logic (see Step 4.31) watches the *Container File's* timestamp to trigger a virtual tree reload; it does not set recursive watches inside the archive itself.
*   **Files to Modify:** `src/fs/watcher.c`, `src/fs/readtree.c`
*   **Context Files:** `include/watcher.h`
*   - [ ] **Status:** Not Started.

---

## **Phase 7: Internationalization and Configurability**
*   **Goal:** Refactor the application to support localization and user-defined keybindings, moving away from hardcoded English-centric values.

### **Step 7.1: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from `~/.ytree`. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Files to Modify:** `src/ui/input.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 7.2: Standardize Localization Strategy** (Use the Auditor Persona here)
*   **Goal:** Audit the codebase for hardcoded German strings or personal author remarks and convert them to English. Ensure all UI text is wrapped in macros suitable for future `gettext` translation.
*   **Rationale:** Prepares the project for a global audience and ensures consistency in the default locale.
*   **Files to Modify:** `src/*/*.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 7.3: Externalize UI Strings for Internationalization (i18n)**
*   **Description:** Move all user-facing strings (menus, prompts, error messages) out of the C code and into a separate, loadable resource file. This is a prerequisite for translation. The application will load the appropriate language file at startup.
*   **Files to Modify:** `src/util/i18n.c` (New), `src/*/*.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 7.4: Implement Command Line Depth Override**
*   **Goal:** Add a `-d <depth>` (or `--depth`) command line argument to override the `TREEDEPTH` configuration setting at startup.
*   **Rationale:** Allows users and scripts to force a deep scan immediately upon launch without requiring interactive key presses (`*`), significantly improving automation capabilities and workflow speed for deep directory structures.
*   **Files to Modify:** `src/core/main.c`, `src/core/init.c`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

---

## **Phase 8: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Step 8.1: Convert Man Page to Markdown**
*   **Description:** The traditional `groff` man page (`ytree.1`) was converted to Markdown (`ytree.1.md`) and is now generated at build time using `pandoc`. This makes the documentation significantly easier to read, edit, and maintain.
*   **Files to Modify:** `Makefile`, `doc/ytree.1.md`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### **Step 8.2: Integrate Test Suite into Build Process**
*   **Goal:** Add a `test` target to the `Makefile` to simplify running the automated test suite.
*   **Rationale:** Formalizes the testing process and makes it trivial for any contributor to validate their changes by simply running `make test`.
*   **Files to Modify:** `Makefile`
*   **Context Files:** None.
*   - [x] **Status:** Completed.

### Step 8.3: Expand and Integrate Test Suite
*   **Goal:** Add a `test` target to the `Makefile` and expand the regression suite from "Core Ops" to "Comprehensive Coverage".
*   **Rationale:** Formalizes the testing process and ensures reliability before release.
*   **Scope:**
    *   **Archive Formats:** Verify read/extract for ZIP, TAR.GZ, TAR.BZ2.
    *   **Split Screen:** Verify file operations between Left and Right panels.
    *   **Tagging:** Verify Invert Tags, Tag All, and Tag Filtering.
    *   **Sorting:** Verify sort orders (Name, Size, Date) actually change the list order.
    *   **Edge Cases:** Empty directories, read-only files, root directory operations.
*   **Files to Modify:** `tests/test_core.py`, `tests/test_volume.py` (New)
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 8.4: Implement CI with GitHub Actions**
*   **Goal:** Set up a basic Continuous Integration workflow to automatically build the project and run the test suite on every push and pull request.
*   **Rationale:** Enforces code quality and prevents regressions by ensuring that all changes are automatically validated.
*   **Files to Modify:** `.github/workflows/ci.yml` (New)
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 8.5: Enhance Build System**
*   **Goal:** Improve Makefile targets for installation and add a proper `uninstall` target to clean up all installed files. Modernize versioning by moving version info from `patchlev.h` into the `Makefile`.
*   **Rationale:** Provides a more robust and complete build system for end-users and packagers.
*   **Files to Modify:** `Makefile`
*   **Context Files:** `include/patchlev.h`
*   - [ ] **Status:** Partially Completed (uninstall exists but can be improved).

### **Step 8.6: Implement De-linting, Static Analysis, and Code Hygiene** (Use the Auditor Persona here)
*   **Goal:** Add `make format`, `make lint`, and `make analyze` targets to the Makefile.
    *   **Format:** Use `clang-format` to enforce a consistent coding style.
    *   **Lint/Analyze:** Use static analysis tools (e.g., `cppcheck`, `clang-tidy`, `flawfinder`) to identify potential bugs and non-standard code.
    *   **Action:** Perform a comprehensive **de-linting** pass to resolve existing warnings, remove dead code (unused variables/functions), and strip obsolete debug comments/cruft.
*   **Rationale:** Automates code quality enforcement. A clean, lint-free codebase minimizes technical debt and ensures the project is professional and maintainable for public release.
*   **Files to Modify:** `Makefile`, `src/**/*.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 8.7: Finalize Documentation**
*   **Goal:** Update the `CHANGELOG`, `README.md`, and `CONTRIBUTING.md` files to reflect all new features and changes before a release.
*   **Rationale:** Ensures users and developers have accurate, up-to-date information about the project.
*   **Files to Modify:** `README.md`, `doc/CHANGES.md`, `doc/CONTRIBUTING.md`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 8.8: Initialize Distributed Issue Tracking (git-bug)**
*   **Goal:** Configure `git-bug` to act as a bridge between the local repository and GitHub Issues. Migrate the contents of `BUGS.md` and `TODO.txt` into this system prior to public release.
*   **Rationale:** Allows the developer to maintain a simple local text-based workflow during heavy development, while ensuring that all tracking data can be synchronized to the public web interface when the project goes live.
*   **Files to Modify:** `doc/BUGS.md`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

---

## **Phase 9: Systems Architecture Remediation & Core Hardening**
*This phase represents the primary technical debt payoff for the v3.0 modernization. It transitions `ytree` from a monolithic legacy C89 codebase into a layered, modular architecture. It systematically decouples Business Logic from Presentation (Model-View Separation), enforces strict Memory Safety, eliminates unsafe Signal Handling patterns, and standardizes low-level System I/O mechanisms.*

### **Step 9.1: Deep-Dive Architectural Remediation**
*   **Goal:** Execute the specific refactoring tasks identified in the code quality audit to eliminate fragile patterns, enforcing strict separation of Logic (Model) and UI (View).
*   **Status:** In Progress.

#### **Step 9.1.1: Decouple Filesystem Scanners (readtree.c)**
*   **Goal:** Remove UI calls (`DrawSpinner`, `doupdate`) from low-level filesystem logic.
*   **Mechanism:** Refactor `ReadTree` to accept a `ProgressCallback` function pointer. Move the UI update logic into a callback provided by the Controller layer.
*   **Files to Modify:** `src/fs/readtree.c`, `src/fs/readarchive.c`, `src/cmd/log.c`.
*   - [x] **Status:** Completed.

#### **Step 9.1.2: Extract Sorting Logic (Model vs Controller)**
*   **Goal:** Move `qsort` and comparison logic out of the UI Controller (`ctrl_file.c`) and into the Core/Model layer.
*   **Mechanism:** Create `src/core/sort.c`. Implement `Volume_Sort(Volume *vol, int method)`.
*   **Files to Modify:** `src/ui/ctrl_file.c`, `src/core/sort.c` (New).
*   - [x] **Status:** Completed.

#### **Step 9.1.3: Refactor LoginDisk (Split I/O and UI)**
*   **Goal:** Split the monolithic `LoginDisk` function into a pure logic function (`Volume_Load`) and a UI wrapper (`Cmd_LoginDisk`).
*   **Mechanism:** `Volume_Load` performs I/O and memory allocation only. `Cmd_LoginDisk` handles prompts, screen clearing, and error display.
*   **Files to Modify:** `src/cmd/log.c`, `src/core/volume.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.4: UI Geometry Standardization (Magic Numbers)**
*   **Goal:** Eliminate hardcoded screen offsets (e.g., `LINES - 4`) used for footer/prompt positioning.
*   **Mechanism:** Extend `YtreeLayout` struct to include footer/prompt geometry. Calculate these once in `Layout_Recalculate`. Replace magic numbers with `layout.footer_y`, etc.
*   **Files to Modify:** `src/ui/display.c`, `src/ui/stats.c`, `src/ui/prompt.c`, `src/ui/vol_menu.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.5: Standardization of Path Parsing**
*   **Goal:** Replace manual pointer-arithmetic path parsing loops with standard functions.
*   **Mechanism:** Refactor `Fnsplit` and similar helpers to use POSIX `dirname()` and `basename()`.
*   **Files to Modify:** `src/util/path_utils.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.6: Encapsulate Command Context**
*   **Goal:** Prevent commands from implicitly operating on the global `CurrentVolume`.
*   **Mechanism:** Refactor `Execute`, `Delete`, and `UserMode` functions to accept a `Volume*` or `Statistic*` context argument. Update callers in `ctrl_dir.c`/`ctrl_file.c` to pass `ActivePanel->vol`.
*   **Files to Modify:** `src/cmd/execute.c`, `src/cmd/delete.c`, `src/cmd/usermode.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.7: Secure String Construction**
*   **Goal:** Eliminate unsafe manual string substitution loops.
*   **Mechanism:** Create `String_Replace()` utility using `snprintf` for bounds-checked substitution of `{}` placeholders.
*   **Files to Modify:** `src/cmd/execute.c`, `src/util/string_utils.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.8: Global Error Buffer Elimination**
*   **Goal:** Remove the non-reentrant global `message` buffer.
*   **Mechanism:** Refactor error macros to use variadic functions (`UI_Message(fmt, ...)`) with local stack buffers.
*   **Files to Modify:** `src/core/global.c`, `src/ui/error.c`, `include/ytree.h`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.9: Modernize User/Group Database Handling**
*   **Goal:** Remove legacy caching of `/etc/passwd` which wastes memory and fails on networked systems.
*   **Mechanism:** Replace `ReadPasswdEntries` cache with direct calls to `getpwuid`/`getgrgid`.
*   **Files to Modify:** `src/cmd/passwd.c`, `src/cmd/group.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.10: Centralize Memory Management (DRY)**
*   **Goal:** Eliminate repetitive `malloc` error checking code scattered across the project.
*   **Mechanism:** Implement `xmalloc`, `xcalloc`, and `xstrdup` in `src/util/memory.c`. These wrappers exit safely on allocation failure. Refactor codebase to use them.
*   **Files to Modify:** `src/util/memory.c` (New), `include/ytree.h`, all source files.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.11: Standardize Internal Viewer Geometry**
*   **Goal:** Decouple the internal Hex/Text viewer from global `LINES`/`COLS` to allow future split-screen integration.
*   **Mechanism:** Update `view_internal.c` to respect the global `YtreeLayout` or accept specific bounds.
*   **Files to Modify:** `src/ui/view_internal.c`.
*   - [ ] **Status:** Not Started.

#### **Step 9.1.12: Enforce Signal Safety (Anti-Crash)**
*   **Goal:** Eliminate "Random" crashes caused by unsafe signal handling (SIGALRM/SIGSEGV).
*   **Mechanism:**
    1.  **Clock:** Remove `SIGALRM` based clock. Move clock updates to the `GetEventOrKey` input loop using `timeout()`.
    2.  **Shutdown:** Remove `endwin()` from signal handlers. Use `sig_atomic_t` flags to trigger graceful shutdown in the main loop.
*   **Files to Modify:** `src/core/clock.c`, `src/core/main.c`, `src/ui/input.c`.
*   - [ ] **Status:** Not Started.

### **Step 9.2: SRP & SoC Enforcement (Audit Execution)**
*   **Goal:** Execute the critical refactorings identified in the Architectural Audit to enforce Single Responsibility and Separation of Concerns.
*   **Rationale:** Decoupling Model (FS), View (UI), and Controller (Input) is prerequisite for stability and testing.

### **Step 9.3: Refactor Input Subsystem (Prompt Manager)**
*   **Goal:** Replace the fragile `InputStringEx` + Callback mechanism with a robust **Prompt Manager**.
*   **Mechanism:** Create a `UI_Prompt` context that handles drawing the *entire* prompt area (Help text + Input Field) automatically. This removes drawing logic from `cmd/*.c` modules.
*   **Target:** `src/ui/input.c`, `src/cmd/copy.c`, `src/cmd/move.c`, `src/ui/prompt.c`.
*   - [x] **Status:** Completed.

### **Step 9.4: Standardize Menu/Popup System**
*   **Goal:** Replace ad-hoc window creation in `vol_menu.c` and `history.c` with a centralized **Menu Manager**.
*   **Mechanism:** Implement `UI_Menu(items, title, callback)` that handles window sizing, scrolling, and borders consistently, respecting the global layout engine.
*   **Target:** `src/ui/vol_menu.c`, `src/util/history.c`.
*   - [ ] **Status:** Not Started.

### **Step 9.5: Centralize Window Management**
*   **Goal:** Ensure all secondary windows (F2, Error, Help) use the `ViewContext` layout engine rather than hardcoded geometry.
*   **Target:** `src/ui/display.c`, `src/core/init.c`.
*   - [ ] **Status:** Not Started.

---

## **Phase 10: Valgrind Audit & Optimization**
*This phase focuses on deep stability, memory safety, and performance profiling to ensure `ytree` is robust enough for long-running sessions.*

### **Step 10.1: Establish Valgrind Baseline**
*   **Goal:** Create a reproducible test case (scripted via the Python test suite) that exercises core features (Log, Copy, Move, Refresh, Exit) while running under Valgrind Memcheck.
*   **Rationale:** Provides a consistent benchmark to measure memory usage and identify existing leaks before optimization begins.
*   **Files to Modify:** `tests/valgrind_test.py` (New)
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 10.2: Memory Leak Remediation** (Use the Auditor Persona here)
*   **Goal:** Systematically analyze Valgrind reports and fix all "Definitely Lost" and "Indirectly Lost" memory errors.
*   **Rationale:** Prevents memory bloat over time, which is critical for a file manager that may be left open for days.
*   **Files to Modify:** `src/**/*.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 10.3: Fix Uninitialized Memory Access** (Use the Auditor Persona here)
*   **Goal:** Identify and fix "Conditional jump or move depends on uninitialized value(s)" errors.
*   **Rationale:** These are often the cause of sporadic, unreproducible crashes and erratic behavior.
*   **Files to Modify:** `src/**/*.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 10.4: Resource Leak Cleanup (File Descriptors)** (Use the Auditor Persona here)
*   **Goal:** Ensure all file descriptors (`open`, `opendir`, `popen`) are properly closed, especially in error paths and during volume switching.
*   **Rationale:** Prevents "Too many open files" errors during heavy usage.
*   **Files to Modify:** `src/cmd/log.c`, `src/cmd/pipe.c`, `src/fs/archive.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

---

## **Phase 11: Public Release Preparation**
*This phase ensures the project is presentable for a public "Developer Preview," emphasizing the architectural achievements over raw feature completeness.*

### **Step 11.0: Release Artifacts**
*   **Goal:** Standardize versioning, document the new architecture, and set user expectations.
*   **Action:**
    *   Set version to `3.0.0-alpha`.
    *   Create `ARCHITECTURE.md` to document the refactoring (MVC, VFS, No God Objects).
    *   Update `README.md` with status banners and project motivation.
*   - [x] **Status:** Completed.

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