# AI-Assisted Development Workflow

This document defines the standards and processes for using AI agents to maintain and modernize the `ytree` codebase. These rules ensure architectural integrity and prevent "hallucination debt."
For canonical governance file ownership and edit targets, see [GOVERNANCE.md](GOVERNANCE.md).

---

## 1. Core Principles

### 1.1 The Golden Loop (Spec-First Integrity)
The development process is strictly hierarchical. The Spec is the "Contract of Truth."
1.  **Write/Verify Spec:** Does `SPECIFICATION.md` describe the desired behavior?
2.  **Write Test:** Create a test case that fails if the behavior is missing (Red).
3.  **Implement:** Write C code to satisfy the test (Green).
4.  **Refactor:** Improve code structure without breaking the test.

**CRITICAL RULE:** If the implementation behaves differently than the Spec, **the implementation is wrong.**
*   **Allowed:** Rewrite the C code to match the Spec.
*   **Allowed:** Fix the Spec if (and only if) the Spec was logically flawed.
*   **FORBIDDEN:** Changing the Test to match the "working" code if it violates the Spec.

### 1.2 General Rules
1.  **Architecture as Blueprint:** [ARCHITECTURE.md](../ARCHITECTURE.md) defines the technical structure.
2.  **Testing as Standard:** [TESTING.md](TESTING.md) defines test naming, structure, and harness rules. All test code must conform to it.
3.  **Human as Architect:** The AI acts as a partner with varying levels of specialization (see Section 2). The human maintainer provides final architectural direction.
4.  **Atomic Missions:** One work item (bug or task) = One session.
5.  **Build-First Verification:** No code is accepted until it compiles and passes tests.
6.  **The "Clean Slate" Rule:** If an AI generates fragile code or follows a wrong path, do not attempt to "patch" the mistake via further dialogue. Revert the changes (`git restore .`), rephrase the original prompt from a different perspective, and restart the mission. This is mandatory because:
    *   **Context Clearing:** It removes the "bad" code and the flawed logic that led to it.
    *   **New Perspective:** Rephrasing prevents the model from staying stuck in a loop of bad assumptions.
    *   **Clean Implementation:** It ensures the final code is the result of a single, clean logical flow rather than a series of ad-hoc patches.
7.  **Minor Step Corrections Should Amend:** If the immediately preceding step only needs a small correction and should remain the same logical history unit, update it with `git commit --amend --no-edit` rather than adding a trivial follow-up fix commit. Create a new commit only when the correction is meaningfully distinct, delayed enough to matter historically, or worth preserving separately in the project history.
8.  **Intent Over Literal Wording:** Treat the human maintainer as authoritative on goals, but not automatically on exact UI wording, naming, key choices, menu structure, or workflow details. If a requested detail does not follow convention or best practices, or conflicts with existing Ytree patterns, the AI must say so explicitly and recommend the stronger conventional approach before implementing it.

### 1.3 Convention & Best-Practice Check

The maintainer may sometimes provide a very specific interaction detail while still intending "do what users would expect." Agents must not assume that specificity means correctness.

When a prompt includes concrete UI or workflow instructions, explicitly evaluate them against:

1.  **Established Ytree behavior:** You MUST NOT break local consistency without explicit, strong justification.
2.  **Lineage expectations:** Preserve XTree/ZTree muscle memory where that is clearly part of the feature intent.
3.  **Broader convention:** You MUST follow Linux/TUI conventions and common user expectations for prompts, menus, help, and key behavior.
4.  **Best practices:** Favor clarity, safety, and maintainability over clever but nonstandard interaction ideas.

Required agent behavior:

*   **Preserve the goal, question the detail:** Keep the user's intended outcome, but challenge interface details that appear nonstandard or weak.
*   **Recommend before encoding:** State the better conventional or best-practice choice before baking the literal wording into specs, prompts, or code.
*   **Explain the tradeoff:** When deviating from the user's literal wording, explain why the recommended version is stronger.
*   **Do not silently comply with weak specifics:** You MUST NOT turn a guessed interaction detail into lasting project behavior without explicit user review.

### 1.4 UX Economy Gate (Mandatory)

Interactive flows must minimize interruption and decision depth.

