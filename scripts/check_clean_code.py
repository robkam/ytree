#!/usr/bin/env python3
"""Reject new clean-code regressions against explicit baseline allowlists."""

from __future__ import annotations

import ast
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from clean_code_allowlist import (
    REPO_ROOT,
    compile_regex,
    literal_strings,
    load_allowlist,
    section_records,
)
from check_module_boundaries import (
    CONTROL_FLOW_KEYWORDS,
    FUNCTION_DEF_RE,
    find_matching_brace,
    strip_non_code,
)

C_SOURCE_GLOB = "src/**/*.c"
TEST_GLOB = "tests/test_*.py"
ALLOWED_SINGLE_LETTER_IDENTIFIERS = {"i", "j", "k"}
MAX_FUNCTION_LINES = 200
MAX_ARGUMENTS = 7
FLAG_PARAMETER_RE = re.compile(
    r"^(?:is|has|allow|include|show|update|move|append|enable|force|skip|keep)_[A-Za-z0-9_]+$"
)
MAGIC_LITERAL_RE = re.compile(r"(?<![A-Za-z0-9_])(-?\d+(?:\.\d+)?)")
CONTEXT_MAGIC_NUMBER_ALLOWED = {"-1", "0", "1", "2"}
COMMENT_RE = re.compile(r"//.*$")
CONSTANT_CONTEXT_RE = re.compile(
    r"^\s*(?:#\s*define|enum\b|static\s+const\b|const\b|typedef\b)"
)
MAGIC_NUMBER_CONTEXT_RE = re.compile(r"\b(?:if|while|for|case)\b|==|!=|<=|>=|[<>]")


@dataclass(frozen=True)
class FunctionInfo:
    relpath: str
    symbol: str
    params: list[str]
    start_line: int
    end_line: int

    @property
    def line_count(self) -> int:
        return self.end_line - self.start_line + 1


@dataclass(frozen=True)
class Finding:
    category: str
    relpath: str
    line: int
    detail: str
    symbol: str | None = None


def iter_c_functions(root: Path) -> Iterable[FunctionInfo]:
    for path in sorted(root.glob(C_SOURCE_GLOB)):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        clean = strip_non_code(text)
        depth_at_index = _depth_at_index(clean)
        for match in FUNCTION_DEF_RE.finditer(clean):
            symbol = match.group(2)
            if symbol in CONTROL_FLOW_KEYWORDS:
                continue
            if depth_at_index[match.start()] != 0:
                continue
            open_index = clean.find("{", match.end() - 1)
            if open_index < 0:
                continue
            close_index = find_matching_brace(clean, open_index)
            if close_index is None:
                continue
            params = _split_params(match.group(3))
            start_line = clean.count("\n", 0, match.start()) + 1
            end_line = clean.count("\n", 0, close_index) + 1
            yield FunctionInfo(
                relpath=relpath,
                symbol=symbol,
                params=params,
                start_line=start_line,
                end_line=end_line,
            )


def iter_single_letter_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "single_letter_identifier_exceptions")
    for info in iter_c_functions(root):
        for param in info.params:
            name = _parameter_name(param)
            if name is None:
                continue
            if len(name) != 1 or name in ALLOWED_SINGLE_LETTER_IDENTIFIERS:
                continue
            if _matches_single_letter_exception(entries, info, name):
                continue
            yield Finding(
                category="single-letter-identifier",
                relpath=info.relpath,
                line=info.start_line,
                symbol=info.symbol,
                detail=f"{info.symbol} uses single-letter parameter '{name}'",
            )


def iter_long_function_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "function_size_exceptions")
    for info in iter_c_functions(root):
        if info.line_count <= MAX_FUNCTION_LINES:
            continue
        if _matches_function_size_exception(entries, info):
            continue
        yield Finding(
            category="function-size",
            relpath=info.relpath,
            line=info.start_line,
            symbol=info.symbol,
            detail=(
                f"{info.symbol} is {info.line_count} lines; budget is {MAX_FUNCTION_LINES}"
            ),
        )


def iter_long_argument_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "long_argument_exceptions")
    for info in iter_c_functions(root):
        count = len(info.params)
        if count <= MAX_ARGUMENTS:
            continue
        if _matches_long_argument_exception(entries, info, count):
            continue
        yield Finding(
            category="long-parameter-list",
            relpath=info.relpath,
            line=info.start_line,
            symbol=info.symbol,
            detail=f"{info.symbol} takes {count} parameters; budget is {MAX_ARGUMENTS}",
        )


