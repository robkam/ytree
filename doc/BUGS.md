# **Bugs and Defects Requiring Fixes**

This file tracks fix-required bugs, architectural violations, and naming inconsistencies that require remediation.

Ordering policy (for all editors, including AI editors):
- Put bugs that are high-impact first after that order remaining bugs by ease of implementation.
- Insert new approved bugs at the correct priority position (do not append by default).
- Bug numbering is a priority countdown where practical (`highest number = highest priority`).
- Bug IDs are unstable labels and are likely to change often due to reprioritization/renumbering.

## **Current Runtime Defects (Highest Priority First)**

### **BUG-38: `Write` Destination Ambiguity and Crash Path**
*   **Description**: In `Write`, entering a plain filename (for example `report-2026-04-11.txt`) can be interpreted as a command execution path instead of file output, and user repro indicates this path can crash after command failure handling.
*   **Impact**: Violates least-surprise behavior, risks data-loss/confusion for new users, and introduces a stability defect (segmentation fault).
*   **Remediation**: Make destination handling explicit and safe: default plain filename/path input to file output, provide an explicit File vs Command destination choice in the flow, keep expert shortcuts as optional aliases, and harden all command/file-open failure paths to eliminate crashes.
*   **Status**: Confirmed.

### **BUG-37: Right Arrow Does Not Drill Down on Expanded Nodes**
*   **Description**: In Directory Mode, `Right Arrow` expands a collapsed node correctly, but performs a no-op when the node is already expanded.
*   **Findings**:
    *   `Right Arrow` on collapsed node: expands one level (works).
    *   `Right Arrow` on already expanded node: no-op (incorrect).
*   **Expected Behavior**:
    *   `Right Arrow` should be progressive: expand if collapsed; if already expanded, move selection to the first child.
*   **Note**:
    *   Right as “expand-or-drill-down” and Left as “collapse-or-parent” is the common expectation.
*   **Impact**: Breaks common tree-navigation muscle memory and slows keyboard-only navigation.
*   **Remediation**: Align `Right Arrow` handling with documented progressive drill-down behavior in Directory Mode.
*   **Status**: Confirmed.

### **BUG-36: F8 Mirrored Tree Changes Can Move Inactive Selection to Wrong Node**
*   **Description**: In `F8` split mode, mirrored expand/collapse changes can move the inactive pane selection unexpectedly to another sibling/ancestor even when the original inactive selection should remain valid.
*   **Repro (Confirmed)**:
*   Select `~/ytree/obj/cmd`, then enter split mode (`F8`).
    *   Collapse `obj`: inactive cursor moves to `obj`; expand `obj`: inactive remains on `obj`.
    *   Collapse a sibling of `obj`: inactive remains on `obj`.
    *   Expand/collapse a sibling above `obj`: inactive selection can jump to another sibling above (pattern not deterministic).
*   **Expected Behavior**:
    *   Inactive selection remains stable under mirrored tree-shape updates while its selected node is still visible/valid.
    *   If invalidated, fallback target must follow a deterministic rule.
*   **Impact**: Breaks split-pane predictability and makes inactive-pane targeting unreliable for copy/move/compare workflows.
*   **Remediation**: Preserve inactive selection by stable node identity (not row/index), and apply deterministic fallback only when the selected node becomes invalid.
*   **Related**: `ROADMAP` Task 45 (inactive split-panel selection semantics + regression coverage).
*   **Status**: Confirmed.

### **BUG-35: F8 Same-Volume Destination Navigation Can Lose Source Selection/Tagged Set**
*   **Description**: In `F8` split mode on the same volume, navigating the destination side (including creating/changing directories during copy/move preparation) can cause the original source file selection/tagged set to disappear or be replaced by the destination-context file list.
*   **Impact**: Breaks split-panel isolation and creates high wrong-target risk in copy/move workflows because source intent is lost while preparing destination paths.
*   **Remediation**: Enforce source-vs-destination state isolation in split mode so destination-side `mkdir`/`cd` and tree navigation cannot mutate source selection/tag state. Preserve source tagged/selection state by stable file identity across destination context changes, and apply deterministic fallback only when a selected source entry truly no longer exists.
*   **Related**: `BUG-36` (inactive split selection stability), `ROADMAP` Task 45 (split selection semantics/regression coverage).
*   **Status**: Confirmed.

