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

*   **`consultant.md`**: Used for planning and analysis. Focuses on behavior-to-C translation without writing code.
*   **`builder.md`**: Used for implementation. Enforces C89/C99 standards, error handling, and file-writing constraints.
*   **`auditor.md`**: Used for identifying architectural fragility and global state coupling.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.

---

## 3. Model Selection Guide

When working with an AI-assisted workflow, treat Large Language Models (LLMs) like developers with varying seniority levels and specialties. Select the model based on task complexity and risk to prioritize **token economy**, leveraging **Serena** across all models for semantic context.

*   **Gemini Flash (The Junior Developer / Scripter):** High-speed, high-quota model for bulk information retrieval, context discovery, documentation, repetitive formatting, and simple, unambiguous refactoring. select this model when performing broad repo-wide searches using **jCodeMunch** (e.g., `search_text`).
*   **Gemini Pro (The Core Contributor / Feature Builder):** The primary development "workhorse." Use for standard feature implementation, writing complex behavioral tests, refactoring, and general architectural planning. Leverages both **Serena** for symbol semantic lookups and **jCodeMunch** for file-tree analysis.
*   **Claude Opus (The Lead Architect / Auditor):** High-reasoning engine reserved for the hardest problems, subtle logic bugs, zero-trust refactoring ("Architect Mode"), and critical code audits where reasoning quality is paramount.

---

## 4. The Agentic Loop (Antigravity / IDE Integration)

The **Google Agentic IDE (Antigravity)** is the primary tool for development. It automates the implementation loop, significantly speeding up the process of editing, building, and testing code.

1.  **Direct Execution:** The agent operates directly within the codebase, eliminating the manual "copy-paste-check-cycle.
2.  **Multimodal Reasoning:** Although the IDE has built-in models, the **Claude Extension** is used specifically for high-reasoning tasks (architectural shifts, subtle bug fixes).
3.  **Semantic Context:** The agent utilizes **Serena** (LSP-based symbol navigation) and **jCodeMunch** (Repository-wide indexing and search) to navigate and understand the codebase efficiently.
4.  **Cross-File State:** Antigravity manages the context of multiple open files and terminal states, ensuring the agent always has the relevant local environment at its disposal.

---

## 5. The Guidance Loop (AI Studio / Manual Research)

**Google AI Studio** is used primarily for high-level guidance, architectural brainstorming, and exploratory research where direct file editing is not yet required.

1.  **Avoid Implementation in Web UI:** Coding directly in AI Studio is considered "manually intensive" and prone to errors. Use it to deliberate on a plan, then move to Antigravity for the execution.
2.  **Initialize Context:** When seeking guidance, copy the relevant persona from `.agent/rules/` into the AI's "System Instructions."
3.  **Large Context Exploration:** Leverage the massive context window of Gemini models in AI Studio to analyze large parts of the codebase simultaneously before committing to a specific `@mission`. Use **jCodeMunch** `get_repo_outline` to provide a high-level map of the codebase as a starting point.

## 6. Debugging Procedures

Never allow the AI to "guess" the cause of a bug; it will confidently hallucinate a solution that may not address the underlying issue. Instead, use one of the following three objective methodologies, selected based on complexity and visibility.

### 6.1 Instrumentation (The Discovery Loop)
Used primarily for high-level research in **AI Studio** or complex exploratory debugging. It forces the AI to "find out" what is actually happening before proposing a fix.

1.  **Instrument:** Use the keyword **"Instrument"** to direct the AI to add `fprintf(stderr, ...)` calls to trace state transitions (e.g., tracing `ActivePanel` focus).
2.  **Observe:** Run the instrumented code and provide terminal output to the AI.
3.  **Solve:** Use the empirical runtime evidence to guide the final fix back to Specification compliance.

### 6.2 The "Hands-Off" Fix Mandate (The Agentic Loop)
The mandatory focused procedure for **Antigravity** and agentic execution. It forces the agent to prove its understanding by writing a failing test *before* editing any code.

1.  **Replicate in Python (The Test):** Describe the manual steps that cause the bug to the **Tester** persona. Instruct it: *"Write a `pytest` using the `pexpect` TUI harness that performs these exact steps and asserts the expected correct behavior. This test MUST fail currently."*
2.  **Verify the Failure (Red):** Run `make test`. Confirm the new test fails exactly where the bug manifests.
3.  **Assign the Fix (The Code):** Hand the failing test output and the relevant C source files to the **Builder** persona. Instruct it: *"Modify the C code to make this specific test pass. Do not change the test."*
4.  **Verify the Fix (Green):** Run `make test`. A passing test provides mathematical proof of the fix and creates a permanent regression test.

### 6.3 External Expert Architecture Analysis
The preferred method for complex bugs involving global state violations, race conditions, or subsystem-wide failures (e.g., ncurses Z-order issues). This method uses a separate, high-reasoning model instance to perform a zero-trust audit.

*   **Workflow:** See **[DEBUGGING.md](DEBUGGING.md)** for the complete five-step evidence-based methodology.

---

## 7. Token Economy & Resource Management

AI tokens and semantic tool calls are finite resources. Wasted tokens mean shorter sessions, lost context, and more frequent restarts. **Serena** and **jCodeMunch** are the primary defenses against "token bleed"—using them is not just about convenience, it is the only way to maintain a high-integrity session.

### 7.1 Avoid "Blind Exploration"
*   **Do not** ask the agent to "look for issues" or "summarize the file" to get started.
*   **The Semantic Advantage:** **jCodeMunch** `get_repo_outline` and **Serena** `get_symbols_overview` provide the necessary context in a fraction of the token cost compared to reading raw files or directory listings.

### 7.2 Targeted Retrieval
*   **Do not** load multiple unrelated files into a single prompt.
*   **The Semantic Advantage:** Use **Serena** `find_symbol` to extract only the relevant function or struct definition. This keeps the prompt focused and the model's reasoning sharp.

### 7.3 Minimize Redundant Calls
*   If a symbol has been read in the current session, do not re-read it.
*   Use **jCodeMunch** `search_text` for broad pattern matching across the entire project rather than manually grepping or opening dozens of files.

### 7.4 Batch Related Edits
*   Group related changes (e.g., "Rename X and update all callers") into a single prompt to avoid the overhead of full context reload for each minor edit.

### 7.5 Exit Early on Hallucinations
*   If the agent begins to speculate or heads in a wrong direction, **stop and restart** (see Section 1.2, "Clean Slate" Rule). Spending tokens to "correct" a hallucinating model is almost always a net loss; a fresh start with refined context is the most efficient path.