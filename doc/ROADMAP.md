# **Modernization Roadmap**

---

## **Phase 2: Foundational C-Standard Modernization**
*This phase fixed critical bugs, established C99/POSIX standards compliance for core utilities, and removed reliance on custom, error-prone functions.*

---

## **Phase 4: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

#### **Step 4.8.7: Implement Directory Filtering (Non-Recursive)**
*   **Description:** Extend the Filter logic to support directory patterns (identified by a trailing slash, e.g., `-obj/` or `build/`). Update the Directory Tree window view to apply these filters. **Note: This logic is non-recursive; it only affects the visibility of directories in the current view, not their internal logged state.**
*   **Files to Modify:** `src/cmd/filter.c`, `src/ui/ctrl_dir.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [ ] **Status:** Not Started.

#### **Step 4.14.1: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.
*   - [ ] **Status:** Not Started.

#### **Step 4.14.2: Refactor Input Loop for Event Handling**
*   **Task:** Modify `key_engine.c` to support non-blocking input handling.
*   **Logic:**
    *   Currently, `Getch()` blocks indefinitely waiting for a key.
    *   **New Logic:** Use `select()` or `poll()` to wait on **two** file descriptors:
        1.  `STDIN_FILENO` (The keyboard).
        2.  `Watcher_GetFD()` (The inotify handle).
    *   If the Watcher FD triggers, call `Watcher_CheckEvents()`. If it confirms a change, set a global flag `refresh_needed = TRUE`.
    *   If STDIN triggers, proceed to `wgetch()`.
*   - [ ] **Status:** Not Started.

#### **Step 4.14.3: Implement Live Refresh Logic**
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
*   - [ ] **Status:** Not Started.

#### **Step 4.14.4: Update Watch Context on Navigation**
*   **Task:** Ensure the watcher always monitors the *current* directory.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:* Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytree`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.
*   - [ ] **Status:** Not Started.

### **Step 4.18: Implement Directory Graft (Copy/Move)**
*   **Goal:** Implement directory manipulation commands in the Directory Window.
    *   **Graft (Move):** Move an entire directory branch to a new parent.
    *   **PathCopy (Copy):** Recursively copy a directory branch to a new location.
*   **Rationale:** Essential directory management features present in XTree&trade; but missing in ytree.
*   **Files to Modify:** `src/ui/ctrl_dir.c` (Navigate to `f2_picker.c`)
*   **Context Files:** `src/cmd/copy.c`, `src/cmd/move.c`
*   - [ ] **Status:** Not Started.

### **Step 4.24: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good, but `pathcop(Y)/^Y` is redundant and should just be `pathcop(Y)`).
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.26: Implement Integrated Help System**
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** `src/ui/key_engine.c`
*   - [ ] **Status:** Not Started.

### **Step 4.27: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option (e.g., in a future F10 settings panel) to globally disable external viewers, forcing the use of the internal viewer.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Files to Modify:** `src/cmd/view.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 4.29: Implement Advanced Log Options**
*   **Goal:** Enhance the `Log` command to present options for controlling the scan depth and scope, similar to ZTree/XTree&trade; (e.g., "Log drive", "Log tree", "Log directory").
*   **Rationale:** Provides essential control over performance when working with very large filesystems, allowing the user to perform a shallow scan when a deep recursive scan is not needed.
*   **Files to Modify:** `src/cmd/log.c`, `src/ui/key_engine.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [ ] **Status:** Not Started.

### **Step 4.30: Archive Write Support (Atomic Breakdown)**
*   **Goal:** Enable modification of archives (ZIP, TAR, ISO, etc.) directly within ytree. Since `libarchive` does not support random-access modification, this requires a "Stream Rewrite" engine: open original, open temp destination, stream entries from old to new (skipping deleted ones, injecting new ones), close, and swap. The Auto-Refresh logic for archives will need to watch the archive file itself (the container) to trigger reloads.

#### **Step 4.30.6: Implement Archive Move (`M`) Support**
*   **Description:** Implement `M` (Move) for archives. Intra-archive moves use the Rewrite Engine to rename paths. Cross-volume moves use Copy-Extract + Delete.
*   **Files to Modify:** `src/cmd/move.c`, `src/fs/archive_write.c`.
*   - [ ] **Status:** Not Started.

### **Step 4.33: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   **Files to Modify:** `src/fs/archive_read.c`, `src/cmd/log.c`
*   **Context Files:** `src/fs/archive_read.c`
*   - [ ] **Status:** Not Started.

### **Step 4.40: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   **Files to Modify:** `src/ui/key_engine.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.41: Implement Responsive Adaptive Footer**
*   **Goal:** Make the two-line command footer dynamic based on terminal width.
    *   **Compact (< 80 cols):** Show only critical navigation and file operation keys (Copy, Move, Delete, Quit).
    *   **Standard (80-120 cols):** Show the standard set (current behavior).
    *   **Wide (> 120 cols):** Unfold "hidden" or advanced commands (Hex, Group, Sort modes) directly into the footer so the user doesn't have to remember them.
