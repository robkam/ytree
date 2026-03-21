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

## Output Contract

- Goal
- Files to Modify
- Context Files
- Instructions
- Acceptance Criteria
- Risks and Invariants

## Guardrails

- Prefer root-cause fixes over patchwork.
- Keep architecture stable and deterministic.
- If critical context is missing, request only the specific file or behavior detail needed.
