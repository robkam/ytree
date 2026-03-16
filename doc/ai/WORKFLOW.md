# AI-Assisted Development Workflow

This document defines the standards and processes for using AI agents to maintain and modernize the `ytree` codebase. These rules ensure architectural integrity and prevent "hallucination debt."

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

---

## 2. Shared Personas (The `.agent/rules/` directory)

The project maintains a set of "Persona Rules" in the `.agent/rules/` directory. These are Markdown files that define the behavior, constraints, and technical standards for different types of AI tasks.

*   **`architect.md`**: Used for planning and architecture design. Defines task boundaries and invariants without coding.
*   **`developer.md`**: Used for implementation. Applies approved changes while preserving architecture and safety constraints.
*   **`code_auditor.md`**: Used for adversarial quality review, fragility detection, and pass/fail gate findings.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.

---

## 3. Model Selection Guide

Model choice should be explicit and repeatable. In this project, default to GPT Codex models first, then use other model families only when you intentionally want a second opinion.

### 3.1 Reasoning Level Meanings

*   **Low:** Mechanical edits, straightforward formatting/docs updates, simple search-and-replace, low-risk file shuffling.
*   **Medium:** Normal feature work, focused bug fixes, standard pytest updates, moderate multi-file changes.
*   **High:** Architectural risk, subtle state bugs, cross-module behavior coupling, non-obvious regressions.
*   **Extra High:** Pre-release gate audits, adversarial code review, ambiguous root-cause analysis, high-cost mistakes.

### 3.2 Primary Selection Matrix

| Task Type | Primary Model | Suggested Level | Overlap / Notes |
|:---|:---|:---|:---|
| Repo scanning, repetitive edits, docs cleanup | GPT Codex 5.3 | Low to Medium | Fast default for throughput; overlap with other "fast" models. |
| Standard implementation and test work | GPT Codex 5.3 | Medium to High | Start here for most missions. |
| Complex refactor with architecture constraints | GPT 5.4 | High | 5.3 at High can often do this too; switch if reasoning stalls. |
| Deep bug forensics across subsystems | GPT 5.4 | High to Extra High | Use when symptoms are cross-cutting or prior fixes regressed behavior. |
| Pre-release review gate (Code Auditor role) | GPT 5.4 | Extra High | Prefer stricter reasoning over speed. |

### 3.3 Escalation Rule (Model vs. Level)

1.  Start with **GPT Codex 5.3 at Medium** for normal work.
2.  Raise level first (**Medium -> High**) before switching models.
3.  Switch to **GPT 5.4** when:
    *   the task is architecture-heavy,
    *   bug cause remains unclear after one focused pass, or
    *   quality gate decisions require stricter reasoning.
4.  For audit-critical calls, use **GPT 5.4 at Extra High** by default.

### 3.4 Intentional Overlap (When Either Works)

*   **GPT Codex 5.3 (High)** and **GPT 5.4 (Medium/High)** overlap for many mid-to-hard engineering tasks.
*   If speed matters and risk is moderate, prefer **5.3**.
*   If certainty matters more than speed, prefer **5.4**.
*   You can use a second model family (Gemini/Claude) as an independent audit pass, not as the default path.

### 3.5 Cross-Model Perspective Policy

Use a two-pass approach for high-risk or ambiguous work:

1.  **Build Pass (Codex):** Implement and verify in Antigravity.
2.  **Challenge Pass (External):** Ask Gemini/Claude to critique assumptions, edge cases, and failure modes.
3.  **Reconcile Pass (Codex):** Apply only evidence-backed improvements and re-run tests.

Use external large-context models when the task spans many documents or subsystems (for example: roadmap + architecture + spec + manpage consistency checks).

---

## 4. The Agentic Loop (Antigravity + Codex Extension)

The primary execution environment is **Antigravity** with the **Codex extension**.

