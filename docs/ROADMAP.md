# **Modernization Roadmap**

---

Ordering policy (for all editors, including AI editors):
- Organize work as `Current Delivery Roadmap` and `Future Enhancements / Wishlist`, then by phase.
- Inside each phase: put items that are high-impact first after that order remaining items by ease of implementation.
- Insert new approved items at the correct priority position (do not append by default).
- In `Current Delivery Roadmap`, number `Task` items top-to-bottom in ascending order (`1` = highest priority).
- In `Future Enhancements / Wishlist`, use `Idea FE-*` IDs in top-to-bottom ascending order (`FE-1` = highest priority in wishlist).
- IDs are unstable labels and are likely to change often due to reprioritization/renumbering.
- Roadmap is forward-looking (`planned`/`in-progress`). Completed items will be removed after landing; add only significant outcomes to `docs/CHANGES.md` and use git history as the full archive.

---

## **Phase 1: Exploitability-First Security Hardening (Pre-Alpha Release Blocker)**
*This phase is first by priority for real-world security readiness. Ship only after these classes are closed or explicitly risk-accepted.*

## **Phase 2: Architecture and Clean-Code Guardrails (Early Prevention)**
*This phase codifies architectural and coding-discipline guardrails early so regressions are blocked before they become backlog debt.*

### **Task 72: Eliminate Oversized Controller Functions (Dispatch-Only End State)**
*   **Goal:** Decompose all oversized controller functions so controllers remain dispatch-focused and behavior logic is extracted into dedicated modules.
*   **Scope:** `src/ui/ctrl_*.c` and any controller-dispatch hotspots identified by `qa-module-boundaries`.
*   **Mechanism:** Set explicit per-function and per-file budgets, split by action family, and keep controller functions as thin routing layers.
*   **Acceptance Criteria:**
*   No controller function exceeds approved line budget.
*   No controller file exceeds approved line budget.
*   `make qa-module-boundaries` and `make qa-all` pass.
*   Split/extracted code preserves existing behavior (no UX/command semantic drift).
*   - [ ] **Status:** Not Started.

### **Task 73: Remove Dead-History Comments + Add Anti-History Comment Gate**
*   **Goal:** Remove comments that describe removed code/history and prevent their reintroduction.
*   **Policy:** Source comments may describe only invariants, ownership/lifetime assumptions, aliasing constraints, or non-obvious design rationale.
*   **Forbidden Comment Classes:** "removed/obsolete/used to", "original code did X", instruction-transcript comments, and commented-out declarations retained as history.
*   **Mechanism:** Add a QA guard script (wired into `qa-all`) that fails on forbidden dead-history comment patterns, with allowlist-only exceptions for migration-required cases.
*   **Acceptance Criteria:**
*   Existing dead-history comments in first-party code are removed or rewritten to durable design intent.
*   New guard fails on forbidden patterns and passes on current baseline.
*   `make qa-all` passes with the new guard enabled.
*   - [ ] **Status:** Not Started.

### **Task 74: Unified Clean-Code Compliance Gate**
*   **Goal:** Enforce clean-code rules continuously through one measurable gate instead of ad-hoc review.
*   **Scope:** Naming quality, function size/argument/side-effect discipline, duplication control, boundary-condition encapsulation, and test clarity/independence.
*   **Mechanism:** Add `qa-clean-code` (included in `qa-all`) that combines static checks + targeted meta-tests:
    *   **Naming checks:** fail on new ambiguous/abbreviated single-letter identifiers outside accepted loop/index conventions; fail on new magic-number literals outside approved constant contexts.
    *   **Function checks:** enforce max function-size budget, max argument-count budget, and fail on new flag-argument signatures; require explicit annotation/wrapper for approved side-effect functions.
    *   **Duplication checks:** fail on new duplicate blocks above threshold unless explicitly allowlisted with rationale.
    *   **Boundary checks:** fail when boundary logic (path bounds/index guards/null-termination guards) is duplicated across modules instead of using shared helpers.
    *   **Test checks:** fail on tests that combine unrelated assertions/flows without clear phase separation; require independent setup/teardown (no order-coupled fixtures or shared mutable global state).
*   **Acceptance Criteria:**
*   `qa-clean-code` exists, is documented, and is wired into `qa-all`.
*   Baseline debt is explicitly allowlisted with owner + removal plan; no silent grandfathering.
*   New violations fail local gate and CI evidence.
*   `make qa-all` passes with `qa-clean-code` enabled on the current baseline.
*   - [ ] **Status:** Not Started.

### **Task 49: Compiler Warning Baseline + No-New-Warnings Gate**
*   **Goal:** Reduce `-Wall/-Wextra` warning debt to a maintained baseline and prevent warning regressions on supported toolchains.
*   **Rationale:** A low-noise warning profile improves signal quality and catches real defects earlier without forcing brittle all-or-nothing local builds.
*   **Scope:** Build/QA policy and warning remediation only; no feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Define and document baseline warning counts for `gcc` and `clang` under current default flags.
*   Add strict QA mode (for example `STRICT=1`) that enables `-Werror` for CI/QA gates while preserving a portable default developer build.
*   Burn down existing warning debt in prioritized batches (safety/correctness first), keeping suppressions minimal and justified.
*   CI/QA fails on new warnings in strict mode for supported compilers.
*   - [ ] **Status:** Not Started.

### **Task 55: Code-Smell Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits current code smells and blocks new/reintroduced structural smell debt.
*   **Scope:** controller growth, god-function budgets, module-boundary violations, complexity hotspots, and architecture drift.
*   **Acceptance Criteria:** Smell baseline audit evidence exists, recurring smell checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unapproved new smell violations.
*   - [ ] **Status:** Not Started.

#### **Task 56: Baseline Code-Smell Audit and Debt Register**
*   **Goal:** Audit current codebase for structural smells and categorize debt with explicit remediation sequencing.
*   **Deliverables:** baseline report covering hotspots, oversized controllers/functions, boundary exceptions, and tracked rationale for retained debt.
*   - [ ] **Status:** Not Started.

#### **Task 57: Strengthen Smell-Prevention Guards**
*   **Goal:** Prevent reintroduction of known smell patterns via automated policy checks.
*   **Mechanism:** Tighten module-boundary/controller-growth policies and require explicit approval paths for exceptions.
*   - [ ] **Status:** Not Started.

#### **Task 58: Smell Gate Evidence as Merge Prerequisite**
*   **Goal:** Ensure smell-audit results are part of mandatory merge evidence, not optional review notes.
*   **Mechanism:** Require successful smell checks in QA artifacts and block integration on unresolved unapproved violations.
*   - [ ] **Status:** Not Started.

## **Phase 3: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Task 1: Add Automated Coverage Reporting and CI Threshold Gate**
*   **Goal:** Integrate `gcov`/`lcov` into Makefile and CI to generate automated statement-coverage reports and enforce a minimum coverage threshold.
*   **Rationale:** Coverage reporting gives a measurable quality signal and prevents silent regression of test effectiveness.
*   **Scope Lock:** Coverage instrumentation, report generation, and CI gating only; no feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Add reproducible coverage targets in Makefile for local and CI usage.
*   CI publishes coverage output as build artifacts and reports pass/fail status.
*   CI fails when measured statement coverage drops below 80% (or configured threshold).
*   Document how to run coverage locally and how threshold policy is enforced in CI.
*   Update `docs/AUDIT.md` in the same change so audit policy reflects implemented coverage commands/gates (not planned-only wording).
*   - [ ] **Status:** Not Started.

### **Task 2: Restructure and Expand Test Suite**
*   **Goal:** Tidy up existing test scripts into a coherent, modular structure and thoroughly expand the regression suite for comprehensive coverage.
*   **Rationale:** A well-structured test suite is easier to maintain and extend. Thorough, systematic coverage ensures reliability and prevents regressions across complex file operations.
*   **Scope Lock:** Test architecture, fixtures, and regression coverage expansion only; no runtime feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Reorganize tests into contributor-friendly modules by domain (filesystem ops, archive ops, split/panel isolation, and UI interaction contracts).
*   Add file/archive integrity regression coverage for mutation workflows with deterministic pre/post file-count and content-hash assertions.
*   Add cancel/interruption/failure-path tests that assert no partial/corrupt leftovers, including archive rewrite paths.
*   Add edge-path coverage for overwrite/self-target, same-path, cross-device behavior, permission/no-space failures, odd filenames, and archive path edge cases.
*   Document fixture/helper conventions so new contributors can add mutation-integrity tests consistently.
*   - [ ] **Status:** Not Started.

### **Task 3: Finalize Documentation**
*   **Goal:** Update the `CHANGELOG`, `README.md`, and `CONTRIBUTING.md` files to reflect all new features and changes before a release.
*   **Rationale:** Ensures users and developers have accurate, up-to-date information about the project.
*   - [ ] **Status:** Not Started.

### **Task 4: Initialize Distributed Issue Tracking (git-bug)**
*   **Goal:** Configure `git-bug` to act as a bridge between the local repository and GitHub Issues. Migrate the contents of `BUGS.md` and `TODO.txt` into this system prior to public release.
*   **Rationale:** Allows the developer to maintain a simple local text-based workflow during heavy development, while ensuring that all tracking data can be synchronized to the public web interface when the project goes live.
*   - [ ] **Status:** Not Started.

### **Task 5: Config Source-of-Truth + Generation/Verification Gate**
*   **Goal:** Enforce one canonical editable default profile source and make generated artifacts deterministic and verifiable.
*   **Source-of-Truth Policy:** `etc/ytree.conf` is the only human-edited default profile source; `src/core/default_profile_template.h` is generated-only and consumed by `--init`.
*   **Mechanism:** Add a reproducible generator path (`etc/ytree.conf` -> `src/core/default_profile_template.h`) and a QA/CI check that fails when generated output is stale or hand-edited.
*   **Acceptance Criteria:**
*   `ytree --init` output remains byte-equivalent to the canonical template semantics.
*   A single documented command regenerates the header deterministically.
*   `make qa-all` (or dedicated gate) fails on source/generated drift.
*   **Files to Modify:** `Makefile`, `scripts/*` (new/updated generator + verifier), `src/core/default_profile_template.h`, and contributor/docs references as needed.
*   - [ ] **Status:** Not Started.

---

## **Phase 4: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

### **Immediate Quick Wins**

