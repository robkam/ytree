# Archive Virtual Filesystem Parity

## Mission
Complete **Archive Virtual Filesystem Parity**: archives must behave as filesystem-like volumes according to runtime-probed capabilities.

## Selected coherent family
Runtime archive capability model and truthful UI/help projection. This is the highest-value prerequisite because the acceptance target requires every archive mutation path, footer, stats line, and contextual help to agree on the operations actually available from the opened archive and installed libarchive support.

## In-scope inventory
- Source of truth: `docs/ROADMAP.md` Task 13 acceptance/capability/move/directory/help contracts.
- Runtime archive support and mutations: `src/fs/archive_read.c`, `src/fs/archive_write.c`, `include/ytnova_fs.h`, `include/ytnova_cmd.h`.
- Copy/move and archive/directory call paths: `src/cmd/copy.c`, `src/cmd/move.c`, `src/cmd/mkdir.c`, `src/cmd/delete.c`, `src/cmd/rename.c`, `src/cmd/rmdir.c`.
- UI projection/dispatch: `src/ui/display.c`, `src/ui/stats.c`, `src/ui/ctrl_file_ops.c`, `src/ui/ctrl_file.c`, `src/ui/ctrl_dir.c`.
- Generated-help source and projections: `etc/help/f1.en.md`, `etc/help/f1.de.md`, generated help assets.
- Tests: `tests/test_archive_write_parity.py`, `tests/test_fileops_integrity.py`, archive/UI/help contract tests and focused TUI regressions.
- Safety seams: `Archive_ValidateInternalPath()`, archive rewrite finalization, collision and subtree/self-target checks, cross-archive source-removal ordering.

## Initial observations
- Existing archive copy/move/rewrite and canonical-path guards are present from earlier history.
- `docs/ROADMAP.md` remains Not Started; no matching audit handoff exists.
- Archive F1 text and static footer definitions currently present mutation actions without an apparent runtime capability projection. This is incompatible with the capability contract and requires exact call-path inspection before implementation.

## Closure status
- Inventory: active; no item reconciled yet.
- Deferred families: recursive directory transfer and cross-archive failure ordering remain in this mission but will be deferred from the capability/UI family only if their runtime contracts are already demonstrably complete or require materially different mutation validation.

## Capability-model progress
- Addressed: `Statistic` now records archive capability flags and archive loading probes the opened archive with libarchive reader/writer format and filter support rather than a filename extension.
- Validation: `make -j"$(nproc)"` passed; `pytest -q tests/test_archive_write_parity.py` passed (9 tests).
- Known gap: the capability flags are not yet enforced by mutation dispatch or projected into footer, stats, and contextual help; this remains the active family.
- Environment note: `make clean` cannot complete because a pre-existing root-owned `build/locale/de/LC_MESSAGES/ytnova.mo` cannot be removed by the current user. Incremental `make` passed.

## Recursive directory transfer progress
- Addressed: archive tree extraction, recursive archive insertion, canonical member validation, and preflight collision detection were added to the archive layer; archive-source directory copy/move dispatch performs destination replacement before source deletion.
- Addressed: directory Pathcopy is routed through the directory controller and archive mutation guards now cover directory add/delete/rename entry paths.
- Validation: `make -j"$(nproc)"` passed; `source .venv/bin/activate && pytest -q tests/test_archive_write_parity.py` passed (`9 passed`).
- Still required: durable layout-resilient TUI coverage for archive directory copy/pathcopy/move and capability UI projection; do not mark the roadmap complete yet.

## Reconciled progress
- Addressed: runtime archive capabilities are probed from libarchive, enforced in archive mutators and file/directory dispatch, filtered from archive footer/F1 labels, and projected in stats.
- Addressed: archive directory Copy, Pathcopy, and Move recursively transfer to filesystem destinations; Move deletes the source only after destination transfer succeeds. Archive tree deletion canonicalizes virtual absolute paths before rewrite.
- Addressed: canonical internal paths, traversal rejection, self/subtree rejection, and destination collision preflight are implemented in archive transfer layers.
- Addressed: authored archive help and generated projections state runtime-dependent capability availability and expected writable formats.
- Validation: `make -j"$(nproc)"`; `pytest -q tests/test_archive_ui.py::test_archive_directory_copy_recursively_preserves_source tests/test_archive_ui.py::test_archive_directory_move_to_filesystem_removes_source tests/test_archive_write_parity.py tests/test_help_source_schema.py` (`21 passed`); `pytest -q tests/test_archive_ui.py::test_archive_directory_pathcopy_recursively_preserves_source` (`1 passed`).
- Remaining/unproven: direct archive-to-archive recursive directory transfer, read-only capability UI rejection test, and injected cross-archive source-side deletion failure preservation. These acceptance surfaces must be covered before roadmap closure.
- Cross-archive runtime probe: a linked `ExtractArchiveTree` → `Archive_AddTree` → `Archive_DeleteTree` sequence returned `0`; source became empty and destination retained `keep.txt` plus `copied_bundle/nested/item.txt`. This proves destination replacement precedes source deletion for the recursive archive primitives.
- Cross-archive failure probe: a destination canonical collision made `Archive_AddTree` fail; the operation returned `0` from the expected-failure harness and the source still contained `bundle/item.txt`. This proves destination failure does not alter the source.


## Completed verification family
Durable acceptance coverage for archive-to-archive directory transfers and failure-preserving moves.

## Remaining inventory
- `docs/ROADMAP.md`: addressed; completion restored after durable proof of every listed acceptance criterion.
- `tests/test_archive_write_parity.py`: addressed; logged archive destination Copy, PathCopy, and Move verify recursive members, source retention/removal, and collision preservation.
- `tests/test_archive_ui.py`: addressed; read-only archive Move is invoked and displays the unsupported directory-transfer rejection while the footer remains truthful.
- `tests/test_archive_write_parity.py`: addressed; a permission-injected source rewrite failure after destination write proves both archives retain the transferred directory.
- `src/ui/ctrl_dir.c`: addressed; archive directory dispatch rejects unavailable transfer capability before root-directory handling can mask the capability error.
- Existing archive transfer/capability runtime paths: intentionally unchanged; the new durable coverage did not expose further defects.

## Closure reconciliation
- Addressed: runtime capabilities, operation/dispatch guards, footer/help/stat projection, filesystem and archive destination directory transfers, canonical collision/traversal/subtree guards, and generated help.
- Addressed: archive-to-archive Copy, PathCopy, and Move now have recursive durable coverage; collision leaves both archives intact; read-only invocation produces a clear rejection; and injected source deletion failure retains duplicate source/destination data.
- Validation: `make -j"$(nproc)"` and `make qa-code-quality` passed; `source .venv/bin/activate && pytest -q tests/test_archive_write_parity.py::test_archive_directory_transfer_matrix_vfs_to_vfs tests/test_archive_write_parity.py::test_archive_directory_transfer_rejects_destination_collision tests/test_archive_write_parity.py::test_archive_directory_move_preserves_source_when_source_delete_fails tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move` passed (`6 passed`); `pytest -q tests/test_archive_backend.py` passed (`3 passed`).
- Deferred: none.

## CI remediation
- Root cause: capability filtering can remove navigation commands, but
  `RenderFooterNavRow()` continued packing and inspecting the original command
  count, reading uninitialized command slots.
- Addressed: `RenderFooterNavRow()` now uses the filtered count returned by
  `ResolveFooterCommandList()`.
- Validation: `make -j"$(nproc)"`, `clang-tidy src/ui/display.c -p .`, and
  `source .venv/bin/activate && pytest -q tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move` passed. The canonical local
  `make qa-clang` could not run because its mandatory clean step hit the known
  root-owned `build/locale/de/LC_MESSAGES/ytnova.mo` artifact; it removed the
  normal build outputs before failing, then the incremental build restored them.

## Static analyzer follow-up
- Root cause: cppcheck reported narrow const-correctness and variable-scope
  findings in the new archive transfer paths after the footer read was fixed.
- Addressed: archive mutator inputs and read-only rewrite contexts are const,
  archive traversal entries are read-only, and temporary path buffers now have
  their smallest valid scope.
- Validation: `make -j"$(nproc)"`; `source .venv/bin/activate && pytest -q
  tests/test_archive_write_parity.py
  tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move`
  passed (`15 passed`). The detached focused cppcheck matrix for
  `src/ui/ctrl_dir.c` passed (`0`).

## Dual-panel footer isolation
- Selected defect family: active-panel archive footer/help context leaks from
  global `ViewContext.view_mode` when split panels hold filesystem and archive
  volumes.
- In-scope inventory: `docs/ROADMAP.md` status; footer context, command
  presentation, capability filtering, and integrated-help selection in
  `src/ui/display.c`; active panel volume state via
  `ctx->active->vol->vol_stats.log_mode`; focused PTY coverage in
  `tests/test_archive_ui.py`.
- Closure status: active. The regression must prove F8/Tab changes the footer
  from archive to filesystem context and back without changing either panel's
  volume.

## Dual-panel footer isolation closure
- Addressed: footer command sets, footer command presentations, capability
  filtering, and generated F1 context now derive archive state from the active
  panel volume rather than global view mode.
- Addressed: `tests/test_archive_ui.py` creates split filesystem/archive
  volumes and switches focus in both directions, asserting the corresponding
  filesystem and archive footer headers without layout coordinates.
- Validation: the regression first failed with an `ARCHIVE` footer after
  switching to the filesystem panel. `make -j"$(nproc)"`; `pytest -q
  tests/test_archive_ui.py` (`24 passed`); and `pytest -q
  tests/test_help_text_contract.py` (`7 passed`) now pass.
- Closure reconciliation: all Task 13 acceptance surfaces remain addressed;
  the dual-panel footer/help projection was the only reopened surface.

## Filesystem-to-archive directory transfer
- Reopened family: filesystem directory Copy, PathCopy, and Move into logged
  archive destinations; collision must preserve the filesystem source.
- Inventory: `HandleDirCopyMove`, archive destination routing in
  `archive_transfer.c`, `Archive_AddTree` collision semantics, filesystem
  source deletion ordering, and focused PTY regression coverage.
- Addressed: destination archive routing now runs before filesystem-only
  source/destination guards, resolves the logged target volume, and calls
  `Archive_AddTree` with the canonical archive-relative destination.
