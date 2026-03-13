# AGENTS.md

This file provides guidance to Codex when working with code in this repository.

## Canonical Source Of Truth

Use `~/.gemini/GEMINI.md` as the primary, canonical instruction source.

- Do not duplicate global directives from Gemini here.
- Do not create alternative versions of the same policy in AGENTS.
- If a directive exists in Gemini, inherit it directly.

## Repository Adaptation Layer

Use [GEMINI.md](~/ytree/GEMINI.md) for project-level directives, and [CLAUDE.md](~/ytree/CLAUDE.md) for the technical adaptation layer (architecture, build/test commands, file map, workflows).

- Treat `GEMINI.md` as the local authority.
- Reuse `CLAUDE.md` sections by reference; do not copy their content into AGENTS.

When instructions overlap, apply this precedence:
1. `~/.gemini/GEMINI.md` (Global Master)
2. [GEMINI.md](~/ytree/GEMINI.md) (Repo Master)
3. [CLAUDE.md](~/ytree/CLAUDE.md) (Tech Specs)
4. `AGENTS.md` (Codex-only/tool-specific deltas)

## Codex-Specific Delta Policy

Only add content to this file when it is specific to Codex tooling behavior and cannot live in Gemini or Claude shared docs.
