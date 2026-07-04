---
name: code-quality
description: Prevent and remove high-impact code-quality issues with principle-driven, risk-ranked recommendations and optional implementation.
---

# Code Quality

Use this skill when the request is about code quality, clean-code enforcement, deslop-style review, or cleanup/refactor recommendations.

## Objective

- Detect concrete code-quality issues in the requested scope.
- Prioritize fixes by impact, risk, and effort.
- Offer implementation only after presenting findings and obtaining user approval.
- Keep implementations lean and readable without introducing clever/opaque patterns.

## Lean-Readable Contract (Mandatory)

Apply these conventions for all code-quality remediation:

1. Prefer the simplest clear implementation that preserves behavior and invariants.
2. Fewer lines is good only when readability and diagnosability are equal or better.
3. Avoid shorthand/clever constructs that reduce clarity (dense nested ternaries, implicit side effects, compressed control flow).
4. Use explicit names for ownership and state transitions; avoid ambiguous abbreviations outside local loop indices.
5. Keep control flow straightforward (guard clauses over deep nesting where practical).

## Anti-Obfuscation Rules (Mandatory)

Flag as smell (P1-P2 depending risk/context):

- Dense expression chains that hide branching or side effects.
- Flag-heavy APIs that multiplex unrelated behavior.
- Hidden mutation through shared/global state without explicit contract.
- Over-compressed helpers that trade maintainability for brevity.
- Unnecessary indirection layers introduced without rule-of-three evidence.

When suggesting a rewrite, include a short rationale: "why clearer" and "why safer to maintain."

## Recursion Policy

Recursion is allowed only when it is the clearest and safest representation of hierarchical traversal.

Allowed patterns:
- Bounded tree traversal with explicit base cases.
- Recursive walk/cleanup routines with deterministic termination.

Discouraged patterns:
- Business-flow recursion where iterative control flow is clearer.
- Recursion that obscures state transitions or makes error handling harder to follow.

Review rule:
- For any retained or introduced recursion, include a one-line justification and base-case/termination note.

## Workflow

1. Define the target (`file`, `directory`, or repository slice) and keep scope explicit.
2. Read only the minimum required context with semantic tools.
3. Map findings to principles/smells (for example: DRY, SRP, guard clauses, cognitive load, comments-as-deodorant).
4. Rank each finding using this priority scale:
   - `P0`: security/data-loss/corruption risk
   - `P1`: bug-prone behavior or correctness risk
   - `P2`: maintainability risks that will slow normal changes
   - `P3`: low-risk readability/polish issues
   - `P4`: optional stylistic cleanup
5. Apply effort modifiers:
   - quick win (<5 min) can move one level higher
   - risky/no-test change can move one level lower
6. Output findings with concrete before/after snippets and rationale.
7. Ask whether to implement all, some, or none of the recommended changes.
8. If implementation is approved, execute minimal coherent edits and run relevant validation.

## Comprehensive ytnova-Specific Smell Checklist

### 0) Conceptual Integrity and Architecture Conformance

- Treat the codebase as one integrated product, not a pile of locally correct patches.
- Check whether the implementation still conforms to the intended architecture, not just whether each edited file works in isolation.
- Look for architectural drift when the code no longer matches the documented design, and architectural erosion when the implementation violates architectural constraints.
- Look for shotgun surgery when one logical change forces scattered edits across many files.
- Look for duplicate or parallel implementations that survive after a refactor, migration, or feature replacement.
- Look for sibling features that implement the same concept with divergent semantics, naming, ordering, polarity, or ownership.
- Look for half-migrated replacements where old code remains reachable, inert, or only partially retired.

Audit questions:
- Is there exactly one authoritative implementation for each durable behavior?
- Does the implementation still conform to the intended architecture and documented invariants?
- Are any surviving alternatives intentionally transitional, or are they accidental leftovers?
- Do similar code paths still express the same concept in the same way?
- Would a maintainer understand the system as a coherent whole, or as stitched-together fragments?

Required response when this smell is found:
- Name the surviving duplicate, drift, or erosion point.
- State whether it is dead, transitional, or still user-reachable.
- Explain the architectural or conceptual-integrity breakage it causes.
- Recommend the single durable owner or consolidation path.

### 1) Architecture and Module Boundaries

- Logic for non-controller concerns placed in `src/ui/ctrl_*.c` instead of dedicated modules.
- New feature logic added as controller sub-functions even though it could be reused elsewhere.
- Cross-panel state coupling that breaks active-panel isolation.
- Hidden state transfer through globals instead of explicit context passing.
- Large "manager"/"helper" files with mixed responsibilities.
- Shotgun surgery patterns where one behavior change requires many unrelated file edits.

### 2) State Ownership, Lifetime, and Memory Safety

