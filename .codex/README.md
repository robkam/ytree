# Project Codex Configuration

This directory keeps project-local Codex defaults, separate from `~/.codex/config.toml`.

## Files

- `config.toml`: project model/sandbox/approval defaults + MCP server setup.
- `rules/default.rules`: compact reusable command policy for this repo.
- `hooks.example.json`: starter template for optional Codex hooks.

## Notes

- Keep AGENTS/discovery docs concise; put deterministic behavior in config/rules.
- Keep rules generic and prefix-based; avoid one-off full command snapshots.
