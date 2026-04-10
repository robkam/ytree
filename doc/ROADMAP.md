# **Modernization Roadmap**

---

## **Phase 0: Security Hardening Baseline**
*This phase is a release gate: security hardening lands before new feature work.*

### **Task 0.1: Refactor Shell Command Construction to a Single Security Boundary**
*   **Goal:** Replace scattered shell string assembly with one hardened command-building boundary for `view`, `execute`, compare helpers, and user-defined actions.
*   **Rationale:** Current per-call-site escaping is inconsistent and fragile; one trusted path prevents command injection drift.
*   **Mechanism:** Centralize quoting/interpolation policy (including placeholder expansion), enforce bounded buffers, and reject unsafe/truncated command materialization.
*   **Files to Modify:** `src/cmd/view.c`, `src/cmd/execute.c`, `src/cmd/usermode.c`, `src/ui/file_compare.c`, `src/ui/dir_compare.c`, `src/ui/interactions.c`, `src/util/path_utils.c`
*   **Context Files:** `include/ytree_cmd.h`, `include/ytree_fs.h`, `include/ytree_ui.h`
*   - [x] **Status:** Completed.

### **Task 0.2: Retire Unsafe Escaping APIs (`StrCp` / legacy shell quote variants)**
*   **Goal:** Replace unbounded escaping helpers with size-aware APIs and remove legacy entry points from runtime paths.
*   **Rationale:** Unbounded escaping enables overflow classes and inconsistent semantics across modules.
*   **Mechanism:** Introduce/standardize bounded escape APIs only, migrate all call sites, and keep any compatibility wrapper isolated and non-production.
*   **Files to Modify:** `src/util/string_utils.c`, `src/util/path_utils.c`, `src/cmd/*.c`, `src/ui/*.c`
*   **Context Files:** `include/ytree_defs.h`, `include/ytree.h`, `include/ytree_ui.h`
*   - [x] **Status:** Completed.

### **Task 0.3: Enforce Archive Internal-Path Trust Policy**
*   **Goal:** Reject unsafe archive member paths before indexing or extraction (absolute paths, `..`, empty segments, and normalization ambiguities).
*   **Rationale:** Archive traversal can escape extraction roots and poison command/view flows.
*   **Mechanism:** Add a single canonical internal-path validator and apply it in archive read, lookup, and extraction paths.
*   **Files to Modify:** `src/fs/archive_read.c`, `src/ui/interactions.c`, `src/cmd/execute.c`, `src/ui/view_preview.c`
*   **Context Files:** `include/ytree_fs.h`
*   - [x] **Status:** Completed.

### **Task 0.4: Replace Predictable `/tmp` Artifacts with Secure Temp Lifecycle**
*   **Goal:** Remove fixed temp filenames and migrate to `mkstemp`/`mkdtemp` + controlled cleanup for preview/extract/debug artifacts.
*   **Rationale:** Predictable `/tmp` paths create symlink/TOCTOU and cross-user disclosure risk.
*   **Mechanism:** Use per-process random temp paths, avoid reopening by name where possible, and gate debug logging behind explicit opt-in path config.
*   **Files to Modify:** `src/ui/view_preview.c`, `src/ui/interactions.c`, `include/ytree_defs.h`, `include/ytree_debug.h`
*   **Context Files:** `src/cmd/view.c`, `src/cmd/hex.c`
*   - [ ] **Status:** Not Started.

### **Task 0.5: Add Regression Tests for Shell Injection and Escaping Boundaries**
*   **Goal:** Lock in non-exploitability for metacharacter-heavy file paths and command placeholders.
*   **Coverage:** `;`, `&`, backslash, single quote, whitespace, and placeholder expansion in compare/view/execute paths.
*   **Files to Modify:** `tests/test_compare_actions.py`, `tests/test_commands_exhaustive.py` (or new `tests/test_security_shell_paths.py`)
*   - [ ] **Status:** Not Started.