### **Task 6: Footer Action Parity in Archive Mode (`Pipe`)**
*   **Goal:** Make archive-mode footer/help lines accurately reflect runtime-available actions, starting with `Pipe`.
*   **Rationale:** Footer/help is the primary discoverability surface; available actions must not be hidden.
*   **Scope Lock:** No command semantics or keybinding behavior changes; visibility/alignment only.
*   **Acceptance Criteria:**
*   Archive footer/help shows `Pipe` whenever it is available in that context.
*   Actions unavailable in archive mode remain absent from archive footer/help.
*   A focused regression test (or existing footer/help test extension) verifies archive footer/action parity.
*   - [ ] **Status:** Not Started.

### **Task 76: Modal Dialog Severity Taxonomy + Full Existing-Modal Color Audit**
*   **Goal:** Classify every existing modal/dialog surface as `severity` (`info`/`warn`/`error`) or `neutral interaction`, then enforce color routing contracts for each class.
*   **Rationale:** Current and future modal trust depends on deterministic visual semantics: severity-bearing messages must use severity colors, while interaction dialogs must remain neutral and consistent.
*   **Scope Lock:** Modal/dialog color taxonomy, routing, and configuration surface only; no keybinding/flow-depth redesign.
*   **Acceptance Criteria:**
*   Define and document a modal taxonomy contract in `docs/SPECIFICATION.md` (severity vs neutral interaction classes).
*   Audit all existing modal/dialog producers and assign each to one class with explicit rationale.
*   Severity-class modals must route through `INFO_COLOR`, `WARN_COLOR`, or `ERR_COLOR` (via mapped runtime color pairs) with focused regression coverage.
*   Neutral-class dialogs (for example selection/picker/help/history/volume interaction surfaces) must not use severity colors; they must use dedicated neutral palette keys.
*   Any neutral dialog surface without a dedicated configurable color key in `ytree.conf` must receive one, with deterministic defaults and synchronized docs/help/manpage text.
*   Add regression coverage that prevents severity/neutral cross-contamination (severity modal rendered neutral or neutral dialog rendered as warn/error).
*   - [ ] **Status:** Not Started.

### **Task 7: Path Message Formatting Audit (`//` Artifact Prevention)**
*   **Goal:** Audit user-facing message/path rendering and eliminate accidental double-slash artifacts in status/error/footer output.
*   **Rationale:** Message correctness is a trust surface; inconsistent path rendering invites avoidable bug reports and operator confusion.
*   **Scope Lock:** Message/path formatting and tests only; no navigation, keybinding, or filesystem behavior changes.
*   **Acceptance Criteria:**
*   Inventory and review path-formatting callsites used by user-visible message surfaces.
*   Route display-path formatting through a shared canonical formatter/contract (or equivalent centralized policy).
*   Add focused regression tests covering root paths, trailing-slash joins, archive/display paths, and no accidental `//` join artifacts.
*   Preserve valid POSIX-leading `//` semantics where intentional; do not blanket-collapse legitimate leading doubles.
*   - [ ] **Status:** Not Started.

### **Task 8: Copy Include-Paths Base/Result Preview Contract (Predictable Root Semantics)**
*   **Goal:** Make `Copy` with `Preserve ancestor paths` explicit and predictable by showing a compact computed preview of base root, relative segment, and resulting destination path.
*   **Rationale:** Users cannot infer include-path base semantics from UI alone, which makes destination depth feel arbitrary and increases wrong-target risk.
*   **Scope Lock:** Prompt/help/docs and regression coverage only; do not change underlying copy/sync semantics in this task.
*   **Acceptance Criteria:**
*   During `Copy` destination input when `Preserve ancestor paths` is enabled, show one compact computed line in the same flow (no extra submenu): `Base:<...>  Rel:<...>  -> <...>`.
*   The displayed base is the active volume root used by runtime path computation.
*   In the same prompt, `[` includes one more parent segment in the relative path (longer path) and `]` removes one parent segment (shorter path); preview updates immediately on each keypress.
*   `F2` destination-directory picker behavior remains unchanged.
*   Rendering stays concise and non-repetitive: single-line summary with deterministic clipping (middle truncation) when width is constrained.
*   Add focused regression tests for preview correctness and resulting destination path in representative filesystem scenarios (including nested roots and absolute destination input).
*   Update `docs/SPECIFICATION.md`, `etc/ytree.1.md`, generated `docs/USAGE.md`, and F1/context help text so include-path root/relative/result contract and `[`/`]` controls are explicit and consistent.
*   - [ ] **Status:** Not Started.

