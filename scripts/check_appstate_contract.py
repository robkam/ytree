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
DEFAULT_EVENT_COVERAGE = REPO_ROOT / "docs" / "appstate_event_coverage.json"
DEFAULT_OWNER_FIELDS = REPO_ROOT / "docs" / "appstate_owner_fields.json"
DEFAULT_DISPATCH_SURFACES = REPO_ROOT / "docs" / "appstate_dispatch_surfaces.json"
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

REQUIRED_EVENT_CLASSES = {
    "terminal_resize_signal",
    "refresh_rebuild",
    "rebuild_rebind_callback",
    "filesystem_mutation_result",
    "watcher_live_refresh",
    "command_completion",
    "modal_completion",
    "volume_lifecycle",
    "render_reflow",
}

REQUIRED_DISPATCH_SURFACE_CATEGORIES = {
    "key_decode_input_dispatch",
    "directory_window_action_dispatch",
    "file_window_action_dispatch",
    "menu_modal_completion",
    "resize_signal_handling",
    "refresh_rebuild_rebind",
    "filesystem_mutation_result",
    "volume_operation",
    "watcher_live_refresh",
    "render_reflow_projection",
}

REQUIRED_DISPATCH_SURFACE_FIELDS = {
    "surface_id",
    "category",
    "source_path",
    "entry_symbol_or_path",
    "transition_id",
    "boundary_status",
    "allowed_direct_writes",
    "migration_notes",
}

REQUIRED_EVENT_FIELDS = {
    "event_id",
    "event_class",
    "transition_id",
    "category",
    "source",
    "owner",
    "declared_write_set",
    "boundary_status",
    "trigger_paths",
    "migration_notes",
}

REQUIRED_OWNER_FIELDS = {
    "field",
    "owner_region",
    "canonical_owner",
    "runtime_carrier",
    "mutation_rule",
    "migration_status",
    "invariant_checks",
}

LIST_FIELDS = {
    "declared_write_set",
    "side_effects",
    "invariant_checks",
    "migration_notes",
}

EVENT_LIST_FIELDS = LIST_FIELDS | {"trigger_paths"}
DISPATCH_LIST_FIELDS = {"migration_notes"}


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


def _validate_required_string_list(
    *,
    value: Any,
    required_values: set[str],
    label: str,
    item_label: str,
) -> list[str]:
    failures = _validate_list_field(value=value, label=label, field=item_label)
    if failures:
        return failures

    seen: set[str] = set()
    declared = set()
    assert isinstance(value, list)
    for index, item in enumerate(value):
        assert isinstance(item, str)
        if item in seen:
            failures.append(f"{label}: duplicate {item_label}[{index}]: {item}")
        seen.add(item)
        declared.add(item)

    missing_values = sorted(required_values - declared)
    if missing_values:
        failures.append(f"{label}: missing required value(s): {', '.join(missing_values)}")

    unknown_values = sorted(declared - required_values)
    if unknown_values:
        failures.append(f"{label}: unknown value(s): {', '.join(unknown_values)}")

    return failures


def _validate_owner_fields(
    *,
    owner_fields_doc: Any,
    owner_fields_path: Path,
) -> tuple[set[str], list[str]]:
    failures: list[str] = []
    registered_fields: set[str] = set()
    if not isinstance(owner_fields_doc, dict):
        failures.append(f"{owner_fields_path}: top-level value must be an object")
        return registered_fields, failures

    owner_records = owner_fields_doc.get("owner_fields")
    if not isinstance(owner_records, list) or not owner_records:
        failures.append(f"{owner_fields_path}: owner_fields must be a non-empty list")
        owner_records = []

    for index, record in enumerate(owner_records):
        label = f"owner_field[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_OWNER_FIELDS,
                list_fields={"invariant_checks"},
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue

        field = record.get("field")
        if isinstance(field, str) and field.strip():
            if field in registered_fields:
                failures.append(f"{label}: duplicate field: {field}")
            registered_fields.add(field)

    return registered_fields, failures


def _validate_registered_write_set(
    *,
    record: dict[str, Any],
    registered_fields: set[str],
    label: str,
) -> list[str]:
    write_set = record.get("declared_write_set")
    if not isinstance(write_set, list):
        return []

    failures: list[str] = []
    for field in write_set:
        if isinstance(field, str) and field.strip() and field not in registered_fields:
            failures.append(
                f"{label}: declared_write_set references unregistered owner field: {field}"
            )
    return failures


