#!/usr/bin/env bash
# Single-terminal relay monitor with selectable detail level.
#
# Views:
#   quiet   -> latest state + action-needed line only
#   normal  -> key transitions (filters heartbeat noise)
#   verbose -> full event stream including heartbeats
#
# Usage:
#   scripts/relay-monitor.sh --view quiet
#   scripts/relay-monitor.sh --run <run_id> --view normal
#   scripts/relay-monitor.sh --run <run_id> --view verbose --once
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
VIEW="normal"
RUN_ID=""
INTERVAL=3
LIMIT=20
FOLLOW=1

usage() {
  cat <<'USAGE'
Usage: scripts/relay-monitor.sh [--run <run_id>] [--view quiet|normal|verbose] [--interval <seconds>] [--limit <n>] [--once]

Defaults:
  --run       latest run in relay DB
  --view      normal
  --interval  3
  --limit     20
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run)
      RUN_ID="${2:-}"
      shift 2
      ;;
    --view)
      VIEW="${2:-}"
      shift 2
      ;;
    --interval)
      INTERVAL="${2:-}"
      shift 2
      ;;
    --limit)
      LIMIT="${2:-}"
      shift 2
      ;;
    --once)
      FOLLOW=0
      shift
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
done

case "$VIEW" in
  quiet|normal|verbose) ;;
  *)
    echo "ERROR: --view must be one of quiet|normal|verbose" >&2
    exit 2
    ;;
esac

if [[ -f "$HOME/.config/ytree/relay.env" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "$HOME/.config/ytree/relay.env"
  set +a
fi

cd "$REPO_ROOT"

if [[ -z "$RUN_ID" ]]; then
  RUN_ID="$(python3 - <<'PY'
import os, sqlite3
path = os.path.expanduser(os.environ.get('RELAY_DB', '~/.local/state/ytree/relay.db'))
conn = sqlite3.connect(path)
row = conn.execute('SELECT run_id FROM runs ORDER BY updated_at DESC LIMIT 1').fetchone()
print(row[0] if row else '')
PY
)"
fi

if [[ -z "$RUN_ID" ]]; then
  echo "No relay runs found." >&2
  exit 1
fi

render_once() {
  local history
  if ! history="$(python3 scripts/relay_runtime.py history --run-id "$RUN_ID" 2>/dev/null)"; then
    echo "ERROR: could not read history for run_id=$RUN_ID" >&2
    return 1
  fi

  printf '%s\n' "$history" | python3 -c "$(cat <<'PY'
import json
import sys

run_id, view, limit_s = sys.argv[1:4]
limit = int(limit_s)
lines = [ln.strip() for ln in sys.stdin if ln.strip()]
rows = []
for ln in lines:
    try:
        rows.append(json.loads(ln))
    except json.JSONDecodeError:
        pass

print(f"RUN: {run_id}")
if not rows:
    print("STATE: no events yet")
    print("ACTION NEEDED (maintainer): none")
    sys.exit(0)

last = rows[-1]
seq = last.get('history_seq', '-')
event = last.get('event_type', '-')
status = last.get('status', '-')
unit = last.get('unit_id', '-')
ts = last.get('ts_utc_iso', '-')
print(f"LATEST: seq={seq} event={event} status={status} unit={unit} at={ts}")

action = "none"
if last.get('event_type') not in ('workflow_completed', 'workflow_failed'):
    for row in reversed(rows):
        msg = str(row.get('message', ''))
        msg_l = msg.lower()
        event_type = str(row.get('event_type', ''))
        if event_type == 'maintainer_update_required':
            action = msg or "maintainer update required"
            break
        if 'maintainer' in msg_l and any(tok in msg_l for tok in ('reply', 'approve', 'confirm', 'decision', 'required')):
            action = msg
            break
print(f"ACTION NEEDED (maintainer): {action}")

if view == 'quiet':
    sys.exit(0)

if view == 'normal':
    interesting = [r for r in rows if r.get('event_type') != 'heartbeat']
else:
    interesting = rows

print("")
print("RECENT EVENTS:")
for row in interesting[-limit:]:
    print(
        f"- seq={row.get('history_seq', '-')} "
        f"event={row.get('event_type', '-')} "
        f"role={row.get('role', '-')} "
        f"unit={row.get('unit_id', '-')} "
        f"status={row.get('status', '-')} "
        f"msg={row.get('message', '')}"
    )
PY
)" "$RUN_ID" "$VIEW" "$LIMIT"
}

if [[ "$FOLLOW" -eq 0 ]]; then
  render_once
  exit $?
fi

while true; do
  clear
  render_once || true
  sleep "$INTERVAL"
done