### **Task 9: Proactive Missing-Destination Directory Creation Prompt**
*   **Goal:** When a destination directory is missing in destination-driven workflows, detect it before execution and offer an explicit one-step create confirmation.
*   **Rationale:** Prevents avoidable late failures, reduces wrong-target mistakes from typos, and improves alpha-readiness of copy/move-style flows.
*   **Scope Lock:** Destination validation and confirmation behavior only; no command semantic/keybinding changes.
*   **Acceptance Criteria:**
*   In `Copy` and `Move` destination flows, if the resolved destination directory does not exist, show a single explicit prompt with full target path and default-safe choice: `Create missing directory? (y/N)`.
*   Choosing `y` creates the missing directory path deterministically before the operation continues.
*   Choosing `N`/`Esc` leaves filesystem state unchanged and returns control to destination input flow.
*   On creation failure (permissions/path errors), show a precise actionable error and do not continue the mutation command.
*   Add focused regression coverage for `yes`, `no/cancel`, and failure-path behavior.
*   Update `etc/ytree.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 10: Add Inline `Shift+N` Create-Link Flow (Symlink/Hardlink)**
*   **Goal:** Add an in-app link creation command that mirrors existing `mkdir/newfile/copy` prompt ergonomics without requiring external `X` shell execution.
*   **Rationale:** Link creation is a core file-manager workflow; requiring shell fallback breaks interaction consistency and discoverability.
*   **Scope Lock:** Filesystem link creation UX/behavior only (`symlink` and `hardlink`); no unrelated command flow redesign.
*   **Acceptance Criteria:**
*   Add one primary keybinding: `Shift+N` (`N`) for `Create Link`.
*   The command is available in both directory and file contexts where filesystem mutations are valid, including showall/global file flows.
*   Flow is single-surface (no pre-step menu): first prompt is the link-target input and footer exposes live type toggle (`s`/`h`) with a default already set.
*   Prompt/header contract is explicit and concise (for example: `CREATE LINK [s=symlink h=hard] TARGET:`), and a second prompt captures `LINK NAME:`.
*   Default link type is `symlink`; pressing `s`/`h` in the first prompt switches mode inline without leaving the prompt.
*   Target prefill follows existing copy/newfile conventions for active selection context and remains overrideable by direct typing.
*   Destination resolution in showall/global targets the owner directory of the highlighted entry (not unrelated tree cursor state).
*   Existing `n`/`N` newfile behavior is remapped to preserve intuitive command grouping while keeping help/footer truthfully synchronized.
*   Add focused regression coverage for: symlink create, hardlink create, cancel/no-op behavior, showall/global owner-directory resolution, split-panel isolation, and error-path messaging.
*   Update `etc/ytree.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 11: F7 Top Path Line Must Preserve Full `filename.ext`**
*   **Goal:** In F7 preview mode, the top line above the directory window must display file context as `path + filename.ext` for the selected file.
*   **Rationale:** In preview workflows, the selected file identity must remain explicit and unambiguous.
*   **Scope Lock:** F7 top-line rendering contract only; no preview navigation/keybinding changes in this task.
*   **Acceptance Criteria:**
*   F7 top line includes path context and the selected file name with extension.
*   When width is insufficient, truncate middle of path segment; keep full selected `filename.ext` visible.
*   The same identity-preservation rule applies in filesystem and archive preview contexts.
*   Add focused regression tests for F7 top-line truncation/identity behavior.
*   Update `etc/ytree.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 12: Manual File-Column Width Controls (`[` Narrower, `]` Wider, `{` / `}` Reset)**
*   **Goal:** Add explicit keyboard controls for file-list column width so users can quickly trade density vs readability in the file window.
*   **Rationale:** Long-name workflows need fast, deterministic control over visible filename identity without terminal resize churn.
*   **Scope Lock:** File-window list column width controls only; no F7 split-preview width redesign in this task.
*   **Acceptance Criteria:**
*   `[` decreases file-column width in fixed-width list layouts.
*   `]` increases file-column width in fixed-width list layouts.
*   `{` / `}` resets to default auto-layout behavior.
*   Behavior is deterministic and static (no marquee/auto-scrolling text).
*   Footer/F1 help documents these keys in file contexts where they apply.
*   Add focused regression coverage for width adjust left/right/reset behavior and bounds handling.
*   **Related:** Task 13 (F7 pane-width tuning).
*   - [ ] **Status:** Not Started.

### **Task 13: Adjustable List/Preview Width in `F7` Mode**
*   **Goal:** Allow users to adjust the relative width of file-list and preview panes while in `F7` preview mode.
*   **Rationale:** Different file types and terminal sizes benefit from quick width tuning during inspect workflows.
*   **Scope Lock:** `F7` pane-width behavior only; no split-mode (`F8`) layout redesign.
*   **Acceptance Criteria:**
*   Provide portable primary resize keys in `F7` (`[` narrower list, `]` wider list, `0` reset default split).
*   Divider movement direction is explicit and intuitive: `[` always reduces file-list width and `]` always increases file-list width, regardless of which border visually moves.
*   Width changes preserve current file selection and preview scroll context.
*   Behavior is deterministic and static (no marquee/auto-scrolling text).
*   Footer/F1 help and config docs are updated when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 14: Progress Indicators for Copy/Move/Delete/Archive Workflows**
*   **Goal:** Add consistent progress feedback for long-running mutation workflows (`Copy`, `Move`, `Delete`, archive create/extract/rewrite).
*   **Rationale:** Users need immediate confidence that work is active and not hung, especially during large operations.
*   **Scope Lock:** Progress signaling and UI/status messaging only; no changes to command semantics, confirmation policies, or keybindings.
*   **Acceptance Criteria:**
*   For measurable work totals, show progress bar + percent (and ETA where stable) for copy/move/delete/archive operations.
*   When total work is initially indeterminate, show spinner by default; transition to bar/percent/ETA only if total becomes measurable.
*   Progress rendering must not overwrite footer/prompt/F1 help surfaces; on constrained layouts, degrade to a compact indicator while preserving help readability.
*   Behavior follows the specification conventions for informative motion and static/non-decorative UI.
*   Footer/F1/manpage wording is updated where needed so behavior is discoverable and consistent.
*   Add focused regression coverage for progress-state selection (indeterminate vs measurable) and completion/error transitions.
*   - [ ] **Status:** Not Started.

### **Task 75: Unify Stats + Main-Pane Frame Redraw Contract**
*   **Goal:** Eliminate intermittent split-brain rendering where stats and main panes update on different redraw lifecycles.
*   **Rationale:** UI trust depends on one coherent frame; partial redraw divergence creates stale/corrupted mixed states.
*   **Scope Lock:** Rendering/invalidation pipeline and regression coverage only; no command/keybinding semantics changes.
*   **Acceptance Criteria:**
*   Stats, path, dir, and file surfaces are drawn from one frame/layout snapshot and flushed in one update cycle.
*   Resize, mode-switch, and recoverable-error paths trigger deterministic full-surface invalidation and redraw.
*   No persistent mixed state where stats is fresh while main panes are stale (or vice versa) after redraw-triggering actions.
*   Add focused regression coverage for redraw coherence across resize/mode toggles and representative recovery paths.
*   - [ ] **Status:** Not Started.

### **Task 15: Clarify Internal `^V` Navigation for File vs Hit Traversal**
*   **Goal:** Make internal `View Tagged` (`^V`) navigation unambiguous by separating file-to-file movement from hit-to-hit movement.
*   **Rationale:** Current flow is easy to misinterpret (`Space` paging, `S` sort, and `^S` tagged search/filter context), which increases user friction during review workflows.
*   **Scope Lock:** Internal `^V` viewer behavior/help only; do not change tagged-filter semantics in file/archive list mode.
*   **Acceptance Criteria:**
*   Keep `n/p` for next/previous file.
*   Keep `Space` and page keys as page movement only.
*   Add hit navigation within current file set as `/` next-hit and `?` previous-hit.
*   In `TAGGEDVIEWER=external` mode, hit traversal remains pager-native (for example `less` keys) and is not remapped by ytree.
*   Footer and F1 help in internal `^V` mode explicitly show file-nav keys and hit-nav keys.
*   `^S` remains the tagged-list search/filter action outside viewer mode and is documented distinctly.
*   Add focused regression coverage for key behavior and help discoverability in this mode.
*   - [ ] **Status:** Not Started.

### **Task 16: Harden `Write` Destination UX (Least Surprise)**
*   **Goal:** Make `Write` destination handling explicit and predictable for both Unix power users and new users, while keeping the interaction path shallow.
*   **Rationale:** Current destination parsing is ambiguous and error-prone; users should not need hidden syntax to perform a basic file write.
*   **Scope Lock:** Keep key as `W` labeled `Write`; no extra submenu layers.
*   **Acceptance Criteria:**
*   `Write` keeps a one-flow path: `W -> format key -> destination type -> destination -> Enter`.
*   Destination type is explicit (`File` / `Command`, plus `Device` if retained), with sensible defaults and history.
*   Plain filename/path input in File destination mode writes to file output by default (no hidden marker required).
*   Command destination mode accepts direct command input (for example printer command `lp`) with explicit prompt wording/examples.
*   Expert shortcuts remain accepted where safe (for example `>path`), but are optional.
*   Destination prompt text avoids unclear jargon (for example prefer user-facing wording over raw `CWD` shorthand).
*   `W` remains labeled `Write`; F1 help clarifies that `Write` includes print/device workflows.
*   No crash on command-not-found or destination-open failures.
*   - [ ] **Status:** Not Started.

### **Task 17: Enforce `Write` Context-Valid Option Matrix + Regression Gate**
*   **Goal:** Ensure `Write` offers only valid formats/actions per active context (`dir`/`file`/`archive`/`tagged`) and that prompt/help always match runtime behavior.
*   **Rationale:** UI option surfaces must be truthful to reduce friction and prevent hidden-feature drift.
*   **Scope Lock:** Behavior alignment and tests only; no new keybindings in this task.
*   **Acceptance Criteria:**
*   A documented matrix defines valid `Write` options by context.
*   Runtime UI exposes only matrix-valid options in each context.
*   Regression tests verify option visibility/behavior parity across at least filesystem + archive contexts.
*   Regression tests verify destination semantics (plain filename file-output default, command destination behavior, and no-crash error paths).
*   F1/help text stays synchronized with the same matrix contract.
*   `docs/SPECIFICATION.md`, `etc/ytree.1.md`, and generated `docs/USAGE.md` are updated in the same delivery so docs match runtime behavior.
*   - [ ] **Status:** Not Started.

### **Task 18: Add `Catalog` Output Mode to `Write`**
*   **Goal:** Extend the existing `Write` format dialog with a `Catalog` mode that exports a deterministic file/directory inventory (similar intent to `ls -1pR`) instead of file contents.
*   **Rationale:** Users need an in-app way to generate list/report output to command or file without dropping to shell-specific workflows.
*   **Scope Lock:** Add format behavior only; do not define or change keybindings in this task.
*   **Acceptance Criteria:**
*   `Write` prompt includes `Catalog` alongside existing formats.
*   Catalog output can be sent to command or file via existing `Write` destination flow.
*   Output contract is documented (recursion rules, directory markers, ordering, archive behavior).
*   Focused regression tests cover at least one filesystem case and one archive case.
*   - [ ] **Status:** Not Started.

### **Task 19: Remove Footer Prompt for / Search**
*   Goal: Keep existing / search behavior in all contexts (Dir, File, Showall, Global), but stop using the footer prompt area for search input.
*   Rationale: Current search semantics already work; only the footer prompt is unnecessary UI churn.
*   Requirements:
*   Preserve current semantics: incremental match while typing, Enter confirms jump, Esc cancels.
*   Typing `/y` must keep the footer help text unchanged and move selection immediately to the first directory/file whose name starts with `y`.
*   Do not redraw/replace footer help lines during / search.
*   Use a non-footer inline input/render path for search text and match feedback.
*   Status: Not Started

### **Task 20: Enforce One-Level Primary Action Depth (Prompt-Chain Audit)**
*   **Goal:** Audit and remediate primary interactive workflows so the common path stays `key -> Enter -> result` with at most one submenu/prompt layer.
*   **Rationale:** Deep prompt chains increase friction and slow high-frequency workflows.
*   **Scope Lock:** Interaction depth, defaults, and prompt composition only; no command semantic changes in this task.
*   **Acceptance Criteria:**
*   Inventory primary action flows across filesystem, archive, and split contexts; identify any flow that exceeds one submenu/prompt layer.
*   Produce a complete `keybinding -> flow` audit for filesystem, archive, and split contexts covering all keybindings that open submenus/prompts.
*   For each audited keybinding, record current chain steps, common-path step count, submenu depth, and manual repro keys.
*   Deliver the audit output as a manual QA checklist so submenu-chain offenders can be exercised directly.
*   For each flagged flow, define a single-surface option model with sensible defaults and per-option key toggles.
*   Keep explicit/safe target confirmation where required (for example compare and write destinations) without reintroducing chained menu depth.
*   If a deep flow is temporarily unavoidable, provide an equivalent fast path and document the exception.
*   Add regression coverage for at least one remediated deep flow to prevent prompt-chain regressions.
*   - [ ] **Status:** Not Started.

### **Task 21: Simplify Compare Mode Flow with Persistent Presets**
*   **Goal:** Keep compare fast and explicit by consolidating options in compare mode while preserving current safe defaults and target confirmation.
*   **Rationale:** Compare is high-frequency and should require fewer chained prompts without hiding safety-critical target selection.
*   **Dependency:** Sequence after Task 20 prompt-chain simplification baseline.
*   **Scope Lock:** Compare flow only (`j/J` entry behavior); no unrelated keybinding or footer redesign.
*   **Acceptance Criteria:**
*   Compare remains a modal state and compare footer/help owns the footer while active.
*   In compare mode, only compare keys are active; non-compare keys are silent no-ops.
*   No conflicting quick-key mappings are permitted in compare mode.
*   Keep explicit compare target prompt (no implicit target execution).
*   Add one-surface compare option toggles (for example basis/tag settings) without reintroducing prompt chains.
*   Persist last-used compare options across restart; config values seed defaults and runtime usage updates remembered defaults.
*   Default behavior remains unchanged when quick/preset config is absent.
*   Add focused regression coverage for compare behavior and split-panel isolation.
*   Update compare docs/help text in `etc/ytree.1.md` and regenerate `docs/USAGE.md`.
*   - [ ] **Status:** Not Started.

### **Task 22: Add Recursive Directory Compare in `J` Flow**
*   **Goal:** Support recursive directory-tree compare from the existing `J` compare flow.
*   **Rationale:** Recursive compare is a practical file-manager workflow and improves alpha usefulness for real tree-diff tasks.
*   **Scope Lock:** Add recursive compare capability and prompt/menu wiring only; do not redesign unrelated compare UI.
*   **UX Direction:** Keep `J` as the compare entry point. Use one submenu level or a direct compare prompt with explicit recursive choice (`Recursive: y/N`).
*   **Acceptance Criteria:**
*   Recursive and non-recursive directory compare are both available from the same `J`-entry compare flow.
*   The recursive choice is explicit and discoverable in compare prompts/help.
*   Compare target confirmation and split-panel isolation behavior remain unchanged.
*   `etc/ytree.1.md` and generated `docs/USAGE.md` are updated when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 23: Lock Inactive Split-Panel Selection Semantics + Regression Coverage**
*   **Goal:** Define and enforce deterministic inactive-panel cursor behavior under mirrored tree-structure changes in `F8` split mode.
*   **Rationale:** Real-time mirrored tree updates are useful, but must stay predictable when parent/ancestor collapse, add, or delete operations change visibility.
*   **Scope Lock:** Selection/cursor semantics and regression coverage only; no unrelated split-layout or keybinding redesign.
*   **Acceptance Criteria:**
*   Inactive selection remains unchanged when its selected node is still visible/valid after mirrored updates.
*   When invalidated, fallback target follows a deterministic order (nearest visible ancestor, then next/previous visible sibling, then root visible node).
*   Add focused regression tests for mirrored collapse, sibling/ancestor delete, and sibling/ancestor add scenarios.
*   Update `docs/SPECIFICATION.md` contract references if implementation details differ during delivery.
*   - [ ] **Status:** Not Started.

### **Task 24: Enable Practical Command Subset in `F7` Preview (Keep `F8`/`Tab` Blocked)**
*   **Goal:** Finish `F7` as an in-place work mode: users can run common file actions without leaving preview, while `F8`/`Tab` stay blocked for preview-state safety.
*   **Rationale:** `F7` currently feels unfinished because common workflows still require repeated exits.
*   **Scope Lock:** `F7` command availability contract, help/footer parity, and regression coverage only; no split-layout redesign.
*   **Acceptance Criteria:**
*   Define and implement the core `F7` action set for inspect-and-act workflows (including tag/search/view results/compare/copy/move/rename, plus existing high-value file actions).
*   In `F7`, `^T` tag-all, `^S` search, and `^V` tagged/search-result view flows work without exiting preview mode.
*   Tagged search hits/results are visibly highlighted in `F7` preview.
*   `F8` and `Tab` are explicit no-ops in `F7` mode.
*   Footer/F1 help in `F7` accurately reflects allowed actions and blocked keys.
*   Add focused regression tests for allowed-command execution in `F7` and blocked-key enforcement (`F8`, `Tab`).
*   Update `etc/ytree.1.md` and regenerate `docs/USAGE.md` when behavior lands.
*   - [ ] **Status:** Not Started.

### **Phase Follow-On Work**

### **Task 25: Harden Build Source Discovery (Recursive + Deterministic)**
*   **Goal:** Update build source discovery so all C files under `src/` are discovered recursively with deterministic ordering.
*   **Rationale:** Current discovery only covers up to one subdirectory level and will miss files after module reorganization.
*   **Scope Lock:** Build discovery and related guard/test updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   Build still succeeds with `make clean && make`.
*   Full QA gate still passes with `make qa-all`.
*   Source file list ordering is deterministic across runs.
*   - [ ] **Status:** Not Started.

### **Task 26: Reorganize Modules into Shallow Hierarchical Folders**
*   **Goal:** Group modules into shallow, purpose-based subfolders and update build/header/linkage references accordingly.
*   **Rationale:** Improves discoverability and ownership without changing behavior.
*   **Scope Lock:** File moves + include/path/build/script/test reference updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   `make clean && make` passes.
*   `make qa-module-boundaries` and `make qa-all` pass.
*   No runtime behavior changes.
*   Folder depth remains shallow (max one extra level under `src/ui` and `src/cmd`).
*   - [ ] **Status:** Not Started.

### **Task 27: Decompose Remaining Hotspot Modules (Atomic Subtasks)**
*   **Goal:** Reduce complexity in remaining hotspot files by extracting cohesive action families into focused modules while preserving behavior.
*   **Rationale:** These files remain risk hotspots after controller decomposition and slow safe feature delivery.
*   **Execution Rule:** Must be delivered one atomic subtask at a time (3.1 to 3.5), each with its own architect plan, developer pass, auditor pass, and QA evidence.
*   - [ ] **Status:** Not Started.

### **Task 28: Decompose `src/ui/ctrl_file_ops.c` (`handle_tag_file_action` focus)**
*   **Goal:** Extract large tagged-action branches from `handle_tag_file_action` into focused helpers/modules.
*   **Scope Lock:** Preserve all tagged-file behavior and command semantics.
*   **Acceptance Criteria:** Smaller dispatcher function, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 29: Decompose `src/ui/key_engine.c`**
*   **Goal:** Separate key mapping/dispatch concerns from input-loop mechanics and context-specific action routing.
*   **Action Name Cleanup:** Normalize tree-expand action identifiers so names match behavior: shallow expand (`+`) is `ACTION_TREE_EXPAND`, recursive expand (`*`) is `ACTION_TREE_EXPAND_RECURSIVE`, and any redundant tree-expand identifier is merged or removed. Update key/action mappings and related tests with no behavior change.
*   **Scope Lock:** No keybinding behavior change unless explicitly approved in a separate task.
*   **Acceptance Criteria:** Cleaner dispatch boundaries, consistent action naming, unchanged key behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 30: Decompose `src/cmd/copy.c`**
*   **Goal:** Isolate copy conflict handling, path/precondition validation, and transfer orchestration into focused units.
*   **Scope Lock:** No copy/move/archive user-visible behavior changes.
*   **Acceptance Criteria:** Reduced complexity in core copy path, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 31: Decompose `src/cmd/profile.c`**
*   **Goal:** Split profile parsing, validation/defaulting, and apply/update logic into focused units.
*   **Scope Lock:** No configuration semantic changes.
*   **Acceptance Criteria:** Clear parser/apply separation, unchanged config behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 32: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/util/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   - [ ] **Status:** Not Started.

### **Task 33: Implement Responsive Adaptive Footer**
*   **Goal:** Make the two-line command footer dynamic based on terminal width.
    *   **Compact (constrained dimensions):** Show all currently available actions as bound-key hints only (minimal/no labels), and always keep `(F1)` visible for full help.
    *   **Standard (80-120 cols):** Show the standard set (current behavior).
    *   **Wide (> 120 cols):** Unfold "hidden" or advanced commands (Hex, Group, Sort modes) directly into the footer so the user doesn't have to remember them.
*   **Rationale:** Preserves full action discoverability in constrained terminals without wrapping/clutter while maximizing utility on larger displays.
*   **Mechanism:** Define command groups (Priority 1, 2, 3). In `DisplayDirHelp` / `DisplayFileHelp`, construct the string dynamically based on `COLS`.
*   - [ ] **Status:** Not Started.

### **Task 34: Implement Integrated Help System**
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   - [ ] **Status:** Not Started.

### **Task 35: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good; redundant duplicate bindings should not be listed).
*   **VI Mode Signaling**: Ensure footer help lines dynamically reflect uppercase commands (e.g., `(K) Vol` instead of `(k) Vol`) when `VI_KEYS=1` is active to avoid navigation collisions.
*   **Ctrl-Held Footer Signaling:** While `Ctrl` is physically held, show the `Ctrl` shortcut footer and keep it visible for the full hold duration. On `Ctrl` release, immediately restore the normal context footer. This is transient key-state feedback, not a toggle mode.
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   - [ ] **Status:** Not Started.

### **Task 36: Enforce Footer/F1 Context-Parity Contract (gettext-ready)**
*   **Goal:** Ensure F1 help is concise, context-specific, and complete for each footer/help variant, with no missing commands.
*   **Rationale:** Footer and F1 are the primary in-app guidance surfaces; they must match exactly while keeping F1 brief and pushing detail to manpage/USAGE.
*   **Scope Lock:** Help contract, coverage matrix, and text-structure readiness only; no command behavior changes in this task.
*   **Acceptance Criteria:**
*   For each supported context, every footer command appears in the matching F1 help set with concise wording and no essay-style descriptions.
*   For active prompt contexts, footer lists currently available prompt actions; F1 may add brief semantics/examples for those same actions but must not introduce actions unavailable at runtime.
*   First-pass required contexts: FS dir, FS file, VFS dir, VFS file, F7, F8, Showall, Global, tagged flows, `VI_KEYS=1` variants, and Ctrl-held footer variant.
*   Context-sensitive actions keep short in-app summaries while full semantics remain in `etc/ytree.1.md`/`docs/USAGE.md` (for example compare `J` modes, compare basis/tag/hash meaning, and compress format/extension behavior).
*   Help text paths are structured for gettext extraction/reuse (no duplicated ad-hoc strings per view path).
*   Add regression checks that detect footer/F1 parity drift in covered contexts.
*   Add a keybinding parity audit gate that verifies active runtime keybindings remain consistently documented across footer, `F1`, and `etc/ytree.1.md`/`docs/USAGE.md`.
*   - [ ] **Status:** Not Started.

### **Task 37: Replace `^F` Mode Cycling with Unified Numeric `FileInfo` Band (`1..9`, `0`)**
*   **Goal:** Replace display-mode cycling with direct numeric `FileInfo` controls for the focused panel.
*   **Behavior Contract:**
*   `1` => Name only (default/baseline). This is also the reset-to-default selection.
*   `2` => Long.
*   `3` => Owner.
*   `4` => Times.
*   `5` => toggle file-size units (`binary` vs `human-readable`) for list/stat surfaces that display size.
*   `5` composes with the currently selected file-info mode and does not reset other file-info toggles.
*   `6` => toggle symlink row rendering (`name` vs `name -> target`) in list rows.
*   `7` => toggle richer metadata/text-snippet view.
*   `8` => toggle file-type/summary view.
*   `9` => toggle brief/full width behavior for the focused panel.
*   `0` => Git-focused file-info band (status-oriented view) when the current scope is inside a Git worktree.
*   Number keys are toggle-driven controls: `0..4` select the primary file-info layout while `5..9` toggle additive display behaviors.
*   Applies to file-display rendering across normal list contexts for the active panel (whether focus is currently on tree/dir window or file window); not active in `F7` preview mode.
*   If a requested mode is unsupported in the active context (for example VFS file mode `4`, or `0` outside a Git worktree), do a silent no-op (no beep).
*   Git band (`0`) defaults to off, uses cached/non-blocking status refresh, and must not stall list rendering in large repos.
*   Add `FILE_SIZE_UNITS=binary|human-readable` profile setting (default `binary`) as the seed for `5`.
*   **Keybinding Policy:** Remove `^F` from runtime behavior and help/manpage docs. This task is the explicit keybinding-change exception referenced by Task 29 scope lock.
*   **UX/Help Policy:** Footer stays concise (`1..0 FileInfo`); full key semantics live in F1 help/manpage.
*   - [ ] **Status:** Not Started.

### **Task 38: Add Case-Sensitive Sort Toggle + Profile Default**
*   **Goal:** Add case-sensitivity as a sort option in the existing sort flow and profile defaults.
*   **Rationale:** Users need deterministic lexical control without introducing extra global keybindings.
*   **Scope Lock:** Sort comparison behavior only; no tree/file model changes.
*   **Acceptance Criteria:**
*   Add `SORT_CASE_SENSITIVE=0|1` profile setting (default `0`) and wire it to default sort behavior.
*   Existing sort prompt (`S` flow) includes a case-sensitivity toggle.
*   Footer/F1/help/manpage text are synchronized for the new sort option.
*   - [ ] **Status:** Not Started.

#### **Task 39: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.
*   - [ ] **Status:** Not Started.

#### **Task 40: Refactor Input Loop for Event Handling**
*   **Task:** Modify `key_engine.c` to support non-blocking input handling.
*   **Logic:**
    *   Currently, `Getch()` blocks indefinitely waiting for a key.
    *   **New Logic:** Use `select()` or `poll()` to wait on **two** file descriptors:
        1.  `STDIN_FILENO` (The keyboard).
        2.  `Watcher_GetFD()` (The inotify handle).
    *   If the Watcher FD triggers, call `Watcher_CheckEvents()`. If it confirms a change, set a global flag `refresh_needed = TRUE`.
    *   If STDIN triggers, proceed to `wgetch()`.
*   - [ ] **Status:** Not Started.

#### **Task 41: Implement Live Refresh Logic**
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

#### **Task 42: Update Watch Context on Navigation (Current-Directory Auto-Refresh Context)**
*   **Task:** Ensure the watcher always monitors the *current* directory so the file list the user is looking at stays fresh without a manual reload.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:** Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytree`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window. In user-facing terms, auto-refresh should follow the current working view.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.
*   - [ ] **Status:** Not Started.

