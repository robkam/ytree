---
name: manpage-sync
description: Keep ytnova manpage and usage docs synchronized by editing etc/ytnova.1.md as source and regenerating derived outputs.
---

# Manpage Sync

Use this skill when commands, options, keybindings, or user-facing behavior docs change.

## Source of Truth

- Edit `etc/ytnova.1.md`.
- Treat `docs/USAGE.md` as generated output.

## Sync Workflow

1. Apply documentation edits in `etc/ytnova.1.md`.
2. Regenerate usage doc:
   - `make docs`
3. If needed, regenerate man page artifact:
   - `make build/ytnova.1`
4. Verify generated docs reflect current behavior.

## Checks

- No direct manual edits to `docs/USAGE.md` without regenerating.
- Options/commands in docs match implemented behavior.
- Prompt/menu labels are consistent with current UI text.
- Documentation placement MUST be audience-relevant and non-duplicative; you MUST NOT scatter the same guidance across unrelated sections.