### **Task 0.6: Add Regression Tests for Archive Traversal Rejection**
*   **Goal:** Prove that `../` and absolute archive entries are ignored/rejected and never extracted outside allowed roots.
*   **Coverage:** tree-load visibility, extract/copy/view flows, and malformed path variants.
*   **Files to Modify:** `tests/test_archive_ui.py`, `tests/test_archive_write_parity.py` (and/or a focused archive security driver test)
*   - [ ] **Status:** Not Started.

### **Task 0.7: Add Regression Tests for Temp-Artifact Safety**
*   **Goal:** Ensure fixed `/tmp/ytree_*` names are no longer used for preview/debug/extract paths.
*   **Coverage:** archive preview cache behavior, tagged-view extraction temp roots, and debug logging path policy.
*   **Files to Modify:** `tests/test_f7_preview.py`, `tests/test_archive_exit_ui.py` (or new `tests/test_security_tempfiles.py`)
*   - [ ] **Status:** Not Started.

---

## **Phase 1: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

### **Task 1: Harden Build Source Discovery (Recursive + Deterministic)**
*   **Priority:** High (prerequisite for future shallow subfolders under `src/`).
*   **Goal:** Update build source discovery so all C files under `src/` are discovered recursively with deterministic ordering.
*   **Rationale:** Current discovery only covers up to one subdirectory level and will miss files after module reorganization.
*   **Scope Lock:** Build discovery and related guard/test updates only. No feature behavior changes.
*   **Files to Modify:** `Makefile` (and tests/guards only if required by this change).
*   **Context Files:** `doc/ARCHITECTURE.md`, `doc/AUDIT.md`
*   **Acceptance Criteria:**
*   Build still succeeds with `make clean && make`.
*   Full QA gate still passes with `make qa-all`.
*   Source file list ordering is deterministic across runs.
*   - [ ] **Status:** Not Started.

### **Task 2: Reorganize Modules into Shallow Hierarchical Folders**
*   **Priority:** Medium-High (after Task 1 build-discovery hardening).
*   **Goal:** Group modules into shallow, purpose-based subfolders and update build/header/linkage references accordingly.
*   **Rationale:** Improves discoverability and ownership without changing behavior.
*   **Scope Lock:** File moves + include/path/build/script/test reference updates only. No feature behavior changes.
*   **Files to Modify:** `src/**`, `include/**` (if include paths change), `Makefile`, `scripts/check_module_boundaries.py`, affected tests/docs.
*   **Acceptance Criteria:**
*   `make clean && make` passes.
*   `make qa-module-boundaries` and `make qa-all` pass.
*   No runtime behavior changes.
*   Folder depth remains shallow (max one extra level under `src/ui` and `src/cmd`).
*   - [ ] **Status:** Not Started.

### **Task 3: Decompose Remaining Hotspot Modules (Atomic Subtasks)**
*   **Priority:** High.
*   **Goal:** Reduce complexity in remaining hotspot files by extracting cohesive action families into focused modules while preserving behavior.
*   **Rationale:** These files remain risk hotspots after controller decomposition and slow safe feature delivery.
*   **Execution Rule:** Must be delivered one atomic subtask at a time (3.1 to 3.5), each with its own architect plan, developer pass, auditor pass, and QA evidence.
*   **Context Files:** `include/ytree_ui.h`, `include/ytree_cmd.h`, `doc/ARCHITECTURE.md`
*   - [ ] **Status:** Not Started.

### **Task 3.1: Decompose `src/ui/interactions.c`**
*   **Goal:** Split mixed responsibilities in `interactions.c` into cohesive UI interaction modules (for example compare prompts/builders, archive payload UI, attribute/ownership/date UI, tagged-view flow).
*   **Scope Lock:** No behavior changes in prompts, key paths, or command execution semantics.
*   **Files to Modify:** `src/ui/interactions.c` and new focused `src/ui/*` modules as needed; `include/ytree_ui.h`.
*   **Acceptance Criteria:** Reduced file size/complexity, stable exported API surface, and green QA.
*   - [ ] **Status:** Not Started.