### **BUG-34: F7 Preview Over-Restricts Command Availability**
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

### **BUG-33: `Write` Offers/Describes Actions That Are Not Context-Valid**
*   **Description**: `Write` format/options/prompt/help are not consistently aligned with context (`dir`/`file`/`archive`/`tagged`) and can imply workflows that are unavailable or misleading in the active mode.
*   **Impact**: Reduces discoverability and trust, and creates avoidable trial-and-error in critical output/export flows.
*   **Remediation**: Define and enforce a context-valid option matrix for `Write`, expose only valid options in each mode, and keep prompt/help text explicit and non-jargon (including destination examples such as file output and printer-command output). Keep `SPECIFICATION`, `F1` help, and manpage/USAGE text synchronized with the same destination semantics.
*   **Status**: Confirmed.

### **BUG-32: `F10` Hard-Fails When `~/.ytree` Is Missing**
*   **Description**: If `~/.ytree` does not exist, pressing `F10` fails with an edit error instead of letting users proceed with configuration editing.
*   **Impact**: Creates avoidable first-run friction in a common workflow and forces users to discover `--init` before they can use in-app config editing.
*   **Remediation**: On `F10` with missing `~/.ytree`, open an editable default config buffer immediately and create `~/.ytree` only if the user saves; if they exit without saving, do not create the file.
*   **Status**: Confirmed.

### **BUG-31: `SMALLWINDOWSKIP=0` Is Ignored**
*   **Description**: Setting `SMALLWINDOWSKIP=0` does not re-enable the small window; runtime still behaves as if small-window skipping is active.
*   **Impact**: Breaks configuration trust and blocks users from restoring a primary layout element through documented config.
*   **Remediation**: Ensure `SMALLWINDOWSKIP` is parsed/applied correctly so `0` reliably enables the small window and `1` skips it across relevant contexts.
*   **Status**: Confirmed.

### **BUG-30: `F10` Save Does Not Reload Configuration in-Session**
*   **Description**: Editing and saving configuration via `F10` does not update active runtime behavior until ytree is quit and restarted.
*   **Impact**: Violates expected interactive-config workflow and adds avoidable friction after in-app edits.
*   **Remediation**: Live-apply is preferred: reload and apply safe settings immediately after `F10` save. For settings that are not safe to rebind mid-session, show a concise explicit notice that restart is required.
*   **Status**: Confirmed.

### **BUG-29: Dir/Tree View Loses Lines/Path After Returning From `F10` Editor**
*   **Description**: From directory/tree context, running `F10` to edit config and returning to ytree can leave directory and small-window lines/path missing; the same flow from file view does not show this symptom.
*   **Impact**: Corrupts primary navigation UI after a common workflow and reduces operator trust in view stability.
*   **Remediation**: On return from external editor/config-save flow, restore/redraw directory-tree and small-window state deterministically for dir context (including path/header lines), and keep behavior consistent with file-view return path.
*   **Related**: `BUG-15` (state/layout transition corruption family), `BUG-30` (post-`F10` apply/return family).
*   **Status**: Confirmed.

### **BUG-28: External Viewer Return Can Corrupt Ytree UI Redraw**
*   **Description**: Returning from external viewer/pager flows can leave ytree in a visibly corrupted state (for example shell prompt fragments or stray text rendered into panes/footer instead of a clean ncurses repaint).
*   **Impact**: Breaks core usability immediately after viewing, obscures navigation/help text, and reduces trust in terminal-state safety.
*   **Remediation**: Ensure external-view return paths always restore curses mode and perform a full deterministic UI repaint (header, panes, footer/help lines), including prompt-line cleanup before accepting further input.
*   **Status**: Confirmed.

### **BUG-27: Viewer Return (`v` -> `q`) Can Corrupt UI and Input State**
*   **Description**: In WSL repro, running ytree, opening `View` on a file (`v`), then quitting viewer (`q`) can leave the main UI mostly vanished/artifacted (for example stray clock digits only). Follow-on input/state is also corrupted: `^L` has negligible/no effect, `Enter` no longer toggles to dir window, and a subsequent `q` exits ytree.
*   **Impact**: Breaks core post-view workflow and can leave the session in a partially unusable state until restart.
*   **Remediation**: Harden viewer-return restore path to reestablish full ncurses paint + mode/input state atomically (window ownership, active pane mode, key handling context) before processing next key events.
*   **Related**: `BUG-28` (same return/redraw defect family).
*   **Status**: Confirmed.