- Addressed: filesystem source removal is performed only after `Archive_AddTree`
  succeeds; collision failure leaves the filesystem source unchanged.
- Addressed: `tests/test_archive_ui.py` covers filesystem-directory Copy,
  PathCopy, and Move into a logged archive with nested contents, plus a Move
  collision preserving both the source and existing archive member.
- Validation: `make -j"$(nproc)"`; `source .venv/bin/activate && pytest -q
  tests/test_archive_ui.py::test_filesystem_directory_transfer_to_logged_archive
  tests/test_archive_ui.py::test_filesystem_directory_move_to_logged_archive_collision_preserves_source`
  (`4 passed`).
- Closure status: active; ROADMAP remains In Progress and no commit or push has
  been made pending the requested wider Task 13 reconciliation.

## Static analyzer reconciliation
- The reported `src/ui/display.c:1649 [knownConditionTrueFalse]` does not
  reproduce on base `b5dbe707`: that line is the unrelated filter fallback
  there. It was introduced by the archive-footer help refactor on the PR head.
- Addressed: simplified the logically redundant global-file help branch in
  `src/ui/display.c`; focused cppcheck on that file now exits zero with no
  `knownConditionTrueFalse` diagnostic.
- Validation: `cppcheck --enable=all --inconclusive --force --std=c99 -I
  include src/ui/display.c` (`0`); `make -j"$(nproc)"`; focused archive UI
  selection, transfer, and collision cases (`5 passed`). No push was made.
- CI remediation: CodeQL identified a TOCTOU race in filesystem-source removal.
  Directory removal now uses parent-directory descriptors with `openat`,
  `O_NOFOLLOW`, and `unlinkat`, so archive Move cannot follow a swapped
  filesystem path after archive insertion.
- Validation: `make -j"$(nproc)"`; focused filesystem-directory archive
  transfer and collision regressions (`4 passed`).

## Corrective audit inventory (active)

- **Manual reproducer / archive directory Delete:** `DeleteDirectory()` calls `Archive_DeleteEntry()` for logged archive directories; `Archive_DeleteTree()` exists but is bypassed. Add a nested-directory PTY red regression, then route archive directory deletion through the tree mutator. **Active.**
- **Archive mutation matrix:** archive file Delete (`Archive_DeleteEntry`), directory Delete (`Archive_DeleteTree`), file/directory Rename (`Archive_RenameEntry` and directory dispatch), and Copy/PathCopy/Move endpoint routing (`copy.c`, `move.c`, `ctrl_dir.c`, `archive_transfer.c`) require a call-path/guard review. Capability and canonical/collision/self/subtree guards are included; move ordering must retain source on destination or later source deletion failure. **Active audit.**
- **Capabilities / projections:** runtime probe, mutator rejection, dispatch rejection, footer command filtering, stats, and contextual F1 in `archive_read.c`, `archive_write.c`, `ctrl_file.c`, `ctrl_dir.c`, `display.c`, `stats.c`, and authored/generated help. **Active audit.**
- **Dual-panel F8/Tab / F1 matrix:** filesystem/archive file and directory active states; archive root/subdirectory; writable/read-only states; footer, dispatch, and help must follow `ctx->active->vol->vol_stats.log_mode` only. Existing focused UI regressions are included for confirmation. **Active audit.**
- **Tests and durable artifacts:** `tests/test_archive_ui.py`, `tests/test_archive_write_parity.py`, `tests/test_archive_backend.py`, help tests, and requested `scripts/bugrec.sh`. **Active audit.**
- **Tracker and handoff:** `docs/ROADMAP.md` remains In Progress until the inventory is reconciled; this handoff is the live record. **Active.**

## Corrective audit reconciliation

- **Manual archive directory Delete:** addressed. `DeleteDirectory()` now uses recursive `Archive_DeleteTree()` for archive directories, removes the matching in-memory subtree only after rewrite success, and decrements archive directory stats by the deleted subtree size. The nested-directory PTY regression was red before the route changed and is green after.
- **Archive mutation matrix:** addressed/unchanged after call-path audit. File Delete retains `Archive_DeleteEntry`; directory Delete now uses `Archive_DeleteTree`; Rename routes through canonical archive rename; Copy/PathCopy/Move endpoint transfer, preflight, capability guards, and destination-before-source removal remain in dedicated transfer layers and already have focused coverage. No additional root cause found.
- **Capabilities / projections:** addressed. Active-panel transition now synchronizes the legacy compatibility `ctx->view_mode` from the newly active volume, preventing dispatch from stale archive mode. Runtime help now receives all planned archive command labels and omits unavailable commands rather than merely retaining unused override metadata. Footer/stats/probe/mutator guards are intentionally unchanged after audit.
- **Dual-panel F8/Tab / F1:** addressed for the found boundary defects: filesystem footer and Makedir dispatch work after Tab from an archive panel; read-only archive F1 omits unavailable directory mutation descriptions. Existing active-volume footer transition coverage remains valid. No inactive-panel footer leak found.
- **Tests and durable artifacts:** addressed. `tests/test_archive_ui.py` now protects recursive Delete, filesystem dispatch after Tab, and read-only F1 filtering. Requested `scripts/bugrec.sh` is included unchanged.
- **Tracker and handoff:** at this earlier corrective-audit checkpoint,
  `docs/ROADMAP.md` remained In Progress. The final closure below supersedes
  that interim state after the added regressions and replacement CI run.

## Archive mutation refresh, progress, and root projection inventory (active)

- **Progress lifecycle:** archive directory Copy, PathCopy, and Move through
  `FilesystemDirectoryTransferToArchive`, `ArchiveDirectoryTransfer`,
  `Archive_AddTree`, `Archive_AddTreeRecursive`, `Archive_AddFile`, and
  `Archive_Rewrite`; callback cadence, an immediate pre-blocking render, footer
  restoration, and writable/read-only rejection are included. **Active audit.**
- **Destination refresh/rebind:** successful filesystem-to-archive recursive
  mutations must reload the destination `Volume` and rebind every panel that
  displays it, while the source panel refresh and Move selection semantics stay
  correct. Logged inactive and active archive destinations and Tab/F8 switching
  are included. **Active audit.**
- **Archive-root projection:** `ReadTreeFromArchive`, `InsertArchiveDirEntry`,
  and `MinimizeArchiveTree` must preserve the archive container as the tree root.
  Root files, one top-level folder, multiple top-level folders, nested
  sole-child chains, and explicit versus implicit directory entries are
  included; member paths remain canonical without changing filesystem tree
  minimization. **Active audit.**
- **Focused proof:** add red-first, layout-resilient regressions for immediate
  visibility of a recursively copied directory, visible pre-blocking progress,
  and a single top-level archive directory remaining below the archive root.
  Existing archive backend/write/UI suites and compatibility seams are included
  for focused confirmation. **Active.**
- **Tracker/handoff:** the roadmap status remains In Progress and this live
  inventory must reconcile every item as addressed, intentionally unchanged
  with reason, or separately deferred/blocked before amend/push. **Active.**

## Archive mutation refresh, progress, and root projection reconciliation

- **Progress lifecycle — addressed:** recursive directory Copy, PathCopy, and
  Move now render `ARCHIVE COPY`/`ARCHIVE MOVE` plus the spinner before entering
  blocking archive work, advance the existing progress surface from archive
  callbacks, and finish it on every post-start exit. Capability rejection stays
  before progress startup. File Delete, Makedir, Rename, and directory Delete are
  intentionally unchanged because their existing mutation paths already draw a
  pre-operation spinner and were covered by the earlier audit.
- **Destination/source refresh and dual-panel rebind — addressed:** successful
  filesystem-to-archive and archive-to-archive directory mutations reload the
  already logged archive volume, preserve its filter/sort and panel viewport
  anchors, rebuild file projections, and rebind every panel displaying it.
  Archive Move reloads a separately mutated source volume as well. Filesystem
  source removal remains after destination success and refresh; archive source
  deletion remains after destination success, so a later failure retains the
  destination duplicate. Focused tests cover Copy, PathCopy, and Move plus an
  inactive archive panel followed by Tab.
- **Archive-root projection — addressed:** archive tree minimization was removed
  from `ReadTreeFromArchive`, retaining the archive container root and each
  member-directory level for root files, one or multiple top-level directories,
  nested sole-child chains, and explicit or implicit directory entries.
  Filesystem tree minimization and canonical archive member paths are
  intentionally unchanged.
- **Adjacent capability/path contracts — intentionally unchanged:** runtime
  capability discovery, footer/F1 filtering, operation-layer rejection,
  traversal/collision/self/subtree checks, and file mutation routing have no new
  root cause in this family and retain the earlier focused coverage. The only
  compatibility correction is `RefreshTreeSafe` consulting the panel volume's
  archive mode rather than global view mode.
- **Focused proof — addressed:** red-first PTY failures covered the sole-folder
  root collapse, stale logged destination for all three directory transfer
  commands, and absent pre-blocking progress. Archive-to-archive destination
  visibility and inactive-panel refresh are also explicit. Build, focused
  cppcheck, 49 archive tests, module-boundary/AppState guards, and the refreshed
  test-contract baseline are green. The first amended CI run exposed three
  legacy tests that assumed the removed archive-root collapse; those tests now
  navigate deliberately to the member directory and their exact focused run is
  green (3 passed), without weakening their footer, split-panel, or viewer
  return contracts.
- **Tracker/docs — addressed:** `docs/ROADMAP.md` is restored to Completed after
  the amended required checks passed. No authored help contract changed, so
  help assets were deliberately not regenerated.

## Final closure

- The first corrective CI run identified three compatibility tests that still
  assumed a collapsed archive root. They now select the archive member
  directory explicitly; focused validation is 3 passed and the replacement CI
  run is green across every required check.
- The bounded archive mutation, capability, F8/Tab/F1, refresh, progress, and
  root-projection inventories are reconciled with no blocked or separately
  deferred defect family. Existing file-mutation spinner paths and authored help
  are the only deliberately unchanged adjacent surfaces, for the reasons above.
- `docs/ROADMAP.md` is restored to Completed now that the corrective audit,
  concrete regressions, local proof, and required PR checks are reconciled.
  This final tracker-only amend must itself retain green required checks before
  review or merge. `scripts/bugrec.sh` remains included in the amended commit.

## Archive rewrite cadence and batching inventory (reconciled locally)

