import os
import subprocess
from pathlib import Path


def _init_git_repo(repo_dir: Path, commit_subject: str) -> None:
    subprocess.run(["git", "init"], cwd=repo_dir, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "config", "user.email", "codex@example.com"],
        cwd=repo_dir,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        ["git", "config", "user.name", "Codex"],
        cwd=repo_dir,
        check=True,
        capture_output=True,
        text=True,
    )

    note_path = repo_dir / "note.txt"
    note_path.write_text("hello\n", encoding="utf-8")
    subprocess.run(["git", "add", "note.txt"], cwd=repo_dir, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "commit", "-m", commit_subject],
        cwd=repo_dir,
        check=True,
        capture_output=True,
        text=True,
    )


def _write_powershell_stub(bin_dir: Path, capture_file: Path) -> Path:
    stub_path = bin_dir / "powershell.exe"
    stub_path.write_text(
        "#!/usr/bin/env bash\n"
        "set -eu\n"
        'printf "%s" "$WSL_NOTIFY_MESSAGE" > "$WSL_NOTIFY_CAPTURE_FILE"\n',
        encoding="utf-8",
    )
    stub_path.chmod(0o755)
    capture_file.write_text("", encoding="utf-8")
    return stub_path


def _run_notify_script(repo_root: Path, cwd: Path, message: str, tmp_path: Path) -> str:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    capture_file = tmp_path / "message.txt"
    _write_powershell_stub(bin_dir, capture_file)

    env = os.environ.copy()
    env["PATH"] = f"{bin_dir}:{env['PATH']}"
    env["WSL_NOTIFY_CAPTURE_FILE"] = str(capture_file)

    subprocess.run(
        [str(repo_root / "scripts" / "wsl-notify.sh"), "ytnova", message],
        cwd=cwd,
        env=env,
        check=True,
        capture_output=True,
        text=True,
    )
    return capture_file.read_text(encoding="utf-8")


def test_wsl_notify_replaces_generic_placeholder_with_repo_context(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    git_repo = tmp_path / "repo"
    git_repo.mkdir()
    _init_git_repo(git_repo, "Draft PR created.")

    captured_message = _run_notify_script(
        repo_root,
        git_repo,
        "This is a notification.",
        tmp_path,
    )

    assert captured_message == "Draft PR created."


def test_wsl_notify_preserves_explicit_milestone_message(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    git_repo = tmp_path / "repo"
    git_repo.mkdir()
    _init_git_repo(git_repo, "Implementation complete, ready for review.")

    captured_message = _run_notify_script(
        repo_root,
        git_repo,
        "Draft PR created.",
        tmp_path,
    )

    assert captured_message == "Draft PR created."