### **BUG-26: Copy/Move Cancel (`Esc`) Can Leave Footer Blank**
*   **Description**: In `Copy`/`Move` flows, canceling with `Esc` can leave footer/help lines blank instead of restoring the normal context footer.
*   **Impact**: Hides command discoverability immediately after a canceled mutation flow and makes the UI look partially broken.
*   **Remediation**: On all `Copy`/`Move` cancel/exit paths (`Esc` and equivalent cancel keys), restore footer/help ownership deterministically to the active view context and force a full footer redraw before accepting the next command.
*   **Related**: `BUG-6` (footer restore consistency during input flows), `ROADMAP` Task 31 (footer/F1 context parity).
*   **Status**: Confirmed.

### **BUG-25: Prompt Footer/F1 Parity Can Hide Available Prompt Actions**
*   **Description**: In prompt-driven workflows, footer/F1 coverage can omit active prompt actions and semantics (for example completion/browse controls and compare/archive prompt meanings), leaving available behavior under-discoverable.
*   **Impact**: Creates hidden-feature workflow confusion and high-friction issue reports during routine operations.
*   **Remediation**: Enforce a prompt-context parity contract: footer shows currently available prompt actions; F1 may add concise semantics/examples for those same actions, but must not advertise unavailable actions.
*   **Related**: `ROADMAP` Task 31 (footer/F1 context parity), `BUG-20` (hidden archive action), `BUG-33` (prompt/help context mismatch).
*   **Status**: Confirmed.

### **BUG-24: Progress Spinner Can Overwrite Footer/Prompt Help Surfaces**
*   **Description**: During long-running operations, spinner/progress rendering can overwrite footer/prompt help text instead of using a non-obtrusive status area.
*   **Impact**: Hides available actions and makes active workflows look unstable or hung.
*   **Remediation**: Preserve footer/prompt/F1 ownership during progress updates. Render progress in a dedicated non-obtrusive status surface, and degrade to a compact indicator when space is constrained rather than overwriting help text.
*   **Related**: `ROADMAP` Task 54 (progress indicators), `ROADMAP` Task 31 (footer/F1 parity contract).
*   **Status**: Confirmed.

### **BUG-23: Modal Severity Messages Render as Error-Red**
*   **Description**: Centered modal messages can render with error-red styling even when the message severity is informational or warning-level, instead of using severity-specific visual treatment.
*   **Impact**: Blurs severity intent, increases operator confusion, and conflicts with documented message tiers and configurable color expectations.
*   **Remediation**: Perform a full user-message surface audit (modal/footer/status paths), identify all message-producing callsites and severity-routing logic, and enforce a single severity-aware rendering contract. Ensure modal severity maps to `INFO_COLOR`, `WARN_COLOR`, and `ERR_COLOR` from `ytree.conf`, then add focused regression tests that prove correct severity-to-color routing (including config-driven overrides and safe defaults).
*   **Related**: `doc/SPECIFICATION.md` section 6.2 modal severity tiers; `etc/ytree.conf` `[COLORS]` keys `INFO_COLOR`, `WARN_COLOR`, `ERR_COLOR`.
*   **Status**: Confirmed.

### **BUG-22: Copy/Move/PathCopy Rename Prompt Missing Explicit `AS:` Label**
*   **Description**: The first rename-target prompt in `Copy`, `Move`, and `PathCopy` can appear as `COPY: <source> <edited_target>` (and equivalents) without explicit `AS:` labeling, making source vs new-name intent ambiguous.
*   **Impact**: Increases wrong-target risk and slows high-frequency copy/move workflows because users must infer prompt semantics from field behavior.
*   **Remediation**: Make rename intent explicit in prompt text for all three flows (for example `COPY: <source> AS: <target>`), keep one-flow interaction depth, and keep destination-dir prompt behavior unchanged. Add focused regression coverage for prompt text/flow parity in `Copy`, `Move`, and `PathCopy`. Keep `F1` help, manpage/USAGE text, and specification wording synchronized with final prompt contract.
*   **Related**: `ROADMAP` Task 32 (prompt/help clarity).
*   **Status**: Confirmed.

