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
1.  **Testing as Standard:** [TESTING.md](TESTING.md) defines test naming, structure, and harness rules. All test code must conform to it.
2.  **Human as Architect:** The AI acts as a partner with varying levels of specialization (see Section 2). The human maintainer provides final architectural direction.
3.  **Atomic Missions:** One task = One session.
4.  **Build-First Verification:** No code is accepted until it compiles and passes tests.
5.  **The "Clean Slate" Rule:** If an AI generates fragile code or follows a wrong path, do not attempt to "patch" the mistake via further dialogue. Revert the changes (`git restore .`), rephrase the original prompt from a different perspective, and restart the mission. This is mandatory because:
    *   **Context Clearing:** It removes the "bad" code and the flawed logic that led to it.
    *   **New Perspective:** Rephrasing prevents the model from staying stuck in a loop of bad assumptions.
    *   **Clean Implementation:** It ensures the final code is the result of a single, clean logical flow rather than a series of ad-hoc patches.
6.  **Minor Step Corrections Should Amend:** If the immediately preceding step only needs a small correction and should remain the same logical history unit, update it with `git commit --amend --no-edit` rather than adding a trivial follow-up fix commit. Create a new commit only when the correction is meaningfully distinct, delayed enough to matter historically, or worth preserving separately in the project history.
7.  **Intent Over Literal Wording:** Treat the human maintainer as authoritative on goals, but not automatically on exact UI wording, naming, key choices, menu structure, or workflow details. If a requested detail does not follow convention or best practices, or conflicts with existing Ytree patterns, the AI must say so explicitly and recommend the stronger conventional approach before implementing it.

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
*   Code-smell audit/remediation (deslop-style) -> `rm-code-smells` (prompt guide: [RM_CODE_SMELLS.md](RM_CODE_SMELLS.md))
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

Use this workflow when the mission is large enough that one-shot implementation is risky.
For the canonical maintainer copy/paste mission prompt, see **[AGENT_PROMPT_TEMPLATE.md](AGENT_PROMPT_TEMPLATE.md)**.

#### 3.1.0 Relay Runtime Prerequisite

Before starting the loop, ensure Tether MCP is installed and configured for every client used in the chain (`architect`, `developer`, `code_auditor`).
The stateless multi-AI loop uses Tether relay handles.

##### 3.1.0.1 MCP Config Bootstrap (Recommended)

To ensure local AI tooling picks up project defaults, run:

```bash
make mcp-doctor FIX=1
```

This bootstraps local MCP client configuration from repo-tracked defaults when missing, while preserving personal auth/session/history settings.
Principle: keep team-shared defaults in repo config and keep user-specific paths, credentials, and local overrides in user-local config.

##### 3.1.0.2 Tether Relay MCP Setup

Install Tether once on your machine:

```bash
mkdir -p "$HOME/.local/src" "$HOME/.local/share/tether"
cd "$HOME/.local/src"
git clone https://github.com/latentcollapse/Tether.git
cd Tether
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
```

Recommended shared DB path:
- `$HOME/.local/share/tether/postoffice.db`

All participating clients in the workflow should point to the same Tether server entry and shared DB path.

#### 3.1.1 Workflow Contract (Mandatory)

1.  This workflow is mandatory for non-trivial missions.
2.  Work is executed as numbered tasks that are:
    *   atomic and independently verifiable,
    *   not fragmented into trivial micro-steps,
    *   executed one task at a time.
3.  All relay artifacts MUST be Tether messages with stable handles.
    *   Completion/status lines shared with maintainer MUST include the relevant handle.
4.  Relay artifacts (prompt/report/status messages) are workflow artifacts and MUST NOT be committed.

#### 3.1.2 Mission Definition Pass (Stateless Planning)

1.  Run a stateless planning session to define mission scope, constraints, and acceptance criteria.
2.  Output must include a prompt for a stateless `architect` pass.

#### 3.1.3 Architect Pass (Stateless, Branch Setup, Tether-Based Handoffs)

1.  Start on a dedicated feature branch (local + remote).
2.  Architect publishes a numbered master plan relay message in Tether and keeps its handle.
3.  Architect emits exactly one developer handoff at a time (never multiple tasks in one handoff).
4.  Each developer handoff must be a Tether prompt message and must include:
    *   strict scope lock,
    *   acceptance criteria,
    *   verification commands,
    *   stop/blocker conditions,
    *   mandatory developer report handle in completion line.
