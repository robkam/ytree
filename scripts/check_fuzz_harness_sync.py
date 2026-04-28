#!/usr/bin/env python3
"""Guardrail check: source changes must include corresponding fuzz harness edits."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

SOURCE_TO_HARNESS = {
    "src/util/string_utils.c": ("tests/fuzz/fuzz_string_utils.c",),
    "src/util/path_utils.c": ("tests/fuzz/fuzz_path_utils.c",),
    "src/fs/filter_core.c": ("tests/fuzz/fuzz_filter_core.c",),
}

ZERO_SHA = "0" * 40


def _normalize_paths(paths: list[str]) -> set[str]:
    normalized: set[str] = set()
    for raw in paths:
        line = raw.strip()
        if not line:
            continue
        if line.startswith("./"):
            line = line[2:]
        normalized.add(line)
    return normalized


def _run_git_changed_files(repo_root: Path, cmd: list[str]) -> set[str]:
    run = subprocess.run(
        cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return _normalize_paths(run.stdout.splitlines())


def _is_revision_error(exc: subprocess.CalledProcessError) -> bool:
    output = ((exc.stdout or "") + (exc.stderr or "")).lower()
    patterns = (
        "invalid revision range",
        "bad revision",
        "unknown revision",
        "ambiguous argument",
    )
    return any(pattern in output for pattern in patterns)


def _git_changed_files(repo_root: Path, base: str | None, head: str | None) -> set[str]:
    if base and head:
        if base == ZERO_SHA:
            cmd = ["git", "show", "--pretty=format:", "--name-only", head, "--"]
        else:
            cmd = ["git", "diff", "--name-only", f"{base}..{head}", "--"]
    else:
        cmd = ["git", "diff", "--name-only", "HEAD", "--"]

    try:
        return _run_git_changed_files(repo_root, cmd)
    except subprocess.CalledProcessError as exc:
        if base and head and base != ZERO_SHA and _is_revision_error(exc):
            fallback = ["git", "show", "--pretty=format:", "--name-only", head, "--"]
            return _run_git_changed_files(repo_root, fallback)
        raise


def find_missing_fuzz_updates(changed_files: set[str]) -> list[str]:
    failures: list[str] = []
    for source_path, harness_paths in SOURCE_TO_HARNESS.items():
        if source_path not in changed_files:
            continue
        if any(harness in changed_files for harness in harness_paths):
            continue
        expected = ", ".join(harness_paths)
        failures.append(
            f"{source_path} changed without matching fuzz harness update "
            f"(expected: {expected})"
        )
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", help="Base git ref/SHA for range checks")
    parser.add_argument("--head", help="Head git ref/SHA for range checks")
    args = parser.parse_args()
    if bool(args.base) != bool(args.head):
        parser.error("--base and --head must be provided together")
    return args


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    try:
        changed_files = _git_changed_files(repo_root, args.base, args.head)
    except subprocess.CalledProcessError as exc:
        cmd = " ".join(exc.cmd)
        output = (exc.stdout or "") + (exc.stderr or "")
        print(f"FAIL: unable to determine changed files from git command: {cmd}")
        print(output.strip())
        return 2

    failures = find_missing_fuzz_updates(changed_files)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: fuzz harness sync guard failed ({len(failures)} issue(s))")
        return 1

    print("PASS: fuzz harness sync guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