### **Task 3.2: Decompose `src/ui/ctrl_file_ops.c` (`handle_tag_file_action` focus)**
*   **Goal:** Extract large tagged-action branches from `handle_tag_file_action` into focused helpers/modules.
*   **Scope Lock:** Preserve all tagged-file behavior and command semantics.
*   **Files to Modify:** `src/ui/ctrl_file_ops.c`, `src/ui/file_tags.c` (and new focused helpers as needed), `include/ytree_ui.h`.
*   **Acceptance Criteria:** Smaller dispatcher function, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 3.3: Decompose `src/ui/key_engine.c`**
*   **Goal:** Separate key mapping/dispatch concerns from input-loop mechanics and context-specific action routing.
*   **Action Name Cleanup**: Consolidate and rename `YtreeAction` identifiers (e.g., `ACTION_TREE_EXPAND_ALL` -> `ACTION_TREE_EXPAND`) to match behavior (see `BUGS.md`).
*   **Scope Lock:** No keybinding behavior change unless explicitly approved in a separate task.
*   **Files to Modify:** `src/ui/key_engine.c`, `include/ytree_defs.h`, and new focused `src/ui/*` modules as needed.
*   **Acceptance Criteria:** Cleaner dispatch boundaries, consistent action naming, unchanged key behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 3.4: Decompose `src/cmd/copy.c`**
*   **Goal:** Isolate copy conflict handling, path/precondition validation, and transfer orchestration into focused units.
*   **Scope Lock:** No copy/move/archive user-visible behavior changes.
*   **Files to Modify:** `src/cmd/copy.c` and new focused `src/cmd/*` helpers as needed; `include/ytree_cmd.h`.
*   **Acceptance Criteria:** Reduced complexity in core copy path, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 3.5: Decompose `src/cmd/profile.c`**
*   **Goal:** Split profile parsing, validation/defaulting, and apply/update logic into focused units.
*   **Scope Lock:** No configuration semantic changes.
*   **Files to Modify:** `src/cmd/profile.c` and new focused `src/cmd/*` helpers as needed; related headers.
*   **Acceptance Criteria:** Clear parser/apply separation, unchanged config behavior, green QA.
*   - [ ] **Status:** Not Started.

### Task 4: Remove Footer Prompt for / Search
*   Goal: Keep existing / search behavior in all contexts (Dir, File, Showall, Global), but stop using the footer prompt area for search input.
*   Rationale: Current search semantics already work; only the footer prompt is unnecessary UI churn.
*   Requirements:
*   Preserve current semantics: incremental match while typing, Enter confirms jump, Esc cancels.
*   Do not redraw/replace footer help lines during / search.
*   Use a non-footer inline input/render path for search text and match feedback.
*   Files to Modify: src/ui/ctrl_dir.c, src/ui/ctrl_file.c, src/ui/interactions.c, src/ui/display.c
*   Status: Not Started

### **Task 5: Refactor Tab Completion for Command Arguments**
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

### **Task 6: Implement Responsive Adaptive Footer**
*   **Goal:** Make the two-line command footer dynamic based on terminal width.
    *   **Compact (< 80 cols):** Show only critical navigation and file operation keys (Copy, Move, Delete, Quit).
    *   **Standard (80-120 cols):** Show the standard set (current behavior).
    *   **Wide (> 120 cols):** Unfold "hidden" or advanced commands (Hex, Group, Sort modes) directly into the footer so the user doesn't have to remember them.
*   **Rationale:** Maximizes utility on large displays while preventing line-wrapping visual corruption on small screens. It reduces the need to consult F1 on large monitors.
*   **Mechanism:** Define command groups (Priority 1, 2, 3). In `DisplayDirHelp` / `DisplayFileHelp`, construct the string dynamically based on `COLS`.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Task 7: Implement Integrated Help System**
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** `src/ui/key_engine.c`
*   - [ ] **Status:** Not Started.

