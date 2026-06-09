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
DEFAULT_INVARIANTS = REPO_ROOT / "docs" / "appstate_invariants.json"
DEFAULT_GENERATION_DOMAINS = REPO_ROOT / "docs" / "appstate_generation_domains.json"
DEFAULT_DIFF_HARNESS = REPO_ROOT / "docs" / "appstate_diff_harness.json"
DEFAULT_TRANSITION_SEQUENCES = REPO_ROOT / "docs" / "appstate_transition_sequences.json"
DEFAULT_ACTION_HEADER = REPO_ROOT / "include" / "ytree_defs.h"
DEFAULT_ACTION_RUNTIME = REPO_ROOT / "src" / "core" / "appstate_actions.c"

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

REQUIRED_INVARIANT_CATEGORIES = {
    "inactive_panel_frozen",
    "render_projection_read_only",
    "hidden_entry_visible_navigation",
    "panel_local_focus_restore",
    "viewport_identity_rebind",
    "shared_state_panel_local_isolation",
    "stale_snapshot_fail_closed",
    "blocked_transition_determinism",
}

REQUIRED_GENERATION_DOMAIN_CATEGORIES = {
    "panel_generation",
    "volume_generation",
    "directory_identity",
    "file_identity",
    "focus_shape",
    "modal_command_target",
    "visibility_filter_state",
    "topology_state",
    "file_payload_state",
    "volume_lifecycle",
    "layout_reflow",
}

REQUIRED_DIFF_HARNESS_CATEGORIES = {
    "transition_before_after_snapshot",
    "declared_write_set_diff",
    "render_projection_read_only_diff",
    "generation_mismatch_check",
    "blocked_transition_no_unrelated_mutation",
}

REQUIRED_SEQUENCE_CATEGORIES = {
    "directory_file_transition",
    "display_mode",
    "filesystem_mutation",
    "layout_split",
    "modal_command",
    "panel_navigation",
    "refresh_rebuild",
    "search_jump",
    "visibility_filter",
    "volume_lifecycle",
}