*   **Rationale:** Maximizes utility on large displays while preventing line-wrapping visual corruption on small screens. It reduces the need to consult F1 on large monitors.
*   **Mechanism:** Define command groups (Priority 1, 2, 3). In `DisplayDirHelp` / `DisplayFileHelp`, construct the string dynamically based on `COLS`.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.43: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/util/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   **Files to Modify:** `src/util/tabcompl.c`, `src/ui/key_engine.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.45: Implement "Tags-Only" View Mode**
*   **Goal:** Implement a toggle (`*` or `8`) to display only the tagged files in the current File Window.
*   **Rationale:** A core ZTreeWin feature that allows users to verify, refine, and operate on a specific subset of files without the visual clutter of non-tagged files.
*   **Files to Modify:** `src/ui/ctrl_file.c`
*   **Context Files:** `src/ui/ctrl_dir.c`
*   - [ ] **Status:** Not Started.

### **Step 4.48: Standardize Internal Viewer Layout**
*   **Goal:** Ensure the internal viewer's layout geometry matches the main application (borders, headers, and footer).
*   **Files to Modify:** `src/ui/view_internal.c`
*   - [ ] **Status:** Not Started.

### **Step 4.51: Implement Global File View (`G`)**
*   **Goal:** Implement the "Global" file view mode, which aggregates files from **all currently logged volumes** into a single flattened list.
*   **Rationale:** A core ZTree feature allowing operations across multiple drives simultaneously.
*   **Prerequisite:** Free up `G` key.
*   **Files to Modify:** `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/core/volume.c`
*   - [ ] **Status:** Not Started.

---

## **Phase 6: Advanced Features**
*This phase builds upon the new architecture to add more high-level functionality.*

### **Step 6.4: Implement Advanced Batch Rename**
*   **Goal:** Enhance the `Rename` command to support advanced masks (e.g., `*_<001>.bak`), sequential numbering, casing changes (`Tab`), and substring replacement.
*   **Rationale:** Essential power-user feature for managing large file sets.
*   **Files to Modify:** `src/cmd/rename.c`, `src/ui/key_engine.c`
*   - [ ] **Status:** Not Started.

### **Step 6.5: Enhance PathCopy to Mirror (Sync)**
*   **Goal:** Enhance the existing `PathCopy` (`^Y`) logic to support "Mirroring". Add options to **synchronize** the destination (delete orphan files not present in source) and compare size and timestamps.
*   **Rationale:** `PathCopy` currently acts as a "Copy", but lacks the "Sync" capabilities of ZTree's `Alt-Mirror`.
*   **Files to Modify:** `src/cmd/copy.c`, `src/cmd/mirror.c` (New)
*   - [ ] **Status:** Not Started.

---

## **Phase 7: Internationalization and Configurability**
*   **Goal:** Refactor the application to support localization and user-defined keybindings, moving away from hardcoded English-centric values.

### **Step 7.1: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from `~/.ytree`. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Files to Modify:** `src/ui/key_engine.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Step 7.2: Standardize Localization Strategy** (Use the Code Auditor persona here)
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

---

## **Phase 8: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Step 8.3: Restructure and Expand Test Suite**
*   **Goal:** Tidy up existing test scripts into a coherent, modular structure and thoroughly expand the regression suite for comprehensive coverage.
*   **Rationale:** A well-structured test suite is easier to maintain and extend. Thorough, systematic coverage ensures reliability and prevents regressions across complex file operations.
*   - [ ] **Status:** Not Started.

### **Step 8.5: Enhance Build System**
*   **Goal:** Improve Makefile targets for installation and add a proper `uninstall` target to clean up all installed files. Modernize versioning by moving version info from `patchlev.h` into the `Makefile`.
*   **Rationale:** Provides a more robust and complete build system for end-users and packagers.
*   **Files to Modify:** `Makefile`
*   **Context Files:** `include/patchlev.h`
*   - [x] **Status:** Partially Completed (uninstall exists but can be improved).

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