def _validate_allowed_direct_writes(
    *,
    record: dict[str, Any],
    registered_fields: set[str],
    label: str,
) -> list[str]:
    writes = record.get("allowed_direct_writes")
    if not isinstance(writes, list):
        return [f"{label}: allowed_direct_writes must be a list"]

    failures: list[str] = []
    seen: set[str] = set()
    for index, field in enumerate(writes):
        if not isinstance(field, str) or not field.strip():
            failures.append(
                f"{label}: allowed_direct_writes[{index}] must be a non-empty string"
            )
            continue
        if field in seen:
            failures.append(
                f"{label}: duplicate allowed_direct_writes[{index}]: {field}"
            )
        seen.add(field)
        if field not in registered_fields:
            failures.append(
                f"{label}: allowed_direct_writes references unregistered owner field: {field}"
            )
    return failures


def _validate_source_path(value: Any, *, label: str) -> list[str]:
    if not isinstance(value, str) or not value.strip():
        return [f"{label}: source_path must be a non-empty string"]

    source_path = value.strip()
    path = Path(source_path)
    if (
        path.is_absolute()
        or "\\" in source_path
        or any(part == ".." for part in path.parts)
    ):
        return [f"{label}: source_path must be a relative repository path"]

    if not (REPO_ROOT / source_path).is_file():
        return [f"{label}: source_path does not exist: {source_path}"]
    return []


def _validate_entry_symbol_or_path(value: Any, *, label: str) -> list[str]:
    if not isinstance(value, str) or not value.strip():
        return [f"{label}: entry_symbol_or_path must be a non-empty string"]

    entry = value.strip()
    path = Path(entry)
    if (
        path.is_absolute()
        or "\\" in entry
        or any(part == ".." for part in path.parts)
        or re.search(r"\s", entry)
    ):
        return [f"{label}: entry_symbol_or_path is malformed: {entry}"]
    return []


