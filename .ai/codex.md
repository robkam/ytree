# Codex Instructions

Codex-specific behavior for this repository.

## Scope

- Follow all rules in `.ai/shared.md`.
- Use `CODEX.md` at repo root only as a discovery stub.

## Codex Notes

- Prefer direct implementation over long planning unless design is explicitly requested.
- Keep edits minimal and coherent; preserve existing style and architecture.
- Use fast repo search and inspect docs before broad refactors.

## Canonical Test Execution

- Run tests from repository root: `/home/rob/ytree`.
- Activate the project virtual environment first: `source .venv/bin/activate`.
- Canonical full suite command: `pytest`.
- The pytest suite uses `pexpect` PTYs; if sandboxed execution blocks `os.openpty()` (for example, `PermissionError: [Errno 13]` or `OSError: out of pty devices`), run the same canonical command with host PTY permissions.