## **Phase 11: Architectural Integrity (SRP/SoC Audit)**
*This phase addresses specific "Fragile Code" patterns identified during the deep-dive audit. The goal is to strictly enforce the Single Responsibility Principle (SRP) and Separation of Concerns (SoC) to prevent regression loops and enable safe future expansion.*

### **Step 11.2: Decouple Logic from UI in Core Commands** (Use the Architect persona here)
*   **Task ID:** [CMD]-[Copy/Move]-[SoC]
*   **Severity:** **High**
*   **The Fragile Code:** `CopyFile` (`src/cmd/copy.c`) and `MoveFile` (`src/cmd/move.c`) call `InputChoice(...)` internally to ask for overwrite confirmation.
*   **The Issue:** "Business Logic" functions are holding the thread hostage for UI input. This prevents scripting, batch operations, or alternative interfaces.
*   **The Robust Fix:**
    1.  Refactor `CopyFile` and `MoveFile` to accept a `ConfirmationCallback` function pointer (or a comprehensive flags bitmask like `OPS_INTERACTIVE | OPS_FORCE`).
    2.  If the destination exists, the logic invokes the callback: `cb(CTX_OVERWRITE, filename)`.
    3.  The callback handles the `InputChoice` UI and returns `YES`, `NO`, or `ALL`.
*   **Files to Modify:** `src/cmd/copy.c`, `src/cmd/move.c`, `include/ytree_cmd.h`.
*   - [x] **Status:** Completed.

### **Step 11.4: Standardize Path Construction (Safety)** (Use the Architect persona here)
*   **Task ID:** [Util]-[Path]-[UnsafeString]
*   **Severity:** **High**
*   **The Fragile Code:** Various commands (`copy.c`, `move.c`) use ad-hoc `strcpy`/`strcat` sequences to build paths:
    ```c
    strcpy(path, dir);
    if (*path != '/') strcat(path, "/");
    strcat(path, file);
    ```
*   **The Issue:** This pattern is error-prone (buffer overflows, double slashes, missing slashes) and repetitive (DRY violation).
*   **The Robust Fix:**
    1.  Implement `Path_Join(char *dest, size_t size, const char *dir, const char *file)` in `src/util/path_utils.c`.
    2.  This function must handle separator insertion/deduplication and `snprintf` bounds checking centrally.
    3.  Refactor all command modules to use this helper.
*   **Files to Modify:** `src/util/path_utils.c`, `src/cmd/copy.c`, `src/cmd/move.c`, `src/cmd/rename.c`, `src/cmd/mkdir.c`.

### **Step 11.5: Encapsulate Internal Viewer Geometry** (Use the Architect persona here)
*   **Task ID:** [UI]-[View]-[Encapsulation]
*   **Severity:** **Medium**
*   **The Fragile Code:** `src/ui/view_internal.c` creates windows (`VIEW`, `BORDER`) based on global `layout` coordinates but manages its own resize logic (`DoResize`) that competes with the main layout engine.
*   **The Issue:** The internal viewer acts as a separate application mode rather than a component. It forces `Getch` loops that are disconnected from the main event loop.
*   **The Robust Fix:**
    1.  Refactor `InternalView` to accept a `WINDOW *parent` or `YtreeGeometry` struct.
    2.  Ensure it respects the bounds passed by the caller, allowing it to eventually run inside a Split Screen pane (future proofing).
*   **Files to Modify:** `src/ui/view_internal.c`.

### **Step 11.6: Strict Header Hygiene** (Use the Architect persona here)
*   **Task ID:** [Core]-[Build]-[Coupling]
*   **Severity:** **Low (Maintenance)**
*   **The Fragile Code:** `ytree.h` includes every other header. Every `.c` file sees every prototype.
*   **The Issue:** Changing a struct in `fs` triggers a recompile of `ui`.
*   **The Robust Fix:**
    1.  Remove `#include "ytree_*.h"` from `ytree.h`.
    2.  Update `.c` files to include only the specific headers they need (e.g., `ctrl_dir.c` needs `ytree_ui.h` and `ytree_fs.h`).
    *Note: This is a large task; prioritize after functional hardening.*
*   **Files to Modify:** `include/ytree.h` and all `src/**/*.c`.

---

## **Future Enhancements / Wishlist**
*A collection of high-complexity or lower-priority features to be considered after the primary roadmap is complete.*

