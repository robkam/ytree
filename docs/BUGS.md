# **Bugs and Defects Requiring Fixes**

This file tracks fix-required bugs, architectural violations, and naming inconsistencies that require remediation.
Buglist is forward-looking (`planned`/`in-progress`). Completed items will be removed after fixed.

Ordering policy (for all editors, including AI editors):
- Put bugs that are high-impact first after that order remaining bugs by ease of implementation.
- Insert new approved bugs at the correct priority position (do not append by default).
- Bug numbering is top-to-bottom ascending (`BUG-1` = highest priority).
- Bug IDs are unstable labels and are likely to change often due to reprioritization/renumbering.

## **Current Runtime Defects (Highest Priority First)**

### **BUG-1: Cycle-Volumes (`<`/`>`, `,`/`.`) Leaks Dir/File View State Across Volumes**
*   **Description**: Cycling logged volumes can cause dir/file window mode/state changes in one volume to appear in the other, instead of each volume retaining its own last-used state.
*   **Impact**: Breaks per-volume navigation predictability and increases wrong-target risk during fast volume switching workflows.
*   **Remediation**: Preserve and restore per-volume dir/file window state independently when cycling volumes. Add regression coverage for repeated `<`/`>` transitions across volumes with different view states.
*   **Regression notes (manual, 2026-05-21)**:
    *   `F10` can fail with `Can't edit "/home/rob/.ytnova"` after volume-cycle transitions, then succeeds again after cycling back.
    *   With `SMALLWINDOWSKIP=0`, cycling away and back from a zoomed file window can return the same location in small-window non-zoom state.
*   **Status**: Fixed.

### **BUG-2: Split-Panel State Isolation and Restore Authority Family**
*   **Description**: BUG-2 is the root split-panel family. BUG-2.1 through BUG-2.5 are all visible effects of the same underlying F8 split-state architecture problem.
*   **Family contract**: each panel must keep its own state record; restore must use stable identity and deterministic fallback; redraw must never become authority; split transitions must not import or guess state from the other panel.
*   **Related**: `ROADMAP` Task 30 (split selection semantics/regression coverage).
*   **Status**: Fixed.

### **BUG-2.1: Split `Tab` Transition Can Trigger Obvious Wrong-Surface Refresh**
*   **Description**: During `F8` split copy-destination preparation, `Tab` can trigger a visibly incorrect/ugly window refresh where redraw surface ownership appears unstable even though source selection/tag state remains intact.
*   **Repro (manual, 2026-05-22)**:
    *   Source flow: enter `/home/rob/ytreenova/src/cmd`, tag first files, start `c`, `Enter` into destination prompt, then cancel/return.
    *   Split flow: `F8`, `Tab`, `Enter` to leave file window, cycle destination tree, `Home`, `M 00`, select `00`.
    *   Press `Tab` back.
*   **Expected**: Stable panel redraw across `Tab` transitions with no obvious wrong-surface flash/churn.
*   **Actual**: Very obvious ugly refresh happens at `Tab` transition.
*   **Notes**:
    *   In this flow, source selection/tag identity stayed unchanged (render/refresh defect, not BUG-2 selection-loss itself).
*   **Impact**: Degrades trust and usability in high-frequency split copy/move workflows.
*   **Remediation**: Audit split redraw ownership/order around `Tab` transition and destination-prep mode switches; enforce deterministic repaint sequencing for active/inactive surfaces.
*   **Status**: Fixed.

### **BUG-2.2: Volume Switch Can Lose Per-Volume File Context (`SMALLWINDOWSKIP=1`)**
*   **Description**: With `SMALLWINDOWSKIP=1`, switching between logged volumes can lose previously selected deep file context and return to parent tree location instead.
*   **Repro (manual, 2026-05-22)**:
    *   `yt ~`
    *   `log /home/rob/xtreefanpage`
    *   Enter `/home/rob/xtreefanpage/download`, select `noans.zip`
    *   `log /home/rob/ytreenova`
    *   Enter `/home/rob/ytreenova/src/cmd`, select `rename.c`
    *   On `~` select end dir, `k release ~`
    *   Switch to `/home/rob/ytreenova`
*   **Expected**: Return to prior file context (`/home/rob/ytreenova/src/cmd`, selected `rename.c`) for that volume.
*   **Actual**: Selection returns in tree context with `/home/rob/ytreenova/src/cmd` moved to a different directory-row position (reported as second-from-bottom), not preserved file-context state.
*   **Additional observation**:
    *   After moving `/home/rob/ytreenova/src/cmd` to `/home/rob/ytreenova` and switching back to `/home/rob/xtreefanpage`, view no longer returns to `/home/rob/xtreefanpage/download`; lands at `/home/rob/xtreefanpage` dir view.
    *   `SMALLWINDOWSKIP=0` not yet tried for this defect family.