#### **Task 43: Implement Directory Filtering (Non-Recursive)**
*   **Description:** Extend Filter to support directory-pattern tokens identified by a trailing slash.
    *   `dir/` means include matching directories in the current tree view.
    *   `-dir/` means exclude matching directories in the current tree view.
    *   Directory tokens can be combined with existing file-pattern tokens in the same filter spec.
    *   This logic is non-recursive and visibility-only: it affects what is shown in the current view, not internal directory state.
*   - [ ] **Status:** Not Started.

### **Task 44: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option to globally disable external viewers, forcing the use of the internal viewer.
*   **UI Note:** Expose this in the planned `F10` configuration UI when that panel is implemented.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Coverage Clarification:** This task also covers single-file `V` parity with tagged viewing: users must be able to choose internal vs external behavior consistently for both single-file view and tagged-view workflows.
*   - [ ] **Status:** Not Started.

### **Task 45: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   - [ ] **Status:** Not Started.

### **Task 46: Standardize Internal Viewer Layout**
*   **Goal:** Ensure the internal viewer's layout geometry matches the main application (borders, headers, and footer).
*   - [ ] **Status:** Not Started.

#### **Task 47: Implement Archive Move (`M`) Support**
*   **Description:** Implement `M` (Move) for archives. Intra-archive moves use the Rewrite Engine to rename paths. Cross-volume moves use Copy-Extract + Delete.
*   - [ ] **Status:** Not Started.

