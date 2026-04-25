#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
MODE="user"
if [[ "${1:-}" == "--system" ]]; then
  MODE="system"
  shift
fi

ACTION="${1:-status}"
SERVICES=(
  ytree-relay-developer.service
  ytree-relay-code-auditor.service
  ytree-relay-watchdog.service
)

if [[ "$MODE" == "user" ]]; then
  SYSTEMCTL=(systemctl --user)
  export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
  export DBUS_SESSION_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS:-unix:path=${XDG_RUNTIME_DIR}/bus}"
  if ! "${SYSTEMCTL[@]}" show-environment >/dev/null 2>&1; then
    echo "ERROR: systemd user bus is unavailable." >&2
    echo "Hint: ensure WSL systemd is enabled and restart WSL, then retry." >&2
    exit 1
  fi
else
  SYSTEMCTL=(systemctl)
fi

case "$ACTION" in
  start)
    "${SYSTEMCTL[@]}" daemon-reload
    "${SYSTEMCTL[@]}" enable --now "${SERVICES[@]}"
    ;;
  stop)
    "${SYSTEMCTL[@]}" disable --now "${SERVICES[@]}" || true
    ;;
  restart)
    "${SYSTEMCTL[@]}" daemon-reload
    "${SYSTEMCTL[@]}" restart "${SERVICES[@]}"
    ;;
  status)
    "${SYSTEMCTL[@]}" --no-pager --full status "${SERVICES[@]}"
    ;;
  health)
    (
      cd "$REPO_ROOT"
      source .venv/bin/activate
      exec python3 scripts/relay_runtime.py health
    )
    ;;
  logs)
    journalctl $( [[ "$MODE" == "user" ]] && echo --user ) -u ytree-relay-developer.service -u ytree-relay-code-auditor.service -u ytree-relay-watchdog.service -n 200 --no-pager
    ;;
  *)
    echo "Usage: $0 [--system] {start|stop|restart|status|health|logs}" >&2
    exit 2
    ;;
esac
