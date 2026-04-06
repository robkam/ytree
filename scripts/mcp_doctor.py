#!/usr/bin/env python3
"""Validate and optionally harden Codex MCP server config."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tomllib
from typing import Any


DEFAULT_CACHE_DIR = "/tmp/codex-uv-cache"
DEFAULT_TOOL_DIR = "/tmp/codex-uv-tools"
REPO_ROOT = Path(__file__).resolve().parent.parent
PROJECT_CONFIG_PATH = REPO_ROOT / ".codex" / "config.toml"
CONFIG_PATH = Path.home() / ".codex" / "config.toml"
SERVERS = ("serena", "jcodemunch")


def _read_config(path: Path) -> tuple[str, dict[str, Any]]:
    text = path.read_text(encoding="utf-8")
    data = tomllib.loads(text)
    return text, data


def _bootstrap_user_config() -> tuple[bool, str]:
    if not PROJECT_CONFIG_PATH.exists():
        return False, f"missing project template: {PROJECT_CONFIG_PATH}"
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(PROJECT_CONFIG_PATH, CONFIG_PATH)
    return True, f"bootstrapped {CONFIG_PATH} from {PROJECT_CONFIG_PATH}"


def _server_cfg(data: dict[str, Any], name: str) -> dict[str, Any]:
    return data.get("mcp_servers", {}).get(name, {})


def _has_env_section(raw_text: str, name: str) -> bool:
    pattern = rf"(?m)^\[mcp_servers\.{re.escape(name)}\.env\]\s*$"
    return re.search(pattern, raw_text) is not None


def _append_env_section(raw_text: str, name: str) -> str:
    block = (
        f"\n[mcp_servers.{name}.env]\n"
        f'UV_CACHE_DIR = "{DEFAULT_CACHE_DIR}"\n'
        f'UV_TOOL_DIR = "{DEFAULT_TOOL_DIR}"\n'
    )
    return raw_text.rstrip() + "\n" + block


def _ensure_dir(path: str) -> tuple[bool, str]:
    try:
        os.makedirs(path, exist_ok=True)
        writable = os.access(path, os.W_OK | os.X_OK)
        return writable, ""
    except OSError as exc:
        return False, str(exc)


def _normalize_args(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, list):
        return [str(v) for v in value]
    return [str(value)]


def _has_nonportable_home_path(cfg: dict[str, Any]) -> str | None:
    command = str(cfg.get("command", "")).strip()
    args = _normalize_args(cfg.get("args"))
    for token in [command] + args:
        if "/home/" in token or token.startswith("~/"):
            return token
    return None


def _run_smoke(name: str, cfg: dict[str, Any]) -> tuple[str, str]:
    command = str(cfg.get("command", "")).strip()
    if not command:
        return "fail", f"{name}: missing command"

    args = _normalize_args(cfg.get("args"))
    env_cfg = cfg.get("env", {}) if isinstance(cfg.get("env"), dict) else {}
    env = os.environ.copy()
    env.update({k: str(v) for k, v in env_cfg.items()})

    probe = [command] + args + ["--help"]
    try:
        res = subprocess.run(
            probe,
            env=env,
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
    except FileNotFoundError:
        return "fail", f"{name}: command not found: {command}"
    except subprocess.TimeoutExpired:
        return "warn", f"{name}: help probe timed out (tool may still be healthy)"

    output = (res.stdout or "") + (res.stderr or "")
    lower = output.lower()
    if res.returncode == 0:
        return "ok", f"{name}: startup probe succeeded"
    if "permission denied" in lower and (
        "/.cache/uv" in lower or "/.local/share/uv" in lower
    ):
        return "fail", f"{name}: uv path permission issue detected"
    if "failed to fetch" in lower or "dns error" in lower or "git operation failed" in lower:
        return "warn", f"{name}: package fetch failed (network/offline/cache issue)"
    return "warn", f"{name}: probe failed with exit {res.returncode}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Check and harden Codex MCP config.")
    parser.add_argument("--fix", action="store_true", help="Append missing UV env blocks")
    args = parser.parse_args()

    if not CONFIG_PATH.exists():
        if not args.fix:
            print(
                f"FAIL: missing config: {CONFIG_PATH} "
                f"(run with --fix to bootstrap from {PROJECT_CONFIG_PATH})"
            )
            return 1
        ok, message = _bootstrap_user_config()
        if not ok:
            print(f"FAIL: {message}")
            return 1
        print(f"FIXED: {message}")

    raw_text, data = _read_config(CONFIG_PATH)
    failures = 0
    warnings = 0
    changed = False

    for name in SERVERS:
        cfg = _server_cfg(data, name)
        if not cfg:
            print(f"FAIL: mcp server '{name}' missing in {CONFIG_PATH}")
            failures += 1
            continue
        print(f"OK: mcp server '{name}' present")

        if not _has_env_section(raw_text, name):
            if args.fix:
                raw_text = _append_env_section(raw_text, name)
                changed = True
                print(
                    f"FIXED: added [mcp_servers.{name}.env] with "
                    f"UV_CACHE_DIR={DEFAULT_CACHE_DIR} and UV_TOOL_DIR={DEFAULT_TOOL_DIR}"
                )
            else:
                print(
                    f"FAIL: missing [mcp_servers.{name}.env] section "
                    "(run with --fix)"
                )
                failures += 1

    if changed:
        backup = CONFIG_PATH.with_name(CONFIG_PATH.name + ".bak-mcp-doctor")
        shutil.copy2(CONFIG_PATH, backup)
        CONFIG_PATH.write_text(raw_text, encoding="utf-8")
        data = tomllib.loads(raw_text)
        print(f"OK: wrote updated config and backup at {backup}")

    for name in SERVERS:
        cfg = _server_cfg(data, name)
        if not cfg:
            continue
        nonportable = _has_nonportable_home_path(cfg)
        if nonportable:
            print(
                f"WARN: {name} config references a home path ({nonportable}). "
                "This is allowed for personal overrides, but shared defaults must stay in "
                f"{PROJECT_CONFIG_PATH}."
            )
            warnings += 1
        env_cfg = cfg.get("env", {}) if isinstance(cfg.get("env"), dict) else {}
        for key in ("UV_CACHE_DIR", "UV_TOOL_DIR"):
            path = env_cfg.get(key)
            if not isinstance(path, str) or not path:
                print(f"FAIL: {name} env missing {key}")
                failures += 1
                continue
            ok, err = _ensure_dir(path)
            if ok:
                print(f"OK: {name} {key} writable at {path}")
            else:
                print(f"FAIL: {name} {key} not writable at {path}: {err}")
                failures += 1

        status, message = _run_smoke(name, cfg)
        if status == "ok":
            print(f"OK: {message}")
        elif status == "warn":
            print(f"WARN: {message}")
            warnings += 1
        else:
            print(f"FAIL: {message}")
            failures += 1

    if failures:
        print(f"RESULT: FAIL ({failures} issues, {warnings} warnings)")
        return 1
    print(f"RESULT: OK ({warnings} warnings)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