### **Task 48: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   - [ ] **Status:** Not Started.

---

## **Phase 5: Permanent Security Gates**
*This phase is an enforcement gate for security risk classes: audit baseline debt, then detect and block introduced/reintroduced security findings on every non-trivial change.*

### **Task 50: Security Risk Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits the current codebase for security risks and blocks new or reintroduced security findings.
*   **Scope:** shell-command construction and escaping boundaries, archive path trust policy, tempfile lifecycle, and unsafe API usage.
*   **Acceptance Criteria:** Security baseline audit evidence exists, recurring security checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unresolved blocker/high security findings.
*   - [ ] **Status:** Not Started.

#### **Task 51: Baseline Security Debt Audit and Classification**
*   **Goal:** Run and document a focused baseline audit of current security risk classes already in scope for Phase 0.
*   **Deliverables:** findings inventory with severity, owner, disposition (fix now vs tracked debt), and explicit residual-risk notes.
*   - [ ] **Status:** Not Started.

#### **Task 52: Expand Security Guard Coverage to Block Reintroduction**
*   **Goal:** Ensure banned/legacy security-sensitive APIs and patterns are explicitly rejected by automated guard scripts.
*   **Mechanism:** Extend guard checks for legacy unsafe escaping/runtime paths and other approved denylisted APIs/patterns.
*   - [ ] **Status:** Not Started.

#### **Task 53: Security Regression Gate in CI + Merge Workflow**
*   **Goal:** Make security verification non-optional in routine change flow.
*   **Mechanism:** Require security gate evidence for non-trivial PRs and keep merge blocked until gates pass.
*   - [ ] **Status:** Not Started.

### **Task 54: Add Security Fuzzing Harness for High-Risk Input Paths**
*   **Goal:** Add fuzzing coverage (for example libFuzzer) for archive parsing and shell-command construction paths to detect malformed-input crashes and security-critical edge cases early.
*   **Rationale:** Complements static checks and regression tests with adversarial input exploration.
*   **Scope Lock:** Harness, seed corpus, and reproducible crash-minimization workflow only; no feature UX changes in this task.
*   **Acceptance Criteria:**
*   Reproducible fuzz targets exist for archive parsing and command-construction boundaries.
*   QA documentation defines how to run fuzz smoke jobs and triage crashes.
*   Findings flow into the existing security gate workflow.
*   - [ ] **Status:** Not Started.

---

## **Phase 6: Current Delivery Completion Queue**
*This phase is still current-delivery scope and contains implementation work that is planned to land.*

### **Task 59: Implement Advanced Batch Rename**
*   **Goal:** Add a ytree-native batch rename flow for tagged files with numbering support, casing changes (`Tab`), substring replacement, and pattern-based keep/remove operations.
*   **Rationale:** Essential power-user feature for managing large file sets without forcing one-by-one rename loops.
*   **Preview/Apply Contract:** Batch rename is preview-first. Show `old -> new` results before mutation and support per-item apply controls: `y` (apply current), `n` (skip current), `a` (apply all remaining), `Esc` (cancel remaining).
*   - [ ] **Status:** Not Started.

### **Task 60: Unify Copy Semantics and Add Directory Sync (`Y`)**
*   **Goal:** Define one clear `Copy` contract (with optional ancestor-path preservation) and add a guided directory-sync flow from dir footer `Y`, backed by `rsync` where practical.
*   **User-Facing Behavior:**
    *   **Copy (file/tagged files):** Non-recursive single-item copy behavior is explicit and predictable.
    *   **Copy (directory/tagged directories):** Recursive copy behavior is explicit and predictable.
    *   **Preserve ancestor paths (option):** Uses the same copy selection as `Copy`, but destination path preserves ancestor-relative path from the operation base root (logged/selected source root, never `/`).
    *   **Dir-footer sync entry:** In directory context, `Y` opens sync flow with explicit source/destination, preview-first execution, and clear completion outcomes.
    *   **Mirror / one-way synchronize:** Treat the selected files or source tree as the source of truth. Copy new files, replace changed files, and optionally delete destination files that do not exist in the source selection.
    *   **Execution model:** Where practical, delegate recursive synchronize/update work to `rsync` rather than reimplementing tree-sync logic inside ytree.
    *   **Source-scope policy:** Unlogged directories are excluded from copy source scope by default unless explicitly selected/logged by the user.
*   **Rationale:** Users need one coherent copy model (source-type-based semantics) plus a reliable repeat-backup workflow; rsync-backed execution reduces reinvention risk.
*   **Acceptance Criteria:**
    *   Prompt/help text makes `Copy` semantics explicit before execution: file sources are non-recursive; directory sources are recursive.
    *   `Preserve ancestor paths` is documented as a `Copy` option with base-root semantics relative to logged/selected source root (never filesystem `/`).
    *   Dir footer exposes `Y` as sync entry with footer/F1/manpage parity.
    *   Sync flow supports both one-off option edits and quick recall of recent/pinned sync presets.
    *   Source and destination roles are explicit; this is one-way synchronization, not bidirectional merge logic.
    *   Deletion of destination-only files is opt-in and clearly confirmed.
    *   Unlogged-directory default-exclusion behavior is explicit and documented.
    *   The synchronize path prefers `rsync` for plain filesystem paths and does not require ytree to own a new recursive sync engine.
*   - [ ] **Status:** Not Started.

### **Task 61: Promote Applications Menu (`F9`) with Safe Default Presets**
*   **Goal:** Bring `F9` Applications Menu into current-delivery scope as a visible, contributor-friendly command surface with sensible default entries.
*   **Semantics:** Entries are user commands/templates (with optional placeholders/parameters) that execute commands; this is not keystroke recording.
*   **Default Presets (initial set):**
    *   `wget` fetch preset (option-heavy fetch with URL prompt).
    *   `ssh` connect-to-known-host preset.
    *   Format-convert preset (for example `ffmpeg` or `pandoc` template).
*   **Rationale:** Users need discoverable access to repeat-heavy external workflows that are not built-in file-manager actions.
*   **Acceptance Criteria:**
    *   `F9` exposes user-configurable entries and ships with documented default presets.
    *   Default presets prioritize non-builtin external workflows with clear input placeholders and safe defaults.
    *   User-added commands are not restricted, including commands that duplicate existing builtins.
    *   Prompt/help/F1/manpage text documents `F9` basics and preset intent in contributor-friendly language.
    *   Command completion reporting is explicit (`success` on zero exit, actionable error summary on non-zero exit).
*   - [ ] **Status:** Not Started.

### **Task 62: Define Extension Surface Contract (`F9` Apps + `F7` Preview Plugins)**
*   **Goal:** Define one explicit extension contract for external-tool integrations so command apps (`F9`) and preview plugins (`F7`) follow the same safety, UX, and fallback rules.
*   **Scope:** Contract/spec-only delivery for external execution surfaces (`X`, `P`, `W`, `FILEDIFF`, `F9`, and `F7` preview-helper boundary).
*   **Rationale:** ytree should reuse mature external tools without accumulating ad-hoc one-off behavior per feature.
*   **Acceptance Criteria:**
    *   Contract defines provider types (`app`, `preview`) and shared lifecycle semantics.
    *   Contract defines placeholder/token policy, argument safety rules, and bounded command construction.
    *   Contract defines deterministic completion/failure reporting and fallback behavior.
    *   Footer/F1/manpage wording aligns with the new contract language.
*   - [ ] **Status:** Not Started.

### **Task 63: Implement Shared Provider Registry (Plugin-Lite, External-Tool-First)**
*   **Goal:** Implement a shared provider registry/runtime for extension providers instead of isolated one-off paths.
*   **Non-Goal:** Do not add in-process arbitrary binary/plugin loading; providers remain external-tool adapters.
*   **Rationale:** A unified provider runtime keeps behavior predictable and lowers maintenance risk while preserving Unix-style composability.
*   **Acceptance Criteria:**
    *   Shared provider model supports at least `app` and `preview` provider classes.
    *   Common execution/safety controls are centralized (timeouts, output caps, exit-code mapping, fallback policy).
    *   Config/profile format is documented and validated with focused regression tests.