### **Task 8: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good, but `pathcop(Y)/^Y` is redundant and should just be `pathcop(Y)`).
*   **VI Mode Signaling**: Ensure footer help lines dynamically reflect uppercase commands (e.g., `(K) Vol` instead of `(k) Vol`) when `VI_KEYS=1` is active to avoid navigation collisions.
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   **Files to Modify:** `src/ui/display.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

#### **Task 9: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.
*   - [ ] **Status:** Not Started.

#### **Task 10: Refactor Input Loop for Event Handling**
*   **Task:** Modify `key_engine.c` to support non-blocking input handling.
*   **Logic:**
    *   Currently, `Getch()` blocks indefinitely waiting for a key.
    *   **New Logic:** Use `select()` or `poll()` to wait on **two** file descriptors:
        1.  `STDIN_FILENO` (The keyboard).
        2.  `Watcher_GetFD()` (The inotify handle).
    *   If the Watcher FD triggers, call `Watcher_CheckEvents()`. If it confirms a change, set a global flag `refresh_needed = TRUE`.
    *   If STDIN triggers, proceed to `wgetch()`.
*   - [ ] **Status:** Not Started.

#### **Task 11: Implement Live Refresh Logic**
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

#### **Task 12: Update Watch Context on Navigation**
*   **Task:** Ensure the watcher always monitors the *current* directory.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:** Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytree`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.
*   - [ ] **Status:** Not Started.

#### **Task 13: Implement Directory Filtering (Non-Recursive)**
*   **Description:** Extend the Filter logic to support directory patterns (identified by a trailing slash, e.g., `-obj/` or `build/`). Update the Directory Tree window view to apply these filters. **Note: This logic is non-recursive; it only affects the visibility of directories in the current view, not their internal logged state.**
*   **Files to Modify:** `src/cmd/filter.c`, `src/ui/ctrl_dir.c`
*   **Context Files:** `src/fs/readtree.c`
*   - [ ] **Status:** Not Started.

### **Task 14: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option to globally disable external viewers, forcing the use of the internal viewer.
*   **UI Note:** Expose this in the planned `F10` configuration UI when that panel is implemented.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Files to Modify:** `src/cmd/view.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Task 15: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   **Files to Modify:** `src/ui/key_engine.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Task 16: Standardize Internal Viewer Layout**
*   **Goal:** Ensure the internal viewer's layout geometry matches the main application (borders, headers, and footer).
*   **Files to Modify:** `src/ui/view_internal.c`
*   - [ ] **Status:** Not Started.

#### **Task 17: Implement Archive Move (`M`) Support**
*   **Description:** Implement `M` (Move) for archives. Intra-archive moves use the Rewrite Engine to rename paths. Cross-volume moves use Copy-Extract + Delete.
*   **Files to Modify:** `src/cmd/move.c`, `src/fs/archive_write.c`.
*   - [ ] **Status:** Not Started.

### **Task 18: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   **Files to Modify:** `src/fs/archive_read.c`, `src/cmd/log.c`
*   **Context Files:** `src/fs/archive_read.c`
*   - [ ] **Status:** Not Started.

---

## **Phase 2: Permanent Security and Code-Quality Gates**
*This phase is an enforcement gate: audit the current codebase for existing debt, then detect and block introduced/reintroduced security risks and code smells on every non-trivial change.*

### **Task 2.1: Security Risk Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits the current codebase for security risks and blocks new or reintroduced security findings.
*   **Scope:** shell-command construction and escaping boundaries, archive path trust policy, tempfile lifecycle, and unsafe API usage.
*   **Acceptance Criteria:** Security baseline audit evidence exists, recurring security checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unresolved blocker/high security findings.
*   - [ ] **Status:** Not Started.

