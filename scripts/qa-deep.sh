#!/usr/bin/env bash
set -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT="${1:-$DEFAULT_ROOT}"

LOG_ROOT="${QA_DEEP_LOG_ROOT:-${TMPDIR:-/tmp}/ytree-qa-deep}"
RETENTION_DAYS="${QA_DEEP_RETENTION_DAYS:-14}"
RUN_ID="$(date +%F_%H-%M-%S)"
RUN_DIR="$LOG_ROOT/$RUN_ID"
STEP_DIR="$RUN_DIR/steps"
PYTEST_DIR="$RUN_DIR/pytest"
CONSOLE_LOG="$RUN_DIR/console.log"
SUMMARY_MD="$RUN_DIR/summary.md"
SUMMARY_JSON="$RUN_DIR/summary.json"
FAILURES_MD="$RUN_DIR/failures.md"
HANDOFF_PROMPT="$RUN_DIR/ai_handoff_prompt.txt"
ENV_FILE="$RUN_DIR/env.txt"
STEPS_TSV="$RUN_DIR/steps.tsv"

mkdir -p "$STEP_DIR" "$PYTEST_DIR"
exec > >(tee -a "$CONSOLE_LOG") 2>&1

fmt_duration() {
  local s="$1"
  printf '%02dh:%02dm:%02ds' $((s / 3600)) $(((s % 3600) / 60)) $((s % 60))
}

capture_env() {
  {
    echo "timestamp=$(date -Is)"
    echo "root=$ROOT"
    echo "run_dir=$RUN_DIR"
    echo "log_root=$LOG_ROOT"
    if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
      echo "git_branch=$(git -C "$ROOT" rev-parse --abbrev-ref HEAD)"
      echo "git_commit=$(git -C "$ROOT" rev-parse HEAD)"
      echo "git_dirty=$( [ -n "$(git -C "$ROOT" status --porcelain)" ] && echo yes || echo no )"
    fi
    echo "uname=$(uname -a)"
    echo "bash=$(bash --version | head -n1)"
    command -v make >/dev/null 2>&1 && echo "make=$(make --version | head -n1)"
    command -v python3 >/dev/null 2>&1 && echo "python3=$(python3 --version 2>&1)"
    command -v valgrind >/dev/null 2>&1 && echo "valgrind=$(valgrind --version 2>&1)"
    command -v clang >/dev/null 2>&1 && echo "clang=$(clang --version | head -n1)"
    command -v lcov >/dev/null 2>&1 && echo "lcov=$(lcov --version | head -n1)"
  } >"$ENV_FILE"
}

cleanup_old_runs() {
  if ! [[ "$RETENTION_DAYS" =~ ^[0-9]+$ ]]; then
    return
  fi
  if [ ! -d "$LOG_ROOT" ]; then
    return
  fi
  find "$LOG_ROOT" -mindepth 1 -maxdepth 1 -type d -mtime "+$RETENTION_DAYS" \
    ! -path "$RUN_DIR" -exec rm -rf {} + 2>/dev/null || true
}

STEP_NAMES=()
STEP_RCS=()
STEP_SECS=()
STEP_WARNS=()
STEP_ERRS=()
STEP_LOGS=()

run_step() {
  local name="$1"
  shift
  local log_file="$STEP_DIR/${name}.log"
  local t0 t1 dt rc warns errs

  echo
  echo "============================================================"
  echo "STEP: $name"
  echo "START: $(date)"
  echo "============================================================"

  t0=$(date +%s)
  (
    cd "$ROOT" || exit 2
    if [ -f .venv/bin/activate ]; then
      # shellcheck disable=SC1091
      source .venv/bin/activate
    fi
    "$@"
  ) 2>&1 | tee "$log_file"
  rc=${PIPESTATUS[0]}

  t1=$(date +%s)
  dt=$((t1 - t0))

  warns=$(grep -Eic '\bwarning(s)?\b' "$log_file" || true)
  errs=$(grep -Eic '\berror(s)?\b|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error|ERROR SUMMARY' "$log_file" || true)

  echo "END: $(date)"
  echo "EXIT CODE: $rc"
  echo "DURATION: $(fmt_duration "$dt")"
  echo "WARNINGS (heuristic): $warns"
  echo "ERRORS   (heuristic): $errs"

  STEP_NAMES+=("$name")
  STEP_RCS+=("$rc")
  STEP_SECS+=("$dt")
  STEP_WARNS+=("$warns")
  STEP_ERRS+=("$errs")
  STEP_LOGS+=("$log_file")

  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$name" "$rc" "$dt" "$warns" "$errs" "$log_file" >>"$STEPS_TSV"

  return 0
}