### **BUG-21: Archive Unavailable-Action Message Reports Wrong Shortcut**
*   **Description**: In archive mode, triggering `^W` can show an error message for a different shortcut (`^P is not available in archive mode`).
*   **Impact**: Misleading feedback increases operator confusion and undermines trust in key/action hints.
*   **Remediation**: Ensure unavailable-action messaging reports the actual attempted action/shortcut in archive context.
*   **Status**: Confirmed.

### **BUG-20: Archive Footer Omits Available `Pipe` Action**
*   **Description**: In archive directory context, `Pipe` works but is not shown in footer/help, while users rely on footer discoverability to know what is available.
*   **Findings**:
    *   `Pipe` is available but omitted from archive footer/help lines.
*   **Impact**: Violates "No Hidden Features" discoverability and makes archive workflows look more limited than they are.
*   **Remediation**: Ensure archive footer/help reflects all currently available actions (including `Pipe`) and keeps unavailable actions hidden.
*   **Status**: Confirmed.

### **BUG-19: F8 File-Window Active/Inactive Indicators Aren't Distinct**
*   **Description**: In F8 split mode, active (inverted) versus inactive (bold and underlined) panel cursors are correct in directory view, but both are inverted in file view.
*   **Impact**: Users can't distinguish the active panel during file navigation.
*   **Remediation**: Make active/inactive indicator rendering consistent between directory and file views in split mode.
*   **Status**: Confirmed.

### **BUG-18: Tree `+` Status Marker Renders in Name Text Instead of Margin Slot**
*   **Description**: When a directory is Unlogged/released, the `+` state marker can appear appended to the directory name text rather than in the dedicated tree status/margin position.
*   **Impact**: Blurs directory identity with state signaling and makes tree-state scanning less reliable in navigation-heavy flows.
*   **Remediation**: Render `+` only in the tree status/margin slot for Unlogged entries and keep directory name text unchanged.
*   **Status**: Confirmed.

### **BUG-17: Single-Empty-Directory Archive Can Collapse Tree Rendering**
*   **Description**: In archive mode, when the archive contains only one empty directory, ytree can skip normal tree-node rendering and show that directory identity as appended archive-name/path text instead.
*   **Impact**: Obscures archive structure and creates high-friction navigation confusion in a common edge case.
*   **Remediation**: Keep archive tree rendering consistent in this edge case: render a proper directory node in the tree and keep archive identity separate from child directory labels.
*   **Related**: `BUG-18` (name-text rendering contamination), `ROADMAP` Task 59A (path/message formatting hygiene).
*   **Status**: Confirmed.

### **BUG-16: File-View Focus Leak After Parent Jump (`\\`)**
*   **Description**: After entering parent-directory context from file view using `\\`, navigation keys affect the directory pane before explicit mode switch.
*   **Findings**:
    *   Arrow keys can change adjacent directory selection while the user is still in file view.
    *   `Home`/`End` act on the directory window instead of the file list in this state.
*   **Impact**: Breaks view isolation and can cause accidental navigation outside the intended file-list scope.
*   **Remediation**: Keep navigation scope in the file list after `\\` parent jump and require explicit `Enter` transition before directory-pane navigation is allowed.
*   **Status**: Confirmed.

### **BUG-15: Zoom/Split State Corruption After Parent/View Toggles**
*   **Description**: After a sequence involving show-all, repeated view-mode toggles, parent jump (`\\`), and another view toggle, the UI exits the expected zoomed state and shows an empty small pane.
*   **Findings**:
    *   Layout unexpectedly switches back to tree + small window.
    *   Small window can become empty instead of preserving the current context.
*   **Impact**: Breaks predictable navigation flow and increases risk of accidental context loss.
*   **Remediation**: Preserve active zoom/view state across parent-jump and view-toggle transitions; prevent empty-pane state in this flow.
*   **Related**: File-View Focus Leak After Parent Jump (`\\`) (same focus/state-transition family).
*   **Status**: Confirmed.

### **BUG-14: Long Lines Wrap Instead of Truncate in List Views**
*   **Description**: Long entries wrap to additional rows instead of truncating in single-row list rendering contexts (reported in `^f` small-window flow and dir/tree list views).
*   **Impact**: Breaks scanability, corrupts row alignment, and causes ambiguous cursor context in navigation-heavy views.
*   **Remediation**: Enforce truncate/clipping semantics for list-row rendering in these views and add regression tests that fail on wrapping behavior.
*   **Status**: Confirmed.