#### **Task 2.1.1: Baseline Security Debt Audit and Classification**
*   **Goal:** Run and document a focused baseline audit of current security risk classes already in scope for Phase 0.
*   **Deliverables:** findings inventory with severity, owner, disposition (fix now vs tracked debt), and explicit residual-risk notes.
*   **Files to Modify:** `doc/AUDIT.md` (security evidence section), security-focused test/docs references as needed.
*   - [ ] **Status:** Not Started.

#### **Task 2.1.2: Expand Security Guard Coverage to Block Reintroduction**
*   **Goal:** Ensure banned/legacy security-sensitive APIs and patterns are explicitly rejected by automated guard scripts.
*   **Mechanism:** Extend guard checks for legacy unsafe escaping/runtime paths and other approved denylisted APIs/patterns.
*   **Files to Modify:** `scripts/check_c_unsafe_apis.py`, related guard configs/tests.
*   - [ ] **Status:** Not Started.

#### **Task 2.1.3: Security Regression Gate in CI + Merge Workflow**
*   **Goal:** Make security verification non-optional in routine change flow.
*   **Mechanism:** Require security gate evidence for non-trivial PRs and keep merge blocked until gates pass.
*   **Files to Modify:** `.github/workflows/ci.yml`, `doc/AUDIT.md`, `doc/ai/WORKFLOW.md` (gate evidence requirements only).
*   - [ ] **Status:** Not Started.

### **Task 2.2: Code-Smell Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits current code smells and blocks new/reintroduced structural smell debt.
*   **Scope:** controller growth, god-function budgets, module-boundary violations, complexity hotspots, and architecture drift.
*   **Acceptance Criteria:** Smell baseline audit evidence exists, recurring smell checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unapproved new smell violations.
*   - [ ] **Status:** Not Started.

#### **Task 2.2.1: Baseline Code-Smell Audit and Debt Register**
*   **Goal:** Audit current codebase for structural smells and categorize debt with explicit remediation sequencing.
*   **Deliverables:** baseline report covering hotspots, oversized controllers/functions, boundary exceptions, and tracked rationale for retained debt.
*   **Files to Modify:** `doc/AUDIT.md` (smell evidence section), roadmap/task references as needed.
*   - [ ] **Status:** Not Started.

#### **Task 2.2.2: Strengthen Smell-Prevention Guards**
*   **Goal:** Prevent reintroduction of known smell patterns via automated policy checks.
*   **Mechanism:** Tighten module-boundary/controller-growth policies and require explicit approval paths for exceptions.
*   **Files to Modify:** `scripts/check_module_boundaries.py`, guard policy/config docs.
*   - [ ] **Status:** Not Started.

#### **Task 2.2.3: Smell Gate Evidence as Merge Prerequisite**
*   **Goal:** Ensure smell-audit results are part of mandatory merge evidence, not optional review notes.
*   **Mechanism:** Require successful smell checks in QA artifacts and block integration on unresolved unapproved violations.
*   **Files to Modify:** `doc/AUDIT.md`, `doc/ai/WORKFLOW.md`, related QA docs.
*   - [ ] **Status:** Not Started.

---

## **Phase 3: Advanced Features**
*This phase builds upon the new architecture to add more high-level functionality.*

### **Task 19: Implement Advanced Batch Rename**
*   **Goal:** Enhance the `Rename` command to support advanced masks (e.g., `*_<001>.bak`), sequential numbering, casing changes (`Tab`), and substring replacement.
*   **Rationale:** Essential power-user feature for managing large file sets.
*   **Files to Modify:** `src/cmd/rename.c`, `src/ui/key_engine.c`
*   - [ ] **Status:** Not Started.

