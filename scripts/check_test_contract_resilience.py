#!/usr/bin/env python3
"""Generate and enforce the reviewed brittle-test-pattern inventory."""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = REPO_ROOT / "tests" / "contract_resilience_baseline.json"
SCHEMA_VERSION = 1
DISPOSITIONS = {"remediated", "retained", "out_of_scope"}


@dataclass(frozen=True)
class Match:
    pattern_id: str
    path: str
    symbol: str | None
    line: int
    evidence: str

    @property
    def identity(self) -> str:
        digest = hashlib.sha256(
            "\0".join(
                (self.pattern_id, self.path, self.symbol or "", str(self.line), self.evidence)
            ).encode("utf-8")
        ).hexdigest()[:16]
        return f"{self.pattern_id}:{self.path}:{self.line}:{digest}"


DEFAULT_DISPOSITIONS = {
    "direct-time-sleep": (
        "waiting and navigation remediation",
        "Direct timing waits require event-driven synchronization redesign.",
    ),
    "polling-or-retry-loop": (
        "waiting and navigation remediation",
        "Polling and retry mechanisms belong to the semantic waiting boundary.",
    ),
    "fixed-navigation-loop": (
        "waiting and navigation remediation",
        "Fixed key-count navigation must be replaced with target-identity navigation.",
    ),
    "terminal-geometry": (
        "geometry and presentation remediation",
        "Geometry assertions require presentation-contract remediation.",
    ),
    "screen-slice-or-grid": (
        "geometry and presentation remediation",
        "Screen-grid assertions require semantic visual-state replacement.",
    ),
    "source-read": (
        "external and static contract classification",
        "Source inspection requires observable-contract or static-invariant classification.",
    ),
    "implementation-string-assertion": (
        "external and static contract classification",
        "Implementation-coupled assertions require runtime or static-invariant classification.",
    ),
    "exact-prose-assertion": (
        "geometry and presentation remediation",
        "Editable presentation prose requires a durable behavioural replacement.",
    ),
}

REVIEWED_EXCEPTIONS = {
    (
        "tests/test_f7_preview.py",
        "test_f7_file_name_clipping_at_boundaries",
        "polling-or-retry-loop",
    ): (
        "out_of_scope",
        "geometry and presentation remediation",
        "Bounded scan parses one captured preview snapshot; it performs no waiting, retry, or user navigation.",
    ),
    (
        "tests/test_stats_panel.py",
        "_stats_strip_bounds",
        "polling-or-retry-loop",
    ): (
        "out_of_scope",
        "geometry and presentation remediation",
        "Bounded string search parses one captured stats snapshot; it performs no waiting, retry, or user navigation.",
    ),
    (
        "tests/tui_harness.py",
        "wait_for_condition",
        "polling-or-retry-loop",
    ): (
        "retained",
        "waiting and navigation remediation",
        "Canonical event-driven PTY-output predicate: waits for observable state with a deadline and diagnostic, never an elapsed test delay or fixed action count.",
    ),
    (
        "tests/ytnova_control.py",
        "wait_for_condition",
        "polling-or-retry-loop",
    ): (
        "retained",
        "waiting and navigation remediation",
        "Canonical control-session event predicate: waits for observable state with a deadline and diagnostic, never an elapsed test delay or fixed action count.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_copy_matrix_fs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies copied fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_copy_matrix_vfs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies copied fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_move_matrix_fs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies moved fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_move_matrix_vfs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies moved fixture bytes; the string is test data, not editable interface prose.",
    ),
}


