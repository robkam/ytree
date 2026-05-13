#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
SYSTEM_MODE=0
SKIP_MCP_DOCTOR=0
NO_START=0

usage() {
  cat <<'USAGE'
Usage: scripts/setup_relay_runtime.sh [--system] [--skip-mcp-doctor] [--no-start]

One-time bootstrap for durable relay runtime on WSL/systemd.

Options:
  --system           Install system units (requires root)
  --skip-mcp-doctor  Skip MCP config bootstrap
  --no-start         Install only; do not start workers
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --system)
      SYSTEM_MODE=1
      ;;
    --skip-mcp-doctor)
      SKIP_MCP_DOCTOR=1
      ;;
    --no-start)
      NO_START=1
      ;;
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

if ! command -v systemctl >/dev/null 2>&1; then
  echo "ERROR: systemctl not found. systemd must be available." >&2
  exit 1
fi

pid1_comm="$(ps -p 1 -o comm= | tr -d '[:space:]')"
if [[ "$pid1_comm" != "systemd" ]]; then
  echo "ERROR: PID 1 is '$pid1_comm', not systemd." >&2
  echo "WSL fix: add [boot] systemd=true to /etc/wsl.conf then run 'wsl --shutdown' from Windows." >&2
  exit 1
fi

if [[ "$SYSTEM_MODE" -eq 0 ]]; then
  export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
  export DBUS_SESSION_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS:-unix:path=${XDG_RUNTIME_DIR}/bus}"
  if ! systemctl --user show-environment >/dev/null 2>&1; then
    echo "ERROR: systemd user bus is unavailable." >&2
    echo "WSL fix: restart WSL ('wsl --shutdown') and reopen the distro shell, then rerun setup." >&2
    exit 1
  fi
fi

if [[ "$SKIP_MCP_DOCTOR" -eq 0 ]]; then
  echo "[setup-relay] Running MCP bootstrap check"
  (
    cd "$REPO_ROOT"
    make mcp-doctor FIX=1
  )
fi

if [[ "$SYSTEM_MODE" -eq 1 ]]; then
  env_target="/etc/ytree/relay.env"
else
  env_target="$HOME/.config/ytree/relay.env"
fi

if [[ ! -f "$env_target" ]]; then
  echo "[setup-relay] Creating relay env file at $env_target"
  install -d "$(dirname "$env_target")"
  sed "s|%h|$HOME|g" "$REPO_ROOT/infra/systemd/relay.env.example" > "$env_target"
else
  echo "[setup-relay] Existing relay env file found at $env_target"
fi

if ! grep -q '^RELAY_DEVELOPER_CMD=' "$env_target"; then
  echo "ERROR: RELAY_DEVELOPER_CMD missing in $env_target" >&2
  exit 1
fi
if ! grep -q '^RELAY_CODE_AUDITOR_CMD=' "$env_target"; then
  echo "ERROR: RELAY_CODE_AUDITOR_CMD missing in $env_target" >&2
  exit 1
fi
if grep -Eq '^RELAY_DEVELOPER_CMD=(/usr/bin/true|/bin/true|true)$' "$env_target"; then
  echo "ERROR: RELAY_DEVELOPER_CMD in $env_target is still placeholder '/usr/bin/true'." >&2
  echo "       Set RELAY_DEVELOPER_CMD to your real developer worker command before start-run." >&2
  exit 1
fi
if grep -Eq '^RELAY_CODE_AUDITOR_CMD=(/usr/bin/true|/bin/true|true)$' "$env_target"; then
  echo "ERROR: RELAY_CODE_AUDITOR_CMD in $env_target is still placeholder '/usr/bin/true'." >&2
  echo "       Set RELAY_CODE_AUDITOR_CMD to your real auditor worker command before start-run." >&2
  exit 1
fi
for required_key in \
  RELAY_POLICY_BLOCK_RETRY_LIMIT \
  RELAY_MAINTAINER_HEARTBEAT_SECONDS \
  RELAY_NO_PROGRESS_STALL_SECONDS \
  RELAY_MAINTAINER_INTERRUPT_REASONS
do
  if ! grep -q "^${required_key}=" "$env_target"; then
    echo "ERROR: ${required_key} missing in $env_target" >&2
    exit 1
  fi
done
if ! grep -Eq '^RELAY_POLICY_BLOCK_RETRY_LIMIT=1$' "$env_target"; then
  echo "ERROR: RELAY_POLICY_BLOCK_RETRY_LIMIT must be set to 1 in $env_target" >&2
  exit 1
fi
if ! grep -Eq '^RELAY_MAINTAINER_INTERRUPT_REASONS=true_blocker_decision,commit_message_approval$' "$env_target"; then
  echo "ERROR: RELAY_MAINTAINER_INTERRUPT_REASONS must be true_blocker_decision,commit_message_approval in $env_target" >&2
  exit 1
fi

if [[ "$SYSTEM_MODE" -eq 1 ]]; then
  echo "[setup-relay] Installing system services"
  "$REPO_ROOT/scripts/relay-systemd-install.sh" --system
  if [[ "$NO_START" -eq 0 ]]; then
    "$REPO_ROOT/scripts/relay-workers.sh" --system start
    "$REPO_ROOT/scripts/relay-workers.sh" --system health
  fi
else
  echo "[setup-relay] Installing user services"
  "$REPO_ROOT/scripts/relay-systemd-install.sh"
  if [[ "$NO_START" -eq 0 ]]; then
    "$REPO_ROOT/scripts/relay-workers.sh" start
    "$REPO_ROOT/scripts/relay-workers.sh" health
  fi
fi

echo "[setup-relay] Complete."
if [[ "$NO_START" -eq 0 ]]; then
  echo "[setup-relay] 1) workers: running"
else
  echo "[setup-relay] 1) workers: run scripts/relay-workers.sh start"
fi
echo "[setup-relay] 2) IDE: paste a task-customized relay prompt template"
echo "[setup-relay] 3) copy/paste when IDE gives run_id:"
echo "    scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2"
echo "[setup-relay] 4) if run output says PROMPT ARTIFACTS PENDING, copy/paste:"
echo "    scripts/relay-prompts.sh stage --run-id <run_id> --auto;scripts/relay-prompts.sh verify --run-id <run_id>"
echo "[setup-relay] 5) optional monitor (or omit --run to follow latest):"
echo "    scripts/relay-monitor.sh --run <run_id> --view quiet --sound"
