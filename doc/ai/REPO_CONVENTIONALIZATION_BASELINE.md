# Repository Conventionalization Baseline

## Scope Note and Drift Observation
Packet 1 requests a compliance matrix against "Master Brief" sections 1-11. No in-repo file explicitly named or labeled "Master Brief" was found during repository scan. For this baseline, sections 1-11 are mapped to the in-repo normative proxy: `.ai/shared.md` "Core Engineering Rules" items 1-11 (`.ai/shared.md:75-85`).

This is a packet-assumption drift condition and is reflected in `report1.txt` final status.

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
| 1 | Prioritize architectural stability, memory safety, maintainability | partially satisfied | Requirement codified (`.ai/shared.md:75`), architecture constraints documented (`doc/ARCHITECTURE.md:21-33`), but unsafe APIs still present (`src/cmd/view.c:51`, `src/ui/ctrl_file_ops.c:597`, 172 total `strcpy/strcat/sprintf` matches in `src/` scan). |
| 2 | Do not use unsafe string APIs (`strcpy`, `sprintf`) | missing | Policy exists (`.ai/shared.md:76`), direct violations present (`src/cmd/view.c:51`, `src/util/path_utils.c:32`, `src/ui/ctrl_file_ops.c:597`). |
| 3 | Preserve architectural invariants (explicit context, panel isolation, deterministic single-thread) | fully satisfied | Policy (`.ai/shared.md:77`), hard architecture rule (`doc/ARCHITECTURE.md:25-33`), panel isolation and directional protocol (`doc/ARCHITECTURE.md:87-97`). |
| 4 | Prefer root-cause fixes, avoid superficial patching | fully satisfied | Policy (`.ai/shared.md:78`), QA remediation hard rules (`doc/ai/WORKFLOW.md:64-74`), dedicated remediation skill (`.ai/skills/qa-root-cause-remediation/SKILL.md:10-16`). |
| 5 | Do not guess behavior; consult docs before architecture changes | fully satisfied | Policy (`.ai/shared.md:79`), anti-guessing debug rule (`doc/ai/WORKFLOW.md:215-220`), contributor doc points to canonical workflow/spec references (`doc/CONTRIBUTING.md:5-8`, `.ai/shared.md:106-117`). |
| 6 | Keep changes scoped to requested task | fully satisfied | Policy (`.ai/shared.md:80`), atomic mission rule (`doc/ai/WORKFLOW.md:25`), contributor guidance for small focused PRs (`doc/CONTRIBUTING.md:163`). |
| 7 | Prefer MCP semantic/navigation tools over broad file loading | fully satisfied | Policy (`.ai/shared.md:81`), semantic context requirement (`doc/ai/WORKFLOW.md:200`), token-economy targeted retrieval guidance (`doc/ai/WORKFLOW.md:244-255`). |
| 8 | Use Conventional Commits style for commit messages | partially satisfied | Requirement is explicit in AI shared policy (`.ai/shared.md:82`), but contributor-facing submission doc only requires "clear, declarative" message (`doc/CONTRIBUTING.md:167`) and does not restate Conventional Commit format. |
| 9 | Minor corrections should amend previous commit when same logical unit | fully satisfied | Explicitly defined in shared policy (`.ai/shared.md:83`) and workflow (`doc/ai/WORKFLOW.md:31`). |
| 10 | Treat user goals as authoritative but challenge weak/nonstandard details | fully satisfied | Explicit in shared policy (`.ai/shared.md:84`) and expanded convention-check behavior (`doc/ai/WORKFLOW.md:34-50`). |
| 11 | Strict bugfix red-green: failing regression first, then fix, then green proof | partially satisfied | Explicit in shared policy (`.ai/shared.md:85`), workflow (`doc/ai/WORKFLOW.md:220`), and bugfix skill (`.ai/skills/bugfix-red-green-proof/SKILL.md:12-16`); contributor-facing top-level docs do not consistently surface this as a universal bugfix gate. |

### Matrix Totals
- fully satisfied: 7
- partially satisfied: 3
- missing: 1

## 3. Prioritized Gap List

### P0 (must-fix)
- Missing in-repo "Master Brief" artifact with explicit sections 1-11; baseline currently relies on inferred proxy mapping, creating governance drift.
- Shared Rule #2 is violated broadly by current code (`strcpy`/`strcat`/`sprintf` occurrences in runtime C paths), leaving policy-to-code mismatch.

### P1 (important)
- Conventional Commits requirement exists in AI policy but is not mirrored in contributor submission instructions (`doc/CONTRIBUTING.md`).
- Strict red-green bugfix gate is strong in AI docs/skills but not equally explicit in contributor-facing non-AI process docs.
- Provider adapter asymmetry: `AGENTS.md` carries richer operational constraints than `CLAUDE.md`/`GEMINI.md`, increasing drift risk across providers.

### P2 (polish/optimization)
- No single "AI governance index" document that maps stubs -> canonical provider files -> shared policy -> skills in one page.
- No lightweight automated lint/check to detect policy drift between root stubs and canonical `.ai/*` provider files.

## 4. First-Pass Migration Sequence

1. Establish canonical Master Brief artifact (`doc/ai/MASTER_BRIEF.md`) with explicit sections 1-11 and normative ownership.
Dependency/order rationale: smallest safe first step; unblocks unambiguous compliance scoring.
Rollback: remove new file and revert references if structure/content is disputed.

2. Align contributor-facing governance wording with canonical policy (Conventional Commits + red-green bugfix gate) in `doc/CONTRIBUTING.md` and cross-link to `doc/ai/WORKFLOW.md`.
Dependency/order rationale: depends on Step 1 canonical wording; prevents duplicate/contradictory language.
Rollback: revert doc edits only; no runtime impact.

3. Normalize provider adapter parity: keep root stubs minimal and ensure all provider-specific details live in `.ai/{codex,claude,gemini}.md` with shared critical constraints synchronized.
Dependency/order rationale: after policy wording is settled, adapter files can be harmonized once.
Rollback: restore previous stub/provider docs from git if external tooling depends on current phrasing.

4. Create and execute a safe-string migration plan for runtime C code (replace `strcpy/strcat/sprintf` with bounded APIs and helper wrappers), with staged regression coverage.
Dependency/order rationale: policy and documentation must be stable first; runtime hardening then proceeds module-by-module.
Rollback: revert module-level migrations independently; preserve tests introduced for each converted module.

5. Add a lightweight docs/policy drift check (CI or local script) to verify stub pointers, provider parity, and core policy anchors.
Dependency/order rationale: guardrail added after normalization to keep future drift low.
Rollback: disable/remove check if false positives block workflow; keep core docs unchanged.

## 5. Explicit Preserve-As-Is List
- Keep the central policy model in `.ai/shared.md` as the single cross-provider authority.
- Keep persona/skill separation (`.agent/rules/*` for role boundaries, `.ai/skills/*` for procedures); this is clear and maintainable.
- Keep cross-cutting skill auto-load mapping structure; it encodes operational gates effectively.
- Keep `doc/ai/WORKFLOW.md` as the detailed execution contract and `doc/AUDIT.md` as canonical gate procedure.
- Keep root discovery-stub pattern (`AGENTS.md`, `CLAUDE.md`, `GEMINI.md`) that points to canonical `.ai/*` files; only reduce asymmetry/drift.