*   **Impact**: Breaks per-volume resume guarantees and creates wrong-target risk during cross-volume workflows.
*   **Remediation**:
    *   Persist a per-volume file-context anchor by stable identity (`dir path` + `selected file path/name` + `focus/view mode`), not by transient row position.
    *   Do not persist raw `DirEntry*`/`FileEntry*` pointers across rebuild/rescan/volume-switch boundaries; re-resolve from stable keys after list rebuild.
    *   Restore with deterministic fallback order: exact file match -> same directory anchor -> first visible entry.
    *   Guard post-switch dereferences when lists are empty/missing (`vol`, `dir_entry_list`, `total_dirs`) so restore paths fail closed instead of reading invalid state.
*   **Status**: Fixed.

### **BUG-2.3: Tree Viewport Reanchors Unexpectedly During Navigation and Panel Reactivation**
*   **Description**: Tree viewport origin must remain panel-local and stable whenever the active selection is still valid and visible. In practice, `Enter` transitions, `Tab` reactivation, and hidden-dotfile restore paths can still reanchor the tree and make it shift upward or downward even though no scroll is required.
*   **Repro (manual, 2026-05-29 / 2026-05-31)**:
    *   `yt ~/ytreenova`
    *   Move selection to `src/cmd`
    *   Press `Enter`, then `Enter` again
    *   `HIDEDOTFILES=0 yt ~/ytreenova /mnt/c/Users/Henry/Desktop/`
    *   Move to `/home/rob/ytreenova/src/cmd`
    *   `F8`, `Tab`, `/`, select `/mnt/c/Users/Henry/Desktop/Wiki & electronics/Music studies`, then `Tab`
*   **Expected**: The tree viewport remains stable unless scroll is required to keep the active selection visible (for example when selection moves beyond the visible viewport). Reactivating a panel must restore the same frozen tree state the panel had before losing focus, regardless of hidden-dotfile settings.
*   **Actual**: Tree content shifts unexpectedly while selection/context remains within what should be a stable viewport, and the reactivated panel can jump to a different origin when `HIDEDOTFILES=0`.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §3.1 **Navigation Stability**
    *   `docs/SPECIFICATION.md` §3.4 **Tree Up/Down Edge-Scroll Rule**
    *   `docs/SPECIFICATION.md` §3.4 **Tree Home/End Visibility Rule**
    *   `docs/SPECIFICATION.md` §5.2 **Window/Mode Context** persistence
    *   `docs/SPECIFICATION.md` §5.1 **Freeze/Resume Rule**
    *   `docs/SPECIFICATION.md` §5.3 **Render Is Not Authority Rule**
    *   `docs/SPECIFICATION.md` §5.3 **Hidden-Prefix Selection Accounting Rule**
*   **Impact**: Breaks navigation predictability and panel-reactivation trust by causing unexpected visual movement during routine navigation and split-state restore flows.
*   **Remediation**:
    *   Enforce explicit viewport-anchor rules for tree rendering during all panel restore and mode-transition paths.
    *   Disallow non-required viewport origin changes when selection remains visible/valid.
    *   Use deterministic viewport adjustment only for visibility preservation, not as a side-effect of redraw/rebind paths.
*   **Status**: Fixed.

### **BUG-2.4: Mkdir Triggers Unnecessary Relog and Resets Tree State**
*   **Description**: Creating a directory with `M` can force a relog-style tree rebuild that reanchors selection and resets the visible tree state, even though the parent tree is already valid and only an incremental redraw/update is needed.
*   **Repro (manual, 2026-05-30)**:
    *   `yt ~/ytreenova /mnt/c/Users/Henry/Desktop/`
    *   Navigate to a tree location with a logged parent and stable viewport
    *   Press `M 00`
*   **Expected**: Directory creation updates the current tree in place, preserving the existing expansion/logging state and viewport origin when the current selection remains valid; no implicit relog or depth reset.
*   **Actual**: The tree is relogged/reset one level and prior tree state is lost.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §3.3 **Directory Memory Commands (Structural Controls)**
    *   `docs/SPECIFICATION.md` §5.2 **State Persistence**
*   **Impact**: Breaks tree-state predictability and can silently discard user navigation context during ordinary directory creation.
*   **Remediation**: Treat mkdir as an incremental tree mutation and restrict any repair to minimal bounds/selection clamping only when the current cursor or viewport offset would otherwise become invalid.
*   **Status**: Confirmed.

### **BUG-2.5: Split-Panel Filter State Leaks Across Panels on Volume Cycle**
*   **Description**: In split mode, a filespec filter set in one panel can appear in the other panel after that panel cycles to the same logged volume, instead of preserving panel-local filter state.
*   **Repro (manual, 2026-05-22)**:
    *   In left panel on volume 1, set a filter.
    *   `Tab` to right panel on volume 2 (no filter change observed there).
    *   In right panel, cycle to volume 1.
