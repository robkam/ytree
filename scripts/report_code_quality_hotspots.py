#!/usr/bin/env python3
"""Report guarded code-quality hotspots and before/after debt deltas."""

from __future__ import annotations

import argparse
import importlib.util
import json
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
GUARD_PATH = REPO_ROOT / "scripts" / "check_module_boundaries.py"


def _load_guard_module() -> Any:
    spec = importlib.util.spec_from_file_location("check_module_boundaries", GUARD_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load guard module: {GUARD_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


guard = _load_guard_module()


def _signed(value: int) -> str:
    return f"{value:+d}"


def _line_count(path: Path) -> int:
    return sum(1 for _ in path.open("r", encoding="utf-8", errors="replace"))


def _make_row(
    *,
    category: str,
    surface: str,
    metric: str,
    current: int,
    budget: int,
) -> dict[str, Any]:
    return {
        "category": category,
        "surface": surface,
        "metric": metric,
        "current": current,
        "budget": budget,
        "slack": budget - current,
    }


def build_snapshot(root: Path) -> dict[str, Any]:
    root = root.resolve()
    controller_files: list[dict[str, Any]] = []
    controller_functions: list[dict[str, Any]] = []

    for rel, budget in sorted(guard.CONTROLLER_FILE_LINE_BUDGET.items()):
        path = root / rel
        if not path.exists():
            raise ValueError(f"missing guarded controller file: {rel}")
        controller_files.append(
            _make_row(
                category="controller-file",
                surface=rel,
                metric="file_lines",
                current=_line_count(path),
                budget=budget,
            )
        )

    for rel, fn_budgets in sorted(guard.CONTROLLER_GOD_FUNCTION_LINE_BUDGET.items()):
        path = root / rel
        if not path.exists():
            raise ValueError(f"missing guarded controller file for function budgets: {rel}")
        lengths = guard.parse_top_level_function_lengths(
            path.read_text(encoding="utf-8", errors="replace")
        )
        for fn_name, budget in sorted(fn_budgets.items()):
            current = lengths.get(fn_name)
            if current is None:
                raise ValueError(f"missing guarded top-level function: {rel}:{fn_name}")
            controller_functions.append(
                _make_row(
                    category="controller-function",
                    surface=f"{rel}:{fn_name}",
                    metric="function_lines",
                    current=current,
                    budget=budget,
                )
            )

    combined = sorted(
        controller_files + controller_functions,
        key=lambda row: (row["slack"], -row["current"], row["surface"]),
    )
    return {
        "repo_root": str(root),
        "guard_source": str(GUARD_PATH.relative_to(REPO_ROOT)),
        "legacy_policy_exception_count": len(guard.LEGACY_POLICY_EXCEPTIONS),
        "controller_files": controller_files,
        "controller_functions": controller_functions,
        "combined": combined,
    }


def _index_rows(rows: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    return {row["surface"]: row for row in rows}


def comparison_rows(
    snapshot: dict[str, Any],
    baseline: dict[str, Any] | None,
) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    baseline_rows: dict[str, dict[str, Any]] = {}
    if baseline is not None:
        baseline_rows = _index_rows(baseline.get("combined", []))

    for row in snapshot["combined"]:
        annotated = dict(row)
        previous = baseline_rows.get(row["surface"])
        if previous is not None:
            annotated["before"] = previous["current"]
            annotated["delta"] = row["current"] - previous["current"]
            annotated["slack_before"] = previous["slack"]
            annotated["slack_delta"] = row["slack"] - previous["slack"]
        else:
            annotated["before"] = None
            annotated["delta"] = None
            annotated["slack_before"] = None
            annotated["slack_delta"] = None
        rows.append(annotated)
    return rows


def _top_rows(rows: list[dict[str, Any]], top: int) -> list[dict[str, Any]]:
    if top <= 0:
        return rows
    return rows[:top]


def render_markdown(
    snapshot: dict[str, Any],
    *,
    baseline: dict[str, Any] | None = None,
    top: int = 5,
) -> str:
    rows = _top_rows(comparison_rows(snapshot, baseline), top)
    lines = [
        "# Code-quality hotspots",
        "",
        f"- Guard source: `{snapshot['guard_source']}`",
        f"- Repo root: `{snapshot['repo_root']}`",
        f"- Legacy policy exceptions: `{snapshot['legacy_policy_exception_count']}`",
        "",
    ]

    if baseline is None:
        lines.extend(
            [
                "| Rank | Surface | Metric | Current | Budget | Slack |",
                "| --- | --- | --- | ---: | ---: | ---: |",
            ]
        )
        for idx, row in enumerate(rows, start=1):
            lines.append(
                f"| {idx} | {row['surface']} | {row['metric']} | "
                f"{row['current']} | {row['budget']} | {_signed(row['slack'])} |"
            )
        return "\n".join(lines) + "\n"

    lines.extend(
        [
            "| Rank | Surface | Metric | Before | After | Delta | Budget | Slack Δ |",
            "| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |",
        ]
    )
    for idx, row in enumerate(rows, start=1):
        before = "n/a" if row["before"] is None else str(row["before"])
        delta = "n/a" if row["delta"] is None else _signed(row["delta"])
        slack_delta = "n/a" if row["slack_delta"] is None else _signed(row["slack_delta"])
        lines.append(
            f"| {idx} | {row['surface']} | {row['metric']} | "
            f"{before} | {row['current']} | {delta} | {row['budget']} | {slack_delta} |"
        )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=REPO_ROOT,
        help="Repository root to inspect (defaults to this checkout).",
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        help="Optional JSON snapshot produced by this script for before/after comparison.",
    )
    parser.add_argument(
        "--format",
        choices=("markdown", "json"),
        default="markdown",
        help="Output format.",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=5,
        help="How many highest-risk guarded hotspots to emit (<=0 means all).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        snapshot = build_snapshot(args.root)
        baseline = None
        if args.baseline is not None:
            baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
    except (OSError, ValueError, json.JSONDecodeError, RuntimeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1

    if args.format == "json":
        payload = dict(snapshot)
        payload["top_hotspots"] = _top_rows(snapshot["combined"], args.top)
        if baseline is not None:
            payload["comparison_top_hotspots"] = _top_rows(
                comparison_rows(snapshot, baseline),
                args.top,
            )
        json.dump(payload, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
        return 0

    sys.stdout.write(render_markdown(snapshot, baseline=baseline, top=args.top))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
