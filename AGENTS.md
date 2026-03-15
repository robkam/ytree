# CODEX.md

Discovery stub for Codex tooling.

Canonical Codex instructions: `./.ai/codex.md`
Shared instructions for all agents: `./.ai/shared.md`

Docs note: `doc/USAGE.md` is generated from `etc/ytree.1.md`; edit `etc/ytree.1.md` as source.

Testing quick reference (Codex):
- Run from repo root: `/home/rob/ytree`
- Activate venv: `source .venv/bin/activate`
- Full suite command: `pytest`
- Always run pytest with host permissions from the start (no sandbox-first run), because PTY-based tests require unrestricted PTY allocation.