*   - [ ] **Status:** Not Started.

### **Task 64: Add Optional Background App Execution (`bg`)**
*   **Goal:** Allow selected external commands to run in background so users can continue navigating immediately.
*   **Entry Direction:** Prefer `F9` as the primary UX surface, with optional command-prompt parity where it fits cleanly.
*   **Scope Lock:** External commands/apps only (no async copy/move/delete queue in this task).
*   **Rationale:** This captures high-value "run and continue" workflow speed without requiring an embedded subshell model.
*   **Acceptance Criteria:**
    *   Users can launch an app in foreground or background using explicit UI choice/marker.
    *   Background job state is visible and queryable (running/success/failure) with actionable completion messaging.
    *   Failed background runs return clear diagnostics without destabilizing curses state.
*   - [ ] **Status:** Not Started.

### **Task 65: Implement F7 Preview Helper Pipeline (Promote Preview-Helper Pipeline into Current Delivery)**
*   **Goal:** Deliver the beta-scope F7 helper pipeline with strict fallback guarantees.
*   **Baseline Contract:** `BINARY` (internal preview, no helpers) and `RENDER` (helper-rendered output with guaranteed fallback to `BINARY` on failure).
*   **Scope Lock:** Ship the baseline safety/fallback pipeline now; defer optional advanced renderer ergonomics until later phases.
*   **Rationale:** This provides practical plugin-like preview extensibility while keeping ytree's internal preview as the reliability floor.
*   **Acceptance Criteria:**
    *   `F7` supports deterministic `BINARY` <-> `RENDER` mode toggling with stable footer labeling.
    *   Helper execution is bounded and safe (argv-first execution, timeout, output cap, failure fallback).
    *   Panel-local mode state is preserved in split mode.
    *   Config/docs/tests are synchronized for the delivered baseline behavior.
*   - [ ] **Status:** Not Started.

---

## **Phase 7: Internationalization and Configurability**
*   **Goal:** Refactor the application to support localization and user-defined keybindings, moving away from hardcoded English-centric values.

### **Task 66: Externalize UI Strings with GNU gettext (i18n Foundation)**
*   **Description:** Replace hardcoded user-facing strings with gettext-backed message lookups (`gettext`/`_()`), initialize locale/domain at startup, and add a standard catalog workflow (`.pot` -> `.po` -> compiled catalogs). Keep default locale as English while enabling translation packs.
*   **Documentation i18n split:** Use `po4a` for manpage/doc translation workflow (source: `etc/ytree.1.md`; generated docs stay derived artifacts). Use gettext for runtime UI surfaces (`F1`, footer labels/help, prompts, status/error/info text).
*   **Translation path policy:** Define default translation discovery paths for system and user installs (for example system locale catalogs under `/usr/share/locale/.../LC_MESSAGES/ytree.mo` with a user-level override path), and document contributor workflow for adding a language.
*   **Pilot locale:** Ship one non-English reference locale (for example German) as a contributor template proving end-to-end UI + manpage translation workflow.
*   **Rationale:** For C/POSIX terminal software, GNU gettext is the most conventional and broadly understood approach. It has mature tooling, standard translator workflow, and broad ecosystem familiarity; a custom loadable language-file system would add avoidable maintenance and onboarding cost.
*   - [ ] **Status:** Not Started.

### **Task 67: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from a separate keymap profile file. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Sequencing dependency:** Implement after Task 35 (Ctrl-held footer signaling + footer wording cleanup). Prefer completing Task 36 parity gate first so keymap work lands on a stable footer/F1 contract.
*   **Config contract:** Select profile via `ytree.conf` (opt-in), keeping a stable default keymap for existing users.
*   **Display contract:** Footer/help text must render active key + localized command label together (for example active binding `C` + translated `Copy` -> `(C)opy`) so runtime hints always match active bindings.
*   **Canonicalization/validation contract:** Normalize terminal byte aliases during keymap load (`^M`=`Enter`/`CR`, `^J`=`LF`/newline enter path, `^I`=`Tab`, `^[`=`Esc`) and reject profiles that map alias-equivalent inputs to different commands. Alias-equivalent inputs mapping to the same command are valid.
*   **Portability fallback contract:** Require workflow-level fallback for core actions (reachable without fragile terminal-specific modifiers). This is not a per-key duplication mandate; tagged/single-item variants may share menu/mode-driven paths when direct keyspace is exhausted.
*   **Behavior stability contract:** Default shipping keymap remains portable and stable; custom overrides are opt-in and must pass collision/unbound-action validation before activation.
*   - [ ] **Status:** Not Started.

---

## **Phase 8: Final Polish (Post-Alpha, Pre-v3.0.0)**
*This phase focuses on release polish. Security, module-boundary, and quality gates remain continuous from earlier phases and are not deferred to this phase.*

### **Task 68: UI/UX Snappiness Polish (Targeted Optimization)**
*   **Goal:** Improve perceived responsiveness in high-frequency flows using profiling-driven optimizations.
*   **Rationale:** Premature optimization is avoided; final polish applies targeted improvements where bottlenecks are measured.
*   - [ ] **Status:** Not Started.

### **Task 69: Source Comment Hygiene Pass**
*   **Goal:** Tidy comments for clarity and maintainability before v3.0.0.
*   **Policy:** Keep comments for invariants and design rationale; remove redundant narration of obvious control flow.
*   **Check:** Verify banner comments are only used where they add design/invariant context.
*   **Targeted K/A/R audit list (banner comments):**
*   **K (Keep):** Top-of-file purpose banners in `src/ui/display_utils.c`, `src/ui/stats.c`, `src/ui/ctrl_file_ops.c`, `src/fs/freesp.c`, and `src/fs/tree_read.c`.
*   **A (Add):** Missing top-of-file purpose banners in `src/ui/archive_payload.c` and `src/core/sort.c`.
*   **R (Remove):** Purely decorative in-body separator banners in `src/ui/display_utils.c`, `src/ui/ctrl_file_ops.c`, `src/fs/freesp.c`, and `src/fs/tree_read.c` when they do not state design/invariant context.
*   **Excluded:** Do not modify third-party `uthash.h`.
*   - [ ] **Status:** Not Started.

### **Task 70: Final Consistency Sweep (Style, Docs, UX Wording)**
*   **Goal:** Run a final consistency pass across style-sensitive surfaces (code style guardrails, docs wording, and help/footer terminology).
*   **Rationale:** Multi-contributor consistency is enforced continuously via guardrails and review; this task is a final convergence pass.
*   - [ ] **Status:** Not Started.

### **Task 71: Multi-Round Adversarial Security Review**
*   **Goal:** Perform a pre-v3.0.0 multi-round security review using adversarial and AppSec perspectives.
*   **Examples:** Senior AppSec reviewer, penetration-tester mindset, and insider-knowledge threat modeling.
*   **Rationale:** Final pre-release pressure test on top of continuous Phase 2 security gates.
*   - [ ] **Status:** Not Started.

---

## **Beta: Stabilization and Performance**
*This phase follows alpha delivery phases and precedes wishlist work. Place stabilization tasks here: bug fixes, regressions, reliability, and performance. Defer non-essential feature work to wishlist phases.*

---

## **Future Enhancements / Wishlist**
*Ideas that are not planned for inclusion at this stage. They are worth keeping a record of so they are not lost, but there is no promise or obligation to ever implement them. If you would like to take one on later, you are very welcome to do so.*
*IDs in this section use `Idea FE-*` and are explicitly non-priority/non-commitment markers.*

### **Future Phase 1: Post-Baseline Configurability Follow-On**

### **Idea FE-1: Explicit Accessibility Mode (Screen-Reader-First Terminal Behavior)**
*   **Goal:** Introduce an opt-in explicit accessibility mode focused on stable, low-noise behavior for screen-reader workflows.
*   **Research Gate (Required Before Implementation):**
    *   Audit current redraw/cursor-update hotspots (clock, spinner, status-line, dialogs, preview loops) for assistive-tech impact.
    *   Validate behavior with real screen-reader workflows (e.g., Speakup/NVDA terminal usage patterns) before locking UX contracts.
    *   Define measurable acceptance criteria (reduced cursor churn, reduced unsolicited announcements, no input-lag regressions).
*   **Implementation Direction (Post-Research):**
    *   Add a dedicated runtime/config toggle (not ad-hoc flags).
    *   Suppress/de-rate non-essential dynamic redraws in accessibility mode (e.g., spinners/timers) and prefer deterministic refresh cadence.
    *   Favor linear, prompt/result interaction paths where feasible; keep existing default behavior unchanged when mode is off.
*   **Rationale:** Terminal UI is not automatically accessible; explicit mode-level contracts are needed to avoid redraw/cursor noise regressions.
*   - [ ] **Status:** Not Started.

### **Idea FE-2: Portable Keyboard Capability Probe + `.ytree` Key Workarounds**
*   **Goal:** Add startup-time terminal key-capability probing and user-configurable key overrides/workarounds in `~/.ytree`.
*   **Behavior Direction:**
    *   Probe optional key availability once at startup (cache results; no per-keystroke probing overhead).
    *   Add explicit config overrides for problematic terminals/layouts so users can remap missing/ambiguous keys without code changes.
    *   Keep the default keymap stable and portable, with overrides as opt-in compatibility tools.
*   **Rationale:** Improves old-terminal portability while keeping runtime input handling fast.
*   - [ ] **Status:** Not Started.

### **Idea FE-3: Keymap Follow-On Work (Post-Baseline)**
*   **Description:** Follow-up keymap work after baseline keymap support lands (preset profiles, conflict diagnostics, import/export format hardening, and migration notes for existing users).
*   **Localized keymap profiles:** Add opt-in locale-oriented profiles as separate keymap files (not automatic locale remapping), while keeping the default keymap stable.
*   **Best-practice guardrails:** Preserve a universal core of stable bindings (function keys/Ctrl/digits/arrows), allow locale mnemonic aliases where safe, and enforce strict collision/unbound-action validation with clear diagnostics.
*   - [ ] **Status:** Not Started.