def iter_flag_argument_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "flag_argument_exceptions")
    for info in iter_c_functions(root):
        matched = sorted(
            name
            for name in (_parameter_name(param) for param in info.params)
            if name and FLAG_PARAMETER_RE.match(name)
        )
        if not matched:
            continue
        if _matches_flag_argument_exception(entries, info, matched):
            continue
        joined = ", ".join(matched)
        yield Finding(
            category="flag-argument",
            relpath=info.relpath,
            line=info.start_line,
            symbol=info.symbol,
            detail=f"{info.symbol} uses flag-style parameter(s): {joined}",
        )


def iter_magic_number_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "magic_number_exceptions")
    for path in sorted(root.glob(C_SOURCE_GLOB)):
        relpath = path.relative_to(root).as_posix()
        raw_text = path.read_text(encoding="utf-8", errors="replace")
        clean_lines = strip_non_code(raw_text).splitlines()
        for line_no, stripped in enumerate(clean_lines, start=1):
            if CONSTANT_CONTEXT_RE.match(stripped):
                continue
            stripped = COMMENT_RE.sub("", stripped).strip()
            if not stripped or not MAGIC_NUMBER_CONTEXT_RE.search(stripped):
                continue
            literals = [
                match.group(1)
                for match in MAGIC_LITERAL_RE.finditer(stripped)
                if match.group(1) not in CONTEXT_MAGIC_NUMBER_ALLOWED
            ]
            if not literals:
                continue
            if _matches_magic_number_exception(entries, relpath, stripped, literals):
                continue
            yield Finding(
                category="magic-number",
                relpath=relpath,
                line=line_no,
                detail=f"magic literal(s) {', '.join(literals)} in '{stripped}'",
            )


def iter_test_fixture_scope_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "test_fixture_scope_exceptions")
    for path in sorted(root.glob(TEST_GLOB)):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        tree = ast.parse(text, filename=relpath)
        for node in tree.body:
            if not isinstance(node, ast.FunctionDef):
                continue
            if not any(_decorator_name(decorator) == "fixture" for decorator in node.decorator_list):
                continue
            scope = _fixture_scope(node)
            if scope in {None, "function"}:
                continue
            if _matches_fixture_scope_exception(entries, relpath, node.name):
                continue
            yield Finding(
                category="test-fixture-scope",
                relpath=relpath,
                line=node.lineno,
                symbol=node.name,
                detail=f"fixture '{node.name}' uses '{scope}' scope",
            )


def iter_test_mutable_global_findings(
    root: Path,
    allowlist: dict[str, object],
) -> Iterable[Finding]:
    entries = section_records(allowlist, "test_mutable_global_exceptions")
    for path in sorted(root.glob(TEST_GLOB)):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        tree = ast.parse(text, filename=relpath)
        for node in tree.body:
            if not isinstance(node, ast.Assign):
                continue
            if not isinstance(node.value, (ast.List, ast.Dict, ast.Set)):
                continue
            for target in node.targets:
                if not isinstance(target, ast.Name):
                    continue
                if target.id.isupper():
                    continue
                if _matches_mutable_global_exception(entries, relpath, target.id):
                    continue
                yield Finding(
                    category="test-mutable-global",
                    relpath=relpath,
                    line=node.lineno,
                    detail=f"mutable global '{target.id}' should move into fixture-local state",
                )


def main() -> int:
    allowlist, failures = load_allowlist()
    findings = list(iter_findings(REPO_ROOT, allowlist))
    failures.extend(f"FAIL: {format_finding(finding)}" for finding in findings)

    if failures:
        for failure in failures:
            print(failure)
        print(
            f"FAIL: clean-code guard failed ({len(findings)} finding(s), "
            f"{len(failures) - len(findings)} allowlist/config issue(s))"
        )
        return 1

    print("PASS: clean-code guard passed")
    return 0


def iter_findings(root: Path, allowlist: dict[str, object]) -> Iterable[Finding]:
    yield from iter_single_letter_findings(root, allowlist)
    yield from iter_long_function_findings(root, allowlist)
    yield from iter_long_argument_findings(root, allowlist)
    yield from iter_flag_argument_findings(root, allowlist)
    yield from iter_magic_number_findings(root, allowlist)
    yield from iter_test_fixture_scope_findings(root, allowlist)
    yield from iter_test_mutable_global_findings(root, allowlist)


