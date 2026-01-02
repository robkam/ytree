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

## Known Issues

*   **Archive "Collapse":** Directories inside archives cannot be collapsed (`-`) because they are virtual; collapsing currently requires a destructive re-scan which is inefficient for archives. (Addressed in Roadmap Step 5.4).
*   **Archive Login Layout Glitch:** Occasionally, logging an archive results in the "Small File Window" rendering incorrectly within or overlapping the Directory Window area. (Intermittent/Not reliably reproducible).

