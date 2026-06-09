#!/usr/bin/env python3
"""Validate the documented AppState transition and compatibility-shim registries."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_TRANSITIONS = REPO_ROOT / "docs" / "appstate_transition_matrix.json"
DEFAULT_SHIMS = REPO_ROOT / "docs" / "appstate_compat_shims.json"
DEFAULT_ACTION_COVERAGE = REPO_ROOT / "docs" / "appstate_action_coverage.json"
DEFAULT_ACTION_HEADER = REPO_ROOT / "include" / "ytree_defs.h"

REQUIRED_TRANSITION_CATEGORIES = {
    "keybinding",
    "menu_action",
    "modal_action",
    "refresh_rebuild",
    "volume_operation",
    "terminal_signal_or_resize",
    "filesystem_mutation_result",
    "command_completion",
    "rebuild_rebind_callback",
    "render_reflow",
}

REQUIRED_TRANSITION_FIELDS = {
    "id",
    "category",
    "source_state",
    "event",
    "guard",
    "allowed_result",
    "blocked_result",
    "target_state",
    "owner",
    "declared_write_set",
    "generation_effect",
    "side_effects",
    "render_invalidation",
    "boundary_status",
    "notes_follow_up",
}

REQUIRED_SHIM_FIELDS = {
    "id",
    "owner",
    "old_authority_path",
    "read_permission",
    "write_permission",
    "invariant_checks",
    "removal_trigger",
    "target_transition",
    "follow_up_task",
    "qa_enforcement",
}

REQUIRED_ACTION_FIELDS = {
    "action",
    "transition_id",
    "category",
    "owner",
    "declared_write_set",
    "boundary_status",
    "migration_notes",
}

LIST_FIELDS = {
    "declared_write_set",
    "side_effects",
    "invariant_checks",
    "migration_notes",
}


def _load_json(path: Path) -> tuple[Any | None, list[str]]:
    try:
        return json.loads(path.read_text(encoding="utf-8")), []
    except OSError as exc:
        return None, [f"{path}: failed to read: {exc}"]
    except json.JSONDecodeError as exc:
        return None, [f"{path}: invalid JSON: {exc}"]


def _is_non_empty(value: Any) -> bool:
    if isinstance(value, str):
        return bool(value.strip())
    if isinstance(value, list):
        return len(value) > 0 and all(_is_non_empty(item) for item in value)
    if isinstance(value, dict):
        return bool(value)
    return value is not None


def _validate_list_field(*, value: Any, label: str, field: str) -> list[str]:
    failures: list[str] = []
    if not isinstance(value, list):
        return [f"{label}: {field} must be a non-empty list"]
    if not value:
        failures.append(f"{label}: {field} must be non-empty")
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item.strip():
            failures.append(f"{label}: {field}[{index}] must be a non-empty string")
    return failures


def _validate_required_fields(
    *,
    record: Any,
    required_fields: set[str],
    list_fields: set[str],
    label: str,
) -> list[str]:
    failures: list[str] = []
    if not isinstance(record, dict):
        return [f"{label}: record must be an object"]

    missing = sorted(required_fields - set(record))
    if missing:
        failures.append(f"{label}: missing required field(s): {', '.join(missing)}")

    for field in sorted((required_fields | list_fields) & set(record)):
        value = record[field]
        if field in list_fields:
            failures.extend(_validate_list_field(value=value, label=label, field=field))
        elif not _is_non_empty(value):
            failures.append(f"{label}: {field} must be non-empty")

    return failures


def _parse_ytree_actions(header_path: Path) -> tuple[list[str], list[str]]:
    try:
        source = header_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{header_path}: failed to read: {exc}"]

    match = re.search(r"typedef\s+enum\s*\{(?P<body>.*?)\}\s*YtreeAction\s*;", source, re.S)
    if match is None:
        return [], [f"{header_path}: failed to find YtreeAction enum"]

    body = re.sub(r"/\*.*?\*/", "", match.group("body"), flags=re.S)
    body = re.sub(r"//.*", "", body)
    actions: list[str] = []
    for item in body.split(","):
        action = item.split("=", 1)[0].strip()
        if not action:
            continue
        if not re.fullmatch(r"ACTION_[A-Z0-9_]+", action):
            return [], [f"{header_path}: invalid YtreeAction enum member: {action}"]
        actions.append(action)

    if not actions:
        return [], [f"{header_path}: YtreeAction enum must not be empty"]
    return actions, []


def validate_contract(
    transitions_path: Path,
    shims_path: Path,
    action_coverage_path: Path,
    actions_header_path: Path,
) -> list[str]:
    failures: list[str] = []
    transitions_doc, transition_load_failures = _load_json(transitions_path)
    shims_doc, shim_load_failures = _load_json(shims_path)
    action_coverage_doc, action_coverage_load_failures = _load_json(action_coverage_path)
    enum_actions, enum_failures = _parse_ytree_actions(actions_header_path)
    failures.extend(transition_load_failures)
    failures.extend(shim_load_failures)
    failures.extend(action_coverage_load_failures)
    failures.extend(enum_failures)
    if failures:
        return failures

    if not isinstance(transitions_doc, dict):
        failures.append(f"{transitions_path}: top-level value must be an object")
        transitions = []
    else:
        transitions = transitions_doc.get("transitions")
        if not isinstance(transitions, list) or not transitions:
            failures.append(f"{transitions_path}: transitions must be a non-empty list")
            transitions = []

    transition_ids: dict[str, dict[str, Any]] = {}
    categories: set[str] = set()
    for index, record in enumerate(transitions):
        label = f"transition[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_TRANSITION_FIELDS,
                list_fields=LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        transition_id = record.get("id")
        if isinstance(transition_id, str) and transition_id.strip():
            if transition_id in transition_ids:
                failures.append(f"{label}: duplicate id: {transition_id}")
            transition_ids[transition_id] = record
        category = record.get("category")
        if isinstance(category, str) and category.strip():
            categories.add(category)

    missing_categories = sorted(REQUIRED_TRANSITION_CATEGORIES - categories)
    if missing_categories:
        failures.append(
            "transition matrix missing required category/categories: "
            + ", ".join(missing_categories)
        )

    if not isinstance(shims_doc, dict):
        failures.append(f"{shims_path}: top-level value must be an object")
        shims = []
    else:
        shims = shims_doc.get("shims")
        if not isinstance(shims, list) or not shims:
            failures.append(f"{shims_path}: shims must be a non-empty list")
            shims = []

    shim_ids: set[str] = set()
    for index, record in enumerate(shims):
        label = f"shim[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_SHIM_FIELDS,
                list_fields=LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        shim_id = record.get("id")
        if isinstance(shim_id, str) and shim_id.strip():
            if shim_id in shim_ids:
                failures.append(f"{label}: duplicate id: {shim_id}")
            shim_ids.add(shim_id)
        target_transition = record.get("target_transition")
        if isinstance(target_transition, str) and target_transition.strip():
            if target_transition not in transition_ids:
                failures.append(
                    f"{label}: target_transition does not match a transition id: {target_transition}"
                )

    if not isinstance(action_coverage_doc, dict):
        failures.append(f"{action_coverage_path}: top-level value must be an object")
        action_records = []
    else:
        action_records = action_coverage_doc.get("actions")
        if not isinstance(action_records, list) or not action_records:
            failures.append(f"{action_coverage_path}: actions must be a non-empty list")
            action_records = []

    expected_actions = set(enum_actions)
    covered_actions: set[str] = set()
    for index, record in enumerate(action_records):
        label = f"action[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_ACTION_FIELDS,
                list_fields=LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue

        action = record.get("action")
        if isinstance(action, str) and action.strip():
            if action in covered_actions:
                failures.append(f"{label}: duplicate action: {action}")
            covered_actions.add(action)
            if action not in expected_actions:
                failures.append(f"{label}: unknown YtreeAction enum member: {action}")

        transition_id = record.get("transition_id")
        transition_record = None
        if isinstance(transition_id, str) and transition_id.strip():
            transition_record = transition_ids.get(transition_id)
            if transition_record is None:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )

        category = record.get("category")
        if (
            isinstance(category, str)
            and category.strip()
            and transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )

    missing_actions = sorted(expected_actions - covered_actions)
    if missing_actions:
        failures.append(
            "action coverage missing YtreeAction enum member(s): "
            + ", ".join(missing_actions)
        )

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transitions", type=Path, default=DEFAULT_TRANSITIONS)
    parser.add_argument("--shims", type=Path, default=DEFAULT_SHIMS)
    parser.add_argument("--action-coverage", type=Path, default=DEFAULT_ACTION_COVERAGE)
    parser.add_argument("--actions-header", type=Path, default=DEFAULT_ACTION_HEADER)
    args = parser.parse_args()

    failures = validate_contract(
        args.transitions,
        args.shims,
        args.action_coverage,
        args.actions_header,
    )
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: AppState contract guard failed ({len(failures)} issue(s))")
        return 1
    print("PASS: AppState contract guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
