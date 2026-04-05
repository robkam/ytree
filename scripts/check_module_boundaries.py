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
LEGACY_POLICY_EXCEPTIONS = set()

# Prevent regression of known "god module" hotspots.
CONTROLLER_FILE_LINE_BUDGET = {
    "src/ui/ctrl_dir.c": 1988,
    "src/ui/ctrl_file.c": 1883,
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
FUNCTION_DEF_RE = re.compile(
    r"(?m)^([A-Za-z_][\w\s\*]*?)\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{}]*)\)\s*\{"
)

CONTROL_FLOW_KEYWORDS = {"if", "for", "while", "switch"}

CONTROLLER_TOP_LEVEL_ALLOWLIST = {
    "src/ui/ctrl_dir.c": {
        "IsPathInside",
        "BuildDirOpCommand",
        "ResolveDirTargetPath",
        "HandleDirCopyMove",
        "ExitArchiveRootToParent",
        "RestorePanelFileSelection",
        "HandleDirectoryCompare",
        "HandleDirWindow",
        "DirListJump",
        "DrawDirListJumpPrompt",
    },
    "src/ui/ctrl_file.c": {
        "FilterPreviewAction",
        "UpdateStatsPanel",
        "ChangeFileOwner",
        "ChangeFileGroup",
        "RefreshFileView",
        "HandleFileWindow",
        "FindDirIndexInVolume",
        "FindVolumeForDir",
        "PositionOwnerFileCursor",
        "JumpToOwnerDirectory",
        "DrawFileListJumpPrompt",
        "ListJump",
        "UpdatePreview",
    },
}

CONTROLLER_GOD_FUNCTION_LINE_BUDGET = {
    "src/ui/ctrl_dir.c": {"HandleDirWindow": 1376},
    "src/ui/ctrl_file.c": {"HandleFileWindow": 1274},
}


def parse_includes(text: str) -> list[str]:
    includes: list[str] = []
    for line in text.splitlines():
        m = INCLUDE_RE.match(line)
        if not m:
            continue
        includes.append(m.group(1))
    return includes


def strip_non_code(text: str) -> str:
    out: list[str] = []
    i = 0
    n = len(text)
    state = "code"
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if ch == "/" and nxt == "/":
                state = "line_comment"
                out.extend((" ", " "))
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = "block_comment"
                out.extend((" ", " "))
                i += 2
                continue
            if ch == '"':
                state = "string"
                out.append('"')
                i += 1
                continue
            if ch == "'":
                state = "char"
                out.append("'")
                i += 1
                continue
            out.append(ch)
            i += 1
            continue

        if state == "line_comment":
            out.append("\n" if ch == "\n" else " ")
            if ch == "\n":
                state = "code"
            i += 1
            continue

        if state == "block_comment":
            if ch == "*" and nxt == "/":
                state = "code"
                out.extend((" ", " "))
                i += 2
                continue
            out.append("\n" if ch == "\n" else " ")
            i += 1
            continue

        if state == "string":
            if ch == "\\":
                out.append(" ")
                if i + 1 < n:
                    out.append(" ")
                    i += 2
                else:
                    i += 1
                continue
            if ch == '"':
                state = "code"
                out.append('"')
                i += 1
                continue
            out.append("\n" if ch == "\n" else " ")
            i += 1
            continue

        if ch == "\\":
            out.append(" ")
            if i + 1 < n:
                out.append(" ")
                i += 2
            else:
                i += 1
            continue
        if ch == "'":
            state = "code"
            out.append("'")
            i += 1
            continue
        out.append("\n" if ch == "\n" else " ")
        i += 1

    return "".join(out)


def find_matching_brace(text: str, open_index: int) -> int | None:
    depth = 0
    for i in range(open_index, len(text)):
        ch = text[i]
        if ch == "{":
            depth += 1
            continue
        if ch == "}":
            depth -= 1
            if depth == 0:
                return i
    return None


def parse_top_level_function_lengths(text: str) -> dict[str, int]:
    clean = strip_non_code(text)
    lengths: dict[str, int] = {}
    brace_depth = 0
    depth_at_index = [0] * (len(clean) + 1)
    for i, ch in enumerate(clean):
        depth_at_index[i] = brace_depth
        if ch == "{":
            brace_depth += 1
        elif ch == "}":
            brace_depth = max(0, brace_depth - 1)
    depth_at_index[len(clean)] = brace_depth

    for m in FUNCTION_DEF_RE.finditer(clean):
        fn_name = m.group(2)
        if fn_name in CONTROL_FLOW_KEYWORDS:
            continue
        if depth_at_index[m.start()] != 0:
            continue

        open_index = clean.find("{", m.end() - 1)
        if open_index < 0:
            continue
        close_index = find_matching_brace(clean, open_index)
        if close_index is None:
            continue

        start_line = clean.count("\n", 0, m.start()) + 1
        end_line = clean.count("\n", 0, close_index) + 1
        lengths[fn_name] = end_line - start_line + 1

    return lengths


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


def check_controller_allowlists(root: Path) -> list[str]:
    failures: list[str] = []
    for rel, approved in sorted(CONTROLLER_TOP_LEVEL_ALLOWLIST.items()):
        path = root / rel
        if not path.exists():
            failures.append(f"{rel}: missing file (controller allowlist check cannot run)")
            continue
        observed = set(
            parse_top_level_function_lengths(
                path.read_text(encoding="utf-8", errors="replace")
            )
        )
        introduced = sorted(observed - approved)
        if introduced:
            introduced_names = ", ".join(introduced)
            failures.append(
                f"{rel}: unapproved top-level controller function(s): {introduced_names}; "
                "move business/utility logic to dedicated module unless inseparable from "
                "the event loop; allowlist updates require explicit architecture approval"
            )
    return failures


def check_controller_god_function_budgets(root: Path) -> list[str]:
    failures: list[str] = []
    for rel, fn_budgets in sorted(CONTROLLER_GOD_FUNCTION_LINE_BUDGET.items()):
        path = root / rel
        if not path.exists():
            failures.append(f"{rel}: missing file (god-function budget check cannot run)")
            continue
        lengths = parse_top_level_function_lengths(
            path.read_text(encoding="utf-8", errors="replace")
        )
        for fn_name, budget in sorted(fn_budgets.items()):
            line_count = lengths.get(fn_name)
            if line_count is None:
                failures.append(f"{rel}: missing top-level function '{fn_name}'")
                continue
            if line_count > budget:
                failures.append(
                    f"{rel}: {fn_name} line budget exceeded ({line_count} > {budget}); "
                    "split logic into dedicated module(s) and keep controller dispatch-only"
                )
    return failures


def check_controller_budgets(root: Path) -> list[str]:
    failures: list[str] = []
    for rel, budget in sorted(CONTROLLER_FILE_LINE_BUDGET.items()):
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

    failures.extend(check_controller_allowlists(root))
    failures.extend(check_controller_god_function_budgets(root))
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