### **Task 20: Enhance PathCopy to Mirror (Sync)**
*   **Goal:** Enhance the existing `PathCopy` (`^Y`) logic to support "Mirroring". Add options to **synchronize** the destination (delete orphan files not present in source) and compare size and timestamps.
*   **Rationale:** `PathCopy` currently acts as a "Copy", but lacks the "Sync" capabilities of ZTree's `Alt-Mirror`.
*   **Files to Modify:** `src/cmd/copy.c`, `src/cmd/mirror.c` (New)
*   - [ ] **Status:** Not Started.

---

## **Phase 4: Internationalization and Configurability**
*   **Goal:** Refactor the application to support localization and user-defined keybindings, moving away from hardcoded English-centric values.

### **Task 21: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from `~/.ytree`. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Files to Modify:** `src/ui/key_engine.c`, `src/cmd/profile.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

### **Task 22: Standardize Localization Strategy** (Use the Code Auditor persona here)
*   **Goal:** Audit the codebase for hardcoded German strings or personal author remarks and convert them to English. Ensure all UI text is wrapped in macros suitable for future `gettext` translation.
*   **Rationale:** Prepares the project for a global audience and ensures consistency in the default locale.
*   **Files to Modify:** `src/*/*.c`
*   **Context Files:** `include/ytree.h`
*   - [ ] **Status:** Not Started.

---

## **Phase 5: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Task 23: Restructure and Expand Test Suite**
*   **Goal:** Tidy up existing test scripts into a coherent, modular structure and thoroughly expand the regression suite for comprehensive coverage.
*   **Rationale:** A well-structured test suite is easier to maintain and extend. Thorough, systematic coverage ensures reliability and prevents regressions across complex file operations.
*   - [ ] **Status:** Not Started.

### **Task 24: Finalize Documentation**
*   **Goal:** Update the `CHANGELOG`, `README.md`, and `CONTRIBUTING.md` files to reflect all new features and changes before a release.
*   **Rationale:** Ensures users and developers have accurate, up-to-date information about the project.
*   **Files to Modify:** `README.md`, `doc/CHANGES.md`, `doc/CONTRIBUTING.md`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Task 25: Initialize Distributed Issue Tracking (git-bug)**
*   **Goal:** Configure `git-bug` to act as a bridge between the local repository and GitHub Issues. Migrate the contents of `BUGS.md` and `TODO.txt` into this system prior to public release.
*   **Rationale:** Allows the developer to maintain a simple local text-based workflow during heavy development, while ensuring that all tracking data can be synchronized to the public web interface when the project goes live.
*   **Files to Modify:** `doc/BUGS.md`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

---

## **Future Enhancements / Wishlist**
*A collection of high-complexity or lower-priority features to be considered after the primary roadmap is complete.*

### **Task 1: Externalize UI Strings for Internationalization (i18n)**
*   **Description:** Move all user-facing strings (menus, prompts, error messages) out of the C code and into a separate, loadable resource file. This is a prerequisite for translation. The application will load the appropriate language file at startup.
*   **Rationale:** As discussed, this is a major architectural shift that risks breaking existing test harnesses. It should be deferred until the core UI logic and path completion refinements are stable.
*   **Files to Modify:** `src/util/i18n.c` (New), `src/*/*.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

### **Phase 1: UI/UX Enhancements and Cleanup**

### **Task 12: Implement In-App Configuration Editor (F10)**
*   **Goal:** Implement a settings panel (activated by `F10`) that allows the user to view and change configuration options from `~/.ytree` (e.g., `CONFIRMQUIT`, colors) and save the changes.
*   **Rationale:** Provides a user-friendly way to configure `ytree` without manually editing the configuration file, improving accessibility.
*   **Files to Modify:** `src/cmd/profile.c`, `src/ui/key_engine.c`
*   **Context Files:** `include/config.h`
*   - [ ] **Status:** Not Started.

### **Task 13: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** A key feature of classic file managers like ZTreeWin and modern ones like Midnight Commander, mouse support dramatically improves speed and ease of use for users in capable terminal environments.
*   **Files to Modify:** `src/ui/key_engine.c`
*   **Context Files:** `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