Hard rule:

*   Common path should be `key -> Enter -> result`.
*   Maximum submenu depth on common path is 1.
*   If a flow exceeds one submenu, include explicit justification and provide an equivalent fast path.

This gate applies to architecture proposals, implementations, and code-audit findings.

### 1.5 QA Remediation Gate (Mandatory)

When a QA gate fails (build, static analysis, sanitizer, valgrind, or pytest), remediation must target root cause rather than symptoms.

Hard rule:

*   Fix the underlying defect causing the failure.
*   Do not patch around failures by weakening behavior or bypassing failing paths.
*   Do not add local suppressions (`NOLINT`, `xfail`, `skip`, ignore lists, disable flags) as a shortcut.
*   Test-only edits are allowed only when the test is demonstrably wrong against the Spec.
*   If temporary suppression is the only safe short-term option, discuss with the maintainer first and get explicit approval with a rollback plan.

---

## 2. Shared Personas (The `.agent/rules/` directory)

The project maintains a set of "Persona Rules" in the `.agent/rules/` directory. These are Markdown files that define the behavior, constraints, and technical standards for different types of AI tasks.

*   **`architect.md`**: Used for planning and architecture design. Defines task boundaries and invariants without coding.
*   **`developer.md`**: Used for implementation. Applies approved changes while preserving architecture and safety constraints.
*   **`code_auditor.md`**: Used for adversarial quality review, fragility detection, and pass/fail gate findings.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.
*   **`greybeard.md`**: Advisory persona for general engineering guidance, convention checks, and practical best-practice sanity checks. This is not a mandatory gate role.

### 2.1 Persona Activation and Skill Auto-Load

Use explicit persona switching in prompts:

*   Full form: `:at architect`, `:at developer`, `:at code_auditor`, `:at tester`, `:at greybeard`
*   Short form: `:at a`, `:at d`, `:at c`, `:at t`, `:at g`
*   Parse guardrail: persona switching triggers only when `:` is in column 1 and `:at` occupies columns 1-3 (`:at ...` at line start).
*   Default when no explicit switch is `architect`.
*   Assistant responses should begin with `<name>:`.

Skills are auto-loaded by active persona from `.ai/skills/*/SKILL.md` (no extra user command needed):

*   `architect` -> `architect-planning`
*   `developer` -> `developer-implementation`
*   `code_auditor` -> `code-auditor-gate`
*   `tester` -> `tester-regression-design`
*   `greybeard` -> `greybeard-meta-guidance`

Optional explicit skill controls:

*   `use skill <skill-name>` to force-load a skill
*   `skip skill <skill-name>` to suppress a skill
*   `only skill <skill-name>[,<skill-name>...]` to load only specific skills
*   `reset skills` to clear explicit overrides and return to defaults

Cross-cutting skill auto-load:

*   Bugfix work -> `bugfix-red-green-proof`
*   Feature-sized, major, and PR-update work -> `full-audit-gate-c`
*   QA-failure remediation work -> `qa-root-cause-remediation`
*   PTY/pexpect flake debugging -> `pty-pexpect-debug`
*   Ncurses rendering/redraw/color work -> `ncurses-render-safety`
*   Keybinding/menu/help key changes -> `keybinding-collision-check`
*   Manpage/usage doc sync tasks -> `manpage-sync`
*   UI workflow/menu-depth/interaction-economy design -> `ui-economy-navigation`
*   UI prompt-chain offender detection/audit -> `ui-flow-offender-audit`
*   Code-quality audit/remediation (clean-code/deslop-style) -> `code-quality` (prompt guide: [CODE_QUALITY.md](CODE_QUALITY.md))
*   AI-writing-tell and prose de-slop tasks -> `ai-writing-tells`

Skill precedence (highest to lowest):

*   `only skill ...`
*   `use skill ...` and `skip skill ...`
*   Persona mapping
*   Cross-cutting auto-load

### 2.2 Dedup Contract (Persona vs Skill vs Docs)

Use this strict separation to avoid instruction drift:

1.  **Personas:** role boundaries, judgment posture, and communication style only.
2.  **Skills:** repeatable step-by-step procedures and checklists.
3.  **Docs:** policy, behavior contracts, and pointers to the right skills.

