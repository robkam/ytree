from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
RELAY_PATH = REPO_ROOT / "scripts" / "relay.py"
CORE_PATH = REPO_ROOT / "scripts" / "relay_core.py"
BACKENDS_PATH = REPO_ROOT / "scripts" / "relay_backends.py"
FAKE_BACKEND_PATH = REPO_ROOT / "scripts" / "relay_backend_fake.py"
CODEX_BACKEND_PATH = REPO_ROOT / "scripts" / "relay_backend_codex.py"


def _load_module(module_name: str, path: Path):
    spec = importlib.util.spec_from_file_location(module_name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


relay_backends = _load_module("relay_backends", BACKENDS_PATH)
relay_core = _load_module("relay_core", CORE_PATH)
relay_backend_fake = _load_module("relay_backend_fake", FAKE_BACKEND_PATH)
relay_backend_codex = _load_module("relay_backend_codex", CODEX_BACKEND_PATH)


def test_config_precedence_cli_over_env_over_user_over_repo_defaults(tmp_path: Path) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    defaults_path = repo_root / "scripts" / "relay.defaults.toml"
    defaults_path.parent.mkdir(parents=True)
    defaults_path.write_text(
        "\n".join(
            [
                "backend = \"codex\"",
                "runtime_dir = \"/tmp/default-runtime\"",
                "stale_seconds = 100",
                "codex_timeout_seconds = 30",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    user_config = tmp_path / "user-relay.toml"
    user_config.write_text(
        "\n".join(
            [
                "backend = \"fake\"",
                f"runtime_dir = \"{tmp_path / 'user-runtime'}\"",
                "stale_seconds = 200",
                "codex_timeout_seconds = 40",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    env = {
        "RELAY_BACKEND": "codex",
        "RELAY_RUNTIME_DIR": str(tmp_path / "env-runtime"),
        "RELAY_STALE_SECONDS": "300",
        "RELAY_CODEX_TIMEOUT_SECONDS": "50",
    }
    cli = {
        "backend": "fake",
        "runtime_dir": str(tmp_path / "cli-runtime"),
        "stale_seconds": 400,
        "codex_timeout_seconds": 60,
    }

    settings = relay_core.resolve_settings(
        repo_root=repo_root,
        cli_overrides=cli,
        env=env,
        user_config_path=user_config,
        repo_defaults_path=Path("scripts/relay.defaults.toml"),
    )

    assert settings.backend == "fake"
    assert settings.runtime_dir == (tmp_path / "cli-runtime")
    assert settings.stale_seconds == 400
    assert settings.codex_timeout_seconds == 60


def test_backend_selection_and_adapter_contract(tmp_path: Path) -> None:
    settings = relay_core.RelaySettings(
        backend="fake",
        runtime_dir=tmp_path / "runtime",
        stale_seconds=10,
        codex_command=(
            "python3",
            "-c",
            "from pathlib import Path; Path('{report_path}').write_text('ok\\n', encoding='utf-8')",
        ),
        codex_timeout_seconds=30,
    )
    work_item = relay_core.WorkItem(kind="bug", numeric_id=1, title="demo", source_path=Path("docs/BUGS.md"))
    context = relay_core.build_run_context(tmp_path, settings, work_item)

    events: list[dict[str, object]] = []

    def emit(event_type: str, message: str, metadata: dict[str, object] | None = None):
        event = {"event_type": event_type, "message": message, "metadata": metadata or {}}
        events.append(event)
        return event

    fake_backend = relay_backend_fake.FakeBackend()
    fake_result = fake_backend.run(context, emit)
    assert fake_result.status == "completed"
    assert context.report_path.exists()
    assert any(event["event_type"] == "backend_completed" for event in events)

    events.clear()
    codex_backend = relay_backend_codex.CodexBackend(command=settings.codex_command, timeout_seconds=30)
    codex_result = codex_backend.run(context, emit)
    assert codex_result.status == "completed"
    assert any(event["event_type"] == "worker_command_completed" for event in events)


def test_runtime_scratch_cleanup_removes_stale_paths(tmp_path: Path) -> None:
    runtime_dir = tmp_path / "ytree-relay"
    runtime_dir.mkdir()
    stale_dir = runtime_dir / "stale"
    stale_dir.mkdir()
    stale_file = runtime_dir / "stale.log"
    stale_file.write_text("x\n", encoding="utf-8")
    fresh_dir = runtime_dir / "fresh"
    fresh_dir.mkdir()

    now = time.time()
    old = now - 100
    os.utime(stale_dir, (old, old))
    os.utime(stale_file, (old, old))
    os.utime(fresh_dir, (now, now))

    removed = relay_core.cleanup_stale_runtime(runtime_dir, stale_seconds=60, now=now)

    assert stale_dir in removed
    assert stale_file in removed
    assert not stale_dir.exists()
    assert not stale_file.exists()
    assert fresh_dir.exists()


def test_monitor_action_needed_clears_after_resolution() -> None:
    events = [
        {"event_type": "action_needed", "message": 'reply "commit_message_approval"'},
        {"event_type": "heartbeat", "message": "working"},
        {"event_type": "action_resolved", "message": "approved"},
        {"event_type": "heartbeat", "message": "done"},
    ]
    assert relay_core.resolve_action_needed(events) == "none"

    pending = [
        {"event_type": "action_needed", "message": 'reply "true_blocker_decision"'},
        {"event_type": "heartbeat", "message": "waiting"},
    ]
    assert relay_core.resolve_action_needed(pending) == 'reply "true_blocker_decision"'


def test_cli_run_supports_bug_and_task_and_rejects_unknown_ids(tmp_path: Path) -> None:
    env = os.environ.copy()
    env["RELAY_BACKEND"] = "fake"

    bug_cmd = [
        "python3",
        str(RELAY_PATH),
        "run",
        "--bug",
        "1",
        "--runtime-dir",
        str(tmp_path / "runtime-bug"),
    ]
    bug = subprocess.run(bug_cmd, cwd=REPO_ROOT, text=True, capture_output=True, env=env, check=False)
    assert bug.returncode == 0
    assert "WORK ITEM: BUG-1:" in bug.stdout
    assert "ACTION NEEDED (maintainer): none" in bug.stdout

    task_cmd = [
        "python3",
        str(RELAY_PATH),
        "run",
        "--task",
        "1",
        "--runtime-dir",
        str(tmp_path / "runtime-task"),
    ]
    task = subprocess.run(task_cmd, cwd=REPO_ROOT, text=True, capture_output=True, env=env, check=False)
    assert task.returncode == 0
    assert "WORK ITEM: Task 1:" in task.stdout

    unknown_cmd = [
        "python3",
        str(RELAY_PATH),
        "run",
        "--bug",
        "99999",
        "--runtime-dir",
        str(tmp_path / "runtime-unknown"),
    ]
    unknown = subprocess.run(unknown_cmd, cwd=REPO_ROOT, text=True, capture_output=True, env=env, check=False)
    assert unknown.returncode == 2
    assert "Unknown bug id BUG-99999" in unknown.stderr
