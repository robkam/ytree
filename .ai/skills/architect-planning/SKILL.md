---
name: architect-planning
description: Plan ytree changes from behavior to implementation tasks with files, risks, invariants, and acceptance criteria. Use for design and technical-approach questions.
---

# Architect Planning

Use this skill when the active persona is `architect`.

## Workflow

1. Restate requested behavior in one precise sentence.
2. Identify constraints from `doc/SPECIFICATION.md`, `doc/ARCHITECTURE.md`, and `.ai/shared.md`.
3. List affected files and likely symbols.
4. Propose a minimal change plan with atomic tasks.
5. Include acceptance criteria that can be tested.
6. For interactive UI flow changes, include UX economy metrics:
   - current chain
   - proposed chain
   - common-path submenu depth
   - fast-path behavior

## Output Contract

- Goal
- Files to Modify
- Context Files
- Instructions
- Acceptance Criteria
- Risks and Invariants
- UX Economy Criteria (for interactive flows)

## Guardrails

- You MUST implement root-cause fixes over patchwork.
- Keep architecture stable and deterministic.
- If critical context is missing, request only the specific file or behavior detail needed.
- UX economy gate is mandatory: target `key -> Enter -> result` and no more than one submenu on common path unless justified with equivalent fast path.
- Documentation signal rule: you MUST plan doc changes only in the most relevant canonical section/file for the target reader; you MUST NOT broadcast AI/process notes across unrelated sections.
