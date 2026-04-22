#!/usr/bin/env python3
"""Validate repo-owned AI defaults for portability and completeness."""

from __future__ import annotations

import sys
import tomllib
from pathlib import Path
from typing import Any

CONFIG_PATH = Path(__file__).resolve().parent.parent / ".codex" / "config.toml"
REPO_ROOT = CONFIG_PATH.parent.parent
MODEL_INSTRUCTIONS_RELATIVE = ".ai/codex.md"
MODEL_INSTRUCTIONS_ABSOLUTE = str((REPO_ROOT / MODEL_INSTRUCTIONS_RELATIVE).resolve())
SERVERS = ("serena", "jcodemunch")
REQUIRED_ENV_KEYS = ("UV_CACHE_DIR", "UV_TOOL_DIR")


def _normalize_args(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, list):
        return [str(v) for v in value]
    return [str(value)]


def _has_home_path(token: str) -> bool:
    return "/home/" in token or token.startswith("~/")


def check_config(path: Path) -> list[str]:
    failures: list[str] = []
    if not path.exists():
        return [f"missing config: {path}"]

    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        return [f"failed to read/parse {path}: {exc}"]

    model_instructions = data.get("model_instructions_file")
    if model_instructions not in (MODEL_INSTRUCTIONS_RELATIVE, MODEL_INSTRUCTIONS_ABSOLUTE):
        failures.append(
            "model_instructions_file must target '.ai/codex.md' (relative or canonical absolute path)"
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


def main() -> int:
    failures = check_config(CONFIG_PATH)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: project AI config guard failed ({len(failures)} issue(s))")
        return 1
    print("PASS: project AI config guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
