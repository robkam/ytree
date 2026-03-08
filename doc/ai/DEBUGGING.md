# Debugging Methodology

## External Expert Architecture Analysis

This document describes the evidence-based debugging methodology used to resolve complex architectural bugs in ytree, particularly the ncurses Z-order issues discovered in 2026-03.

### When to Use This Method

Use this approach when:

- Multiple seemingly unrelated bugs appear simultaneously
- Symptoms suggest architectural fragmentation or violated invariants
- Previous patch attempts made things worse or introduced new bugs
- The subsystem has well-documented canonical patterns (ncurses, X11, networking protocols, etc.)
- You have access to a high-quality LLM for architectural consultation

Do NOT use this method when:

- Single isolated bug with obvious fix
- No architectural concerns
- Surface bug in throwaway code
- Time-critical hotfix needed

### The Five-Step Process

#### Step 1: Stop Symptom Patching

When multiple surface bugs appear, resist the urge to patch each one individually.

**Key Questions:**
- Is this a symptom of a deeper architectural issue?
- Are these bugs related through a common subsystem?
- Have previous fixes made things worse?

If yes to any of these, STOP coding and move to analysis phase.

**Anti-Pattern to Avoid:**
```c
// Adding conditional checks to hide symptoms
if (in_big_window && !preview_mode && footer_visible) {
  // Patch symptom without understanding root cause
}
```

#### Step 2: Create Comprehensive Diagnostic Prompt

Document all observed bugs and formulate architectural questions about the subsystem.

**Template Structure:**

```
You are an expert in [SUBSYSTEM]. Analyze this codebase for
architectural violations of [CANONICAL PATTERN].

## Observed Bugs
1. [Bug A]: [Reproduction steps] - [Expected vs Actual behavior]
2. [Bug B]: [Reproduction steps] - [Expected vs Actual behavior]
...

## Relevant Files
- [file:line] - [Brief description of role]
- [file:line] - [Brief description of role]

## Key Architectural Questions
1. What are the canonical patterns for [SUBSYSTEM]?
2. What is the actual implementation structure in this codebase?
3. Where are the invariants? (e.g., ordering rules, lifecycle requirements)
4. What are the state transitions?
5. What tools or tests would detect violations?

## Your Task
Identify root causes with specific line numbers and call chains.
Provide evidence for each diagnosis.
DO NOT suggest fixes - only diagnose root causes.
```

**Example from ytree ncurses case:**

Key questions included:
- What are orthodox ncurses refresh patterns? (wnoutrefresh/doupdate model)
- What is ytree's window hierarchy? (stdscr, border, dir, file, menu, dialogs)
- What are the Z-order rules? (last staged window wins on overlaps)
- Where does stats panel draw? (separate window or into border?)
- What are the state transitions? (mode changes, window visibility)

#### Step 3: External Expert Consultation

Use a **separate, fresh LLM instance** as an architectural auditor. This is critical for avoiding confirmation bias and conversation baggage.

**Best Practices:**

1. **Use a high-quality model** (Claude Opus, GPT-4, etc.) for diagnosis
2. **Fresh context only** - do not continue from existing conversation
3. **Provide high-level map** - use **jCodeMunch** `get_repo_outline` or `get_file_tree` to show the expert the project structure and symbol counts before diving into specific files.
4. **Provide relevant source files** - key functions, not entire codebase. Use **Serena** `find_symbol` to extract only the relevant definitions.
5. **Request evidence-based diagnosis** - line numbers, call chains, not speculation
6. **Explicitly forbid suggesting fixes** - keep analysis separate from implementation

**Why Fresh Context Matters:**

- No confirmation bias from previous failed attempts
- No token budget wasted on conversation history
- Clean perspective on architectural issues
- Forces comprehensive problem statement in prompt

#### Step 4: Extract Surgical Fixes

Review the expert's diagnosis and identify the minimal set of changes that address root causes.

**Each fix must have:**

1. **Specific location** - file path and line number
2. **Evidence-based rationale** - not speculation or "might help"
3. **Expected impact** - which specific bugs it addresses
4. **Minimal scope** - change only what's necessary

**Target: 3-5 surgical edits, not large refactors**

**Example from ytree:**

```
Fix 1: src/ui/dialog.c:98
Root cause: UI_Dialog_RefreshAll() re-stages ctx_border_window
            AFTER ctx_menu_window, violating Z-order
Change: Add ctx_menu_window staging after border window
Impact: Fixes Bug A (footer occlusion), Bug B (footer bleed)

Fix 2: src/ui/ctrl_dir.c:216
Root cause: Duplicate doupdate() before RefreshView() call
Change: Delete redundant doupdate()
Impact: Eliminates flicker (2-3 commits -> 1 per keystroke)

Fix 3: src/ui/display.c:489-490
Root cause: Menu window staged even in preview mode
Change: Move menu staging inside else block
Impact: Fixes footer in F7 preview mode
```

#### Step 5: Sequential Implementation