REQUIRED_SEQUENCE_FLOWS = {
    "dotfile_reveal_conceal",
    "enter_directory_file_transition",
    "esc_modal_dismissal",
    "file_small_big_transitions",
    "filesystem_mutation_result",
    "refresh_rebuild",
    "search_jump",
    "showall_global_tagged_only",
    "split_close_reopen",
    "split_toggle_f8",
    "tab_panel_switch",
    "volume_cycling_release",
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

REQUIRED_INVARIANT_FIELDS = {
    "invariant_id",
    "category",
    "owner_region",
    "protected_fields",
    "transition_ids",
    "dispatch_surface_ids",
    "failure_mode",
    "enforcement_status",
    "test_strategy",
    "migration_notes",
}

REQUIRED_GENERATION_DOMAIN_FIELDS = {
    "domain_id",
    "category",
    "owner_region",
    "generation_owner_field",
    "identity_fields",
    "advances_on_transition_ids",
    "stale_snapshot_policy",
    "fail_closed_fallback",
    "restore_boundary",
    "enforcement_status",
    "migration_notes",
}

REQUIRED_DIFF_HARNESS_FIELDS = {
    "harness_id",
    "check_category",
    "snapshot_phases",
    "snapshot_regions",
    "transition_ids",
    "owner_field_refs",
    "invariant_ids",
    "generation_domain_ids",
    "expected_behavior",
    "failure_mode",
    "enforcement_status",
    "migration_notes",
}

REQUIRED_SEQUENCE_FIELDS = {
    "scenario_id",
    "category",
    "flow",
    "description",
    "steps",
}

REQUIRED_SEQUENCE_STEP_FIELDS = {
    "ordinal",
    "step_id",
    "transition_id",
    "stimulus",
    "expected_result",
    "invariant_ids",
    "diff_harness_ids",
    "generation_domain_expectations",
}

REQUIRED_GENERATION_EXPECTATION_FIELDS = {
    "domain_id",
    "expectation",
}

REQUIRED_FALLBACK_FIELDS = {
    "outcome",
    "allowed_mutation_scope",
}

REQUIRED_NO_UNRELATED_MUTATION_FIELDS = {
    "diff_harness_id",
    "expectation",
}

LIST_FIELDS = {
    "declared_write_set",
    "side_effects",
    "invariant_checks",
    "migration_notes",
}

EVENT_LIST_FIELDS = LIST_FIELDS | {"trigger_paths"}
DISPATCH_LIST_FIELDS = {"migration_notes"}
INVARIANT_LIST_FIELDS = {
    "protected_fields",
    "transition_ids",
    "migration_notes",
}
GENERATION_DOMAIN_LIST_FIELDS = {"identity_fields", "migration_notes"}
DIFF_HARNESS_LIST_FIELDS = {
    "snapshot_phases",
    "snapshot_regions",
    "transition_ids",
    "owner_field_refs",
    "invariant_ids",
    "generation_domain_ids",
    "migration_notes",
}
SEQUENCE_LIST_FIELDS = {
    "invariant_ids",
    "diff_harness_ids",
    "generation_domain_expectations",
}
GENERATION_FIELD_RE = re.compile(r"\b(?:[A-Za-z0-9_]+\.)?[A-Za-z0-9_]+_generation\b")


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


def _parse_runtime_action_transitions(
    runtime_path: Path,
) -> tuple[list[dict[str, str]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    match = re.search(
        r"kAppStateActionTransitions\s*\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime action transition table"]

    records: list[dict[str, str]] = []
    row_re = re.compile(
        r"\{\s*(ACTION_[A-Z0-9_]+)\s*,\s*\"([^\"]+)\"\s*,\s*\"([^\"]+)\"\s*\}"
    )
    for row_match in row_re.finditer(match.group("body")):
        records.append(
            {
                "action": row_match.group(1),
                "transition_id": row_match.group(2),
                "category": row_match.group(3),
            }
        )

    if not records:
        return [], [f"{runtime_path}: runtime action transition table must not be empty"]
    return records, []


def _validate_runtime_action_lookup(
    *,
    runtime_records: list[dict[str, str]],
    runtime_path: Path,
    enum_actions: list[str],
    action_coverage_by_action: dict[str, dict[str, Any]],
    transition_ids: dict[str, dict[str, Any]],
) -> list[str]:
    failures: list[str] = []
    expected_actions = set(enum_actions)
    covered_actions: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_action[{index}]"
        action = record["action"]
        if action in covered_actions:
            failures.append(f"{label}: duplicate runtime action: {action}")
        covered_actions.add(action)
        if action not in expected_actions:
            failures.append(f"{label}: unknown YtreeAction enum member: {action}")
        elif index >= len(enum_actions) or action != enum_actions[index]:
            expected = enum_actions[index] if index < len(enum_actions) else "<none>"
            failures.append(
                f"{label}: runtime row order does not match YtreeAction enum: "
                f"expected {expected}, found {action}"
            )

        transition_id = record["transition_id"]
        transition_record = transition_ids.get(transition_id)
        if transition_record is None:
            failures.append(
                f"{label}: transition_id does not match a transition id: {transition_id}"
            )

        category = record["category"]
        if (
            transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )

        coverage_record = action_coverage_by_action.get(action)
        if coverage_record is None:
            failures.append(
                f"{label}: runtime action missing from action coverage: {action}"
            )
            continue
        if transition_id != coverage_record.get("transition_id"):
            failures.append(
                f"{label}: runtime transition_id does not match action coverage "
                f"for {action}: {transition_id}"
            )
        if category != coverage_record.get("category"):
            failures.append(
                f"{label}: runtime category does not match action coverage "
                f"for {action}: {category}"
            )

    missing_actions = sorted(expected_actions - covered_actions)
    if missing_actions:
        failures.append(
            f"{runtime_path}: runtime action lookup missing YtreeAction enum member(s): "
            + ", ".join(missing_actions)
        )

    return failures


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


def _collect_string_ids(doc: Any, *, collection_key: str, id_field: str) -> set[str]:
    if not isinstance(doc, dict):
        return set()
    records = doc.get(collection_key)
    if not isinstance(records, list):
        return set()
    return {
        value
        for record in records
        if isinstance(record, dict)
        for value in [record.get(id_field)]
        if isinstance(value, str) and value.strip()
    }


def _collect_transition_ids_by_string_id(
    doc: Any, *, collection_key: str, id_field: str
) -> dict[str, str]:
    if not isinstance(doc, dict):
        return {}
    records = doc.get(collection_key)
    if not isinstance(records, list):
        return {}

    transition_ids: dict[str, str] = {}
    for record in records:
        if not isinstance(record, dict):
            continue
        value = record.get(id_field)
        transition_id = record.get("transition_id")
        if (
            isinstance(value, str)
            and value.strip()
            and isinstance(transition_id, str)
            and transition_id.strip()
        ):
            transition_ids[value] = transition_id
    return transition_ids


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


def _validate_appstate_diff_harness(
    *,
    diff_harness_doc: Any,
    diff_harness_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
    invariant_ids: set[str],
    generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(diff_harness_doc, dict):
        failures.append(f"{diff_harness_path}: top-level value must be an object")
        return failures

    records = diff_harness_doc.get("diff_harness_checks")
    if not isinstance(records, list) or not records:
        failures.append(
            f"{diff_harness_path}: diff_harness_checks must be a non-empty list"
        )
        records = []

    harness_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(records):
        label = f"diff_harness_check[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_DIFF_HARNESS_FIELDS,
                list_fields=DIFF_HARNESS_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue

        harness_id = record.get("harness_id")
        if isinstance(harness_id, str) and harness_id.strip():
            if harness_id in harness_ids:
                failures.append(f"{label}: duplicate harness_id: {harness_id}")
            harness_ids.add(harness_id)

        check_category = record.get("check_category")
        if isinstance(check_category, str) and check_category.strip():
            covered_categories.add(check_category)
            if check_category not in REQUIRED_DIFF_HARNESS_CATEGORIES:
                failures.append(f"{label}: unknown check_category: {check_category}")

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids references unknown transition id: {transition_id}"
                    )

        owner_field_refs = record.get("owner_field_refs")
        if isinstance(owner_field_refs, list):
            for field in owner_field_refs:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: owner_field_refs references unregistered owner field: {field}"
                    )

        invariant_refs = record.get("invariant_ids")
        if isinstance(invariant_refs, list):
            for invariant_id in invariant_refs:
                if (
                    isinstance(invariant_id, str)
                    and invariant_id.strip()
                    and invariant_id not in invariant_ids
                ):
                    failures.append(
                        f"{label}: invariant_ids references unknown invariant id: {invariant_id}"
                    )

        generation_domain_refs = record.get("generation_domain_ids")
        if isinstance(generation_domain_refs, list):
            for domain_id in generation_domain_refs:
                if (
                    isinstance(domain_id, str)
                    and domain_id.strip()
                    and domain_id not in generation_domain_ids
                ):
                    failures.append(
                        f"{label}: generation_domain_ids references unknown generation domain id: {domain_id}"
                    )

    missing_categories = sorted(REQUIRED_DIFF_HARNESS_CATEGORIES - covered_categories)
    if missing_categories:
        failures.append(
            "diff harness missing required check_category/categories: "
            + ", ".join(missing_categories)
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


def _validate_invariants(
    *,
    invariants_doc: Any,
    invariants_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
    dispatch_surface_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(invariants_doc, dict):
        failures.append(f"{invariants_path}: top-level value must be an object")
        return failures

    invariant_records = invariants_doc.get("invariants")
    if not isinstance(invariant_records, list) or not invariant_records:
        failures.append(f"{invariants_path}: invariants must be a non-empty list")
        invariant_records = []

    invariant_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(invariant_records):
        label = f"invariant[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_INVARIANT_FIELDS - {"dispatch_surface_ids"},
                list_fields=INVARIANT_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        if "dispatch_surface_ids" not in record:
            failures.append(f"{label}: missing required field(s): dispatch_surface_ids")
        else:
            surface_refs_value = record.get("dispatch_surface_ids")
            if not isinstance(surface_refs_value, list):
                failures.append(
                    f"{label}: dispatch_surface_ids must be a list"
                )
            else:
                for surface_index, surface_id in enumerate(surface_refs_value):
                    if not isinstance(surface_id, str) or not surface_id.strip():
                        failures.append(
                            f"{label}: dispatch_surface_ids[{surface_index}] must be a non-empty string"
                        )

        invariant_id = record.get("invariant_id")
        if isinstance(invariant_id, str) and invariant_id.strip():
            if invariant_id in invariant_ids:
                failures.append(f"{label}: duplicate invariant_id: {invariant_id}")
            invariant_ids.add(invariant_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_INVARIANT_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        protected_fields = record.get("protected_fields")
        if isinstance(protected_fields, list):
            for field in protected_fields:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: protected_fields references unregistered owner field: {field}"
                    )

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids references unknown transition id: {transition_id}"
                    )

        surface_refs = record.get("dispatch_surface_ids")
        if isinstance(surface_refs, list):
            if not surface_refs:
                migration_notes = record.get("migration_notes")
                has_cross_cutting_note = (
                    isinstance(migration_notes, list)
                    and any(
                        isinstance(note, str)
                        and "cross-cutting" in note.lower()
                        for note in migration_notes
                    )
                )
                if not has_cross_cutting_note:
                    failures.append(
                        f"{label}: empty dispatch_surface_ids requires a cross-cutting migration_notes explanation"
                    )
            for surface_id in surface_refs:
                if (
                    isinstance(surface_id, str)
                    and surface_id.strip()
                    and surface_id not in dispatch_surface_ids
                ):
                    failures.append(
                        f"{label}: dispatch_surface_ids references unknown dispatch surface id: {surface_id}"
                    )

    missing_categories = sorted(REQUIRED_INVARIANT_CATEGORIES - covered_categories)
    if missing_categories:
        failures.append(
            "invariants missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return failures


def _validate_generation_transition_refs(
    *,
    value: Any,
    label: str,
    transition_ids: dict[str, dict[str, Any]],
    migration_notes: Any,
) -> list[str]:
    failures: list[str] = []
    field = "advances_on_transition_ids"
    if not isinstance(value, list):
        return [f"{label}: {field} must be a list"]

    for index, transition_id in enumerate(value):
        if not isinstance(transition_id, str) or not transition_id.strip():
            failures.append(f"{label}: {field}[{index}] must be a non-empty string")
            continue
        if transition_id not in transition_ids:
            failures.append(
                f"{label}: {field} references unknown transition id: {transition_id}"
            )

    if not value:
        has_projection_note = (
            isinstance(migration_notes, list)
            and any(
                isinstance(note, str)
                and (
                    "read-only" in note.lower()
                    or "projection-only" in note.lower()
                )
                for note in migration_notes
            )
        )
        if not has_projection_note:
            failures.append(
                f"{label}: empty {field} requires a read-only/projection-only migration_notes explanation"
            )

    return failures


def _validate_generation_domains(
    *,
    generation_domains_doc: Any,
    generation_domains_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
) -> tuple[set[str], list[str]]:
    failures: list[str] = []
    generation_owner_fields: set[str] = set()
    if not isinstance(generation_domains_doc, dict):
        failures.append(f"{generation_domains_path}: top-level value must be an object")
        return generation_owner_fields, failures

    domain_records = generation_domains_doc.get("generation_domains")
    if not isinstance(domain_records, list) or not domain_records:
        failures.append(
            f"{generation_domains_path}: generation_domains must be a non-empty list"
        )
        domain_records = []

    domain_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(domain_records):
        label = f"generation_domain[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_GENERATION_DOMAIN_FIELDS
                - {"advances_on_transition_ids"},
                list_fields=GENERATION_DOMAIN_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue

        if "advances_on_transition_ids" not in record:
            failures.append(
                f"{label}: missing required field(s): advances_on_transition_ids"
            )
        else:
            failures.extend(
                _validate_generation_transition_refs(
                    value=record.get("advances_on_transition_ids"),
                    label=label,
                    transition_ids=transition_ids,
                    migration_notes=record.get("migration_notes"),
                )
            )

        domain_id = record.get("domain_id")
        if isinstance(domain_id, str) and domain_id.strip():
            if domain_id in domain_ids:
                failures.append(f"{label}: duplicate domain_id: {domain_id}")
            domain_ids.add(domain_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_GENERATION_DOMAIN_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        generation_owner_field = record.get("generation_owner_field")
        if isinstance(generation_owner_field, str) and generation_owner_field.strip():
            generation_owner_fields.add(generation_owner_field)
            if generation_owner_field not in registered_owner_fields:
                failures.append(
                    f"{label}: generation_owner_field references unregistered owner field: {generation_owner_field}"
                )

        identity_fields = record.get("identity_fields")
        if isinstance(identity_fields, list):
            for field in identity_fields:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: identity_fields references unregistered owner field: {field}"
                    )

    missing_categories = sorted(
        REQUIRED_GENERATION_DOMAIN_CATEGORIES - covered_categories
    )
    if missing_categories:
        failures.append(
            "generation domains missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return generation_owner_fields, failures


def _validate_transition_generation_effects(
    *,
    transitions: list[Any],
    generation_owner_fields: set[str],
) -> list[str]:
    failures: list[str] = []
    registered_names = set(generation_owner_fields)
    registered_names.update(field.rsplit(".", 1)[-1] for field in generation_owner_fields)

    for index, record in enumerate(transitions):
        if not isinstance(record, dict):
            continue
        effect = record.get("generation_effect")
        if not isinstance(effect, str):
            continue
        for field in sorted(set(GENERATION_FIELD_RE.findall(effect))):
            if field not in registered_names:
                failures.append(
                    f"transition[{index}]: generation_effect names unregistered generation field: {field}"
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


def _validate_step_stimulus(
    *,
    stimulus: Any,
    label: str,
    transition_id: Any,
    action_ids: set[str],
    action_transition_ids: dict[str, str],
    event_ids: set[str],
    event_transition_ids: dict[str, str],
) -> list[str]:
    if not isinstance(stimulus, dict) or not stimulus:
        return [f"{label}: stimulus must be a non-empty object"]

    failures: list[str] = []
    action_id = stimulus.get("action_id")
    event_id = stimulus.get("event_id")
    if action_id is None and event_id is None:
        failures.append(f"{label}: stimulus must include action_id or event_id")

    if action_id is not None:
        if not isinstance(action_id, str) or not action_id.strip():
            failures.append(f"{label}: stimulus.action_id must be a non-empty string")
        elif action_id not in action_ids:
            failures.append(f"{label}: stimulus.action_id references unknown action: {action_id}")
        elif isinstance(transition_id, str) and transition_id.strip():
            action_transition_id = action_transition_ids.get(action_id)
            if action_transition_id != transition_id:
                failures.append(
                    f"{label}: stimulus.action_id {action_id} maps to transition_id "
                    f"{action_transition_id}, not step.transition_id {transition_id}"
                )

    if event_id is not None:
        if not isinstance(event_id, str) or not event_id.strip():
            failures.append(f"{label}: stimulus.event_id must be a non-empty string")
        elif event_id not in event_ids:
            failures.append(f"{label}: stimulus.event_id references unknown event id: {event_id}")
        elif isinstance(transition_id, str) and transition_id.strip():
            event_transition_id = event_transition_ids.get(event_id)
            if event_transition_id != transition_id:
                failures.append(
                    f"{label}: stimulus.event_id {event_id} maps to transition_id "
                    f"{event_transition_id}, not step.transition_id {transition_id}"
                )

    return failures


def _validate_step_reference_list(
    *,
    value: Any,
    valid_ids: set[str],
    label: str,
    field: str,
    reference_label: str,
) -> list[str]:
    failures = _validate_list_field(value=value, label=label, field=field)
    if failures:
        return failures

    assert isinstance(value, list)
    seen: set[str] = set()
    for index, item in enumerate(value):
        assert isinstance(item, str)
        if item in seen:
            failures.append(f"{label}: duplicate {field}[{index}]: {item}")
        seen.add(item)
        if item not in valid_ids:
            failures.append(f"{label}: {field} references unknown {reference_label}: {item}")
    return failures


def _validate_generation_expectations(
    *,
    value: Any,
    label: str,
    generation_domain_ids: set[str],
) -> list[str]:
    if not isinstance(value, list) or not value:
        return [f"{label}: generation_domain_expectations must be a non-empty list"]

    failures: list[str] = []
    seen: set[str] = set()
    for index, record in enumerate(value):
        expectation_label = f"{label}.generation_domain_expectations[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_GENERATION_EXPECTATION_FIELDS,
                list_fields=set(),
                label=expectation_label,
            )
        )
        if not isinstance(record, dict):
            continue

        domain_id = record.get("domain_id")
        if isinstance(domain_id, str) and domain_id.strip():
            if domain_id in seen:
                failures.append(
                    f"{expectation_label}: duplicate domain_id: {domain_id}"
                )
            seen.add(domain_id)
            if domain_id not in generation_domain_ids:
                failures.append(
                    f"{expectation_label}: domain_id references unknown generation domain id: {domain_id}"
                )

    return failures


def _requires_deterministic_fallback(record: dict[str, Any]) -> bool:
    precondition = record.get("precondition")
    expected_result = record.get("expected_result")
    return (
        precondition in {"generation_mismatch", "stale_snapshot"}
        or expected_result == "fallback"
    )


def _validate_deterministic_fallback(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    fallback = record.get("deterministic_fallback")
    if not isinstance(fallback, dict):
        return [
            f"{label}: stale-snapshot/generation-mismatch steps require deterministic_fallback"
        ]
    return _validate_required_fields(
        record=fallback,
        required_fields=REQUIRED_FALLBACK_FIELDS,
        list_fields=set(),
        label=f"{label}.deterministic_fallback",
    )


def _validate_no_unrelated_mutation(
    *,
    record: dict[str, Any],
    label: str,
    diff_harness_ids: set[str],
) -> list[str]:
    expectation = record.get("no_unrelated_mutation")
    if not isinstance(expectation, dict):
        return [
            f"{label}: blocked/invalid steps require no_unrelated_mutation expectations"
        ]

    failures = _validate_required_fields(
        record=expectation,
        required_fields=REQUIRED_NO_UNRELATED_MUTATION_FIELDS,
        list_fields=set(),
        label=f"{label}.no_unrelated_mutation",
    )
    harness_id = expectation.get("diff_harness_id")
    if isinstance(harness_id, str) and harness_id.strip() and harness_id not in diff_harness_ids:
        failures.append(
            f"{label}.no_unrelated_mutation: diff_harness_id references unknown diff harness id: {harness_id}"
        )
    return failures


def _validate_appstate_transition_sequences(
    *,
    transition_sequences_doc: Any,
    transition_sequences_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    action_ids: set[str],
    action_transition_ids: dict[str, str],
    event_ids: set[str],
    event_transition_ids: dict[str, str],
    invariant_ids: set[str],
    diff_harness_ids: set[str],
    generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(transition_sequences_doc, dict):
        failures.append(f"{transition_sequences_path}: top-level value must be an object")
        return failures

    records = transition_sequences_doc.get("scenarios")
    if not isinstance(records, list) or not records:
        failures.append(f"{transition_sequences_path}: scenarios must be a non-empty list")
        records = []

    scenario_ids: set[str] = set()
    covered_flows: set[str] = set()
    for scenario_index, record in enumerate(records):
        scenario_label = f"transition_sequence[{scenario_index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_SEQUENCE_FIELDS,
                list_fields=set(),
                label=scenario_label,
            )
        )
        if not isinstance(record, dict):
            continue

        scenario_id = record.get("scenario_id")
        if isinstance(scenario_id, str) and scenario_id.strip():
            if scenario_id in scenario_ids:
                failures.append(f"{scenario_label}: duplicate scenario_id: {scenario_id}")
            scenario_ids.add(scenario_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            if category not in REQUIRED_SEQUENCE_CATEGORIES:
                failures.append(f"{scenario_label}: unknown category: {category}")

        flow = record.get("flow")
        if isinstance(flow, str) and flow.strip():
            covered_flows.add(flow)
            if flow not in REQUIRED_SEQUENCE_FLOWS:
                failures.append(f"{scenario_label}: unknown flow: {flow}")

        steps = record.get("steps")
        if not isinstance(steps, list) or not steps:
            failures.append(f"{scenario_label}: steps must be a non-empty list")
            continue

        ordinals: set[int] = set()
        previous_ordinal = 0
        for step_index, step in enumerate(steps):
            label = f"{scenario_label}.step[{step_index}]"
            failures.extend(
                _validate_required_fields(
                    record=step,
                    required_fields=REQUIRED_SEQUENCE_STEP_FIELDS,
                    list_fields=SEQUENCE_LIST_FIELDS - {"generation_domain_expectations"},
                    label=label,
                )
            )
            if not isinstance(step, dict):
                continue

            ordinal = step.get("ordinal")
            if not isinstance(ordinal, int) or ordinal < 1:
                failures.append(f"{label}: ordinal must be a positive integer")
            else:
                if ordinal in ordinals:
                    failures.append(f"{label}: duplicate ordinal: {ordinal}")
                if ordinal <= previous_ordinal:
                    failures.append(f"{label}: ordinal must be greater than previous step ordinal")
                ordinals.add(ordinal)
                previous_ordinal = ordinal

            transition_id = step.get("transition_id")
            if isinstance(transition_id, str) and transition_id.strip():
                if transition_id not in transition_ids:
                    failures.append(
                        f"{label}: transition_id references unknown transition id: {transition_id}"
                    )

            failures.extend(
                _validate_step_stimulus(
                    stimulus=step.get("stimulus"),
                    label=label,
                    transition_id=transition_id,
                    action_ids=action_ids,
                    action_transition_ids=action_transition_ids,
                    event_ids=event_ids,
                    event_transition_ids=event_transition_ids,
                )
            )
            failures.extend(
                _validate_step_reference_list(
                    value=step.get("invariant_ids"),
                    valid_ids=invariant_ids,
                    label=label,
                    field="invariant_ids",
                    reference_label="invariant id",
                )
            )
            failures.extend(
                _validate_step_reference_list(
                    value=step.get("diff_harness_ids"),
                    valid_ids=diff_harness_ids,
                    label=label,
                    field="diff_harness_ids",
                    reference_label="diff harness id",
                )
            )
            failures.extend(
                _validate_generation_expectations(
                    value=step.get("generation_domain_expectations"),
                    label=label,
                    generation_domain_ids=generation_domain_ids,
                )
            )

            expected_result = step.get("expected_result")
            if (
                isinstance(expected_result, str)
                and expected_result.strip()
                and expected_result not in {"allowed", "blocked", "fallback", "invalid"}
            ):
                failures.append(f"{label}: unknown expected_result: {expected_result}")

            if _requires_deterministic_fallback(step):
                failures.extend(
                    _validate_deterministic_fallback(record=step, label=label)
                )

            if expected_result in {"blocked", "invalid"}:
                failures.extend(
                    _validate_no_unrelated_mutation(
                        record=step,
                        label=label,
                        diff_harness_ids=diff_harness_ids,
                    )
                )

    missing_flows = sorted(REQUIRED_SEQUENCE_FLOWS - covered_flows)
    if missing_flows:
        failures.append(
            "transition sequences missing required flow(s): " + ", ".join(missing_flows)
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
    invariants_path: Path = DEFAULT_INVARIANTS,
    generation_domains_path: Path = DEFAULT_GENERATION_DOMAINS,
    diff_harness_path: Path = DEFAULT_DIFF_HARNESS,
    transition_sequences_path: Path = DEFAULT_TRANSITION_SEQUENCES,
    action_runtime_path: Path = DEFAULT_ACTION_RUNTIME,
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
    invariants_doc, invariants_load_failures = _load_json(invariants_path)
    generation_domains_doc, generation_domains_load_failures = _load_json(
        generation_domains_path
    )
    diff_harness_doc, diff_harness_load_failures = _load_json(diff_harness_path)
    transition_sequences_doc, transition_sequences_load_failures = _load_json(
        transition_sequences_path
    )
    enum_actions, enum_failures = _parse_ytree_actions(actions_header_path)
    runtime_action_records, runtime_action_failures = _parse_runtime_action_transitions(
        action_runtime_path
    )
    failures.extend(transition_load_failures)
    failures.extend(shim_load_failures)
    failures.extend(action_coverage_load_failures)
    failures.extend(event_coverage_load_failures)
    failures.extend(owner_fields_load_failures)
    failures.extend(dispatch_surfaces_load_failures)
    failures.extend(invariants_load_failures)
    failures.extend(generation_domains_load_failures)
    failures.extend(diff_harness_load_failures)
    failures.extend(transition_sequences_load_failures)
    failures.extend(enum_failures)
    failures.extend(runtime_action_failures)
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

    generation_owner_fields, generation_domain_failures = _validate_generation_domains(
        generation_domains_doc=generation_domains_doc,
        generation_domains_path=generation_domains_path,
        transition_ids=transition_ids,
        registered_owner_fields=registered_owner_fields,
    )
    failures.extend(generation_domain_failures)
    failures.extend(
        _validate_transition_generation_effects(
            transitions=transitions,
            generation_owner_fields=generation_owner_fields,
        )
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
    action_coverage_by_action: dict[str, dict[str, Any]] = {}
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
            else:
                action_coverage_by_action[action] = record
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
        _validate_runtime_action_lookup(
            runtime_records=runtime_action_records,
            runtime_path=action_runtime_path,
            enum_actions=enum_actions,
            action_coverage_by_action=action_coverage_by_action,
            transition_ids=transition_ids,
        )
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
    dispatch_surface_ids = _collect_string_ids(
        dispatch_surfaces_doc,
        collection_key="dispatch_surfaces",
        id_field="surface_id",
    )
    failures.extend(
        _validate_invariants(
            invariants_doc=invariants_doc,
            invariants_path=invariants_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
            dispatch_surface_ids=dispatch_surface_ids,
        )
    )
    invariant_ids = _collect_string_ids(
        invariants_doc,
        collection_key="invariants",
        id_field="invariant_id",
    )
    generation_domain_ids = _collect_string_ids(
        generation_domains_doc,
        collection_key="generation_domains",
        id_field="domain_id",
    )
    diff_harness_ids = _collect_string_ids(
        diff_harness_doc,
        collection_key="diff_harness_checks",
        id_field="harness_id",
    )
    failures.extend(
        _validate_appstate_diff_harness(
            diff_harness_doc=diff_harness_doc,
            diff_harness_path=diff_harness_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
            invariant_ids=invariant_ids,
            generation_domain_ids=generation_domain_ids,
        )
    )
    action_ids = _collect_string_ids(
        action_coverage_doc,
        collection_key="actions",
        id_field="action",
    )
    action_transition_ids = _collect_transition_ids_by_string_id(
        action_coverage_doc,
        collection_key="actions",
        id_field="action",
    )
    event_ids = _collect_string_ids(
        event_coverage_doc,
        collection_key="events",
        id_field="event_id",
    )
    event_transition_ids = _collect_transition_ids_by_string_id(
        event_coverage_doc,
        collection_key="events",
        id_field="event_id",
    )
    failures.extend(
        _validate_appstate_transition_sequences(
            transition_sequences_doc=transition_sequences_doc,
            transition_sequences_path=transition_sequences_path,
            transition_ids=transition_ids,
            action_ids=action_ids,
            action_transition_ids=action_transition_ids,
            event_ids=event_ids,
            event_transition_ids=event_transition_ids,
            invariant_ids=invariant_ids,
            diff_harness_ids=diff_harness_ids,
            generation_domain_ids=generation_domain_ids,
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
    parser.add_argument("--invariants", type=Path, default=DEFAULT_INVARIANTS)
    parser.add_argument(
        "--generation-domains", type=Path, default=DEFAULT_GENERATION_DOMAINS
    )
    parser.add_argument("--diff-harness", type=Path, default=DEFAULT_DIFF_HARNESS)
    parser.add_argument(
        "--transition-sequences", type=Path, default=DEFAULT_TRANSITION_SEQUENCES
    )
    parser.add_argument("--actions-header", type=Path, default=DEFAULT_ACTION_HEADER)
    parser.add_argument("--action-runtime", type=Path, default=DEFAULT_ACTION_RUNTIME)
    args = parser.parse_args()

    failures = validate_contract(
        args.transitions,
        args.shims,
        args.action_coverage,
        args.actions_header,
        args.event_coverage,
        args.owner_fields,
        args.dispatch_surfaces,
        args.invariants,
        args.generation_domains,
        args.diff_harness,
        args.transition_sequences,
        args.action_runtime,
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
