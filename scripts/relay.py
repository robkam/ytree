#!/usr/bin/env python3
"""Python-first relay supervisor entrypoint."""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path
from typing import Any

from relay_backend_codex import CodexBackend
from relay_backend_fake import FakeBackend
from relay_backends import RelayBackend
from relay_core import (
    RelayEventLog,
    build_run_context,
    cleanup_stale_runtime,
    latest_run_dir,
    load_metadata,
    read_events,
    resolve_action_needed,
    resolve_settings,
    resolve_work_item,
)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Python-first relay supervisor")
    sub = parser.add_subparsers(dest="command", required=True)

    run = sub.add_parser("run", help="Run one relay work item")
    target = run.add_mutually_exclusive_group(required=True)
    target.add_argument("--bug", type=int, help="Bug id from docs/BUGS.md")
    target.add_argument("--task", type=int, help="Task id from docs/ROADMAP.md")
    run.add_argument("--backend", help="Backend adapter (codex|fake)")
    run.add_argument("--runtime-dir", help="Runtime scratch root (default: /tmp/ytree-relay)")
    run.add_argument("--stale-seconds", type=int, help="Cleanup threshold for stale runtime paths")
    run.add_argument("--codex-command", nargs="+", help="Override codex command tokens")
    run.add_argument("--codex-timeout-seconds", type=int, help="Codex command timeout")
    run.add_argument(
        "--fake-require-action",
        action="store_true",
        help="Fake backend emits/then resolves ACTION NEEDED event",
    )

    monitor = sub.add_parser("monitor", help="Monitor one relay run")
    monitor.add_argument("--run-id", help="Run id to monitor (default: latest)")
    monitor.add_argument("--run", dest="run_id_alias", help="Compatibility alias for --run")
    monitor.add_argument("--runtime-dir", help="Runtime scratch root (default: /tmp/ytree-relay)")
    monitor.add_argument("--view", choices=("quiet", "normal", "verbose"), default="quiet")
    monitor.add_argument("--limit", type=int, default=20)
    monitor.add_argument("--interval", type=float, default=3.0)
    monitor.add_argument("--once", action="store_true")

    health = sub.add_parser("health", help="Show relay runtime health")
    health.add_argument("--runtime-dir", help="Runtime scratch root (default: /tmp/ytree-relay)")



    return parser


def _make_backend(name: str, *, codex_command: tuple[str, ...], codex_timeout_seconds: int, fake_require_action: bool) -> RelayBackend:
    if name == "fake":
        return FakeBackend(require_action=fake_require_action)
    if name == "codex":
        return CodexBackend(command=codex_command, timeout_seconds=codex_timeout_seconds)
    raise ValueError(f"Unsupported backend: {name}")


