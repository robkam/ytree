#!/usr/bin/env python3
"""Validate repo-owned AI defaults for portability and completeness."""

from __future__ import annotations

import sys
import tomllib
from pathlib import Path
from typing import Any

CONFIG_PATH = Path(__file__).resolve().parent.parent / ".codex" / "config.toml"
REPO_ROOT = Path(__file__).resolve().parent.parent
MODEL_INSTRUCTIONS_RELATIVE = ".ai/codex.md"
SERVERS = ("serena", "jcodemunch")
REQUIRED_ENV_KEYS = ("UV_CACHE_DIR", "UV_TOOL_DIR")
RELAY_POLICY_FILES = (
    "docs/ai/WORKFLOW.md",
    "docs/ai/PROMPT_TEMPLATE.md",
    ".ai/shared.md",
)
RELAY_POLICY_TOKENS = (
    "policy_block_retry_once",
    "watchdog_stall_retry_terminal",
    "maintainer_pause_gate=true_blocker_decision|commit_message_approval",
)
RELAY_ENV_TEMPLATE = "infra/systemd/relay.env.example"
REQUIRED_RELAY_ENV_SETTINGS = {
    "RELAY_POLICY_BLOCK_RETRY_LIMIT": "1",
    "RELAY_MAINTAINER_HEARTBEAT_SECONDS": None,
    "RELAY_NO_PROGRESS_STALL_SECONDS": None,
    "RELAY_MAINTAINER_INTERRUPT_REASONS": "true_blocker_decision,commit_message_approval",
}
RELAY_RUNTIME_FILE = "scripts/relay_runtime.py"
RELAY_RUNTIME_REQUIRED_MARKERS = (
    "--retry-limit",
    "--heartbeat-stale",
    "--maintainer-heartbeat-seconds",
    "--no-progress-stall-seconds",
    "POLICY_BLOCK_RETRY_LIMIT",
    "_is_policy_block_failure",
    "policy_block_detected",
    "policy_retry_recovered",
    "policy_retry_exhausted",
    "RELAY_REDUCED_PROMPT_PROFILE",
    "STRICT_MAINTAINER_INTERRUPT_REASONS",
    "_maintainer_pause_allowed",
    "maintainer_pause_required",
    "stall_detected",
    "retry_scheduled",
    "workflow_failed",
)


def _normalize_args(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, list):
        return [str(v) for v in value]
    return [str(value)]


def _has_home_path(token: str) -> bool:
    return "/home/" in token or token.startswith("~/")


def _is_valid_model_instructions_file(value: Any) -> bool:
    if not isinstance(value, str) or not value:
        return False
    if value == MODEL_INSTRUCTIONS_RELATIVE:
        return True
    path = Path(value)
    return path.is_absolute() and path.parts[-2:] == (".ai", "codex.md")


def check_config(path: Path) -> list[str]:
    failures: list[str] = []
    if not path.exists():
        return [f"missing config: {path}"]

    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        return [f"failed to read/parse {path}: {exc}"]

    if not _is_valid_model_instructions_file(data.get("model_instructions_file")):
        failures.append(
            "model_instructions_file must target '.ai/codex.md' (relative or absolute path)"
        )

    mcp_servers = data.get("mcp_servers", {})
    if not isinstance(mcp_servers, dict):
        return ["missing or invalid [mcp_servers] section"]

    for name in SERVERS:
        cfg = mcp_servers.get(name)
        if not isinstance(cfg, dict):
            failures.append(f"missing [mcp_servers.{name}]")
            continue

        command = str(cfg.get("command", "")).strip()
        if not command:
            failures.append(f"{name}: missing command")
            continue

        args = _normalize_args(cfg.get("args"))
        for token in [command] + args:
            if _has_home_path(token):
                failures.append(f"{name}: non-portable home path detected: {token}")

        env = cfg.get("env", {})
        if not isinstance(env, dict):
            failures.append(f"{name}: missing [mcp_servers.{name}.env]")
            continue
        for key in REQUIRED_ENV_KEYS:
            value = env.get(key)
            if not isinstance(value, str) or not value:
                failures.append(f"{name}: missing env key {key}")

    return failures


def _parse_env_lines(path: Path) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        parsed[key.strip()] = value.strip()
    return parsed


def check_relay_policy_requirements(repo_root: Path) -> list[str]:
    failures: list[str] = []
    for relative_path in RELAY_POLICY_FILES:
        policy_path = repo_root / relative_path
        if not policy_path.exists():
            failures.append(f"missing relay policy doc: {relative_path}")
            continue
        content = policy_path.read_text(encoding="utf-8")
        for token in RELAY_POLICY_TOKENS:
            if token not in content:
                failures.append(f"{relative_path}: missing relay policy token '{token}'")

    env_path = repo_root / RELAY_ENV_TEMPLATE
    if not env_path.exists():
        failures.append(f"missing relay env template: {RELAY_ENV_TEMPLATE}")
        return failures

    env_values = _parse_env_lines(env_path)
    for key, expected in REQUIRED_RELAY_ENV_SETTINGS.items():
        value = env_values.get(key)
        if value is None or value == "":
            failures.append(f"{RELAY_ENV_TEMPLATE}: missing setting {key}")
            continue
        if expected is not None and value != expected:
            failures.append(f"{RELAY_ENV_TEMPLATE}: expected {key}={expected}")
        if key in {"RELAY_MAINTAINER_HEARTBEAT_SECONDS", "RELAY_NO_PROGRESS_STALL_SECONDS"}:
            try:
                if int(value) < 1:
                    failures.append(f"{RELAY_ENV_TEMPLATE}: {key} must be >= 1")
            except ValueError:
                failures.append(f"{RELAY_ENV_TEMPLATE}: {key} must be an integer")

    runtime_path = repo_root / RELAY_RUNTIME_FILE
    if not runtime_path.exists():
        failures.append(f"missing relay runtime file: {RELAY_RUNTIME_FILE}")
        return failures
    runtime_content = runtime_path.read_text(encoding="utf-8")
    for marker in RELAY_RUNTIME_REQUIRED_MARKERS:
        if marker not in runtime_content:
            failures.append(f"{RELAY_RUNTIME_FILE}: missing runtime marker '{marker}'")
    return failures


def main() -> int:
    failures = check_config(CONFIG_PATH)
    failures.extend(check_relay_policy_requirements(REPO_ROOT))
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: project AI config guard failed ({len(failures)} issue(s))")
        return 1
    print("PASS: project AI config guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