collect_pytest_timings() {
  if ! command -v python3 >/dev/null 2>&1; then
    return
  fi

  python3 - "$PYTEST_DIR" "$RUN_DIR" <<'PY'
import glob
import json
import os
import sys
import xml.etree.ElementTree as ET

pytest_dir = sys.argv[1]
run_dir = sys.argv[2]
xml_paths = sorted(glob.glob(os.path.join(pytest_dir, "*.junit.xml")))

cases = []
failures = []
for path in xml_paths:
    try:
        root = ET.parse(path).getroot()
    except Exception:
        continue
    for tc in root.iter("testcase"):
        name = tc.attrib.get("name", "<unknown>")
        file_attr = tc.attrib.get("file")
        class_attr = tc.attrib.get("classname", "")
        nodeid = f"{file_attr}::{name}" if file_attr else f"{class_attr}::{name}"
        try:
            seconds = float(tc.attrib.get("time", "0") or 0)
        except ValueError:
            seconds = 0.0
        status = "passed"
        detail = ""
        fail_node = tc.find("failure")
        err_node = tc.find("error")
        skip_node = tc.find("skipped")
        if fail_node is not None:
            status = "failed"
            detail = (fail_node.attrib.get("message") or fail_node.text or "").strip().splitlines()[0:1]
        elif err_node is not None:
            status = "error"
            detail = (err_node.attrib.get("message") or err_node.text or "").strip().splitlines()[0:1]
        elif skip_node is not None:
            status = "skipped"
            detail = (skip_node.attrib.get("message") or skip_node.text or "").strip().splitlines()[0:1]

        cases.append((seconds, status, nodeid, os.path.basename(path)))
        if status in {"failed", "error"}:
            failures.append((status, nodeid, (detail[0] if detail else ""), os.path.basename(path)))

cases.sort(key=lambda x: x[0], reverse=True)

slow_path = os.path.join(run_dir, "pytest_slowest.txt")
with open(slow_path, "w", encoding="utf-8") as f:
    for seconds, status, nodeid, xml_name in cases[:200]:
        f.write(f"{seconds:9.3f}s\t{status:7s}\t{nodeid}\t[{xml_name}]\n")

fail_path = os.path.join(run_dir, "pytest_failures.txt")
with open(fail_path, "w", encoding="utf-8") as f:
    for status, nodeid, detail, xml_name in failures:
        line = f"{status.upper():7s}\t{nodeid}\t[{xml_name}]"
        if detail:
            line += f"\t{detail}"
        f.write(line + "\n")

summary = {
    "xml_files": xml_paths,
    "testcase_count": len(cases),
    "failure_count": sum(1 for c in cases if c[1] == "failed"),
    "error_count": sum(1 for c in cases if c[1] == "error"),
    "skipped_count": sum(1 for c in cases if c[1] == "skipped"),
}
with open(os.path.join(run_dir, "pytest_summary.json"), "w", encoding="utf-8") as f:
    json.dump(summary, f, indent=2)
PY
}

