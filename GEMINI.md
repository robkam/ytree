# GEMINI.md

Discovery stub for Gemini tooling.

Canonical Gemini instructions: `./.ai/gemini.md`
Shared instructions for all agents: `./.ai/shared.md`

Persona quick switch:
- Full names: `use architect`, `use developer`, `use code_auditor`, `use tester`, `use greybeard`
- Abbreviations: `use a`, `use d`, `use c`, `use t`, `use g`
- Default when no explicit override is `architect` (see `./.ai/shared.md`)

Persona skill auto-load:
- Skills live in `./.ai/skills/*/SKILL.md`.
- Optional explicit skill controls: `use skill <name>`, `skip skill <name>`, `only skill <name>[,<name>...]`, `reset skills`
