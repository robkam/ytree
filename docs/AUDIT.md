# AUDIT.md

## 1. Purpose
This document defines the mandatory quality process for the Ytree modernization project. Auditing is an ongoing process that starts during implementation and continues through merge and release. The release gate is the final checkpoint.

## 1.1 Cadence
- Use focused checks during feature-sized change or PR iteration.
- Before merge to `main`, require green PR full-QA CI (`make qa-all` equivalent). Local full audit loop is optional unless explicitly requested by the maintainer.
- For feature-sized changes, include explicit `make qa-module-boundaries` evidence (controller allowlists + growth budgets) in the verification notes.
- Always run the merge/release gate before merge/release.
- Do not run the full gate after every prompt-level micro-edit unless risk justifies it.

## 1.2 QA Layers

The project uses six QA layers with increasing depth and cost:

| Layer | Command | What it checks | When to run |
|---|---|---|---|
| CI Gate | `git push` (automatic) | Draft-PR baseline confidence checks (`qa-code-quality` + `qa-fileops-integrity`, coverage pytest gate, fuzz gate) | Every push to `main` and every PR update targeting `main` (automatic) |
| PR Full QA CI (required) | `.github/workflows/full-qa.yml` (`make qa-all`) | clang-tidy, cppcheck, scan-build, Valgrind smoke (`--version`), full `pytest`, unsafe API guard, gitleaks, module-boundary guard, ai-config guard, fuzz guard | Must be green before merge to `main` |
| Fileops Integrity Gate | `make qa-fileops-integrity` | Deterministic file/archive mutation integrity + security regression checks (copy/move/delete/rename/archive rewrite, cancel/failure safeguards, shell/tempfile hardening contracts) | Before merge and when touching file/archive mutation flows |
| Sanitizer QA | `make qa-sanitize` | Main ytree build + `pytest` under AddressSanitizer/UndefinedBehaviorSanitizer | Before release, after memory/UB-sensitive changes, or when triaging suspicious crashes |
| Deep Audit | `make qa-valgrind-full` | Automated interactive Valgrind Memcheck session (leak, uninit, FD, use-after-free checks) | Before release, after major refactoring, or periodically |
| Manual Feature Audit | `make qa-valgrind-interactive` | You manually drive ytree under Valgrind to exercise new feature code paths | After adding a major new feature |

- **CI Gate** runs automatically on push via GitHub Actions. No developer action needed.
- **PR Full QA CI** (`.github/workflows/full-qa.yml`, `make qa-all` equivalent) is the standard pre-merge gate.
- **Local QA** (`make qa-all`) is optional for faster local confidence and maintainer-requested deep preflight; avoid running it on every iteration by default.
- **Fileops Integrity Gate** (`make qa-fileops-integrity`) is the dedicated regression wall for mutation integrity/security contracts; run it before merge and whenever file/archive mutation code or prompts change.
- **Deep Audit** (`make qa-valgrind-full`) is on-demand. It drives a scripted interactive ytree session under Valgrind and takes ~2-3 minutes. Run it:
  - Before tagging a release
  - After changes to memory management, allocation, or cleanup paths
  - After major refactoring sessions
  - Periodically as a health check
  - After reviewing the results, remove `valgrind.log` from the repository root when it is no longer needed
- **Sanitizer QA** (`make qa-sanitize`) is on-demand and complementary to Valgrind. It builds the main binary with ASan/UBSan and runs `pytest` with fail-fast sanitizer settings. Run it:
  - After touching pointer arithmetic, allocation/free paths, or integer-heavy logic
  - When triaging intermittent or environment-sensitive crashes
  - Before alpha/release candidates as an extra memory/UB gate
- **Manual Feature Audit** (`make qa-valgrind-interactive`) launches ytree under Valgrind for you to drive manually. Use it after adding a major feature to exercise the new code paths specifically. Exit cleanly, then inspect `valgrind.txt`.

## 1.3 Gate Organization & Efficiency

This section defines how to keep QA gates efficient during iteration without reducing safety.
Scope is strict: QA/check/test organization and efficiency only. Non-QA workflow-policy edits (for example PR title/body wording rules) are out of scope here and must be tracked in a separate task/PR.

