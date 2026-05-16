#!/usr/bin/env bash
set -u -o pipefail

ROOT="${1:-$HOME/ytree}"
LOG_DIR="${2:-$ROOT/logs}"
STAMP="$(date +%F_%H-%M-%S)"
LOG_FILE="$LOG_DIR/overnight_qa_timed_$STAMP.txt"

mkdir -p "$LOG_DIR"
exec > >(tee -a "$LOG_FILE") 2>&1

fmt_duration() {
  local s="$1"
  printf '%02dh:%02dm:%02ds' $((s/3600)) $(((s%3600)/60)) $((s%60))
}

step_names=()
step_secs=()
step_rcs=()
step_warns=()
step_errs=()

run_step() {
  local name="$1"
  shift
  local tmp
  tmp="$(mktemp)"
  local t0 t1 dt rc warns errs

  echo
  echo "============================================================"
  echo "STEP: $name"
  echo "START: $(date)"
  echo "============================================================"

  t0=$(date +%s)

  (
    cd "$ROOT" || exit 2
    source .venv/bin/activate
    make clean
    "$@"
  ) 2>&1 | tee "$tmp"
  rc=${PIPESTATUS[0]}

  t1=$(date +%s)
  dt=$((t1 - t0))

  # Heuristic counts (for triage; manual review still needed)
  warns=$(grep -Eic '\bwarning(s)?\b' "$tmp" || true)
  errs=$(grep -Eic '\berror(s)?\b|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error|ERROR SUMMARY' "$tmp" || true)

  echo "END: $(date)"
  echo "EXIT CODE: $rc"
  echo "DURATION: $(fmt_duration "$dt")"
  echo "WARNINGS (heuristic): $warns"
  echo "ERRORS   (heuristic): $errs"

  step_names+=("$name")
  step_secs+=("$dt")
  step_rcs+=("$rc")
  step_warns+=("$warns")
  step_errs+=("$errs")

  rm -f "$tmp"
  return 0
}

overall_start=$(date +%s)
echo "Overnight timed QA started: $(date)"
echo "Repo: $ROOT"
echo "Log:  $LOG_FILE"

run_step "qa-sanitize"        make qa-sanitize
run_step "qa-valgrind-full"   make qa-valgrind-full
run_step "qa-pytest-coverage" make qa-pytest-coverage

overall_end=$(date +%s)
overall_dt=$((overall_end - overall_start))

echo
echo "==================== FINAL SUMMARY ===================="
fail=0
total_warns=0
total_errs=0

for i in "${!step_names[@]}"; do
  n="${step_names[$i]}"
  s="${step_secs[$i]}"
  r="${step_rcs[$i]}"
  w="${step_warns[$i]}"
  e="${step_errs[$i]}"
  total_warns=$((total_warns + w))
  total_errs=$((total_errs + e))
  [[ "$r" -ne 0 ]] && fail=1

  printf '%-20s rc=%-3s time=%-10s warns=%-5s errs=%-5s\n' \
    "$n" "$r" "$(fmt_duration "$s")" "$w" "$e"
done

echo "-------------------------------------------------------"
echo "TOTAL TIME: $(fmt_duration "$overall_dt")"
echo "TOTAL WARNINGS (heuristic): $total_warns"
echo "TOTAL ERRORS   (heuristic): $total_errs"

if [[ $fail -eq 0 ]]; then
  echo "RESULT: PASS (all exit codes 0)"
  exit 0
else
  echo "RESULT: FAIL (one or more steps returned non-zero)"
  exit 1
fi