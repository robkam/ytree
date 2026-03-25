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
2.  **Human as Architect:** The AI acts as a partner with varying levels of specialization (see Section 3). The human maintainer provides final architectural direction.
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

## 3. Model Selection Guide

Model choice should be explicit and repeatable. In this project, default to GPT Codex models first. If the GPT Codex usage limit is reached, fallback to Gemini (Flash/Pro) or Claude (Sonnet/Opus) depending on task complexity. Use other model families only when you intentionally want a second opinion or hit the primary usage limit.

### 3.1 Reasoning Level Meanings

*   **Low:** Mechanical edits, straightforward formatting/docs updates, simple search-and-replace, low-risk file shuffling.
*   **Medium:** Normal feature work, focused bug fixes, standard pytest updates, moderate multi-file changes.
*   **High:** Architectural risk, subtle state bugs, cross-module behavior coupling, non-obvious regressions.
*   **Extra High:** Pre-release gate audits, adversarial code review, ambiguous root-cause analysis, high-cost mistakes.

### 3.2 Primary Selection Matrix

| Task Type | Primary Model | Fallback Models (Usage Limit Reached) | Suggested Level | Overlap / Notes |
|:---|:---|:---|:---|:---|
| Repo scanning, repetitive edits, docs cleanup | GPT Codex 5.3 | Gemini Flash | Low to Medium | Fast default for throughput; overlap with other "fast" models. |
| Standard implementation and test work | GPT Codex 5.3 | Gemini Pro / Claude Sonnet | Medium to High | Start here for most missions. |
| Complex refactor with architecture constraints | GPT 5.4 | Gemini Pro / Claude Sonnet | High | 5.3 at High can often do this too; switch if reasoning stalls. |
| Deep bug forensics across subsystems | GPT 5.4 | Claude Opus | High to Extra High | Use when symptoms are cross-cutting or prior fixes regressed behavior. |
| Pre-release review gate (Code Auditor role) | GPT 5.4 | Claude Opus | Extra High | Prefer stricter reasoning over speed. |

### 3.3 Escalation Rule (Model vs. Level)

1.  Start with **GPT Codex 5.3 at Medium** for normal work. If the Codex usage limit is reached, start with **Gemini Pro** or **Claude Sonnet**.
2.  Raise level first (**Medium -> High**) before switching models.
3.  Switch to **GPT 5.4** (or **Claude Opus** if the Codex usage limit is reached) when:
    *   the task is architecture-heavy,
    *   bug cause remains unclear after one focused pass, or
    *   quality gate decisions require stricter reasoning.
4.  For audit-critical calls, use **GPT 5.4 at Extra High** by default (fallback: **Claude Opus**).
5.  For simple mechanical tasks when the Codex usage limit is reached, use **Gemini Flash**.

### 3.4 Intentional Overlap (When Either Works)

*   **GPT Codex 5.3 (High)** and **GPT 5.4 (Medium/High)** overlap for many mid-to-hard engineering tasks.
*   If speed matters and risk is moderate, prefer **5.3** (or **Gemini Flash** / **Sonnet** if the usage limit is reached).
*   If certainty matters more than speed, prefer **5.4** (or **Claude Opus** / **Gemini Pro** if the usage limit is reached).
*   You can use a second model family (Gemini/Claude) as an independent audit pass, or as primary if the Codex usage limit is reached.

### 3.5 Cross-Model Perspective Policy

Use a two-pass approach for high-risk or ambiguous work:

1.  **Build Pass (Codex):** Implement and verify in Antigravity. (Use Gemini/Claude if the Codex usage limit is reached).
2.  **Challenge Pass (External):** Ask Gemini/Claude to critique assumptions, edge cases, and failure modes.
3.  **Reconcile Pass (Codex):** Apply only evidence-backed improvements and re-run tests.

Use external large-context models when the task spans many documents or subsystems (for example: roadmap + architecture + spec + manpage consistency checks).

---

## 4. The Agentic Loop (Antigravity + Codex Extension)

The primary execution environment is **Antigravity** with the **Codex extension**, or fallback to **Gemini/Claude** if the usage limit is reached.

1.  **Direct Execution:** Run edits, builds, and tests in-repo to avoid manual copy/paste workflows.
2.  **Model Routing:** Default to GPT Codex 5.3, escalate level first, then move to GPT 5.4 for harder reasoning tasks (see Section 3). If the Codex usage limit is reached, route to Gemini Flash for simple work, Gemini Pro/Claude Sonnet for standard work, and Claude Opus for hard reasoning tasks.
3.  **Semantic Context:** You MUST use **Serena** (symbol navigation) and **jCodeMunch** (repo indexing/search) for targeted context retrieval. You MUST NOT use raw bash tools like grep or find.
4.  **Cross-File State:** Keep terminal state, open-file context, and task history in one working loop.

