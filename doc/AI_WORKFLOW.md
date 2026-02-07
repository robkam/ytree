# AI-Assisted Development Workflow

This document defines the standards and protocols for using AI agents to maintain and modernize the `ytree` codebase. These rules ensure architectural integrity and prevent "hallucination debt."

---

## 1. Core Principles

### 1.1 The Golden Loop (Spec-First Integrity)
The development process is strictly hierarchical. The Spec is the "Contract of Truth."
1.  **Write/Verify Spec:** Does `specification.md` describe the desired behavior?
2.  **Write Test:** Create a test case that fails if the behavior is missing (Red).
3.  **Implement:** Write C code to satisfy the test (Green).
4.  **Refactor:** Improve code structure without breaking the test.

**CRITICAL RULE:** If the implementation behaves differently than the Spec, **the implementation is wrong.**
*   **Allowed:** Rewrite the C code to match the Spec.
*   **Allowed:** Fix the Spec if (and only if) the Spec was logically flawed.
*   **FORBIDDEN:** Changing the Test to match the "working" code if it violates the Spec.

### 1.2 General Rules
1.  **Architecture as Blueprint:** [ARCHITECTURE.md](ARCHITECTURE.md) defines the technical structure.
2.  **Human as Architect:** The AI is a "Junior Developer." The human maintainer provides direction.
3.  **Atomic Missions:** One task = One session.
4.  **Build-First Verification:** No code is accepted until it compiles and passes tests.
5.  **The "Clean Slate" Rule:** If an AI generates fragile code, revert and restart. Do not patch.

---

## 2. Shared Personas (The `.agent/rules/` directory)

The project maintains a set of "Persona Rules" in the `.agent/rules/` directory. These are Markdown files that define the behavior, constraints, and technical standards for different types of AI tasks.

*   **`consultant.md`**: Used for planning and analysis. Focuses on behavior-to-C translation without writing code.
*   **`builder.md`**: Used for implementation. Enforces C89/C99 standards, error handling, and file-writing constraints.
*   **`auditor.md`**: Used for identifying architectural fragility and global state coupling.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.
*   **`mentor.md`**: Used for general technical advice.

---

## 3. Workflow: Behavioral SDD (Specification-Driven Development)

All development follows an **Architectural SDD** approach:

1.  **Identify Protocol:** Determine which Protocol (A, B, or C) or Invariant (Focus vs. Freeze) the current bug violates.
2.  **Contract Verification:** Direct the AI to analyze the code specifically through the lens of that protocol.
3.  **Refactor to Spec:** Fix the code so it adheres to the Specification. Do not accept "smart" shortcuts that bypass the established state machine logic.

---

## 4. Workflow: Other IDEs or Web Interfaces (Manual)

If you are using a standard text editor and a web-based AI (like Google AI Studio or ChatGPT), the workflow remains identical but the context management is manual.

1.  **Initialize Context:** Copy the relevant persona from `.agent/rules/` into the AI's "System Instructions" or the first prompt.
2.  **Provide Context:** Provide the AI with the source files relevant to the task.
3.  **Generate & Apply:** Ask the AI to execute the task. Manually copy the code into your editor.
4.  **Verify:** Run the build and test commands in your local terminal.

---

## 5. Debugging Protocol
1.  **Instrument:** Direct the AI to add `fprintf(stderr, ...)` calls to trace state transitions (e.g., tracing `ActivePanel` focus).
2.  **Observe:** Run the instrumented code and provide terminal output to the AI.
3.  **Solve:** Use the runtime evidence to guide the final fix back to Specification compliance.