If procedural instructions appear in persona files, move them into skills and leave only a pointer.

---

## 3. The Agentic Loop

1.  **Direct Execution:** Run edits, builds, and tests in-repo to avoid manual copy/paste workflows.
2.  **Model Routing Principle:** Choose the available model/runtime by task risk and uncertainty: lower capability for mechanical edits, higher capability for ambiguous or high-impact decisions. Keep this as a capability decision, not a provider-name decision.
3.  **Semantic Context:** You MUST use **Serena** (symbol navigation) and **jCodeMunch** (repo indexing/search) for targeted context retrieval. You MUST NOT use raw bash tools like grep or find.
4.  **Cross-File State:** Keep terminal state, open-file context, and task history in one working loop.

### 3.1 Stateless Multi-AI Delivery Workflow (Non-Trivial Missions)

Use this workflow when a tracked bug or task needs architect-supervised implementation.
Prompt template:
- **[PROMPT_TEMPLATE.md](PROMPT_TEMPLATE.md)**: one architect-led entrypoint. The architect decides whether the work is a single developer/auditor unit or must be split into atomic units. If the work likely creates a large PR, the architect must explain why it is large, why it should or should not be split, and what could break once before dispatching implementation, repeating only if scope materially changes.

##### 3.1.0.1 MCP Config Bootstrap (Recommended)

To ensure local AI tooling picks up project defaults, run:

```bash
make mcp-doctor FIX=1
```

This bootstraps local MCP client configuration from repo-tracked defaults when missing, while preserving personal auth/session/history settings.
Principle: keep team-shared defaults in repo config and keep user-specific paths, credentials, and local overrides in user-local config.

##### 3.1.0.2 Periodic Codex/AI Tooling Refresh (Recommended)

If you use the Codex/AI workflow, run this checklist periodically from the repository root:

```bash
# Tether (pull + reinstall)
./scripts/tether-update.sh

# uvx-managed tools: force fresh resolve + latest
uvx --refresh codex-lb@latest --help
uvx --refresh --from git+https://github.com/oraios/serena@main serena --help
uvx --refresh jcodemunch-mcp@latest --help

# GitHub MCP Docker image (only if you use that MCP server)
docker pull ghcr.io/github/github-mcp-server:latest

# Optional MCP config health check
make mcp-doctor
# If drift/missing config is reported:
make mcp-doctor FIX=1

# Optional: refresh pinned helper-script requirements
source .venv/bin/activate
pip-compile --upgrade -o scripts/requirements.txt scripts/requirements.in
```

Fuzz harnesses under `tests/fuzz/` are hand-maintained source files (not generated). Keep them in sync with their target modules:

- `tests/fuzz/fuzz_string_utils.c` -> `src/util/string_utils.c`
- `tests/fuzz/fuzz_path_utils.c` -> `src/util/path_utils.c`
- `tests/fuzz/fuzz_filter_core.c` -> `src/fs/filter_core.c`
- `tests/fuzz/fuzz_common.c` / `tests/fuzz/fuzz_common.h` -> shared helper layer used by all fuzz harnesses

When you change any of those target modules, update the matching fuzz harness(es) in the same change and run:

```bash
make qa-fuzz
```

#### 3.1.1 Workflow Contract (Mandatory)

1.  This workflow is mandatory for non-trivial missions.
2.  Work is executed as numbered work items that are:
    *   atomic and independently verifiable,
    *   not fragmented into trivial micro-steps,
    *   executed one work item at a time.
3.  Prompt/report artifacts used for handoff are workflow artifacts and MUST NOT be committed.

#### 3.1.2 Mission Definition Pass (Stateless Planning)

1.  Run a stateless planning session to define mission scope, constraints, and acceptance criteria.
2.  Output must include a prompt for a stateless `architect` pass.

#### 3.1.3 Architect Pass (Stateless, Branch Setup)

1.  Start on a dedicated feature branch (local + remote).
2.  Architect emits exactly one runnable developer unit at a time.
3.  Every unit definition must include:
    *   strict scope lock,
    *   acceptance criteria,
    *   verification commands,
    *   blocker conditions,
    *   expected completion report path (for example `/tmp/report<id>.txt`).
