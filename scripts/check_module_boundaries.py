#!/usr/bin/env python3
"""Structural guardrails for module boundaries and controller growth."""

from __future__ import annotations

import re
from pathlib import Path

LAYER_ORDER = ("core", "cmd", "fs", "ui", "util")

HEADER_LAYER = {
    "config.h": "cmd",
    "default_profile_template.h": "core",
    "patchlev.h": "core",
    "sort.h": "core",
    "watcher.h": "fs",
    "ytree.h": "core",
    "ytree_cmd.h": "cmd",
    "ytree_debug.h": "core",
    "ytree_defs.h": "core",
    "ytree_dialog.h": "ui",
    "ytree_fs.h": "fs",
    "ytree_ui.h": "ui",
}

# Target per-directory policy. New violations are blocked by default.
POLICY_ALLOWED_DEPENDENCIES = {
    "core": {"core"},
    "cmd": {"core", "cmd", "fs", "util"},
    "fs": {"core", "fs", "util"},
    "ui": {"core", "cmd", "fs", "ui", "util"},
    "util": {"core", "util"},
}

# Temporary debt exceptions for pre-existing cross-layer couplings.
# Any new exception requires explicit architecture review.
LEGACY_POLICY_EXCEPTIONS = {
    ("src/core/init.c", "config.h"),
    ("src/core/init.c", "watcher.h"),
    ("src/core/init.c", "ytree_cmd.h"),
    ("src/core/init.c", "ytree_fs.h"),
    ("src/core/init.c", "ytree_ui.h"),
    ("src/core/main.c", "ytree_cmd.h"),
    ("src/core/main.c", "ytree_fs.h"),
    ("src/core/main.c", "ytree_ui.h"),
    ("src/core/volume.c", "ytree_fs.h"),
}

# Prevent regression of known "god module" hotspots.
CONTROLLER_LINE_BUDGET = {
    "src/ui/ctrl_dir.c": 2833,
    "src/ui/ctrl_file.c": 2336,
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')


def parse_includes(text: str) -> list[str]:
    includes: list[str] = []
    for line in text.splitlines():
        m = INCLUDE_RE.match(line)
        if not m:
            continue
        includes.append(m.group(1))
    return includes


def check_source_file(path: Path, root: Path) -> list[str]:
    rel = path.relative_to(root).as_posix()
    layer = path.parts[-2]
    failures: list[str] = []
    text = path.read_text(encoding="utf-8", errors="replace")

    if layer not in LAYER_ORDER:
        failures.append(f"{rel}: unknown source layer '{layer}'")
        return failures

    for raw_include in parse_includes(text):
        include_name = Path(raw_include).name

        if include_name.endswith(".c"):
            failures.append(f"{rel}: forbidden implementation include '{raw_include}'")
            continue

        target_layer = HEADER_LAYER.get(include_name)
        if not target_layer:
            continue

        allowed_targets = POLICY_ALLOWED_DEPENDENCIES[layer]
        if target_layer not in allowed_targets:
            exception_key = (rel, include_name)
            if exception_key not in LEGACY_POLICY_EXCEPTIONS:
                failures.append(
                    f"{rel}: policy violation {layer} -> {target_layer} "
                    f"(via {include_name}); add module boundary or approved exception"
                )

    return failures


def check_controller_budgets(root: Path) -> list[str]:
    failures: list[str] = []
    for rel, budget in sorted(CONTROLLER_LINE_BUDGET.items()):
        path = root / rel
        if not path.exists():
            failures.append(f"{rel}: missing file (budget check cannot run)")
            continue
        line_count = sum(1 for _ in path.open("r", encoding="utf-8", errors="replace"))
        if line_count > budget:
            failures.append(
                f"{rel}: line budget exceeded ({line_count} > {budget}); "
                "move logic to dedicated module(s)"
            )
    return failures


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    src_root = root / "src"
    failures: list[str] = []

    for path in sorted(src_root.rglob("*.c")):
        failures.extend(check_source_file(path, root))

    failures.extend(check_controller_budgets(root))

    stale_exceptions = []
    for rel, include_name in sorted(LEGACY_POLICY_EXCEPTIONS):
        path = root / rel
        if not path.exists():
            stale_exceptions.append(f"{rel}: legacy exception references missing file")
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        includes = {Path(raw).name for raw in parse_includes(text)}
        if include_name not in includes:
            stale_exceptions.append(
                f"{rel}: legacy exception for {include_name} is stale and should be removed"
            )

    failures.extend(stale_exceptions)

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: module-boundary guard failed ({len(failures)} issue(s))")
        return 1

    print(
        "PASS: module-boundary guard passed "
        f"({len(LEGACY_POLICY_EXCEPTIONS)} documented legacy exceptions)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