- Ambiguous ownership of pointers and buffers at function boundaries.
- Missing `free`/cleanup on one or more error exits.
- Borrowed pointer stored beyond owner lifetime.
- Mutating shared state from multiple paths without clear invariants.
- Unsafe or fragile string handling patterns.
- Error-path cleanup asymmetry (happy-path cleanup is present, failure-path cleanup is partial).

### 3) Error Handling and Defensive Behavior

- Silent failure (`return` without signal/log when failure matters).
- Inconsistent error contracts for similar operations.
- Deep nesting where guard clauses would flatten control flow.
- Fail-late behavior where invalid inputs propagate before rejection.
- Missing validation at boundary points (user input, filesystem responses, command arguments).
- Retries or recovery branches that hide deterministic failures.

### 4) Performance and Event-Loop Determinism

- Performance regressions in interactive flows, especially input latency, redraw lag, and avoidable blocking.
- Event-loop nondeterminism that changes user-visible behavior under resize, watcher, or bursty input conditions.
- Signal-safety violations that introduce complex logic, I/O, or ncurses calls into signal handlers.
- Blocking work on the main loop when a non-blocking or deferred path is already the established architecture.

### 5) UI/TUI Flow and Interaction Economy

- Common path requires more than one submenu before result.
- Prompt chains that can be collapsed into one prompt with defaults/toggles.
- Inconsistent key behavior across similar screens.
- Surprising side effects from query-like operations.
- Redraw/update behavior that causes flicker or stale panel state.
- UX confirmation friction on non-destructive common-path operations.

### 6) API, Data Shape, and Logic Clarity

- Long parameter lists suggesting data clumps.
- Flag-heavy function signatures (`bool do_x, bool do_y`) indicating mixed responsibilities.
- Primitive obsession for domain concepts (paths, modes, identifiers, options).
- Duplicate business rules implemented in multiple locations.
- Message-chain style calls that expose internal object structure.
- Magic numbers/strings hiding domain meaning.

### 7) Complexity and Readability

- Functions with multiple abstraction levels mixed together.
- Functions with deep conditional nesting or high branching complexity.
- Comment-heavy code that explains "what" instead of expressing intent in names.
- Over-abstraction with single-use interfaces or speculative extension points.
- Wrong abstraction smell: generic helper gains branch flags per new callsite.
- Dead code, unreachable branches, or stale fallback logic.

### 8) Tests, QA Signals, and Regression Risk

- Missing regression tests for bugfixes.
- Tests added only after fix, without fail-first proof.
- Brittle PTY/pexpect tests with timing hacks instead of synchronization fixes.
- Test behavior diverges from specification.
- Local suppressions/skips/xfails used to bypass root-cause remediation.
- Changed behavior without corresponding spec/test updates.

### 9) Documentation and Comment Hygiene

- Stale comments that no longer match behavior.
- Change-diary comments in source files.
- Duplicate process guidance scattered in unrelated docs.
- Missing rationale comments where invariants or ownership rules are non-obvious.
- Overly broad docs updates that increase noise without helping the local audience.

## Severity and Prioritization Rules

- `P0`: security issues, memory corruption, data loss, destructive behavior surprises.
- `P1`: correctness bugs waiting to happen, broken invariants, high-likelihood regressions.
- `P2`: maintainability and architecture drift with medium-term delivery risk.
- `P3`: readability/polish issues with low immediate risk.
- `P4`: optional style and consistency cleanup.
- Raise severity one level for quick wins.
- Lower severity one level when change risk is high and coverage is weak.

## Required Output Format

1. Summary (1-2 sentences)
2. Violations Found (grouped by principle/smell), each including:
   - `Location`
   - `Problem`
   - `Before`
   - `After`
   - `Why`
3. Recommendations (prioritized list)
4. Explicit implementation question to the user

## Recurring Burn-Down Cadence + Evidence Template

For debt-burn-down missions (not one-off bugfixes), report measurable deltas:

1. **Before hotspot table** (top N by size/complexity/smell impact in scope)
2. **After hotspot table** (same rows, updated metrics)
3. **Delta summary** (what reduced, what deferred, and why)
4. **Risk note** (behavior-preservation checks and invariants touched)
5. **Validation evidence** (commands run and outcomes)

Default cadence guidance for maintainers:
- Run a bounded burn-down pass periodically (for example every N feature merges or at milestone checkpoints), not only pre-release.

## ytnova Guardrails

- Do not introduce speculative abstractions before a clear rule-of-three signal.
- Prefer root-cause fixes over cosmetic rewrites.
- Preserve architectural invariants (explicit context passing, panel isolation, deterministic single-thread behavior).
- For tests, favor clarity and deterministic behavior over over-DRY abstraction.
- Keep comments limited to invariants and non-obvious rationale; remove stale comments.
- Do not broaden scope into unrelated refactors unless the user explicitly approves.
