# Repository Conventionalization Baseline

## Scope Note and Drift Observation
This baseline is an evergreen repository-state artifact for compliance against "Master Brief" sections 1-11. The repository source-of-truth brief artifacts are `/home/rob/ytree/master_brief_minimal.txt` and `/home/rob/ytree/doc/ai/MASTER_BRIEF.md`. The matrix body is preserved and uses the in-repo normative proxy mapping to `.ai/shared.md` "Core Engineering Rules" items 1-11 (`.ai/shared.md:75-85`).

## 1. Existing Assets Inventory

### 1.1 Current docs relevant to human contributors
- `README.md` (project overview, build/install/usage/contributing entry points; `README.md:1-136`).
- `doc/CONTRIBUTING.md` (setup, build, test, audit, submission and coding/style guidance; `doc/CONTRIBUTING.md:1-216`).
- `doc/SPECIFICATION.md` (behavior contract and module organization; `doc/SPECIFICATION.md:1-159`).
- `doc/ARCHITECTURE.md` (context-passing invariants, panel isolation, ownership model; `doc/ARCHITECTURE.md:19-110`).
- `doc/AUDIT.md` (mandatory audit loop, role mapping, gate cadence; `doc/AUDIT.md:1-97`).
- `doc/ai/WORKFLOW.md` (AI workflow policy, personas, models, debug/audit/token policy; `doc/ai/WORKFLOW.md:1-260`).
- `doc/ai/TESTING.md` (test naming/structure and fail-first workflow references; `doc/ai/TESTING.md:1-151`).
- `doc/ai/DEBUGGING.md` (root-cause and hands-off fail-first debugging procedures; `doc/ai/DEBUGGING.md:1-124`).
- `doc/USAGE.md` (generated user manual; source-of-truth note appears in AGENTS stub; `doc/USAGE.md:1-371`, `AGENTS.md:8`).
- `doc/ROADMAP.md` (long-range modernization plan with persona annotations; `doc/ROADMAP.md:1+`).
- `doc/FAQ.md`, `doc/CHANGES.md`, `doc/AUTHORS.md` (context/history/contributor reference).

### 1.2 Current `.ai` policy files
- `.ai/shared.md` (cross-agent policy, routing, skills, engineering rules, validation, references).
- `.ai/codex.md` (Codex-specific execution and pytest host-permission rule).
- `.ai/claude.md` (Claude-specific scope/build/test/architecture notes).
- `.ai/gemini.md` (Gemini-specific scope and behavioral notes).

### 1.3 Current persona/skill files
- Persona rule files (`.agent/rules/`):
  - `architect.md`
  - `developer.md`
  - `code_auditor.md`
  - `tester.md`
  - `greybeard.md`
- Skill files (`.ai/skills/*/SKILL.md`):
  - `architect-planning`
  - `developer-implementation`
  - `code-auditor-gate`
  - `tester-regression-design`
  - `greybeard-meta-guidance`
  - `bugfix-red-green-proof`
  - `full-audit-gate-c`
  - `qa-root-cause-remediation`
  - `pty-pexpect-debug`
  - `ncurses-render-safety`
  - `keybinding-collision-check`
  - `manpage-sync`
  - `ui-economy-navigation`
  - `ui-flow-offender-audit`

### 1.4 Current provider adapter files for Codex/Claude/Gemini
- Root discovery stubs:
  - `AGENTS.md` (Codex)
  - `CLAUDE.md` (Claude)
  - `GEMINI.md` (Gemini)
- Canonical provider files:
  - `.ai/codex.md`
  - `.ai/claude.md`
  - `.ai/gemini.md`
- Shared policy anchor for all providers:
  - `.ai/shared.md`

## 2. Compliance Matrix Against Master Brief Sections 1-11 (Proxy-Mapped)

