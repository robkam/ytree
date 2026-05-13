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


def test_relay_run_script_attempts_prompt_auto_stage_and_prints_fallback_command() -> None:
    script = (Path(__file__).resolve().parents[1] / "scripts" / "relay-run.sh").read_text(
        encoding="utf-8"
    )

    assert "scripts/relay-prompts.sh stage --run-id \"$run_id\" --auto" in script
    assert "scripts/relay-prompts.sh verify --run-id \"$run_id\"" in script
    assert "PROMPT ARTIFACTS AUTO-STAGED" in script
    assert "PROMPT ARTIFACTS PENDING" in script
    assert "auto_src_dir=\"$HOME/.local/state/ytree/prompt-sources/$run_id\"" in script
    assert "INFO: auto prompt sources not emitted yet" in script
    assert "INFO: auto prompt sources incomplete in" in script
    assert "NEXT: $prompt_stage_cmd" in script


def test_relay_monitor_normalizes_and_clears_stale_action_needed() -> None:
    script = (Path(__file__).resolve().parents[1] / "scripts" / "relay-monitor.sh").read_text(
        encoding="utf-8"
    )

    assert 'prefix = "ACTION NEEDED (maintainer):"' in script
    assert '"workflow_resumed"' in script
    assert '"heartbeat"' in script
    assert 'action = normalize_action_message(msg)' in script


def test_relay_prompts_auto_stage_reports_wait_for_architect_when_sources_missing() -> None:
    script = (Path(__file__).resolve().parents[1] / "scripts" / "relay-prompts.sh").read_text(
        encoding="utf-8"
    )

    assert "expected files like: developer*.txt and auditor*.txt" in script
    assert "wait for architect update that confirms prompt sources were emitted" in script