def _emit(event_log: RelayEventLog, event_type: str, message: str, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
    return event_log.append(event_type, message, metadata=metadata)


def _runtime_overrides(args: argparse.Namespace) -> dict[str, Any]:
    return {
        "backend": args.backend,
        "runtime_dir": args.runtime_dir,
        "stale_seconds": args.stale_seconds,
        "codex_command": args.codex_command,
        "codex_timeout_seconds": args.codex_timeout_seconds,
    }


def _command_run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parent.parent
    settings = resolve_settings(repo_root=repo_root, cli_overrides=_runtime_overrides(args))
    cleanup_stale_runtime(settings.runtime_dir, settings.stale_seconds)

    kind = "bug" if args.bug is not None else "task"
    numeric_id = args.bug if args.bug is not None else args.task

    try:
        work_item = resolve_work_item(repo_root, kind=kind, numeric_id=numeric_id)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    context = build_run_context(repo_root, settings, work_item)
    event_log = RelayEventLog(context.events_path)
    _emit(event_log, "run_started", f"{work_item.label} started", {"run_id": context.run_id, "backend": settings.backend})


    backend = _make_backend(
        settings.backend,
        codex_command=settings.codex_command,
        codex_timeout_seconds=settings.codex_timeout_seconds,
        fake_require_action=bool(getattr(args, "fake_require_action", False)),
    )
    result = backend.run(context, lambda event_type, message, metadata=None: _emit(event_log, event_type, message, metadata))

    if result.status == "completed":
        _emit(event_log, "run_completed", result.summary, {"run_id": context.run_id})
        print(f"RUN ID: {context.run_id}")
        print(f"WORK ITEM: {work_item.label}: {work_item.title}")
        print(f"BACKEND: {backend.name}")
        print(f"RUNTIME DIR: {settings.runtime_dir}")
        print(f"PROMPT: {context.prompt_path}")
        print(f"REPORT: {context.report_path}")
        print("ACTION NEEDED (maintainer): none")
        print("STATUS: completed")
        return 0

    _emit(event_log, "run_failed", result.summary, {"run_id": context.run_id})
    action = result.action_needed or "none"
    print(f"RUN ID: {context.run_id}")
    print(f"WORK ITEM: {work_item.label}: {work_item.title}")
    print(f"BACKEND: {backend.name}")
    print(f"RUNTIME DIR: {settings.runtime_dir}")
    print(f"ACTION NEEDED (maintainer): {action}")
    print(f"STATUS: failed ({result.summary})")
    return 1


def _render_snapshot(run_dir: Path, *, view: str, limit: int) -> tuple[str, int]:
    metadata = load_metadata(run_dir / "metadata.json")
    events = read_events(run_dir / "events.jsonl")
    run_id = metadata.get("run_id", run_dir.name)
    work = metadata.get("work_item", {})
    label = f"{work.get('kind', '').upper()}-{work.get('numeric_id', '-')}: {work.get('title', '')}".strip(": ")
    latest = events[-1] if events else {}

    action = resolve_action_needed(events)
    lines = [
        f"RUN: {run_id}",
        f"WORK ITEM: {label if label else 'unknown'}",
        f"LATEST: seq={latest.get('seq', '-')} event={latest.get('event_type', '-')} msg={latest.get('message', '')}",
        f"ACTION NEEDED (maintainer): {action}",
    ]
    if view != "quiet":
        rows = events[-limit:]
        lines.append("RECENT EVENTS:")
        for row in rows:
            lines.append(
                f"- seq={row.get('seq', '-')} event={row.get('event_type', '-')} msg={row.get('message', '')}"
            )

    status = 0
    if latest.get("event_type") == "run_failed":
        status = 1
    return "\n".join(lines), status


def _command_monitor(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parent.parent
    settings = resolve_settings(repo_root=repo_root, cli_overrides={"runtime_dir": args.runtime_dir})
    run_id = args.run_id or args.run_id_alias

    def resolve_run() -> Path | None:
        if run_id:
            candidate = settings.runtime_dir / "runs" / run_id
            return candidate if candidate.exists() else None
        return latest_run_dir(settings.runtime_dir)

    run_dir = resolve_run()
    if run_dir is None:
        print("ERROR: no relay runs found", file=sys.stderr)
        return 1

    while True:
        snapshot, status = _render_snapshot(run_dir, view=args.view, limit=args.limit)
        print(snapshot)
        if args.once:
            return status
        time.sleep(max(0.5, args.interval))


def _command_health(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parent.parent
    settings = resolve_settings(repo_root=repo_root, cli_overrides={"runtime_dir": args.runtime_dir})
    run_dir = latest_run_dir(settings.runtime_dir)
    if run_dir is None:
        print("relay health")
        print(f"runtime_dir={settings.runtime_dir}")
        print("runs=0")
        print("latest_run=none")
        return 0

    snapshot, status = _render_snapshot(run_dir, view="quiet", limit=10)
    print("relay health")
    print(f"runtime_dir={settings.runtime_dir}")
    print(snapshot)
    return status


def main(argv: list[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    if args.command == "run":
        return _command_run(args)
    if args.command == "monitor":
        return _command_monitor(args)
    if args.command == "health":
        return _command_health(args)
    parser.error(f"unknown command: {args.command}")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