| Section | Requirement (proxy from `.ai/shared.md` rules 1-11) | Status | Concrete File Evidence |
|---|---|---|---|
| 1 | Prioritize architectural stability, memory safety, maintainability | fully satisfied | Requirement codified (`.ai/shared.md:75`), architecture constraints documented (`doc/ARCHITECTURE.md:21-33`), and runtime unsafe-string regressions are now guarded by `scripts/check_c_unsafe_apis.py` wired into `Makefile` target `qa-unsafe-apis` and `qa-all`. |
| 2 | Do not use unsafe string APIs (`strcpy`, `sprintf`) | fully satisfied | Policy exists (`.ai/shared.md:76`), runtime `src/**/*.c` callsites are clean, and enforcement is automated via `scripts/check_c_unsafe_apis.py` and `make qa-unsafe-apis` (`Makefile`). |
| 3 | Preserve architectural invariants (explicit context, panel isolation, deterministic single-thread) | fully satisfied | Policy (`.ai/shared.md:77`), hard architecture rule (`doc/ARCHITECTURE.md:25-33`), panel isolation and directional protocol (`doc/ARCHITECTURE.md:87-97`). |
| 4 | Prefer root-cause fixes, avoid superficial patching | fully satisfied | Policy (`.ai/shared.md:78`), QA remediation hard rules (`doc/ai/WORKFLOW.md:64-74`), dedicated remediation skill (`.ai/skills/qa-root-cause-remediation/SKILL.md:10-16`). |
| 5 | Do not guess behavior; consult docs before architecture changes | fully satisfied | Policy (`.ai/shared.md:79`), anti-guessing debug rule (`doc/ai/WORKFLOW.md:215-220`), contributor doc points to canonical workflow/spec references (`doc/CONTRIBUTING.md:5-8`, `.ai/shared.md:106-117`). |
| 6 | Keep changes scoped to requested task | fully satisfied | Policy (`.ai/shared.md:80`), atomic mission rule (`doc/ai/WORKFLOW.md:25`), contributor guidance for small focused PRs (`doc/CONTRIBUTING.md:163`). |
| 7 | Prefer MCP semantic/navigation tools over broad file loading | fully satisfied | Policy (`.ai/shared.md:81`), semantic context requirement (`doc/ai/WORKFLOW.md:200`), token-economy targeted retrieval guidance (`doc/ai/WORKFLOW.md:244-255`). |
| 8 | Use Conventional Commits style for commit messages | fully satisfied | Requirement is explicit in AI shared policy (`.ai/shared.md:82`) and mirrored in contributor submission instructions with required format and examples (`doc/CONTRIBUTING.md:178-183`). |
| 9 | Minor corrections should amend previous commit when same logical unit | fully satisfied | Explicitly defined in shared policy (`.ai/shared.md:83`) and workflow (`doc/ai/WORKFLOW.md:31`). |
| 10 | Treat user goals as authoritative but challenge weak/nonstandard details | fully satisfied | Explicit in shared policy (`.ai/shared.md:84`) and expanded convention-check behavior (`doc/ai/WORKFLOW.md:34-50`). |
| 11 | Strict bugfix red-green: failing regression first, then fix, then green proof | fully satisfied | Explicit in shared policy (`.ai/shared.md:85`), workflow (`doc/ai/WORKFLOW.md:220`), bugfix skill (`.ai/skills/bugfix-red-green-proof/SKILL.md:12-16`), and contributor-facing bugfix gate requirements (`doc/CONTRIBUTING.md:187-193`). |

### Matrix Totals
- fully satisfied: 11
- partially satisfied: 0
- missing: 0

## 3. Prioritized Gap List

### P0 (must-fix)
- No open P0 gaps. Runtime unsafe-string governance is now enforced by `scripts/check_c_unsafe_apis.py`, integrated as `qa-unsafe-apis` in `Makefile` and included in `qa-all`.

### P1 (important)
- No open P1 governance gaps remain; Master Brief presence, contributor Conventional Commits guidance, red-green contributor gate wording, and provider root-stub shared metadata parity remain resolved (`doc/ai/MASTER_BRIEF.md:1-170`, `doc/CONTRIBUTING.md:178-193`, `AGENTS.md:3-47`, `CLAUDE.md:3-46`, `GEMINI.md:3-46`).