### Branch + PR Workflow (Mandatory)

- Never push directly to `main`.
- Always use a non-`main` branch and PR workflow.
- If work was committed locally on `main`, create a branch from that current `HEAD` before first push.

### Hybrid PR Cadence (Mandatory)

- Before first push, run a quick local gate (`make`, plus targeted smoke/tests for touched scope).
- Open a draft PR early; draft PR checks may be red while iterating.
- Do not request review while draft checks are red.
- Before merge to `main`, require green PR full-QA CI (`make qa-all` equivalent) plus required loop evidence from this document; local full audit reruns are optional unless the maintainer explicitly requests them.
- Before merge, require green PR checks and reviewer signoff.

### Required Gate Tiers

| Tier | Owner | Trigger | Required checks | Non-overlap default intent |
|---|---|---|---|---|
| Tier A (local fast iteration) | Developer | During implementation before first push and between risky edits | `make`; targeted pytest for touched scope; targeted guards (`qa-unsafe-apis`, `qa-fileops-integrity`) when relevant | Keep iteration fast; avoid full-suite duplication unless local risk demands it |
| Tier B (draft PR baseline CI) | CI + PR author | Every push while PR is draft | `ci-baseline` (`qa-code-quality` + `qa-fileops-integrity` + `qa-pytest-coverage` + `qa-fuzz`) | Provide baseline branch-protection signal (includes coverage by design); do not duplicate Tier C full local gate content |
| Tier C (pre-merge full gate) | CI + PR author | Before merge to `main` (or earlier only when explicitly requested) | Green PR full-QA CI (`.github/workflows/full-qa.yml`, `make qa-all` equivalent); optional local `make qa-all-log` evidence when maintainer-requested; plus explicit `qa-fileops-integrity` evidence when mutation workflows changed | Use CI as canonical full-gate signal; avoid duplicate local full-gate reruns during routine iteration |
| Tier D (merge/release gate) | Maintainer + reviewer | Before merge and before release/tag cut | Branch-protection checks green; reviewer signoff; `qa-sanitize`; `qa-valgrind-full` for release-risk changes; `qa-valgrind-interactive` after major feature flows | Reserve deepest runtime checks for merge/release assurance to avoid slowing every draft iteration |

### branch-protection and PR State Criteria (Mandatory)

- **Draft PR:** Branch exists and PR stays draft. Red checks are acceptable while iterating. No review requests.
- **Ready for review:** Tier B branch-protection checks are green and no unresolved blocker findings remain. Tier C may be deferred to pre-merge unless explicitly requested earlier.
- **Merge:** All required branch-protection checks are green, reviewer signoff is present, and Tier D evidence is attached for the current diff/risk.
- Required branch-protection checks (sync with current workflows):
  - `.github/workflows/ci.yml`: `Guard fuzz harness sync`
  - `.github/workflows/ci.yml`: `File mutation integrity gate`
  - `.github/workflows/ci.yml`: `Full coverage baseline gate`
  - `.github/workflows/ci.yml`: `Fuzz baseline gate`
  - `.github/workflows/pr-conflict-assistant.yml`: `Up To Date With Main`

### Duplication Control Policy

- Keep all beneficial checks, but avoid running overlapping suites in the same gate unless explicitly justified.
- Prefer targeted suites for iteration cadence and reserve full-suite runs for review/merge gates.
- Any gate optimization must preserve equivalent or stronger defect-detection coverage.

### Efficiency Rules for Tests

- Prefer parametrized tests and shared helpers over near-duplicate test bodies.
- Replace fixed sleeps with deterministic condition/poll-based waits where possible.
- Treat flaky behavior as a root-cause issue; do not weaken gates with silent skips/suppressions.

### Fail-Fast Ordering Guidance

Within a gate, prefer this order:

1. Cheap high-signal policy checks (guards/scripts).
2. Targeted pytest suites for touched risk areas.
3. Full pytest (when required by the gate).
4. Static analyzers and deep runtime analyzers.

### Runtime Budget + Trend Reporting (Mandatory)

