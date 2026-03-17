# AUDIT.md

## 1. Purpose
This document defines the mandatory quality process for the Ytree modernization project. Auditing is an ongoing process that starts during implementation and continues through merge and release. The release gate is the final checkpoint.

## 2. Role Mapping
The workflow relies on four distinct roles. When using AI agents, these must be treated as adversarial personas to ensure objectivity.

- **Architect (Human):** Defines scope, constraints, and acceptance criteria.
- **Developer (Agent):** Implements logic. Primary tool: `clangd` (LSP) via `compile_commands.json`.
- **Code Auditor (Agent/Human):** adversarial review. Static tools: `clang-tidy`, `cppcheck`, `scan-build`.
- **Tester (Human/Agent):** Validates regression safety. Dynamic tools: `pytest`, `Valgrind`.

## 3. Mandatory Toolchain Commands
The Auditor and Tester roles must use these exact commands to generate evidence-based findings.

- **Linting & Modernization:** `clang-tidy src/*.c -p .`
- **Static Analysis:** `cppcheck --enable=all --inconclusive --std=c99 -I include .`
- **Logic Path Analysis:** `make clean && scan-build make`
- **Memory/Runtime Analysis:** `valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind.txt ./build/ytree`

## 4. Continuous Audit Loop (Default)
Run this loop for every non-trivial change and every PR.

### Phase A: Architecture Check (Architect)
- Confirm scope, invariants, and acceptance criteria.
- Identify modules/symbols affected and known risk areas (e.g., UI redraw, pointer arithmetic).

### Phase B: Fix (Developer)
- **Real-time Guard:** `clangd` must be active. The Developer must resolve all LSP diagnostics before proceeding.
- **Task:** Implement fixes or features using atomic prompts. Iterate on the prompt (not the code) until functional requirements are met.

### Phase C: Audit (Code Auditor)
- **Lint Gate:** Run `clang-tidy src/*.c -p .`. Any Blocker/High finding is an automatic REJECT.
- **Static Gate:** Run `cppcheck --enable=all --inconclusive --std=c99 -I include .`. Any Blocker/High finding is an automatic REJECT.
- **Logic Gate:** Run `scan-build`. Any "Null Pointer Dereference" or "Garbage Value" is an automatic REJECT.
- **Rules:** Provide file:line and evidence for every finding. Prefer root-cause fixes over "if(ptr)" symptom patches.
- **Tracking Rule:** Medium/Low findings must be either fixed in the PR or explicitly tracked with rationale.

### Phase D: Verify (Tester)
- **Dynamic Gate:** Run relevant `pytest` suites for the touched scope.
- **Memory Gate:** Run `valgrind --leak-check=full --error-exitcode=1 [test_command]`.
- **Criteria:** Failure occurs if any test fails OR if Valgrind returns a non-zero exit code (leaks/corruption detected).

### Phase E: Merge Gate (Code Auditor)
- Re-audit the final PR diff before merge.
- **Exit Criteria:** Blocker = 0, High = 0, scan-build = Green for scoped analysis, and verification evidence attached.

## 5. Release Gate (Final Checkpoint)
Before a release tag/cut, run a full audit pass across the release scope:
- Full `pytest` suite.
- Fresh `scan-build` run.
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

**Technical Note:** To enable the Developer's LSP, ensure `bear -- make` is run in the WSL environment to maintain an up-to-date `compile_commands.json`.