*   **Implement Shell Script Generator:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept. (Relocated from Phase 6).
*   **Callback API Constification Cleanup (cppcheck strict mode):** `cppcheck` suggests const-qualifying callback `user_data`, but doing this correctly likely requires changing callback typedef/API signatures (e.g., `RewriteCallback`) and related call sites. Defer this to a focused API pass to avoid scattered casts and partial churn.
*   **Per-Window Filter State (Split Screen Prerequisite):** Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left pane versus `*.h` in the right).
*   **Implement Advanced, ncurses-native Command Line Editing:** Full cursor navigation (left/right, home/end, word-by-word). In-line text editing (insert, delete, backspace, clear line). Persistent command history accessible via up/down arrows. Maybe even context-aware tab completion for files and directories.
*   **Shell-Style Tab Completion:** Replace the current history-based tab completion with true filename/directory completion in the input line.
*   **Print Functionality:** Investigate and potentially implement a "Print" command that is distinct from "Pipe," if a clear use case for modern systems can be defined.
*   **Terminal-Independent TUI Mode:** Investigate a mode to run the `ytree` TUI directly on a **Unix virtual console** (e.g., via `/dev/tty*` or framebuffer), independent of a terminal emulator session.
*   **Windows Port:** Adapt the build system and platform-specific APIs to support compilation and execution on modern Windows via MinGW-w64 and PDCurses.
*   **Scroll bars:**  On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list, (configurable to char or line).
*   **State Preservation on Reload (^L):** Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   **Preserve Tree Expansion on Refresh:** Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.
*   **Implement "Safe Delete" (Trash Can):** Support the FreeDesktop.org Trash specification to allow file recovery.

### **Step 4.m: Applications menu**
*   Implement a customizable Application Menu.
*   **Files to Modify:** `src/cmd/usermode.c`, `src/cmd/profile.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Step 4.n: Implement In-App Configuration Editor (F10)**
*   **Goal:** Implement a settings panel (activated by a key like `F10`) that allows the user to view and change configuration options from `~/.ytree` (e.g., `CONFIRMQUIT`, colors) and save the changes.
*   **Rationale:** Provides a user-friendly way to configure `ytree` without manually editing the configuration file, improving accessibility.
*   **Files to Modify:** `src/cmd/profile.c`, `src/ui/key_engine.c`
*   **Context Files:** `include/config.h`
*   - [ ] **Status:** Not Started.

### **Step 4.o: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** A key feature of classic file managers like ZTreeWin and modern ones like Midnight Commander, mouse support dramatically improves speed and ease of use for users in capable terminal environments.
*   **Files to Modify:** `src/ui/key_engine.c`
*   **Context Files:** `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

### **Step 6.n: Implement Keyboard Macros (`F12`)**
*   **Goal:** Implement a macro recording and playback system. `F12` starts/stops recording keystrokes to a buffer/file.
*   **Rationale:** Allows automation of repetitive tasks (e.g., "Tag, Move, Rename, Repeat").
*   **Files to Modify:** `src/ui/key_engine.c`, `src/core/macro.c` (New)
*   - [ ] **Status:** Not Started.

### **Step 6.o: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   **Files to Modify:** `src/cmd/batch.c` (New)
*   **Context Files:** `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

### **Step 6.p: Implement Recursive Directory Watching** (Use the Code Auditor persona here)
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

### **Step 6.x: Implement VFS Abstraction Layer** (Use the Architect persona here)
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytree` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"

#### **Step 6.x.1: Define VFS Interface & Volume Integration** (Use the Architect persona here)
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytree_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytree_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.
*   **Files:** `include/ytree_vfs.h`, `include/ytree_defs.h`.

#### **Step 6.x.2: Implement VFS Registry** (Use the Architect persona here)
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.
*   **Files:** `src/fs/vfs.c`, `include/ytree_vfs.h`.

#### **Step 6.x.3: Implement "Local" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/readtree.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.
*   **Files:** `src/fs/drv_local.c`, `src/fs/readtree.c` (cleanup).

#### **Step 6.x.4: Implement "Archive" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/readarchive.c` and `src/fs/archive.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.
*   **Files:** `src/fs/drv_archive.c`, `src/fs/readarchive.c` (delete).

#### **Step 6.x.5: Switch `LogDisk` to VFS** (Use the Architect persona here)
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.
*   **Files:** `src/cmd/log.c`.

#### **Step 6.x.6: Refactor Consumers (Polymorphism)** (Use the Architect persona here)
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.
*   **Files:** `src/cmd/*.c`.