### **BUG-13: Intermittent Showall/Global Filter Can Hide Matching Files**
*   **Description**: In intermittent edge-case `Showall/Global` + filter combinations, directories can show no files even when matching files exist.
*   **Impact**: Breaks trust in filter correctness and can make users miss valid results.
*   **Remediation**: Capture a minimal deterministic repro and add regression coverage; verify file-list build/count/filter logic remains consistent in `Showall/Global` contexts with active filters.
*   **Status**: Confirmed.

### **BUG-12: Directory Copy Prompt Does Not Clearly State Recursive Behavior**
*   **Description**: Directory copy flow prompt text is not explicit enough that directory copy is recursive, so users cannot reliably predict whether descendants are included.
*   **Impact**: Creates avoidable trust/friction issues in backup-style workflows and increases accidental over-copy concern.
*   **Remediation**: Make directory-copy prompts/confirmation text explicit about recursive behavior and resulting destination semantics before execution.
*   **Status**: Confirmed.

### **BUG-11: Directory Copy Can Appear Successful While Producing No Effective Update**
*   **Description**: In some destination states (for example existing target or edge-case destination handling), directory-copy flow can look like it executed successfully while leaving destination contents unchanged or unclear to the user.
*   **Impact**: High wrong-assurance risk for repeat backup workflows where users expect an update on each run.
*   **Remediation**: Report explicit copy outcome (`updated`, `skipped`, `destination exists`, or error) and never leave no-op outcomes ambiguous. Add regression coverage for existing-destination and missing-parent edge paths.
*   **Status**: Confirmed.

### **BUG-10: Archive Mutations Do Not Show Immediate Results in Archive View**
*   **Description**: In archive mode, mutating actions (for example `rename`, `mkdir`, and copy-in flows) can succeed but the current archive listing does not reflect the change immediately.
*   **Impact**: Creates false-failure perception and high-friction workflow confusion because users may repeat operations that already succeeded.
*   **Remediation**: After any successful archive mutation, update the archive view in-place so users immediately see the effect in the same archive context, with no manual refresh/re-entry/relog required. Preserve cursor/selection when possible.
*   **Related**: `BUG-11` (copy outcome clarity).
*   **Status**: Confirmed.

### **BUG-9: After Directory Copy/Execute Return, Header/Footer Lines Can Disappear**
*   **Description**: After directory-copy flow or execute-return paths, ncurses header/footer/path lines can disappear while stats/clock remain, and users must press additional keys to restore full frame.
*   **Impact**: Makes the UI look partially broken in high-frequency workflows and reduces operator trust.
*   **Remediation**: Harden post-command frame-restore/redraw ownership so full header/path/footer surfaces are restored atomically before next input.
*   **Related**: `BUG-28`, `BUG-27` (return/redraw defect family).
*   **Status**: Confirmed.

### **BUG-8: Attributes Name Truncation Can Hide File Identity**
*   **Description**: In attributes/stat contexts, long file names can be truncated using a tail-only style (for example `...fy_xml_integrity.sh`) that hides too much distinguishing information and makes similarly named files harder to differentiate at a glance.
*   **Impact**: Increases wrong-target risk during metadata workflows and slows navigation in dense directories with similar filenames.
*   **Remediation**: Apply an identity-preserving truncation policy for filename-bearing attribute surfaces: keep static deterministic text (no marquee/auto-scroll), prefer `prefix…suffix` for plain filenames, and use suffix-focused clipping only where path-tail context is explicitly higher-value.
*   **Related**: `BUG-14` (no-wrap/truncate contract), `ROADMAP` Task 56 (manual file-column width controls).
*   **Status**: Confirmed.

### **BUG-7: Internal Preview Down-Scroll Can Pass EOF and Repeat Last Page**
*   **Description**: In internal preview paths (`F7` preview and internal `^V` tagged viewer), down-scroll/page-down can continue past EOF and keep redisplaying the last page. Up-scroll behavior does not show this defect. External viewer mode stops correctly at EOF.
*   **Impact**: Produces misleading navigation state and inconsistent behavior between internal and external viewing paths.
*   **Remediation**: Clamp downward preview offsets at the last valid page in shared internal preview rendering paths so bottom-of-file is a hard stop.
*   **Related**: `F7` preview and internal `^V` tagged viewer should be treated as one defect family.
*   **Status**: Confirmed.

