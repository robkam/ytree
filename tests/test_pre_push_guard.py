from __future__ import annotations

import importlib.util
import io
import sys
from pathlib import Path

import pytest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "pre_push_guard.py"
SCRIPT_SPEC = importlib.util.spec_from_file_location("pre_push_guard", SCRIPT_PATH)
assert SCRIPT_SPEC is not None and SCRIPT_SPEC.loader is not None
pre_push_guard = importlib.util.module_from_spec(SCRIPT_SPEC)
SCRIPT_SPEC.loader.exec_module(pre_push_guard)


def test_push_without_codebase_changes_skips_local_gate(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    monkeypatch.setattr(pre_push_guard, "_git_repo_root", lambda: repo_root)
    monkeypatch.setattr(
        pre_push_guard, "_changed_paths_for_range", lambda repo_root, diff_range: ["README.md"]
    )
    monkeypatch.setattr(pre_push_guard, "_local_contains_commit", lambda repo_root, sha: False)
    monkeypatch.setenv("YTNOVA_PRE_PUSH_FORCE", "0")

    stdin = io.StringIO(
        "refs/heads/feat deadbeefdeadbeefdeadbeefdeadbeefdeadbeef "
        "refs/heads/feat 1111111111111111111111111111111111111111\n"
    )
    rc = pre_push_guard.main(["pre_push_guard.py", "origin", "https://github.com/robkam/ytreenova.git"], stdin)
    output = capsys.readouterr().out

    assert rc == 0


def test_failed_remote_head_blocks_fix_on_fix_push(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    monkeypatch.setattr(pre_push_guard, "_git_repo_root", lambda: repo_root)
    monkeypatch.setattr(pre_push_guard, "_local_contains_commit", lambda repo_root, sha: True)
    monkeypatch.setattr(pre_push_guard, "_load_remote_ci_state", lambda *args, **kwargs: "failure")
    monkeypatch.setattr(
        pre_push_guard, "_changed_paths_for_range", lambda repo_root, diff_range: ["src/ui/ctrl_file_ops.c"]
    )
    monkeypatch.setenv("YTNOVA_PRE_PUSH_FORCE", "0")

    stdin = io.StringIO(
        "refs/heads/feat cafebabecafebabecafebabecafebabecafebabe "
        "refs/heads/feat 1111111111111111111111111111111111111111\n"
    )
    rc = pre_push_guard.main(["pre_push_guard.py", "origin", "https://github.com/robkam/ytreenova.git"], stdin)
    output = capsys.readouterr().out

    assert rc == 1


def test_rewritten_branch_after_failed_remote_head_can_proceed(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    monkeypatch.setattr(pre_push_guard, "_git_repo_root", lambda: repo_root)
    monkeypatch.setattr(pre_push_guard, "_local_contains_commit", lambda repo_root, sha: False)
    monkeypatch.setattr(
        pre_push_guard, "_changed_paths_for_range", lambda repo_root, diff_range: ["src/ui/ctrl_file_ops.c"]
    )
    monkeypatch.setenv("YTNOVA_PRE_PUSH_FORCE", "0")

    calls: list[str] = []
    monkeypatch.setattr(pre_push_guard, "_run_make", lambda repo_root, target: calls.append(target))

    stdin = io.StringIO(
        "refs/heads/feat cafebabecafebabecafebabecafebabecafebabe "
        "refs/heads/feat 1111111111111111111111111111111111111111\n"
    )
    rc = pre_push_guard.main(["pre_push_guard.py", "origin", "https://github.com/robkam/ytreenova.git"], stdin)
    output = capsys.readouterr().out

    assert rc == 0
    assert calls == ["qa-code-quality"]


def test_force_mode_still_respects_failed_remote_head_block(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    monkeypatch.setattr(pre_push_guard, "_git_repo_root", lambda: repo_root)
    monkeypatch.setattr(pre_push_guard, "_local_contains_commit", lambda repo_root, sha: True)
    monkeypatch.setattr(pre_push_guard, "_load_remote_ci_state", lambda *args, **kwargs: "failure")
    monkeypatch.setenv("YTNOVA_PRE_PUSH_FORCE", "1")

    stdin = io.StringIO(
        "refs/heads/feat cafebabecafebabecafebabecafebabecafebabe "
        "refs/heads/feat 1111111111111111111111111111111111111111\n"
    )
    rc = pre_push_guard.main(["pre_push_guard.py", "origin", "https://github.com/robkam/ytreenova.git"], stdin)
    output = capsys.readouterr().out

    assert rc == 1
