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
RELAY_DEFAULTS_FILE = "scripts/relay.defaults.toml"
REQUIRED_RELAY_DEFAULTS = {
    "backend": "codex",
    "runtime_dir": "/tmp/ytree-relay",
    "stale_seconds": 86400,
    "codex_timeout_seconds": 3600,
}
REQUIRED_RELAY_CODEX_COMMAND = ["codex", "exec", "--prompt-file", "{prompt_path}"]
RELAY_RUNTIME_MARKER_REQUIREMENTS = {
    "scripts/relay.py": (
        'sub.add_parser("run"',
        "--bug",
        "--task",
        "resolve_work_item",
        "_command_monitor",
        "_command_health",
    ),
    "scripts/relay_core.py": (
        'Path("/tmp")',
        "policy_block_retry_once",
        "watchdog_stall_retry_terminal",
        "maintainer_pause_gate=true_blocker_decision|commit_message_approval",
        "def resolve_settings",
        "def cleanup_stale_runtime",
    ),
    "scripts/relay_backends.py": (
        "class RelayBackend(Protocol)",
        "class BackendResult",
    ),
    "scripts/relay_backend_codex.py": (
        "class CodexBackend",
        "subprocess.run(",
        "action_needed",
    ),
}


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

    defaults_path = repo_root / RELAY_DEFAULTS_FILE
    if not defaults_path.exists():
        failures.append(f"missing relay defaults file: {RELAY_DEFAULTS_FILE}")
        return failures

    try:
        defaults_data = tomllib.loads(defaults_path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        failures.append(f"failed to read/parse {RELAY_DEFAULTS_FILE}: {exc}")
        return failures

    for key, expected in REQUIRED_RELAY_DEFAULTS.items():
        value = defaults_data.get(key)
        if value is None:
            failures.append(f"{RELAY_DEFAULTS_FILE}: missing setting {key}")
            continue
        if value != expected:
            failures.append(f"{RELAY_DEFAULTS_FILE}: expected {key}={expected}")

    codex_command = defaults_data.get("codex_command")
    if codex_command != REQUIRED_RELAY_CODEX_COMMAND:
        failures.append(
            f"{RELAY_DEFAULTS_FILE}: expected codex_command={REQUIRED_RELAY_CODEX_COMMAND}"
        )

    for relative_path, markers in RELAY_RUNTIME_MARKER_REQUIREMENTS.items():
        runtime_path = repo_root / relative_path
        if not runtime_path.exists():
            failures.append(f"missing relay runtime file: {relative_path}")
            continue
        runtime_content = runtime_path.read_text(encoding="utf-8")
        for marker in markers:
            if marker not in runtime_content:
                failures.append(f"{relative_path}: missing runtime marker '{marker}'")
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
