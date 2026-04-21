# Gemini Instructions

Gemini-specific behavior for this repository.

## Scope

- Follow all rules in `.ai/shared.md`.
- Use `GEMINI.md` at repo root only as a discovery stub.

## Commit Messages

- Follow `.ai/shared.md` rule 8. Enforcement lives in `.githooks/commit-msg`.
- Prefer outcome-first wording and never use workflow labels (`task`, `step`) or numeric workflow IDs in commit text.

## Context Budget

- Treat startup instructions as session-scoped: load once unless files changed or the maintainer requests reload.
- After startup, use delta-only reporting with only net-new state and next action unless a full recap is requested.

## Gemini Notes

- Keep responses concise and implementation-focused.
- You MUST preserve existing architecture unless the requested task explicitly requires a refactor that still complies with `.ai/shared.md`.
- When uncertain, you MUST inspect docs and source before proposing changes.