### **Future Phase 2: UI/UX Enhancements and Cleanup**

### **Idea FE-4: Configurable VCS Provider for `0` FileInfo Band**
*   **Goal:** Keep `0` as one stable VCS info band while allowing users to choose which backend powers it.
*   **Config Direction (`ytree.conf`):** Add a single-provider selector (for example `VCS_PROVIDER=off|git|hg|svn|fossil|auto`).
*   **Behavior Contract:**
    *   Only one VCS provider is active at a time for `0`; no mixed multi-provider rendering in one view.
    *   Default remains off for performance/noise control.
    *   If the selected provider is unavailable in the current path/repo, `0` performs a silent no-op.
*   **Rationale:** Preserves key stability and avoids renumbering while keeping a path open for non-Git users.
*   - [ ] **Status:** Not Started.

### **Idea FE-5: Typed Filter Modes (`glob` default, `re:`, `fz:`)**
*   **Goal:** Extend file filtering with explicit typed terms while preserving today's glob-first behavior and key flow.
*   **User-Facing Behavior:**
    *   Keep existing glob syntax as default (`*.c`, `*.c,*.h`, `-*.tmp`).
    *   Add typed terms:
        *   `re:<expr>` for POSIX ERE regex.
        *   `fz:<text>` for simple fuzzy subsequence matching (case-insensitive).
        *   `glob:<pattern>` as explicit glob alias (optional but accepted).
    *   Keep exclusion semantics explicit and deterministic: exclusion matches always win.
    *   Matching target remains basename (`fe->name`) to preserve current expectations.
*   **Parsing/Validation Direction:**
    *   Support quoted terms so commas can be used inside a term (for example `re:"^x{1,3}$",*.c`).
    *   Treat malformed specs as invalid (for example `,,`, trailing comma, unmatched quote, empty `re:`/`fz:`/`glob:` term, or bare `-` term).
*   **UX/Help Direction:**
    *   Keep `FILTER:` prompt flow unchanged (`key -> Enter -> result`).
    *   Add lightweight inline hint text only (for example `glob(default) | re: | fz:`), without using `?` (reserved for backward search).
    *   Put full syntax/examples in `F1` help and manpage source (`etc/ytree.1.md`).
*   **Rationale:** Adds regex/fuzzy power in a Unix-style, scriptable format without breaking existing wildcard workflows or adding submenu friction.
*   - [ ] **Status:** Not Started.

### **Idea FE-6: Prompt Input Decode Hardening (curses-first, legacy ESC fallback)**
*   **Goal:** Replace prompt-path manual ESC sequence parsing with curses/terminfo-first decoding, while keeping legacy manual ESC parsing as controlled fallback (or config-gated compatibility mode).
*   **Rationale:** Reduces xterm-specific assumptions in prompt entry and improves cross-terminal correctness on older UNIX environments.
*   - [ ] **Status:** Not Started.

### **Idea FE-7: Input Portability Regression Matrix (`TERM`)**
*   **Goal:** Expand UI regression coverage with a terminal-profile matrix and action-level assertions for keyboard behavior.
*   **Initial Matrix Target:** `xterm`, `vt100`, `screen`, `tmux`, `linux`.
*   **Rationale:** Existing UI tests prove behavior well in xterm-like sequences, but matrix runs provide stronger evidence for old/variant terminal compatibility.
*   - [ ] **Status:** Not Started.

### **Idea FE-8: Extended `sYsinfo` in Directory-Window Mode**
*   **Goal:** Add an on-demand extended stats/system-info surface (`sYsinfo`) for directory-window workflows without replacing the default compact stats panel.
*   **Rationale:** Advanced disk/system context is useful for planning operations, but should stay opt-in to avoid clutter in normal navigation.
*   **Keybinding Direction:** Keep context-specific `Y` behavior collision-free: directory-window `Y` may expose `sYsinfo`; file-window `Y` may expose sync workflow entry.
*   **Scope Lock:** Extended stats/sysinfo rendering and help/footer discoverability only; no copy/sync semantic redesign in this task.
*   **Acceptance Criteria:**
*   Extended stats view is reachable from directory mode and visually distinct from default stats.
*   Footer/F1/manpage wording explicitly documents context split where `Y` differs by mode.
*   - [ ] **Status:** Not Started.

### **Idea FE-9: Implement In-App Configuration Editor (F10)**
*   **Goal:** Implement a user-friendly configuration editor (activated by `F10`) that supports guided editing for common options in `~/.ytree` (e.g., `CONFIRMQUIT`, colors), while retaining an expert raw-text path.
*   **Rationale:** Reduces configuration friction for most users without removing power-user flexibility.
*   - [ ] **Status:** Not Started.

### **Idea FE-10: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** In capable terminal environments, mouse support can improve speed and ease of use for navigation and selection without changing the keyboard-first design.
*   - [ ] **Status:** Not Started.

### **Idea FE-11: Configurable Split Header Path Display (`active` or `both`)**
*   **Goal:** Add a user option for split-mode header path display so users can choose active-panel-only path or both-panel paths.
*   **Rationale:** Active-only header is cleaner by default, while dual-path header can improve orientation for users managing two distant locations.
*   **Scope Lock:** Header display policy only; no split navigation, selection, or command behavior changes.
*   **Acceptance Criteria:**
*   Default mode remains `active` (current behavior).
*   Optional mode `both` renders left/right panel paths in split mode with deterministic truncation/clipping and no wrapping.
*   Active panel remains visually obvious in both modes.
*   Footer/F1 help and config docs are updated when option lands.
*   - [ ] **Status:** Not Started.

### **Idea FE-12: Prompt Path Entry, Shell-Style Completion, and ncurses-Native Input Editing**
*   **Goal:** Replace the current history-biased prompt input with a first-class path-entry workflow that is good enough for deep navigation, destination entry, and command prompts.
*   **Scope:** This task subsumes the previous separate ideas for shell-style tab completion, deep path jump, and advanced ncurses-native command-line editing.
*   **Behavior to Deliver:**
    *   **Shell-style completion:** `Tab` completes file and directory names in prompts instead of only recalling history.
    *   **Deep path entry and navigation:** Users can type or complete absolute paths, relative paths, and archive paths directly in a prompt (for example `/mnt/backups/../daily/2026-04-11/archive.tar.gz`) and jump there without changing `/` list-jump semantics.
    *   **Rich inline editing:** Full cursor movement (left/right, home/end, word-by-word), insert/delete/backspace, clear-to-start/end, and persistent prompt history accessible via arrow keys.
    *   **Prompt reuse:** The same editing/completion behavior should apply consistently to Log, Copy, Move, Rename, Filter, and command-entry prompts.
*   **Rationale:** Prompt entry should be strong enough that common path-based workflows stay direct: "type path -> complete/adjust -> Enter -> result" without forcing a separate browser/menu detour.
*   - [ ] **Status:** Not Started.

### **Idea FE-13: Tagged-Only Results View**
*   **Goal:** Add a view mode that shows only tagged files without altering the tag set itself.
*   **User-Facing Behavior:**
    *   `F4` toggles **Tagged-Only** view mode.
    *   In file lists, Showall, Global, and archive file views, users can toggle a **Tagged-Only** filter to temporarily narrow the visible list to currently tagged items.
    *   Leaving the mode restores the normal file/filter view; tags remain unchanged.
    *   This should compose cleanly with existing filters, grep-on-tagged workflows, and compare/tag workflows.
*   **Rationale:** After tagging, compare, or grep operations, users often want a focused "show me only the files I marked" result view instead of manually navigating through the full list.
*   - [ ] **Status:** In Progress (tagged-only toggle shipped on `o/O`; broader workflow/key-shape refinements remain).

### **Idea FE-14: Investigate Recursive Tagging vs Existing Showall/Global Workflow**
*   **Goal:** Determine whether recursive tagging provides enough real workflow benefit over the current `log dir -> Showall/Global -> tag` path to justify added complexity.
*   **Rationale:** Recursive tagging may reduce steps in some trees, but can also add command ambiguity and accidental broad-selection risk.
*   **Investigation Output:** Document concrete user workflows, interaction-depth impact, and safety tradeoffs; propose either (a) no change, or (b) a minimal, default-safe recursive tagging design with clear scope/confirmation semantics.
*   - [ ] **Status:** Not Started.

### **Idea FE-15: Richer Compare Result Views**
*   **Goal:** Extend compare workflows so the result can be viewed directly, not just turned into tags on the active side.
*   **User-Facing Behavior:**
    *   After comparing two directories/trees, users can narrow the result to categories such as **left/source only**, **right/target only**, **newer**, **older**, **size different**, **content different**, or **identical**.
    *   The result should be explorable as a list/view mode, not only as tag side effects.
    *   Compare output should stay explicit about which side is being shown and why an entry appears.
*   **Rationale:** Current compare behavior is useful but blunt. A richer result view makes compare a practical review tool rather than only a tag generator.
*   - [ ] **Status:** Not Started.

### **Idea FE-16: Recent-Directory Bookmarks and Pinned Favorites**
*   **Goal:** Add a first-class recent-directory and pinned-favorites picker for fast return to commonly visited locations.
*   **User-Facing Behavior:**
    *   Show a compact list of recently visited directories together with user-pinned favorites.
    *   Selecting an entry should log or activate that directory directly.
    *   Reuse existing log/history persistence where practical, but present this as a navigation surface, not merely raw prompt history text.
    *   Prefer a portable key or prompt hook (for example `F3`) over browser-style back/forward semantics unless a stronger need emerges later.
*   **Rationale:** Prompt history helps when the user remembers what they typed. A dedicated recent-directory/favorites list helps when the user remembers the place, not the exact command string.
*   - [ ] **Status:** Not Started.

### **Idea FE-17: Dual-Preview Split Mode**
*   **Goal:** Allow each `F8` split panel to enter and retain its own `F7`-style preview state independently.
*   **User-Facing Behavior:**
    *   In split mode, each panel can independently enter preview without forcing preview state changes in the other panel.
    *   Switching active panel preserves the preview/list state already held by each side.
    *   Entering or leaving preview on one panel must not unexpectedly reset scroll position, selection, or return-state on the other panel.
    *   The design must keep panel ownership obvious so users can still tell which side is active, which side is in preview, and what `Enter`/`Tab`/`F7` will affect next.
