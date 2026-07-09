---
name: architect-planning
description: Plan ytnova changes from behavior to implementation tasks with files, risks, invariants, and acceptance criteria. Use for design and technical-approach questions.
---

# Architect Planning

Use this skill when the active persona is `architect`.

## Workflow

1. Restate requested behavior in one precise sentence.
2. Identify constraints from `docs/SPECIFICATION.md`, `docs/ARCHITECTURE.md`, and `.ai/shared.md`.
3. List affected files and likely symbols, then build the explicit in-scope inventory the developer will need to reconcile.
4. Propose the largest coherent adjacent implementation batch that preserves reviewable scope; for migration or enforcement work, batch by boundary family rather than helper-by-helper.
5. Include acceptance criteria that can be tested, including what final sweep proves the unit is actually complete.
6. For interactive UI flow changes, include UX economy metrics:
   - current chain
   - proposed chain
   - common-path submenu depth
   - fast-path behavior

## Output Contract

- Goal
- Files to Modify
- Context Files
- Inventory to Reconcile
- Instructions
- Acceptance Criteria
- Risks and Invariants
- UX Economy Criteria (for interactive flows)

When generating a stateless developer handoff artifact, you MUST also require:
- developer creates only the minimal temporary relay file(s) actually needed for the active work item
- developer deletes consumed relay files when the work item reaches a neutral stop state
- developer completion reply is a delta-only status line with concrete evidence handles, not a hardcoded task-numbered file-path formula

## Guardrails

- You MUST implement root-cause fixes over patchwork.
- Keep architecture stable and deterministic.
- Boundary-family batching gate is mandatory: you MUST plan the largest coherent adjacent batch that shares the same owner boundary, generation domain, risk class, and validation path; you MUST NOT plan helper-by-helper micro-PR slicing unless a material split condition exists.
- Coverage-proof gate is mandatory: every non-trivial task, migration, or bugfix plan MUST name the in-scope inventory, require reconciliation of each inventory item before completion, and state the final sweep needed to prove no in-scope surfaces were silently omitted.
- If critical context is missing, request only the specific file or behavior detail needed.
- UX economy gate is mandatory: target `key -> Enter -> result` and no more than one submenu on common path unless justified with equivalent fast path.
- Documentation signal rule: you MUST plan doc changes only in the most relevant canonical section/file for the target reader; you MUST NOT broadcast AI/process notes across unrelated sections.
