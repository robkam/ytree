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
AUTO_SELECT_RUN=0
SOUND=0
DEFAULT_SOUND_DIR="$REPO_ROOT/scripts/assets/sounds"
SOUND_INPUT="${RELAY_MONITOR_SOUND_INPUT:-$DEFAULT_SOUND_DIR/hey.mp3}"
SOUND_FAIL="${RELAY_MONITOR_SOUND_FAIL:-$DEFAULT_SOUND_DIR/gen.mp3}"
SOUND_SUCCESS="${RELAY_MONITOR_SOUND_SUCCESS:-$DEFAULT_SOUND_DIR/all.mp3}"
LAST_NOTIFIED_SEQ=""
LAST_NOTIFIED_ACTION=""
SOUND_PLAYER=""

usage() {
  cat <<'USAGE'
Usage: scripts/relay-monitor.sh [--run <run_id>] [--view quiet|normal|verbose] [--interval <seconds>] [--limit <n>] [--once]

Sound options:
  --sound                         enable sound notifications
  --sound-input <file.mp3>        sound when maintainer input is needed
  --sound-fail <file.mp3>         sound when workflow fails/errors
  --sound-success <file.mp3>      sound when workflow completes

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
    --sound)
      SOUND=1
      shift
      ;;
    --sound-input)
      SOUND=1
      SOUND_INPUT="${2:-}"
      shift 2
      ;;
    --sound-fail)
      SOUND=1
      SOUND_FAIL="${2:-}"
      shift 2
      ;;
    --sound-success)
      SOUND=1
      SOUND_SUCCESS="${2:-}"
      shift 2
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

if [[ "$SOUND" -eq 0 && (-n "${SOUND_INPUT:-}" || -n "${SOUND_FAIL:-}" || -n "${SOUND_SUCCESS:-}") ]]; then
  SOUND=1
fi

cd "$REPO_ROOT"

