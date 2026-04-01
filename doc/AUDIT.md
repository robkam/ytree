# AUDIT.md

## 1. Purpose
This document defines the mandatory quality process for the Ytree modernization project. Auditing is an ongoing process that starts during implementation and continues through merge and release. The release gate is the final checkpoint.

## 1.1 Cadence
- Run the full audit loop for each feature-sized change or PR.
- Always run the merge/release gate before merge/release.
- Do not run the full gate after every prompt-level micro-edit unless risk justifies it.

## 1.2 QA Layers

The project uses four QA layers with increasing depth and cost:

| Layer | Command | What it checks | When to run |
|---|---|---|---|
| CI Gate | `git push` (automatic) | Build, unsafe C API guard, `pytest` | Every push (automatic) |
| Local QA | `make qa-all` | clang-tidy, cppcheck, scan-build, Valgrind smoke (`--version`), `pytest`, unsafe API guard, module-boundary guard | Before every PR or feature merge |
| Deep Audit | `make qa-valgrind-full` | Automated interactive Valgrind Memcheck session (leak, uninit, FD, use-after-free checks) | Before release, after major refactoring, or periodically |
| Manual Feature Audit | `make qa-valgrind-interactive` | You manually drive ytree under Valgrind to exercise new feature code paths | After adding a major new feature |

- **CI Gate** runs automatically on push via GitHub Actions. No developer action needed.
- **Local QA** (`make qa-all`) is the standard pre-merge gate. Run it for every non-trivial change.
- **Deep Audit** (`make qa-valgrind-full`) is on-demand. It drives a scripted interactive ytree session under Valgrind and takes ~2-3 minutes. Run it:
  - Before tagging a release
  - After changes to memory management, allocation, or cleanup paths
  - After major refactoring sessions
  - Periodically as a health check
- **Manual Feature Audit** (`make qa-valgrind-interactive`) launches ytree under Valgrind for you to drive manually. Use it after adding a major feature to exercise the new code paths specifically. Exit cleanly, then inspect `valgrind.txt`.

## 2. Role Mapping
The workflow relies on four distinct roles. When using AI agents, these must be treated as adversarial personas to ensure objectivity.

- **Architect (Human):** Defines scope, constraints, and acceptance criteria.
- **Developer (Agent):** Implements logic. Primary tool: `clangd` (LSP) via `compile_commands.json`.
- **Code Auditor (Agent/Human):** adversarial review. Static tools: `clang-tidy`, `cppcheck`, `scan-build`.
- **Tester (Human/Agent):** Validates regression safety. Dynamic tools: `pytest`, `Valgrind`.

### 2.1 AI Persona Execution Mapping
When an AI agent performs this workflow, it must execute the loop explicitly using these personas in order:
1. Architect (scope/invariants/acceptance criteria)
2. Developer (implementation/fixes)
3. Code Auditor (findings/gate decision)
4. Tester (verification/regression)
5. Code Auditor (final pass/fail)

## 3. Mandatory Toolchain Commands
Run these commands in this order to generate evidence-based findings.

0. **Compile Database Preflight:** `make clean && bear -- make`
1. **Linting & Modernization:** `clang-tidy $(rg --files src -g '*.c') -p .`
2. **Static Analysis:** `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include`
3. **Logic Path Analysis:** `make clean && scan-build --status-bugs make`
4. **Memory/Runtime Analysis:** `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.txt ./build/ytree .` (then exit ytree cleanly). For automated interactive runs, use `make qa-valgrind-full` which drives a scripted pexpect session.
5. **Regression Tests:** `source .venv/bin/activate && pytest`

For step 4 in automated runs, drive a deterministic start/exit path (for example with `pexpect`) so Valgrind can finish and return an actionable exit code.

Local shortcut targets are available in the `Makefile`:
- `make qa-clang`
- `make qa-cppcheck`
- `make qa-scan`
- `make qa-valgrind`
- `make qa-pytest`
- `make qa-unsafe-apis`
- `make qa-module-boundaries`
- `make qa-all` (runs `qa-clang`, `qa-cppcheck`, `qa-scan`, `qa-valgrind`, `qa-pytest`, `qa-unsafe-apis`, `qa-module-boundaries` in order)
- `make qa-all-log` (same as `qa-all`, with full output captured to `qa-all.log` in repo root; override with `QA_LOG=/path/to/file`)

GitHub CI is a baseline gate (build + unsafe C API guard + `pytest`) and does not replace the full local audit loop.

## 4. Continuous Audit Loop (Default)
Run this loop for every non-trivial change and every PR.

### Phase A: Architecture Check (Architect)
- Confirm scope, invariants, and acceptance criteria.
- Identify modules/symbols affected and known risk areas (e.g., UI redraw, pointer arithmetic).

### Phase B: Fix (Developer)
- **Real-time Guard:** `clangd` must be active. The Developer must resolve all LSP diagnostics before proceeding.
- **Task:** Implement fixes or features using atomic prompts. Iterate on the prompt (not the code) until functional requirements are met.

### Phase C: Audit (Code Auditor)
- **Lint Gate:** Run `clang-tidy $(rg --files src -g '*.c') -p .`. Any unresolved correctness/safety finding is an automatic REJECT.
- **Static Gate:** Run `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include`. Any unresolved finding is an automatic REJECT.
- **Logic Gate:** Run `make clean && scan-build --status-bugs make`. Any reported analyzer bug is an automatic REJECT.
- **Rules:** Provide file:line and evidence for every finding. Prefer root-cause fixes over "if(ptr)" symptom patches.
- **Tracking Rule:** Medium/Low findings must be either fixed in the PR or explicitly tracked with rationale.

### Phase D: Verify (Tester)
- **Memory Gate:** Run `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.txt ./build/ytree .` and exit cleanly.
- **Dynamic Gate:** Run relevant `pytest` suites for the touched scope (or full `pytest` when scope is broad).
- **Criteria:** Failure occurs if any test fails or if Valgrind returns a non-zero exit code.

### Phase E: Merge Gate (Code Auditor)
- Re-audit the final PR diff before merge.
- **Exit Criteria:** Blocker = 0, High = 0, scan-build = Green for scoped analysis, and verification evidence attached.

## 5. Release Gate (Final Checkpoint)
Before a release tag/cut, run a full audit pass across the release scope:
- Fresh `clang-tidy` run against all `src/**/*.c` files.
- Fresh `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include` run.
- Full `pytest` suite.
- Fresh `scan-build --status-bugs` run.
- Valgrind verification for release-critical paths.
- Re-audit of final release diff.
- **Exit Criteria:** Blocker = 0, High = 0, scan-build = Green, Valgrind = 0 bytes definitely lost.

## 6. Audit Notes (Lightweight)
Audience: project maintainer, future contributors, and AI agents continuing work on the same area.

Before merge and before release, keep short notes:
- **Summary:** Total findings by severity.
- **Checks Run:** Which tools were run and outcomes (`clang-tidy`, `cppcheck`, `scan-build`, `pytest`, `valgrind` as applicable).
- **Residual Risk:** List any remaining Low/Medium items.
- **Status:** PASS or FAIL.

## 7. Ytree-Specific Priorities
- **Stability Over Novelty:** Features must not compromise the database-logger tree integrity.
- **MVC Integrity:** Maintain context-passing; avoid global variables.
- **ncurses Hygiene:** Every `newwin` or `derwin` must have a matching `delwin`.
- **Deterministic State:** No uninitialized variables. Every allocation must have a clear ownership and cleanup path.

---

**Technical Note:** Keep `compile_commands.json` current for `clangd` and `clang-tidy`. Recommended command: `make clean && bear -- make`.
