#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
UNIT_SRC_DIR="$REPO_ROOT/infra/systemd"
MODE="user"

if [[ "${1:-}" == "--system" ]]; then
  MODE="system"
fi

if [[ "$MODE" == "user" ]]; then
  UNIT_DST_DIR="$HOME/.config/systemd/user"
  SYSTEMCTL=(systemctl --user)
  export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
  export DBUS_SESSION_BUS_ADDRESS="${DBUS_SESSION_BUS_ADDRESS:-unix:path=${XDG_RUNTIME_DIR}/bus}"
  if ! "${SYSTEMCTL[@]}" show-environment >/dev/null 2>&1; then
    echo "ERROR: systemd user bus is unavailable." >&2
    echo "Hint: ensure WSL systemd is enabled and restart WSL, then retry." >&2
    exit 1
  fi
else
  UNIT_DST_DIR="/etc/systemd/system"
  SYSTEMCTL=(systemctl)
fi

mkdir -p "$UNIT_DST_DIR"
install -m 0644 "$UNIT_SRC_DIR/ytree-relay-developer.service" "$UNIT_DST_DIR/ytree-relay-developer.service"
install -m 0644 "$UNIT_SRC_DIR/ytree-relay-code-auditor.service" "$UNIT_DST_DIR/ytree-relay-code-auditor.service"
install -m 0644 "$UNIT_SRC_DIR/ytree-relay-watchdog.service" "$UNIT_DST_DIR/ytree-relay-watchdog.service"

"${SYSTEMCTL[@]}" daemon-reload
"${SYSTEMCTL[@]}" enable ytree-relay-developer.service ytree-relay-code-auditor.service ytree-relay-watchdog.service

echo "Installed relay units to $UNIT_DST_DIR ($MODE mode)."
echo "Use scripts/relay-workers.sh start to launch workers."
