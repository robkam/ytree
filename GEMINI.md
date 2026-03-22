# GEMINI.md

Discovery stub for Gemini tooling.

Canonical Gemini instructions: `./.ai/gemini.md`
Shared instructions for all agents: `./.ai/shared.md`

Persona quick switch:
- Full names: `:at architect`, `:at developer`, `:at code_auditor`, `:at tester`, `:at greybeard`
- Abbreviations: `:at a`, `:at d`, `:at c`, `:at t`, `:at g`
- Parsing rule: persona switching only triggers when `:` is in column 1 and `:at` occupies columns 1-3 (`:at ...` at line start).
- Default when no explicit override is `architect` (see `./.ai/shared.md`)

Persona skill auto-load:
- Skills live in `./.ai/skills/*/SKILL.md`.
- Optional explicit skill controls: `use skill <name>`, `skip skill <name>`, `only skill <name>[,<name>...]`, `reset skills`
