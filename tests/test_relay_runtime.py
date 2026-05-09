from __future__ import annotations

import importlib.util
import json
import os
import sys
from pathlib import Path

import pytest

RUNTIME_PATH = Path(__file__).resolve().parents[1] / "scripts" / "relay_runtime.py"
RUNTIME_SPEC = importlib.util.spec_from_file_location("relay_runtime", RUNTIME_PATH)
assert RUNTIME_SPEC is not None and RUNTIME_SPEC.loader is not None
relay = importlib.util.module_from_spec(RUNTIME_SPEC)
sys.modules[RUNTIME_SPEC.name] = relay
RUNTIME_SPEC.loader.exec_module(relay)


def _new_store(tmp_path: Path) -> tuple[relay.TemporalRelayStore, relay.AppendOnlyEventLog, Path, Path]:
    db_path = tmp_path / "relay.db"
    log_path = tmp_path / "relay-events.jsonl"
    event_log = relay.AppendOnlyEventLog(log_path)
    store = relay.TemporalRelayStore(db_path, event_log)
    return store, event_log, db_path, log_path


def _start_run(store: relay.TemporalRelayStore, run_id: str, now: float = 1000.0) -> None:
    store.start_or_resume_run(
        run_id=run_id,
        idempotency_key=f"idem-{run_id}",
        retry_policy=relay.RetryPolicy(max_attempts=3, backoff_seconds=0),
        lease_policy=relay.LeasePolicy(lease_ttl_seconds=2, heartbeat_late_seconds=3, heartbeat_stale_seconds=4),
        activity_timeout_seconds=30,
        now=now,
    )


def _start_run_custom_lease(
    store: relay.TemporalRelayStore,
    run_id: str,
    *,
    now: float,
    lease_ttl_seconds: int,
    heartbeat_late_seconds: int,
    heartbeat_stale_seconds: int,
) -> None:
    store.start_or_resume_run(
        run_id=run_id,
        idempotency_key=f"idem-{run_id}",
        retry_policy=relay.RetryPolicy(max_attempts=3, backoff_seconds=0),
        lease_policy=relay.LeasePolicy(
            lease_ttl_seconds=lease_ttl_seconds,
            heartbeat_late_seconds=heartbeat_late_seconds,
            heartbeat_stale_seconds=heartbeat_stale_seconds,
        ),
        activity_timeout_seconds=30,
        now=now,
    )


def _start_run_custom_retry(
    store: relay.TemporalRelayStore,
    run_id: str,
    *,
    now: float,
    max_attempts: int,
    backoff_seconds: int = 0,
) -> None:
    store.start_or_resume_run(
        run_id=run_id,
        idempotency_key=f"idem-{run_id}",
        retry_policy=relay.RetryPolicy(max_attempts=max_attempts, backoff_seconds=backoff_seconds),
        lease_policy=relay.LeasePolicy(lease_ttl_seconds=2, heartbeat_late_seconds=3, heartbeat_stale_seconds=4),
        activity_timeout_seconds=30,
        now=now,
    )


def _write_executable_script(path: Path, body: str) -> Path:
    path.write_text(body, encoding="utf-8")
    path.chmod(0o755)
    return path


def test_crash_restart_recovery_resumes_and_completes(tmp_path: Path) -> None:
    store, _event_log, db_path, log_path = _new_store(tmp_path)
    _start_run(store, "run-crash")

    developer_a = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-a")
    lease = developer_a.claim_next(now=1000.0)
    assert lease is not None
    assert lease.unit_id == "developer_run"

    # Simulate process crash: lease is left running without completion.
    watchdog_result = store.watchdog_scan(now=1005.0)
    assert watchdog_result["stalled"] == 1
    assert watchdog_result["requeued"] == 1

    # New process should recover from durable state and finish the unit.
    store_after_restart = relay.TemporalRelayStore(db_path, relay.AppendOnlyEventLog(log_path))
    developer_b = relay.RelayWorker(store_after_restart, role=relay.ROLE_DEVELOPER, worker_id="developer-b")
    recovered_lease = developer_b.claim_next(now=1005.1)
    assert recovered_lease is not None
    assert recovered_lease.unit_id == "developer_run"
    developer_b.complete(recovered_lease, success=True, message="developer done", now=1005.2)

    auditor = relay.RelayWorker(store_after_restart, role=relay.ROLE_AUDITOR, worker_id="auditor-a")
    auditor_lease = auditor.claim_next(now=1005.3)
    assert auditor_lease is not None
    assert auditor_lease.unit_id == "auditor_run"
    auditor.complete(auditor_lease, success=True, message="auditor done", now=1005.4)

    run_state = store_after_restart.run_state("run-crash")
    assert run_state is not None
    assert run_state["status"] == relay.RUN_STATUS_COMPLETED