*   **Task 14: Shell-Style Tab Completion:** Replace the current history-based tab completion with true filename/directory completion in the input line.
*   **Task 15: Implement Advanced, ncurses-native Command Line Editing:** Full cursor navigation (left/right, home/end, word-by-word). In-line text editing (insert, delete, backspace, clear line). Persistent command history accessible via up/down arrows. Maybe even context-aware tab completion for files and directories.
*   **Task 16: Per-Window Filter State (Split Screen Prerequisite):** Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left pane versus `*.h` in the right).
*   **Task 17: State Preservation on Reload (^L):** Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   **Task 18: Preserve Tree Expansion on Refresh:** Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.

### **Task 19: Step 4.m: Applications menu (F9)**
*   Implement a customizable Application Menu (low-priority; add only with a qualified use-case).
*   **Files to Modify:** `src/cmd/usermode.c`, `src/cmd/profile.c`
*   **Context Files:** None.
*   - [ ] **Status:** Not Started.

*   **Task 20: Scroll bars:**  On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list, (configurable to char or line).
*   **Task 23: Callback API Constification Cleanup (cppcheck strict mode):** `cppcheck` suggests const-qualifying callback `user_data`, but doing this correctly likely requires changing callback typedef/API signatures (e.g., `RewriteCallback`) and related call sites. Defer this to a focused API pass to avoid scattered casts and partial churn.


### **Phase 2: Advanced Features**

### **Task 2: Implement VFS Abstraction Layer** (Use the Architect persona here)
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytree` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"

#### **Task 3: Define VFS Interface & Volume Integration** (Use the Architect persona here)
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytree_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytree_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.
*   **Files:** `include/ytree_vfs.h`, `include/ytree_defs.h`.

#### **Task 4: Implement VFS Registry** (Use the Architect persona here)
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.
*   **Files:** `src/fs/vfs.c`, `include/ytree_vfs.h`.

#### **Task 5: Implement "Local" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/readtree.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.
*   **Files:** `src/fs/drv_local.c`, `src/fs/readtree.c` (cleanup).

#### **Task 6: Implement "Archive" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/readarchive.c` and `src/fs/archive.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.
*   **Files:** `src/fs/drv_archive.c`, `src/fs/readarchive.c` (delete).

#### **Task 7: Switch `LogDisk` to VFS** (Use the Architect persona here)
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.
*   **Files:** `src/cmd/log.c`.

#### **Task 8: Refactor Consumers (Polymorphism)** (Use the Architect persona here)
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.
*   **Files:** `src/cmd/*.c`.

### **Task 9: Implement Recursive Directory Watching** (Use the Code Auditor persona here)
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

### **Task 10: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   **Files to Modify:** `src/cmd/batch.c` (New)
*   **Context Files:** `src/ui/ctrl_file.c`
*   - [ ] **Status:** Not Started.

### **Task 11: Implement Keyboard Macros (F12)**
*   **Goal:** Implement a macro recording and playback system. `F12` starts/stops recording keystrokes to a buffer/file.
*   **Rationale:** Allows automation of repetitive tasks (e.g., "Tag, Move, Rename, Repeat").
*   **Files to Modify:** `src/ui/key_engine.c`, `src/core/macro.c` (New)
*   - [ ] **Status:** Not Started.

*   **Task 21: Implement "Safe Delete" (Trash Can):** Support the FreeDesktop.org Trash specification to allow file recovery.
*   **Task 22: Print Functionality:** Investigate and potentially implement a "Print" command that is distinct from "Pipe," if a clear use case for modern systems can be defined.

### **Phase 3: Internationalization and Configurability**

*   **Task 24: Terminal-Independent TUI Mode:** Investigate a mode to run the `ytree` TUI directly on a **Unix virtual console** (e.g., via `/dev/tty*` or framebuffer), independent of a terminal emulator session.