*   **Expected**: Right panel restores its own saved filter state for that panel/volume context (panel-local isolation), not the left panel’s filter.
*   **Actual**: Right panel on volume 1 shows the left panel’s filter without being set in that panel.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §5.1 **Active-Only Mutation Rule**
    *   `docs/SPECIFICATION.md` §5.2 **Filter (Filespec): Independent search/filter strings**
*   **Impact**: Cross-panel state leakage can silently narrow file lists and increase wrong-target risk.
*   **Remediation**:
    *   Make filter ownership panel-local for split state (panel + volume context), not volume-global.
    *   On `Tab` and volume-cycle transitions, restore filter from the target panel snapshot only; do not import from the opposite panel.
    *   Remove shared-buffer/aliasing paths that let both panels read/write one filter instance.
    *   Enforce active-only mutation and inactive freeze semantics for filter state.
*   **Status**: Confirmed.

### **BUG-2.6: Hidden UI Entries Still Influence Visible-Tree State**
*   **Description**: Entries that are hidden from the UI are not being fully excluded from navigation/state resolution. Hidden-dotfile and hidden-prefix paths can still influence visible-tree behavior, so the app can resolve or restore against a hidden ancestor or sibling path instead of treating the visible tree as authoritative.
*   **Repro (manual, 2026-06-04)**:
    *   Open a tree that contains both a visible target such as `src` and hidden-prefix content such as `./tmp/session.cast` or `~/.local/src`.
    *   Perform a normal jump/search flow that should land on the visible target.
*   **Expected**: Items hidden from the UI should not participate in ordinary visible-tree jumps, selection, or restore decisions.
*   **Actual**: The jump resolves to a hidden-prefix path such as `/home/rob/.local/src` instead of the visible `~/ytreenova/src`.
*   **Impact**: Demonstrates a broader architectural defect in hidden-item accounting and visible-tree authority, not just a one-off wrong row. It can misdirect routine navigation and make hidden entries behave as if they were still visible.
*   **Remediation**: Make hidden-from-UI entries non-participants in the normal visible-tree resolver paths unless explicitly requested, and keep the visible-tree contract authoritative for jump, selection, and restore logic. Add regression coverage that distinguishes visible-tree targets from hidden-prefix matches.
*   **Status**: Confirmed.