Track these metrics per PR and as rolling team trends:

- **median draft feedback time:** median elapsed time from draft-PR push to first CI result.
- **time-to-green:** elapsed time from first draft push to all required branch-protection checks passing.
- **full-gate runtime:** runtime for full local/CI gates (`make qa-all-log`, plus sanitizer/deep gates when invoked).
- **flake rate:** reruns caused by non-deterministic failures ÷ total gate runs.

Report metrics in PR notes or release prep notes with before/after deltas when gate organization changes.

Provisional targets (make completion auditable; tighten over time as data improves):

- **Tier B p50 runtime target:** <= 30 minutes per draft-PR push.
- **Full-gate p50 runtime target:** <= 60 minutes for `make qa-all-log` evidence runs.
- **Max acceptable flake rate:** <= 5% of gate runs requiring rerun due to non-determinism.
- **Target time-to-green:** <= 120 minutes from first draft push to required checks all green.

### Rollout, Rollback, and Risk Register (Mandatory)

Rollout criteria:

- Every check listed in Tier B/C/D remains present or has documented equivalent/stronger replacement.
- At least one full Ready-for-review cycle demonstrates complete evidence flow with no coverage regressions.
- Trend metrics are captured for comparison against prior baseline.

Rollback criteria:

- Any safety-critical check is removed or bypassed without equivalent replacement.
- Flake rate or time-to-green regresses for consecutive cycles with no matching quality gain.
- A missed regression shows reduced signal quality caused by gate reorganization.

Risk register (coverage/signal preservation):

| Risk | Signal-quality guard | Rollback trigger |
|---|---|---|
| Fast-tier drift removes critical checks | Tier B/C/D matrix requires explicit named checks and owners | Any missing required check in CI/PR evidence |
| Runtime optimizations hide flaky behavior | Track flake rate and require deterministic repro/root-cause handling | Flake rate trend worsens without root-cause fixes |
| Deep runtime checks deferred too aggressively | Tier D requires sanitizer + deep Valgrind at merge/release cadence | Release/merge evidence lacks required deep-runtime gates |

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
6. **Secret Scanning:** `gitleaks detect --source . --redact --exit-code 1` (or `gitleaks dir --redact --exit-code 1 .` on newer CLI variants)

For step 4 in automated runs, drive a deterministic start/exit path (for example with `pexpect`) so Valgrind can finish and return an actionable exit code.

Local shortcut targets are available in the `Makefile`:
- `make qa-clang`
- `make qa-cppcheck`
- `make qa-scan`
- `make qa-valgrind`
- `make qa-valgrind-full`
- `make qa-valgrind-interactive`
- `make qa-sanitize`
- `make qa-pytest`
- `make qa-fileops-integrity`
- `make qa-unsafe-apis`
- `make qa-gitleaks`
- `make qa-module-boundaries`
- `make qa-ai-config`
- `make qa-code-quality` (runs `qa-unsafe-apis`, `qa-module-boundaries`, `qa-ai-config`)
- `make qa-fuzz`
- `make qa-all` (runs `qa-clang`, `qa-cppcheck`, `qa-scan`, `qa-valgrind`, `qa-pytest`, `qa-unsafe-apis`, `qa-gitleaks`, `qa-module-boundaries`, `qa-ai-config`, `qa-fuzz` in order; run `qa-fileops-integrity` separately when touching mutation flows)
- `make qa-all-log` (same as `qa-all`, with full output captured to `qa-all.log` in repo root; override with `QA_LOG=/path/to/file`)

For feature-sized/PR-scope changes, audit evidence must include a successful `make qa-module-boundaries` run so controller-slimming checks are explicitly validated.
Audit evidence must also include `make qa-unsafe-apis` results as explicit enforcement evidence for the shared Security gate policy in `.ai/shared.md` Core Engineering Rules.

GitHub CI is a baseline gate (including `qa-fileops-integrity`, coverage pytest, and `qa-fuzz`) and does not replace the full local audit loop.

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
- Fresh `make qa-sanitize` run (main binary + pytest under ASan/UBSan).
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
