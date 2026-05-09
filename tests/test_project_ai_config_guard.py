from __future__ import annotations

import importlib.util
import subprocess
import textwrap
from pathlib import Path

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_project_ai_config.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_project_ai_config", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def test_check_config_flags_nonportable_home_path(tmp_path: Path) -> None:
    config = tmp_path / "config.toml"
    _write(
        config,
        textwrap.dedent(
            """\
            model_instructions_file = ".ai/codex.md"

            [mcp_servers.serena]
            command = "uvx"
            args = ["--from", "/home/alice/src/serena", "serena", "start-mcp-server", "--context", "codex"]

            [mcp_servers.serena.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"

            [mcp_servers.jcodemunch]
            command = "uvx"
            args = ["jcodemunch-mcp"]

            [mcp_servers.jcodemunch.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"
            """
        ),
    )

    failures = guard.check_config(config)
    assert any("non-portable home path" in failure for failure in failures)


def test_check_config_requires_model_instructions_file(tmp_path: Path) -> None:
    config = tmp_path / "config.toml"
    _write(
        config,
        textwrap.dedent(
            """\
            [mcp_servers.serena]
            command = "uvx"
            args = ["serena", "start-mcp-server", "--context", "codex"]

            [mcp_servers.serena.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"

            [mcp_servers.jcodemunch]
            command = "uvx"
            args = ["jcodemunch-mcp"]

            [mcp_servers.jcodemunch.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"
            """
        ),
    )

    failures = guard.check_config(config)
    assert any("model_instructions_file" in failure for failure in failures)


def test_check_config_accepts_absolute_model_instructions_file(tmp_path: Path) -> None:
    config = tmp_path / "config.toml"
    _write(
        config,
        textwrap.dedent(
            """\
            model_instructions_file = "/home/rob/ytree/.ai/codex.md"

            [mcp_servers.serena]
            command = "uvx"
            args = ["serena", "start-mcp-server", "--context", "codex"]

            [mcp_servers.serena.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"

            [mcp_servers.jcodemunch]
            command = "uvx"
            args = ["jcodemunch-mcp"]

            [mcp_servers.jcodemunch.env]
            UV_CACHE_DIR = "/tmp/codex-uv-cache"
            UV_TOOL_DIR = "/tmp/codex-uv-tools"
            """
        ),
    )

    failures = guard.check_config(config)
    assert not any("model_instructions_file" in failure for failure in failures)


def _write_relay_policy_files(repo_root: Path) -> None:
    for relative_path in guard.RELAY_POLICY_FILES:
        path = repo_root / relative_path
        _write(
            path,
            "\n".join(
                [
                    "# relay policy",
                    "policy_block_retry_once",
                    "watchdog_stall_retry_terminal",
                    "maintainer_pause_gate=true_blocker_decision|commit_message_approval",
                ]
            )
            + "\n",
        )


def _write_relay_runtime_stub(repo_root: Path) -> None:
    _write(
        repo_root / guard.RELAY_RUNTIME_FILE,
        "\n".join(["# relay runtime markers", *guard.RELAY_RUNTIME_REQUIRED_MARKERS]) + "\n",
    )


def _write_required_relay_env(repo_root: Path) -> None:
    _write(
        repo_root / guard.RELAY_ENV_TEMPLATE,
        textwrap.dedent(
            """\
            RELAY_POLICY_BLOCK_RETRY_LIMIT=1
            RELAY_MAINTAINER_HEARTBEAT_SECONDS=300
            RELAY_NO_PROGRESS_STALL_SECONDS=900
            RELAY_MAINTAINER_INTERRUPT_REASONS=true_blocker_decision,commit_message_approval
            """
        ),
    )


def test_relay_policy_requirements_pass_when_markers_are_present(tmp_path: Path) -> None:
    _write_relay_policy_files(tmp_path)
    _write_required_relay_env(tmp_path)
    _write_relay_runtime_stub(tmp_path)

    failures = guard.check_relay_policy_requirements(tmp_path)
    assert failures == []


def test_relay_policy_requirements_fail_when_token_missing(tmp_path: Path) -> None:
    _write_relay_policy_files(tmp_path)
    _write_relay_runtime_stub(tmp_path)
    _write(
        tmp_path / guard.RELAY_POLICY_FILES[0],
        "# relay policy\npolicy_block_retry_once\nwatchdog_stall_retry_terminal\n",
    )
    _write_required_relay_env(tmp_path)

    failures = guard.check_relay_policy_requirements(tmp_path)
    assert any("missing relay policy token" in failure for failure in failures)


def test_relay_policy_requirements_fail_when_env_setting_missing(tmp_path: Path) -> None:
    _write_relay_policy_files(tmp_path)
    _write_relay_runtime_stub(tmp_path)
    _write(
        tmp_path / guard.RELAY_ENV_TEMPLATE,
        textwrap.dedent(
            """\
            RELAY_MAINTAINER_HEARTBEAT_SECONDS=300
            RELAY_NO_PROGRESS_STALL_SECONDS=900
            RELAY_MAINTAINER_INTERRUPT_REASONS=true_blocker_decision,commit_message_approval
            """
        ),
    )

    failures = guard.check_relay_policy_requirements(tmp_path)
    assert any("RELAY_POLICY_BLOCK_RETRY_LIMIT" in failure for failure in failures)


def test_relay_policy_requirements_fail_when_interrupt_reasons_not_strict(tmp_path: Path) -> None:
    _write_relay_policy_files(tmp_path)
    _write_relay_runtime_stub(tmp_path)
    _write(
        tmp_path / guard.RELAY_ENV_TEMPLATE,
        textwrap.dedent(
            """\
            RELAY_POLICY_BLOCK_RETRY_LIMIT=1
            RELAY_MAINTAINER_HEARTBEAT_SECONDS=300
            RELAY_NO_PROGRESS_STALL_SECONDS=900
            RELAY_MAINTAINER_INTERRUPT_REASONS=true_blocker_decision
            """
        ),
    )

    failures = guard.check_relay_policy_requirements(tmp_path)
    assert any("RELAY_MAINTAINER_INTERRUPT_REASONS" in failure for failure in failures)


def test_relay_policy_requirements_fail_when_runtime_marker_missing(tmp_path: Path) -> None:
    _write_relay_policy_files(tmp_path)
    _write_required_relay_env(tmp_path)
    _write(
        tmp_path / guard.RELAY_RUNTIME_FILE,
        "\n".join(["# relay runtime markers", "--retry-limit", "stall_detected"]) + "\n",
    )

    failures = guard.check_relay_policy_requirements(tmp_path)
    assert any("missing runtime marker" in failure for failure in failures)


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_project_ai_config.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
