from __future__ import annotations

from pathlib import Path

import pytest

UNIT_DIR = Path(__file__).resolve().parents[1] / "infra" / "systemd"


@pytest.mark.parametrize(
    "unit_name",
    [
        "ytree-relay-developer.service",
        "ytree-relay-code-auditor.service",
        "ytree-relay-watchdog.service",
    ],
)
def test_relay_units_are_supervised(unit_name: str) -> None:
    unit_path = UNIT_DIR / unit_name
    content = unit_path.read_text(encoding="utf-8")

    assert "Restart=always" in content
    assert "RestartSec=" in content
    assert "EnvironmentFile=" in content
    assert "ExecStart=" in content


def test_worker_units_include_role_specific_runtime_invocation() -> None:
    developer = (UNIT_DIR / "ytree-relay-developer.service").read_text(encoding="utf-8")
    auditor = (UNIT_DIR / "ytree-relay-code-auditor.service").read_text(encoding="utf-8")

    assert "relay_runtime.py worker --role developer" in developer
    assert "relay_runtime.py worker --role code_auditor" in auditor


def test_watchdog_unit_invokes_watchdog_loop() -> None:
    watchdog = (UNIT_DIR / "ytree-relay-watchdog.service").read_text(encoding="utf-8")
    assert "relay_runtime.py watchdog" in watchdog


def test_setup_relay_runtime_script_orchestrates_one_time_bootstrap() -> None:
    script = (Path(__file__).resolve().parents[1] / "scripts" / "setup_relay_runtime.sh").read_text(
        encoding="utf-8"
    )

    assert "make mcp-doctor FIX=1" in script
    assert "relay-systemd-install.sh" in script
    assert "relay-workers.sh" in script
    assert "RELAY_DEVELOPER_CMD" in script
    assert "RELAY_CODE_AUDITOR_CMD" in script
    assert "placeholder '/usr/bin/true'" in script
    assert "XDG_RUNTIME_DIR" in script
    assert "systemctl --user show-environment" in script