### **BUG-6: `/` Jump Replaces Footer Help with Prompt UI**
*   **Description**: Pressing `/` currently switches footer content to a `Jump to:` prompt UI instead of keeping the normal footer help text visible while incremental jump runs.
*   **Impact**: Breaks the expected inline jump flow and creates avoidable UI churn during frequent navigation.
*   **Remediation**: Keep footer help text unchanged during `/` incremental jump and apply immediate selection movement as characters are typed (for example `/y` jumps to the first matching entry) without footer prompt takeover.
*   **Status**: Confirmed.

### **BUG-5: Recursive Scan Interrupt Responsiveness**
*   **Description**: Interrupting a recursive expansion (`*`) via `ESC` is supported but requires multiple keypresses (Prompt Y/N).
*   **Impact**: Users cannot instantly halt accidental large-branch scans.
*   **Remediation**: Evaluate if `ESC` during `ReadTree` should immediately halt the scan once instead of prompting, given that partial results are preserved.
*   **Status**: Confirmed.

## **Correctness, Consistency, and Naming Defects (Priority Ordered)**

### **BUG-4: Configuration Template Drift (`VI_KEYS`)**
*   **Description**: Discrepancy in default visibility and documentation for `VI_KEYS`.
*   **Findings**:
    *   `default_profile_template.h` uses `VI_KEYS=0`.
    *   `etc/ytree.conf` uses `VI_KEYS=0`.
    *   User scratchpad reports `ytree.conf` had `1`.
*   **Impact**: Users may experience "magic" behavior changes if the disk-based config diverges from the internal template.
*   **Remediation**: Ensure the `ytree --init` generation path strictly matches the `etc/ytree.conf` provided in the distribution.
*   **Status**: Confirmed.

### **BUG-3: VI Mode Key Ambiguity and Collisions**
*   **Description**: When `VI_KEYS=1` is enabled, lowercase navigation keys (`h/j/k/l`) collide with primary command keys without clear UI signaling.
*   **Findings**:
    *   `j` maps to both `ACTION_MOVE_DOWN` (via `VI_KEY_DOWN`) and historically to `ACTION_LOG_VOLUME` (though currently `l/L` is the log volume key, older documentation/muscle memory remains confused).
    *   `k/K` is used for `ACTION_VOL_MENU`. In VI mode, lowercase `k` is stolen for `Up`, making the volume menu reachable only via uppercase `K`.
*   **Impact**: Inconsistent UI accessibility for power users.
*   **Remediation**: Audit all `VI_KEY` remappings in `key_engine.c` and ensure the footer help lines (`display.c`) dynamically update to show the uppercase variants when `VI_KEYS=1`.
*   **Status**: Confirmed.

### **BUG-2: Incremental Search Legacy Mapping (`F12`)**
*   **Description**: `F12` is used as an alias for `/` (Incremental Search/Jump), but its presence is inconsistent in help strings and documentation.
*   **Impact**: Confuses users about "hidden" keys.
*   **Remediation**: Explicitly document `F12` as a legacy alias or deprecate it in favor of standard `/`.
*   **Parity Principle:** Treat this as a documentation-parity defect class: no active keybinding may exist in runtime without consistent footer, `F1`, and manpage/USAGE coverage.
*   **Status**: Confirmed.

### **BUG-1: Misleading Tree Expansion Action Names**
*   **Description**: The internal `YtreeAction` names for tree expansion are swapped relative to their behavior and documentation.
*   **Findings**:
    *   `+` key maps to `ACTION_TREE_EXPAND_ALL`, but only expands **one level**.
    *   `*` key maps to `ACTION_ASTERISK`, and expands **recursively**.
*   **Impact**: Developer confusion and maintenance risk.
*   **Remediation**:
    *   Rename `ACTION_TREE_EXPAND_ALL` -> `ACTION_TREE_EXPAND` (or `ACTION_TREE_EXPAND_LEVEL`).
    *   Rename `ACTION_ASTERISK` -> `ACTION_TREE_EXPAND_RECURSIVE`.
*   **Status**: Confirmed.
