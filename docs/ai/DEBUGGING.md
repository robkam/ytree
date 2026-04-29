# Debugging Methodologies

This document defines the objective methodologies for debugging the `ytree` codebase. These methods prioritize evidence over guesswork and ensure that fixes address root causes rather than symptoms.

---

## 1. Targeted Root Cause Analysis

### When to Use
Use this lighter-weight approach when:
- You can isolate specific failing vs working cases.
- The bug is localized to a subsystem.
- You have semantic tools (**Serena**/**jCodemunch**) available.
- A full **External Expert Analysis** would be overkill.

### Core Techniques

#### 1.1 Comparative Analysis
When part of the system works but a similar part doesn't:
1. **Find working example** (e.g., "FILTER VOLUME renders correctly").
2. **Find broken example** (e.g., "CURRENT section missing junctions").
3. **Use semantic search** to compare implementations.
4. **Apply working pattern** to broken code.

#### 1.2 Hypothesis-Driven Search
When you have a theory about the cause:
1. **Form specific hypothesis** (e.g., "removed `doupdate()` causes delay").
2. **Use semantic tools** to find exact code location.
3. **Verify hypothesis** with minimal change.
4. **Test** to confirm/refute.

#### 1.3 Semantic Tool Leverage
Instead of instrumentation:
- Use `mcp__serena__find_symbol()` to locate functions.
- Use `mcp__serena__search_for_pattern()` for specific patterns.
- Use `mcp__serena__find_referencing_symbols()` to trace usage.
- Compare implementations directly in code.

#### 1.4 Direct Surgical Fixes
Based on understanding:
- Change 1-3 lines only.
- Target root cause, not symptoms.
- Test immediately.
- Revert if hypothesis is wrong.

---

## 2. The "Hands-Off" Fix Mandate (The Agentic Loop)

### When to Use
The mandatory focused procedure for **Antigravity** and agentic execution. It forces the agent to prove its understanding by writing a failing test *before* editing any code. See **[TESTING.md](TESTING.md)** for detailed test protocols and harness documentation.

### The Process

1.  **Replicate in Python (The Test):** Describe the manual steps that cause the bug to the **Tester** persona. Instruct it: *"Write a `pytest` using the `pexpect` TUI harness that performs these exact steps and asserts the expected correct behavior. This test MUST fail currently."*
2.  **Verify the Failure (Red):** Run `make test`. Confirm the new test fails exactly where the bug manifests.
3.  **Assign the Fix (The Code):** Hand the failing test output and the relevant C source files to the **Developer** persona. Instruct it: *"Modify the C code to make this specific test pass. Do not change the test."*
4.  **Verify the Fix (Green):** Run `make test`. A passing test provides mathematical proof of the fix and creates a permanent regression test.

---

## 3. Instrumentation (The Discovery Loop)

### When to Use
Used primarily for high-level research in **AI Studio** or complex exploratory debugging. It forces the AI to "find out" what is actually happening before proposing a fix.

### The Process

1.  **Instrument:** Use the keyword **"Instrument"** to direct the AI to add `fprintf(stderr, ...)` calls to trace state transitions (e.g., tracing `ActivePanel` focus).
2.  **Observe:** Run the instrumented code and provide terminal output to the AI.
3.  **Solve:** Use the empirical runtime evidence to guide the final fix back to Specification compliance.

---

## 4. External Expert Architecture Analysis

### When to Use
Use this approach when:
- Multiple seemingly unrelated bugs appear simultaneously.
- Symptoms suggest architectural fragmentation or violated invariants.
- Previous patch attempts made things worse or introduced new bugs.
- The subsystem has well-documented canonical patterns (ncurses, networking protocols, etc.).
- You have access to a high-quality LLM for architectural consultation.

### The Five-Step Process

#### Step 1: Stop Symptom Patching
When multiple surface bugs appear, resist the urge to patch each one individually. STOP coding and move to the analysis phase if symptoms suggest a deeper issue.

#### Step 2: Create Comprehensive Diagnostic Prompt
Document all observed bugs and formulate architectural questions about the subsystem. Use the template below:

```markdown
You are an expert in [SUBSYSTEM]. Analyze this codebase for architectural violations of [CANONICAL PATTERN].

## Observed Bugs
1. [Bug A]: [Reproduction steps] - [Expected vs Actual behavior]
...

## Relevant Files
- [file:line] - [Brief description of role]

## Key Architectural Questions
1. What are the canonical patterns for [SUBSYSTEM]?
2. What is the actual implementation structure in this codebase?
3. Where are the invariants?
...

## Your Task
Identify root causes with specific line numbers and call chains. Provide evidence for each diagnosis. DO NOT suggest fixes.
```

#### Step 3: External Expert Consultation
Use a **separate, fresh LLM instance** (e.g., Claude Opus) as a **Code Auditor** for architectural analysis. Provide a high-level map using **jCodeMunch** and relevant symbols via **Serena**.

#### Step 4: Extract Surgical Fixes
Review the expert's diagnosis and identify the minimal set of changes (target 3-5 surgical edits).

#### Step 5: Sequential Implementation
Implement fixes atomically using low-cost models (e.g., Gemini Flash) for mechanical edits. Compile and test once all related fixes are applied.

---

### See also
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System invariants and window hierarchy
- [SPECIFICATION.md](../SPECIFICATION.md) - The behavioral "Contract of Truth"