resolve_sound_path() {
  local sound_file="$1"
  if [[ -z "$sound_file" ]]; then
    printf '%s' ""
    return 0
  fi
  if [[ -f "$sound_file" ]]; then
    printf '%s' "$sound_file"
    return 0
  fi
  if [[ "$sound_file" != */* && -f "$DEFAULT_SOUND_DIR/$sound_file" ]]; then
    printf '%s' "$DEFAULT_SOUND_DIR/$sound_file"
    return 0
  fi
  printf '%s' "$sound_file"
}

SOUND_INPUT="$(resolve_sound_path "$SOUND_INPUT")"
SOUND_FAIL="$(resolve_sound_path "$SOUND_FAIL")"
SOUND_SUCCESS="$(resolve_sound_path "$SOUND_SUCCESS")"

if [[ "$SOUND" -eq 1 ]]; then
  if command -v ffplay >/dev/null 2>&1; then
    SOUND_PLAYER="ffplay"
  elif command -v mpv >/dev/null 2>&1; then
    SOUND_PLAYER="mpv"
  elif command -v mpg123 >/dev/null 2>&1; then
    SOUND_PLAYER="mpg123"
  elif command -v play >/dev/null 2>&1; then
    SOUND_PLAYER="play"
  else
    echo "WARN: no audio player found for --sound (ffplay/mpv/mpg123/play). Falling back to terminal bell." >&2
  fi
fi

if [[ -z "$RUN_ID" ]]; then
  AUTO_SELECT_RUN=1
fi

resolve_run_id() {
  python3 - <<'PY'
import os
import sqlite3

path = os.path.expanduser(os.environ.get("RELAY_DB", "~/.local/state/ytree/relay.db"))
conn = sqlite3.connect(path)
row = conn.execute(
    "SELECT run_id FROM runs WHERE status = 'running' ORDER BY updated_at DESC LIMIT 1"
).fetchone()
if row:
    print(row[0])
else:
    fallback = conn.execute("SELECT run_id FROM runs ORDER BY updated_at DESC LIMIT 1").fetchone()
    print(fallback[0] if fallback else "")
PY
}

if [[ -z "$RUN_ID" ]]; then
  RUN_ID="$(resolve_run_id)"
fi

if [[ -z "$RUN_ID" ]]; then
  echo "No relay runs found." >&2
  exit 1
fi

if [[ "$AUTO_SELECT_RUN" -eq 1 ]]; then
  echo "Auto-selecting run_id=$RUN_ID" >&2
fi

refresh_auto_selected_run() {
  if [[ "$AUTO_SELECT_RUN" -ne 1 ]]; then
    return
  fi
  local selected
  selected="$(resolve_run_id)"
  if [[ -n "$selected" ]]; then
    RUN_ID="$selected"
  fi
}

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

def find_report_handle(unit_id: str) -> str:
    for row in reversed(rows):
        if row.get("event_type") != "unit_completed":
            continue
        if row.get("unit_id") != unit_id:
            continue
        msg = str(row.get("message", ""))
        marker = "report_handle="
        idx = msg.find(marker)
        if idx < 0:
            continue
        remainder = msg[idx + len(marker):]
        return remainder.split(";", 1)[0].strip()
    return "-"

action = "none"
if event == "workflow_completed":
    action = f'if IDE is silent, reply "provide final delivery package now for run_id {run_id} from history_seq {seq}"'
elif event == "workflow_failed":
    action = f'if IDE is silent, reply "explain workflow failure for run_id {run_id} from history_seq {seq}"'
else:
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

if event in ("workflow_completed", "workflow_failed"):
    dev_report = find_report_handle("developer_run")
    auditor_report = find_report_handle("auditor_run")
    print(f"REPORTS: developer={dev_report} auditor={auditor_report}")

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

play_sound_file() {
  local sound_file="$1"
  if [[ -z "$SOUND_PLAYER" ]]; then
    printf '\a'
    return 0
  fi
  if [[ -z "$sound_file" || ! -f "$sound_file" ]]; then
    printf '\a'
    return 0
  fi
  if [[ "$SOUND_PLAYER" == "ffplay" ]]; then
    (ffplay -nodisp -autoexit -loglevel quiet "$sound_file" >/dev/null 2>&1 &)
  elif [[ "$SOUND_PLAYER" == "mpv" ]]; then
    (mpv --really-quiet --no-video "$sound_file" >/dev/null 2>&1 &)
  elif [[ "$SOUND_PLAYER" == "mpg123" ]]; then
    (mpg123 -q "$sound_file" >/dev/null 2>&1 &)
  elif [[ "$SOUND_PLAYER" == "play" ]]; then
    (play -q "$sound_file" >/dev/null 2>&1 &)
  else
    printf '\a'
  fi
}

notify_from_rendered() {
  local rendered="$1"
  [[ "$SOUND" -eq 1 ]] || return 0

  local latest_line action_line seq event action
  latest_line="$(printf '%s\n' "$rendered" | sed -n 's/^LATEST: /LATEST: /p' | head -n1)"
  action_line="$(printf '%s\n' "$rendered" | sed -n 's/^ACTION NEEDED (maintainer): /ACTION NEEDED (maintainer): /p' | head -n1)"
  seq="$(printf '%s\n' "$latest_line" | sed -n 's/^LATEST: seq=\([^ ]*\) .*/\1/p')"
  event="$(printf '%s\n' "$latest_line" | sed -n 's/^LATEST: seq=[^ ]* event=\([^ ]*\) .*/\1/p')"
  action="${action_line#ACTION NEEDED (maintainer): }"

  if [[ -n "$seq" && "$seq" != "$LAST_NOTIFIED_SEQ" ]]; then
    case "$event" in
      workflow_completed)
        play_sound_file "$SOUND_SUCCESS"
        LAST_NOTIFIED_SEQ="$seq"
        LAST_NOTIFIED_ACTION="$action"
        return 0
        ;;
      workflow_failed|unit_failed|worker_command_failed)
        play_sound_file "$SOUND_FAIL"
        LAST_NOTIFIED_SEQ="$seq"
        LAST_NOTIFIED_ACTION="$action"
        return 0
        ;;
    esac
  fi

  if [[ "${action:-none}" != "none" && "${action:-}" != "$LAST_NOTIFIED_ACTION" ]]; then
    play_sound_file "$SOUND_INPUT"
  fi

  if [[ -n "$seq" ]]; then
    LAST_NOTIFIED_SEQ="$seq"
  fi
  LAST_NOTIFIED_ACTION="${action:-none}"
}

if [[ "$FOLLOW" -eq 0 ]]; then
  refresh_auto_selected_run
  rendered_once="$(render_once)"
  printf '%s\n' "$rendered_once"
  notify_from_rendered "$rendered_once"
  exit $?
fi

while true; do
  refresh_auto_selected_run
  clear
  rendered_loop="$(render_once || true)"
  printf '%s\n' "$rendered_loop"
  notify_from_rendered "$rendered_loop"
  sleep "$INTERVAL"
done
