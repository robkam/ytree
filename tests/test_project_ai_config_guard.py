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