def _validate_dispatch_surfaces(
    *,
    dispatch_surfaces_doc: Any,
    dispatch_surfaces_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(dispatch_surfaces_doc, dict):
        failures.append(f"{dispatch_surfaces_path}: top-level value must be an object")
        return failures

    surface_records = dispatch_surfaces_doc.get("dispatch_surfaces")
    if not isinstance(surface_records, list) or not surface_records:
        failures.append(
            f"{dispatch_surfaces_path}: dispatch_surfaces must be a non-empty list"
        )
        surface_records = []

    surface_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(surface_records):
        label = f"dispatch_surface[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_DISPATCH_SURFACE_FIELDS
                - {"allowed_direct_writes"},
                list_fields=DISPATCH_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        if "allowed_direct_writes" not in record:
            failures.append(f"{label}: missing required field(s): allowed_direct_writes")
        else:
            failures.extend(
                _validate_allowed_direct_writes(
                    record=record,
                    registered_fields=registered_owner_fields,
                    label=label,
                )
            )

        surface_id = record.get("surface_id")
        if isinstance(surface_id, str) and surface_id.strip():
            if surface_id in surface_ids:
                failures.append(f"{label}: duplicate surface_id: {surface_id}")
            surface_ids.add(surface_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_DISPATCH_SURFACE_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        transition_id = record.get("transition_id")
        if isinstance(transition_id, str) and transition_id.strip():
            if transition_id not in transition_ids:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )

        failures.extend(_validate_source_path(record.get("source_path"), label=label))
        failures.extend(
            _validate_entry_symbol_or_path(
                record.get("entry_symbol_or_path"), label=label
            )
        )

    missing_categories = sorted(
        REQUIRED_DISPATCH_SURFACE_CATEGORIES - covered_categories
    )
    if missing_categories:
        failures.append(
            "dispatch surfaces missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return failures


def _validate_event_coverage(
    *,
    event_coverage_doc: Any,
    event_coverage_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(event_coverage_doc, dict):
        failures.append(f"{event_coverage_path}: top-level value must be an object")
        return failures

    required_event_classes = event_coverage_doc.get("required_event_classes")
    failures.extend(
        _validate_required_string_list(
            value=required_event_classes,
            required_values=REQUIRED_EVENT_CLASSES,
            label="event coverage required_event_classes",
            item_label="required_event_classes",
        )
    )

    event_records = event_coverage_doc.get("events")
    if not isinstance(event_records, list) or not event_records:
        failures.append(f"{event_coverage_path}: events must be a non-empty list")
        event_records = []

    event_ids: set[str] = set()
    covered_event_classes: set[str] = set()
    for index, record in enumerate(event_records):
        label = f"event[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_EVENT_FIELDS,
                list_fields=EVENT_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )

        event_id = record.get("event_id")
        if isinstance(event_id, str) and event_id.strip():
            if event_id in event_ids:
                failures.append(f"{label}: duplicate event_id: {event_id}")
            event_ids.add(event_id)

        event_class = record.get("event_class")
        if isinstance(event_class, str) and event_class.strip():
            if event_class in covered_event_classes:
                failures.append(f"{label}: duplicate event_class: {event_class}")
            covered_event_classes.add(event_class)
            if event_class not in REQUIRED_EVENT_CLASSES:
                failures.append(f"{label}: unknown event_class: {event_class}")

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

    missing_event_classes = sorted(REQUIRED_EVENT_CLASSES - covered_event_classes)
    if missing_event_classes:
        failures.append(
            "event coverage missing required event_class(es): "
            + ", ".join(missing_event_classes)
        )

    return failures


def validate_contract(
    transitions_path: Path,
    shims_path: Path,
    action_coverage_path: Path,
    actions_header_path: Path,
    event_coverage_path: Path = DEFAULT_EVENT_COVERAGE,
    owner_fields_path: Path = DEFAULT_OWNER_FIELDS,
    dispatch_surfaces_path: Path = DEFAULT_DISPATCH_SURFACES,
) -> list[str]:
    failures: list[str] = []
    transitions_doc, transition_load_failures = _load_json(transitions_path)
    shims_doc, shim_load_failures = _load_json(shims_path)
    action_coverage_doc, action_coverage_load_failures = _load_json(action_coverage_path)
    event_coverage_doc, event_coverage_load_failures = _load_json(event_coverage_path)
    owner_fields_doc, owner_fields_load_failures = _load_json(owner_fields_path)
    dispatch_surfaces_doc, dispatch_surfaces_load_failures = _load_json(
        dispatch_surfaces_path
    )
    enum_actions, enum_failures = _parse_ytree_actions(actions_header_path)
    failures.extend(transition_load_failures)
    failures.extend(shim_load_failures)
    failures.extend(action_coverage_load_failures)
    failures.extend(event_coverage_load_failures)
    failures.extend(owner_fields_load_failures)
    failures.extend(dispatch_surfaces_load_failures)
    failures.extend(enum_failures)
    if failures:
        return failures

    registered_owner_fields, owner_field_failures = _validate_owner_fields(
        owner_fields_doc=owner_fields_doc,
        owner_fields_path=owner_fields_path,
    )
    failures.extend(owner_field_failures)

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
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
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
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )

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

    failures.extend(
        _validate_event_coverage(
            event_coverage_doc=event_coverage_doc,
            event_coverage_path=event_coverage_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
        )
    )
    failures.extend(
        _validate_dispatch_surfaces(
            dispatch_surfaces_doc=dispatch_surfaces_doc,
            dispatch_surfaces_path=dispatch_surfaces_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
        )
    )

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transitions", type=Path, default=DEFAULT_TRANSITIONS)
    parser.add_argument("--shims", type=Path, default=DEFAULT_SHIMS)
    parser.add_argument("--action-coverage", type=Path, default=DEFAULT_ACTION_COVERAGE)
    parser.add_argument("--event-coverage", type=Path, default=DEFAULT_EVENT_COVERAGE)
    parser.add_argument("--owner-fields", type=Path, default=DEFAULT_OWNER_FIELDS)
    parser.add_argument(
        "--dispatch-surfaces", type=Path, default=DEFAULT_DISPATCH_SURFACES
    )
    parser.add_argument("--actions-header", type=Path, default=DEFAULT_ACTION_HEADER)
    args = parser.parse_args()

    failures = validate_contract(
        args.transitions,
        args.shims,
        args.action_coverage,
        args.actions_header,
        args.event_coverage,
        args.owner_fields,
        args.dispatch_surfaces,
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