- **New recording evidence:**
  `ytnova-20260907-005000-790902-zLDa8n.cast` renders `ARCHIVE COPY`
  immediately at 26.6 seconds but emits no spinner update for about 58 seconds;
  refresh, recursive Delete, and container-root projection behave correctly.
- **Collision preflight:** `ArchiveTreeDestinationAvailable` recursively opens
  and scans the complete archive once for every filesystem member. This is an
  O(source members × archive members) silent preflight and is the first long
  pause in the recording. **Address in the archive-write owner boundary.**
- **Directory insertion:** `Archive_AddTreeRecursive` calls `Archive_AddFile`
  per member, rewriting and replacing the complete archive each time. Replace
  this with one collision-aware rewrite followed by recursive append to the
  same writer, preserving atomic replacement and all-or-nothing source
  retention. **Address in the same family.**
- **Progress cadence:** `process_rewrite_loop` reports only every 50 headers and
  reports nothing while copying a large entry or writing a new entry. Emit
  progress from header and bounded data-copy units; rate-limit terminal redraws
  in the UI layer so callback density cannot dominate transfer time. **Address.**
- **Compatibility seams:** preserve single-file replace semantics, Makedir's
  source-less virtual directory insertion, recursive collision rejection,
  Rename data streaming, archive format/metadata preservation, cancellation,
  destination-before-source deletion, logged-volume reload, and dual-panel
  rebind. **Audit with focused backend/write/UI suites.**
- **Red proof:** the focused backend callback contract returned zero progress
  events while rewriting a one-entry archive before implementation. It must
  report progress during the rewrite and preserve the old member plus the
  recursively added tree. **Red observed.**
- **Tracker:** status returned to In Progress while this concrete recording
  regression is active. Do not restore Completed until focused validation and
  replacement required checks reconcile this inventory.

## Archive rewrite cadence and batching reconciliation (local)

- **Recording regression — addressed:** recursive archive insertion now performs
  one collision-aware rewrite and one recursive append instead of scanning and
  rewriting the archive once per source member. Existing members and the added
  tree are written to one temporary archive before the original is replaced.
- **Progress and cancellation — addressed:** rewrite headers, copied archive
  data, new-entry headers, and new-entry data emit progress callbacks. UI
  rendering is limited to once per second while cancellation polling remains at
  callback cadence. Directory Copy/PathCopy/Move retains its immediate progress
  render, and file/directory Rename now renders immediately and passes its live
  context through streamed-data callbacks.
- **Atomicity and compatibility — addressed:** a destination collision aborts
  the temporary rewrite and leaves the original archive byte-for-byte unchanged;
  the source-less directory path used by archive Makedir remains supported; and
  Rename streams retained entry data through the same progress callback. The
  `Archive_AddFile` source input is const-correct across its public declaration
  and callers.
- **Mutation ordering, refresh, projection, and capability contracts —
  intentionally unchanged:** transfer controllers still refresh/rebind logged
  volumes only after successful destination replacement; Move still removes its
  source only after destination success; archive-root projection remains
  container-rooted; and capability/footer/F1 guards retain the previously green
  implementation. The complete focused archive suite covers these adjacent
  surfaces and exposed no new failure.
- **Tracker — intentionally still In Progress:** local reconciliation is green,
  but Completed must not be restored until the amended SHA has replacement green
  required checks. `scripts/bugrec.sh` remains present in the commit.
- **Validation:** strict red proof for the callback regression was `0` progress
  callbacks before implementation. `make clean && make -j"$(nproc)"` passed
  after quarantining the pre-existing root-owned build tree at
  `/home/rob/ytreenova-build-root-owned-stale-20260907`; `pytest -q
  tests/test_archive_backend.py tests/test_archive_ui.py
  tests/test_archive_write_parity.py
  tests/test_archive_exit_ui.py::test_log_command_on_current_volume_reloads_tree_state`
  passed (`52 passed`); `make qa-fileops-integrity` passed (`42 passed`);
  `make qa-code-quality` passed; the module-boundary, AppState, and contract
  resilience test files passed (`4`, `756`, `9`, and `8` tests respectively).
  The first full `make qa-cppcheck` run found only the new archive source
  const-correctness issue; after correction, focused cppcheck over all changed C
  paths passed. Full local QA and a second full cppcheck sweep are deliberately
  left to required PR CI because focused reconciliation is complete.

## Live archive progress-frame inventory (active)

- **Newest recording / installed runtime:**
  `ytnova-20260907-173048-124831-CpDKS3.cast` shows the same-archive
  directory Move progress surface at 21.316 seconds, then no terminal output
  at all until completion at 29.121 seconds. The installed binary and
  `build/ytnova` are byte-identical, so this is a current-runtime regression,
  not a stale installation. **Active reproducer.**
- **Archive read/extract cadence:** `ExtractArchiveTree()` accepts an archive
  progress callback but never invokes it, including while streaming member
  data. `ReadTreeFromArchive()` reports only once per twenty headers, leaving
  logged-volume reload silent while libarchive skips a large compressed
  member. Header and bounded data-block callbacks, abort behavior, and reload
  callback delivery are in scope. **Active.**
- **Ncurses frame ownership:** `DrawSpinner()` writes through `stdscr` and then
  calls `doupdate()` without first staging that window, so the claimed
  immediate spinner update cannot reach the terminal. The menu/progress window
  owns the affected rows; preserve explicit window ownership and avoid a
  whole-screen refresh that could overwrite panel/menu projections. **Active.**
- **Focused proof:** add a layout-resilient PTY regression that gates archive
  data blocks without sleeps and observes two distinct spinner/progress frames
  while a same-archive directory Move is still blocked. Add focused backend
  proof for extraction callback cadence if needed to isolate the owner
  boundary. **Active.**
- **Adjacent contracts:** archive atomic replacement, destination-before-source
  deletion, collision/traversal guards, logged-volume refresh/rebind,
  container-root projection, capability/footer/F1 isolation, and requested
  `scripts/bugrec.sh` remain in the reconciliation set. Change them only if the
  live-frame regression exposes a shared root cause. **Audit after fix.**
- **Tracker/CI:** `docs/ROADMAP.md` remains In Progress. Amend the existing
  archive parity commit and rerun replacement required checks only after the
  focused red/green proof and inventory reconciliation are complete.

## Live archive progress-frame reconciliation (local)

- **Newest recording / installed runtime — addressed:** the silent interval is
  reproduced by a same-archive directory Move whose extraction scan crosses
  archive data before reaching the selected tree. The regression was run
  against byte-identical installed/build binaries before implementation.
- **Archive read/extract cadence — addressed:** `ExtractArchiveTree()` now
  reports every scanned header and every bounded data block actually extracted
  for the selected tree, and honors callback aborts. `ReadTreeFromArchive()` is
  intentionally unchanged: forcing a metadata-only scan to drain member data
  would make seekable archive formats decode content they can otherwise skip.
  Its existing header cadence remains sufficient for the refresh phase, while
  the archive rewrites surrounding refresh already report copied data blocks.
- **Ncurses frame ownership — addressed:** `DrawSpinner()` renders and refreshes
  the menu window that owns the progress rows, with `stdscr` only as a fallback.
  It no longer calls `doupdate()` on an unstaged `stdscr` mutation or risks a
  whole-screen refresh over independently owned windows.
- **Focused proof — addressed:** the event-gated PTY regression has no sleeps,
  blocks real libarchive data reads, and observes two different semantic
  progress rows with two distinct spinner glyphs before allowing the Move to
  continue. It failed red with two identical `Time: 00:00 ... MB/s` frames and
  passes after the runtime changes. Three consecutive focused reruns passed.
  A disposable copy of the exact 141 MB archive shape from the recording also
  produced two live spinner frames (`-` then `\\`) before Move completion and
  retained the expected moved member.
- **Adjacent contracts — intentionally unchanged:** archive replacement,
  collision/traversal checks, destination-before-source deletion, volume
  refresh/rebind, archive-root projection, capability/footer/F1 isolation, and
  `scripts/bugrec.sh` were not implicated. The complete archive suite and
  file-operation integrity matrix remain green.
- **Validation:** `make clean && make -j"$(nproc)"` passed after quarantining a
  newly recreated root-owned build tree at
  `/home/rob/ytreenova-build-root-owned-stale-20260907-live-progress`;
  the focused archive matrix passed (`53 passed`), `make qa-fileops-integrity`
  passed (`42 passed`), `make qa-code-quality` passed, focused cppcheck and
  clang-tidy completed with no new diagnostics, and the contract-resilience
  matrix/guard passed (`17 passed`). Full local QA remains deliberately unrun;
  required PR CI is the replacement full-QA gate.
- **Tracker/CI — active:** `docs/ROADMAP.md` remains In Progress until the
  amended SHA has green required checks. No inventory item is locally blocked
  or deferred.

## Live-progress CI remediation

- **Failure classification:** the full-coverage run failed the older
  filesystem-to-archive progress test because its ungated 2 MB transfer
  completed before the test sampled the transient progress row. The archive
  mutation itself completed successfully; this was a test synchronization
  defect exposed by coverage timing, not an implementation failure.
- **Addressed:** the shared libarchive test gate now intercepts both block and
  streaming read APIs. The older filesystem-to-archive test uses the same
  event-driven gate as the new same-archive regression, holds a real archive
  rewrite in flight, asserts the progress surface, and then explicitly releases
  it. No timeout inflation, sleep, fixed coordinate, or completion-as-progress
  fallback was added.
- **Validation:** both gated progress tests passed under a locally rebuilt
  `COVERAGE=1` binary (`2 passed`), and their normal-build run also passed
  (`2 passed`). The contract-resilience baseline remains unchanged and its
  guard passes.
- **CI:** amend and replacement required checks remain necessary; tracker stays
  In Progress.

## Final live-progress closure

- Required checks on `abf72ca7afbb951bdb77e94ea0477049fde41dba`
  passed, including the replacement full-coverage run, full pytest, static
  analysis, sanitizer, CodeQL, Fedora build/install smoke, file-mutation
  integrity, split-panel, runtime/security, docs, guards, and fuzz gates.
- The tracker is restored to Completed only after that green replacement run.
  The resulting tracker-only amend must retain green required checks before
  review or merge.
