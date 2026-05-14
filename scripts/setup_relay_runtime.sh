#!/usr/bin/env bash
# Prepare Python-first relay user config.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
SKIP_MCP_DOCTOR=0

usage() {
  cat <<'USAGE'
Usage: scripts/setup_relay_runtime.sh [--skip-mcp-doctor]

Sets up Python relay defaults in ~/.config/ytree/relay.toml.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-mcp-doctor) SKIP_MCP_DOCTOR=1 ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [[ "$SKIP_MCP_DOCTOR" -eq 0 ]]; then
  (
    cd "$REPO_ROOT"
    make mcp-doctor FIX=1
  )
fi

config_dir="$HOME/.config/ytree"
config_path="$config_dir/relay.toml"
mkdir -p "$config_dir"
if [[ ! -f "$config_path" ]]; then
  cat > "$config_path" <<'TOML'
backend = "codex"
runtime_dir = "/tmp/ytree-relay"
stale_seconds = 86400
codex_command = ["codex", "exec", "--prompt-file", "{prompt_path}"]
codex_timeout_seconds = 3600
TOML
  echo "Created $config_path"
else
  echo "Config already exists: $config_path"
fi

echo "Primary run command: scripts/relay.py run --bug <id>"
echo "Alternative run command: scripts/relay.py run --task <id>"
