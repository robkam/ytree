#!/usr/bin/env python3
"""Watch branch GitHub Actions and spawn fresh Codex repair attempts on red."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import time
from typing import Any


REPO_ROOT = Path(__file__).resolve().parent.parent
HANDOFF_DIR = REPO_ROOT / ".agent" / "handoffs"
FAILURE_PACKET_PATH = HANDOFF_DIR / "ci-failure.current.md"
PROMPT_PATH = HANDOFF_DIR / "ci-repair.prompt.current.txt"
RESPONSE_PATH = HANDOFF_DIR / "ci-repair.response.current.txt"
STATE_PATH = HANDOFF_DIR / "ci-repair.current.md"
LOG_PATH = HANDOFF_DIR / "ci-repair.current.log"
LAUNCH_INFO_PATH = HANDOFF_DIR / "ci-repair.launch.current.json"
DEFAULT_POLL_SECONDS = 300
DEFAULT_MAX_ATTEMPTS = 3
RUN_FIELDS = (
    "databaseId,workflowName,headSha,status,conclusion,event,displayTitle,createdAt,updatedAt,url"
)
RUN_DETAIL_FIELDS = (
    "databaseId,workflowName,headSha,status,conclusion,url,jobs,displayTitle,event,createdAt,updatedAt"
)
SUCCESS_CONCLUSIONS = {"success", "neutral", "skipped"}
FAILURE_CONCLUSIONS = {
    "failure",
    "cancelled",
    "timed_out",
    "action_required",
    "startup_failure",
    "stale",
}


def _now() -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S")


def _run(
    args: list[str],
    *,
    cwd: Path = REPO_ROOT,
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd,
        input=input_text,
        text=True,
        capture_output=True,
        check=False,
    )


def _require_ok(
    args: list[str],
    *,
    cwd: Path = REPO_ROOT,
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    run = _run(args, cwd=cwd, input_text=input_text)
    if run.returncode != 0:
        detail = (run.stderr or run.stdout or "").strip()
        raise RuntimeError(f"{' '.join(args)} failed: {detail}")
    return run


def _git(args: list[str], *, repo_root: Path) -> str:
    return _require_ok(["git", *args], cwd=repo_root).stdout.strip()


def _gh_json(args: list[str], *, repo: str, repo_root: Path) -> Any:
    run = _require_ok(["gh", *args, "--repo", repo], cwd=repo_root)
    return json.loads(run.stdout)


def _gh_text(args: list[str], *, repo: str, repo_root: Path) -> str:
    return _require_ok(["gh", *args, "--repo", repo], cwd=repo_root).stdout


def _parse_repo_slug(remote_url: str) -> str:
    remote = remote_url.strip()
    ssh_match = re.search(r"[:/]([^/:]+/[^/]+?)(?:\.git)?$", remote)
    if ssh_match:
        return ssh_match.group(1)
    raise RuntimeError(f"unsupported origin remote URL: {remote_url}")


def _discover_handoff(explicit: str | None) -> Path | None:
    if explicit:
        path = Path(explicit)
        if not path.is_absolute():
            path = REPO_ROOT / path
        if not path.exists():
            raise RuntimeError(f"handoff file not found: {path}")
        return path

    candidates = sorted(
        path
        for path in HANDOFF_DIR.glob("*.current.md")
        if path.name not in {FAILURE_PACKET_PATH.name, STATE_PATH.name}
    )
    if not candidates:
        return None
    if len(candidates) > 1:
        names = ", ".join(path.name for path in candidates)
        raise RuntimeError(
            "multiple current handoffs found; pass --handoff explicitly "
            f"({names})"
        )
    return candidates[0]


def _load_runs(repo: str, *, repo_root: Path, branch: str, sha: str) -> list[dict[str, Any]]:
    payload = _gh_json(
        [
            "run",
            "list",
            "--branch",
            branch,
            "--commit",
            sha,
            "--limit",
            "50",
            "--json",
            RUN_FIELDS,
        ],
        repo=repo,
        repo_root=repo_root,
    )
    if not isinstance(payload, list):
        raise RuntimeError("unexpected gh run list payload")
    return payload


def _classify_runs(runs: list[dict[str, Any]]) -> str:
    if not runs:
        return "missing"
    for run in runs:
        status = str(run.get("status", "") or "")
        conclusion = str(run.get("conclusion", "") or "")
        if status != "completed" or not conclusion:
            return "pending"
    for run in runs:
        conclusion = str(run.get("conclusion", "") or "")
        if conclusion in FAILURE_CONCLUSIONS:
            return "failure"
        if conclusion not in SUCCESS_CONCLUSIONS:
            return "failure"
    return "success"


def _failed_runs(runs: list[dict[str, Any]]) -> list[dict[str, Any]]:
    failed = []
    for run in runs:
        conclusion = str(run.get("conclusion", "") or "")
        if conclusion in FAILURE_CONCLUSIONS or (
            conclusion and conclusion not in SUCCESS_CONCLUSIONS
        ):
            failed.append(run)
    return failed


def _failed_signature(sha: str, failed_runs: list[dict[str, Any]]) -> tuple[str, tuple[int, ...]]:
    return sha, tuple(sorted(int(run["databaseId"]) for run in failed_runs))


def _summarize_pending(runs: list[dict[str, Any]]) -> str:
    pieces = []
    for run in sorted(runs, key=lambda item: (item.get("workflowName", ""), item.get("databaseId", 0))):
        status = str(run.get("status", "") or "")
        conclusion = str(run.get("conclusion", "") or "")
        if status != "completed" or not conclusion:
            pieces.append(f"{run.get('workflowName', 'workflow')}={status}")
    return ", ".join(pieces) if pieces else "waiting"


def _trim_log(text: str, *, max_lines: int = 220, max_chars: int = 30000) -> str:
    text = text.strip()
    if not text:
        return "(no failed-step log output returned by gh)"
    lines = text.splitlines()
    if len(lines) > max_lines:
        lines = lines[-max_lines:]
        text = "[... truncated to last failed log lines ...]\n" + "\n".join(lines)
    else:
        text = "\n".join(lines)
    if len(text) > max_chars:
        text = "[... truncated log excerpt ...]\n" + text[-max_chars:]
    return text


def _load_run_detail(repo: str, *, repo_root: Path, run_id: int) -> dict[str, Any]:
    payload = _gh_json(
        ["run", "view", str(run_id), "--json", RUN_DETAIL_FIELDS],
        repo=repo,
        repo_root=repo_root,
    )
    if not isinstance(payload, dict):
        raise RuntimeError(f"unexpected gh run view payload for run {run_id}")
    return payload


def _load_failed_log(repo: str, *, repo_root: Path, run_id: int) -> str:
    return _gh_text(
        ["run", "view", str(run_id), "--log-failed"],
        repo=repo,
        repo_root=repo_root,
    )


def _failure_jobs(detail: dict[str, Any]) -> list[dict[str, Any]]:
    jobs = detail.get("jobs", [])
    if not isinstance(jobs, list):
        return []
    failed_jobs: list[dict[str, Any]] = []
    for job in jobs:
        if not isinstance(job, dict):
            continue
        conclusion = str(job.get("conclusion", "") or "")
        if conclusion not in FAILURE_CONCLUSIONS and conclusion in SUCCESS_CONCLUSIONS:
            continue
        steps = job.get("steps", [])
        failed_steps = []
        if isinstance(steps, list):
            for step in steps:
                if not isinstance(step, dict):
                    continue
                step_conclusion = str(step.get("conclusion", "") or "")
                if step_conclusion in FAILURE_CONCLUSIONS or (
                    step_conclusion and step_conclusion not in SUCCESS_CONCLUSIONS
                ):
                    failed_steps.append(str(step.get("name", "unnamed step")))
        failed_jobs.append(
            {
                "name": str(job.get("name", "unnamed job")),
                "url": str(job.get("url", "")),
                "failed_steps": failed_steps,
            }
        )
    return failed_jobs


def _build_failure_packet(
    *,
    repo: str,
    branch: str,
    sha: str,
    attempt: int,
    handoff_path: Path | None,
    failed_runs: list[dict[str, Any]],
    run_details: list[dict[str, Any]],
    logs: list[str],
) -> str:
    lines = [
        "# GitHub CI failure packet",
        "",
        "This file is the authoritative current failure context for the next fresh repair pass.",
        "",
        f"- Generated: {_now()}",
        f"- Repository: {repo}",
        f"- Branch: {branch}",
        f"- Head SHA: {sha}",
        f"- Repair attempt: {attempt}",
        f"- Background handoff: {handoff_path if handoff_path else '(none)'}",
        "",
        "## Failed workflow runs",
        "",
        "| Workflow | Event | Conclusion | Run |",
        "| --- | --- | --- | --- |",
    ]
    for run in failed_runs:
        lines.append(
            f"| {run.get('workflowName', 'workflow')} | {run.get('event', '')} | "
            f"{run.get('conclusion', '')} | {run.get('url', '')} |"
        )

    for detail, log_text in zip(run_details, logs):
        workflow_name = str(detail.get("workflowName", "workflow"))
        lines.extend(["", f"## {workflow_name}", ""])
        failed_jobs = _failure_jobs(detail)
        if not failed_jobs:
            lines.append("- No failed jobs were returned by `gh run view`.")
        else:
            for job in failed_jobs:
                lines.append(f"### Job: {job['name']}")
                if job["failed_steps"]:
                    lines.append(f"- Failed step(s): {', '.join(job['failed_steps'])}")
                if job["url"]:
                    lines.append(f"- Job URL: {job['url']}")
                lines.append("")
        lines.extend(["```text", _trim_log(log_text), "```"])
    lines.append("")
    return "\n".join(lines)


def _build_prompt(
    *,
    branch: str,
    sha: str,
    handoff_path: Path | None,
    failure_packet_path: Path,
) -> str:
    handoff_text = str(handoff_path) if handoff_path else "(none)"
    return "\n".join(
        [
            f"Fresh repair pass for branch {branch} at HEAD {sha}.",
            "",
            "Current GitHub CI is red.",
            "",
            "Read these files first:",
            f"- Failure packet (authoritative current truth): {failure_packet_path}",
            f"- Background task handoff (scope/acceptance only): {handoff_text}",
            "",
            "Rules:",
            "- Treat the failure packet as the source of truth for what is failing now.",
            "- Use the background handoff only for task scope and acceptance context.",
            "- If the background handoff sounds complete but the failure packet says CI is red, trust the failure packet.",
            "- Make the smallest root-cause repo fix that gets the branch green.",
            "- Run focused local validation for the failing area before you change GitHub state.",
            "- If you make a repo fix, amend or create the branch commit as appropriate and push the same branch.",
            "- Do not wait for human approval.",
            "- If the failure is external, flaky, or inconclusive and no safe repo-side fix exists, explain that clearly and stop without repo changes.",
            "",
            "Goal: get the current branch's GitHub CI green without human babysitting.",
        ]
    )


def _write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _write_state(
    *,
    branch: str,
    sha: str,
    attempts: int,
    status: str,
    handoff_path: Path | None,
    note: str,
) -> None:
    body = "\n".join(
        [
            "# CI repair loop",
            "",
            f"- Updated: {_now()}",
            f"- Branch: {branch}",
            f"- Head SHA: {sha}",
            f"- Attempts used: {attempts}",
            f"- Status: {status}",
            f"- Background handoff: {handoff_path if handoff_path else '(none)'}",
            f"- Failure packet: {FAILURE_PACKET_PATH}",
            f"- Prompt: {PROMPT_PATH}",
            f"- Last Codex response: {RESPONSE_PATH}",
            "",
            note,
            "",
        ]
    )
    _write_text(STATE_PATH, body)


def _notify(repo_root: Path, message: str) -> None:
    script = repo_root / "scripts" / "wsl-notify.sh"
    if not script.exists():
        return
    subprocess.run(
        [str(script), "ytnova", message],
        cwd=repo_root,
        text=True,
        capture_output=True,
        check=False,
    )


def _session_name(branch: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "-", branch).strip("-")
    if not safe:
        safe = "branch"
    return f"ytnova-ci-repair-{safe}"


def _base_command(*, handoff: str | None, poll_seconds: int, max_attempts: int,
                  model: str | None, profile: str | None, dry_run: bool) -> list[str]:
    args = ["python3", str(REPO_ROOT / "scripts" / "ci_repair_loop.py")]
    if handoff:
        args.extend(["--handoff", handoff])
    if poll_seconds != DEFAULT_POLL_SECONDS:
        args.extend(["--poll-seconds", str(poll_seconds)])
    if max_attempts != DEFAULT_MAX_ATTEMPTS:
        args.extend(["--max-attempts", str(max_attempts)])
    if model:
        args.extend(["--model", model])
    if profile:
        args.extend(["--profile", profile])
    if dry_run:
        args.append("--dry-run")
    return args


def _write_launch_info(payload: dict[str, Any]) -> None:
    _write_text(LAUNCH_INFO_PATH, json.dumps(payload, indent=2) + "\n")


def _tmux_session_exists(session_name: str, *, repo_root: Path) -> bool:
    run = _run(["tmux", "has-session", "-t", session_name], cwd=repo_root)
    return run.returncode == 0


def _start_detached(
    *,
    repo_root: Path,
    branch: str,
    handoff: str | None,
    poll_seconds: int,
    max_attempts: int,
    model: str | None,
    profile: str | None,
    dry_run: bool,
) -> tuple[str, str]:
    HANDOFF_DIR.mkdir(parents=True, exist_ok=True)
    base_cmd = _base_command(
        handoff=handoff,
        poll_seconds=poll_seconds,
        max_attempts=max_attempts,
        model=model,
        profile=profile,
        dry_run=dry_run,
    )
    command_text = " ".join(shlex.quote(part) for part in base_cmd)
    log_handle = LOG_PATH.open("a", encoding="utf-8")
    session_name = _session_name(branch)

    if shutil.which("tmux"):
        if _tmux_session_exists(session_name, repo_root=repo_root):
            _write_launch_info(
                {
                    "mode": "tmux",
                    "session": session_name,
                    "branch": branch,
                    "log": str(LOG_PATH),
                    "command": command_text,
                    "status": "already-running",
                }
            )
            return "already-running", session_name
        shell_command = (
            f"cd {shlex.quote(str(repo_root))} && "
            f"{command_text} >> {shlex.quote(str(LOG_PATH))} 2>&1"
        )
        run = _require_ok(
            ["tmux", "new-session", "-d", "-s", session_name, shell_command],
            cwd=repo_root,
        )
        del run
        _write_launch_info(
            {
                "mode": "tmux",
                "session": session_name,
                "branch": branch,
                "log": str(LOG_PATH),
                "command": command_text,
                "status": "started",
            }
        )
        return "tmux", session_name

    proc = subprocess.Popen(
        base_cmd,
        cwd=repo_root,
        stdout=log_handle,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        start_new_session=True,
        text=True,
    )
    log_handle.close()
    _write_launch_info(
        {
            "mode": "background",
            "pid": proc.pid,
            "branch": branch,
            "log": str(LOG_PATH),
            "command": command_text,
            "status": "started",
        }
    )
    return "background", str(proc.pid)


def _invoke_codex(
    *,
    repo_root: Path,
    prompt_text: str,
    model: str | None,
    profile: str | None,
    dry_run: bool,
) -> int:
    _write_text(PROMPT_PATH, prompt_text)
    if dry_run:
        _write_text(RESPONSE_PATH, "dry-run: Codex invocation skipped.\n")
        return 0

    args = [
        "codex",
        "exec",
        "--cd",
        str(repo_root),
        "--dangerously-bypass-approvals-and-sandbox",
        "--color",
        "never",
        "-o",
        str(RESPONSE_PATH),
        "-",
    ]
    if model:
        args[2:2] = ["--model", model]
    if profile:
        insert_at = 2 if model is None else 4
        args[insert_at:insert_at] = ["--profile", profile]

    run = subprocess.run(args, cwd=repo_root, input=prompt_text, text=True, check=False)
    return run.returncode


def _branch_green_message(branch: str, sha: str) -> str:
    return f"{branch} CI green at {sha[:12]}"


def _blocked_message(branch: str, reason: str) -> str:
    return f"{branch} CI repair blocked: {reason}"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Watch current branch GitHub Actions and launch fresh Codex repairs on red."
    )
    parser.add_argument("--handoff", help="Task handoff file to layer under live CI failure context.")
    parser.add_argument(
        "--poll-seconds",
        type=int,
        default=DEFAULT_POLL_SECONDS,
        help=f"Polling interval while GitHub runs are pending (default: {DEFAULT_POLL_SECONDS}).",
    )
    parser.add_argument(
        "--max-attempts",
        type=int,
        default=DEFAULT_MAX_ATTEMPTS,
        help=f"Maximum fresh Codex repair attempts before stopping (default: {DEFAULT_MAX_ATTEMPTS}).",
    )
    parser.add_argument("--model", help="Optional Codex model override for repair attempts.")
    parser.add_argument("--profile", help="Optional Codex profile override for repair attempts.")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Write failure packet/prompt but skip the Codex invocation.",
    )
    parser.add_argument(
        "--detach",
        action="store_true",
        help="Start the repair loop in tmux if available, otherwise as a detached background process.",
    )
    args = parser.parse_args()

    if args.poll_seconds < 0:
        print("--poll-seconds must be >= 0", file=sys.stderr)
        return 2
    if args.max_attempts < 1:
        print("--max-attempts must be >= 1", file=sys.stderr)
        return 2

    repo_root = REPO_ROOT
    handoff_path = _discover_handoff(args.handoff)
    branch = _git(["branch", "--show-current"], repo_root=repo_root)
    sha = _git(["rev-parse", "HEAD"], repo_root=repo_root)
    repo = _parse_repo_slug(_git(["remote", "get-url", "origin"], repo_root=repo_root))
    attempts = 0
    last_attempt_signature: tuple[str, tuple[int, ...]] | None = None

    if args.detach:
        handoff_text = str(handoff_path) if handoff_path else None
        mode, handle = _start_detached(
            repo_root=repo_root,
            branch=branch,
            handoff=handoff_text,
            poll_seconds=args.poll_seconds,
            max_attempts=args.max_attempts,
            model=args.model,
            profile=args.profile,
            dry_run=args.dry_run,
        )
        if mode == "already-running":
            print(
                f"ci-repair loop already running in tmux session {handle}\n"
                f"log: {LOG_PATH}\n"
                f"state: {STATE_PATH}"
            )
            return 0
        print(
            f"started ci-repair loop in {mode}: {handle}\n"
            f"log: {LOG_PATH}\n"
            f"state: {STATE_PATH}"
        )
        return 0

    print(f"[{_now()}] Watching {repo} branch {branch} at {sha[:12]}")
    _write_state(
        branch=branch,
        sha=sha,
        attempts=attempts,
        status="active",
        handoff_path=handoff_path,
        note="Waiting for GitHub Actions state for the current branch head.",
    )

    while True:
        branch = _git(["branch", "--show-current"], repo_root=repo_root)
        sha = _git(["rev-parse", "HEAD"], repo_root=repo_root)
        runs = _load_runs(repo, repo_root=repo_root, branch=branch, sha=sha)
        state = _classify_runs(runs)

        if state == "missing":
            print(f"[{_now()}] No GitHub runs yet for {sha[:12]}; sleeping {args.poll_seconds}s.")
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="active",
                handoff_path=handoff_path,
                note="No GitHub workflow runs have appeared yet for the current head SHA.",
            )
            time.sleep(args.poll_seconds)
            continue

        if state == "pending":
            pending_summary = _summarize_pending(runs)
            print(
                f"[{_now()}] GitHub runs still pending for {sha[:12]}: "
                f"{pending_summary}. Sleeping {args.poll_seconds}s."
            )
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="active",
                handoff_path=handoff_path,
                note=f"Pending workflows: {pending_summary}",
            )
            time.sleep(args.poll_seconds)
            continue

        if state == "success":
            message = _branch_green_message(branch, sha)
            print(f"[{_now()}] {message}")
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="completed",
                handoff_path=handoff_path,
                note="All observed GitHub workflow runs for the current head SHA are green.",
            )
            _notify(repo_root, message)
            return 0

        failed_runs = _failed_runs(runs)
        signature = _failed_signature(sha, failed_runs)
        if signature == last_attempt_signature:
            reason = "same failed run set is still red after the last repair attempt"
            print(f"[{_now()}] BLOCKED: {reason}.")
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="blocked",
                handoff_path=handoff_path,
                note=reason,
            )
            _notify(repo_root, _blocked_message(branch, reason))
            return 1

        if attempts >= args.max_attempts:
            reason = f"attempt budget exhausted ({args.max_attempts})"
            print(f"[{_now()}] BLOCKED: {reason}.")
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="blocked",
                handoff_path=handoff_path,
                note=reason,
            )
            _notify(repo_root, _blocked_message(branch, reason))
            return 1

        attempts += 1
        print(
            f"[{_now()}] GitHub CI red for {sha[:12]}; "
            f"launching fresh repair attempt {attempts}/{args.max_attempts}."
        )
        run_details = [
            _load_run_detail(repo, repo_root=repo_root, run_id=int(run["databaseId"]))
            for run in failed_runs
        ]
        logs = [
            _load_failed_log(repo, repo_root=repo_root, run_id=int(run["databaseId"]))
            for run in failed_runs
        ]
        failure_packet = _build_failure_packet(
            repo=repo,
            branch=branch,
            sha=sha,
            attempt=attempts,
            handoff_path=handoff_path,
            failed_runs=failed_runs,
            run_details=run_details,
            logs=logs,
        )
        _write_text(FAILURE_PACKET_PATH, failure_packet)
        prompt_text = _build_prompt(
            branch=branch,
            sha=sha,
            handoff_path=handoff_path,
            failure_packet_path=FAILURE_PACKET_PATH,
        )
        _write_state(
            branch=branch,
            sha=sha,
            attempts=attempts,
            status="active",
            handoff_path=handoff_path,
            note=f"Repair attempt {attempts} launched from current failure packet.",
        )
        rc = _invoke_codex(
            repo_root=repo_root,
            prompt_text=prompt_text,
            model=args.model,
            profile=args.profile,
            dry_run=args.dry_run,
        )
        last_attempt_signature = signature
        if rc != 0:
            reason = f"Codex repair process exited {rc}"
            print(f"[{_now()}] BLOCKED: {reason}.")
            _write_state(
                branch=branch,
                sha=sha,
                attempts=attempts,
                status="blocked",
                handoff_path=handoff_path,
                note=reason,
            )
            _notify(repo_root, _blocked_message(branch, reason))
            return 1


if __name__ == "__main__":
    sys.exit(main())