- The full mutation/capability/dual-panel/help/refresh/projection/progress
  inventory is reconciled: all defects found in the bounded audit are
  addressed; explicitly unchanged adjacent contracts have focused green proof;
  no item is deferred or blocked. `scripts/bugrec.sh` remains in the commit.

## Indeterminate archive progress inventory (active)

- **Manual progress-row reproducer:** same-archive directory Move reports an
  empty bar, `0%`, unknown ETA, and byte speed while elapsed time and the
  spinner advance. The user-visible row therefore hides real callback work and
  falsely presents unknown completion as zero percent. **Active regression.**
- **Shared progress contract and callers:** `ProgressContext`,
  `Progress_Start`, `Progress_Update`, `Progress_ShouldRender`, and
  `Progress_Render` are in scope. `ArchiveDirectoryTransferStart` is the only
  runtime caller and starts with unknown byte/item totals; existing and future
  known-byte or known-item determinate rendering must remain correct. **Active
  audit.**
- **Archive callback and ncurses cadence:**
  `ArchiveDirectoryTransferProgress` already increments `items_done`; retain
  callback-cadence cancellation and once-per-second full-row redraw while the
  spinner remains independently live. Unknown-total work must render a truthful
  indeterminate bar and visible increasing work-unit count without inventing a
  percentage, byte speed, or ETA. **Active.**
- **Focused proof:** extend the event-gated same-archive Move PTY regression so
  it proves semantic activity beyond elapsed time/spinner, without coordinates,
  complete grids, sleeps, or incidental padding. Add a focused renderer-level
  contract only if deterministic PTY gating cannot directly isolate the
  unknown-total state. **Active.**
- **Adjacent archive contracts:** atomic replacement, destination-before-source
  deletion, collision/traversal checks, logged-volume refresh/rebind,
  container-root projection, capability/footer/F1 isolation, and
  `scripts/bugrec.sh` remain in the reconciliation set and are intentionally
  unchanged unless this renderer audit reveals a shared root cause. **Audit
  after fix.**
- **Tracker/CI:** roadmap status is In Progress. Amend the existing archive
  parity commit and replace required checks only after red/green proof and full
  inventory reconciliation. **Active.**

## Indeterminate archive progress reconciliation (local)

- **Manual progress-row regression — addressed:** unknown-total archive work
  now uses a moving indeterminate block, shows `--%` instead of a false zero,
  and exposes the increasing callback work-unit count. Elapsed time remains
  available; byte speed is omitted when no byte count exists, and ETA remains
  explicitly unknown.
- **All progress-bar presentations — addressed/audited:** semantic searches for
  the progress API, ETA/speed row, and block-bar renderer found one shared bar
  implementation, `Progress_Render`. Its only runtime starter is
  `ArchiveDirectoryTransferStart`, reached by filesystem-to-archive,
  archive-to-archive, and archive-to-filesystem directory transfers; Copy and
  PathCopy share copy mode and Move shares the same renderer/callback. Other
  file and directory operations use the separate spinner surface rather than a
  second progress bar, so there is no duplicate bar implementation to repair.
- **Shared progress modes — addressed:** known byte totals retain determinate
  percentage, ETA, and measured speed. Known item totals now use item-based
  determinate percentage and an item counter. Unknown totals use the truthful
  indeterminate work mode. `Progress_Update` and `ProgressContext` interfaces
  remain unchanged.
- **Callback/ncurses cadence — intentionally unchanged:** archive callbacks
  continue accumulating work at callback cadence, with full-row renders rate
  limited by `Progress_ShouldRender` and the existing spinner refresh following
  each rendered row. The renderer still owns only `ctx_menu_window`; no panel or
  global-screen redraw was added.
- **Focused proof — addressed:** the event-gated same-archive Move test was red
  with two empty `0%` frames and no work count. It now observes increasing work
  values, distinct non-empty bar states, unknown percentage, and distinct
  spinner frames without coordinates or sleeps. The existing gated
  filesystem-to-archive Copy regression also passes under normal and coverage
  builds. Archive-to-filesystem and PathCopy take the same audited start and
  callback symbols and remain green in the complete archive suite.
- **Adjacent archive contracts — intentionally unchanged:** archive atomicity,
  move ordering, path guards, refresh/rebind, root projection, capability/help
  isolation, and `scripts/bugrec.sh` are outside the renderer root cause and
  remain covered by the complete archive and file-operation matrices.
- **Validation:** `make clean && make -j"$(nproc)"` passed after moving the
  user-created root-owned build tree to
  `/home/rob/ytreenova-build-root-owned-stale-20260907-indeterminate-progress`;
  the gated regression passed three consecutive runs; the complete focused
  archive matrix passed (`53 passed`); `make qa-code-quality` passed; the
  file-operation integrity matrix passed (`42 passed`) after one unrelated
  archive-create PTY flake passed alone three times; the contract-resilience
  matrix/guard passed (`17 passed`); focused cppcheck introduced no diagnostic;
  and both progress regressions passed under a coverage build (`2 passed`).
  Full local QA remains deliberately unrun because required PR CI is the
  pre-merge full-QA gate.
- **Tracker/CI — active:** the roadmap remains In Progress until the amended
  runtime SHA has green replacement required checks. No inventory item is
  deferred or blocked.

## Indeterminate archive progress closure

- Required checks on `5a696c2c796067d45a728124a4778361ed347a84`
  passed: full coverage, full pytest, static analysis, sanitizer, runtime and
  security, file mutation integrity, docs, guards/code quality, fuzz sync, and
  branch freshness. The first coverage run hit an unrelated filesystem-copy
  PTY visibility flake; that exact coverage-built case passed three consecutive
  local reruns and the job-only replacement run passed.
- The bounded shared progress inventory is reconciled: all progress bars use
  the corrected renderer, all archive directory endpoint routes use its
  unknown-total mode, known-byte and known-item modes are truthful, and the
  separate spinner-only operation surface is intentionally unchanged. No item
  is deferred or blocked. `scripts/bugrec.sh` remains included.
- `docs/ROADMAP.md` is restored to Completed only after the runtime amend had
  green replacement required checks. This tracker-only amend must itself retain
  green required checks before review or merge.

## Shared operation progress and archive information inventory (active)

- **Specification and UI contract — active:** refine the existing progress
  selection/ownership rules in `docs/SPECIFICATION.md`: potentially blocking
  filesystem and archive-VFS work must show immediate activity, promote after
  about one second to a compact centered main-display surface, keep an animated
  spinner beside a possibly stationary bar, use determinate progress only for
  cheap trustworthy totals, expose truthful bytes/items/work otherwise, and
  never pre-scan solely to invent a percentage. The footer, prompts, F1, and
  inactive panel remain independently owned.
- **Shared progress lifecycle — active:** audit and reconcile
  `ProgressContext`, `Progress_Start`, `Progress_ShouldRender`,
  `Progress_Update`, `Progress_Render`, `Progress_Finish`, and `DrawSpinner`.
  Move the bar away from `ctx_menu_window` to one separately owned centered
  ncurses window; handle small terminals, resize/recreation, dialog-stack
  ownership, cleanup, and restoration without stale overlay pixels.
- **Filesystem scan/mutation callbacks — active:** `Dir_Progress` covers log,
  relog, subtree scan, rescan, and refresh; `CopyFileContent` covers byte-copy
  and cross-device Move; `FileTags_UI_DeleteTaggedFiles` covers bulk deletion.
  Each potentially long route must enter/leave the shared lifecycle exactly
  once, render immediate activity, preserve cancellation semantics, and use a
  cheap source `stat` byte total where already available. Plain atomic rename,
  same-filesystem rename/move, mkdir, and single unlink remain intentionally
  spinner/bar-free unless their runtime path crosses a long callback boundary.
- **Archive callback/mutation family — active:** `UI_ArchiveCallback`, the
  Delete/Rename archive callbacks, archive create/add/extract/rewrite, preview,
  view/hex, tagged-view extraction, directory Copy/PathCopy/Move, and archive
  reload callbacks must feed the same lifecycle. Directory/archive totals stay
  indeterminate unless supplied by the backend without a new pre-scan; callback
  work units must advance the indeterminate bar. Preserve archive capability,
  path/collision/traversal, refresh/rebind, and destination-before-source-delete
  contracts.
- **Progress proof — active:** add event-gated, no-sleep, layout-resilient PTY
  coverage proving immediate spinner state, one-second promotion to a centered
  bar, live spinner/work changes while the bar is stationary, footer survival,
  and cleanup/restoration. Retain focused renderer coverage for known-byte,
  known-item, and unknown-total modes, and cover both filesystem and archive
  callback callers.
- **`0` dispatch and active-volume isolation — active:** add the missing `0`
  decode in `GetKeyAction`; preserve the filesystem stats-panel toggle contract,
  but in archive directory/file focus dispatch from the active panel volume
  only to archive information. The inactive split panel and global
  `ctx->view_mode` must not select the behavior.
- **Archive metadata backend — active:** add a dedicated archive-read query that
  obtains the container size with `stat`, sums non-negative declared libarchive
  member sizes with overflow detection, records unknown sizes truthfully, and
  computes no ratio when either size is zero/unknown or the comparison is not
  meaningful. Do not extract payloads or follow member paths for this query.
- **Archive information presentation — active:** show archive path/container
  size, total declared uncompressed member size, and archive-size-to-member-size
  compression ratio in a compact centered informational surface, with explicit
  unavailable/not-meaningful wording and ordinary dismissal/restoration.
- **Help and generated projections — active:** update English/German archive
  directory and archive-file F1 topics plus `etc/help/man.en.md`, keep ordinary
  filesystem `0` help truthful, regenerate runtime/man/usage assets with
  `make help-assets`, inspect projections, and run `make qa-help-assets`.
- **Compatibility and final sweep — active:** reconcile action registry/coverage
  declarations, headers/build source lists, focused backend and PTY tests,
  `docs/ROADMAP.md`, this handoff, and `scripts/bugrec.sh`. The script must remain
  included. No item is deferred or blocked at inventory time.

## Shared operation progress and archive information reconciliation (local)

- **Specification and UI contract — addressed:** `docs/SPECIFICATION.md` now
  requires immediate activity, promotion after approximately one second,
  determinate progress only from cheap trustworthy totals, truthful
  indeterminate work otherwise, and a separately owned centered main-display
  surface that leaves footer/prompt/help ownership intact.