class PatternVisitor(ast.NodeVisitor):
    def __init__(self, path: str, lines: list[str]) -> None:
        self.path = path
        self.lines = lines
        self.symbols: list[str] = []
        self.matches: list[Match] = []

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_For(self, node: ast.For) -> None:
        if _is_range_loop(node) and _contains_key_send(node):
            self._add("fixed-navigation-loop", node)
        elif _is_range_loop(node) and (_contains_retry_signal(node) or _has_retry_target(node)):
            self._add("polling-or-retry-loop", node)
        self.generic_visit(node)

    def visit_While(self, node: ast.While) -> None:
        if _contains_retry_signal(node) or _contains_time_call(node.test):
            self._add("polling-or-retry-loop", node)
        self.generic_visit(node)

    def visit_Call(self, node: ast.Call) -> None:
        if _dotted_name(node.func) == "time.sleep":
            self._add("direct-time-sleep", node)
        elif _is_source_read(node):
            self._add("source-read", node)
        if _has_terminal_geometry(node):
            self._add("terminal-geometry", node)
        self.generic_visit(node)

    def visit_Subscript(self, node: ast.Subscript) -> None:
        if _looks_like_screen_slice(node):
            self._add("screen-slice-or-grid", node)
        self.generic_visit(node)

    def visit_Assert(self, node: ast.Assert) -> None:
        if _is_implementation_string_assertion(node.test):
            self._add("implementation-string-assertion", node)
        if _is_exact_prose_assertion(node.test):
            self._add("exact-prose-assertion", node)
        self.generic_visit(node)

    def visit_Compare(self, node: ast.Compare) -> None:
        if _looks_like_geometry_comparison(node):
            self._add("terminal-geometry", node)
        self.generic_visit(node)

    def _visit_symbol(self, name: str, node: ast.AST) -> None:
        self.symbols.append(name)
        self.generic_visit(node)
        self.symbols.pop()

    def _add(self, pattern_id: str, node: ast.AST) -> None:
        line = getattr(node, "lineno", 1)
        evidence = self.lines[line - 1].strip() if line <= len(self.lines) else ""
        self.matches.append(
            Match(pattern_id, self.path, self.symbols[-1] if self.symbols else None, line, evidence)
        )


def _dotted_name(node: ast.AST) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        parent = _dotted_name(node.value)
        return f"{parent}.{node.attr}" if parent else node.attr
    return None


def _is_range_loop(node: ast.For) -> bool:
    return isinstance(node.iter, ast.Call) and _dotted_name(node.iter.func) == "range"


def _contains_key_send(node: ast.AST) -> bool:
    return any(
        isinstance(item, ast.Call)
        and (_dotted_name(item.func) or "").split(".")[-1]
        in {"send", "send_keystroke", "send_key", "press"}
        for item in ast.walk(node)
    )


def _contains_retry_signal(node: ast.AST) -> bool:
    return any(
        isinstance(item, (ast.Try, ast.Break, ast.Continue))
        or (isinstance(item, ast.Call) and _contains_time_call(item))
        for item in ast.walk(node)
    )


def _has_retry_target(node: ast.For) -> bool:
    return isinstance(node.target, ast.Name) and any(
        token in node.target.id.lower() for token in ("attempt", "retry", "poll")
    )


def _contains_time_call(node: ast.AST) -> bool:
    return any(
        isinstance(item, ast.Call)
        and (_dotted_name(item.func) or "") in {"time.monotonic", "time.time", "perf_counter"}
        for item in ast.walk(node)
    )


def _is_source_read(node: ast.Call) -> bool:
    method = (_dotted_name(node.func) or "").split(".")[-1]
    if method in {"_read_source", "read_repo_source", "extract_function_block"}:
        return True
    if method not in {"read_text", "read_bytes", "open"}:
        return False
    return any(
        isinstance(value, ast.Constant)
        and isinstance(value.value, str)
        and ("src/" in value.value or value.value.endswith((".c", ".h")))
        for value in ast.walk(node)
    )


def _has_terminal_geometry(node: ast.Call) -> bool:
    return any(
        keyword.arg in {"rows", "cols", "columns", "width", "height"}
        and isinstance(keyword.value, ast.Constant)
        and isinstance(keyword.value.value, int)
        for keyword in node.keywords
    )


def _looks_like_screen_slice(node: ast.Subscript) -> bool:
    value = _dotted_name(node.value) or ""
    if value.split(".")[-1] in {"lines", "screen", "footer_rows", "rows"}:
        return True
    return isinstance(node.slice, ast.Slice) and "screen" in value.lower()


def _string_constants(node: ast.AST) -> list[str]:
    return [item.value for item in ast.walk(node) if isinstance(item, ast.Constant) and isinstance(item.value, str)]


def _is_implementation_string_assertion(node: ast.AST) -> bool:
    strings = _string_constants(node)
    return any(
        token in value
        for value in strings
        for token in ("src/", ".c", ".h", "static ", "void ", "int ")
    )


def _is_exact_prose_assertion(node: ast.AST) -> bool:
    if not isinstance(node, (ast.Compare, ast.BoolOp)):
        return False
    return any(
        len(value.split()) >= 2 or "\n" in value for value in _string_constants(node)
    )


