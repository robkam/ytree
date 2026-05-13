#!/usr/bin/env bash
# Start or resume a durable relay run with env preflight loaded automatically.
#
# Usage examples:
#   scripts/relay-run.sh --run-id my-run --idempotency-key my-run
#   scripts/relay-run.sh --run-id my-run --idempotency-key my-run --activity-timeout 900
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

usage() {
  cat <<'USAGE'
Usage: scripts/relay-run.sh --run-id <id> --idempotency-key <key> [options]

Options (pass-through to relay_runtime.py start-run):
  --retry-limit <n>
  --retry-backoff <seconds>
  --activity-timeout <seconds>
  --lease-ttl <seconds>
  --heartbeat-late <seconds>
  --heartbeat-stale <seconds>
USAGE
}

if [[ $# -eq 0 ]]; then
  usage
  exit 2
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

if [[ -f "$HOME/.config/ytree/relay.env" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "$HOME/.config/ytree/relay.env"
  set +a
fi

cd "$REPO_ROOT"

output="$(python3 scripts/relay_runtime.py start-run "$@")"
printf '%s\n' "$output"

run_id="$(sed -n 's/^run_id=\([^ ]*\) state=.*/\1/p' <<<"$output" | tail -n1)"
state="$(sed -n 's/^run_id=[^ ]* state=\(.*\)$/\1/p' <<<"$output" | tail -n1)"

if [[ -n "$run_id" ]]; then
  if [[ "$state" == "started" ]]; then
    echo "RUN STARTED: $run_id"
  else
    echo "RUN RESUMED: $run_id"
  fi

  prompt_dir="${RELAY_PROMPT_DIR:-$HOME/.config/ytree/relay-prompts}"
  developer_prompt="$prompt_dir/${run_id}_developer_run_developer.txt"
  auditor_prompt="$prompt_dir/${run_id}_auditor_run_code_auditor.txt"
  auto_src_dir="$HOME/.local/state/ytree/prompt-sources/$run_id"
  prompt_stage_cmd="scripts/relay-prompts.sh stage --run-id $run_id --auto;scripts/relay-prompts.sh verify --run-id $run_id"
  if [[ -f "$developer_prompt" && -f "$auditor_prompt" ]]; then
    echo "PROMPT ARTIFACTS READY: $run_id"
  else
    auto_developer_src=""
    auto_auditor_src=""
    if [[ -d "$auto_src_dir" ]]; then
      auto_developer_src="$(ls -1t "$auto_src_dir"/developer*.txt 2>/dev/null | head -n1 || true)"
      auto_auditor_src="$(ls -1t "$auto_src_dir"/auditor*.txt 2>/dev/null | head -n1 || true)"
    fi
    if [[ -n "$auto_developer_src" && -n "$auto_auditor_src" ]]; then
      auto_stage_output=""
      auto_verify_output=""
      if auto_stage_output="$(scripts/relay-prompts.sh stage --run-id "$run_id" --auto 2>&1)" \
        && auto_verify_output="$(scripts/relay-prompts.sh verify --run-id "$run_id" 2>&1)"; then
        printf '%s\n' "$auto_stage_output"
        printf '%s\n' "$auto_verify_output"
        echo "PROMPT ARTIFACTS AUTO-STAGED: $run_id"
      else
        echo "PROMPT ARTIFACTS PENDING: $run_id"
        if [[ -n "$auto_stage_output" ]]; then
          printf '%s\n' "$auto_stage_output"
        fi
        if [[ -n "$auto_verify_output" ]]; then
          printf '%s\n' "$auto_verify_output"
        fi
        echo "NEXT: $prompt_stage_cmd"
      fi
    else
      echo "PROMPT ARTIFACTS PENDING: $run_id"
      if [[ -d "$auto_src_dir" ]]; then
        echo "INFO: auto prompt sources incomplete in $auto_src_dir (need developer*.txt and auditor*.txt)"
      else
        echo "INFO: auto prompt sources not emitted yet: $auto_src_dir"
      fi
      echo "NEXT: $prompt_stage_cmd"
    fi
  fi
fi
