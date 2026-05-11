#!/usr/bin/env bash
# Stage/verify relay prompt artifacts for a run_id.
#
# Usage:
#   scripts/relay-prompts.sh stage --run-id <id> --developer <path> --auditor <path>
#   scripts/relay-prompts.sh verify --run-id <id>
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

MODE=""
RUN_ID=""
DEVELOPER_SRC=""
AUDITOR_SRC=""

usage() {
  cat <<'USAGE'
Usage:
  scripts/relay-prompts.sh stage --run-id <id> --developer <path> --auditor <path>
  scripts/relay-prompts.sh verify --run-id <id>
USAGE
}

if [[ $# -lt 1 ]]; then
  usage
  exit 2
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

MODE="${1:-}"
shift

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run-id)
      RUN_ID="${2:-}"
      shift 2
      ;;
    --developer)
      DEVELOPER_SRC="${2:-}"
      shift 2
      ;;
    --auditor)
      AUDITOR_SRC="${2:-}"
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

if [[ -f "$HOME/.config/ytree/relay.env" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "$HOME/.config/ytree/relay.env"
  set +a
fi

if [[ -z "$RUN_ID" ]]; then
  echo "ERROR: --run-id is required" >&2
  exit 2
fi

PROMPT_DIR="${RELAY_PROMPT_DIR:-$HOME/.config/ytree/relay-prompts}"
mkdir -p "$PROMPT_DIR"
DEVELOPER_DST="$PROMPT_DIR/${RUN_ID}_developer_run_developer.txt"
AUDITOR_DST="$PROMPT_DIR/${RUN_ID}_auditor_run_code_auditor.txt"

cd "$REPO_ROOT"

case "$MODE" in
  stage)
    if [[ -z "$DEVELOPER_SRC" || -z "$AUDITOR_SRC" ]]; then
      echo "ERROR: stage requires --developer and --auditor" >&2
      exit 2
    fi
    if [[ ! -f "$DEVELOPER_SRC" ]]; then
      echo "ERROR: developer prompt source not found: $DEVELOPER_SRC" >&2
      exit 1
    fi
    if [[ ! -f "$AUDITOR_SRC" ]]; then
      echo "ERROR: auditor prompt source not found: $AUDITOR_SRC" >&2
      exit 1
    fi
    cp "$DEVELOPER_SRC" "$DEVELOPER_DST"
    cp "$AUDITOR_SRC" "$AUDITOR_DST"
    echo "PROMPT STAGED: $DEVELOPER_DST"
    echo "PROMPT STAGED: $AUDITOR_DST"
    echo "PROMPT READY: $RUN_ID"
    ;;
  verify)
    missing=0
    if [[ -f "$DEVELOPER_DST" ]]; then
      echo "PROMPT FOUND: $DEVELOPER_DST"
    else
      echo "PROMPT MISSING: $DEVELOPER_DST"
      missing=1
    fi
    if [[ -f "$AUDITOR_DST" ]]; then
      echo "PROMPT FOUND: $AUDITOR_DST"
    else
      echo "PROMPT MISSING: $AUDITOR_DST"
      missing=1
    fi
    if [[ "$missing" -eq 0 ]]; then
      echo "PROMPT READY: $RUN_ID"
    else
      exit 1
    fi
    ;;
  *)
    echo "ERROR: mode must be stage or verify" >&2
    usage >&2
    exit 2
    ;;
esac