1.  **Direct Execution:** Run edits, builds, and tests in-repo to avoid manual copy/paste workflows.
2.  **Model Routing:** Default to GPT Codex 5.3, escalate level first, then move to GPT 5.4 for harder reasoning tasks (see Section 3).
3.  **Semantic Context:** Use **Serena** (symbol navigation) and **jCodeMunch** (repo indexing/search) for targeted context retrieval.
4.  **Cross-File State:** Keep terminal state, open-file context, and task history in one working loop.

---

## 5. Optional Second-Opinion Loop

External model families (Gemini/Claude or others) are optional and used only when you intentionally want an independent perspective.

1.  **No Primary Implementation There:** Do planning/review externally if useful, then implement in Antigravity with Codex.
2.  **Persona Alignment:** Use the same persona framing (`Architect`, `Developer`, `Code Auditor`, `Tester`) to keep outputs compatible.
3.  **Use Cases:** Architecture challenge, tie-break between competing fixes, adversarial pre-release audit cross-check, or huge-context synthesis across multiple project docs.

## 6. Debugging Procedures

Never allow the AI to "guess" the cause of a bug. Use one of the following objective methodologies, described in detail in **[DEBUGGING.md](DEBUGGING.md)**.

### 6.1 Summary of Methodologies (by Usefulness)

1.  **Targeted Root Cause Analysis:** Lightweight, hypothesis-driven approach using semantic tools (**Serena**/**jCodeMunch**) to compare working vs. broken cases.
2.  **The "Hands-Off" Fix Mandate (Testing):** The mandatory procedure for **Antigravity**. Prove understanding by writing a failing `pytest`/`pexpect` test *before* editing code.
3.  **Instrumentation (The Discovery Loop):** Add `fprintf(stderr, ...)` to trace state. Used primarily for exploratory research in **AI Studio**.
4.  **External Expert Architecture Analysis:** High-reasoning audit for complex, multi-subsystem, or architectural bugs. Requires a fresh LLM context.

*   **Detailed Workflow:** See **[DEBUGGING.md](DEBUGGING.md)** for the complete step-by-step procedures for each method.

---

## 7. Default Pre-Release Review Gate

Before code is shown to human reviewers, follow the default quality loop in **[PRE_RELEASE_REVIEW.md](PRE_RELEASE_REVIEW.md)**.

This is a multi-persona loop:

1.  **Code Auditor**: strict evidence-based review and finding generation.
2.  **Developer**: implement targeted fixes.
3.  **Tester**: run verification and regression checks.
4.  **Code Auditor**: re-audit final diff and decide pass/fail.

Code is not review-ready until the gate exits with zero blocker/high findings and green verification.

---

## 8. Token Economy & Resource Management

AI tokens and semantic tool calls are finite resources. Wasted tokens mean shorter sessions, lost context, and more frequent restarts. **Serena** and **jCodeMunch** are the primary defenses against "token bleed"—using them is not just about convenience, it is the only way to maintain a high-integrity session.

### 8.1 Avoid "Blind Exploration"
*   **Do not** ask the agent to "look for issues" or "summarize the file" to get started.
*   **The Semantic Advantage:** **jCodeMunch** `get_repo_outline` and **Serena** `get_symbols_overview` provide the necessary context in a fraction of the token cost compared to reading raw files or directory listings.

### 8.2 Targeted Retrieval
*   **Do not** load multiple unrelated files into a single prompt.
*   **The Semantic Advantage:** Use **Serena** `find_symbol` to extract only the relevant function or struct definition. This keeps the prompt focused and the model's reasoning sharp.

### 8.3 Minimize Redundant Calls
*   If a symbol has been read in the current session, do not re-read it.
*   Use **jCodeMunch** `search_text` for broad pattern matching across the entire project rather than manually grepping or opening dozens of files.

### 8.4 Batch Related Edits
*   Group related changes (e.g., "Rename X and update all callers") into a single prompt to avoid the overhead of full context reload for each minor edit.

### 8.5 Exit Early on Hallucinations
*   If the agent begins to speculate or heads in a wrong direction, **stop and restart** (see Section 1.2, "Clean Slate" Rule). Spending tokens to "correct" a hallucinating model is almost always a net loss; a fresh start with refined context is the most efficient path.
