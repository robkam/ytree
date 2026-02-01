# AI-Assisted Development Workflow

This document defines the standards and protocols for using AI agents to maintain and modernize the `ytree` codebase. These rules ensure architectural integrity and prevent "hallucination debt."

## 1. Core Principles

1.  **Human as Architect:** The AI is a "Junior Developer" or "Implementation Agent." The human maintainer provides the architectural direction and must approve every change.
2.  **Atomic Missions:** One task = One session. Do not bundle multiple unrelated features or bug fixes into a single AI prompt/mission.
3.  **Build-First Verification:** No code is accepted until it compiles and passes the test suite (`make clean && make && make test`).
4.  **The "Clean Slate" Rule:** If an AI generates incorrect or "fragile" code, do not attempt to "patch" the conversation. Revert the code (`git restore .`), end the session, and start a new mission with refined instructions.

---

## 2. Shared Personas (The `.agent/rules/` directory)

The project maintains a set of "Persona Rules" in the `.agent/rules/` directory. These are Markdown files that define the behavior, constraints, and technical standards for different types of AI tasks.

*   **`consultant.md`**: Used for planning and analysis. Focuses on behavior-to-C translation without writing code.
*   **`builder.md`**: Used for implementation. Enforces C89/C99 standards, error handling, and file-writing constraints.
*   **`auditor.md`**: Used for identifying architectural fragility and global state coupling.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.

---

## 3. Workflow: Antigravity (Agentic IDE)

If you are using **Google Antigravity**, the IDE automatically ingests the rules in `.agent/rules/`.

1.  **Start a Mission:** Open the Agent Manager and describe the goal.
2.  **Review the Plan:** The Agent will produce an "Implementation Plan." Verify that the correct header and source files are identified.
3.  **Execute & Diff:** The Agent will propose code changes. Use the side-by-side diff to verify logic.
4.  **Terminal Integration:** Use the Agent to run `make test` directly in the terminal to verify the fix.

---

## 4. Workflow: Other IDEs or Web Interfaces (Manual)

If you are using a standard text editor and a web-based AI (like Google AI Studio or ChatGPT), the workflow remains identical but the context management is manual.

1.  **Initialize Context:** Copy the relevant persona from `.agent/rules/` into the AI's "System Instructions" or the first prompt.
2.  **Provide Context:** Provide the AI with the source files relevant to the task.
3.  **Generate & Apply:** Ask the AI to execute the task. Manually copy the code into your editor.
4.  **Verify:** Run the build and test commands in your local terminal.

---

## 5. Technical Constraints for AI Agents

To maintain the legacy-compatible nature of `ytree`, all AI agents must strictly follow these technical constraints:

*   **Standards:** Use C89/C99 compatible code. Avoid modern C features that break portability on older Unix-like systems.
*   **Safety:** Always check return values of system calls (`malloc`, `chdir`, `open`, etc.) and handle `errno`.
*   **No "Zalgo" Text:** Strictly prohibited to use stacked Unicode characters (combining diacritics) in code, comments, or UI strings.
*   **Encapsulation:** Prioritize passing context structures (`Context *ctx`) over accessing global variables.

## 6. Debugging Protocol

If a bug cannot be identified through code review:
1.  **Instrument:** Direct the AI to create a mission solely to add debug `fprintf(stderr, ...)` calls to the relevant logic.
2.  **Observe:** Run the instrumented code and provide the terminal output back to the AI.
3.  **Solve:** Use the runtime evidence to guide the final fix.