write_summary_markdown() {
  local overall_dt="$1"
  local total_warns="$2"
  local total_errs="$3"
  local failed_steps="$4"

  {
    echo "# qa-deep summary"
    echo
    echo "- **Run started:** $RUN_ID"
    echo "- **Repository root:** $ROOT"
    echo "- **Run directory:** $RUN_DIR"
    echo "- **Total duration:** $(fmt_duration "$overall_dt")"
    echo "- **Failed steps:** $failed_steps"
    echo "- **Total warnings (heuristic):** $total_warns"
    echo "- **Total errors (heuristic):** $total_errs"
    echo
    echo "## Step results"
    echo
    echo "| Step | RC | Duration | Warns | Errs | Log |"
    echo "|---|---:|---:|---:|---:|---|"

    local i
    for i in "${!STEP_NAMES[@]}"; do
      echo "| ${STEP_NAMES[$i]} | ${STEP_RCS[$i]} | $(fmt_duration "${STEP_SECS[$i]}") | ${STEP_WARNS[$i]} | ${STEP_ERRS[$i]} | ${STEP_LOGS[$i]} |"
    done

    echo
    echo "## Pytest timing artifacts"
    echo
    echo "- Slowest tests: $RUN_DIR/pytest_slowest.txt"
    echo "- Pytest failures: $RUN_DIR/pytest_failures.txt"
    echo "- Pytest summary JSON: $RUN_DIR/pytest_summary.json"
    echo
    echo "## Environment"
    echo
    echo "- $ENV_FILE"
  } >"$SUMMARY_MD"
}

write_summary_json() {
  if ! command -v python3 >/dev/null 2>&1; then
    return
  fi

  python3 - "$STEPS_TSV" "$RUN_DIR" "$SUMMARY_JSON" <<'PY'
import json
import os
import sys

steps_tsv, run_dir, out_path = sys.argv[1:4]
steps = []
if os.path.exists(steps_tsv):
    with open(steps_tsv, "r", encoding="utf-8") as f:
        for line in f:
            name, rc, secs, warns, errs, log = line.rstrip("\n").split("\t")
            steps.append({
                "name": name,
                "rc": int(rc),
                "seconds": int(secs),
                "warns_heuristic": int(warns),
                "errs_heuristic": int(errs),
                "log": log,
            })

pytest_summary_path = os.path.join(run_dir, "pytest_summary.json")
pytest_summary = {}
if os.path.exists(pytest_summary_path):
    with open(pytest_summary_path, "r", encoding="utf-8") as f:
        pytest_summary = json.load(f)

payload = {
    "run_dir": run_dir,
    "steps": steps,
    "failed_steps": [s["name"] for s in steps if s["rc"] != 0],
    "pytest": pytest_summary,
}
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(payload, f, indent=2)
PY
}

write_failures_markdown() {
  {
    echo "# qa-deep failures"
    echo
    echo "Run directory: $RUN_DIR"
    echo

    local i
    local had_failures=0
    for i in "${!STEP_NAMES[@]}"; do
      if [ "${STEP_RCS[$i]}" -ne 0 ]; then
        had_failures=1
        echo "## Step failure: ${STEP_NAMES[$i]} (rc=${STEP_RCS[$i]})"
        echo
        echo "Log: ${STEP_LOGS[$i]}"
        echo
        echo '```text'
        tail -n 200 "${STEP_LOGS[$i]}"
        echo '```'
        echo
      fi
    done

    if [ -s "$RUN_DIR/pytest_failures.txt" ]; then
      had_failures=1
      echo "## Pytest failed/error test cases"
      echo
      echo '```text'
      cat "$RUN_DIR/pytest_failures.txt"
      echo '```'
      echo
    fi

    if [ "$had_failures" -eq 0 ]; then
      echo "No failures detected."
    fi
  } >"$FAILURES_MD"
}

write_handoff_prompt() {
  {
    echo "Use this qa-deep run as source of truth for triage."
    echo
    echo "Primary files:"
    echo "- Summary: $SUMMARY_MD"
    echo "- Failures: $FAILURES_MD"
    echo "- Machine summary: $SUMMARY_JSON"
    echo "- Environment: $ENV_FILE"
    echo
    echo "Detailed logs:"
    echo "- Step logs: $STEP_DIR/*.log"
    echo "- Slowest tests: $RUN_DIR/pytest_slowest.txt"
    echo "- Pytest failures: $RUN_DIR/pytest_failures.txt"
    echo
    echo "Task: identify root causes and propose minimal, high-confidence fixes."
  } >"$HANDOFF_PROMPT"
}

