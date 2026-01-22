# BUGS

## Hard/Intractable Bugs

Use this section to document bugs that have proven difficult to fix. Recording attempted fixes and theories here saves time during future debugging sessions.

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