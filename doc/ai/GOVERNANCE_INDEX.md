# AI Governance Index

## Scope and Intent

This page is the canonical navigation map for AI-governance documentation in `ytree`: what each file governs, where to edit, and how to avoid policy drift across duplicate copies.

## Root Discovery Stubs (Repository Root)

Root stubs are entry points only. They should stay short and contain:
- Pointer to the provider canonical file under `.ai/`.
- Pointer to shared policy in `.ai/shared.md`.
- Small, provider-relevant quick reference (for example, common test invocation notes).
- No duplicated long-form policy text that belongs in canonical files.

Files:
- `AGENTS.md` (Codex discovery stub)
- `CLAUDE.md` (Claude discovery stub)
- `GEMINI.md` (Gemini discovery stub)

## Canonical Provider and Shared Policy Files

Primary governance sources:
- Shared cross-provider policy: `.ai/shared.md`
- Codex provider policy: `.ai/codex.md`
- Claude provider policy: `.ai/claude.md`
- Gemini provider policy: `.ai/gemini.md`

Ownership model:
- Shared policy changes are centralized in `.ai/shared.md`.
- Provider-specific execution details live only in that provider's `.ai/<provider>.md`.
- Root stubs mirror discoverability only; they do not become policy sources.

## Persona Rules vs Skill Procedures

Keep role and procedure guidance separated:
- Persona role rules and boundaries: `.agent/rules/*`
- Skill procedures and checklists: `.ai/skills/*/SKILL.md`

Use persona files for role posture and constraints; use skill files for step-by-step workflows.

## Edit Here, Not There

For common governance edits, use these canonical targets:

| Change Type | Edit Here (Canonical) | Do Not Treat As Canonical |
| --- | --- | --- |
| Rule shared across all providers | `.ai/shared.md` | `AGENTS.md`, `CLAUDE.md`, `GEMINI.md` |
| Codex-only behavior | `.ai/codex.md` | `AGENTS.md` |
| Claude-only behavior | `.ai/claude.md` | `CLAUDE.md` |
| Gemini-only behavior | `.ai/gemini.md` | `GEMINI.md` |
| Persona role boundary or posture | `.agent/rules/<persona>.md` | `.ai/skills/*` |
| Repeatable operational checklist/procedure | `.ai/skills/<skill>/SKILL.md` | `.agent/rules/*` |
| Workflow policy/process narrative | `doc/ai/WORKFLOW.md` | Root stubs |

## Drift-Prevention Checklist for Planner Packet Handoffs

Before handing off or closing a packet:
1. Confirm each requested governance change has one canonical target file.
2. Edit canonical files first; add at most minimal discovery pointers elsewhere.
3. Avoid copying long policy blocks across stubs, persona rules, and workflow docs.
4. Verify changed-file scope matches packet allowlist exactly.
5. Record any assumption mismatch with concrete path evidence in the report.
6. Set final status to drift only when a real repository-vs-packet mismatch exists.
7. Run `python3 scripts/check_ai_governance_drift.py` to verify root-stub invariants before handoff.
