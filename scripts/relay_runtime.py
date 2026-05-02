#!/usr/bin/env python3
"""Durable auto-relay runtime with Temporal-style lifecycle orchestration."""

from __future__ import annotations

import argparse
import datetime as dt
import fcntl
import json
import os
import re
import shlex
import signal
import sqlite3
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterable, Sequence

RUN_STATUS_RUNNING = "running"
RUN_STATUS_COMPLETED = "completed"
RUN_STATUS_FAILED = "failed"

UNIT_STATUS_BLOCKED = "blocked"
UNIT_STATUS_QUEUED = "queued"
UNIT_STATUS_RUNNING = "running"
UNIT_STATUS_RETRYABLE = "retryable"
UNIT_STATUS_SUCCEEDED = "succeeded"
UNIT_STATUS_FAILED = "failed"

ROLE_DEVELOPER = "developer"
ROLE_AUDITOR = "code_auditor"
ROLE_ARCHITECT = "architect"
ROLE_SYSTEM = "system"

WORKFLOW_UNITS: tuple[tuple[str, str], ...] = (
    ("architect_handoff", ROLE_ARCHITECT),
    ("developer_run", ROLE_DEVELOPER),
    ("auditor_run", ROLE_AUDITOR),
    ("architect_validation", ROLE_ARCHITECT),
)

AUTO_COMPLETE_UNITS = {"architect_handoff", "architect_validation"}

_NOOP_PLACEHOLDER_COMMANDS: set[tuple[str, ...]] = {
    ("true",),
    ("/usr/bin/true",),
    ("/bin/true",),
}

_REPORT_HANDLE_PATTERN = re.compile(r"(?:^|\s)report_handle=(\S+)")
DEFAULT_MAINTAINER_HEARTBEAT_SECONDS = max(
    30, int(os.environ.get("RELAY_MAINTAINER_HEARTBEAT_SECONDS", "300"))
)
DEFAULT_NO_PROGRESS_STALL_SECONDS = max(
    DEFAULT_MAINTAINER_HEARTBEAT_SECONDS + 30,
    int(
        os.environ.get(
            "RELAY_NO_PROGRESS_STALL_SECONDS",
            str(DEFAULT_MAINTAINER_HEARTBEAT_SECONDS * 3),
        )
    ),
)


def _utc_iso(ts: float | None = None) -> str:
    value = ts if ts is not None else time.time()
    return dt.datetime.fromtimestamp(value, tz=dt.timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )


@dataclass(frozen=True)
class RetryPolicy:
    max_attempts: int = 3
    backoff_seconds: int = 10


@dataclass(frozen=True)
class LeasePolicy:
    lease_ttl_seconds: int = 30
    heartbeat_late_seconds: int = 45
    heartbeat_stale_seconds: int = 90


@dataclass(frozen=True)
class Lease:
    run_id: str
    unit_id: str
    role: str
    lease_owner: str
    lease_token: int
    lease_version: int
    lease_expires_at: float
    heartbeat_at: float
    attempt: int
    max_attempts: int
    activity_timeout_seconds: int