- **Shared progress lifecycle — addressed:** `Progress_Start`,
  `Progress_ShouldRender`, `Progress_Update`, `Progress_Render`, and
  `Progress_Finish` use monotonic time, an immediate compact spinner window, a
  promoted centered bar window, determinate byte/item modes, and an
  indeterminate work mode. Window recreation and close use the dialog stack
  without a stale full-panel refresh. Small terminals omit the overlay rather
  than overwriting the footer.
- **Filesystem scan/mutation callbacks — addressed:** filesystem byte copy,
  recursive deletion, tagged deletion, log/expand/rescan/refresh, and subtree
  scans enter and leave the shared lifecycle at their owning boundary.
  Filesystem copy uses the already-open source descriptor size for determinate
  progress. Plain atomic rename/move, mkdir, and single unlink are
  intentionally unchanged because they have no iterative callback boundary
  and normally complete in one syscall.
- **Archive callback/mutation family — addressed:** create/add/delete/rename,
  rewrite/extract, view/hex/preview/tagged view, directory transfer, and archive
  reload callbacks feed the shared lifecycle. Backend header/data callback
  cadence and cancellation remain intact. Capability, path, collision,
  refresh/rebind, and destination-before-source-delete contracts are
  intentionally unchanged after the progress audit and remain covered by the
  archive/file-operation matrices.
- **Progress proof — addressed:** event-gated PTY tests prove immediate spinner
  state, one-second promotion, determinate filesystem byte progress,
  indeterminate archive work advancement, independently changing activity,
  and footer survival without fixed coordinates or sleeps. The accelerated
  scan cadence exposed an older timing-dependent collapse/mkdir test; that test
  now waits for semantic collapsed and restored tree states and passes three
  consecutive runs.
- **`0` dispatch and active-volume isolation — addressed:** `0` is decoded and
  preserves the filesystem stats toggle while archive directory/file focus
  opens container information. Split behavior derives only from the active
  panel volume; the inactive panel and legacy global view mode do not select
  the action.
- **Archive metadata backend and presentation — addressed:** the archive-read
  query stats the container, sums declared non-negative member sizes with
  overflow/unknown handling, does not extract payloads, and reports archive
  size, total uncompressed size, and the uncompressed-to-container ratio only
  when meaningful in a dismissible centered information surface.
- **Help and generated projections — addressed:** English/German F1 sources and
  authored man help describe `0`; runtime help, man markdown, and usage
  projections were regenerated and the help drift gate passes.
- **Compatibility and ownership seams — addressed:** progress lifecycle and
  archive callback ports are bound through `ViewContext` runtime hooks, so
  command modules no longer import UI headers. The module-boundary,
  compatibility, unsafe-API, security, clean-code, and contract-resilience
  guards pass. `scripts/bugrec.sh` remains included in the amended archive
  parity commit.
- **Validation:** `make clean && make -j"$(nproc)"` passed with only the known
  pre-existing const/trigraph warnings; `make help-assets && make
  qa-help-assets` passed; `make qa-code-quality` passed; the focused guard
  matrix passed (`35 passed`); the complete focused archive, information,
  stats, help, file-operation, preview, tagged-action, command, and panel
  matrix passed (`209 passed`); and the corrected panel synchronization case
  passed three consecutive focused runs.
- **Tracker/CI — active:** `docs/ROADMAP.md` remains In Progress until the
  amended runtime SHA has green replacement required checks. No inventory item
  is deferred or blocked.

## Shared progress CI remediation

- **Failure classification:** Fedora and clang-tidy found the same missing
  declaration for `AppStateMarkResizeRequest` in the new `0` filesystem stats
  dispatch. The file-operation gate separately found an exact security-contract
  regression where the tagged archive view temp-template builder passed the
  constant path limit rather than the destination array's actual size.
- **Addressed:** `fileinfo_band.c` now imports the owning AppState render port
  declaration, and tagged archive view uses `sizeof(temp_dir_template)` for the
  bounded temp-template call. The security regression now asserts the extracted
  helper's `temp_dir_out` ownership form. These are narrow declaration/bounds
  corrections on the intended dispatch and archive temp-cleanup paths; no
  behavior or owner boundary changed.
- **Validation/CI:** `make clean && make -j"$(nproc)"` passed with known baseline
  const/trigraph warnings. The focused security, stats, module-boundary, and
  contract-resilience pytest set passed (`19 passed`), followed by a green
  `make qa-code-quality`. Replacement required checks are still required; the
  tracker remains In Progress.

## Strict C99 public-header remediation

- **Red proof:** the required pytest/coverage jobs failed twelve isolated
  AppState probes because `ProgressContext` exposed `struct timespec`, which is
  incomplete under strict C99 without POSIX feature macros.
- **Inventory:** the shared `ProgressContext` definition, its only timing owner
  in `src/ui/progress.c`, strict-C99 AppState compile probes, the clean build,
  and replacement required checks. Archive progress behavior and monotonic
  timing must remain unchanged.
- **Correction:** keep `struct timespec` private to the progress implementation
  and store monotonic timestamps as seconds in the public context. No tests,
  feature macros, or unrelated public consumers need compatibility changes.

## Footer-spinner correction

- **Maintainer clarification:** immediate activity remains in the footer's
  reserved spinner cell. Only the progress bar is promoted into the centered
  main-display surface after approximately one second; the bar must not carry
  a second spinner.
- **Red proof:** the filesystem-copy and same-archive-move PTY regressions now
  require the immediate footer spinner, no immediate centered `Working`
  surface, a centered promoted bar, continued footer animation, and no
  `Activity:` spinner inside the bar. Both failed against the current runtime.
- **Inventory:** `DrawSpinner`, `Progress_Start`, `Progress_Update`,
  `Progress_Render`, progress-window sizing/cleanup, all three live-progress PTY
  paths, specification wording, generated-help drift (unchanged unless authored
  help changes), and PR wording. `0` remains available from both archive file
  and archive directory focus and reports archive-level size totals/ratio from
  the active panel only. `scripts/bugrec.sh` is added by this change.
- **Reconciled:** the footer owns immediate/continuing spinner animation;
  `progress.c` owns only the delayed centered bar; the shared context no longer
  exposes POSIX-only time types; filesystem copy, filesystem-to-archive rewrite,
  same-archive move, archive-directory `0`, archive-file `0`, active-panel
  isolation, the specification, and test-contract baseline are addressed.
  Authored help already states the `0` behavior in both archive contexts, so it
  is intentionally unchanged; its drift check remains required. No item is
  deferred.
- **Local proof:** `make clean && make -j"$(nproc)"` passed with baseline
  warnings. The strict-C99 probe matrix passed (`12 passed`); the complete
  focused archive/progress/info/stats/contract/boundary matrix passed
  (`50 passed`); `make qa-code-quality` and `make qa-help-assets` passed.
  Replacement required PR checks remain required before tracker completion.

## Final required-check remediation (active)

- **Static-analysis failure — implementation defect:** cppcheck reports the
  retained `Archive_CreateFromPaths` compatibility entry point as unused in
  the no-libarchive configuration after runtime callers moved to the
  progress-aware entry point. Reconcile the paired fallback entry points so
  the progress API delegates to the established compatibility API there,
  preserving both public contracts and the same unsupported result without a
  suppression.
- **Coverage failure — likely PTY visibility flake:** the coverage-only run
  missed the newly copied directory in
  `test_dir_copy_move_keeps_full_frame_after_command[c-dir_copy_out]`, while
  full pytest passed the same case and SHA. Re-run the exact coverage-built
  case repeatedly before classifying it as unrelated timing instability; do
  not change runtime or tests without a reproducible defect.
- **Remaining gate:** sanitizer is still running. Amend and push only after
  the static correction, clean build, focused cppcheck, exact PTY reruns, and
  final inventory reconciliation are green.

## Final required-check remediation reconciliation (local)

- **Static-analysis family — addressed:** the no-libarchive progress fallback
  now delegates to the retained archive-create compatibility entry point, and
  all five additional cppcheck findings use narrower const/scope ownership.
  No API was removed and no suppression was added. The full local
  `make qa-cppcheck` gate passes.
- **Tagged archive-view buffer contract — addressed with red/green proof:**
  the audit exposed that `sizeof` on the helper's decayed template parameter
  supplied only pointer size. The helper now accepts a pointer to the complete
  template array and forwards `sizeof` that array to `Path_BuildTempTemplate`,
  preserving the clean-code parameter budget while enforcing the bound in the
  type.
  The security invariant test failed before this correction and passes after
  it, along with archive-information and tagged-view regressions.
- **Coverage-only directory-copy miss — intentionally unchanged:** full pytest
  and sanitizer passed on the failed SHA, and the exact failing test passed
  three consecutive times using a coverage build. This is classified as a
  non-reproducing PTY visibility flake, not evidence for a runtime or test
  change. Replacement coverage CI remains required.
- **Inventory/remaining gate:** clean normal and coverage builds pass; the
  compatibility fallback, progress window, archive information, tagged-view
  template ownership, security contract, and exact coverage PTY case are
  reconciled. No item is deferred or blocked. The roadmap remains In Progress
  until every required replacement check passes on the amended SHA.

## Copy progress null-context remediation reconciliation

- **Static analyzer red proof:** replacement scan-build reported a possible
  null dereference at the archive-destination progress finish hook. Progress
  ownership implied a non-null `CopyOperation.ctx`, but the analyzer could not
  carry that implication to all early and normal finish paths. The public copy
  helpers deliberately permit a null UI context.
- **Addressed family:** archive-destination copy, archive-source extraction,
  and filesystem byte-copy now acquire progress ownership only inside an
  explicit non-null context branch and repeat that guard at every owned finish.
  Copy bytes, cancellation, cleanup, archive routing, and move ordering are
  intentionally unchanged.
- **Validation:** `make qa-scan` passed with no reports; `make clean && make
  -j"$(nproc)"` passed with baseline warnings; the focused filesystem-copy,
  filesystem-to-archive rewrite, and same-archive Move progress PTY tests
  passed (`3 passed`); the filesystem-to-archive, archive-to-filesystem, and
  archive-to-archive focused copy matrix passed (`3 passed`); and `make
  qa-code-quality` passed. No inventory item is deferred or blocked. The
  roadmap remains In Progress until replacement required checks pass.

## Archive virtual filesystem parity closure

