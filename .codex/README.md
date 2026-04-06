# Project Codex Configuration

This directory keeps project-local Codex defaults, separate from `~/.codex/config.toml`.

## Files

- `config.toml`: project model/sandbox/approval defaults + MCP server setup.
- `rules/default.rules`: compact reusable command policy for this repo.
- `hooks.example.json`: starter template for optional Codex hooks.

## Clone Bootstrap

Run this once per clone:

```bash
make mcp-doctor FIX=1
```

Behavior:
- If `~/.codex/config.toml` is missing, it is bootstrapped from this repo's `.codex/config.toml`.
- Existing `~/.codex` personal files (`auth.json`, sessions, local rules, etc.) stay in `~` and are not copied into the repo.
- If `~/.codex/config.toml` already exists, `mcp-doctor` validates it and can add missing MCP `env` blocks.
- `mcp-doctor` warns when MCP server config references home paths (for example `/home/...`): personal overrides are allowed, but shared defaults must be edited in repo `.codex/config.toml`.

## Notes

- Keep AGENTS/discovery docs concise; put deterministic behavior in config/rules.
- Keep rules generic and prefix-based; avoid one-off full command snapshots.
