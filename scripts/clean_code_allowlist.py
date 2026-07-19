#!/usr/bin/env python3
"""Shared clean-code allowlist loading and validation helpers."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ALLOWLIST_PATH = REPO_ROOT / "docs" / "clean_code_allowlist.json"

ENTRY_REQUIRED_FIELDS = {"id", "owner", "removal_plan"}
SECTION_REQUIRED_FIELDS = {
    "controller_file_line_budgets": {"path", "budget"},
    "controller_function_line_budgets": {"path", "symbol", "budget"},
    "single_letter_identifier_exceptions": {"path_regex", "symbol_regex", "identifiers"},
    "function_size_exceptions": {"path_regex", "symbol_regex", "max_lines"},
    "long_argument_exceptions": {"path_regex", "symbol_regex", "max_args"},
    "flag_argument_exceptions": {"path_regex", "symbol_regex", "parameters"},
    "magic_number_exceptions": {"path_regex", "line_regex", "literals"},
    "test_fixture_scope_exceptions": {"path_regex", "fixture_regex"},
    "test_mutable_global_exceptions": {"path_regex", "name_regex"},
}
REGEX_FIELDS = {
    "path_regex",
    "symbol_regex",
    "line_regex",
    "fixture_regex",
    "name_regex",
}


def load_allowlist(path: Path = DEFAULT_ALLOWLIST_PATH) -> tuple[dict[str, Any], list[str]]:
    failures: list[str] = []
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        return {}, [f"{path}: unable to read clean-code allowlist ({exc})"]
    except json.JSONDecodeError as exc:
        return {}, [f"{path}: invalid JSON ({exc})"]

    if not isinstance(document, dict):
        return {}, [f"{path}: top-level clean-code allowlist must be a JSON object"]

    seen_ids: set[str] = set()
    for section, required_fields in sorted(SECTION_REQUIRED_FIELDS.items()):
        records = document.get(section, [])
        if not isinstance(records, list):
            failures.append(f"{path}: section '{section}' must be a list")
            continue
        for index, record in enumerate(records, start=1):
            label = f"{path}:{section}[{index}]"
            if not isinstance(record, dict):
                failures.append(f"{label}: entry must be an object")
                continue
            missing = sorted((ENTRY_REQUIRED_FIELDS | required_fields) - set(record))
            if missing:
                failures.append(f"{label}: missing field(s): {', '.join(missing)}")
            entry_id = record.get("id")
            if isinstance(entry_id, str):
                if entry_id in seen_ids:
                    failures.append(f"{label}: duplicate id '{entry_id}'")
                else:
                    seen_ids.add(entry_id)
            for key in sorted(REGEX_FIELDS & set(record)):
                value = record.get(key)
                if not isinstance(value, str):
                    failures.append(f"{label}: field '{key}' must be a string")
                    continue
                try:
                    re.compile(value)
                except re.error as exc:
                    failures.append(f"{label}: invalid regex in '{key}' ({exc})")
            if "budget" in record and not isinstance(record.get("budget"), int):
                failures.append(f"{label}: field 'budget' must be an integer")
            if "max_args" in record and not isinstance(record.get("max_args"), int):
                failures.append(f"{label}: field 'max_args' must be an integer")
            if "max_lines" in record and not isinstance(record.get("max_lines"), int):
                failures.append(f"{label}: field 'max_lines' must be an integer")
            if "identifiers" in record and not _is_string_list(record.get("identifiers")):
                failures.append(f"{label}: field 'identifiers' must be a list of strings")
            if "parameters" in record and not _is_string_list(record.get("parameters")):
                failures.append(f"{label}: field 'parameters' must be a list of strings")
            if "literals" in record and not _is_scalar_list(record.get("literals")):
                failures.append(
                    f"{label}: field 'literals' must be a list of strings, ints, or floats"
                )

    return document, failures


def section_records(document: dict[str, Any], section: str) -> list[dict[str, Any]]:
    records = document.get(section, [])
    if not isinstance(records, list):
        return []
    return [record for record in records if isinstance(record, dict)]


def compile_regex(pattern: str) -> re.Pattern[str]:
    return re.compile(pattern)


def literal_strings(values: list[Any]) -> set[str]:
    return {str(value) for value in values}


def _is_string_list(value: Any) -> bool:
    return isinstance(value, list) and all(isinstance(item, str) for item in value)


def _is_scalar_list(value: Any) -> bool:
    return isinstance(value, list) and all(
        isinstance(item, (str, int, float)) and not isinstance(item, bool)
        for item in value
    )