---

## 5. Optional Second-Opinion Loop (and Usage Limit Fallback)

External model families (Gemini Flash/Pro, Claude Sonnet/Opus, or others) are used when the GPT Codex usage limit is reached, or when you intentionally want an independent perspective.

1.  **Fallback Implementation:** If the Codex usage limit is reached, implement directly using Gemini Pro/Claude Sonnet for standard tasks, Gemini Flash for simple edits, and Claude Opus for complex architecture tasks.
2.  **No Primary Implementation There (If Codex is Available):** Do planning/review externally if useful, then implement in Antigravity with Codex.
3.  **Persona Alignment:** Use the same persona framing (`Architect`, `Developer`, `Code Auditor`, `Tester`) to keep outputs compatible.
4.  **Use Cases:** Usage limit fallback, architecture challenge, tie-break between competing fixes, adversarial pre-release audit cross-check, or huge-context synthesis across multiple project docs.
## 6. Debugging Procedures

Never allow the AI to "guess" the cause of a bug. Use one of the following objective methodologies, described in detail in **[DEBUGGING.md](DEBUGGING.md)**.

### 6.1 Summary of Methodologies (by Usefulness)

1.  **Targeted Root Cause Analysis:** Lightweight, hypothesis-driven approach using semantic tools (**Serena**/**jCodeMunch**) to compare working vs. broken cases.
2.  **The "Hands-Off" Fix Mandate (Testing):** The mandatory procedure for **Antigravity**. Prove understanding by writing a failing `pytest`/`pexpect` test *before* editing code.
3.  **Instrumentation (The Discovery Loop):** Add `fprintf(stderr, ...)` to trace state. Used primarily for exploratory research in **AI Studio**.
4.  **External Expert Architecture Analysis:** High-reasoning audit for complex, multi-subsystem, or architectural bugs. Requires a fresh LLM context.

*   **Detailed Workflow:** See **[DEBUGGING.md](DEBUGGING.md)** for the complete step-by-step procedures for each method.

---

## 7. Audit Loop and Merge Gate

Follow **[../AUDIT.md](../AUDIT.md)** as the canonical process.

Cadence:
- Treat auditing as a continuous process during implementation, not an end-only step.
- Run the full audit loop for each feature-sized change or PR.
- Run the merge gate before merging.
- Do not run the full loop after every single prompt-level edit unless risk justifies it.

---

## 8. Resource Management & Usage Allowance Economy

AI computational resources (often referred to as tokens, usage allowance, context window limits, or quotas) are strictly finite. Wasted resources lead to shorter sessions, lost history, and increased costs. 

### 8.1 Mandatory Usage Allowance Guard
*   **The AI MUST warn the user** when any requested action will unnecessarily consume massive amounts of context, generate immense output, or pull in disproportionately large data unsuited for the immediate task.
*   **The AI MUST always suggest the more economical alternative** before proceeding.
*   **The AI MUST NOT prevent** the user from proceeding if the user explicitly confirms they want to proceed with the expensive action anyway.

### 8.2 Avoid "Blind Exploration"
*   **Do not** ask the agent to "look for issues" or "summarize the file" to get started.
*   **The Semantic Advantage:** **jCodeMunch** `get_repo_outline` and **Serena** `get_symbols_overview` provide the necessary context in a fraction of the cost compared to reading raw files or directory listings.

### 8.3 Targeted Retrieval
*   **Do not** load multiple unrelated files into a single prompt.
*   **The Semantic Advantage:** You MUST use **Serena** `find_symbol` to extract only the relevant function or struct definition. This keeps the prompt focused and the model's reasoning sharp.

### 8.4 Minimize Redundant Calls
*   If a symbol has been read in the current session, do not re-read it.
*   Use **jCodeMunch** `search_text` for broad pattern matching across the entire project rather than manually grepping or opening dozens of files. This is strictly required over bash generic tools.

### 8.5 Batch Related Edits
*   Group related changes into a single prompt to avoid the overhead of full context reload for each minor edit.

### 8.6 Chat Pruning and Context
*   **New Chats for New Tasks:** Once a specific bug is fixed or a task is done, close the chat and start a new one to drop old contexts.
*   **Exit Early on Hallucinations:** Stop and restart if the agent speculates. Do not waste usage allowance trying to correct a hallucinating model.