class AppendOnlyEventLog:
    """Strict append-only JSONL event writer with monotonic sequence numbers."""

    def __init__(self, path: Path):
        self.path = path
        self.seq_path = Path(f"{path}.seq")
        self.lock_path = Path(f"{path}.lock")
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.lock_path.parent.mkdir(parents=True, exist_ok=True)

    def _tail_seq_from_log(self) -> int:
        if not self.path.exists():
            return 0
        last_seq = 0
        with self.path.open("r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                parsed = json.loads(line)
                seq = parsed.get("seq")
                if isinstance(seq, int) and seq > last_seq:
                    last_seq = seq
        return last_seq

    def _read_seq(self) -> int:
        if not self.seq_path.exists():
            return 0
        raw = self.seq_path.read_text(encoding="utf-8").strip()
        if not raw:
            return 0
        try:
            return int(raw)
        except ValueError as exc:
            raise RuntimeError(f"invalid sequence counter: {self.seq_path}") from exc

    def append(
        self,
        *,
        run_id: str,
        unit_id: str,
        role: str,
        event_type: str,
        status: str,
        message: str,
        handles: dict[str, Any] | None = None,
        metadata: dict[str, Any] | None = None,
        ts: float | None = None,
    ) -> dict[str, Any]:
        stamp = ts if ts is not None else time.time()
        self.lock_path.touch(exist_ok=True)
        with self.lock_path.open("r+", encoding="utf-8") as lock_fh:
            fcntl.flock(lock_fh.fileno(), fcntl.LOCK_EX)
            seq = max(self._read_seq(), self._tail_seq_from_log()) + 1
            event: dict[str, Any] = {
                "seq": seq,
                "ts_utc_iso": _utc_iso(stamp),
                "run_id": run_id,
                "unit_id": unit_id,
                "role": role,
                "event_type": event_type,
                "status": status,
                "message": message,
            }
            if handles:
                event["handles"] = handles
            if metadata:
                event["metadata"] = metadata

            with self.path.open("a", encoding="utf-8") as out:
                out.write(json.dumps(event, sort_keys=True))
                out.write("\n")
                out.flush()
                os.fsync(out.fileno())
            self.seq_path.write_text(str(seq), encoding="utf-8")
            fcntl.flock(lock_fh.fileno(), fcntl.LOCK_UN)
        return event

    def read_events(self) -> list[dict[str, Any]]:
        if not self.path.exists():
            return []
        events: list[dict[str, Any]] = []
        with self.path.open("r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                events.append(json.loads(line))
        events.sort(key=lambda item: item.get("seq", 0))
        return events


class TemporalRelayStore:
    """Durable store for relay run state, lease CAS, retries, and temporal history."""

    def __init__(self, db_path: Path, event_log: AppendOnlyEventLog):
        self.db_path = db_path
        self.event_log = event_log
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self._init_schema()

    def _conn(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA foreign_keys = ON")
        return conn

    def _init_schema(self) -> None:
        with self._conn() as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS runs (
                    run_id TEXT PRIMARY KEY,
                    idempotency_key TEXT NOT NULL UNIQUE,
                    status TEXT NOT NULL,
                    retry_limit INTEGER NOT NULL,
                    retry_backoff_seconds INTEGER NOT NULL,
                    activity_timeout_seconds INTEGER NOT NULL,
                    lease_ttl_seconds INTEGER NOT NULL,
                    heartbeat_late_seconds INTEGER NOT NULL,
                    heartbeat_stale_seconds INTEGER NOT NULL,
                    terminal_message TEXT,
                    created_at REAL NOT NULL,
                    updated_at REAL NOT NULL
                );

                CREATE TABLE IF NOT EXISTS units (
                    run_id TEXT NOT NULL,
                    unit_id TEXT NOT NULL,
                    role TEXT NOT NULL,
                    status TEXT NOT NULL,
                    attempt INTEGER NOT NULL DEFAULT 0,
                    max_attempts INTEGER NOT NULL,
                    retry_not_before REAL NOT NULL DEFAULT 0,
                    unit_idempotency_key TEXT NOT NULL,
                    lease_owner TEXT,
                    lease_token INTEGER,
                    lease_version INTEGER NOT NULL DEFAULT 0,
                    lease_expires_at REAL,
                    heartbeat_at REAL,
                    last_error TEXT,
                    created_at REAL NOT NULL,
                    updated_at REAL NOT NULL,
                    PRIMARY KEY (run_id, unit_id),
                    FOREIGN KEY(run_id) REFERENCES runs(run_id) ON DELETE CASCADE
                );

                CREATE INDEX IF NOT EXISTS idx_units_claim
                    ON units(role, status, retry_not_before, updated_at);

                CREATE TABLE IF NOT EXISTS temporal_history (
                    history_seq INTEGER PRIMARY KEY AUTOINCREMENT,
                    run_id TEXT NOT NULL,
                    unit_id TEXT NOT NULL,
                    role TEXT NOT NULL,
                    event_type TEXT NOT NULL,
                    status TEXT NOT NULL,
                    message TEXT NOT NULL,
                    handles_json TEXT,
                    metadata_json TEXT,
                    ts_epoch REAL NOT NULL
                );
                """
            )

    def _record_events(self, conn: sqlite3.Connection, events: Sequence[dict[str, Any]]) -> None:
        for event in events:
            conn.execute(
                """
                INSERT INTO temporal_history(
                    run_id, unit_id, role, event_type, status, message, handles_json, metadata_json, ts_epoch
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    event["run_id"],
                    event["unit_id"],
                    event["role"],
                    event["event_type"],
                    event["status"],
                    event["message"],
                    json.dumps(event.get("handles")) if event.get("handles") else None,
                    json.dumps(event.get("metadata")) if event.get("metadata") else None,
                    event.get("ts", time.time()),
                ),
            )

    def _emit_events(self, events: Sequence[dict[str, Any]]) -> None:
        for event in events:
            self.event_log.append(
                run_id=event["run_id"],
                unit_id=event["unit_id"],
                role=event["role"],
                event_type=event["event_type"],
                status=event["status"],
                message=event["message"],
                handles=event.get("handles"),
                metadata=event.get("metadata"),
                ts=event.get("ts"),
            )

    def start_or_resume_run(
        self,
        *,
        run_id: str,
        idempotency_key: str,
        retry_policy: RetryPolicy,
        lease_policy: LeasePolicy,
        activity_timeout_seconds: int,
        now: float | None = None,
    ) -> tuple[str, str]:
        stamp = now if now is not None else time.time()
        events: list[dict[str, Any]] = []
        state = "started"
        with self._conn() as conn:
            row = conn.execute("SELECT * FROM runs WHERE run_id = ?", (run_id,)).fetchone()
            if row:
                if row["idempotency_key"] != idempotency_key:
                    raise ValueError("run_id already exists with different idempotency_key")
                state = "resumed"
                events.append(
                    {
                        "run_id": run_id,
                        "unit_id": "-",
                        "role": ROLE_SYSTEM,
                        "event_type": "workflow_resumed",
                        "status": row["status"],
                        "message": "existing run resumed from durable state",
                        "ts": stamp,
                    }
                )
                self._record_events(conn, events)
            else:
                key_row = conn.execute(
                    "SELECT run_id, status FROM runs WHERE idempotency_key = ?", (idempotency_key,)
                ).fetchone()
                if key_row:
                    state = "resumed"
                    run_id = str(key_row["run_id"])
                    events.append(
                        {
                            "run_id": run_id,
                            "unit_id": "-",
                            "role": ROLE_SYSTEM,
                            "event_type": "workflow_idempotent_reuse",
                            "status": key_row["status"],
                            "message": "idempotency key reused; returning existing run",
                            "metadata": {"idempotency_key": idempotency_key},
                            "ts": stamp,
                        }
                    )
                    self._record_events(conn, events)
                else:
                    conn.execute(
                        """
                        INSERT INTO runs(
                            run_id, idempotency_key, status, retry_limit, retry_backoff_seconds,
                            activity_timeout_seconds, lease_ttl_seconds, heartbeat_late_seconds,
                            heartbeat_stale_seconds, terminal_message, created_at, updated_at
                        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, NULL, ?, ?)
                        """,
                        (
                            run_id,
                            idempotency_key,
                            RUN_STATUS_RUNNING,
                            retry_policy.max_attempts,
                            retry_policy.backoff_seconds,
                            activity_timeout_seconds,
                            lease_policy.lease_ttl_seconds,
                            lease_policy.heartbeat_late_seconds,
                            lease_policy.heartbeat_stale_seconds,
                            stamp,
                            stamp,
                        ),
                    )
                    for unit_id, role in WORKFLOW_UNITS:
                        if unit_id == "architect_handoff":
                            status = UNIT_STATUS_SUCCEEDED
                            attempt = 1
                        elif unit_id == "developer_run":
                            status = UNIT_STATUS_QUEUED
                            attempt = 0
                        else:
                            status = UNIT_STATUS_BLOCKED
                            attempt = 0
                        max_attempts = 1 if unit_id in AUTO_COMPLETE_UNITS else retry_policy.max_attempts
                        conn.execute(
                            """
                            INSERT INTO units(
                                run_id, unit_id, role, status, attempt, max_attempts, retry_not_before,
                                unit_idempotency_key, created_at, updated_at
                            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                            """,
                            (
                                run_id,
                                unit_id,
                                role,
                                status,
                                attempt,
                                max_attempts,
                                stamp,
                                f"{idempotency_key}:{unit_id}",
                                stamp,
                                stamp,
                            ),
                        )
                    events.extend(
                        [
                            {
                                "run_id": run_id,
                                "unit_id": "-",
                                "role": ROLE_SYSTEM,
                                "event_type": "workflow_started",
                                "status": RUN_STATUS_RUNNING,
                                "message": "relay workflow started",
                                "metadata": {"idempotency_key": idempotency_key},
                                "ts": stamp,
                            },
                            {
                                "run_id": run_id,
                                "unit_id": "architect_handoff",
                                "role": ROLE_ARCHITECT,
                                "event_type": "unit_completed",
                                "status": UNIT_STATUS_SUCCEEDED,
                                "message": "architect handoff auto-completed",
                                "ts": stamp,
                            },
                            {
                                "run_id": run_id,
                                "unit_id": "developer_run",
                                "role": ROLE_DEVELOPER,
                                "event_type": "unit_queued",
                                "status": UNIT_STATUS_QUEUED,
                                "message": "developer unit queued",
                                "ts": stamp,
                            },
                        ]
                    )
                    self._record_events(conn, events)

        self._emit_events(events)
        return run_id, state

    def acquire_next_unit(self, *, role: str, lease_owner: str, now: float | None = None) -> Lease | None:
        stamp = now if now is not None else time.time()
        events: list[dict[str, Any]] = []
        lease: Lease | None = None
        with self._conn() as conn:
            candidate = conn.execute(
                """
                SELECT u.*, r.activity_timeout_seconds, r.lease_ttl_seconds
                FROM units u
                INNER JOIN runs r ON r.run_id = u.run_id
                WHERE u.role = ?
                  AND u.status IN (?, ?)
                  AND u.retry_not_before <= ?
                  AND r.status = ?
                ORDER BY u.updated_at ASC
                LIMIT 1
                """,
                (role, UNIT_STATUS_QUEUED, UNIT_STATUS_RETRYABLE, stamp, RUN_STATUS_RUNNING),
            ).fetchone()
            if not candidate:
                return None

            lease_version = int(candidate["lease_version"])
            lease_token = lease_version + 1
            lease_expires_at = stamp + int(candidate["lease_ttl_seconds"])
            updated = conn.execute(
                """
                UPDATE units
                SET status = ?,
                    attempt = attempt + 1,
                    lease_owner = ?,
                    lease_token = ?,
                    lease_version = ?,
                    lease_expires_at = ?,
                    heartbeat_at = ?,
                    updated_at = ?
                WHERE run_id = ?
                  AND unit_id = ?
                  AND lease_version = ?
                  AND status IN (?, ?)
                """,
                (
                    UNIT_STATUS_RUNNING,
                    lease_owner,
                    lease_token,
                    lease_token,
                    lease_expires_at,
                    stamp,
                    stamp,
                    candidate["run_id"],
                    candidate["unit_id"],
                    lease_version,
                    UNIT_STATUS_QUEUED,
                    UNIT_STATUS_RETRYABLE,
                ),
            )
            if updated.rowcount != 1:
                return None

            refreshed = conn.execute(
                """
                SELECT u.*, r.activity_timeout_seconds
                FROM units u
                INNER JOIN runs r ON r.run_id = u.run_id
                WHERE u.run_id = ? AND u.unit_id = ?
                """,
                (candidate["run_id"], candidate["unit_id"]),
            ).fetchone()
            if not refreshed:
                return None

            lease = Lease(
                run_id=str(refreshed["run_id"]),
                unit_id=str(refreshed["unit_id"]),
                role=str(refreshed["role"]),
                lease_owner=str(refreshed["lease_owner"]),
                lease_token=int(refreshed["lease_token"]),
                lease_version=int(refreshed["lease_version"]),
                lease_expires_at=float(refreshed["lease_expires_at"]),
                heartbeat_at=float(refreshed["heartbeat_at"]),
                attempt=int(refreshed["attempt"]),
                max_attempts=int(refreshed["max_attempts"]),
                activity_timeout_seconds=int(refreshed["activity_timeout_seconds"]),
            )
            events.append(
                {
                    "run_id": lease.run_id,
                    "unit_id": lease.unit_id,
                    "role": lease.role,
                    "event_type": "lease_acquired",
                    "status": UNIT_STATUS_RUNNING,
                    "message": "worker acquired lease",
                    "metadata": {
                        "lease_owner": lease_owner,
                        "lease_token": lease.lease_token,
                        "attempt": lease.attempt,
                    },
                    "ts": stamp,
                }
            )
            self._record_events(conn, events)

        self._emit_events(events)
        return lease

    def renew_heartbeat(self, lease: Lease, *, now: float | None = None) -> bool:
        stamp = now if now is not None else time.time()
        events: list[dict[str, Any]] = []
        success = False
        with self._conn() as conn:
            row = conn.execute(
                "SELECT lease_ttl_seconds FROM runs WHERE run_id = ?",
                (lease.run_id,),
            ).fetchone()
            if row:
                expires = stamp + int(row["lease_ttl_seconds"])
                updated = conn.execute(
                    """
                    UPDATE units
                    SET heartbeat_at = ?, lease_expires_at = ?, updated_at = ?
                    WHERE run_id = ?
                      AND unit_id = ?
                      AND status = ?
                      AND lease_owner = ?
                      AND lease_token = ?
                    """,
                    (
                        stamp,
                        expires,
                        stamp,
                        lease.run_id,
                        lease.unit_id,
                        UNIT_STATUS_RUNNING,
                        lease.lease_owner,
                        lease.lease_token,
                    ),
                )
                success = updated.rowcount == 1

            if success:
                events.append(
                    {
                        "run_id": lease.run_id,
                        "unit_id": lease.unit_id,
                        "role": lease.role,
                        "event_type": "heartbeat",
                        "status": UNIT_STATUS_RUNNING,
                        "message": "lease heartbeat renewed",
                        "metadata": {"lease_owner": lease.lease_owner, "lease_token": lease.lease_token},
                        "ts": stamp,
                    }
                )
            else:
                events.append(
                    {
                        "run_id": lease.run_id,
                        "unit_id": lease.unit_id,
                        "role": lease.role,
                        "event_type": "heartbeat_rejected",
                        "status": "error",
                        "message": "heartbeat rejected due to lease mismatch or stale state",
                        "metadata": {"lease_owner": lease.lease_owner, "lease_token": lease.lease_token},
                        "ts": stamp,
                    }
                )
            self._record_events(conn, events)

        self._emit_events(events)
        return success

    def complete_unit(self, lease: Lease, *, success: bool, message: str, now: float | None = None) -> str:
        stamp = now if now is not None else time.time()
        events: list[dict[str, Any]] = []
        final_status = UNIT_STATUS_FAILED
        with self._conn() as conn:
            unit = conn.execute(
                "SELECT * FROM units WHERE run_id = ? AND unit_id = ?",
                (lease.run_id, lease.unit_id),
            ).fetchone()
            if not unit:
                raise RuntimeError("unit not found")
            if (
                unit["status"] != UNIT_STATUS_RUNNING
                or unit["lease_owner"] != lease.lease_owner
                or unit["lease_token"] != lease.lease_token
            ):
                events.append(
                    {
                        "run_id": lease.run_id,
                        "unit_id": lease.unit_id,
                        "role": lease.role,
                        "event_type": "unit_completion_rejected",
                        "status": "error",
                        "message": "completion rejected due to lease mismatch",
                        "metadata": {"lease_owner": lease.lease_owner, "lease_token": lease.lease_token},
                        "ts": stamp,
                    }
                )
                self._record_events(conn, events)
                self._emit_events(events)
                return "rejected"

            if success:
                final_status = UNIT_STATUS_SUCCEEDED
                conn.execute(
                    """
                    UPDATE units
                    SET status = ?,
                        lease_owner = NULL,
                        lease_token = NULL,
                        lease_expires_at = NULL,
                        heartbeat_at = NULL,
                        last_error = NULL,
                        updated_at = ?
                    WHERE run_id = ? AND unit_id = ?
                    """,
                    (UNIT_STATUS_SUCCEEDED, stamp, lease.run_id, lease.unit_id),
                )
                events.append(
                    {
                        "run_id": lease.run_id,
                        "unit_id": lease.unit_id,
                        "role": lease.role,
                        "event_type": "unit_completed",
                        "status": UNIT_STATUS_SUCCEEDED,
                        "message": message,
                        "ts": stamp,
                    }
                )

                if lease.unit_id == "developer_run":
                    conn.execute(
                        """
                        UPDATE units
                        SET status = ?, retry_not_before = ?, updated_at = ?
                        WHERE run_id = ? AND unit_id = ? AND status = ?
                        """,
                        (UNIT_STATUS_QUEUED, stamp, stamp, lease.run_id, "auditor_run", UNIT_STATUS_BLOCKED),
                    )
                    events.append(
                        {
                            "run_id": lease.run_id,
                            "unit_id": "auditor_run",
                            "role": ROLE_AUDITOR,
                            "event_type": "unit_queued",
                            "status": UNIT_STATUS_QUEUED,
                            "message": "auditor unit queued after developer success",
                            "ts": stamp,
                        }
                    )
                elif lease.unit_id == "auditor_run":
                    conn.execute(
                        """
                        UPDATE units
                        SET status = ?, attempt = 1, updated_at = ?
                        WHERE run_id = ? AND unit_id = ? AND status = ?
                        """,
                        (UNIT_STATUS_SUCCEEDED, stamp, lease.run_id, "architect_validation", UNIT_STATUS_BLOCKED),
                    )
                    conn.execute(
                        """
                        UPDATE runs
                        SET status = ?, terminal_message = ?, updated_at = ?
                        WHERE run_id = ?
                        """,
                        (RUN_STATUS_COMPLETED, "architect validation completed", stamp, lease.run_id),
                    )
                    events.append(
                        {
                            "run_id": lease.run_id,
                            "unit_id": "architect_validation",
                            "role": ROLE_ARCHITECT,
                            "event_type": "unit_completed",
                            "status": UNIT_STATUS_SUCCEEDED,
                            "message": "architect validation auto-completed",
                            "ts": stamp,
                        }
                    )
                    events.append(
                        {
                            "run_id": lease.run_id,
                            "unit_id": "-",
                            "role": ROLE_SYSTEM,
                            "event_type": "workflow_completed",
                            "status": RUN_STATUS_COMPLETED,
                            "message": "relay workflow completed",
                            "ts": stamp,
                        }
                    )
            else:
                attempts = int(unit["attempt"])
                max_attempts = int(unit["max_attempts"])
                run_row = conn.execute(
                    "SELECT retry_backoff_seconds FROM runs WHERE run_id = ?",
                    (lease.run_id,),
                ).fetchone()
                backoff = int(run_row["retry_backoff_seconds"]) if run_row else 0
                if attempts < max_attempts:
                    final_status = UNIT_STATUS_RETRYABLE
                    retry_at = stamp + backoff * attempts
                    conn.execute(
                        """
                        UPDATE units
                        SET status = ?,
                            retry_not_before = ?,
                            lease_owner = NULL,
                            lease_token = NULL,
                            lease_expires_at = NULL,
                            heartbeat_at = NULL,
                            last_error = ?,
                            updated_at = ?
                        WHERE run_id = ? AND unit_id = ?
                        """,
                        (UNIT_STATUS_RETRYABLE, retry_at, message, stamp, lease.run_id, lease.unit_id),
                    )
                    events.extend(
                        [
                            {
                                "run_id": lease.run_id,
                                "unit_id": lease.unit_id,
                                "role": lease.role,
                                "event_type": "unit_failed",
                                "status": UNIT_STATUS_RETRYABLE,
                                "message": message,
                                "metadata": {"attempt": attempts, "max_attempts": max_attempts},
                                "ts": stamp,
                            },
                            {
                                "run_id": lease.run_id,
                                "unit_id": lease.unit_id,
                                "role": lease.role,
                                "event_type": "retry_scheduled",
                                "status": UNIT_STATUS_RETRYABLE,
                                "message": "watchdog/worker retry scheduled",
                                "metadata": {"retry_not_before": _utc_iso(retry_at)},
                                "ts": stamp,
                            },
                        ]
                    )
                else:
                    final_status = UNIT_STATUS_FAILED
                    conn.execute(
                        """
                        UPDATE units
                        SET status = ?,
                            lease_owner = NULL,
                            lease_token = NULL,
                            lease_expires_at = NULL,
                            heartbeat_at = NULL,
                            last_error = ?,
                            updated_at = ?
                        WHERE run_id = ? AND unit_id = ?
                        """,
                        (UNIT_STATUS_FAILED, message, stamp, lease.run_id, lease.unit_id),
                    )
                    conn.execute(
                        """
                        UPDATE runs
                        SET status = ?, terminal_message = ?, updated_at = ?
                        WHERE run_id = ?
                        """,
                        (RUN_STATUS_FAILED, message, stamp, lease.run_id),
                    )
                    events.extend(
                        [
                            {
                                "run_id": lease.run_id,
                                "unit_id": lease.unit_id,
                                "role": lease.role,
                                "event_type": "unit_failed",
                                "status": UNIT_STATUS_FAILED,
                                "message": message,
                                "metadata": {"attempt": attempts, "max_attempts": max_attempts},
                                "ts": stamp,
                            },
                            {
                                "run_id": lease.run_id,
                                "unit_id": "-",
                                "role": ROLE_SYSTEM,
                                "event_type": "workflow_failed",
                                "status": RUN_STATUS_FAILED,
                                "message": "relay workflow reached terminal failure",
                                "ts": stamp,
                            },
                        ]
                    )

            self._record_events(conn, events)

        self._emit_events(events)
        return final_status

    def watchdog_scan(
        self,
        *,
        now: float | None = None,
        maintainer_heartbeat_seconds: int = DEFAULT_MAINTAINER_HEARTBEAT_SECONDS,
        no_progress_stall_seconds: int = DEFAULT_NO_PROGRESS_STALL_SECONDS,
    ) -> dict[str, int]:
        stamp = now if now is not None else time.time()
        maintainer_interval = max(30, int(maintainer_heartbeat_seconds))
        progress_stall_interval = max(maintainer_interval + 30, int(no_progress_stall_seconds))
        stalled = 0
        requeued = 0
        failed = 0
        events: list[dict[str, Any]] = []

        with self._conn() as conn:
            candidates = conn.execute(
                """
                SELECT u.*, r.retry_backoff_seconds, r.heartbeat_stale_seconds
                FROM units u
                INNER JOIN runs r ON r.run_id = u.run_id
                WHERE u.status = ?
                  AND r.status = ?
                """,
                (UNIT_STATUS_RUNNING, RUN_STATUS_RUNNING),
            ).fetchall()

            for row in candidates:
                run_id = str(row["run_id"])
                unit_id = str(row["unit_id"])
                role = str(row["role"])
                heartbeat_at = float(row["heartbeat_at"]) if row["heartbeat_at"] is not None else None
                lease_expired = row["lease_expires_at"] is not None and float(row["lease_expires_at"]) <= stamp
                heartbeat_stale = heartbeat_at is not None and heartbeat_at <= stamp - int(
                    row["heartbeat_stale_seconds"]
                )

                progress_row = conn.execute(
                    """
                    SELECT MAX(ts_epoch) AS ts_epoch
                    FROM temporal_history
                    WHERE run_id = ?
                      AND unit_id = ?
                      AND event_type != ?
                    """,
                    (run_id, unit_id, "heartbeat"),
                ).fetchone()
                last_progress_epoch = (
                    float(progress_row["ts_epoch"])
                    if progress_row and progress_row["ts_epoch"] is not None
                    else stamp
                )
                progress_age = max(0.0, stamp - last_progress_epoch)
                no_progress_stall = progress_age >= float(progress_stall_interval)

                stale_reason = None
                stall_event_type = "stall_detected"
                stall_message = "watchdog detected stale lease"
                if heartbeat_stale:
                    stale_reason = "heartbeat_stale"
                elif lease_expired:
                    stale_reason = "lease_expired"
                elif no_progress_stall:
                    stale_reason = "no_progress"
                    stall_event_type = "stall_detected_no_progress"
                    stall_message = "watchdog detected no-progress stall"

                if stale_reason is None:
                    if progress_age >= float(maintainer_interval):
                        reminder_row = conn.execute(
                            """
                            SELECT MAX(ts_epoch) AS ts_epoch
                            FROM temporal_history
                            WHERE run_id = ?
                              AND unit_id = ?
                              AND event_type = ?
                            """,
                            (run_id, unit_id, "maintainer_update_required"),
                        ).fetchone()
                        last_reminder_epoch = (
                            float(reminder_row["ts_epoch"])
                            if reminder_row and reminder_row["ts_epoch"] is not None
                            else None
                        )
                        reminder_due = (
                            last_reminder_epoch is None
                            or (stamp - last_reminder_epoch) >= float(maintainer_interval)
                        )
                        if reminder_due:
                            events.append(
                                {
                                    "run_id": run_id,
                                    "unit_id": unit_id,
                                    "role": role,
                                    "event_type": "maintainer_update_required",
                                    "status": UNIT_STATUS_RUNNING,
                                    "message": "unit still running; maintainer update required",
                                    "metadata": {
                                        "progress_age_s": round(progress_age, 1),
                                        "heartbeat_age_s": round(stamp - heartbeat_at, 1)
                                        if heartbeat_at is not None
                                        else None,
                                        "maintainer_heartbeat_seconds": maintainer_interval,
                                    },
                                    "ts": stamp,
                                }
                            )
                    continue

                stalled += 1

                attempts = int(row["attempt"])
                max_attempts = int(row["max_attempts"])
                base_event = {
                    "run_id": run_id,
                    "unit_id": unit_id,
                    "role": role,
                    "event_type": stall_event_type,
                    "status": "stalled",
                    "message": stall_message,
                    "metadata": {
                        "stale_reason": stale_reason,
                        "lease_owner": row["lease_owner"],
                        "lease_token": row["lease_token"],
                        "progress_age_s": round(progress_age, 1),
                    },
                    "ts": stamp,
                }

                if attempts < max_attempts:
                    retry_at = stamp + int(row["retry_backoff_seconds"]) * max(attempts, 1)
                    updated = conn.execute(
                        """
                        UPDATE units
                        SET status = ?,
                            retry_not_before = ?,
                            lease_owner = NULL,
                            lease_token = NULL,
                            lease_expires_at = NULL,
                            heartbeat_at = NULL,
                            last_error = ?,
                            updated_at = ?
                        WHERE run_id = ?
                          AND unit_id = ?
                          AND status = ?
                          AND lease_version = ?
                        """,
                        (
                            UNIT_STATUS_RETRYABLE,
                            retry_at,
                            f"watchdog: {stale_reason}",
                            stamp,
                            row["run_id"],
                            row["unit_id"],
                            UNIT_STATUS_RUNNING,
                            row["lease_version"],
                        ),
                    )
                    if updated.rowcount == 1:
                        requeued += 1
                        events.append(base_event)
                        events.append(
                            {
                                "run_id": run_id,
                                "unit_id": unit_id,
                                "role": role,
                                "event_type": "retry_scheduled",
                                "status": UNIT_STATUS_RETRYABLE,
                                "message": "watchdog requeued stalled unit",
                                "metadata": {"retry_not_before": _utc_iso(retry_at)},
                                "ts": stamp,
                            }
                        )
                else:
                    updated = conn.execute(
                        """
                        UPDATE units
                        SET status = ?,
                            lease_owner = NULL,
                            lease_token = NULL,
                            lease_expires_at = NULL,
                            heartbeat_at = NULL,
                            last_error = ?,
                            updated_at = ?
                        WHERE run_id = ?
                          AND unit_id = ?
                          AND status = ?
                          AND lease_version = ?
                        """,
                        (
                            UNIT_STATUS_FAILED,
                            f"watchdog terminal: {stale_reason}",
                            stamp,
                            row["run_id"],
                            row["unit_id"],
                            UNIT_STATUS_RUNNING,
                            row["lease_version"],
                        ),
                    )
                    if updated.rowcount == 1:
                        failed += 1
                        conn.execute(
                            """
                            UPDATE runs
                            SET status = ?, terminal_message = ?, updated_at = ?
                            WHERE run_id = ?
                            """,
                            (
                                RUN_STATUS_FAILED,
                                f"watchdog terminal failure for {row['unit_id']}",
                                stamp,
                                row["run_id"],
                            ),
                        )
                        events.append(base_event)
                        events.append(
                            {
                                "run_id": run_id,
                                "unit_id": "-",
                                "role": ROLE_SYSTEM,
                                "event_type": "workflow_failed",
                                "status": RUN_STATUS_FAILED,
                                "message": "watchdog moved run to terminal failure",
                                "ts": stamp,
                            }
                        )

            if events:
                self._record_events(conn, events)

        if events:
            self._emit_events(events)

        return {"stalled": stalled, "requeued": requeued, "failed": failed}

    def list_units(self, *, run_id: str | None = None) -> list[sqlite3.Row]:
        query = "SELECT * FROM units"
        args: tuple[Any, ...] = ()
        if run_id:
            query += " WHERE run_id = ?"
            args = (run_id,)
        query += " ORDER BY run_id, unit_id"
        with self._conn() as conn:
            return conn.execute(query, args).fetchall()

    def run_state(self, run_id: str) -> sqlite3.Row | None:
        with self._conn() as conn:
            return conn.execute("SELECT * FROM runs WHERE run_id = ?", (run_id,)).fetchone()

    def history(self, run_id: str) -> list[sqlite3.Row]:
        with self._conn() as conn:
            return conn.execute(
                "SELECT * FROM temporal_history WHERE run_id = ? ORDER BY history_seq ASC",
                (run_id,),
            ).fetchall()

    def heartbeat_snapshot(self, *, now: float | None = None) -> list[dict[str, Any]]:
        stamp = now if now is not None else time.time()
        rows: list[dict[str, Any]] = []
        with self._conn() as conn:
            data = conn.execute(
                """
                SELECT u.run_id, u.unit_id, u.role, u.status, u.heartbeat_at,
                       r.lease_ttl_seconds, r.heartbeat_late_seconds, r.heartbeat_stale_seconds
                FROM units u
                INNER JOIN runs r ON r.run_id = u.run_id
                WHERE u.status = ?
                ORDER BY u.run_id, u.unit_id
                """,
                (UNIT_STATUS_RUNNING,),
            ).fetchall()
        for row in data:
            hb = float(row["heartbeat_at"]) if row["heartbeat_at"] is not None else None
            freshness = "UNKNOWN"
            age = None
            if hb is not None:
                age = max(0.0, stamp - hb)
                if age <= int(row["lease_ttl_seconds"]):
                    freshness = "OK"
                elif age <= int(row["heartbeat_late_seconds"]):
                    freshness = "LATE"
                else:
                    freshness = "STALE"
            rows.append(
                {
                    "run_id": row["run_id"],
                    "unit_id": row["unit_id"],
                    "role": row["role"],
                    "status": row["status"],
                    "heartbeat_age_s": age,
                    "freshness": freshness,
                }
            )
        return rows


class RelayWorker:
    """Worker that acquires lease, renews heartbeat, and executes one unit."""

    def __init__(self, store: TemporalRelayStore, *, role: str, worker_id: str):
        self.store = store
        self.role = role
        self.worker_id = worker_id

    def claim_next(self, *, now: float | None = None) -> Lease | None:
        return self.store.acquire_next_unit(role=self.role, lease_owner=self.worker_id, now=now)

    def heartbeat(self, lease: Lease, *, now: float | None = None) -> bool:
        return self.store.renew_heartbeat(lease, now=now)

    def complete(self, lease: Lease, *, success: bool, message: str, now: float | None = None) -> str:
        return self.store.complete_unit(lease, success=success, message=message, now=now)

    def run_once(
        self,
        executor: Callable[[Lease], tuple[bool, str]],
        *,
        now: float | None = None,
    ) -> bool:
        lease = self.claim_next(now=now)
        if not lease:
            return False
        try:
            success, message = executor(lease)
        except Exception as exc:  # noqa: BLE001 - failures must be explicit and persisted.
            success = False
            message = f"executor exception: {exc}"
        self.complete(lease, success=success, message=message, now=now)
        return True


class DashboardReader:
    def __init__(self, event_log: AppendOnlyEventLog, store: TemporalRelayStore):
        self.event_log = event_log
        self.store = store

    def render(self, *, verbose: bool = False, raw: bool = False, limit: int = 50) -> str:
        events = self.event_log.read_events()
        if not verbose:
            events = [event for event in events if event.get("event_type") != "heartbeat"]
        if limit > 0:
            events = events[-limit:]

        if raw:
            return "\n".join(json.dumps(event, sort_keys=True) for event in events)

        lines: list[str] = []
        for event in events:
            lines.append(
                "[{ts}] seq={seq} run={run} unit={unit} role={role} {etype} {status}: {message}".format(
                    ts=event.get("ts_utc_iso", "-"),
                    seq=event.get("seq", "-"),
                    run=event.get("run_id", "-"),
                    unit=event.get("unit_id", "-"),
                    role=event.get("role", "-"),
                    etype=event.get("event_type", "-"),
                    status=event.get("status", "-"),
                    message=event.get("message", ""),
                )
            )

        freshness = self.store.heartbeat_snapshot()
        if freshness:
            lines.append("-- heartbeat freshness --")
            for item in freshness:
                age = "-" if item["heartbeat_age_s"] is None else f"{item['heartbeat_age_s']:.1f}s"
                lines.append(
                    f"run={item['run_id']} unit={item['unit_id']} role={item['role']} "
                    f"status={item['status']} heartbeat_age={age} freshness={item['freshness']}"
                )

        return "\n".join(lines)


def _parse_command(command_text: str) -> list[str]:
    parts = shlex.split(command_text)
    if not parts:
        raise ValueError("worker command is empty")
    return parts


def _preflight_worker_command(command: Sequence[str], *, role: str) -> str | None:
    if not command:
        return f"worker command for role {role} is empty"

    normalized = tuple(command)
    if normalized in _NOOP_PLACEHOLDER_COMMANDS:
        return f"worker command for role {role} is placeholder no-op ({command[0]})"

    executable = command[0]
    if "/" in executable:
        exe_path = Path(executable).expanduser()
        if not exe_path.exists():
            return f"worker command executable not found for role {role}: {executable}"
        if not exe_path.is_file():
            return f"worker command executable is not a file for role {role}: {executable}"
        if not os.access(exe_path, os.X_OK):
            return f"worker command executable is not executable for role {role}: {executable}"

    return None


def _preflight_configured_worker_commands() -> list[str]:
    errors: list[str] = []
    for role in (ROLE_DEVELOPER, ROLE_AUDITOR):
        resolved_command = _default_worker_command(role)
        if not resolved_command:
            errors.append(f"missing command for role {role}")
            continue
        try:
            parsed = _parse_command(resolved_command)
        except ValueError as exc:
            errors.append(f"invalid command for role {role}: {exc}")
            continue
        validation_error = _preflight_worker_command(parsed, role=role)
        if validation_error:
            errors.append(validation_error)
    return errors


def _extract_report_handle(command_output: str) -> str | None:
    matches = _REPORT_HANDLE_PATTERN.findall(command_output)
    if not matches:
        return None
    return matches[-1]


def _validate_worker_success_output(*, lease: Lease, command_output: str) -> tuple[bool, str]:
    report_handle = _extract_report_handle(command_output)
    if not report_handle:
        return False, "worker command succeeded but did not emit required report_handle=... output"

    if lease.run_id not in report_handle or lease.unit_id not in report_handle:
        return (
            False,
            "worker command emitted report_handle that is not namespaced for this run/unit "
            f"(run_id={lease.run_id}, unit_id={lease.unit_id})",
        )

    report_path = Path(report_handle).expanduser()
    if not report_path.exists():
        return False, f"worker command emitted missing report handle path: {report_handle}"
    if not report_path.is_file():
        return False, f"worker command emitted non-file report handle path: {report_handle}"

    # Filesystems may have 1-second mtime granularity; tolerate sub-second rounding.
    stale_cutoff = lease.heartbeat_at - 1.0
    report_mtime = report_path.stat().st_mtime
    if report_mtime < stale_cutoff:
        return (
            False,
            f"worker command emitted stale report handle path: {report_handle} "
            f"(mtime={_utc_iso(report_mtime)} < lease_start={_utc_iso(lease.heartbeat_at)})",
        )

    return True, report_handle


def _run_command_with_heartbeats(
    *,
    worker: RelayWorker,
    lease: Lease,
    command: Sequence[str],
    heartbeat_interval: int,
) -> tuple[bool, str]:
    stop = threading.Event()
    heartbeat_failed = threading.Event()

    def _heartbeat_loop() -> None:
        while not stop.wait(timeout=heartbeat_interval):
            if not worker.heartbeat(lease):
                heartbeat_failed.set()
                return

    heartbeat_thread = threading.Thread(target=_heartbeat_loop, name=f"hb-{worker.role}", daemon=True)
    heartbeat_thread.start()

    try:
        proc = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=lease.activity_timeout_seconds,
            check=False,
        )
        if heartbeat_failed.is_set():
            return False, "heartbeat renewal failed while command was running"
        if proc.returncode == 0:
            stdout = proc.stdout.strip()
            stderr = proc.stderr.strip()
            combined_output = "\n".join(part for part in (stdout, stderr) if part)
            output_is_valid, output_message = _validate_worker_success_output(
                lease=lease, command_output=combined_output
            )
            if not output_is_valid:
                return False, output_message
            return True, (stdout or stderr or "command completed")[:400]
        stderr = proc.stderr.strip() or proc.stdout.strip() or f"exit={proc.returncode}"
        return False, f"command failed: {stderr[:400]}"
    except subprocess.TimeoutExpired:
        return False, f"command exceeded timeout ({lease.activity_timeout_seconds}s)"
    except Exception as exc:  # noqa: BLE001
        return False, f"worker command exception: {exc}"
    finally:
        stop.set()
        heartbeat_thread.join(timeout=heartbeat_interval + 1)


def _default_worker_command(role: str) -> str | None:
    if role == ROLE_DEVELOPER:
        return os.environ.get("RELAY_DEVELOPER_CMD")
    if role == ROLE_AUDITOR:
        return os.environ.get("RELAY_CODE_AUDITOR_CMD")
    return None


def _run_worker_loop(
    *,
    store: TemporalRelayStore,
    role: str,
    worker_id: str,
    command_text: str | None,
    once: bool,
    poll_seconds: int,
    heartbeat_interval: int,
) -> int:
    worker = RelayWorker(store, role=role, worker_id=worker_id)
    resolved_command = command_text or _default_worker_command(role)

    if not resolved_command:
        store.event_log.append(
            run_id="-",
            unit_id="-",
            role=role,
            event_type="worker_misconfigured",
            status="error",
            message=f"missing command for role {role}",
        )
        return 2

    try:
        parsed_command = _parse_command(resolved_command)
    except ValueError as exc:
        store.event_log.append(
            run_id="-",
            unit_id="-",
            role=role,
            event_type="worker_misconfigured",
            status="error",
            message=str(exc),
        )
        return 2
    command_preflight_error = _preflight_worker_command(parsed_command, role=role)
    if command_preflight_error:
        store.event_log.append(
            run_id="-",
            unit_id="-",
            role=role,
            event_type="worker_misconfigured",
            status="error",
            message=command_preflight_error,
        )
        return 2

    while True:
        lease = worker.claim_next()
        if not lease:
            if once:
                return 0
            time.sleep(poll_seconds)
            continue

        success, message = _run_command_with_heartbeats(
            worker=worker,
            lease=lease,
            command=parsed_command,
            heartbeat_interval=heartbeat_interval,
        )
        worker.complete(lease, success=success, message=message)

        if once:
            return 0


def _print_health(store: TemporalRelayStore) -> int:
    snapshots = store.heartbeat_snapshot()
    units = store.list_units()
    by_role: dict[str, dict[str, int]] = {}
    for row in units:
        role = str(row["role"])
        by_role.setdefault(role, {})
        status = str(row["status"])
        by_role[role][status] = by_role[role].get(status, 0) + 1

    print("relay health")
    for role, counts in sorted(by_role.items()):
        summary = ", ".join(f"{key}={value}" for key, value in sorted(counts.items()))
        print(f"- role={role}: {summary}")

    if snapshots:
        print("heartbeat freshness")
        for item in snapshots:
            age = "-" if item["heartbeat_age_s"] is None else f"{item['heartbeat_age_s']:.1f}s"
            print(
                f"- run={item['run_id']} unit={item['unit_id']} role={item['role']} "
                f"freshness={item['freshness']} age={age}"
            )
    return 0


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Durable auto-relay runtime")
    parser.add_argument(
        "--db",
        default=os.environ.get("RELAY_DB", str(Path.home() / ".local" / "state" / "ytree" / "relay.db")),
        help="SQLite path for durable relay state",
    )
    parser.add_argument(
        "--events",
        default=os.environ.get(
            "RELAY_EVENT_LOG",
            str(Path.home() / ".local" / "state" / "ytree" / "relay-events.jsonl"),
        ),
        help="Append-only event log JSONL path",
    )

    sub = parser.add_subparsers(dest="command", required=True)

    start = sub.add_parser("start-run", help="Create or resume a durable relay workflow run")
    start.add_argument("--run-id", required=True)
    start.add_argument("--idempotency-key", required=True)
    start.add_argument("--retry-limit", type=int, default=3)
    start.add_argument("--retry-backoff", type=int, default=10)
    start.add_argument("--activity-timeout", type=int, default=120)
    start.add_argument("--lease-ttl", type=int, default=30)
    start.add_argument("--heartbeat-late", type=int, default=45)
    start.add_argument("--heartbeat-stale", type=int, default=90)

    worker = sub.add_parser("worker", help="Run developer/code_auditor worker")
    worker.add_argument("--role", choices=[ROLE_DEVELOPER, ROLE_AUDITOR], required=True)
    worker.add_argument("--worker-id", default=f"{os.uname().nodename}-{os.getpid()}")
    worker.add_argument("--command-text", default=None)
    worker.add_argument("--once", action="store_true")
    worker.add_argument("--poll-seconds", type=int, default=2)
    worker.add_argument("--heartbeat-interval", type=int, default=10)

    watchdog = sub.add_parser("watchdog", help="Detect stale leases and requeue/fail units")
    watchdog.add_argument("--once", action="store_true")
    watchdog.add_argument("--poll-seconds", type=int, default=5)
    watchdog.add_argument(
        "--maintainer-heartbeat-seconds",
        type=int,
        default=DEFAULT_MAINTAINER_HEARTBEAT_SECONDS,
        help=(
            "Emit maintainer_update_required when unit progress is silent for this interval "
            "(minimum 30s)"
        ),
    )
    watchdog.add_argument(
        "--no-progress-stall-seconds",
        type=int,
        default=DEFAULT_NO_PROGRESS_STALL_SECONDS,
        help=(
            "Treat a running unit as stalled when no non-heartbeat progress arrives for this interval "
            "(minimum maintainer heartbeat + 30s)"
        ),
    )

    dash = sub.add_parser("dashboard", help="Render low-noise dashboard view")
    dash.add_argument("--verbose", action="store_true")
    dash.add_argument("--raw", action="store_true")
    dash.add_argument("--limit", type=int, default=50)

    history = sub.add_parser("history", help="Print durable temporal history for a run")
    history.add_argument("--run-id", required=True)

    health = sub.add_parser("health", help="Lightweight healthcheck for operators")
    health.add_argument("--fail-on-stale", action="store_true")

    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    event_log = AppendOnlyEventLog(Path(args.events))
    store = TemporalRelayStore(Path(args.db), event_log)

    if args.command == "start-run":
        if args.retry_limit < 1:
            raise SystemExit("--retry-limit must be >= 1")
        if args.activity_timeout < 1:
            raise SystemExit("--activity-timeout must be >= 1")
        preflight_errors = _preflight_configured_worker_commands()
        if preflight_errors:
            for error in preflight_errors:
                print(f"preflight_error={error}", file=sys.stderr)
            raise SystemExit("worker preflight failed")
        run_id, state = store.start_or_resume_run(
            run_id=args.run_id,
            idempotency_key=args.idempotency_key,
            retry_policy=RetryPolicy(max_attempts=args.retry_limit, backoff_seconds=args.retry_backoff),
            lease_policy=LeasePolicy(
                lease_ttl_seconds=args.lease_ttl,
                heartbeat_late_seconds=args.heartbeat_late,
                heartbeat_stale_seconds=args.heartbeat_stale,
            ),
            activity_timeout_seconds=args.activity_timeout,
        )
        print(f"run_id={run_id} state={state}")
        return 0

    if args.command == "worker":
        return _run_worker_loop(
            store=store,
            role=args.role,
            worker_id=args.worker_id,
            command_text=args.command_text,
            once=args.once,
            poll_seconds=args.poll_seconds,
            heartbeat_interval=args.heartbeat_interval,
        )

    if args.command == "watchdog":
        while True:
            result = store.watchdog_scan(
                maintainer_heartbeat_seconds=args.maintainer_heartbeat_seconds,
                no_progress_stall_seconds=args.no_progress_stall_seconds,
            )
            print(
                f"watchdog stalled={result['stalled']} requeued={result['requeued']} failed={result['failed']}"
            )
            if args.once:
                return 0
            time.sleep(args.poll_seconds)

    if args.command == "dashboard":
        reader = DashboardReader(event_log, store)
        print(reader.render(verbose=args.verbose, raw=args.raw, limit=args.limit))
        return 0

    if args.command == "history":
        rows = store.history(args.run_id)
        for row in rows:
            payload = {
                "history_seq": row["history_seq"],
                "run_id": row["run_id"],
                "unit_id": row["unit_id"],
                "role": row["role"],
                "event_type": row["event_type"],
                "status": row["status"],
                "message": row["message"],
                "ts_utc_iso": _utc_iso(float(row["ts_epoch"])),
            }
            print(json.dumps(payload, sort_keys=True))
        return 0

    if args.command == "health":
        rc = _print_health(store)
        if args.fail_on_stale:
            stale = [item for item in store.heartbeat_snapshot() if item["freshness"] == "STALE"]
            if stale:
                return 3
        return rc

    return 1


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    raise SystemExit(main())