if [ ! -d "$ROOT" ]; then
  echo "ERROR: root path does not exist: $ROOT"
  exit 2
fi

rm -f "$LOG_ROOT/latest"
ln -s "$RUN_DIR" "$LOG_ROOT/latest"
cleanup_old_runs
capture_env

echo "qa-deep started: $(date)"
echo "Repo: $ROOT"
echo "Run dir: $RUN_DIR"
echo "Latest link: $LOG_ROOT/latest"
echo "Retention days: $RETENTION_DAYS"

overall_t0=$(date +%s)

PYTEST_TIMING_OPTS="--durations=0 --durations-min=0"
run_step "qa-all" env "PYTEST_ADDOPTS=${PYTEST_TIMING_OPTS} --junitxml=$PYTEST_DIR/qa-all.junit.xml" make qa-all
run_step "qa-pytest-coverage" env "PYTEST_ADDOPTS=${PYTEST_TIMING_OPTS} --junitxml=$PYTEST_DIR/qa-pytest-coverage.junit.xml" make qa-pytest-coverage
run_step "qa-sanitize" env "PYTEST_ADDOPTS=${PYTEST_TIMING_OPTS} --junitxml=$PYTEST_DIR/qa-sanitize.junit.xml" make qa-sanitize
run_step "qa-valgrind-full" make qa-valgrind-full

overall_t1=$(date +%s)
overall_dt=$((overall_t1 - overall_t0))

total_warns=0
total_errs=0
failed_steps=0

for i in "${!STEP_NAMES[@]}"; do
  total_warns=$((total_warns + STEP_WARNS[i]))
  total_errs=$((total_errs + STEP_ERRS[i]))
  if [ "${STEP_RCS[$i]}" -ne 0 ]; then
    failed_steps=$((failed_steps + 1))
  fi
done

if [ -f "$ROOT/valgrind.log" ]; then
  cp "$ROOT/valgrind.log" "$RUN_DIR/valgrind.log" || true
fi
if [ -f "$ROOT/coverage/summary.txt" ]; then
  cp "$ROOT/coverage/summary.txt" "$RUN_DIR/coverage-summary.txt" || true
fi

collect_pytest_timings
write_summary_markdown "$overall_dt" "$total_warns" "$total_errs" "$failed_steps"
write_summary_json
write_failures_markdown
write_handoff_prompt

{
  echo
  echo "==================== FINAL SUMMARY ===================="
  printf '%-24s %-4s %-10s %-7s %-7s %s\n' "STEP" "RC" "TIME" "WARNS" "ERRS" "LOG"
  for i in "${!STEP_NAMES[@]}"; do
    printf '%-24s %-4s %-10s %-7s %-7s %s\n' \
      "${STEP_NAMES[$i]}" "${STEP_RCS[$i]}" "$(fmt_duration "${STEP_SECS[$i]}")" \
      "${STEP_WARNS[$i]}" "${STEP_ERRS[$i]}" "${STEP_LOGS[$i]}"
  done
  echo "-------------------------------------------------------"
  echo "TOTAL TIME: $(fmt_duration "$overall_dt")"
  echo "TOTAL WARNINGS (heuristic): $total_warns"
  echo "TOTAL ERRORS   (heuristic): $total_errs"
  echo "FAILED STEPS: $failed_steps"
  echo "RUN DIR: $RUN_DIR"
  echo "SUMMARY: $SUMMARY_MD"
  echo "FAILURES: $FAILURES_MD"
  echo "HANDOFF: $HANDOFF_PROMPT"
}

if [ "$failed_steps" -eq 0 ]; then
  exit 0
fi
exit 1