4.  Architect status updates to maintainer must be delta-only and include concrete evidence handles.

#### 3.1.4 Developer Pass (Single Unit)

1.  Developer executes one scoped unit and produces a completion report.
2.  Verification cadence inside one atomic unit remains mandatory:
    *   initial pass: full verification set listed for the unit,
    *   correction/rework pass: rerun failing checks + directly impacted targeted tests,
    *   avoid full `make qa-all` during routine iteration unless maintainer explicitly requests it,
    *   rely on failing-check reruns + directly impacted targeted tests between implementation steps.
5.  Worker MUST NOT mark unit complete while required checks are failing.
6.  On success/failure/timeout, worker MUST emit explicit event log entries (no silent loops).
7.  Developer status line to maintainer must be delta-only: net-new state + next action + changed handles only.
8.  Developer/architect relay updates are facts-first:
    *   state completed work in past tense before planned next actions,
    *   include concrete evidence handles for each completion event (`report_handle`, event seq, command excerpt),
    *   keep updates delta-only: net-new state + next action + changed handles.

#### 3.1.5 Auditor Pass (Single Unit)

1.  Auditor runs only after developer evidence is available.
2.  Auditor workflow is evidence-first:
    *   validate code diff + verification evidence first,
    *   rerun commands only when evidence is incomplete, contradictory, or risk is high.
3.  Correction/rework iterations are separate atomic units and must be re-audited.
4.  Auditor output must include explicit pass/fail decision and severity-ranked risks.

#### 3.1.6 Architect Validation, Commit, and Cleanup

1.  Architect validates durable run state plus developer/auditor evidence.
2.  Watchdog continuously enforces liveness:
    *   expired lease or stale heartbeat -> `stall_detected` event,
    *   requeue/reassign with bounded retry,
    *   terminal fail when retry budget is exhausted (no silent stop states),
    *   if worker creation is policy-blocked, retry once with a reduced subagent-safe prompt profile (minimal technical payload only) and do not pause maintainer for that recoverable path.
3.  Relay execution remains autonomous end-to-end; maintainer interruption is reserved strictly for `true_blocker_decision` and `commit_message_approval`.
    *   Workers must not be stopped/paused for routine process gating; stop/cancel is only for explicit maintainer stop requests or terminal failure recovery.
    *   When maintainer input is required, architect MUST emit exactly one standalone line:
        `ACTION NEEDED (maintainer): reply "<exact text to send>"`.
    *   When no maintainer input is required, architect MUST emit:
        `ACTION NEEDED (maintainer): none`.
4.  Canonical relay autonomy policy tokens (required in docs + guards):
    *   `policy_block_retry_once`: policy-blocked worker prompt failure is auto-retried once with reduced prompt profile and no maintainer interruption.
    *   `watchdog_stall_retry_terminal`: stale heartbeat/timeout must emit `stall_detected`, then bounded retry/reassign, then terminal escalation on retry exhaustion.
    *   `maintainer_pause_gate=true_blocker_decision|commit_message_approval`: pause gate allows maintainer interruption only for those two reasons.
    *   Runtime event naming should prefer explicit completion semantics (`worker_command_started`, `worker_command_completed`, `worker_command_failed`, `unit_completed`, `unit_failed`) so maintainers can distinguish done-vs-next without prompt interpretation.
5.  Before merge to `main`, architect MUST ensure fresh full-gate evidence (`make qa-all`) for accepted branch state.
6.  If accepted:
    *   commit only code/doc files (no relay/runtime artifacts),
    *   use maintainer-approved commit message describing durable behavior (no task numbering),
    *   PR title/summary and commit wording must describe concrete behavior/problem and must not rely on volatile tracker IDs alone,
    *   include explicit work-item status text in the same commit (for example `Status: Confirmed.`, `Status: In Progress.`, or `Status: Fixed.`) so no status transition is left ambiguous,
    *   first push: `git push-fast-up`; tracked branch: `git push-fast`.
5.  If correction is needed for the same logical change set, amend and repush:
    *   `git commit --amend --no-edit`
    *   push with the branch rule above.
