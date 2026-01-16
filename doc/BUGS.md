# BUGS

## Hard/Intractable Bugs

Use this section to document bugs that have proven difficult to fix. Recording attempted fixes and theories here saves time during future debugging sessions.

### Archive Tagged Copy/PathCopy Failure

*   **Description:**
    When performing bulk copy operations from an Archive volume (e.g., ISO, ZIP) to a filesystem, the operation fails to process the complete list of tagged files.
    *   **Copy Tagged (`^K`):** Copies only the first file, then stops.
    *   **PathCopy Tagged (`^Y`):** Creates the destination directory structure for the first entry but copies zero files.
    *   *Note:* These operations work perfectly when copying from a standard filesystem.

*   **Steps to Reproduce:**
    1.  Start ytree and log an archive file (e.g., `ytree test.iso`).
    2.  Enter a subdirectory within the archive.
    3.  Tag multiple files (e.g., 3 files).
    4.  Press `^K` (Copy Tagged) and specify a destination directory.
    5.  **Result:** Only one file appears in destination.
    6.  Repeat with `^Y` (PathCopy Tagged).
    7.  **Result:** Directory is created, but it is empty.

*   **Current Theory:**
    The issue likely lies in the interaction between the `WalkTaggedFiles` iterator loop and the `libarchive` state. If the extraction function (`ExtractArchiveNode` in `archive.c`) does not fully reset or close the archive handle between iterations, or if it returns a specific status code that `WalkTaggedFiles` interprets as a "stop" signal, the loop terminates prematurely.

*   **Attempted Fixes (Failed):**
    *   Extensive man-hours spent debugging the loop logic in `filewin.c`.
    *   Attempts to force reset the archive read state between extractions.

*   **Future Resolution:**
    This bug is expected to be resolved (or the code path completely replaced) during **Roadmap Step 4.30 (Archive Write Support)** or **Step 5.3 (Enhanced Archive Format Detection)**, which will likely require a rewrite of the archive virtual filesystem layer.

*   **Relevant Code:**
    *   `src/copy.c`
    *   `src/archive.c` (`ExtractArchiveNode`)
    *   `src/filewin.c` (`WalkTaggedFiles`)

---

### F2 Destination Selection Context Loss & UI Corruption

*   **Description:**
    When using the F2 key to select a destination directory during file operations (Copy, Move, PathCopy), returning to the main interface results in significant UI corruption and loss of navigational context. This issue affects both standard Filesystem mode and Archive mode.

    Upon returning from the F2 window, the following anomalies occur:
    1.  **Context Reset:** The Directory Tree view resets to the volume root (or position 0,0) instead of restoring the directory the user was previously viewing.
    2.  **Prompt Erasure:** The input prompt text (e.g., "TO: ...") is erased by the screen refresh, leaving a floating cursor with no label.
    3.  **Footer Corruption:** The main button bar/footer disappears or displays incorrect context (e.g., displaying artifacts from the F2 window or nothing at all).
    4.  **Focus Mismatch:** Visually, the focus appears to return to the Directory Window (with the root highlighted), but the application logically remains inside the input loop. Navigation keys (Down Arrow) may move the cursor in the small file window or do nothing, rather than navigating the tree.

*   **Steps to Reproduce:**
    1.  Navigate to any directory containing files (either on a Disk or inside an Archive).
    2.  Enter the File Window (`Enter`).
    3.  Initiate a file operation (e.g., press `C` for Copy).
    4.  At the "TO:" input prompt, press `F2` to browse for a destination.
    5.  Inside the F2 window, log a new path (`L`) or cycle volumes (`<` / `>`).
    6.  Select a target directory and press `Enter` to confirm.
    7.  **Result:** You are returned to the main screen, but the "TO:" text is gone, the directory tree has collapsed/reset to the root, and the footer menu is missing.

*   **Current Theory:**
    The issue stems from a disconnect between the modal `KeyF2Get` function and the blocking `InputString` loop.
    1.  `KeyF2Get` performs a full UI redraw (`DisplayMenu`) to clean up the F2 window, which inadvertently wipes the prompt text drawn by `InputString`. `InputString` is unaware of this erasure and does not repaint its label.
    2.  The `CurrentVolume->vol_stats` structure, used to redraw the tree upon return, is likely desynchronized from the `ActivePanel` state. If `KeyF2Get` restores the volume pointer but fails to reload the specific cursor coordinates from the active panel, the display logic defaults to the root position.

*   **Relevant Code:**
    *   `src/dirwin.c` (`KeyF2Get`)
    *   `src/input.c` (`InputStringEx`)
    *   `src/display.c` (`DisplayMenu`)

---

## Known Issues

*   **Archive "Collapse":** Directories inside archives cannot be collapsed (`-`) because they are virtual; collapsing currently requires a destructive re-scan which is inefficient for archives. (Addressed in Roadmap Step 5.4).
*   **Archive Login Layout Glitch:** Occasionally, logging an archive results in the "Small File Window" rendering incorrectly within or overlapping the Directory Window area. (Intermittent/Not reliably reproducible).
*   **Keybinding Mismatch (`*`):** The `*` key is intended to trigger `ACTION_TREE_EXPAND` (expand the current directory level). However, behavior is inconsistent, sometimes triggering file tagging instead, particularly when focus is ambiguous.