- **Required checks:** all required checks passed on runtime SHA
  `f2e1d64c1036c834ef24929f9e7d720cd601d849`, including full pytest,
  sanitizer, static analysis, runtime/security, file-mutation integrity,
  documentation, guards/code quality, split-panel, Fedora, CodeQL, fuzz, and
  freshness gates.
- **Coverage rerun:** the first coverage job had one unrelated F2 picker PTY
  selection miss while full pytest was green. The exact test passed three
  consecutive times with a coverage build, and the job-only replacement run
  passed. It is classified as a non-reproducing PTY visibility flake; archive
  runtime and tests are intentionally unchanged.
- **Final reconciliation:** the manual archive-directory Delete regression,
  full archive mutation/endpoint/capability/path/move-ordering matrix,
  active-panel footer/F1 isolation, immediate footer spinner, delayed centered
  progress bar, live determinate/indeterminate progress, archive refresh/root
  projection, archive file/directory `0` information, authored/generated help,
  specification, and added `scripts/bugrec.sh` are addressed. No inventory item
  is deferred or blocked. The roadmap is Completed; replacement checks on the
  tracker amend remain required before review or merge.

## Archive file-list information and progress-label correction (active)

- **Manual correction / `0` behavior:** the archive `0` action currently opens
  a container-level popup, which violates the numeric FileInfo-band contract.
  Replace it with an active-panel archive file-list projection that adds
  per-entry `Size`, `Packed`, and `Ratio` columns from archive file and
  directory focus. Filesystem `0` keeps its stats-panel toggle; inactive panels
  and global view mode must not select archive behavior.
- **Archive entry metadata:** inventory `FileEntry`, archive tree insertion and
  read cadence, libarchive compressed-position support, mutation refresh, and
  file-row sizing/render modes. Packed size must be shown only where the
  backend can derive a trustworthy per-entry value; ratio is space saved as a
  percentage and is unavailable for zero/unknown sizes. Human-readable size
  formatting is required for both Size and Packed.
- **Documentation/generation:** correct the authored English/German F1 archive
  topics and `etc/help/man.en.md` in plain language; reconcile the stale
  generated/manual projections and any generator bootstrap text; update the
  specification and roadmap compatibility wording that still calls `0`
  unused. Run `make help-assets` and `make qa-help-assets`.
- **Progress labels:** preserve the accepted footer spinner and delayed
  centered bar. Change its summary to the conventional order: percent,
  elapsed, remaining, and rate for known-byte work; show truthful unavailable
  values plus item/work count for operations without totals. Audit elapsed and
  ETA calculation/rendering so completed fields cannot remain visually blank.
- **Focused proof:** replace modal archive-info PTY assertions with
  layout-resilient file-row projection assertions from archive directory and
  file focus, plus active-panel isolation and toggle/reset behavior. Extend
  backend coverage for packed/ratio availability and renderer tests for known
  bytes versus unknown archive work. No item is deferred at inventory time.
- **Tracker/PR:** roadmap is reopened to In Progress. Amend the existing commit
  only; do not push until the inventory and focused local proof are green.

## Archive file-list information and progress-label reconciliation (local)

- **Addressed — archive `0`:** archive directory and file focus now toggle an
  active-panel file-row overlay instead of opening a popup. ZIP rows show
  human-readable Size and Packed values plus percentage space saved; mixed
  stored/compressed and nested members are covered. Formats without reliable
  per-member packed attribution show dashes and are not needlessly decoded.
  Metadata is cached only for the current volume generation. Filesystem `0`
  and the inactive split panel are intentionally unchanged.
- **Addressed — ownership and compatibility:** archive decoding/path lookup
  remains in `archive_read.c`; the FileInfo action owns progress/caching and
  dispatch; `render_file.c` owns row formatting. Obsolete aggregate
  `ArchiveInfo`, `Archive_ReadInfo`, `UI_ShowArchiveInformation`, popup module,
  driver, and tests are removed. Current-tree search finds no live obsolete
  references. Archive mutation refresh invalidates the generation cache through
  the existing volume-generation contract.
- **Addressed — progress:** the footer remains the only spinner owner. After
  approximately one second, the centered bar reports Progress, Elapsed, Left,
  and Rate using nonblank `hh:mm:ss` durations when totals permit them. Unknown
  work reports truthful dashes and an advancing Work count. Known item totals
  use item rate rather than inventing byte throughput.
- **Addressed — help/docs:** English and German F1 sources, authored manpage,
  generator bootstrap text, specification, roadmap compatibility wording, and
  all generated projections describe the file-row behavior in plain language.
  The PR body is corrected to concise bullets. `scripts/bugrec.sh` remains in
  the amended commit.
- **Addressed — bounded mutation/footer family:** the previously reconciled
  Delete/Rename/Copy/PathCopy/Move endpoint, capability, path-safety,
  copy-before-delete, refresh/root projection, footer/F1 transition, and
  dual-panel inventory remains covered and unchanged by this correction.
- **Red/green proof:** the focused archive `0` regression failed before the
  correction because the old popup lacked Packed data, then passed with the
  file-row overlay. Progress-label regressions failed against the old
  Time/ETA/Speed contract, then passed with live ordered values.
- **Local validation:** `make clean && make -j4` passed with baseline warning
  debt; the complete archive/write/stats/help focused set passed (`97 passed`);
  archive backend/exit/help generator/text contracts passed (`64 passed`);
  help generation/drift, test-contract resilience, unsafe-API,
  module-boundary, clean-code, AppState contract, and cppcheck gates passed.
- **Tracker/remaining gate:** no correction-inventory item is deferred or
  blocked. The roadmap intentionally remains In Progress until the amended SHA
  is pushed and every required replacement PR check passes; only then may the
  Completed tracker amend and merge-safety cycle run.

## Coverage PTY wait remediation (local)

- **Failure classification:** the first replacement coverage job reproduced
  the previously observed directory-copy visibility miss while full pytest was
  green. The exact coverage-built case then passed once and failed once. Its
  post-operation wait searched for the fixture root name, which was already on
  screen, so it could return before the copied directory redraw.
- **Addressed — test synchronization:** the test now waits event-wise for the
  copied directory name that it actually asserts. Runtime directory-copy and
  redraw behavior are intentionally unchanged; this is a separate test defect,
  not an archive implementation failure or a reason to add timing delay.
- **Proof:** with the coverage build, the pre-correction exact loop reproduced
  `1 passed, 1 failed`; after correction the exact case passed three consecutive
  runs. The test-contract baseline was regenerated and its drift gate passed.
  Full pytest, static-analyzer, sanitizer, and every other initial required
  check passed; only the coverage job failed. Amend and push this narrow test
  correction, then require the complete replacement check set again.

## Corrective audit closure

- **Replacement checks:** every required check passed on corrective runtime SHA
  `c0f56c33514eb7b0e6403f29f7a76d3e623e25d9`, including full pytest,
  coverage, sanitizer, static analysis, runtime/security, file mutation,
  split-panel, documentation, Fedora, CodeQL, fuzz, and freshness gates.
- **Final inventory:** archive file/directory mutation and endpoint parity,
  capability/UI rejection, canonical-path safety, copy-before-delete ordering,
  footer/F1 active-panel isolation, recursive refresh, root projection, footer
  spinner, delayed centered progress bar, ordered progress details, archive `0`
  Size/Packed/Ratio rows, authored/generated help, specification, and
  `scripts/bugrec.sh` are addressed. The coverage-only PTY wait defect is also
  addressed without changing runtime behavior. No item is deferred or blocked.
- **Tracker:** the roadmap is restored to Completed. Amend and push this tracker
  closure, then require the complete final check set on that new SHA before any
  review or merge action.

## Determinate archive progress and transfer-failure audit (active)

- **Manual reproducer:** the `2026-09-08 16:00:52` recording copies
  `/mnt/d/Media/Comics` to `/home/rob/00.zip`. The archive already displays a
  top-level `Comics` member; the rewrite scans existing members, shows only
  indeterminate work, then replaces the backend collision reason with the
  generic `Directory archive transfer failed` status. Reproduce this as a
  focused collision/failure path before changing runtime behavior.
- **Progress phase and totals:** inventory the archive rewrite scan, source-tree
  preflight, recursive payload write, archive reload, and source removal phases.
  The centered bar may pulse only while totals are being discovered; once a
  trustworthy item/byte total is known it must transition to monotonic
  determinate progress, with advancing elapsed time and truthful remaining time
  or rate. The footer remains the spinner owner.
- **Progress ownership/lifetime:** inventory `Progress_Start`, archive callback
  dispatch, progress updates/render cadence, nested archive reload callbacks,
  `Progress_Finish`, cancellation, success, and failure. The promoted surface
  must remain live until the operation really completes or fails and must not
  disappear merely because one internal phase ended.
- **Transfer result:** inventory collision, unsupported source member,
  canonical/path-length, libarchive read/write, temporary rewrite/finalization,
  refresh, and move source-removal failures. Preserve the most specific safe
  backend reason rather than overwriting it with a generic controller status;
  never replace or partially merge an existing destination silently.
- **Adjacent operation family:** audit filesystem file copy, filesystem to
  archive, archive to filesystem, archive to archive, same-archive rewrite,
  Delete, Rename, Copy, PathCopy, and Move progress producers. Address shared
  progress-contract defects together; leave operations with already reliable
  totals intentionally unchanged with evidence.
- **Focused proof and docs:** add red-first backend and layout-resilient,
  event-driven PTY regressions for calculation-to-determinate transition,
  monotonically increasing completion, live elapsed/remaining values,
  lifetime through terminal result, and specific collision rejection. Update
  the canonical progress specification if phase semantics need clarification;
  retain `scripts/bugrec.sh` in the amended commit.
- **Tracker/closure:** `docs/ROADMAP.md` is reopened to In Progress. Reconcile
  each item above before amend/push; replacement required checks are mandatory
  before restoring Completed or merging.

## Compact duration formatting reconciliation (local)

- **User-facing contract:** progress `Elapsed` and `Left` use adaptive compact
  units (`8s`, `1m 08s`, `1h 02m 03s`) instead of leading zero-valued units.
- **Ownership inventory:** one shared string-formatting helper owns this
  convention; the progress renderer calls it for both fields. The focused
  filesystem-copy, same-archive Move, and interrupted Move PTY paths protect
  the semantic output without fixed screen coordinates.