def test_watchdog_marks_stale_and_requeues(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-stale", now=2000.0)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-stale")
    lease = developer.claim_next(now=2000.0)
    assert lease is not None
    assert developer.heartbeat(lease, now=2001.0)

    # Heartbeat stops; watchdog should emit stall + retry events.
    result = store.watchdog_scan(now=2007.0)
    assert result == {"stalled": 1, "requeued": 1, "failed": 0}

    unit = [row for row in store.list_units(run_id="run-stale") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RETRYABLE

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "stall_detected" in event_types
    assert "retry_scheduled" in event_types
    assert "maintainer_update_required" not in event_types
    assert "maintainer_pause_required" not in event_types


def test_watchdog_emits_maintainer_update_required_for_long_running_unit(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run_custom_lease(
        store,
        "run-maintainer-update",
        now=1000.0,
        lease_ttl_seconds=600,
        heartbeat_late_seconds=900,
        heartbeat_stale_seconds=1200,
    )

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-maintainer")
    lease = developer.claim_next(now=1000.0)
    assert lease is not None

    first = store.watchdog_scan(
        now=1031.0,
        maintainer_heartbeat_seconds=30,
        no_progress_stall_seconds=120,
    )
    assert first == {"stalled": 0, "requeued": 0, "failed": 0}

    second = store.watchdog_scan(
        now=1040.0,
        maintainer_heartbeat_seconds=30,
        no_progress_stall_seconds=120,
    )
    assert second == {"stalled": 0, "requeued": 0, "failed": 0}

    third = store.watchdog_scan(
        now=1062.0,
        maintainer_heartbeat_seconds=30,
        no_progress_stall_seconds=120,
    )
    assert third == {"stalled": 0, "requeued": 0, "failed": 0}

    reminder_events = [
        event for event in event_log.read_events() if event["event_type"] == "maintainer_update_required"
    ]
    assert len(reminder_events) == 2
    assert reminder_events[0]["metadata"]["maintainer_heartbeat_seconds"] == 30

    unit = [row for row in store.list_units(run_id="run-maintainer-update") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RUNNING


def test_watchdog_no_progress_stall_requeues_when_heartbeats_continue(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run_custom_lease(
        store,
        "run-no-progress",
        now=2000.0,
        lease_ttl_seconds=600,
        heartbeat_late_seconds=900,
        heartbeat_stale_seconds=1200,
    )

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-no-progress")
    lease = developer.claim_next(now=2000.0)
    assert lease is not None
    assert developer.heartbeat(lease, now=2090.0)

    result = store.watchdog_scan(
        now=2100.0,
        maintainer_heartbeat_seconds=30,
        no_progress_stall_seconds=60,
    )
    assert result == {"stalled": 1, "requeued": 1, "failed": 0}

    unit = [row for row in store.list_units(run_id="run-no-progress") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RETRYABLE
    assert unit["last_error"] == "watchdog: no_progress"

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "stall_detected_no_progress" in event_types
    assert "retry_scheduled" in event_types


def test_watchdog_terminal_escalation_when_retry_budget_exhausted(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run_custom_retry(store, "run-watchdog-terminal", now=2600.0, max_attempts=1)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-terminal")
    lease = developer.claim_next(now=2600.0)
    assert lease is not None
    assert developer.heartbeat(lease, now=2601.0)

    result = store.watchdog_scan(now=2608.0)
    assert result == {"stalled": 1, "requeued": 0, "failed": 1}

    run_state = store.run_state("run-watchdog-terminal")
    assert run_state is not None
    assert run_state["status"] == relay.RUN_STATUS_FAILED

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "stall_detected" in event_types
    assert "workflow_failed" in event_types


def test_concurrent_contention_allows_single_lease_owner(tmp_path: Path) -> None:
    store, _event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-lease", now=3000.0)

    developer_a = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-a")
    developer_b = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-b")

    lease_a = developer_a.claim_next(now=3000.0)
    lease_b = developer_b.claim_next(now=3000.0)

    assert lease_a is not None
    assert lease_b is None

    unit = [row for row in store.list_units(run_id="run-lease") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RUNNING
    assert unit["lease_owner"] == "developer-a"


def test_event_log_is_append_only_and_dashboard_low_noise(tmp_path: Path) -> None:
    store, event_log, _db_path, log_path = _new_store(tmp_path)
    _start_run(store, "run-log", now=4000.0)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-log")
    lease = developer.claim_next(now=4000.0)
    assert lease is not None
    assert developer.heartbeat(lease, now=4000.2)
    developer.complete(lease, success=True, message="developer ok", now=4000.3)

    lines_before = log_path.read_text(encoding="utf-8").splitlines()
    event_log.append(
        run_id="run-log",
        unit_id="developer_run",
        role=relay.ROLE_DEVELOPER,
        event_type="heartbeat",
        status=relay.UNIT_STATUS_RUNNING,
        message="synthetic heartbeat",
        ts=4000.4,
    )
    lines_after = log_path.read_text(encoding="utf-8").splitlines()

    assert len(lines_after) == len(lines_before) + 1
    assert lines_after[: len(lines_before)] == lines_before

    events = [json.loads(line) for line in lines_after]
    seqs = [event["seq"] for event in events]
    assert seqs == sorted(seqs)
    assert seqs == list(range(1, len(seqs) + 1))

    required = {
        "seq",
        "ts_utc_iso",
        "run_id",
        "unit_id",
        "role",
        "event_type",
        "status",
        "message",
    }
    for event in events:
        assert required.issubset(event)

    dashboard = relay.DashboardReader(event_log, store)
    low_noise = dashboard.render(verbose=False, raw=False, limit=200)
    verbose = dashboard.render(verbose=True, raw=False, limit=200)
    assert "synthetic heartbeat" not in low_noise
    assert "synthetic heartbeat" in verbose


def test_end_to_end_cycle_completes_durably(tmp_path: Path) -> None:
    store, event_log, db_path, log_path = _new_store(tmp_path)
    run_id, state = store.start_or_resume_run(
        run_id="run-e2e",
        idempotency_key="idem-run-e2e",
        retry_policy=relay.RetryPolicy(max_attempts=2, backoff_seconds=0),
        lease_policy=relay.LeasePolicy(lease_ttl_seconds=5, heartbeat_late_seconds=8, heartbeat_stale_seconds=12),
        activity_timeout_seconds=30,
        now=5000.0,
    )
    assert state == "started"

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-e2e")
    assert developer.run_once(lambda _lease: (True, "developer complete"), now=5000.1)

    auditor = relay.RelayWorker(store, role=relay.ROLE_AUDITOR, worker_id="auditor-e2e")
    assert auditor.run_once(lambda _lease: (True, "auditor complete"), now=5000.2)

    run_state = store.run_state(run_id)
    assert run_state is not None
    assert run_state["status"] == relay.RUN_STATUS_COMPLETED

    history_events = [row["event_type"] for row in store.history(run_id)]
    assert history_events.count("workflow_completed") == 1
    assert "unit_queued" in history_events

    # Re-open durable state and verify idempotent resume behavior.
    reopened_store = relay.TemporalRelayStore(db_path, relay.AppendOnlyEventLog(log_path))
    resumed_run_id, resumed_state = reopened_store.start_or_resume_run(
        run_id="run-e2e",
        idempotency_key="idem-run-e2e",
        retry_policy=relay.RetryPolicy(max_attempts=2, backoff_seconds=0),
        lease_policy=relay.LeasePolicy(lease_ttl_seconds=5, heartbeat_late_seconds=8, heartbeat_stale_seconds=12),
        activity_timeout_seconds=30,
        now=5000.3,
    )
    assert resumed_run_id == "run-e2e"
    assert resumed_state == "resumed"

    reader = relay.DashboardReader(event_log, reopened_store)
    rendered = reader.render(verbose=False, raw=False, limit=200)
    assert "workflow_completed" in rendered


def test_preflight_rejects_placeholder_worker_commands() -> None:
    error = relay._preflight_worker_command(["/usr/bin/true"], role=relay.ROLE_DEVELOPER)
    assert error is not None
    assert "placeholder" in error


def test_worker_policy_block_retry_is_recovered_without_maintainer_prompt(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-policy-recover", now=9000.0)

    report_path = tmp_path / "run-policy-recover_developer_run_report.md"
    command = _write_executable_script(
        tmp_path / "policy_retry_worker.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                "if [[ \"${RELAY_REDUCED_PROMPT_PROFILE:-0}\" != \"1\" ]]; then",
                "  echo \"policy blocked: safety policy rejected worker prompt\" >&2",
                "  exit 7",
                "fi",
                f"echo ok > {report_path}",
                f"echo report_handle={report_path}",
            ]
        )
        + "\n",
    )

    rc = relay._run_worker_loop(
        store=store,
        role=relay.ROLE_DEVELOPER,
        worker_id="developer-policy-recover",
        command_text=str(command),
        once=True,
        poll_seconds=1,
        heartbeat_interval=10,
    )
    assert rc == 0

    unit = [row for row in store.list_units(run_id="run-policy-recover") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_SUCCEEDED

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "worker_command_started" in event_types
    assert "worker_command_completed" in event_types
    assert "policy_block_detected" in event_types
    assert "policy_retry_recovered" in event_types
    assert "maintainer_update_required" not in event_types
    assert "maintainer_pause_required" not in event_types
    completion_events = [event for event in event_log.read_events() if event["event_type"] == "worker_command_completed"]
    assert completion_events
    assert completion_events[-1]["metadata"]["report_handle"] == str(report_path)


def test_worker_policy_block_retryable_failure_does_not_prompt_maintainer(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-policy-retryable", now=9050.0)

    command = _write_executable_script(
        tmp_path / "policy_retryable_worker.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                "echo \"policy blocked: safety policy rejected worker prompt\" >&2",
                "exit 7",
            ]
        )
        + "\n",
    )

    rc = relay._run_worker_loop(
        store=store,
        role=relay.ROLE_DEVELOPER,
        worker_id="developer-policy-retryable",
        command_text=str(command),
        once=True,
        poll_seconds=1,
        heartbeat_interval=10,
    )
    assert rc == 0

    unit = [row for row in store.list_units(run_id="run-policy-retryable") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RETRYABLE

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "worker_command_started" in event_types
    assert "worker_command_failed" in event_types
    assert "policy_block_detected" in event_types
    assert "policy_retry_exhausted" in event_types
    assert "retry_scheduled" in event_types
    assert "maintainer_update_required" not in event_types
    assert "maintainer_pause_required" not in event_types


def test_worker_timeout_failure_is_retryable_without_maintainer_prompt(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    store.start_or_resume_run(
        run_id="run-timeout-retry",
        idempotency_key="idem-run-timeout-retry",
        retry_policy=relay.RetryPolicy(max_attempts=3, backoff_seconds=0),
        lease_policy=relay.LeasePolicy(lease_ttl_seconds=2, heartbeat_late_seconds=3, heartbeat_stale_seconds=4),
        activity_timeout_seconds=1,
        now=9060.0,
    )

    command = _write_executable_script(
        tmp_path / "timeout_retry_worker.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                "sleep 2",
            ]
        )
        + "\n",
    )

    rc = relay._run_worker_loop(
        store=store,
        role=relay.ROLE_DEVELOPER,
        worker_id="developer-timeout-retry",
        command_text=str(command),
        once=True,
        poll_seconds=1,
        heartbeat_interval=1,
    )
    assert rc == 0

    unit = [row for row in store.list_units(run_id="run-timeout-retry") if row["unit_id"] == "developer_run"][0]
    assert unit["status"] == relay.UNIT_STATUS_RETRYABLE
    assert "command exceeded timeout (1s)" in unit["last_error"]

    event_types = [event["event_type"] for event in event_log.read_events()]
    assert "retry_scheduled" in event_types
    assert "maintainer_update_required" not in event_types
    assert "maintainer_pause_required" not in event_types


def test_maintainer_pause_gate_allows_only_blocker_and_commit_approval() -> None:
    assert relay._maintainer_pause_allowed(reason="true_blocker_decision", recoverable=False)
    assert relay._maintainer_pause_allowed(reason="commit_message_approval", recoverable=False)
    assert not relay._maintainer_pause_allowed(reason="policy_block_recoverable", recoverable=False)
    assert not relay._maintainer_pause_allowed(reason="true_blocker_decision", recoverable=True)


def test_maintainer_pause_event_emitted_only_for_allowed_terminal_reason(tmp_path: Path) -> None:
    store, event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run_custom_retry(store, "run-pause-allowed", now=9100.0, max_attempts=1)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-pause-allowed")
    lease = developer.claim_next(now=9100.0)
    assert lease is not None
    developer.complete(
        lease,
        success=False,
        message="pause_reason=true_blocker_decision: maintainer decision required",
        now=9100.1,
    )

    pause_events = [event for event in event_log.read_events() if event["event_type"] == "maintainer_pause_required"]
    assert len(pause_events) == 1
    assert pause_events[0]["metadata"]["pause_reason"] == "true_blocker_decision"

    store2, event_log2, _db_path2, _log_path2 = _new_store(tmp_path / "second")
    _start_run_custom_retry(store2, "run-pause-commit", now=9200.0, max_attempts=1)
    developer2 = relay.RelayWorker(store2, role=relay.ROLE_DEVELOPER, worker_id="developer-pause-commit")
    lease2 = developer2.claim_next(now=9200.0)
    assert lease2 is not None
    developer2.complete(
        lease2,
        success=False,
        message="pause_reason=commit_message_approval: commit message approval required",
        now=9200.1,
    )

    commit_pause_events = [event for event in event_log2.read_events() if event["event_type"] == "maintainer_pause_required"]
    assert len(commit_pause_events) == 1
    assert commit_pause_events[0]["metadata"]["pause_reason"] == "commit_message_approval"

    store3, event_log3, _db_path3, _log_path3 = _new_store(tmp_path / "third")
    _start_run_custom_retry(store3, "run-pause-denied", now=9300.0, max_attempts=1)
    developer3 = relay.RelayWorker(store3, role=relay.ROLE_DEVELOPER, worker_id="developer-pause-denied")
    lease3 = developer3.claim_next(now=9300.0)
    assert lease3 is not None
    developer3.complete(
        lease3,
        success=False,
        message="policy-blocked failure after reduced profile retry",
        now=9300.1,
    )

    denied_events = [event for event in event_log3.read_events() if event["event_type"] == "maintainer_pause_required"]
    assert denied_events == []


def test_worker_success_rejects_stale_report_handle(tmp_path: Path) -> None:
    store, _event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-stale-report", now=6000.0)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-stale-report")
    lease = developer.claim_next(now=6000.0)
    assert lease is not None

    report_path = tmp_path / "run-stale-report_developer_run_report.md"
    report_path.write_text("stale\n", encoding="utf-8")
    os.utime(report_path, (5900.0, 5900.0))
    command = _write_executable_script(
        tmp_path / "stale_report.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                f"echo report_handle={report_path}",
            ]
        )
        + "\n",
    )

    success, message = relay._run_command_with_heartbeats(
        worker=developer,
        lease=lease,
        command=[str(command)],
        heartbeat_interval=10,
    )
    assert success is False
    assert "stale report handle" in message


def test_worker_success_rejects_non_namespaced_report_handle(tmp_path: Path) -> None:
    store, _event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-namespaced", now=7000.0)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-namespaced")
    lease = developer.claim_next(now=7000.0)
    assert lease is not None

    report_path = tmp_path / "wrong_report.md"
    command = _write_executable_script(
        tmp_path / "wrong_report.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                f"echo ok > {report_path}",
                f"echo report_handle={report_path}",
            ]
        )
        + "\n",
    )

    success, message = relay._run_command_with_heartbeats(
        worker=developer,
        lease=lease,
        command=[str(command)],
        heartbeat_interval=10,
    )
    assert success is False
    assert "not namespaced for this run/unit" in message


def test_worker_success_accepts_fresh_namespaced_report_handle(tmp_path: Path) -> None:
    store, _event_log, _db_path, _log_path = _new_store(tmp_path)
    _start_run(store, "run-fresh-report", now=8000.0)

    developer = relay.RelayWorker(store, role=relay.ROLE_DEVELOPER, worker_id="developer-fresh-report")
    lease = developer.claim_next(now=8000.0)
    assert lease is not None

    report_path = tmp_path / "run-fresh-report_developer_run_report.md"
    command = _write_executable_script(
        tmp_path / "fresh_report.sh",
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "set -euo pipefail",
                f"echo ok > {report_path}",
                f"echo report_handle={report_path}",
            ]
        )
        + "\n",
    )

    success, message = relay._run_command_with_heartbeats(
        worker=developer,
        lease=lease,
        command=[str(command)],
        heartbeat_interval=10,
    )
    assert success is True
    assert "report_handle=" in message
