from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

DOCTOR_PATH = Path(__file__).resolve().parents[1] / "scripts" / "mcp_doctor.py"
DOCTOR_SPEC = importlib.util.spec_from_file_location("mcp_doctor", DOCTOR_PATH)
assert DOCTOR_SPEC is not None and DOCTOR_SPEC.loader is not None
doctor = importlib.util.module_from_spec(DOCTOR_SPEC)
DOCTOR_SPEC.loader.exec_module(doctor)

SAMPLE_CONFIG = """\
model = "gpt-5.3-codex"

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

NON_PORTABLE_CONFIG = """\
model = "gpt-5.3-codex"

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


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def test_mcp_doctor_fix_bootstraps_user_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    home_config = tmp_path / "home" / ".codex" / "config.toml"
    project_config = tmp_path / "repo" / ".codex" / "config.toml"
    _write(project_config, SAMPLE_CONFIG)

    monkeypatch.setattr(doctor, "CONFIG_PATH", home_config)
    monkeypatch.setattr(doctor, "PROJECT_CONFIG_PATH", project_config)
    monkeypatch.setattr(doctor, "_ensure_dir", lambda path: (True, ""))
    monkeypatch.setattr(
        doctor, "_run_smoke", lambda name, cfg: ("ok", f"{name}: startup probe succeeded")
    )
    monkeypatch.setattr(sys, "argv", ["mcp_doctor.py", "--fix"])

    rc = doctor.main()
    output = capsys.readouterr().out

    assert rc == 0
    assert home_config.exists()
    assert "FIXED: bootstrapped" in output
    assert "RESULT: OK" in output


def test_mcp_doctor_without_fix_reports_missing_user_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    home_config = tmp_path / "home" / ".codex" / "config.toml"
    project_config = tmp_path / "repo" / ".codex" / "config.toml"
    _write(project_config, SAMPLE_CONFIG)

    monkeypatch.setattr(doctor, "CONFIG_PATH", home_config)
    monkeypatch.setattr(doctor, "PROJECT_CONFIG_PATH", project_config)
    monkeypatch.setattr(sys, "argv", ["mcp_doctor.py"])

    rc = doctor.main()
    output = capsys.readouterr().out

    assert rc == 1
    assert "FAIL: missing config" in output
    assert "run with --fix" in output


def test_mcp_doctor_allows_home_path_personal_override_with_warning(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    home_config = tmp_path / "home" / ".codex" / "config.toml"
    project_config = tmp_path / "repo" / ".codex" / "config.toml"
    _write(home_config, NON_PORTABLE_CONFIG)
    _write(project_config, SAMPLE_CONFIG)

    monkeypatch.setattr(doctor, "CONFIG_PATH", home_config)
    monkeypatch.setattr(doctor, "PROJECT_CONFIG_PATH", project_config)
    monkeypatch.setattr(doctor, "_ensure_dir", lambda path: (True, ""))
    monkeypatch.setattr(
        doctor, "_run_smoke", lambda name, cfg: ("ok", f"{name}: startup probe succeeded")
    )
    monkeypatch.setattr(sys, "argv", ["mcp_doctor.py"])

    rc = doctor.main()
    output = capsys.readouterr().out

    assert rc == 0
    assert "WARN: serena config references a home path" in output
