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
1.  **Architecture as Blueprint:** [ARCHITECTURE.md](ARCHITECTURE.md) defines the technical structure.
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

*   **Gemini Flash (The Junior Developer / Scripter):** High-speed, high-quota model for bulk information retrieval, context discovery, documentation, repetitive formatting, and simple, unambiguous refactoring.
*   **Gemini Pro (The Core Contributor / Feature Builder):** The primary development "workhorse." Use for standard feature implementation, writing complex behavioral tests, refactoring, and general architectural planning.
*   **Claude Opus (The Lead Architect / Auditor):** High-reasoning engine reserved for the hardest problems, subtle logic bugs, zero-trust refactoring ("Architect Mode"), and critical code audits where reasoning quality is paramount.

---

## 4. The Agentic Loop (Antigravity / IDE Integration)

The **Google Agentic IDE (Antigravity)** is the primary tool for development. It automates the implementation loop, significantly speeding up the process of editing, building, and testing code.

1.  **Direct Execution:** The agent operates directly within the codebase, eliminating the manual "copy-paste-check" cycle.
2.  **Multimodal Reasoning:** Although the IDE has built-in models, the **Claude Extension** is used specifically for high-reasoning tasks (architectural shifts, subtle bug fixes).
3.  **Cross-File State:** Antigravity manages the context of multiple open files and terminal states, ensuring the agent always has the relevant local environment at its disposal.

---

## 5. The Guidance Loop (AI Studio / Manual Research)

**Google AI Studio** is used primarily for high-level guidance, architectural brainstorming, and exploratory research where direct file editing is not yet required.

1.  **Avoid Implementation in Web UI:** Coding directly in AI Studio is considered "manually intensive" and prone to errors. Use it to deliberate on a plan, then move to Antigravity for the execution.
2.  **Initialize Context:** When seeking guidance, copy the relevant persona from `.agent/rules/` into the AI's "System Instructions."
3.  **Large Context Exploration:** Leverage the massive context window of Gemini models in AI Studio to analyze large parts of the codebase simultaneously before committing to a specific `@mission`.

## 6. Debugging Procedures

Never allow the AI to "guess" the cause of a bug; it will confidently hallucinate a solution that may not address the underlying issue. Instead, use one of the two following objective loops:

### 6.1 Instrumentation (The Discovery Loop)

Used primarily for high-level research in **AI Studio** or complex exploratory debugging. It forces the AI to "find out" what is actually happening before proposing a fix.

1.  **Instrument:** Use the keyword **"Instrument"** to direct the AI to add `fprintf(stderr, ...)` calls to trace state transitions (e.g., tracing `ActivePanel` focus).
2.  **Observe:** Run the instrumented code and provide terminal output to the AI.
3.  **Solve:** Use the empirical runtime evidence to guide the final fix back to Specification compliance.

### 6.2 The "Hands-Off" Fix Mandate (The Agentic Loop)

The mandatory focused procedure for **Antigravity** and agentic execution. It forces the agent to prove its understanding by writing a failing test *before* editing any code.

1.  **Replicate in Python (The Test):** Describe the manual steps that cause the bug to the **Tester** persona. Instruct it: *"Write a `pytest` using the `pexpect` TUI harness that performs these exact steps and asserts the expected correct behavior. This test MUST fail currently."*
2.  **Verify the Failure (Red):** Run `make test`. Confirm the new test fails exactly where the bug manifests. You now have a mathematical proof of the bug.
3.  **Assign the Fix (The Code):** Hand the failing test output and the relevant C source files to the **Builder** persona. Instruct it: *"Modify the C code to make this specific test pass. Do not change the test."*
4.  **Verify the Fix (Green):** Run `make test`. When the test passes, the bug is fixed, and you have automatically gained a permanent regression test.

**Why this works:** It removes human translation errors and forces the AI to debug against a rigid, objective standard rather than a subjective description.

For test naming conventions, structure standards, and harness rules, see **[TESTING.md](TESTING.md)**.

---

## 7. Token Conservation

AI tokens are a finite resource per session. Wasted tokens mean shorter sessions, lost context, and more frequent "clean slate" restarts. Follow these rules to maximize productive output per session.

### 7.1 One Context at a Time
*   **Do not** load multiple unrelated files, symbols, or topics into a single prompt. Each prompt should focus on one module, one feature, or one bug.
*   **Why:** Broad context retrieval floods the window with irrelevant information, reducing the space available for reasoning about the actual problem.

### 7.2 Minimize Redundant Reads
*   If a file or symbol has already been read in the current session, do not re-read it unless it has been modified since.
*   Use targeted symbol lookups (e.g., Serena `find_symbol`) instead of reading entire files when only one function is relevant.

### 7.3 Be Specific in Prompts
*   **Bad:** "Look at the codebase and find issues."
*   **Good:** "Audit `src/fs/archive_read.c` for unchecked return values from `archive_read_next_header`."
*   Vague prompts cause the agent to speculatively read dozens of files, burning tokens on exploration rather than execution.

### 7.4 Batch Related Changes
*   Group related edits into a single prompt (e.g., "Rename X and update all callers") rather than issuing them one at a time with full context reload each time.

### 7.5 Avoid Premature Optimization of Prompts
*   Do not ask the agent to "summarize everything it knows" or "list all functions in the project" as a warm-up. Start with the specific task - the agent will pull context as needed.

### 7.6 Exit Early on Wrong Paths
*   If the agent is heading in a wrong direction, **stop and restart** (see Section 1.2, "Clean Slate" Rule) rather than spending tokens on corrections and follow-up clarifications that compound the original mistake.