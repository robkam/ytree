#!/usr/bin/env python3
"""Core orchestration helpers for Python-first relay runtime."""

from __future__ import annotations

import json
import os
import re
import shutil
import tempfile
import time
import tomllib
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Mapping

BUGS_PATH = Path("docs/BUGS.md")
ROADMAP_PATH = Path("docs/ROADMAP.md")
REPO_DEFAULTS_PATH = Path("scripts/relay.defaults.toml")
USER_CONFIG_PATH = Path("~/.config/ytree/relay.toml")


@dataclass(frozen=True)
class WorkItem:
    kind: str
    numeric_id: int
    title: str
    source_path: Path

    @property
    def label(self) -> str:
        if self.kind == "bug":
            return f"BUG-{self.numeric_id}"
        return f"Task {self.numeric_id}"


@dataclass(frozen=True)
class RelaySettings:
    backend: str
    runtime_dir: Path
    stale_seconds: int
    codex_command: tuple[str, ...]
    codex_timeout_seconds: int


@dataclass(frozen=True)
class RunContext:
    repo_root: Path
    settings: RelaySettings
    work_item: WorkItem
    run_id: str
    run_dir: Path
    prompt_path: Path
    report_path: Path
    events_path: Path
    metadata_path: Path


class RelayEventLog:
    """Append-only JSONL event writer for one run."""

    def __init__(self, path: Path):
        self._path = path

    def append(self, event_type: str, message: str, *, metadata: Mapping[str, Any] | None = None) -> dict[str, Any]:
        seq = 1
        if self._path.exists():
            with self._path.open("r", encoding="utf-8") as handle:
                seq = sum(1 for _ in handle) + 1
        event = {
            "seq": seq,
            "ts_unix": time.time(),
            "event_type": event_type,
            "message": message,
            "metadata": dict(metadata or {}),
        }
        with self._path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(event, sort_keys=True) + "\n")
        return event