### **BUG-3: F8 Dotfiles Toggle Leaks Across Panels**
*   **Description**: In `F8` split mode, toggling dotfiles visibility (`` ` `` do/undo) in the active panel can apply the same visibility change to the inactive panel.
*   **Repro (manual)**:
    *   Enter split mode with `F8`.
    *   Keep the inactive panel parked on a different directory/view context.
    *   Toggle dotfiles in the active panel with `` ` ``.
*   **Expected**: Dotfiles visibility is panel-local in split mode; only the active panel changes.
*   **Actual**: Inactive panel visibility changes too.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §5.1 **Active-Only Mutation Rule**
    *   `docs/SPECIFICATION.md` §5.2 **Freeze/Resume Rule** (inactive panel must resume unchanged)
*   **Impact**: Breaks panel isolation and can silently alter the source/destination working view.
*   **Remediation**: Move dotfile-visibility ownership to panel-local split state and keep shared-tree updates limited to topology-only mirroring. Add split regression coverage for active-side dotfile toggles with inactive-panel state snapshots.
*   **Tests/Gates**: Must have deterministic panel-isolation regression coverage under `tests/test_panel_isolation.py`; gate via `pytest` split-isolation subset and PR full-QA CI.
*   **Regression notes (manual, 2026-05-21)**:
    *   In split mode, active-panel dotfile toggles still influence inactive-panel behavior after `Tab` transitions.
    *   Toggling in active file window can still perturb inactive tree presentation.
*   **Status**: Fixed.

### **BUG-4: F8 Dotfiles Toggle Causes Inactive Selection Jitter**
*   **Description**: In split mode, toggling dotfiles in one panel can make the inactive panel’s selected directory move away and then return (transient cursor/selection drift).
*   **Repro (manual)**:
    *   Enter split mode with `F8`.
    *   Park inactive panel on a stable directory selection.
    *   Toggle dotfiles in active panel (do/undo).
*   **Expected**: Inactive selection stays on the same directory identity when still visible/valid.
*   **Actual**: Inactive panel selection briefly moves and then snaps back.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §5.3 **Selection Retention Rule**
    *   `docs/SPECIFICATION.md` §5.3 **Non-Invalidating Changes Rule**
    *   `docs/SPECIFICATION.md` §5.3 **Render Is Not Authority Rule**
*   **Impact**: Indicates unstable cross-panel state restoration and increases risk of wrong-target operations after state churn.
*   **Remediation**: Re-anchor inactive selection strictly by stable directory identity/path during mirror updates; never re-resolve by transient index unless deterministic fallback is required.
*   **Tests/Gates**: Add deterministic regression asserting no inactive selection movement on non-invalidating dotfile toggles.
*   **Status**: Fixed.

### **BUG-5: F8 + SMALLWINDOWSKIP=0 Tab Can Force Inactive Panel into Wrong Focus**
*   **Description**: With `SMALLWINDOWSKIP=0`, when cursor is in the small file window on one panel, `Tab` to the other panel can show that inactive panel as file-focused/zoomed unexpectedly; tabbing back restores prior tree/small state.
*   **Extension (manual, 2026-05-23)**: With `SMALLWINDOWSKIP=1`, splitting from file view (`Enter` then `F8`) could leave the inactive panel shown as tree/small until `Tab`, instead of immediately preserving file-view shape.
*   **Repro (manual)**:
    *   `F10` config, set `SMALLWINDOWSKIP=0`.
    *   Enter split mode (`F8`), enter small file window in one panel.
    *   Press `Tab` to switch active panel.
*   **Expected**: Inactive panel remains in its frozen tree/small-window focus state; only active panel focus changes.
*   **Actual**: Inactive panel can temporarily present the wrong file-focus/zoom state, then reverts on round-trip.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §5.1 **Active-Only Mutation Rule**
    *   `docs/SPECIFICATION.md` §5.2 **Window/Mode Context** persistence
    *   `docs/SPECIFICATION.md` §5.2 **Freeze/Resume Rule**
*   **Impact**: Breaks trust in split focus separation and can trigger wrong-context commands.
*   **Remediation**: Harden switch-time state transfer so focus/view state is restored from panel-owned snapshots only; forbid cross-panel focus inheritance during `Tab` unless explicitly commanded by the active panel.
*   **Tests/Gates**:
    *   `tests/test_panel_isolation.py::test_split_tab_from_small_file_does_not_expand_inactive_panel`
    *   `tests/test_panel_isolation.py::test_split_from_big_file_keeps_inactive_panel_in_file_view`
*   **Regression notes (manual)**:
    *   2026-05-21: With `SMALLWINDOWSKIP=0`, `Tab` could show the inactive panel in zoomed file-window focus until tabbing back.
    *   2026-05-23: With `SMALLWINDOWSKIP=1`, split from file view could keep inactive panel in tree/small until `Tab` flipped it to file view.
*   **Status**: Fixed.

### **BUG-6: Dir Display Mode Resets After Context Switch (`^F`/Future `1..4`)**
*   **Description**: Setting a directory display mode while focused in dir/tree context can be lost after entering file/small-window context and returning to dir/tree context.
*   **Repro (manual)**:
    *   In dir/tree focus, press `^F` to cycle to a non-default dir display mode (for example Owner/Times).
    *   Enter file/small-window focus.
    *   Return to dir/tree focus.
*   **Expected**: Dir display mode remains as last set in dir context until explicitly changed.
*   **Actual**: Dir display mode reverts/reset unexpectedly.
*   **Impact**: Breaks context-local view-state persistence and makes display-mode controls feel unreliable.
*   **Remediation**: Persist dir-context display mode independently across focus/context transitions, and preserve it when file-context display mode changes.
*   **Related**: `ROADMAP` Task 44 (`1..0 FileInfo` ownership contract).
*   **Status**: Confirmed.

### **BUG-7: F7 Preview Over-Restricts Command Availability**
*   **Description**: `F7` mode is currently incomplete for inspect-and-act workflows. Too many common file actions are disabled, so users must leave preview to continue work.
*   **Expected Behavior**:
    *   `F7` should allow a practical command subset for in-context file work (for example attributes/copy/delete/edit/filter/compare/move/new-date/open/print/rename/tag/untag/view/execute/quit paths as applicable).
    *   Tagged/search workflow should operate in `F7`: `^T` (tag-all), `^S` (search), then `^V` (view tagged/search results) without leaving preview.
    *   In `F7` preview, tagged search hits/results should be visibly highlighted.
    *   `F8` and `Tab` should remain disabled in `F7` preview mode so split/layout switching cannot mutate preview state unexpectedly.
*   **Impact**: Makes `F7` feel unfinished and adds avoidable friction in routine review workflows.
*   **Remediation**: Finish `F7` as an in-place work mode: allow core actions (tag/search/view results/compare/copy/move/rename) without leaving preview, keep `F8`/`Tab` blocked for state safety, and add regression coverage for allowed actions and blocked keys.
*   **Related**: Existing regression intent for `F8`-in-`F7` state safety should remain preserved and extended to `Tab`.
*   **Status**: Confirmed.

### **BUG-8: `Write` Offers/Describes Actions That Are Not Context-Valid**
*   **Description**: `Write` format/options/prompt/help are not consistently aligned with context (`dir`/`file`/`archive`/`tagged`) and can imply workflows that are unavailable or misleading in the active mode.
*   **Impact**: Reduces discoverability and trust, and creates avoidable trial-and-error in critical output/export flows.
*   **Remediation**: Define and enforce a context-valid option matrix for `Write`, expose only valid options in each mode, and keep prompt/help text explicit and non-jargon (including destination examples such as file output and printer-command output). Keep `SPECIFICATION`, `F1` help, and manpage/USAGE text synchronized with the same destination semantics.
*   **Status**: Confirmed.

### **BUG-9: Footer/Help/Prompt Trust Family**
*   **Description**: BUG-9 is the root footer/help/prompt trust family. BUG-9.1 through BUG-9.4 are visible effects of the same underlying discoverability problem.
*   **Family contract**: footer, F1 help, and prompt text must report the same available actions and context; help surfaces must not imply unavailable actions; cancel/exit paths must restore the normal context footer; archive-specific messages must report the actual attempted shortcut; archive tree rendering must stay structurally honest.
*   **Related**: `ROADMAP` Task 43 (Refine Contextual F1 Content and Footer-Parity Contract) and Task 43.1 (Add Contextual F1 Hyperlinks and Shared Explainer Pages).
*   **Status**: Fixed.

### **BUG-9.1: Copy/Move Cancel (`Esc`) Can Leave Footer Blank**
*   **Description**: In `Copy`/`Move` flows, canceling with `Esc` can leave footer/help lines blank instead of restoring the normal context footer.
*   **Findings**:
    *   Deterministic repro under sanitizer gate (`make qa-sanitize`) on 2026-05-16: `tests/test_display_layout.py::test_dir_copy_to_missing_destination_prompts_create_and_no_restores_footer` fails with `AssertionError: Header/path row disappeared after canceling create prompt`.
    *   The failing path is directory copy to a missing destination where the create-directory prompt is canceled with `No`.
*   **Impact**: Hides command discoverability immediately after a canceled mutation flow and makes the UI look partially broken.
*   **Remediation**: On all `Copy`/`Move` cancel/exit paths (`Esc` and equivalent cancel keys), restore footer/help ownership deterministically to the active view context and force a full footer redraw before accepting the next command.
*   **Related**: `BUG-21` (footer restore consistency during input flows), `ROADMAP` Task 43 (Refine Contextual F1 Content and Footer-Parity Contract).
*   **Status**: Confirmed.

### **BUG-9.2: Prompt Footer/F1 Parity Can Hide Available Prompt Actions**
*   **Description**: In prompt-driven workflows, footer/F1 coverage can omit active prompt actions and semantics (for example completion/browse controls and compare/archive prompt meanings), leaving available behavior under-discoverable.
*   **Impact**: Creates hidden-feature workflow confusion and high-friction issue reports during routine operations.
*   **Remediation**: Enforce a prompt-context parity contract: footer shows currently available prompt actions; F1 may add concise semantics/examples for those same actions, but must not advertise unavailable actions.
*   **Related**: `ROADMAP` Task 43 (Refine Contextual F1 Content and Footer-Parity Contract), `BUG-9.3` (archive unavailable-action messaging), `BUG-8` (prompt/help context mismatch).
*   **Status**: Confirmed.

### **BUG-9.3: Archive Unavailable-Action Message Reports Wrong Shortcut**
*   **Description**: In archive mode, triggering `^W` can show an error message for a different shortcut (`^P is not available in archive mode`).
*   **Impact**: Misleading feedback increases operator confusion and undermines trust in key/action hints.
*   **Remediation**: Ensure unavailable-action messaging reports the actual attempted action/shortcut in archive context.
*   **Status**: Confirmed.

### **BUG-9.4: Single-Empty-Directory Archive Can Collapse Tree Rendering**
*   **Description**: In archive mode, when the archive contains only one empty directory, ytnova can skip normal tree-node rendering and show that directory identity as appended archive-name/path text instead.
*   **Impact**: Obscures archive structure and creates high-friction navigation confusion in a common edge case.
*   **Remediation**: Keep archive tree rendering consistent in this edge case: render a proper directory node in the tree and keep archive identity separate from child directory labels.
*   **Related**: `BUG-9.3` (name-text rendering contamination), `ROADMAP` Task 13 (path/message formatting hygiene).
*   **Status**: Confirmed.

### **BUG-10: Progress Spinner Can Overwrite Footer/Prompt Help Surfaces**
*   **Description**: During long-running operations, spinner/progress rendering can overwrite footer/prompt help text instead of using a non-obtrusive status area.
*   **Impact**: Hides available actions and makes active workflows look unstable or hung.
*   **Remediation**: Preserve footer/prompt/F1 ownership during progress updates. Render progress in a dedicated non-obtrusive status surface, and degrade to a compact indicator when space is constrained rather than overwriting help text.
*   **Related**: `ROADMAP` Task 20 (Progress Indicators for Copy/Move/Delete/Archive Workflows), `ROADMAP` Task 43 (Refine Contextual F1 Content and Footer-Parity Contract), and `ROADMAP` Task 43.2 (Keep Progress Indicators from Clobbering Footer/Prompt/F1 Guidance).
*   **Status**: Confirmed.

### **BUG-11: Copy/Move/PathCopy Rename Prompt Missing Explicit `AS:` Label**
*   **Description**: The first rename-target prompt in `Copy`, `Move`, and `PathCopy` can appear as `COPY: <source> <edited_target>` (and equivalents) without explicit `AS:` labeling, making source vs new-name intent ambiguous.
*   **Impact**: Increases wrong-target risk and slows high-frequency copy/move workflows because users must infer prompt semantics from field behavior.
*   **Remediation**: Make rename intent explicit in prompt text for all three flows (for example `COPY: <source> AS: <target>`), keep one-flow interaction depth, and keep destination-dir prompt behavior unchanged. Add focused regression coverage for prompt text/flow parity in `Copy`, `Move`, and `PathCopy`. Keep `F1` help, manpage/USAGE text, and specification wording synchronized with final prompt contract.
*   **Related**: `ROADMAP` Task 42 (prompt/help clarity).
*   **Status**: Confirmed.

### **BUG-12: File-View Focus Leak After Parent Jump (`\\`)**
*   **Description**: After entering parent-directory context from file view using `\\`, navigation keys affect the directory pane before explicit mode switch.
*   **Findings**:
    *   Arrow keys can change adjacent directory selection while the user is still in file view.
    *   `Home`/`End` act on the directory window instead of the file list in this state.
*   **Impact**: Breaks view isolation and can cause accidental navigation outside the intended file-list scope.
*   **Remediation**: Keep navigation scope in the file list after `\\` parent jump and require explicit `Enter` transition before directory-pane navigation is allowed.
*   **Status**: Confirmed.

### **BUG-13: Zoom/Split State Corruption After Parent/View Toggles**
*   **Description**: After a sequence involving show-all, repeated view-mode toggles, parent jump (`\\`), and another view toggle, the UI exits the expected zoomed state and shows an empty small pane.
*   **Findings**:
    *   Layout unexpectedly switches back to tree + small window.
    *   Small window can become empty instead of preserving the current context.
*   **Impact**: Breaks predictable navigation flow and increases risk of accidental context loss.
*   **Remediation**: Preserve active zoom/view state across parent-jump and view-toggle transitions; prevent empty-pane state in this flow.
*   **Related**: `BUG-12` (same focus/state-transition family).
*   **Status**: Confirmed.

### **BUG-14: Long Lines Wrap Instead of Truncate in List Views**
*   **Description**: Long entries wrap to additional rows instead of truncating in single-row list rendering contexts (reported in `^f` small-window flow and dir/tree list views).
*   **Impact**: Breaks scanability, corrupts row alignment, and causes ambiguous cursor context in navigation-heavy views.
*   **Remediation**: Enforce truncate/clipping semantics for list-row rendering in these views and add regression tests that fail on wrapping behavior.
*   **Status**: Confirmed.

### **BUG-15: Intermittent Showall/Global Filter Can Hide Matching Files**
*   **Description**: In intermittent edge-case `Showall/Global` + filter combinations, directories can show no files even when matching files exist.
*   **Impact**: Breaks trust in filter correctness and can make users miss valid results.
*   **Remediation**: Capture a minimal deterministic repro and add regression coverage; verify file-list build/count/filter logic remains consistent in `Showall/Global` contexts with active filters.
*   **Status**: Confirmed.

### **BUG-16: Directory Copy Prompt Does Not Clearly State Recursive Behavior**
*   **Description**: Directory copy flow prompt text is not explicit enough that directory copy is recursive, so users cannot reliably predict whether descendants are included.
*   **Impact**: Creates avoidable trust/friction issues in backup-style workflows and increases accidental over-copy concern.
*   **Remediation**: Make directory-copy prompts/confirmation text explicit about recursive behavior and resulting destination semantics before execution.
*   **Status**: Confirmed.

### **BUG-17: Directory Copy Can Appear Successful While Producing No Effective Update**
*   **Description**: In some destination states (for example existing target or edge-case destination handling), directory-copy flow can look like it executed successfully while leaving destination contents unchanged or unclear to the user.
*   **Impact**: High wrong-assurance risk for repeat backup workflows where users expect an update on each run.
*   **Remediation**: Report explicit copy outcome (`updated`, `skipped`, `destination exists`, or error) and never leave no-op outcomes ambiguous. Add regression coverage for existing-destination and missing-parent edge paths.
*   **Status**: Confirmed.

### **BUG-18: Archive Mutations Do Not Show Immediate Results in Archive View**
*   **Description**: In archive mode, mutating actions (for example `rename`, `mkdir`, and copy-in flows) can succeed but the current archive listing does not reflect the change immediately.
*   **Impact**: Creates false-failure perception and high-friction workflow confusion because users may repeat operations that already succeeded.
*   **Remediation**: After any successful archive mutation, update the archive view in-place so users immediately see the effect in the same archive context, with no manual refresh/re-entry/relog required. Preserve cursor/selection when possible.
*   **Related**: `BUG-17` (copy outcome clarity).
*   **Status**: Confirmed.

### **BUG-19: Attributes Name Truncation Can Hide File Identity**
*   **Description**: In attributes/stat contexts, long file names can be truncated using a tail-only style (for example `...fy_xml_integrity.sh`) that hides too much distinguishing information and makes similarly named files harder to differentiate at a glance.
*   **Impact**: Increases wrong-target risk during metadata workflows and slows navigation in dense directories with similar filenames.
*   **Remediation**: Apply an identity-preserving truncation policy for filename-bearing attribute surfaces: keep static deterministic text (no marquee/auto-scroll), prefer `prefix…suffix` for plain filenames, and use suffix-focused clipping only where path-tail context is explicitly higher-value.
*   **Related**: `BUG-14` (no-wrap/truncate contract), `ROADMAP` Task 18 (manual file-column width controls).
*   **Status**: Confirmed.

### **BUG-20: Internal Preview Down-Scroll Can Pass EOF and Repeat Last Page**
*   **Description**: In internal preview paths (`F7` preview and internal `^V` tagged viewer), down-scroll/page-down can continue past EOF and keep redisplaying the last page. Up-scroll behavior does not show this defect. External viewer mode stops correctly at EOF.
*   **Impact**: Produces misleading navigation state and inconsistent behavior between internal and external viewing paths.
*   **Remediation**: Clamp downward preview offsets at the last valid page in shared internal preview rendering paths so bottom-of-file is a hard stop.
*   **Related**: `F7` preview and internal `^V` tagged viewer should be treated as one defect family.
*   **Status**: Confirmed.

### **BUG-21: `/` Jump Replaces Footer Help with Prompt UI**
*   **Description**: Pressing `/` currently switches footer content to a `Jump to:` prompt UI instead of keeping the normal footer help text visible while incremental jump runs.
*   **Impact**: Breaks the expected inline jump flow and creates avoidable UI churn during frequent navigation.
*   **Remediation**: Keep footer help text unchanged during `/` incremental jump and apply immediate selection movement as characters are typed (for example `/y` jumps to the first matching entry) without footer prompt takeover.
*   **Status**: Confirmed.

### **BUG-22: Color Configuration Roles Are Misrouted Across Unrelated UI Surfaces**
*   **Description**: Legacy color-pair names and rendering usage did not map cleanly to visible UI roles. Changing one color key could affect unrelated surfaces, while some documented keys appeared unused or hard to observe.
*   **Historical manual findings (2026-06-25)**:
    *   Stats panel dynamic values are rendered with the same color as nearby static labels in several places, so values such as paths, filesystem names, counts, sizes, attributes, owners, and timestamps cannot be made white independently of labels.
    *   Stats section titles (`FILTER`, `VOLUME`, `VOLUME STATS`, `CURRENT DIR`/`CURRENT FILE`, `ATTRIBUTES`) are inconsistently treated as border text versus ordinary text.
    *   `WINDIR_COLOR` appears to affect the current filter value, static+dynamic text in volume stats, and current-dir totals/matches/tags text.
    *   `WINFILE_COLOR` appears to affect autoview text.
    *   `WINSTATS_COLOR` has no obvious observed effect in the checked flows.
    *   `BORDERS_COLOR` affects line art, but also appears to affect the dynamic path part of the header, stats box titles, static+dynamic text in volume sections, current-dir/attributes path text, and all text in the attributes box.
    *   `MENU_COLOR` affects footer menu text and also the clock color.
    *   Neutral interaction surfaces are inconsistent: footer prompts can turn grey wholesale, history can remain cyan-on-blue when it should be neutral dialog styling, and F2 option/help text mixes prompt, menu, and content roles.
*   **Expected**: Color keys should map to coherent semantic roles. Borders/box lines, static labels, dynamic values, keybinding text, neutral dialog/history surfaces, prompt input fields, preview text, and footer/help text must be independently predictable enough that changing one role does not unexpectedly recolor unrelated UI surfaces.
*   **Impact**: Makes theme tuning unreliable and confusing; users cannot produce a restrained, readable theme because color controls behave like cross-wired chimeras rather than intentional UI roles.
*   **Remediation**:
    *   Audit all uses of semantic role pairs, `WbkgdSet`, `wattr*`, and `COLOR_PAIR` in the header, stats panel, footer/prompt, F2/history, autoview/preview, and dialog paths.
    *   Split static stats labels from dynamic stats values at render call sites.
    *   Separate border/box-line roles from title text and content text.
*   **Resolution**:
    *   Public configuration now selects a semantic theme, while role definitions and file-type palettes live in `etc/ytnova.themes` or user theme catalogs.
    *   Stats labels/titles, dynamic values, borders, tree guides/margins, picker/help surfaces, severity dialogs, preview search hits, header paths, viewer paths, and the clock have dedicated semantic-role routing.
    *   Runtime color-pair vocabulary is semantic.
*   **Related**: `ROADMAP` Task 60 (role-based theme system and restrained default palette).
*   **Status**: Resolved.

### **BUG-23: Recursive Scan Interrupt Responsiveness**
*   **Description**: Interrupting a recursive expansion (`*`) via `ESC` is supported but requires multiple keypresses (Prompt Y/N).
*   **Impact**: Users cannot instantly halt accidental large-branch scans.
*   **Remediation**: Evaluate if `ESC` during `ReadTree` should immediately halt the scan once instead of prompting, given that partial results are preserved.
*   **Status**: Confirmed.

## **Correctness, Consistency, and Naming Defects (Priority Ordered)**

### **BUG-24: Configuration Template Drift (`VI_KEYS`)**
*   **Description**: Discrepancy in default visibility and documentation for `VI_KEYS`.
*   **Findings**:
    *   `default_profile_template.h` uses `VI_KEYS=0`.
    *   `etc/ytnova.conf` uses `VI_KEYS=0`.
    *   User scratchpad reports `ytnova.conf` had `1`.
*   **Impact**: Users may experience "magic" behavior changes if the disk-based config diverges from the internal template.
*   **Remediation**: Ensure the `ytnova --init` generation path strictly matches the `etc/ytnova.conf` provided in the distribution.
*   **Status**: Confirmed.

### **BUG-25: VI Mode Key Ambiguity and Collisions**
*   **Description**: When `VI_KEYS=1` is enabled, lowercase navigation keys (`h/j/k/l`) collide with primary command keys without clear UI signaling.
*   **Findings**:
    *   `j` maps to both `ACTION_MOVE_DOWN` (via `VI_KEY_DOWN`) and historically to `ACTION_LOG_VOLUME` (though currently `l/L` is the log volume key, older documentation/muscle memory remains confused).
    *   `k/K` is used for `ACTION_VOL_MENU`. In VI mode, lowercase `k` is stolen for `Up`, making the volume menu reachable only via uppercase `K`.
*   **Impact**: Inconsistent UI accessibility for power users.
*   **Remediation**: Audit all `VI_KEY` remappings in `key_engine.c` and ensure the footer help lines (`display.c`) dynamically update to show the uppercase variants when `VI_KEYS=1`.
*   **Status**: Confirmed.

### **BUG-26: Incremental Search Legacy Mapping (`F12`)**
*   **Description**: `F12` is used as an alias for `/` (Incremental Search/Jump), but its presence is inconsistent in help strings and documentation.
*   **Impact**: Confuses users about "hidden" keys.
*   **Remediation**: Explicitly document `F12` as a legacy alias or deprecate it in favor of standard `/`.
*   **Parity Principle:** Treat this as a documentation-parity defect class: no active keybinding may exist in runtime without consistent footer, `F1`, and manpage/USAGE coverage.
*   **Status**: Confirmed.

### **BUG-27: Misleading Tree Expansion Action Names**
*   **Description**: The internal `YtreeNovaAction` names for tree expansion are swapped relative to their behavior and documentation.
*   **Findings**:
    *   `+` key maps to `ACTION_TREE_EXPAND_ALL`, but only expands **one level**.
    *   `*` key maps to `ACTION_ASTERISK`, and expands **recursively**.
*   **Impact**: Developer confusion and maintenance risk.
*   **Remediation**:
    *   Rename `ACTION_TREE_EXPAND_ALL` -> `ACTION_TREE_EXPAND` (or `ACTION_TREE_EXPAND_LEVEL`).
    *   Rename `ACTION_ASTERISK` -> `ACTION_TREE_EXPAND_RECURSIVE`.
*   **Status**: Confirmed.

### **BUG-28: Intermittent Split-Brain Redraw Between Stats Box and Main Panes**
*   **Description**: Intermittently, the stats box redraw state can diverge from the main UI surfaces (`path`, `dir`, and `file` windows), leaving one surface fresh while the other appears stale/corrupted.
*   **Impact**: Creates a visibly broken UI state and undermines trust in navigation context during active workflows.
*   **Remediation**: Unify frame redraw ownership so stats and main panes are rendered from one layout snapshot in one update cycle, and force full-surface invalidation/redraw on resize/recovery/error paths.
*   **Related**: `ROADMAP` Task 21 (unified frame redraw contract).
*   **Status**: Confirmed (intermittent; no deterministic repro sequence yet).