def format_finding(finding: Finding) -> str:
    symbol = f"{finding.symbol}: " if finding.symbol else ""
    return f"{finding.relpath}:{finding.line}: {finding.category}: {symbol}{finding.detail}"


def _depth_at_index(text: str) -> list[int]:
    depth = 0
    values = [0] * (len(text) + 1)
    for index, ch in enumerate(text):
        values[index] = depth
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
    values[len(text)] = depth
    return values


def _split_params(raw: str) -> list[str]:
    if not raw.strip() or raw.strip() == "void":
        return []
    return [part.strip() for part in raw.split(",") if part.strip()]


def _parameter_name(param: str) -> str | None:
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*$", param)
    if match is None:
        return None
    return match.group(1)


def _matches_single_letter_exception(
    entries: list[dict[str, object]],
    info: FunctionInfo,
    identifier: str,
) -> bool:
    for entry in entries:
        if not _regex_matches(entry.get("path_regex"), info.relpath):
            continue
        if not _regex_matches(entry.get("symbol_regex"), info.symbol):
            continue
        identifiers = entry.get("identifiers")
        if isinstance(identifiers, list) and identifier in identifiers:
            return True
    return False


def _matches_long_argument_exception(
    entries: list[dict[str, object]],
    info: FunctionInfo,
    count: int,
) -> bool:
    for entry in entries:
        if not _regex_matches(entry.get("path_regex"), info.relpath):
            continue
        if not _regex_matches(entry.get("symbol_regex"), info.symbol):
            continue
        max_args = entry.get("max_args")
        if isinstance(max_args, int) and count <= max_args:
            return True
    return False


def _matches_function_size_exception(
    entries: list[dict[str, object]],
    info: FunctionInfo,
) -> bool:
    for entry in entries:
        if not _regex_matches(entry.get("path_regex"), info.relpath):
            continue
        if not _regex_matches(entry.get("symbol_regex"), info.symbol):
            continue
        max_lines = entry.get("max_lines")
        if isinstance(max_lines, int) and info.line_count <= max_lines:
            return True
    return False


def _matches_flag_argument_exception(
    entries: list[dict[str, object]],
    info: FunctionInfo,
    matched: list[str],
) -> bool:
    for entry in entries:
        if not _regex_matches(entry.get("path_regex"), info.relpath):
            continue
        if not _regex_matches(entry.get("symbol_regex"), info.symbol):
            continue
        parameters = entry.get("parameters")
        if not isinstance(parameters, list):
            continue
        if set(matched).issubset(set(parameters)):
            return True
    return False


def _matches_magic_number_exception(
    entries: list[dict[str, object]],
    relpath: str,
    line: str,
    literals: list[str],
) -> bool:
    for entry in entries:
        if not _regex_matches(entry.get("path_regex"), relpath):
            continue
        if not _regex_matches(entry.get("line_regex"), line):
            continue
        allowed = entry.get("literals")
        if isinstance(allowed, list) and set(literals).issubset(literal_strings(allowed)):
            return True
    return False


def _matches_fixture_scope_exception(
    entries: list[dict[str, object]],
    relpath: str,
    fixture_name: str,
) -> bool:
    for entry in entries:
        if _regex_matches(entry.get("path_regex"), relpath) and _regex_matches(
            entry.get("fixture_regex"),
            fixture_name,
        ):
            return True
    return False


def _matches_mutable_global_exception(
    entries: list[dict[str, object]],
    relpath: str,
    name: str,
) -> bool:
    for entry in entries:
        if _regex_matches(entry.get("path_regex"), relpath) and _regex_matches(
            entry.get("name_regex"),
            name,
        ):
            return True
    return False


def _regex_matches(pattern: object, text: str) -> bool:
    return isinstance(pattern, str) and bool(compile_regex(pattern).search(text))


def _decorator_name(node: ast.expr) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return node.attr
    if isinstance(node, ast.Call):
        return _decorator_name(node.func)
    return None


def _fixture_scope(node: ast.FunctionDef) -> str | None:
    for decorator in node.decorator_list:
        if not isinstance(decorator, ast.Call):
            continue
        if _decorator_name(decorator) != "fixture":
            continue
        for keyword in decorator.keywords:
            if keyword.arg != "scope":
                continue
            if isinstance(keyword.value, ast.Constant) and isinstance(keyword.value.value, str):
                return keyword.value.value
    return None


if __name__ == "__main__":
    raise SystemExit(main())