*   **Rationale:** Active-panel-only preview is useful, but independent per-panel preview would make split review/compare workflows more powerful for users who want to inspect both sides without repeatedly toggling state back and forth.
*   **Scope Lock:** This is an advanced split/preview state feature only. It does not require a broader orthodox-style layout redesign and should preserve ytree's existing xtree/unixtree/ztree-derived interaction style.
*   **Acceptance Criteria:**
*   Both panels can hold independent preview/list state without leaking state across panels.
*   `Tab`, `F7`, and return-to-list behavior are deterministic and documented in footer/F1/manpage text.
*   Split-panel active/inactive indicators remain unambiguous while one or both panels are in preview.
*   Focused regression coverage proves per-panel state retention, panel switching, and exit/return behavior.
*   - [ ] **Status:** Not Started.

### **Idea FE-18: Directory-Focus Small-File Peek Navigation (`Shift` + Nav Keys)**
*   **Goal:** In directory focus, allow `Shift+Up/Down/Page/Home/End` to scroll the small file window for the selected directory without switching to full file-window focus.
*   **Rationale:** This gives a fast "peek and keep tree focus" workflow and mirrors the existing `Shift`-navigation feel used in `F7` preview.
*   **Scope Lock:** Directory-focus small-file-window navigation only; no new submenu flow, no change to normal unshifted tree navigation, and no change to `F7` preview behavior.
*   **Acceptance Criteria:**
*   In directory focus with the small file window visible, `Shift+Up/Down/Page/Home/End` moves the small-window file selection/offset deterministically.
*   Common path remains direct (`Shift+key -> immediate movement`) with zero submenu depth.
*   `/` list-jump remains directory-window scoped in this mode (no hidden mode switch of jump target).
*   If there are no files in scope, shifted navigation is a silent no-op (no modal/beep).
*   Footer/F1/manpage text clearly documents where shifted navigation applies.
*   Add focused regression coverage for shifted small-window navigation bounds/offset behavior and isolation from directory navigation.
*   - [ ] **Status:** Not Started.

### **Idea FE-19: Unified `N Create` Entry Point (Capability-Filtered by Backend)**
*   **Goal:** Replace the narrow `NewFile` entry point with a single explicit `Create` chooser whose available options are filtered by the active backend and context.
*   **User-Facing Behavior:**
    *   Where creation is supported, `n`/`N` opens `Create:` with only the actions that are valid for the active backend/context.
    *   In local filesystem contexts, the chooser should expose `Create: [f]ile [d]irectory [s]ymlink`.
    *   Pressing `Enter` at the chooser defaults to `[f]ile` when file creation is available.
    *   `f` preserves the current empty-file creation flow.
    *   `d` opens directory creation from the same top-level entry point where directory creation is supported.
    *   `s` opens a native symlink flow where symlink creation is supported, rather than relying on shell escape commands.
    *   Keep `M` as a direct `Make Directory` alias for backward-compatible speed; `N Create` becomes the canonical discoverable path.
    *   In file view, if a selected entry makes the symlink target unambiguous, prefer a shallow flow that prompts only for the link name/path; otherwise prompt explicitly for target and link destination.
    *   On read-only backends (for example ISO-style browsing), `N Create` does not appear at all.
*   **Rationale:** Keybindings are scarce. A one-submenu `Create` chooser is more discoverable and contributor-friendly than adding another top-level key or hiding mode switches inside a `MAKE FILE:` prompt, and capability-filtering keeps backend differences explicit instead of misleading.
*   **Scope Lock:** This task defines the common `N Create` entry point and capability-filtered option exposure. It does not require identical create semantics across all current or future backends. Hard-link creation stays out of scope unless a later roadmap item proves enough demand to justify the extra constraints and error handling.
*   **Acceptance Criteria:**
*   Footer/F1/help/manpage wording uses `Create` for the `N` entry point rather than `NewFile`.
*   Where creation is supported, the common path remains one submenu deep: `N` -> choice -> prompt -> result.
*   In local filesystem contexts, `Enter` at the chooser behaves as `f` and preserves current file-creation semantics.
*   `M` still performs direct directory creation.
*   The chooser shows only backend-valid create options; unavailable create types are omitted rather than advertised and rejected later.
*   Read-only backends expose no `N Create` entry in footer/help.
*   Symlink creation is available natively where supported, with explicit prompts and focused regression coverage for both selected-target and explicit-target flows.
*   - [ ] **Status:** Not Started.

### **Idea FE-20: Per-Window Filter State (Split Screen Prerequisite)**
*   Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left panel versus `*.h` in the right).
*   - [ ] **Status:** Not Started.

### **Idea FE-21: State Preservation on Reload (`^L`)**
*   Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   - [ ] **Status:** Not Started.

### **Idea FE-22: Preserve Tree Expansion on Refresh**
*   Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-23: Scroll Bars**
*   On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list (configurable to char or line).
*   - [ ] **Status:** Not Started.

### **Idea FE-24: Callback API Constification Cleanup (cppcheck strict mode)**
*   `cppcheck` suggests const-qualifying callback `user_data`, but doing this correctly likely requires changing callback typedef/API signatures (e.g., `RewriteCallback`) and related call sites. Defer this to a focused API pass to avoid scattered casts and partial churn.
*   - [ ] **Status:** Not Started.

### **Future Phase 3: Long-Horizon Experiments**

### **Idea FE-25: Implement VFS Abstraction Layer** (Use the Architect persona here)
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytree` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"
*   **Follow-on Direction:** Include remote logging backends under this VFS model (FTP/SFTP candidates), with final protocol choice deferred until security and maintenance review.

### **Idea FE-26: Define VFS Interface & Volume Integration** (Use the Architect persona here)
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytree_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytree_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.

### **Idea FE-27: Implement VFS Registry** (Use the Architect persona here)
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.

### **Idea FE-28: Implement "Local" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/tree_read.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.

### **Idea FE-29: Implement "Archive" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/archive_read.c` and `src/fs/archive_write.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.

### **Idea FE-30: Switch `LogDisk` to VFS** (Use the Architect persona here)
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.

### **Idea FE-31: Refactor Consumers (Polymorphism)** (Use the Architect persona here)
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.

### **Idea FE-32: Database Browsing and Editing via Virtual Filesystem Drivers**
*   **Goal:** After the driver-based VFS abstraction exists, allow ytree to browse supported database formats as navigable virtual filesystems and eventually edit them through driver-defined operations.
*   **User-Facing Direction:** Treat a database as a structured volume (for example database -> tables -> rows/records or exported views) rather than as one opaque file blob.
*   **Rationale:** This is a specialized extension of the VFS model, not a core file-manager requirement. Keep it as a future experiment until a clear driver design and real use-case exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-33: Implement Recursive Directory Watching**
*   **Goal:** Keep visible tree and file-list state fresh by watching all currently expanded filesystem directories, not only the active cursor directory.
*   **Rationale:** Without recursive watch coverage, edits in visible sibling/child directories can leave the UI stale until manual refresh.
*   **Scope Lock:** Filesystem watcher behavior only; no archive-internal recursive watching.
*   **Mechanism:**
    *   In `watcher.c`, maintain a `wd -> DirEntry*` map (for example via `uthash`) so events can be routed to the correct tree node.
    *   On `ReadTree` (expand), add watch descriptors for newly expanded directories.
    *   On `UnReadTree` / `DeleteTree` (collapse/free), remove corresponding watches immediately.
    *   On watch-limit failure (`ENOSPC`), degrade gracefully to active-directory-only watch mode without crashing or UI corruption.
*   **Archive Boundary:** For archives, watch the container file timestamp and trigger virtual tree reload; do not add recursive in-archive watches.
*   **Acceptance Criteria:**
    *   Expanded dirs update automatically when changed externally.
    *   Collapsing/removing nodes cleans up watches deterministically (no fd/watch leaks).
    *   `ENOSPC` fallback is explicit, stable, and non-fatal.
*   - [ ] **Status:** Not Started.

### **Idea FE-34: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   - [ ] **Status:** Not Started.

### **Idea FE-35: Implement Keyboard Macros (F12 Record/Playback)**
*   **Goal:** Implement keystroke recording and replay. `F12` starts/stops recording command/input sequences for deterministic playback.
*   **Rationale:** Allows automation of repetitive interaction sequences (for example "Tag, Move, Rename, Repeat") without creating shell command templates.
*   - [ ] **Status:** Not Started.

### **Idea FE-36: Enhance Built-In Viewer**
*   **Goal:** Evolve ytree's internal viewer from a basic fallback inspector into a more capable built-in viewing tool for normal terminal workflows.
*   **Builds On:** Current-delivery viewer work such as `Add Configurable Bypass for External Viewers` and `Standardize Internal Viewer Layout`.
*   **Candidate Scope:**
    *   Stronger text viewing modes such as plain text, wrapped text, and hex/dump mode with consistent navigation.
    *   Better in-view search, jump-to-offset or jump-to-line behavior, and clearer file identity/status in the header/footer.
    *   Improved parity between single-file view, tagged-file view, and `F7` preview behavior where that makes sense.
    *   Optional lightweight conveniences such as line numbers, bookmarks, or simple gather/copy/export behavior if someone later proves the use-case.
*   **Non-Goal:** Do not turn ytree into a native all-format viewer for images, PDFs, office files, multimedia, or GUI-centric content. External helper programs remain the preferred Unix-style answer for those cases.
*   **Rationale:** A stronger built-in viewer would make ytree more self-contained for terminal inspection work, while still keeping the project focused on file management rather than format-specific rendering.
*   - [ ] **Status:** Not Started.


### **Idea FE-37: Terminal-Independent TUI Runtime (ncurses-Decoupling Investigation)**
*   **Goal:** Investigate a runtime path where ytree's TUI is not tightly coupled to ncurses.
*   **Rationale:** This is a platform/input architecture effort intended to evaluate whether backend decoupling can reduce current control-key handling constraints (including limitations around mappings like `^M`) while preserving ytree interaction semantics.
*   - [ ] **Status:** Not Started.

### **Idea FE-38: Implement "Safe Delete" (Trash Can)**
*   **Goal:** Add optional trash-backed delete where the active filesystem/backend supports it.
*   **Config:** Add a `ytree.conf` switch for trash-delete with default `1` (enabled).
*   **Fallback:** If trash-delete is disabled or unsupported for the active backend, use permanent delete with explicit confirmation.
*   - [ ] **Status:** Not Started.
