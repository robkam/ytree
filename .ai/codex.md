# Codex Instructions

Codex-specific behavior for this repository.

## Scope

- Follow all rules in `.ai/shared.md`.
- Use `AGENTS.md` at repo root only as a discovery stub.

## Commit Messages

- Follow `.ai/shared.md` rule 8. Enforcement lives in `.githooks/commit-msg`.
- Prefer outcome-first wording and never use workflow labels (`task`, `step`) or numeric workflow IDs in commit text.

## Codex Notes

- Prefer direct implementation over long planning unless design is explicitly requested.
- Keep edits minimal and coherent. You MUST preserve existing style and architecture unless the requested task explicitly requires a refactor that still complies with `.ai/shared.md`.
- Before any broad refactor, you MUST use repo search and inspect the relevant docs to confirm the affected surface area.

## Context Budget

- Startup instructions are session-scoped: load once, then continue in delta-only mode.
- Do not echo full instruction files, full user boilerplate, or repeated status recaps unless the user explicitly asks for a full recap.
- Operational updates should include only: completed state, current next action, and newly-created/changed handles.

## Canonical Test Execution

- Run tests from repository root: `~/ytree`.
- Activate the project virtual environment first: `source .venv/bin/activate`.
- Canonical full suite command: `pytest`.
- The pytest suite uses `pexpect` PTYs; always run pytest with host permissions (non-sandboxed command execution) so PTY allocation works.
- Do not run pytest in sandbox first. Start with host-permission execution immediately for any pytest command (`pytest`, `pytest -q`, targeted tests, or `-k` runs).
- For audit targets (`make qa-all` and `make qa-*`), run with host permissions (non-sandboxed command execution) from the start.
- Do not run `make qa-*` in sandbox first and then retry; execute host-first to avoid predictable permission failures and duplicate runs.

## MCP Doctor

- Run `make mcp-doctor` when MCP tools fail to start or disappear.
- Run `make mcp-doctor FIX=1` to bootstrap missing `~/.codex/config.toml` from repo `.codex/config.toml` and auto-add writable `UV_CACHE_DIR`/`UV_TOOL_DIR` env blocks.

## Mandatory Audit Cadence

- For every feature-sized change, major change, and PR update, rerun the full audit loop defined in `doc/AUDIT.md` before marking work complete.
- Treat this as a required gate, not an optional checklist.