- **Red proof:** the filesystem-copy progress regression required compact
  durations and failed while the runtime still rendered `00:00:01`.
- **Reconciled — single owner:** `String_FormatCompactDuration()` in
  `string_utils.c` is the only implementation. Both progress fields call it;
  the public declaration lives in the shared definitions header, its exact
  boundary values have a strict-C99 driver test, and the existing string-utils
  fuzz harness exercises arbitrary signed durations.
- **Reconciled — operation surfaces:** filesystem copy, archive size scan,
  filesystem-to-archive rewrite, same-archive Move, and interrupted Move PTY
  paths use the common progress renderer and require semantic compact duration
  output. The archive size scan starts with volume byte/member totals, so it is
  determinate after promotion rather than maintaining a separate formatter or
  progress surface.
- **Validation:** `make clean && make -j4` could not pass its clean phase due
  the known root-owned `build/locale/de/LC_MESSAGES/ytnova.mo`; the subsequent
  complete incremental `make -j4` passed. `pytest -q tests/test_archive_ui.py
  tests/test_archive_backend.py tests/test_archive_write_parity.py
  tests/test_string_utils.py` passed (`61 passed`); tagged-view/security/fuzz
  focused tests passed (`19 passed`); the complete focused progress/archive
  matrix passed (`28 passed`); and `make qa-code-quality` passed. Focused
  cppcheck on `copy.c`, `progress.c`, `tagged_view.c`, and `string_utils.c`
  exposed three scope findings, which were corrected; the corrected paths then
  passed focused cppcheck. The earlier archive Rename PTY miss did not reproduce
  in three consecutive exact reruns and is intentionally unchanged.
- **Closure state:** all compact-duration and shared-owner inventory items are
  addressed. No item is deferred or blocked. The roadmap remains In Progress
  until the amended SHA passes every replacement required PR check.

## Determinate progress and transfer-failure reconciliation (local)

- **Manual collision reproducer — addressed:** an existing archive destination
  is rejected before replacement and the specific backend collision reason is
  retained through progress cleanup instead of being overwritten by a generic
  controller error. The filesystem Move source remains intact.
- **Totals and phases — addressed:** archive creation, file and recursive
  directory transfer, archive rewrite, packed-size scans, refresh, and Move
  source removal publish or precompute bounded byte/item totals. Progress is
  indeterminate only before totals are available, then advances monotonically;
  active display is capped below completion until terminal cleanup.
- **Ownership and terminal behavior — addressed:** all archive callbacks carry
  exact byte/item deltas into the one `ProgressContext`; nested work does not
  finish the outer operation. Escape retains `Operation Interrupted`, closes
  the centered window through dialog redraw ownership, clears the footer
  spinner, and preserves any destination already written before source-removal
  failure. Successful operations remain silent.
- **Adjacent operations — addressed or intentionally unchanged:** archive file
  and directory Delete/Rename/Copy/PathCopy/Move use the shared lifecycle and
  existing capability/path/collision guards. Filesystem byte copy now reports
  every block. Existing destination-before-source Move ordering and duplicate
  retention on source-removal failure are unchanged because the audit and
  focused regressions confirm them.
- **Proof:** backend tests assert exact totals/deltas and archive-creation
  totals. PTY regressions cover calculation-to-determinate transition, multiple
  increasing frames, source-rewrite incompleteness, collision specificity,
  Escape cleanup, footer-spinner continuity, and active archive Size/Packed/
  Ratio rows. The canonical specification records the common lifecycle and
  compact duration convention. No item is deferred or blocked.

## Final corrective closure

- **Required checks:** every required check passed on corrective runtime SHA
  `cfaf409d3c8e2570a2daebb84ac6500ef4f23f36`, including full pytest,
  coverage, sanitizer, static analysis, runtime/security, file-mutation,
  split-panel, documentation, Fedora, CodeQL, fuzz, freshness, and code-quality
  gates.
- **Final inventory:** archive file/directory mutations and endpoint routing,
  capability and path rejection, destination-before-source Move safety,
  mutation refresh and archive-root projection, active-panel footer/F1
  isolation, footer spinner, centered delayed determinate progress, graceful
  error/cancellation cleanup, shared compact duration formatting, archive
  Size/Packed/Ratio rows, authored/generated help, specification, and added
  `scripts/bugrec.sh` are addressed. No item is deferred or blocked.
- **Tracker:** restored to Completed after the corrective runtime SHA passed
  the full required set. The tracker-only amended SHA must pass its complete
  replacement required-check set before review or merge.

## Instant archive information toggle correction (active)

- **Filesystem `0` dispatch:** `ApplyFileProjectionToggleSelection()` currently
  aliases filesystem `0` to the stats-panel visibility state. This conflicts
  with the numeric FileInfo contract requested by the maintainer: only `F6`
  owns stats visibility, while filesystem `0` is a silent no-op. Audit tree,
  file, Showall/Global, and split-panel dispatch through the shared FileInfo
  helper. **Active.**
- **Archive `0` latency:** the first archive `0` calls
  `Archive_LoadPackedSizes()`, reopens ZIP input, and decodes every regular
  member solely to derive compressed-byte deltas. Move trustworthy packed-size
  capture into the normal archive scan/generation lifecycle so `0` only changes
  the active panel projection. Non-ZIP formats must retain unknown Packed/Ratio
  values without an extra decode. **Active.**
- **Ownership/compatibility:** keep archive decoding and member-path mapping in
  `archive_read.c`, keep display selection in `fileinfo_band.c`, preserve
  active-panel isolation and mutation-generation refresh, and remove the lazy
  loader/cache seam if no caller remains. **Active.**
- **Focused proof/docs:** add red-first coverage that filesystem `0` leaves
  stats visibility unchanged and archive metadata exists before the projection
  toggle; update the canonical specification, numeric FileInfo roadmap contract,
  authored F1/man sources, and generated help projections. **Active.**

## Instant archive information toggle reconciliation (local)

- **Filesystem `0` — addressed:** the shared FileInfo projection handler now
  treats `0` as a silent no-op on filesystem volumes. Tree, file,
  Showall/Global, and split-panel callers all converge through this handler;
  `F6` remains the sole stats-visibility action.
- **Archive `0` — addressed:** ZIP packed-byte metadata is captured into each
  `FileEntry` during the normal archive-generation scan. Pressing `0` now only
  toggles the active panel's archive row overlay and performs no archive I/O;
  nested members retain exact Size/Packed/Ratio values. Formats for which the
  backend cannot attribute packed bytes retain dashes without an extra scan.
- **Ownership/compatibility — addressed:** archive decoding and canonical
  member lookup remain in `archive_read.c`; presentation remains in
  `fileinfo_band.c`. The obsolete on-demand loader, public declaration, and
  generation cache fields were removed, leaving no parallel packed-size path.
  Mutation reloads naturally repopulate metadata through the existing archive
  volume scan. Encrypted ZIP members remain browsable and deliberately retain
  unknown Packed/Ratio values rather than forcing a data decode without a
  passphrase.
- **Proof — addressed:** both regressions failed before the runtime correction.
  The filesystem test observed stats disappear; the gated ZIP test blocked in
  `archive_read_data()` after `0`. They now pass, and active-panel isolation,
  nested ZIP attribution, and unknown packed sizes are covered without fixed
  layout assertions or sleeps.
- **Docs/generated assets — addressed:** the specification, numeric FileInfo
  roadmap contract, English/German F1 sources, English/German authored manual
  sources, generator bootstrap text, and generated English help projections
  state that filesystem `0` is inert, `F6` owns stats, and archive `0` uses
  values collected during load.
- **Validation:** `make` passed; focused archive/file-info/help tests passed
  (`15`, then `13`, then `25` tests); `make qa-help-assets`,
  `make qa-code-quality`, `make qa-split-panel-gates` (`6 passed`), and
  `make qa-fileops-integrity` (`42 passed`) passed. Focused cppcheck on
  `archive_read.c` and `fileinfo_band.c` exited zero with informational missing
  system-header notices only. One footer-spinner frame comparison missed once
  in the broad focused set and then passed three consecutive exact runs; it is
  unrelated to these dispatch/archive-load changes and is intentionally
  unchanged.
- **Remaining gate:** no implementation item is deferred or blocked. The
  roadmap remains In Progress until this amended runtime SHA passes every
  required PR check; only then may its Completed state be restored.

## Low-percentage determinate progress correction (active)

- **Manual recordings:** the `21:58:48` filesystem copy shows a visibly filled
  bar at 63--99%, while the `22:00:58` filesystem-directory-to-archive Copy
  reports live 1%/Elapsed/Left/Rate values but leaves the bar interior blank.
  Escape reaches `Operation Interrupted` in the latter recording. The
  post-cancel filesystem refresh is a separate indeterminate progress phase
  and is intentionally unchanged unless focused proof exposes a lifecycle
  defect.
- **Root boundary:** `Progress_Render()` is the single shared renderer for
  filesystem and archive operations. Its integer fill calculation rounds every
  positive completion below one cell down to zero, so large transfers can show
  a blank determinate bar for a long time even though work is advancing.
- **Inventory:** preserve the existing one-second promotion, footer spinner,
  pulse-only-while-indeterminate behavior, percentage/elapsed/left/rate text,
  terminal 99% cap, and cancellation cleanup. Add a red-first semantic
  regression proving every positive determinate value renders at least one
  filled cell, then fix the shared renderer so all filesystem/VFS callers gain
  the same behavior. Reconcile the renderer, filesystem copy, archive transfer,
  existing monotonic-frame tests, handoff, tracker, and pending encrypted-entry
  safety guard before amend/push.

## Low-percentage determinate progress reconciliation (local)

- **Shared renderer — addressed:** every positive determinate completion now
  paints at least one cell after percentage calculation. Zero work remains
  blank, unknown totals still pulse, later cells remain proportional, and the
  existing pre-terminal 99% cap is unchanged. Filesystem and archive operations
  receive the correction through the sole `Progress_Render()` implementation.
- **Manual operation family — addressed/intentionally unchanged:** the large
  filesystem-directory-to-archive Copy now gains a visible 1% cell. The
  ordinary filesystem Copy and same-archive Move paths still produce increasing
  determinate frames. Escape cleanup is intentionally unchanged because the
  latest recording reaches `Operation Interrupted` and the focused cancellation
  regression remains green; its subsequent source-tree refresh correctly owns
  a separate indeterminate progress phase.
