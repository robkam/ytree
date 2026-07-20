#!/usr/bin/env python3
"""Repository pre-push quality and clean-slate guard."""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Iterable, TextIO


ZERO_SHA = "0" * 40
FAILURE_CONCLUSIONS = {
    "failure",
    "cancelled",
    "timed_out",
    "action_required",
    "startup_failure",
    "stale",
}
SUCCESS_CONCLUSIONS = {"success", "neutral", "skipped"}


def is_codebase_path(path: str) -> bool:
    return path.startswith(
        ("src/", "include/", "tests/", "scripts/", ".githooks/")
    ) or path == "Makefile"


def _run(
    args: list[str],
    *,
    cwd: Path,
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


def _require_ok(args: list[str], *, cwd: Path) -> subprocess.CompletedProcess[str]:
    run = _run(args, cwd=cwd)
    if run.returncode != 0:
        detail = (run.stderr or run.stdout or "").strip()
        raise RuntimeError(f"{' '.join(args)} failed: {detail}")
    return run


def _git_repo_root() -> Path:
    return Path(_require_ok(["git", "rev-parse", "--show-toplevel"], cwd=Path.cwd()).stdout.strip())


def _parse_repo_slug(remote_url: str) -> str | None:
    if "github.com" not in remote_url:
        return None
    match = re.search(r"[:/]([^/:]+/[^/]+?)(?:\.git)?$", remote_url.strip())
    if not match:
        return None
    return match.group(1)


def _branch_name(remote_ref: str) -> str | None:
    prefix = "refs/heads/"
    if not remote_ref.startswith(prefix):
        return None
    return remote_ref[len(prefix) :]


def _changed_paths_for_range(repo_root: Path, diff_range: str) -> list[str]:
    run = _require_ok(
        ["git", "diff", "--name-only", "--diff-filter=ACMRTUXB", diff_range],
        cwd=repo_root,
    )
    return [line for line in run.stdout.splitlines() if line]


def _local_contains_commit(repo_root: Path, sha: str) -> bool:
    run = _run(["git", "merge-base", "--is-ancestor", sha, "HEAD"], cwd=repo_root)
    if run.returncode == 0:
        return True
    if run.returncode == 1:
        return False
    detail = (run.stderr or run.stdout or "").strip()
    raise RuntimeError(f"git merge-base --is-ancestor {sha} HEAD failed: {detail}")


def _load_remote_ci_state(repo_root: Path, repo_slug: str, branch: str, sha: str) -> str:
    run = _run(
        [
            "gh",
            "run",
            "list",
            "--repo",
            repo_slug,
            "--branch",
            branch,
            "--commit",
            sha,
            "--limit",
            "20",
            "--json",
            "databaseId,workflowName,status,conclusion",
        ],
        cwd=repo_root,
    )
    if run.returncode != 0:
        detail = (run.stderr or run.stdout or "").strip()
        raise RuntimeError(f"gh run list failed: {detail}")
    payload = json.loads(run.stdout)
    if not isinstance(payload, list) or not payload:
        return "missing"
    for item in payload:
        status = str(item.get("status", "") or "")
        conclusion = str(item.get("conclusion", "") or "")
        if status != "completed" or not conclusion:
            return "pending"
    for item in payload:
        conclusion = str(item.get("conclusion", "") or "")
        if conclusion in FAILURE_CONCLUSIONS or conclusion not in SUCCESS_CONCLUSIONS:
            return "failure"
    return "success"


def _run_make(repo_root: Path, target: str) -> None:
    run = subprocess.run(["make", target], cwd=repo_root, check=False)
    if run.returncode != 0:
        raise SystemExit(run.returncode)


def _parse_updates(lines: Iterable[str]) -> list[tuple[str, str, str, str]]:
    updates: list[tuple[str, str, str, str]] = []
    for raw in lines:
        line = raw.strip()
        if not line:
            continue
        local_ref, local_sha, remote_ref, remote_sha = line.split()
        updates.append((local_ref, local_sha, remote_ref, remote_sha))
    return updates


def main(argv: list[str] | None = None, stdin: TextIO | None = None) -> int:
    argv = list(sys.argv if argv is None else argv)
    stdin = sys.stdin if stdin is None else stdin
    remote_name = argv[1] if len(argv) > 1 else "origin"
    remote_url = argv[2] if len(argv) > 2 else ""

    repo_root = _git_repo_root()
    updates = _parse_updates(stdin)
    delete_only = True
    updates_main = False
    codebase_changed = False

    failed_remote_heads: list[tuple[str, str]] = []
    force_gate = os.environ.get("YTNOVA_PRE_PUSH_FORCE", "0") == "1"

    for local_ref, local_sha, remote_ref, remote_sha in updates:
        if local_sha == ZERO_SHA or local_ref == "(delete)":
            continue
        delete_only = False
        if remote_ref == "refs/heads/main":
            updates_main = True

        branch = _branch_name(remote_ref)
        repo_slug = _parse_repo_slug(remote_url)
        if (
            branch
            and repo_slug
            and remote_sha != ZERO_SHA
            and _local_contains_commit(repo_root, remote_sha)
        ):
            state = _load_remote_ci_state(repo_root, repo_slug, branch, remote_sha)
            if state == "failure":
                failed_remote_heads.append((branch, remote_sha))

        if force_gate:
            codebase_changed = True
            continue

        diff_range = local_sha if remote_sha == ZERO_SHA else f"{remote_sha}..{local_sha}"
        for changed_path in _changed_paths_for_range(repo_root, diff_range):
            if is_codebase_path(changed_path):
                codebase_changed = True
                break

    if delete_only:
        print("[pre-push] Delete-only push detected: skipping CI gate.")
        return 0

    if failed_remote_heads:
        branch, sha = failed_remote_heads[0]
        print(
            "[pre-push] Blocked: remote branch head has failing GitHub CI and is still in local history."
        )
        print(
            f"[pre-push] Branch '{branch}' remote SHA {sha[:12]} must be rewritten from the last green state,"
        )
        print(
            "[pre-push] not layered with more fixes on top of the failed attempt."
        )
        print(
            "[pre-push] Reset/rebase to drop the failed SHA, redo the work from a fresh context, then push again."
        )
        return 1

    if not codebase_changed:
        print("[pre-push] Push without codebase changes: skipping local quality gate.")
        print("[pre-push] Set YTNOVA_PRE_PUSH_FORCE=1 to force make qa-code-quality.")
        return 0

    print("[pre-push] Running local code-quality gate: make qa-code-quality")
    _run_make(repo_root, "qa-code-quality")

    if updates_main:
        print("[pre-push] Push updates main: running baseline gate: make ci-baseline")
        _run_make(repo_root, "ci-baseline")

    print("[pre-push] Checks passed. Proceeding with push.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