Implement all related fixes atomically.

**Best Practices:**

1. **Use low-cost LLM instances** (Flash, GPT-3.5) for mechanical edits
2. **One file per instance** - saves tokens, reduces errors
3. **No premature testing** - compile once after all fixes applied
4. **Single atomic commit** - all related changes together

**Implementation Pattern:**

```bash
# Flash instance #1: Edit file A
# Flash instance #2: Edit file B
# Flash instance #3: Edit file C

# Then compile and test
make clean && make
pytest tests/

# Single atomic commit
git add src/ui/dialog.c src/ui/ctrl_dir.c src/ui/display.c
git commit -m "fix: resolve architectural issue X

- Fix 1: [file] - [change] - [impact]
- Fix 2: [file] - [change] - [impact]
- Fix 3: [file] - [change] - [impact]

Root cause: [diagnosis summary]
Based on analysis by [expert model]"
```

### Why This Works

1. **Separation of Concerns**
   - Analysis: High-quality model with fresh context
   - Implementation: Low-cost models for mechanical edits
   - Testing: Human verification and test suite

2. **Token Efficiency**
   - Expensive model only for diagnosis phase
   - Cheap models for straightforward edits
   - No conversation baggage or retry loops

3. **Evidence-Based**
   - Every fix maps to specific line numbers
   - Root causes identified, not symptoms
   - Testable predictions about bug resolution

4. **Orthodox Patterns**
   - Expert asked about "correct patterns" for the subsystem
   - Not "how to fix my specific code"
   - Identifies violations of canonical approaches

5. **Fresh Perspective**
   - External LLM has no investment in previous approaches
   - No confirmation bias from failed attempts
   - Can identify obvious issues obscured by familiarity

### Case Study: ytree ncurses Z-order Bugs (2026-03)

**Initial State:**
- 6 TUI bugs (A-F) affecting footer display, stats panel, separators
- Multiple failed patch attempts over several hours
- New bugs introduced by "fixes"
- 9 test failures after refactoring

**Failed Approaches:**
- Adding debug logging and instrumentation (wasted tokens)
- Conditional checks to hide symptoms (made things worse)
- Removing RefreshView() calls (architectural misdiagnosis)

**Successful Approach:**

1. **Stopped patching** after user asked "is this the proper way?"
2. **Created diagnostic prompt** about ncurses refresh patterns, window hierarchy, Z-order rules
3. **Consulted Claude Opus** (fresh instance) with comprehensive architectural questions
4. **Extracted 3 surgical fixes:**
   - UI_Dialog_RefreshAll() missing menu window staging
   - Duplicate doupdate() in HandleDirWindow() loop
   - Menu window staged unconditionally in preview mode
5. **Implemented atomically** via 3 Flash instances, single compile, single commit

**Key Insight:**

The root cause was a Z-order violation: `UI_Dialog_RefreshAll()` was re-staging `ctx_border_window` AFTER `RefreshView()` had already staged `ctx_menu_window`. In ncurses, the last staged window wins on overlaps. This single architectural error caused multiple surface bugs.

**Outcome:**

Three surgical edits (< 10 lines of code total) addressed the root cause, expected to resolve 6 surface bugs. Evidence-based diagnosis prevented further architectural damage from symptom patching.

### Prompt Pattern Reference

```
You are an expert in [SUBSYSTEM] (e.g., ncurses, X11, TCP/IP).

Analyze this codebase for architectural violations of [CANONICAL PATTERN]
(e.g., ncurses virtual screen model, X11 event loop, TCP state machine).

## Observed Bugs
[Numbered list with reproduction steps and expected vs actual behavior]

## Codebase Context
[Relevant files, key functions, current architecture]

## Subsystem Questions
1. What are the canonical patterns for [SUBSYSTEM]?
2. What is the actual implementation structure?
3. Where are the invariants and ordering requirements?
4. What are the lifecycle rules?
5. What would orthodox implementations do differently?

## Your Task
- Identify root causes with specific file:line references
- Provide evidence (call chains, state transitions, ordering violations)
- DO NOT suggest fixes - only diagnose root causes
- Focus on architectural violations, not surface symptoms
```

### Integration with ytree Development Workflow

This methodology complements the existing Spec-First development protocol documented in [WORKFLOW.md](WORKFLOW.md):

- **For new features:** Use Spec-First (design -> implement -> test)
- **For architectural bugs:** Use External Expert Analysis (diagnose -> extract fixes -> implement)
- **For simple bugs:** Direct fix with test coverage

See [ARCHITECTURE.md](../ARCHITECTURE.md) for system invariants and [TESTING.md](TESTING.md) for test protocols.

### References

- ncurses Programming HOWTO: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
- ytree Architecture: [ARCHITECTURE.md](../ARCHITECTURE.md)
- ytree Specification: [SPECIFICATION.md](../SPECIFICATION.md)

---

**Document History:**
- 2026-03-08: Initial version documenting ncurses Z-order bug resolution methodology