### P2 (polish/optimization)
- Governance index gap is resolved: `doc/ai/GOVERNANCE_INDEX.md` now provides a canonical map for root stubs, provider files, shared policy, persona/skill separation, and edit-target ownership (`doc/ai/GOVERNANCE_INDEX.md:1-64`), and `doc/ai/WORKFLOW.md` now points to it directly (`doc/ai/WORKFLOW.md:4`).
- Governance drift-check gap is resolved: `scripts/check_ai_governance_drift.py` now enforces stub invariants (provider pointer, shared pointer, docs-note line, persona markers, UX/QA gate markers) across `AGENTS.md`, `CLAUDE.md`, and `GEMINI.md` (`scripts/check_ai_governance_drift.py:21-87`).
- Runtime unsafe-string guardrail is now continuous: `scripts/check_c_unsafe_apis.py` is callable directly and enforced via `make qa-unsafe-apis` within `qa-all` (`Makefile`).

## 4. First-Pass Migration Sequence

1. Establish canonical Master Brief artifact (`doc/ai/MASTER_BRIEF.md`) with explicit sections 1-11 and normative ownership. **Completed.**
Completion note: canonical in-repo brief exists and is populated for sections 1-11 (`doc/ai/MASTER_BRIEF.md:1-170`).
Rollback: remove new file and revert references if structure/content is disputed.

2. Align contributor-facing governance wording with canonical policy (Conventional Commits + red-green bugfix gate) in `doc/CONTRIBUTING.md` and cross-link to `doc/ai/WORKFLOW.md`. **Completed.**
Completion note: contributor submission section now requires Conventional Commits and includes examples; bugfix gate now requires fail-before-fix and post-fix green proof (`doc/CONTRIBUTING.md:178-193`).
Rollback: revert doc edits only; no runtime impact.

3. Normalize provider adapter parity: keep root stubs minimal and ensure all provider-specific details live in `.ai/{codex,claude,gemini}.md` with shared critical constraints synchronized. **Completed for shared metadata parity.**
Completion note: root stubs now mirror shared discovery metadata (canonical pointer, shared pointer, docs note, persona/skill mappings, UX/QA gates) across providers; Codex-only pytest host-permission note remains an intentional provider-specific operational detail (`AGENTS.md:3-47`, `CLAUDE.md:3-46`, `GEMINI.md:3-46`).
Rollback: restore previous stub/provider docs from git if external tooling depends on current phrasing.

4. Create and execute a safe-string migration plan for runtime C code (replace `strcpy/strcat/sprintf` with bounded APIs and helper wrappers), with staged regression coverage. **Completed.**
Completion note: runtime `src/**/*.c` callsites are now clean for banned APIs, and regression prevention is automated through `scripts/check_c_unsafe_apis.py` plus `Makefile` target `qa-unsafe-apis` in `qa-all`.
Dependency/order rationale: policy and documentation were stabilized first; runtime hardening and enforcement then closed the lane durably.
Rollback: revert module-level migrations independently if needed; keep or reintroduce guardrails only with explicit policy decision.

5. Add a lightweight docs/policy drift check (CI or local script) to verify stub pointers, provider parity, and core policy anchors, with governance index anchoring for canonical edit targets. **Completed.**
Completion note: governance index is present (`doc/ai/GOVERNANCE_INDEX.md:1-64`), workflow now links to it (`doc/ai/WORKFLOW.md:4`), and drift check script exists with concrete invariant checks for root stubs (`scripts/check_ai_governance_drift.py:21-87`).
Dependency/order rationale: guardrail and canonical index now operate together to keep future drift low after normalization.
Rollback: disable/remove check if false positives block workflow; keep core docs unchanged.

## 5. Explicit Preserve-As-Is List
- Keep the central policy model in `.ai/shared.md` as the single cross-provider authority.
- Keep persona/skill separation (`.agent/rules/*` for role boundaries, `.ai/skills/*` for procedures); this is clear and maintainable.
- Keep cross-cutting skill auto-load mapping structure; it encodes operational gates effectively.
- Keep `doc/ai/WORKFLOW.md` as the detailed execution contract and `doc/AUDIT.md` as canonical gate procedure.
- Keep root discovery-stub pattern (`AGENTS.md`, `CLAUDE.md`, `GEMINI.md`) that points to canonical `.ai/*` files; preserve shared metadata parity and keep provider-specific exceptions explicit.