def _read_toml(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    return tomllib.loads(path.read_text(encoding="utf-8"))


def _default_runtime_dir() -> Path:
    tmp_root = Path("/tmp")
    if tmp_root.exists():
        return tmp_root / "ytree-relay"
    return Path(tempfile.gettempdir()) / "ytree-relay"


def _normalize_command(value: Any) -> tuple[str, ...]:
    if isinstance(value, str):
        stripped = value.strip()
        return (stripped,) if stripped else ()
    if isinstance(value, (list, tuple)):
        normalized = [str(item).strip() for item in value if str(item).strip()]
        return tuple(normalized)
    return ()


def _normalize_int(value: Any, fallback: int) -> int:
    try:
        parsed = int(str(value))
    except (TypeError, ValueError):
        return fallback
    return parsed if parsed > 0 else fallback


def resolve_settings(
    *,
    repo_root: Path,
    cli_overrides: Mapping[str, Any],
    env: Mapping[str, str] | None = None,
    user_config_path: Path | None = None,
    repo_defaults_path: Path | None = None,
) -> RelaySettings:
    env_map = dict(os.environ if env is None else env)
    repo_defaults_file = repo_root / (repo_defaults_path or REPO_DEFAULTS_PATH)
    user_file = (user_config_path or Path(USER_CONFIG_PATH)).expanduser()

    defaults: dict[str, Any] = {
        "backend": "codex",
        "runtime_dir": str(_default_runtime_dir()),
        "stale_seconds": 24 * 60 * 60,
        "codex_command": ["codex", "exec", "--prompt-file", "{prompt_path}"],
        "codex_timeout_seconds": 60 * 60,
    }
    defaults.update(_read_toml(repo_defaults_file))
    defaults.update(_read_toml(user_file))

    if env_map.get("RELAY_BACKEND"):
        defaults["backend"] = env_map["RELAY_BACKEND"]
    if env_map.get("RELAY_RUNTIME_DIR"):
        defaults["runtime_dir"] = env_map["RELAY_RUNTIME_DIR"]
    if env_map.get("RELAY_STALE_SECONDS"):
        defaults["stale_seconds"] = env_map["RELAY_STALE_SECONDS"]
    if env_map.get("RELAY_CODEX_TIMEOUT_SECONDS"):
        defaults["codex_timeout_seconds"] = env_map["RELAY_CODEX_TIMEOUT_SECONDS"]
    if env_map.get("RELAY_CODEX_COMMAND"):
        defaults["codex_command"] = env_map["RELAY_CODEX_COMMAND"]

    for key, value in cli_overrides.items():
        if value is None:
            continue
        defaults[key] = value

    runtime_dir = Path(str(defaults["runtime_dir"])).expanduser()
    return RelaySettings(
        backend=str(defaults["backend"]).strip().lower(),
        runtime_dir=runtime_dir,
        stale_seconds=_normalize_int(defaults["stale_seconds"], 24 * 60 * 60),
        codex_command=_normalize_command(defaults.get("codex_command")),
        codex_timeout_seconds=_normalize_int(defaults.get("codex_timeout_seconds"), 60 * 60),
    )


def resolve_work_item(repo_root: Path, *, kind: str, numeric_id: int) -> WorkItem:
    if numeric_id <= 0:
        raise ValueError(f"{kind} id must be a positive integer")

    if kind == "bug":
        path = repo_root / BUGS_PATH
        title = _find_title(path, rf"^###\s+\*\*BUG-{numeric_id}:\s*(.*?)\*\*$")
        if title is None:
            raise ValueError(f"Unknown bug id BUG-{numeric_id} in {BUGS_PATH}")
        return WorkItem(kind="bug", numeric_id=numeric_id, title=title, source_path=BUGS_PATH)

    if kind == "task":
        path = repo_root / ROADMAP_PATH
        title = _find_title(path, rf"^#{{3,4}}\s+\*\*Task\s+{numeric_id}:\s*(.*?)\*\*$")
        if title is None:
            raise ValueError(f"Unknown task id {numeric_id} in {ROADMAP_PATH}")
        return WorkItem(kind="task", numeric_id=numeric_id, title=title, source_path=ROADMAP_PATH)

    raise ValueError(f"Unsupported work-item kind: {kind}")


def _find_title(path: Path, pattern: str) -> str | None:
    if not path.exists():
        raise ValueError(f"Missing work-item source document: {path}")
    matcher = re.compile(pattern)
    for line in path.read_text(encoding="utf-8").splitlines():
        match = matcher.match(line.strip())
        if match:
            return match.group(1).strip()
    return None


def cleanup_stale_runtime(runtime_dir: Path, stale_seconds: int, *, now: float | None = None) -> list[Path]:
    now_ts = time.time() if now is None else now
    cutoff = now_ts - stale_seconds
    removed: list[Path] = []
    if not runtime_dir.exists():
        return removed

    for child in runtime_dir.iterdir():
        try:
            modified = child.stat().st_mtime
        except FileNotFoundError:
            continue
        if modified >= cutoff:
            continue
        if child.is_dir():
            shutil.rmtree(child, ignore_errors=True)
        else:
            child.unlink(missing_ok=True)
        removed.append(child)
    return removed


def build_run_context(repo_root: Path, settings: RelaySettings, work_item: WorkItem) -> RunContext:
    run_id = _build_run_id(work_item)
    run_dir = settings.runtime_dir / "runs" / run_id
    run_dir.mkdir(parents=True, exist_ok=True)
    prompt_path = run_dir / "prompt.txt"
    report_path = run_dir / "report.md"
    events_path = run_dir / "events.jsonl"
    metadata_path = run_dir / "metadata.json"

    context = RunContext(
        repo_root=repo_root,
        settings=settings,
        work_item=work_item,
        run_id=run_id,
        run_dir=run_dir,
        prompt_path=prompt_path,
        report_path=report_path,
        events_path=events_path,
        metadata_path=metadata_path,
    )
    _write_prompt(context)
    _write_metadata(context)
    return context


def _build_run_id(work_item: WorkItem) -> str:
    ts = time.strftime("%Y%m%d%H%M%S", time.gmtime())
    title_slug = re.sub(r"[^a-z0-9]+", "-", work_item.title.lower()).strip("-")
    title_slug = title_slug[:40] if title_slug else "untitled"
    return f"{work_item.kind}-{work_item.numeric_id}-{title_slug}-{ts}"


def _write_prompt(context: RunContext) -> None:
    policy_tokens = (
        "policy_block_retry_once",
        "watchdog_stall_retry_terminal",
        "maintainer_pause_gate=true_blocker_decision|commit_message_approval",
    )
    prompt = "\n".join(
        [
            "You are executing one ytree relay work item.",
            f"Work item: {context.work_item.label}",
            f"Title: {context.work_item.title}",
            f"Source: {context.work_item.source_path}",
            "Constraints:",
            "- Keep scope limited to the single requested work item.",
            "- Preserve existing AI governance and strict QA policy checks.",
            "- Maintainer interruption only for true blockers or commit-message approval.",
            "Relay policy tokens:",
            *[f"- {token}" for token in policy_tokens],
            "Output requirement:",
            f"- Write final report to: {context.report_path}",
        ]
    )
    context.prompt_path.write_text(prompt + "\n", encoding="utf-8")


def _write_metadata(context: RunContext) -> None:
    payload = {
        "run_id": context.run_id,
        "work_item": {
            "kind": context.work_item.kind,
            "numeric_id": context.work_item.numeric_id,
            "title": context.work_item.title,
            "source_path": str(context.work_item.source_path),
        },
        "backend": context.settings.backend,
        "runtime_dir": str(context.settings.runtime_dir),
        "prompt_path": str(context.prompt_path),
        "report_path": str(context.report_path),
        "created_unix": time.time(),
    }
    context.metadata_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def read_events(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    rows: list[dict[str, Any]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    return rows


def list_run_dirs(runtime_dir: Path) -> list[Path]:
    runs_dir = runtime_dir / "runs"
    if not runs_dir.exists():
        return []
    return sorted((path for path in runs_dir.iterdir() if path.is_dir()), key=lambda p: p.stat().st_mtime)


def latest_run_dir(runtime_dir: Path) -> Path | None:
    runs = list_run_dirs(runtime_dir)
    return runs[-1] if runs else None


def resolve_action_needed(events: list[dict[str, Any]]) -> str:
    resolution_events = {
        "action_resolved",
        "backend_completed",
        "run_completed",
        "run_failed",
    }
    for event in reversed(events):
        event_type = str(event.get("event_type", "")).strip()
        if event_type in resolution_events:
            return "none"
        if event_type == "action_needed":
            message = str(event.get("message", "")).strip()
            return message or "maintainer update required"
    return "none"


def load_metadata(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}