6.  Cleanup consumed transient artifacts after usefulness ends (for example `compile_commands.json`, `valgrind.log`, temporary `/tmp` files).

#### 3.1.7 Completion Gate, Merge, and Manual Fallback

1.  When preparing merge to `main`, require green full project gate (`make qa-all`).
2.  Integrate branch to `main` using fast-forward only.
3.  For any bug or task, mark final status (Fixed/Completed) in the commit that is fast-forwarded to main; before that, status must stay non-final (Confirmed/In Progress).
4.  Delete temporary feature branch locally and on remote after merge.
5.  Verify workflow artifacts are not committed.
6.  Manual mode is default: one-unit-at-a-time architect -> developer -> auditor handoff.

## 4. Debugging Procedures

Never allow the AI to "guess" the cause of a bug. Use one of the following objective methodologies, described in detail in **[DEBUGGING.md](DEBUGGING.md)**.

### 4.1 Summary of Methodologies (by Usefulness)

1.  **Targeted Root Cause Analysis:** Lightweight, hypothesis-driven approach using semantic tools (**Serena**/**jCodeMunch**) to compare working vs. broken cases.
2.  **The "Hands-Off" Fix Mandate (Testing):** The mandatory procedure for **Antigravity**. Prove understanding by writing a failing `pytest`/`pexpect` test *before* editing code.
3.  **Instrumentation (The Discovery Loop):** Add `fprintf(stderr, ...)` to trace state. Used primarily for exploratory research in **AI Studio**.
4.  **External Expert Architecture Analysis:** High-reasoning audit for complex, multi-subsystem, or architectural bugs. Requires a fresh LLM context.

*   **Detailed Workflow:** See **[DEBUGGING.md](DEBUGGING.md)** for the complete step-by-step procedures for each method.

---

## 5. Audit Loop and Merge Gate

Follow **[../AUDIT.md](../AUDIT.md)** as the canonical process.

Cadence:
- Treat auditing as a continuous process during implementation, not an end-only step.
- Use focused build/test checks during each feature-sized change or PR iteration.
- Run full `make qa-all` only before merge to `main` or when the maintainer explicitly requests it.
- Run the merge gate before merging.
- Do not run the full loop after every single prompt-level edit unless risk justifies it.

---

## 6. Resource Management & Usage Allowance Economy

AI computational resources (often referred to as tokens, usage allowance, context window limits, or quotas) are strictly finite. Wasted resources lead to shorter sessions, lost history, and increased costs.

### 6.1 Mandatory Usage Allowance Guard
*   **The AI MUST warn the user** when any requested action will unnecessarily consume massive amounts of context, generate immense output, or pull in disproportionately large data unsuited for the immediate task.
*   **The AI MUST always suggest the more economical alternative** before proceeding.
*   **The AI MUST NOT prevent** the user from proceeding if the user explicitly confirms they want to proceed with the expensive action anyway.

### 6.2 Avoid "Blind Exploration"
*   **Do not** ask the agent to "look for issues" or "summarize the file" to get started.
*   **The Semantic Advantage:** **jCodeMunch** `get_repo_outline` and **Serena** `get_symbols_overview` provide the necessary context in a fraction of the cost compared to reading raw files or directory listings.

### 6.3 Targeted Retrieval
*   **Do not** load multiple unrelated files into a single prompt.
*   **The Semantic Advantage:** You MUST use **Serena** `find_symbol` to extract only the relevant function or struct definition. This keeps the prompt focused and the model's reasoning sharp.

### 6.4 Minimize Redundant Calls
*   If a symbol has been read in the current session, do not re-read it.
*   Use **jCodeMunch** `search_text` for broad pattern matching across the entire project rather than manually grepping or opening dozens of files. This is strictly required over bash generic tools.

### 6.5 Batch Related Edits
*   Group related changes into a single prompt to avoid the overhead of full context reload for each minor edit.

### 6.6 Chat Pruning and Context
*   **New Chats for New Work Items:** Once a specific work item (bug or task) is done, close the chat and start a new one to drop old contexts.
*   **Exit Early on Hallucinations:** Stop and restart if the agent speculates. Do not waste usage allowance trying to correct a hallucinating model.