5.  Architect relay response to maintainer MUST include:
    *   `Reasoning level: <Low|Medium|High|Extra High>`
    *   `Handoff line: developer: Execute task <task-id> from handle <handoff-handle> exactly as written (Task ... only).`

#### 3.1.4 Developer Pass (Stateless, Single Task Only)

1.  Run a stateless `developer` for exactly one task.
2.  Developer executes only the assigned task prompt.
3.  Developer is the primary per-task verifier. Verification cadence within the same atomic task is mandatory:
    *   initial pass for the task: run the full verification set listed in the task prompt (including full gate if listed).
    *   correction/rework passes for that same task: rerun only previously failing checks and directly impacted targeted tests.
    *   do not rerun full `make qa-all` on every iteration unless architect explicitly requests it or risk materially changes.
4.  Developer MUST NOT mark the task complete while any required verification command is failing.
5.  Developer report MUST include:
    *   exact commands executed,
    *   pass/fail outcome per command,
    *   enough output summary to validate evidence quickly.
6.  On completion, developer MUST publish a Tether report message and capture its handle.
7.  Developer chat response to maintainer MUST be one line only:
    *   `Task <task-id> completed, report handle: <report-handle>`

#### 3.1.5 Auditor Pass (Stateless, Single Task Only)

1.  After each developer-completed atomic task, architect prepares a single-task auditor prompt message in Tether.
2.  This includes correction/rework tasks (`R` tasks): each rework is treated as a new atomic task and must receive its own auditor pass.
3.  Run a stateless `code_auditor` for that same task only.
4.  Auditor workflow is evidence-first: validate code changes + developer verification evidence before rerunning commands.
5.  Auditor SHOULD rerun verification commands only when needed (missing evidence, contradictory results, suspicious behavior/risk, or inability to confirm a claimed pass from artifacts).
6.  On correction/rework iterations, auditor reruns should default to the failing checks and directly affected targeted tests; do not rerun full `make qa-all` by default.
7.  Full-suite rerun by auditor is reserved for unresolved high-risk uncertainty or explicit architect request.
8.  Auditor report MUST state whether commands were rerun and why.
9.  Auditor MUST publish an auditor report message in Tether and capture its handle.
10. Auditor chat response to maintainer MUST be one line only:
    *   `Task <task-id> auditor pass completed, report handle: <auditor-report-handle>`

#### 3.1.6 Architect Validation, Commit, and Cleanup

1.  Maintainer provides developer/auditor completion lines and report handles back to architect.
2.  Architect validates repository state + report evidence.
3.  Architect should avoid redundant per-task reruns. Re-run commands only when evidence is incomplete, conflicting, or high-risk details remain unresolved.
4.  Before commit, architect MUST ensure fresh full-gate evidence (`make qa-all`) exists for the current accepted branch state (from current developer evidence or architect rerun when evidence is stale/incomplete).
5.  If task is accepted:
    *   commit only task code files (no local relay/report artifacts),
    *   use maintainer-approved commit message describing behavior/scope (no task numbering),
    *   for first push of a new branch, use `git push-fast-up`,
    *   for already-tracked branches, use `git push-fast`.
6.  If a follow-up correction is needed for the same task or set, amend and repush:
    *   `git commit --amend --no-edit`
    *   then push using the branch rule above (`push-fast-up` first push, else `push-fast`).
7.  After commit/push, expire or delete consumed relay messages according to TTL/retention policy.
8.  Repeat from architect handoff for the next single task.
9.  If a task fails audit or has implementation defects, architect must issue a new tailored correction task prompt for a new stateless `developer` pass. Do not reuse the previous developer session.

#### 3.1.7 Completion Gate and Merge

1.  When final numbered task is accepted, run full project gate (`make qa-all`) and require green results.
2.  Integrate branch to `main` with fast-forward only; do not create a merge message commit.
3.  Delete feature branch locally and on remote after successful integration.
4.  Ensure relay artifacts are either expired/archived per policy and never committed.

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
- Run the full audit loop for each feature-sized change or PR.
- Within a single feature relay chain, run full `make qa-all` on the initial task pass and before architect commit; use failing-check + targeted reruns between those points.
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
*   **New Chats for New Tasks:** Once a specific bug is fixed or a task is done, close the chat and start a new one to drop old contexts.
*   **Exit Early on Hallucinations:** Stop and restart if the agent speculates. Do not waste usage allowance trying to correct a hallucinating model.