- **Regression/spec — addressed:** the filesystem-copy PTY case now holds a
  large sparse source at 1%, proves the filled interior is nonblank, and then
  proves percentage and fill advance without screen-coordinate assumptions or
  sleeps. The canonical progress rule now requires a visible filled cell once
  positive determinate work begins. The regenerated test-contract baseline is
  reconciled.
- **Pending packed-size safety guard — addressed:** encrypted ZIP entries skip
  packed-size decoding during archive load and remain browsable with unknown
  Packed/Ratio values; the archive `0` focused matrix remains green.
- **Validation:** the red regression reported a blank first bar
  (`['                                                        ', '▮ ...']`);
  after the shared fix the exact case passed three consecutive runs. `make -j4`
  passed after the known root-owned locale artifact prevented `make clean`;
  the complete focused progress/cancellation/archive-`0` matrix passed (`9
  passed`); `make qa-code-quality` passed; and focused cppcheck exited zero
  with informational missing-system-header and existing cross-file
  `ExtractArchiveTree` notices only.
- **Tracker/gate:** no correction item is deferred or blocked. The roadmap
  remains In Progress until the amended runtime SHA passes every replacement
  required PR check.

## Final low-percentage progress closure

- **Required checks:** every observed required check passed on runtime SHA
  `9a764b449d24355cbfef8eec162f51f49c21279a`, including full pytest,
  coverage, sanitizer, static analysis, runtime/security, mutation,
  split-panel, documentation, Fedora, CodeQL, fuzz, and freshness gates.
- **Final inventory:** the shared renderer visibly fills positive determinate
  progress for filesystem and archive operations; promotion, footer spinner,
  monotonic completion, compact details, terminal cleanup, and cancellation
  remain reconciled. The instant archive `0` correction, encrypted-member load
  guard, full archive mutation/footer/help family, generated assets, and
  `scripts/bugrec.sh` remain addressed. No item is deferred or blocked.
- **Tracker:** restored to Completed after the corrected runtime passed the full
  required set. The tracker-only amended SHA must pass its complete replacement
  check set before review or merge.

## Archive destination collision wording correction (active)

- **Manual reproducer:** the `23:36:53` recording tags all files in
  `/home/rob`, creates `0101.zip`, then copies `/mnt/d/Media/Comics` into its
  root. Archive creation stored the tagged `Comics` symlink, so the later
  directory basename correctly collides with that non-directory member. The
  backend reports `Archive destination already contains this directory`, which
  falsely describes the existing member and obscures why a new archive can
  collide.
- **Inventory:** `ArchiveTreeAddContext`, `cb_add_tree()`, and
  `Archive_AddTree()` own collision detection/reporting; filesystem-to-archive,
  archive-to-archive, Copy, PathCopy, and Move directory routes share that
  backend. Update the specific safe backend reason to identify the requested
  canonical member path without claiming its type. Extend the existing PTY
  collision regression, whose fixture already collides with a regular archive
  file, and preserve source-retention, no-overwrite, progress cleanup, and
  generic-error non-overwrite contracts. Reconcile specification/help only if
  they expose this runtime wording; retain `scripts/bugrec.sh`.
- **Tracker/gate:** reopened to In Progress until red-green proof, focused
  validation, amended push, and replacement required checks are reconciled.

## Archive destination collision wording reconciliation (local)

- **Backend/reporting — addressed:** `Archive_AddTree()` now reports
  `Archive destination already contains '<canonical path>'`. It identifies the
  actual requested archive location without guessing whether the existing
  colliding entry is a file, directory, symlink, or implicit parent. The shared
  backend makes the correction available to filesystem/archive Copy, PathCopy,
  and Move directory routes; detection and no-overwrite behavior are unchanged.
- **Focused regression — addressed:** the existing filesystem-directory Move
  collision fixture deliberately places a regular file at `moved`, now requires
  the path-specific neutral message, and still proves the source tree and
  destination archive are unchanged. It failed red on the former `this
  directory` wording, passed green after the backend change, and passed three
  consecutive exact reruns.
- **Adjacent contracts — addressed/intentionally unchanged:** the focused
  Copy/PathCopy/Move success matrix, archive collision rejection, and
  source-delete-failure preservation all remain green (`6 passed`). Progress
  cleanup and specific-error preservation are unchanged. No authored help or
  specification surface contains the superseded runtime sentence, so generated
  help is intentionally unchanged. The test-contract baseline is regenerated
  and reconciled; `scripts/bugrec.sh` remains included.
- **Validation:** `make clean && make -j4` again stopped only at the known
  root-owned locale artifact; the subsequent complete `make -j4` passed with
  baseline warnings. `make qa-code-quality` passed. Focused cppcheck exited zero
  with informational missing-system-header and pre-existing standalone
  const/unused-function notices only.
- **Tracker/gate:** no item is deferred or blocked. The roadmap remains In
  Progress until the amended runtime SHA passes every replacement required PR
  check.

## Archive destination collision wording closure

- **Required checks:** every observed required check passed on runtime SHA
  `28a097c66ea4356bf80605ac23a7b746f226fcf3`, including full pytest,
  coverage, sanitizer, static analysis, runtime/security, mutation,
  split-panel, documentation, Fedora, CodeQL, fuzz, and freshness gates.
- **Final inventory:** the collision message identifies the canonical archive
  member without guessing its type; shared directory Copy, PathCopy, and Move
  routes retain their collision, no-overwrite, source-preservation, and
  progress-cleanup behavior. Specification and help remain intentionally
  unchanged because they do not expose the corrected runtime sentence.
  `scripts/bugrec.sh` remains included. No item is deferred or blocked.
- **Tracker:** restored to Completed after the corrected runtime passed the
  complete required set. The tracker-only amended SHA must pass its complete
  replacement check set before review or merge.

## Archive creation preflight activity correction (active)

- **Manual reproducer:** recursively creating an archive from
  `/mnt/d/Media/Comics` can spend several seconds expanding the source tree
  before the existing archive-write progress lifecycle starts. During that
  bounded preflight, neither the footer spinner nor the promoted main-display
  bar is visible, so the single-threaded UI appears frozen.
- **Owner boundary and runtime path:** `UI_GatherArchivePayload()` invokes
  `UI_BuildArchivePayloadFromPaths()` before `UI_CreateArchiveFromPayload()`
  starts progress. Recursive and non-recursive directory expansion in
  `src/ui/archive_payload.c` are the shared blocking preflight surface for
  Tree and File view archive creation. The shared `Progress_*` renderer and
  `UI_ArchiveCallback()` remain the required display/cancellation owners.
- **Inventory:** add progress/cancellation callbacks to the payload-expansion
  layer without exposing `ViewContext`; start and finish the outer archive
  creation preflight lifecycle around expansion; cover recursive and
  non-recursive walkers plus top-level file selection; preserve the existing
  no-progress compatibility entry point used by the backend driver; prove
  immediate footer activity, one-second indeterminate promotion, prompt
  cleanup, and Escape cancellation with an event-driven PTY gate. Existing
  archive-write progress, destination exclusion, overwrite, payload mapping,
  dual-panel isolation, and help text are adjacent but intentionally unchanged
  unless focused validation finds a shared regression. The specification
  already requires blocking scan/preflight activity, so authored help and
  generated assets are intentionally unchanged. `scripts/bugrec.sh` remains
  included.
- **Tracker/gate:** reopened to In Progress until red-green proof, inventory
  reconciliation, amended push, and replacement required checks are green.

## Archive creation preflight activity reconciliation (local)

- **Progress lifecycle — addressed:** Tree and File view archive creation now
  starts the shared `Progress_*` lifecycle before payload expansion. Both
  recursive and non-recursive walkers report discovered entries through an
  `ArchiveProgressCallback`, keeping the footer spinner live and promoting the
  existing centered indeterminate bar after one second. Escape aborts the
  expansion, closes both progress surfaces, and preserves `Operation
  Interrupted` instead of continuing to the destination prompt.
- **Compatibility/adjacent paths — addressed or intentionally unchanged:**
  the original no-progress payload-builder entry point remains as a wrapper
  for backend callers. Top-level file selection uses the same lifecycle.
  Existing archive-write progress starts after destination confirmation and is
  intentionally unchanged. Destination exclusion, overwrite, archive member
  mapping, dual-panel state, and the shared renderer remain unchanged and their
  focused tests are green. The existing specification already covers blocking
  scan/preflight activity; authored help and generated assets remain unchanged.
  `scripts/bugrec.sh` remains included. No item is deferred or blocked.
- **Red-green proof:** the gated recursive PTY regression first failed because
  the first blocked directory entry had no footer spinner. After the fix, the
  recursive and non-recursive cases prove immediate footer activity,
  one-second centered-bar promotion, live footer motion, and clean Escape
  cancellation without sleeps or layout coordinates.
- **Validation:** `make clean && make -j4` stopped only at the known root-owned
  locale artifact; the subsequent full `make -j4` passed with baseline
  warnings. The complete focused payload/create/shared-progress matrix passed
  (`10 passed`). `make qa-code-quality` passed after regenerating the shifted
  contract-resilience baseline. Focused cppcheck exited zero with only
  missing-system-header information and standalone test-interposer
  unused-function notices. `clang-format --dry-run --Werror ...` was
  unavailable because `clang-format` is not installed.
- **Tracker/gate:** the roadmap remains In Progress until the amended SHA
  passes every replacement required PR check.

## Archive creation preflight activity closure

- **Required checks:** every observed required check passed on runtime SHA
  `a126ce84fa760ef4c3e5b0fa5296b838cd40b3fd`, including full pytest,
  coverage, sanitizer, static analysis, runtime/security, mutation,
  split-panel, documentation, Fedora, CodeQL, fuzz, and freshness gates.
- **Final inventory:** recursive and non-recursive archive-creation preflight
  now owns immediate footer activity, one-second indeterminate promotion, and
  clean Escape cancellation through the shared progress callback. Existing
  write-phase progress, compatibility entry point, archive destination and
  payload contracts, generated help, and `scripts/bugrec.sh` remain
  reconciled. No item is deferred or blocked.
- **Tracker:** restored to Completed after the corrected runtime passed the
  complete required set. The tracker-only amended SHA must pass its complete
  replacement check set before review or merge.
