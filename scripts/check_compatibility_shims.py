#!/usr/bin/env python3
"""Reject untracked or expired temporary compatibility shims."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parent.parent
REGISTRY_PATH = Path("registry/compatibility_shims.json")
ROADMAP_PATH = Path("docs/ROADMAP.md")
MARKER_RE = re.compile(
    r"YTNOVA_COMPAT_SHIM:\s*id=(?P<id>[^\s]+)\s+"
    r"owner=(?P<owner>\d+(?:\.\d+)*)\s+removal=(?P<removal>[^\s]+)"
)
TASK_RE = re.compile(r"^#+\s+\*{0,2}Task\s+(?P<id>\d+(?:\.\d+)*):", re.MULTILINE)
COMPATIBILITY_SHIM_RE = re.compile(r"\bcompatibility shim\b", re.IGNORECASE)
REQUIRED_FIELDS = {"id", "file", "symbol", "owner_task", "removal_condition"}


def iter_source_paths(root: Path) -> Iterable[Path]:
    for pattern in ("src/**/*.c", "include/**/*.h"):
        yield from sorted(root.glob(pattern))


def load_registry(root: Path) -> tuple[dict[str, dict[str, str]], list[str]]:
    path = root / REGISTRY_PATH
    if not path.is_file():
        return {}, [f"missing compatibility shim inventory: {REGISTRY_PATH}"]
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        return {}, [f"invalid compatibility shim inventory: {error}"]
    entries = data.get("shims") if isinstance(data, dict) else None
    if not isinstance(entries, list):
        return {}, ["compatibility shim inventory must contain a shims list"]

    failures: list[str] = []
    registry: dict[str, dict[str, str]] = {}
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict) or set(entry) != REQUIRED_FIELDS:
            failures.append(f"shim inventory entry {index} must contain {sorted(REQUIRED_FIELDS)}")
            continue
        if not all(isinstance(value, str) and value for value in entry.values()):
            failures.append(f"shim inventory entry {index} has an empty required field")
            continue
        shim_id = entry["id"]
        if shim_id in registry:
            failures.append(f"duplicate shim inventory id: {shim_id}")
            continue
        registry[shim_id] = entry
    return registry, failures


def completed_tasks(root: Path) -> set[str]:
    path = root / ROADMAP_PATH
    if not path.is_file():
        return set()
    roadmap = path.read_text(encoding="utf-8")
    matches = list(TASK_RE.finditer(roadmap))
    completed: set[str] = set()
    for index, match in enumerate(matches):
        section_end = matches[index + 1].start() if index + 1 < len(matches) else len(roadmap)
        if re.search(r"- \[x\] \*\*Status:\*\* Complete\.", roadmap[match.start() : section_end]):
            completed.add(match.group("id"))
    return completed


def check_repository(root: Path) -> list[str]:
    registry, failures = load_registry(root)
    completed = completed_tasks(root)
    seen: set[str] = set()

    for path in iter_source_paths(root):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        markers = list(MARKER_RE.finditer(text))
        if COMPATIBILITY_SHIM_RE.search(text) and not markers:
            failures.append(f"{relpath}: untagged compatibility shim")
        for marker in markers:
            shim_id = marker.group("id")
            entry = registry.get(shim_id)
            if entry is None:
                failures.append(f"{relpath}: unregistered shim marker: {shim_id}")
                continue
            seen.add(shim_id)
            if entry["file"] != relpath:
                failures.append(f"{relpath}: shim marker file disagrees with inventory: {shim_id}")
            if entry["owner_task"] != marker.group("owner"):
                failures.append(f"{relpath}: shim marker owner disagrees with inventory: {shim_id}")
            if entry["removal_condition"] != marker.group("removal"):
                failures.append(f"{relpath}: shim marker removal disagrees with inventory: {shim_id}")
            if entry["owner_task"] in completed:
                failures.append(f"{relpath}: completed owner task retains shim: {shim_id}")

    for shim_id in sorted(set(registry) - seen):
        failures.append(f"shim inventory entry has no marker: {shim_id}")
    return failures


def main() -> int:
    failures = check_repository(REPO_ROOT)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print("PASS: compatibility shim guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