def _looks_like_geometry_comparison(node: ast.Compare) -> bool:
    names = {_dotted_name(value) or "" for value in ast.walk(node)}
    return any(
        name.split(".")[-1] in {"rows", "cols", "columns", "width", "height", "x", "y"}
        for name in names
    )


def scan(root: Path) -> list[Match]:
    matches: list[Match] = []
    for path in sorted((root / "tests").rglob("*.py")):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        visitor = PatternVisitor(relpath, text.splitlines())
        visitor.visit(ast.parse(text, filename=relpath))
        matches.extend(visitor.matches)
    unique = {
        (match.pattern_id, match.path, match.symbol, match.line, match.evidence): match
        for match in matches
    }
    return sorted(
        unique.values(), key=lambda match: (match.path, match.line, match.pattern_id, match.evidence)
    )


def _baseline_row(match: Match) -> dict[str, object]:
    disposition = "out_of_scope"
    owner, reason = DEFAULT_DISPOSITIONS[match.pattern_id]
    exception = REVIEWED_EXCEPTIONS.get((match.path, match.symbol, match.pattern_id))
    if exception:
        disposition, owner, reason = exception
    return {
        "id": match.identity,
        "pattern_id": match.pattern_id,
        "path": match.path,
        "symbol": match.symbol,
        "line": match.line,
        "evidence": match.evidence,
        "disposition": disposition,
        "reason": reason,
        "owner": owner,
    }


def build_baseline(root: Path) -> dict[str, object]:
    return {
        "schema_version": SCHEMA_VERSION,
        "contract": "Reviewed brittle-pattern inventory for Python tests",
        "matches": [_baseline_row(match) for match in scan(root)],
    }


def validate_baseline(document: object, root: Path) -> list[str]:
    if not isinstance(document, dict):
        return ["baseline must be a JSON object"]
    if document.get("schema_version") != SCHEMA_VERSION:
        return [f"baseline schema_version must be {SCHEMA_VERSION}"]
    rows = document.get("matches")
    if not isinstance(rows, list):
        return ["baseline matches must be a list"]

    failures: list[str] = []
    expected = {match.identity: match for match in scan(root)}
    observed: dict[str, dict[str, object]] = {}
    required = {"id", "pattern_id", "path", "symbol", "line", "evidence", "disposition", "reason", "owner"}
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            failures.append(f"matches[{index}] must be an object")
            continue
        missing = sorted(required - row.keys())
        if missing:
            failures.append(f"matches[{index}] lacks required field(s): {', '.join(missing)}")
            continue
        row_id = row["id"]
        if not isinstance(row_id, str):
            failures.append(f"matches[{index}].id must be a string")
            continue
        if row_id in observed:
            failures.append(f"duplicate baseline match: {row_id}")
        observed[row_id] = row
        expected_match = expected.get(row_id)
        if expected_match is not None:
            expected_fields = {
                "pattern_id": expected_match.pattern_id,
                "path": expected_match.path,
                "symbol": expected_match.symbol,
                "line": expected_match.line,
                "evidence": expected_match.evidence,
            }
            for field, value in expected_fields.items():
                if row.get(field) != value:
                    failures.append(f"{row_id}: {field} does not match scanned evidence")
        if row.get("disposition") not in DISPOSITIONS:
            failures.append(f"{row_id}: invalid disposition")
        for field in ("reason", "owner"):
            if not isinstance(row.get(field), str) or not row[field].strip():
                failures.append(f"{row_id}: {field} must be non-empty")

    for row_id in sorted(expected.keys() - observed.keys()):
        match = expected[row_id]
        failures.append(f"unreviewed match: {match.path}:{match.line}: {match.pattern_id}")
    for row_id in sorted(observed.keys() - expected.keys()):
        failures.append(f"stale baseline match: {row_id}")
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument("--baseline", type=Path, default=BASELINE_PATH)
    parser.add_argument("--write-baseline", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    if args.write_baseline:
        args.baseline.write_text(
            json.dumps(build_baseline(root), indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        print(f"WROTE: {args.baseline}")
        return 0
    try:
        document = json.loads(args.baseline.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: unable to read baseline: {exc}", file=sys.stderr)
        return 1
    failures = validate_baseline(document, root)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print("PASS: test-contract resilience baseline is fully reconciled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
