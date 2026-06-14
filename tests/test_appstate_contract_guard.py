from __future__ import annotations

import copy
import importlib.util
import re
import subprocess
from pathlib import Path

import pytest

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_appstate_contract.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_appstate_contract", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


REQUIRED_CATEGORIES = sorted(guard.REQUIRED_TRANSITION_CATEGORIES)
REQUIRED_EVENT_CLASSES = sorted(guard.REQUIRED_EVENT_CLASSES)
REQUIRED_DISPATCH_SURFACE_CATEGORIES = sorted(guard.REQUIRED_DISPATCH_SURFACE_CATEGORIES)
REQUIRED_INVARIANT_CATEGORIES = sorted(guard.REQUIRED_INVARIANT_CATEGORIES)
REQUIRED_GENERATION_DOMAIN_CATEGORIES = sorted(
    guard.REQUIRED_GENERATION_DOMAIN_CATEGORIES
)
REQUIRED_DIFF_HARNESS_CATEGORIES = sorted(guard.REQUIRED_DIFF_HARNESS_CATEGORIES)
REQUIRED_SEQUENCE_FLOWS = sorted(guard.REQUIRED_SEQUENCE_FLOWS)
FIXTURE_ACTIONS = ["ACTION_NONE", "ACTION_MOVE_UP", "ACTION_USER_CMD"]
REQUIRED_LIST_FIELD_CASES = [
    ("action", "declared_write_set", "action[0]"),
    ("action", "migration_notes", "action[0]"),
    ("event", "declared_write_set", "event[0]"),
    ("event", "migration_notes", "event[0]"),
    ("event", "trigger_paths", "event[0]"),
    ("transition", "declared_write_set", "transition[0]"),
    ("transition", "side_effects", "transition[0]"),
    ("owner_field", "invariant_checks", "owner_field[0]"),
    ("generation_domain", "identity_fields", "generation_domain[0]"),
    ("generation_domain", "migration_notes", "generation_domain[0]"),
    ("invariant", "protected_fields", "invariant[0]"),
    ("invariant", "transition_ids", "invariant[0]"),
    ("invariant", "migration_notes", "invariant[0]"),
    ("diff_harness", "snapshot_phases", "diff_harness_check[0]"),
    ("diff_harness", "snapshot_regions", "diff_harness_check[0]"),
    ("diff_harness", "transition_ids", "diff_harness_check[0]"),
    ("diff_harness", "owner_field_refs", "diff_harness_check[0]"),
    ("diff_harness", "invariant_ids", "diff_harness_check[0]"),
    ("diff_harness", "generation_domain_ids", "diff_harness_check[0]"),
    ("diff_harness", "migration_notes", "diff_harness_check[0]"),
    (
        "transition_sequence_step",
        "invariant_ids",
        "transition_sequence[0].step[0]",
    ),
    (
        "transition_sequence_step",
        "diff_harness_ids",
        "transition_sequence[0].step[0]",
    ),
    ("shim", "invariant_checks", "shim[0]"),
    ("shim", "owner_field_refs", "shim[0]"),
    ("shim", "generation_domain_refs", "shim[0]"),
    ("shim", "diff_harness_refs", "shim[0]"),
    ("shim", "migration_notes", "shim[0]"),
]


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _transition(category: str, transition_id: str | None = None) -> dict[str, object]:
    return {
        "id": transition_id or f"transition.{category}",
        "category": category,
        "source_state": "AppState.source",
        "event": "event",
        "guard": "guard",
        "allowed_result": "allowed",
        "blocked_result": "blocked",
        "target_state": "AppState.target",
        "owner": "owner",
        "declared_write_set": ["field"],
        "generation_effect": "generation",
        "side_effects": ["none"],
        "render_invalidation": "view",
        "boundary_status": "test",
        "notes_follow_up": "follow-up",
    }


def _shim(
    target_transition: str = "transition.keybinding",
    owner_field_refs: list[str] | None = None,
    generation_domain_refs: list[str] | None = None,
    diff_harness_refs: list[str] | None = None,
) -> dict[str, object]:
    return {
        "id": "shim.test",
        "owner": "owner",
        "old_authority_path": "legacy.path",
        "read_permission": "read",
        "write_permission": "write",
        "write_capability": "write_capable",
        "invariant_checks": ["invariant.inactive_panel_frozen"],
        "owner_field_refs": owner_field_refs or ["field"],
        "generation_domain_refs": generation_domain_refs or ["domain.panel_generation"],
        "diff_harness_refs": diff_harness_refs
        or ["harness.transition_before_after_snapshot"],
        "removal_trigger": "trigger",
        "target_transition": target_transition,
        "follow_up_task": "task",
        "qa_enforcement": "guard",
    }


def _action(
    action: str,
    transition_id: str = "transition.keybinding",
    category: str = "keybinding",
) -> dict[str, object]:
    return {
        "action": action,
        "transition_id": transition_id,
        "category": category,
        "owner": "owner",
        "declared_write_set": ["panel.tree_selection_key"],
        "boundary_status": "test",
        "migration_notes": ["fixture action coverage"],
    }


def _event(
    event_class: str,
    transition_id: str | None = None,
    category: str | None = None,
) -> dict[str, object]:
    event_categories = {
        "terminal_resize_signal": "terminal_signal_or_resize",
        "refresh_rebuild": "refresh_rebuild",
        "rebuild_rebind_callback": "rebuild_rebind_callback",
        "filesystem_mutation_result": "filesystem_mutation_result",
        "watcher_live_refresh": "refresh_rebuild",
        "command_completion": "command_completion",
        "modal_completion": "modal_action",
        "volume_lifecycle": "volume_operation",
        "render_reflow": "render_reflow",
    }
    resolved_category = category or event_categories.get(event_class, "refresh_rebuild")
    return {
        "event_id": f"event.{event_class}",
        "event_class": event_class,
        "transition_id": transition_id or f"transition.{resolved_category}",
        "category": resolved_category,
        "source": "fixture source",
        "owner": "owner",
        "declared_write_set": ["field"],
        "boundary_status": "test",
        "trigger_paths": ["fixture trigger"],
        "migration_notes": ["fixture event coverage"],
    }


def _dispatch_surface(
    category: str,
    transition_id: str | None = None,
    surface_id: str | None = None,
) -> dict[str, object]:
    transition_by_surface_category = {
        "key_decode_input_dispatch": "transition.keybinding",
        "directory_window_action_dispatch": "transition.keybinding",
        "file_window_action_dispatch": "transition.keybinding",
        "menu_modal_completion": "transition.modal_action",
        "resize_signal_handling": "transition.terminal_signal_or_resize",
        "refresh_rebuild_rebind": "transition.refresh_rebuild",
        "filesystem_mutation_result": "transition.filesystem_mutation_result",
        "volume_operation": "transition.volume_operation",
        "watcher_live_refresh": "transition.refresh_rebuild",
        "render_reflow_projection": "transition.render_reflow",
    }
    return {
        "surface_id": surface_id or f"surface.{category}",
        "category": category,
        "source_path": "src/ui/key_engine.c",
        "entry_symbol_or_path": "GetEventOrKey",
        "transition_id": transition_id or transition_by_surface_category[category],
        "boundary_status": "test",
        "allowed_direct_writes": ["field"],
        "migration_notes": ["fixture coverage"],
    }


def _invariant(
    category: str,
    invariant_id: str | None = None,
    transition_ids: list[str] | None = None,
    dispatch_surface_ids: list[str] | None = None,
) -> dict[str, object]:
    return {
        "invariant_id": invariant_id or f"invariant.{category}",
        "category": category,
        "owner_region": "panel-local state",
        "protected_fields": ["field", "panel.tree_selection_key"],
        "transition_ids": transition_ids or ["transition.keybinding"],
        "dispatch_surface_ids": (
            dispatch_surface_ids or ["surface.key_decode_input_dispatch"]
        ),
        "failure_mode": "fixture failure mode",
        "enforcement_status": "documented_foundation_only",
        "test_strategy": "fixture state-sequence coverage",
        "migration_notes": ["fixture coverage"],
    }


def _generation_domain(
    category: str,
    domain_id: str | None = None,
    advances_on_transition_ids: list[str] | None = None,
) -> dict[str, object]:
    return {
        "domain_id": domain_id or f"domain.{category}",
        "category": category,
        "owner_region": "panel-local state",
        "generation_owner_field": "field",
        "identity_fields": ["field"],
        "advances_on_transition_ids": (
            advances_on_transition_ids or ["transition.keybinding"]
        ),
        "stale_snapshot_policy": "fixture stale snapshot policy",
        "fail_closed_fallback": "fixture fail-closed fallback",
        "restore_boundary": "fixture restore boundary",
        "enforcement_status": "documented_foundation_only",
        "migration_notes": ["fixture coverage"],
    }


def _diff_harness(
    category: str,
    harness_id: str | None = None,
    transition_ids: list[str] | None = None,
    owner_field_refs: list[str] | None = None,
    invariant_ids: list[str] | None = None,
    generation_domain_ids: list[str] | None = None,
) -> dict[str, object]:
    return {
        "harness_id": harness_id or f"harness.{category}",
        "check_category": category,
        "snapshot_phases": ["before", "after"],
        "snapshot_regions": ["panel-local state"],
        "transition_ids": transition_ids or ["transition.keybinding"],
        "owner_field_refs": owner_field_refs or ["field", "panel.tree_selection_key"],
        "invariant_ids": invariant_ids or ["invariant.inactive_panel_frozen"],
        "generation_domain_ids": generation_domain_ids or ["domain.panel_generation"],
        "expected_behavior": "fixture expected behavior",
        "failure_mode": "fixture failure mode",
        "enforcement_status": "documented_foundation_only",
        "migration_notes": ["fixture coverage"],
    }


def _transition_sequence(
    flow: str,
    *,
    scenario_id: str | None = None,
    category: str = "layout_split",
    steps: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    return {
        "scenario_id": scenario_id or f"sequence.{flow}",
        "category": category,
        "flow": flow,
        "description": "fixture transition sequence",
        "steps": steps or [_sequence_step()],
    }


def _sequence_step(
    *,
    ordinal: int = 1,
    transition_id: str = "transition.keybinding",
    action_id: str = "ACTION_NONE",
    event_id: str | None = None,
    invariant_ids: list[str] | None = None,
    diff_harness_ids: list[str] | None = None,
    generation_domain_id: str = "domain.panel_generation",
    expected_result: str = "allowed",
) -> dict[str, object]:
    stimulus: dict[str, object] = {"action_id": action_id}
    if event_id is not None:
        stimulus["event_id"] = event_id
    return {
        "ordinal": ordinal,
        "step_id": f"step.{ordinal}",
        "transition_id": transition_id,
        "stimulus": stimulus,
        "expected_result": expected_result,
        "invariant_ids": invariant_ids or ["invariant.inactive_panel_frozen"],
        "diff_harness_ids": diff_harness_ids
        or ["harness.transition_before_after_snapshot"],
        "generation_domain_expectations": [
            {
                "domain_id": generation_domain_id,
                "expectation": "fixture expectation",
            }
        ],
    }


def _complete_transition_sequences() -> list[dict[str, object]]:
    return [
        _transition_sequence(
            flow,
            category="layout_split" if flow.startswith("split_") else "panel_navigation",
        )
        for flow in REQUIRED_SEQUENCE_FLOWS
    ]


def _owner_field(field: str = "field") -> dict[str, object]:
    return {
        "field": field,
        "owner_region": "panel-local state",
        "canonical_owner": "YtreeNovaPanel(fixture)",
        "runtime_carrier": "YtreeNovaPanel fixture carrier",
        "mutation_rule": "Fixture transitions may mutate only declared fields.",
        "migration_status": "test",
        "invariant_checks": ["invariant.inactive_panel_frozen"],
    }


def _write_fixture(
    tmp_path: Path,
    *,
    transitions: list[dict[str, object]],
    shims: list[dict[str, object]] | None = None,
    actions: list[dict[str, object]] | None = None,
    events: list[dict[str, object]] | None = None,
    owner_fields: list[dict[str, object]] | None = None,
    dispatch_surfaces: list[dict[str, object]] | None = None,
    invariants: list[dict[str, object]] | None = None,
    generation_domains: list[dict[str, object]] | None = None,
    diff_harness_checks: list[dict[str, object]] | None = None,
    transition_sequences: list[dict[str, object]] | None = None,
    required_event_classes: list[str] | None = None,
    enum_actions: list[str] | None = None,
    runtime_actions: list[dict[str, object]] | None = None,
    runtime_action_coverages: list[dict[str, object]] | None = None,
    runtime_events: list[dict[str, object]] | None = None,
    runtime_transitions: list[dict[str, object]] | None = None,
    runtime_dispatch_surfaces: list[dict[str, object]] | None = None,
    runtime_shims: list[dict[str, object]] | None = None,
    runtime_invariants: list[dict[str, object]] | None = None,
    runtime_owner_fields: list[dict[str, object]] | None = None,
    runtime_generation_domains: list[dict[str, object]] | None = None,
    runtime_diff_harness_checks: list[dict[str, object]] | None = None,
    runtime_transition_sequences: list[dict[str, object]] | None = None,
) -> tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path]:
    transitions_path = tmp_path / "transitions.json"
    shims_path = tmp_path / "shims.json"
    action_coverage_path = tmp_path / "action_coverage.json"
    event_coverage_path = tmp_path / "event_coverage.json"
    owner_fields_path = tmp_path / "owner_fields.json"
    dispatch_surfaces_path = tmp_path / "dispatch_surfaces.json"
    invariants_path = tmp_path / "invariants.json"
    generation_domains_path = tmp_path / "generation_domains.json"
    diff_harness_path = tmp_path / "diff_harness.json"
    transition_sequences_path = tmp_path / "transition_sequences.json"
    actions_header_path = tmp_path / "ytnova_defs.h"
    action_runtime_path = tmp_path / "appstate_actions.c"
    _write(transitions_path, _jsonish({"schema_version": 1, "transitions": transitions}))
    _write(shims_path, _jsonish({"schema_version": 1, "shims": shims or [_shim()]}))
    _write(
        action_coverage_path,
        _jsonish({"schema_version": 1, "actions": actions or _complete_actions()}),
    )
    _write(
        event_coverage_path,
        _jsonish(
            {
                "schema_version": 1,
                "required_event_classes": required_event_classes or REQUIRED_EVENT_CLASSES,
                "events": events or _complete_events(),
            }
        ),
    )
    _write(
        owner_fields_path,
        _jsonish({"schema_version": 1, "owner_fields": owner_fields or _complete_owner_fields()}),
    )
    _write(
        dispatch_surfaces_path,
        _jsonish(
            {
                "schema_version": 1,
                "dispatch_surfaces": dispatch_surfaces or _complete_dispatch_surfaces(),
            }
        ),
    )
    _write(
        invariants_path,
        _jsonish({"schema_version": 1, "invariants": invariants or _complete_invariants()}),
    )
    _write(
        generation_domains_path,
        _jsonish(
            {
                "schema_version": 1,
                "generation_domains": (
                    generation_domains or _complete_generation_domains()
                ),
            }
        ),
    )
    _write(
        diff_harness_path,
        _jsonish(
            {
                "schema_version": 1,
                "diff_harness_checks": (
                    diff_harness_checks or _complete_diff_harness_checks()
                ),
            }
        ),
    )
    _write(
        transition_sequences_path,
        _jsonish(
            {
                "schema_version": 1,
                "scenarios": transition_sequences
                or _complete_transition_sequences(),
            }
        ),
    )
    _write(actions_header_path, _enum_header(enum_actions or FIXTURE_ACTIONS))
    _write(
        action_runtime_path,
        _runtime_source(
            runtime_actions if runtime_actions is not None else _complete_actions(),
            runtime_action_coverages
            if runtime_action_coverages is not None
            else (actions if actions is not None else _complete_actions()),
            runtime_events
            if runtime_events is not None
            else (events if events is not None else _complete_events()),
            runtime_transitions if runtime_transitions is not None else transitions,
            runtime_owner_fields
            if runtime_owner_fields is not None
            else (
                owner_fields
                if owner_fields is not None
                else _complete_owner_fields()
            ),
            runtime_dispatch_surfaces
            if runtime_dispatch_surfaces is not None
            else (
                dispatch_surfaces
                if dispatch_surfaces is not None
                else _complete_dispatch_surfaces()
            ),
            runtime_shims
            if runtime_shims is not None
            else (shims if shims is not None else [_shim()]),
            runtime_invariants
            if runtime_invariants is not None
            else (invariants if invariants is not None else _complete_invariants()),
            runtime_generation_domains
            if runtime_generation_domains is not None
            else (
                generation_domains
                if generation_domains is not None
                else _complete_generation_domains()
            ),
            runtime_diff_harness_checks
            if runtime_diff_harness_checks is not None
            else (
                diff_harness_checks
                if diff_harness_checks is not None
                else _complete_diff_harness_checks()
            ),
            runtime_transition_sequences
            if runtime_transition_sequences is not None
            else (
                transition_sequences
                if transition_sequences is not None
                else _complete_transition_sequences()
            ),
        ),
    )
    return (
        transitions_path,
        shims_path,
        action_coverage_path,
        actions_header_path,
        event_coverage_path,
        owner_fields_path,
        dispatch_surfaces_path,
        invariants_path,
        generation_domains_path,
        diff_harness_path,
        transition_sequences_path,
        action_runtime_path,
    )


def _jsonish(value: object) -> str:
    import json

    return json.dumps(value, indent=2)


def _enum_header(actions: list[str]) -> str:
    members = "\n".join(f"  {action}," for action in actions)
    return f"typedef enum {{\n{members}\n}} YtreeNovaAction;\n"


def _runtime_source(
    actions: list[dict[str, object]],
    action_coverages: list[dict[str, object]],
    events: list[dict[str, object]],
    transitions: list[dict[str, object]],
    owner_fields: list[dict[str, object]],
    dispatch_surfaces: list[dict[str, object]],
    shims: list[dict[str, object]],
    invariants: list[dict[str, object]],
    generation_domains: list[dict[str, object]],
    diff_harness_checks: list[dict[str, object]],
    transition_sequences: list[dict[str, object]],
) -> str:
    transition_write_sets = []
    transition_rows = []
    for index, record in enumerate(transitions):
        declared_write_set = record.get("declared_write_set")
        if not isinstance(declared_write_set, list):
            declared_write_set = []
        write_set_rows = "\n".join(
            f'  "{field}",' for field in declared_write_set if isinstance(field, str)
        )
        transition_write_sets.append(
            "static const char *const kAppStateTransitionWriteSet"
            f"{index}[] = {{\n{write_set_rows}\n}};\n"
        )
        transition_rows.append(
            f'  {{"{record.get("id", "")}", "{record.get("category", "")}", '
            f'"{record.get("owner", "")}", '
            f"kAppStateTransitionWriteSet{index}, "
            f"sizeof(kAppStateTransitionWriteSet{index}) / "
            f"sizeof(kAppStateTransitionWriteSet{index}[0])}},"
        )
    action_coverage_arrays = []
    action_coverage_rows = []
    for index, record in enumerate(action_coverages):
        declared_write_set = record.get("declared_write_set")
        if not isinstance(declared_write_set, list):
            declared_write_set = []
        write_set_rows = "\n".join(
            f'  "{field}",' for field in declared_write_set if isinstance(field, str)
        )
        write_set_table = f"kAppStateActionCoverageWriteSet{index}"
        action_coverage_arrays.append(
            f"static const char *const {write_set_table}[] = "
            f"{{\n{write_set_rows}\n}};\n"
        )
        migration_notes = record.get("migration_notes")
        if not isinstance(migration_notes, list):
            migration_notes = []
        note_rows = "\n".join(
            f'  "{note}",' for note in migration_notes if isinstance(note, str)
        )
        notes_table = f"kAppStateActionCoverageMigrationNotes{index}"
        action_coverage_arrays.append(
            f"static const char *const {notes_table}[] = "
            f"{{\n{note_rows}\n}};\n"
        )
        action = record.get("action", "")
        action_coverage_rows.append(
            f'  {{{action}, "{action}", "{record.get("transition_id", "")}", '
            f'"{record.get("category", "")}", "{record.get("owner", "")}", '
            f"{write_set_table}, sizeof({write_set_table}) / "
            f"sizeof({write_set_table}[0]), "
            f'"{record.get("boundary_status", "")}", '
            f"{notes_table}, sizeof({notes_table}) / sizeof({notes_table}[0])}},"
        )
    event_coverage_arrays = []
    event_coverage_rows = []
    transition_index_by_id = {
        str(record.get("id", "")): index for index, record in enumerate(transitions)
    }
    for index, record in enumerate(events):
        trigger_paths = record.get("trigger_paths")
        if not isinstance(trigger_paths, list):
            trigger_paths = []
        trigger_rows = "\n".join(
            f'  "{trigger}",' for trigger in trigger_paths if isinstance(trigger, str)
        )
        trigger_table = f"kAppStateEventCoverageTriggerPaths{index}"
        event_coverage_arrays.append(
            f"static const char *const {trigger_table}[] = "
            f"{{\n{trigger_rows}\n}};\n"
        )
        migration_notes = record.get("migration_notes")
        if not isinstance(migration_notes, list):
            migration_notes = []
        note_rows = "\n".join(
            f'  "{note}",' for note in migration_notes if isinstance(note, str)
        )
        notes_table = f"kAppStateEventCoverageMigrationNotes{index}"
        event_coverage_arrays.append(
            f"static const char *const {notes_table}[] = "
            f"{{\n{note_rows}\n}};\n"
        )
        transition_index = transition_index_by_id.get(str(record.get("transition_id", "")), 0)
        write_set_table = f"kAppStateTransitionWriteSet{transition_index}"
        event_coverage_rows.append(
            f'  {{"{record.get("event_id", "")}", "{record.get("event_class", "")}", '
            f'"{record.get("transition_id", "")}", "{record.get("category", "")}", '
            f'"{record.get("source", "")}", "{record.get("owner", "")}", '
            f"{write_set_table}, sizeof({write_set_table}) / "
            f"sizeof({write_set_table}[0]), "
            f'"{record.get("boundary_status", "")}", '
            f"{trigger_table}, sizeof({trigger_table}) / sizeof({trigger_table}[0]), "
            f"{notes_table}, sizeof({notes_table}) / sizeof({notes_table}[0])}},"
        )
    owner_field_arrays = []
    owner_field_rows = []
    for index, record in enumerate(owner_fields):
        invariant_checks = record.get("invariant_checks")
        if not isinstance(invariant_checks, list):
            invariant_checks = []
        invariant_rows = "\n".join(
            f'  "{check}",' for check in invariant_checks if isinstance(check, str)
        )
        checks_table = f"kAppStateOwnerFieldInvariantChecks{index}"
        owner_field_arrays.append(
            f"static const char *const {checks_table}[] = "
            f"{{\n{invariant_rows}\n}};\n"
        )
        owner_field_rows.append(
            f'  {{"{record.get("field", "")}", '
            f'"{record.get("owner_region", "")}", '
            f'"{record.get("canonical_owner", "")}", '
            f'"{record.get("runtime_carrier", "")}", '
            f'"{record.get("mutation_rule", "")}", '
            f'"{record.get("migration_status", "")}", '
            f"{checks_table}, sizeof({checks_table}) / sizeof({checks_table}[0])}},"
        )
    dispatch_surface_arrays = []
    dispatch_surface_rows = []
    for index, record in enumerate(dispatch_surfaces):
        allowed_direct_writes = record.get("allowed_direct_writes")
        if isinstance(allowed_direct_writes, list) and allowed_direct_writes:
            write_rows = "\n".join(
                f'  "{field}",'
                for field in allowed_direct_writes
                if isinstance(field, str)
            )
            dispatch_surface_arrays.append(
                "static const char *const "
                f"kAppStateDispatchSurfaceAllowedDirectWrites{index}[] = "
                f"{{\n{write_rows}\n}};\n"
            )
            writes_table = f"kAppStateDispatchSurfaceAllowedDirectWrites{index}"
            writes_count = (
                f"sizeof({writes_table}) / "
                f"sizeof({writes_table}[0])"
            )
        else:
            writes_table = "NULL"
            writes_count = "0"
        migration_notes = record.get("migration_notes")
        if not isinstance(migration_notes, list):
            migration_notes = []
        note_rows = "\n".join(
            f'  "{note}",' for note in migration_notes if isinstance(note, str)
        )
        notes_table = f"kAppStateDispatchSurfaceMigrationNotes{index}"
        dispatch_surface_arrays.append(
            f"static const char *const {notes_table}[] = "
            f"{{\n{note_rows}\n}};\n"
        )
        dispatch_surface_rows.append(
            f'  {{"{record.get("surface_id", "")}", '
            f'"{record.get("category", "")}", '
            f'"{record.get("source_path", "")}", '
            f'"{record.get("entry_symbol_or_path", "")}", '
            f'"{record.get("transition_id", "")}", '
            f'"{record.get("boundary_status", "")}", '
            f"{writes_table}, {writes_count}, "
            f"{notes_table}, sizeof({notes_table}) / sizeof({notes_table}[0])}},"
        )
    shim_invariants = []
    shim_owner_field_refs = []
    shim_generation_domain_refs = []
    shim_diff_harness_refs = []
    shim_rows = []
    for index, record in enumerate(shims):
        invariant_checks = record.get("invariant_checks")
        if not isinstance(invariant_checks, list):
            invariant_checks = []
        invariant_rows = "\n".join(
            f'  "{check}",' for check in invariant_checks if isinstance(check, str)
        )
        shim_invariants.append(
            "static const char *const kAppStateCompatibilityShimInvariantChecks"
            f"{index}[] = {{\n{invariant_rows}\n}};\n"
        )
        owner_field_refs = record.get("owner_field_refs")
        if not isinstance(owner_field_refs, list):
            owner_field_refs = []
        owner_ref_rows = "\n".join(
            f'  "{field}",' for field in owner_field_refs if isinstance(field, str)
        )
        shim_owner_field_refs.append(
            "static const char *const kAppStateCompatibilityShimOwnerFieldRefs"
            f"{index}[] = {{\n{owner_ref_rows}\n}};\n"
        )
        generation_domain_refs = record.get("generation_domain_refs")
        if not isinstance(generation_domain_refs, list):
            generation_domain_refs = []
        generation_ref_rows = "\n".join(
            f'  "{domain_id}",'
            for domain_id in generation_domain_refs
            if isinstance(domain_id, str)
        )
        shim_generation_domain_refs.append(
            "static const char *const kAppStateCompatibilityShimGenerationDomainRefs"
            f"{index}[] = {{\n{generation_ref_rows}\n}};\n"
        )
        diff_harness_refs = record.get("diff_harness_refs")
        if not isinstance(diff_harness_refs, list):
            diff_harness_refs = []
        diff_ref_rows = "\n".join(
            f'  "{harness_id}",'
            for harness_id in diff_harness_refs
            if isinstance(harness_id, str)
        )
        shim_diff_harness_refs.append(
            "static const char *const kAppStateCompatibilityShimDiffHarnessRefs"
            f"{index}[] = {{\n{diff_ref_rows}\n}};\n"
        )
        shim_rows.append(
            f'  {{"{record.get("id", "")}", "{record.get("owner", "")}", '
            f'"{record.get("old_authority_path", "")}", '
            f'"{record.get("read_permission", "")}", '
            f'"{record.get("write_permission", "")}", '
            f'"{record.get("write_capability", "")}", '
            f"kAppStateCompatibilityShimInvariantChecks{index}, "
            f"sizeof(kAppStateCompatibilityShimInvariantChecks{index}) / "
            f"sizeof(kAppStateCompatibilityShimInvariantChecks{index}[0]), "
            f"kAppStateCompatibilityShimOwnerFieldRefs{index}, "
            f"sizeof(kAppStateCompatibilityShimOwnerFieldRefs{index}) / "
            f"sizeof(kAppStateCompatibilityShimOwnerFieldRefs{index}[0]), "
            f"kAppStateCompatibilityShimGenerationDomainRefs{index}, "
            f"sizeof(kAppStateCompatibilityShimGenerationDomainRefs{index}) / "
            f"sizeof(kAppStateCompatibilityShimGenerationDomainRefs{index}[0]), "
            f"kAppStateCompatibilityShimDiffHarnessRefs{index}, "
            f"sizeof(kAppStateCompatibilityShimDiffHarnessRefs{index}) / "
            f"sizeof(kAppStateCompatibilityShimDiffHarnessRefs{index}[0]), "
            f'"{record.get("removal_trigger", "")}", '
            f'"{record.get("target_transition", "")}", '
            f'"{record.get("follow_up_task", "")}", '
            f'"{record.get("qa_enforcement", "")}"}},'
        )
    invariant_arrays = []
    invariant_rows = []
    invariant_list_fields = (
        ("protected_fields", "ProtectedFields"),
        ("transition_ids", "TransitionIds"),
        ("dispatch_surface_ids", "DispatchSurfaceIds"),
        ("migration_notes", "MigrationNotes"),
    )
    for index, record in enumerate(invariants):
        for field, prefix in invariant_list_fields:
            values = record.get(field)
            if not isinstance(values, list):
                values = []
            rows = "\n".join(
                f'  "{value}",' for value in values if isinstance(value, str)
            )
            invariant_arrays.append(
                f"static const char *const kAppStateInvariant{prefix}{index}[] "
                f"= {{\n{rows}\n}};\n"
            )
        invariant_rows.append(
            f'  {{"{record.get("invariant_id", "")}", '
            f'"{record.get("category", "")}", '
            f'"{record.get("owner_region", "")}", '
            f"kAppStateInvariantProtectedFields{index}, "
            f"sizeof(kAppStateInvariantProtectedFields{index}) / "
            f"sizeof(kAppStateInvariantProtectedFields{index}[0]), "
            f"kAppStateInvariantTransitionIds{index}, "
            f"sizeof(kAppStateInvariantTransitionIds{index}) / "
            f"sizeof(kAppStateInvariantTransitionIds{index}[0]), "
            f"kAppStateInvariantDispatchSurfaceIds{index}, "
            f"sizeof(kAppStateInvariantDispatchSurfaceIds{index}) / "
            f"sizeof(kAppStateInvariantDispatchSurfaceIds{index}[0]), "
            f'"{record.get("failure_mode", "")}", '
            f'"{record.get("enforcement_status", "")}", '
            f'"{record.get("test_strategy", "")}", '
            f"kAppStateInvariantMigrationNotes{index}, "
            f"sizeof(kAppStateInvariantMigrationNotes{index}) / "
            f"sizeof(kAppStateInvariantMigrationNotes{index}[0])}},"
        )
    generation_domain_arrays = []
    generation_domain_rows = []
    generation_domain_list_fields = (
        ("identity_fields", "IdentityFields"),
        ("advances_on_transition_ids", "AdvancesOnTransitionIds"),
        ("migration_notes", "MigrationNotes"),
    )
    for index, record in enumerate(generation_domains):
        for field, prefix in generation_domain_list_fields:
            values = record.get(field)
            if not isinstance(values, list):
                values = []
            rows = "\n".join(
                f'  "{value}",' for value in values if isinstance(value, str)
            )
            generation_domain_arrays.append(
                f"static const char *const kAppStateGenerationDomain{prefix}{index}[] "
                f"= {{\n{rows}\n}};\n"
            )
        generation_domain_rows.append(
            f'  {{"{record.get("domain_id", "")}", '
            f'"{record.get("category", "")}", '
            f'"{record.get("owner_region", "")}", '
            f'"{record.get("generation_owner_field", "")}", '
            f"kAppStateGenerationDomainIdentityFields{index}, "
            f"sizeof(kAppStateGenerationDomainIdentityFields{index}) / "
            f"sizeof(kAppStateGenerationDomainIdentityFields{index}[0]), "
            f"kAppStateGenerationDomainAdvancesOnTransitionIds{index}, "
            f"sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds{index}) / "
            f"sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds{index}[0]), "
            f'"{record.get("stale_snapshot_policy", "")}", '
            f'"{record.get("fail_closed_fallback", "")}", '
            f'"{record.get("restore_boundary", "")}", '
            f'"{record.get("enforcement_status", "")}", '
            f"kAppStateGenerationDomainMigrationNotes{index}, "
            f"sizeof(kAppStateGenerationDomainMigrationNotes{index}) / "
            f"sizeof(kAppStateGenerationDomainMigrationNotes{index}[0])}},"
        )
    diff_harness_arrays = []
    diff_harness_rows = []
    diff_harness_list_fields = (
        ("snapshot_phases", "SnapshotPhases"),
        ("snapshot_regions", "SnapshotRegions"),
        ("transition_ids", "TransitionIds"),
        ("owner_field_refs", "OwnerFieldRefs"),
        ("invariant_ids", "InvariantIds"),
        ("generation_domain_ids", "GenerationDomainIds"),
        ("migration_notes", "MigrationNotes"),
    )
    for index, record in enumerate(diff_harness_checks):
        for field, prefix in diff_harness_list_fields:
            values = record.get(field)
            if not isinstance(values, list):
                values = []
            rows = "\n".join(
                f'  "{value}",' for value in values if isinstance(value, str)
            )
            diff_harness_arrays.append(
                f"static const char *const kAppStateDiffHarness{prefix}{index}[] "
                f"= {{\n{rows}\n}};\n"
            )
        diff_harness_rows.append(
            f'  {{"{record.get("harness_id", "")}", '
            f'"{record.get("check_category", "")}", '
            f"kAppStateDiffHarnessSnapshotPhases{index}, "
            f"sizeof(kAppStateDiffHarnessSnapshotPhases{index}) / "
            f"sizeof(kAppStateDiffHarnessSnapshotPhases{index}[0]), "
            f"kAppStateDiffHarnessSnapshotRegions{index}, "
            f"sizeof(kAppStateDiffHarnessSnapshotRegions{index}) / "
            f"sizeof(kAppStateDiffHarnessSnapshotRegions{index}[0]), "
            f"kAppStateDiffHarnessTransitionIds{index}, "
            f"sizeof(kAppStateDiffHarnessTransitionIds{index}) / "
            f"sizeof(kAppStateDiffHarnessTransitionIds{index}[0]), "
            f"kAppStateDiffHarnessOwnerFieldRefs{index}, "
            f"sizeof(kAppStateDiffHarnessOwnerFieldRefs{index}) / "
            f"sizeof(kAppStateDiffHarnessOwnerFieldRefs{index}[0]), "
            f"kAppStateDiffHarnessInvariantIds{index}, "
            f"sizeof(kAppStateDiffHarnessInvariantIds{index}) / "
            f"sizeof(kAppStateDiffHarnessInvariantIds{index}[0]), "
            f"kAppStateDiffHarnessGenerationDomainIds{index}, "
            f"sizeof(kAppStateDiffHarnessGenerationDomainIds{index}) / "
            f"sizeof(kAppStateDiffHarnessGenerationDomainIds{index}[0]), "
            f'"{record.get("expected_behavior", "")}", '
            f'"{record.get("failure_mode", "")}", '
            f'"{record.get("enforcement_status", "")}", '
            f"kAppStateDiffHarnessMigrationNotes{index}, "
            f"sizeof(kAppStateDiffHarnessMigrationNotes{index}) / "
            f"sizeof(kAppStateDiffHarnessMigrationNotes{index}[0])}},"
        )
    transition_sequence_arrays = []
    transition_sequence_rows = []
    for sequence_index, record in enumerate(transition_sequences):
        steps = record.get("steps")
        if not isinstance(steps, list):
            steps = []
        step_rows = []
        for step_index, step in enumerate(steps):
            if not isinstance(step, dict):
                continue
            invariant_ids = step.get("invariant_ids")
            if not isinstance(invariant_ids, list):
                invariant_ids = []
            sequence_invariant_rows = "\n".join(
                f'  "{value}",' for value in invariant_ids if isinstance(value, str)
            )
            invariant_table = (
                "kAppStateTransitionSequenceStepInvariantIds"
                f"{sequence_index}_{step_index}"
            )
            transition_sequence_arrays.append(
                f"static const char *const {invariant_table}[] = "
                f"{{\n{sequence_invariant_rows}\n}};\n"
            )
            diff_harness_ids = step.get("diff_harness_ids")
            if not isinstance(diff_harness_ids, list):
                diff_harness_ids = []
            diff_rows = "\n".join(
                f'  "{value}",' for value in diff_harness_ids if isinstance(value, str)
            )
            diff_table = (
                "kAppStateTransitionSequenceStepDiffHarnessIds"
                f"{sequence_index}_{step_index}"
            )
            transition_sequence_arrays.append(
                f"static const char *const {diff_table}[] = "
                f"{{\n{diff_rows}\n}};\n"
            )
            expectations = step.get("generation_domain_expectations")
            if not isinstance(expectations, list):
                expectations = []
            expectation_rows = "\n".join(
                f'  {{"{expectation.get("domain_id", "")}", '
                f'"{expectation.get("expectation", "")}"}},'
                for expectation in expectations
                if isinstance(expectation, dict)
            )
            expectation_table = (
                "kAppStateTransitionSequenceStepGenerationExpectations"
                f"{sequence_index}_{step_index}"
            )
            transition_sequence_arrays.append(
                "static const "
                "AppStateTransitionSequenceGenerationExpectationMetadata "
                f"{expectation_table}[] = {{\n{expectation_rows}\n}};\n"
            )
            no_unrelated = step.get("no_unrelated_mutation")
            if isinstance(no_unrelated, dict):
                no_unrelated_table = (
                    "kAppStateTransitionSequenceStepNoUnrelatedMutation"
                    f"{sequence_index}_{step_index}"
                )
                transition_sequence_arrays.append(
                    "static const "
                    "AppStateTransitionSequenceNoUnrelatedMutationMetadata "
                    f'{no_unrelated_table} = '
                    f'{{"{no_unrelated.get("diff_harness_id", "")}", '
                    f'"{no_unrelated.get("expectation", "")}"}};\n'
                )
                no_unrelated_ref = f"&{no_unrelated_table}"
            else:
                no_unrelated_ref = "NULL"
            fallback = step.get("deterministic_fallback")
            if isinstance(fallback, dict):
                fallback_table = (
                    "kAppStateTransitionSequenceStepDeterministicFallback"
                    f"{sequence_index}_{step_index}"
                )
                transition_sequence_arrays.append(
                    "static const "
                    "AppStateTransitionSequenceDeterministicFallbackMetadata "
                    f'{fallback_table} = '
                    f'{{"{fallback.get("outcome", "")}", '
                    f'"{fallback.get("allowed_mutation_scope", "")}"}};\n'
                )
                fallback_ref = f"&{fallback_table}"
            else:
                fallback_ref = "NULL"
            stimulus = step.get("stimulus")
            if not isinstance(stimulus, dict):
                stimulus = {}
            action_id = stimulus.get("action_id")
            event_id = stimulus.get("event_id")
            action_expr = f'"{action_id}"' if isinstance(action_id, str) else "NULL"
            event_expr = f'"{event_id}"' if isinstance(event_id, str) else "NULL"
            precondition = step.get("precondition")
            precondition_expr = (
                f'"{precondition}"' if isinstance(precondition, str) else "NULL"
            )
            step_rows.append(
                f'  {{{step.get("ordinal", 0)}, "{step.get("step_id", "")}", '
                f'"{step.get("transition_id", "")}", {action_expr}, {event_expr}, '
                f'"{step.get("expected_result", "")}", '
                f"{invariant_table}, sizeof({invariant_table}) / "
                f"sizeof({invariant_table}[0]), "
                f"{diff_table}, sizeof({diff_table}) / sizeof({diff_table}[0]), "
                f"{expectation_table}, sizeof({expectation_table}) / "
                f"sizeof({expectation_table}[0]), "
                f"{no_unrelated_ref}, {precondition_expr}, {fallback_ref}}},"
            )
        steps_table = f"kAppStateTransitionSequenceSteps{sequence_index}"
        transition_sequence_arrays.append(
            "static const AppStateTransitionSequenceStepMetadata "
            f"{steps_table}[] = {{\n" + "\n".join(step_rows) + "\n};\n"
        )
        transition_sequence_rows.append(
            f'  {{"{record.get("scenario_id", "")}", '
            f'"{record.get("category", "")}", '
            f'"{record.get("flow", "")}", '
            f'"{record.get("description", "")}", '
            f"{steps_table}, sizeof({steps_table}) / sizeof({steps_table}[0])}},"
        )
    action_rows = "\n".join(
        f'  {{{record["action"]}, "{record["transition_id"]}", "{record["category"]}"}},'
        for record in actions
    )
    return (
        "".join(transition_write_sets)
        + "".join(action_coverage_arrays)
        + "".join(event_coverage_arrays)
        + "".join(owner_field_arrays)
        + "".join(dispatch_surface_arrays)
        + "".join(shim_invariants)
        + "".join(shim_owner_field_refs)
        + "".join(shim_generation_domain_refs)
        + "".join(shim_diff_harness_refs)
        + "".join(invariant_arrays)
        + "".join(generation_domain_arrays)
        + "".join(diff_harness_arrays)
        + "static const AppStateTransitionMetadata kAppStateTransitions[] = {\n"
        + "\n".join(transition_rows)
        + "\n};\n"
        "static const AppStateOwnerFieldMetadata kAppStateOwnerFields[] = {\n"
        + "\n".join(owner_field_rows)
        + "\n};\n"
        "static const AppStateDispatchSurfaceMetadata "
        "kAppStateDispatchSurfaces[] = {\n"
        + "\n".join(dispatch_surface_rows)
        + "\n};\n"
        "static const AppStateCompatibilityShimMetadata "
        "kAppStateCompatibilityShims[] = {\n"
        + "\n".join(shim_rows)
        + "\n};\n"
        "static const AppStateGenerationDomainMetadata "
        "kAppStateGenerationDomains[] = {\n"
        + "\n".join(generation_domain_rows)
        + "\n};\n"
        "static const AppStateDiffHarnessMetadata kAppStateDiffHarnesses[] = {\n"
        + "\n".join(diff_harness_rows)
        + "\n};\n"
        "static const AppStateInvariantMetadata kAppStateInvariants[] = {\n"
        + "\n".join(invariant_rows)
        + "\n};\n"
        + "".join(transition_sequence_arrays)
        + "static const AppStateTransitionSequenceMetadata "
        "kAppStateTransitionSequences[] = {\n"
        + "\n".join(transition_sequence_rows)
        + "\n};\n"
        "static const AppStateActionTransitionMetadata\n"
        "    kAppStateActionTransitions[APPSTATE_ACTION_TRANSITION_COUNT] = {\n"
        f"{action_rows}\n"
        "};\n"
        "static const AppStateActionCoverageMetadata\n"
        "    kAppStateActionCoverages[APPSTATE_ACTION_COVERAGE_COUNT] = {\n"
        + "\n".join(action_coverage_rows)
        + "\n};\n"
        "static const AppStateEventCoverageMetadata\n"
        "    kAppStateEventCoverages[APPSTATE_EVENT_COVERAGE_COUNT] = {\n"
        + "\n".join(event_coverage_rows)
        + "\n};\n"
    )


def _complete_transitions() -> list[dict[str, object]]:
    return [
        _transition(category, "transition.keybinding" if category == "keybinding" else None)
        for category in REQUIRED_CATEGORIES
    ]


def _complete_transition_ids() -> list[str]:
    return [str(record["id"]) for record in _complete_transitions()]


def _complete_actions() -> list[dict[str, object]]:
    return [_action(action) for action in FIXTURE_ACTIONS]


def _complete_events() -> list[dict[str, object]]:
    return [_event(event_class) for event_class in REQUIRED_EVENT_CLASSES]


def _complete_dispatch_surfaces() -> list[dict[str, object]]:
    return [
        _dispatch_surface(category)
        for category in REQUIRED_DISPATCH_SURFACE_CATEGORIES
    ]


def _complete_invariant_dispatch_surface_ids() -> dict[str, list[str]]:
    categories = REQUIRED_INVARIANT_CATEGORIES
    surface_ids = [str(record["surface_id"]) for record in _complete_dispatch_surfaces()]
    return {
        category: [
            surface_id
            for surface_index, surface_id in enumerate(surface_ids)
            if surface_index % len(categories) == category_index
        ]
        for category_index, category in enumerate(categories)
    }


def _complete_invariants() -> list[dict[str, object]]:
    dispatch_surface_ids_by_category = _complete_invariant_dispatch_surface_ids()
    return [
        _invariant(
            category,
            transition_ids=(
                _complete_transition_ids()
                if category == "blocked_transition_determinism"
                else None
            ),
            dispatch_surface_ids=dispatch_surface_ids_by_category[category],
        )
        for category in REQUIRED_INVARIANT_CATEGORIES
    ]


def _complete_generation_domains() -> list[dict[str, object]]:
    return [
        _generation_domain(
            category,
            "domain.panel_generation" if category == "panel_generation" else None,
            _complete_transition_ids(),
        )
        for category in REQUIRED_GENERATION_DOMAIN_CATEGORIES
    ]


def _complete_generation_domain_ids() -> list[str]:
    return [str(record["domain_id"]) for record in _complete_generation_domains()]


def _complete_diff_harness_checks() -> list[dict[str, object]]:
    return [
        _diff_harness(
            category,
            "harness.transition_before_after_snapshot"
            if category == "transition_before_after_snapshot"
            else None,
            _complete_transition_ids(),
            invariant_ids=[
                "invariant.blocked_transition_determinism",
                "invariant.inactive_panel_frozen",
            ],
            generation_domain_ids=_complete_generation_domain_ids(),
        )
        for category in REQUIRED_DIFF_HARNESS_CATEGORIES
    ]


def _remove_dispatch_surface_id(
    invariant: dict[str, object],
    surface_id: str,
    fallback_surface_id: str,
) -> None:
    refs = invariant.get("dispatch_surface_ids")
    if not isinstance(refs, list):
        invariant["dispatch_surface_ids"] = [fallback_surface_id]
        return
    remaining_refs = [ref for ref in refs if ref != surface_id]
    invariant["dispatch_surface_ids"] = remaining_refs or [fallback_surface_id]


def _split_dispatch_surface_invariant_coverage(
    invariants: list[dict[str, object]],
    surface_id: str,
    fallback_surface_id: str,
) -> None:
    for invariant in invariants:
        _remove_dispatch_surface_id(invariant, surface_id, fallback_surface_id)

    invariants[0]["dispatch_surface_ids"] = [surface_id]
    invariants[0]["protected_fields"] = ["panel.tree_selection_key"]
    invariants[1]["dispatch_surface_ids"] = [fallback_surface_id]
    invariants[1]["protected_fields"] = ["field"]


def _split_transition_invariant_coverage(
    invariants: list[dict[str, object]],
    transition_id: str,
    fallback_transition_id: str,
    field: str = "field",
) -> None:
    alternate_field = "panel.tree_selection_key" if field == "field" else "field"
    for invariant in invariants:
        transition_refs = invariant.get("transition_ids")
        if not isinstance(transition_refs, list):
            invariant["transition_ids"] = [fallback_transition_id]
            continue
        remaining_refs = [ref for ref in transition_refs if ref != transition_id]
        invariant["transition_ids"] = remaining_refs or [fallback_transition_id]

    invariants[0]["transition_ids"] = [transition_id]
    invariants[0]["protected_fields"] = [alternate_field]
    invariants[1]["transition_ids"] = [fallback_transition_id]
    invariants[1]["protected_fields"] = [field]


def _complete_owner_fields() -> list[dict[str, object]]:
    return [_owner_field("field"), _owner_field("panel.tree_selection_key")]


def _validate(
    paths: tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path],
) -> list[str]:
    return guard.validate_contract(*paths)


def _fixture_with_list_field_value(
    tmp_path: Path, record_type: str, field: str, value: object
) -> tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path, Path]:
    transitions = _complete_transitions()
    shims = [_shim()]
    actions = _complete_actions()
    events = _complete_events()
    owner_fields = _complete_owner_fields()
    dispatch_surfaces = _complete_dispatch_surfaces()
    invariants = _complete_invariants()
    generation_domains = _complete_generation_domains()
    diff_harness_checks = _complete_diff_harness_checks()
    transition_sequences = _complete_transition_sequences()
    if record_type == "action":
        actions[0][field] = value
    elif record_type == "event":
        events[0][field] = value
    elif record_type == "owner_field":
        owner_fields[0][field] = value
    elif record_type == "generation_domain":
        generation_domains[0][field] = value
    elif record_type == "transition":
        transitions[0][field] = value
    elif record_type == "shim":
        shims[0][field] = value
    elif record_type == "dispatch_surface":
        dispatch_surfaces[0][field] = value
    elif record_type == "invariant":
        invariants[0][field] = value
    elif record_type == "diff_harness":
        diff_harness_checks[0][field] = value
    elif record_type == "transition_sequence_step":
        transition_sequences[0]["steps"][0][field] = value
    else:
        raise AssertionError(f"unknown record type: {record_type}")
    return _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=shims,
        actions=actions,
        events=events,
        owner_fields=owner_fields,
        dispatch_surfaces=dispatch_surfaces,
        invariants=invariants,
        generation_domains=generation_domains,
        diff_harness_checks=diff_harness_checks,
        transition_sequences=transition_sequences,
    )


def test_current_repository_appstate_contract_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_appstate_contract.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_passes_complete_temporary_fixtures(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)

    failures = _validate(paths)

    assert failures == []


def test_guard_accepts_invariant_registry_cli_override(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    override_path = tmp_path / "appstate_invariants.json"
    override_path.write_text(
        (repo_root / "docs" / "appstate_invariants.json").read_text(encoding="utf-8"),
        encoding="utf-8",
    )

    run = subprocess.run(
        [
            "python3",
            "scripts/check_appstate_contract.py",
            "--invariants",
            str(override_path),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )

    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_accepts_generation_domain_registry_cli_override(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    override_path = tmp_path / "appstate_generation_domains.json"
    override_path.write_text(
        (repo_root / "docs" / "appstate_generation_domains.json").read_text(
            encoding="utf-8"
        ),
        encoding="utf-8",
    )

    run = subprocess.run(
        [
            "python3",
            "scripts/check_appstate_contract.py",
            "--generation-domains",
            str(override_path),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )

    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_accepts_diff_harness_registry_cli_override(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    override_path = tmp_path / "appstate_diff_harness.json"
    override_path.write_text(
        (repo_root / "docs" / "appstate_diff_harness.json").read_text(
            encoding="utf-8"
        ),
        encoding="utf-8",
    )

    run = subprocess.run(
        [
            "python3",
            "scripts/check_appstate_contract.py",
            "--diff-harness",
            str(override_path),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )

    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_accepts_transition_sequence_registry_cli_override(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    override_path = tmp_path / "appstate_transition_sequences.json"
    override_path.write_text(
        (repo_root / "docs" / "appstate_transition_sequences.json").read_text(
            encoding="utf-8"
        ),
        encoding="utf-8",
    )

    run = subprocess.run(
        [
            "python3",
            "scripts/check_appstate_contract.py",
            "--transition-sequences",
            str(override_path),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )

    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_fails_when_required_transition_sequence_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0].pop("description")
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0]" in failure
        and "missing required field" in failure
        and "description" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_transition_sequence_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[1]["scenario_id"] = transition_sequences[0]["scenario_id"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[1]" in failure and "duplicate scenario_id" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_has_unknown_category(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["category"] = "unknown_sequence_category"
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0]" in failure
        and "unknown category" in failure
        and "unknown_sequence_category" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_has_unknown_flow(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["flow"] = "unknown_sequence_flow"
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0]" in failure
        and "unknown flow" in failure
        and "unknown_sequence_flow" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_steps_are_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"] = {}
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0]" in failure
        and "steps must be a non-empty list" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_transition_sequence_step_ordinal(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    duplicate_step = _sequence_step(ordinal=1)
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"] = [_sequence_step(ordinal=1), duplicate_step]
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[1]" in failure
        and "duplicate ordinal" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "value", "expected"),
    (
        ("transition_id", "transition.missing", "unknown transition id"),
        ("stimulus", {"action_id": "ACTION_MISSING"}, "unknown action"),
        ("stimulus", {"event_id": "event.missing"}, "unknown event id"),
        ("invariant_ids", ["invariant.missing"], "unknown invariant id"),
        ("diff_harness_ids", ["harness.missing"], "unknown diff harness id"),
        (
            "generation_domain_expectations",
            [{"domain_id": "domain.missing", "expectation": "fixture"}],
            "unknown generation domain id",
        ),
    ),
)
def test_guard_fails_on_unknown_transition_sequence_step_references(
    tmp_path: Path, field: str, value: object, expected: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0][field] = value
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure and expected in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_invariants_do_not_cover_step(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    unrelated_transition_id = next(
        record["id"] for record in transitions if record["id"] != "transition.keybinding"
    )
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["transition_ids"] = [unrelated_transition_id]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "invariant_ids must include at least one invariant covering transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_transition_lacks_same_harness_invariant(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    harness = next(
        record
        for record in diff_harness_checks
        if record["harness_id"] == "harness.transition_before_after_snapshot"
    )
    harness["transition_ids"] = ["transition.keybinding"]
    harness["invariant_ids"] = ["invariant.inactive_panel_frozen"]
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["transition_ids"] = ["transition.refresh_rebuild"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        invariants=invariants,
        diff_harness_checks=diff_harness_checks,
        runtime_invariants=_complete_invariants(),
        runtime_diff_harness_checks=_complete_diff_harness_checks(),
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check" in failure
        and "harness.transition_before_after_snapshot" in failure
        and "invariant_ids must include at least one invariant covering transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_owner_field_lacks_same_harness_invariant(
    tmp_path: Path,
) -> None:
    diff_harness_checks = _complete_diff_harness_checks()
    harness = next(
        record
        for record in diff_harness_checks
        if record["harness_id"] == "harness.transition_before_after_snapshot"
    )
    harness["owner_field_refs"] = ["panel.tree_selection_key"]
    harness["invariant_ids"] = ["invariant.inactive_panel_frozen"]
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["protected_fields"] = ["field"]
    paths = _write_fixture(
        tmp_path,
        transitions=_complete_transitions(),
        invariants=invariants,
        diff_harness_checks=diff_harness_checks,
        runtime_invariants=_complete_invariants(),
        runtime_diff_harness_checks=_complete_diff_harness_checks(),
    )

    failures = _validate(paths)

    expected = (
        "invariant_ids must include at least one invariant protecting "
        "owner_field_refs field panel.tree_selection_key"
    )
    assert any(
        "diff_harness_check" in failure
        and "harness.transition_before_after_snapshot" in failure
        and expected in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_diff_harness_misses_step_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["transition_ids"] = ["transition.refresh_rebuild"]
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0]["diff_harness_ids"] = [
        diff_harness_checks[0]["harness_id"]
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        diff_harness_checks=diff_harness_checks,
        transition_sequences=transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "diff_harness_ids must include at least one diff harness" in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_action_mismatches_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0]["transition_id"] = "transition.refresh_rebuild"
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "stimulus.action_id" in failure
        and "transition.keybinding" in failure
        and "transition.refresh_rebuild" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_sequence_event_mismatches_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0]["transition_id"] = "transition.refresh_rebuild"
    transition_sequences[0]["steps"][0]["stimulus"] = {
        "event_id": "event.modal_completion"
    }
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "stimulus.event_id" in failure
        and "transition.modal_action" in failure
        and "transition.refresh_rebuild" in failure
        for failure in failures
    )


@pytest.mark.parametrize("precondition", ("stale_snapshot", "generation_mismatch"))
def test_guard_fails_when_transition_sequence_fallback_expectation_is_missing(
    tmp_path: Path, precondition: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0]["precondition"] = precondition
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "require deterministic_fallback" in failure
        for failure in failures
    )


def test_guard_fails_when_blocked_transition_sequence_lacks_no_mutation_expectation(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0]["expected_result"] = "blocked"
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "require no_unrelated_mutation" in failure
        for failure in failures
    )


@pytest.mark.parametrize("precondition", ("stale_snapshot", "generation_mismatch"))
def test_guard_fallback_steps_require_no_unrelated_mutation_expectation(
    tmp_path: Path, precondition: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    step = transition_sequences[0]["steps"][0]
    step["precondition"] = precondition
    step["expected_result"] = "fallback"
    step["deterministic_fallback"] = {
        "outcome": "restore stable identity",
        "allowed_mutation_scope": "declared transition fields only",
    }
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "require no_unrelated_mutation" in failure
        for failure in failures
    )


def test_guard_fallback_no_unrelated_diff_harness_must_be_listed_on_step(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    step = transition_sequences[0]["steps"][0]
    step["precondition"] = "stale_snapshot"
    step["expected_result"] = "fallback"
    step["deterministic_fallback"] = {
        "outcome": "restore stable identity",
        "allowed_mutation_scope": "declared transition fields only",
    }
    step["no_unrelated_mutation"] = {
        "diff_harness_id": "harness.blocked-transition-no-unrelated-mutation",
        "expectation": "no unrelated owner fields change",
    }
    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0].no_unrelated_mutation" in failure
        and "diff_harness_id must be listed in step diff_harness_ids" in failure
        for failure in failures
    )


def test_guard_fallback_no_unrelated_diff_harness_must_cover_step_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        if harness["harness_id"] == "harness.blocked-transition-no-unrelated-mutation":
            harness["transition_ids"] = ["transition.refresh_rebuild"]
    transition_sequences = _complete_transition_sequences()
    step = transition_sequences[0]["steps"][0]
    step["precondition"] = "generation_mismatch"
    step["expected_result"] = "fallback"
    step["diff_harness_ids"] = [
        "harness.transition_before_after_snapshot",
        "harness.blocked-transition-no-unrelated-mutation",
    ]
    step["deterministic_fallback"] = {
        "outcome": "restore stable identity",
        "allowed_mutation_scope": "declared transition fields only",
    }
    step["no_unrelated_mutation"] = {
        "diff_harness_id": "harness.blocked-transition-no-unrelated-mutation",
        "expectation": "no unrelated owner fields change",
    }
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        diff_harness_checks=diff_harness_checks,
        transition_sequences=transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0].no_unrelated_mutation" in failure
        and "diff_harness_id must cover transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    missing_id = transition_sequences[-1]["scenario_id"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transition_sequences=transition_sequences[:-1],
    )

    failures = _validate(paths)

    assert any(
        "runtime transition sequence registry missing scenario id" in failure
        and missing_id in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_is_extra(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transition_sequences = _complete_transition_sequences() + [
        _transition_sequence("split_toggle_f8", scenario_id="sequence.extra")
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence" in failure
        and "scenario_id does not match a transition sequence" in failure
        and "sequence.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_is_duplicate(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences[1]["scenario_id"] = runtime_transition_sequences[0][
        "scenario_id"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[1]" in failure
        and "duplicate runtime transition sequence scenario_id" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_top_level_row_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            "static const AppStateTransitionSequenceMetadata "
            "kAppStateTransitionSequences[] = {\n",
            "static const AppStateTransitionSequenceMetadata "
            "kAppStateTransitionSequences[] = {\n"
            '  {"sequence.malformed"},\n',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0]" in failure
        and "malformed runtime transition sequence registry row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_step_row_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            "static const AppStateTransitionSequenceStepMetadata "
            "kAppStateTransitionSequenceSteps0[] = {\n",
            "static const AppStateTransitionSequenceStepMetadata "
            "kAppStateTransitionSequenceSteps0[] = {\n"
            '  {"step.malformed"},\n',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence_step[kAppStateTransitionSequenceSteps0][0]"
        in failure
        and "malformed runtime transition sequence step row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            "static const char *const kAppStateTransitionSequenceStepInvariantIds0_0[] = {\n"
            '  "invariant.inactive_panel_frozen",',
            "static const char *const kAppStateTransitionSequenceStepInvariantIds0_0[] = {\n"
            "  NULL,\n"
            '  "invariant.inactive_panel_frozen",',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateTransitionSequenceStepInvariantIds0_0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "value", "expected"),
    (
        ("transition_id", "transition.missing", "runtime transition registry"),
        ("stimulus", {"action_id": "ACTION_MISSING"}, "unknown action"),
        ("stimulus", {"event_id": "event.missing"}, "unknown event id"),
        ("invariant_ids", ["invariant.missing"], "runtime invariant id"),
        ("diff_harness_ids", ["harness.missing"], "runtime diff harness id"),
        (
            "generation_domain_expectations",
            [{"domain_id": "domain.missing", "expectation": "fixture"}],
            "unknown generation domain id",
        ),
    ),
)
def test_guard_fails_on_runtime_transition_sequence_invalid_links(
    tmp_path: Path, field: str, value: object, expected: str
) -> None:
    transitions = _complete_transitions()
    runtime_transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences[0]["steps"][0][field] = value
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and expected in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_invariants_do_not_cover_step(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    unrelated_transition_id = next(
        record["id"] for record in transitions if record["id"] != "transition.keybinding"
    )
    runtime_invariants = _complete_invariants()
    for invariant in runtime_invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["transition_ids"] = [unrelated_transition_id]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and "invariant_ids must include at least one invariant covering transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_transition_lacks_same_harness_invariant(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    harness = next(
        record
        for record in runtime_diff_harness_checks
        if record["harness_id"] == "harness.transition_before_after_snapshot"
    )
    harness["transition_ids"] = ["transition.keybinding"]
    harness["invariant_ids"] = ["invariant.inactive_panel_frozen"]
    runtime_invariants = _complete_invariants()
    for invariant in runtime_invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["transition_ids"] = ["transition.refresh_rebuild"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_diff_harness" in failure
        and "harness.transition_before_after_snapshot" in failure
        and "invariant_ids must include at least one invariant covering transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_owner_field_lacks_same_harness_invariant(
    tmp_path: Path,
) -> None:
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    harness = next(
        record
        for record in runtime_diff_harness_checks
        if record["harness_id"] == "harness.transition_before_after_snapshot"
    )
    harness["owner_field_refs"] = ["panel.tree_selection_key"]
    harness["invariant_ids"] = ["invariant.inactive_panel_frozen"]
    runtime_invariants = _complete_invariants()
    for invariant in runtime_invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["protected_fields"] = ["field"]
    paths = _write_fixture(
        tmp_path,
        transitions=_complete_transitions(),
        runtime_invariants=runtime_invariants,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    expected = (
        "invariant_ids must include at least one invariant protecting "
        "owner_field_refs field panel.tree_selection_key"
    )
    assert any(
        "runtime_diff_harness" in failure
        and "harness.transition_before_after_snapshot" in failure
        and expected in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_diff_harness_misses_step_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    runtime_diff_harness_checks[0]["transition_ids"] = ["transition.refresh_rebuild"]
    runtime_transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences[0]["steps"][0]["diff_harness_ids"] = [
        runtime_diff_harness_checks[0]["harness_id"]
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and "diff_harness_ids must include at least one diff harness" in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


def test_guard_runtime_fallback_steps_require_no_unrelated_mutation_expectation(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    for sequences in (transition_sequences, runtime_transition_sequences):
        step = sequences[0]["steps"][0]
        step["precondition"] = "stale_snapshot"
        step["expected_result"] = "fallback"
        step["deterministic_fallback"] = {
            "outcome": "restore stable identity",
            "allowed_mutation_scope": "declared transition fields only",
        }
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and "require no_unrelated_mutation" in failure
        for failure in failures
    )


def test_guard_runtime_fallback_no_unrelated_diff_harness_must_be_listed_on_step(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    for sequences in (transition_sequences, runtime_transition_sequences):
        step = sequences[0]["steps"][0]
        step["precondition"] = "generation_mismatch"
        step["expected_result"] = "fallback"
        step["deterministic_fallback"] = {
            "outcome": "restore stable identity",
            "allowed_mutation_scope": "declared transition fields only",
        }
        step["no_unrelated_mutation"] = {
            "diff_harness_id": "harness.blocked-transition-no-unrelated-mutation",
            "expectation": "no unrelated owner fields change",
        }
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0].no_unrelated_mutation" in failure
        and "diff_harness_id must be listed in step diff_harness_ids" in failure
        for failure in failures
    )


def test_guard_runtime_fallback_no_unrelated_diff_harness_must_cover_step_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        if harness["harness_id"] == "harness.blocked-transition-no-unrelated-mutation":
            harness["transition_ids"] = ["transition.refresh_rebuild"]
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    for sequences in (transition_sequences, runtime_transition_sequences):
        step = sequences[0]["steps"][0]
        step["precondition"] = "stale_snapshot"
        step["expected_result"] = "fallback"
        step["diff_harness_ids"] = [
            "harness.transition_before_after_snapshot",
            "harness.blocked-transition-no-unrelated-mutation",
        ]
        step["deterministic_fallback"] = {
            "outcome": "restore stable identity",
            "allowed_mutation_scope": "declared transition fields only",
        }
        step["no_unrelated_mutation"] = {
            "diff_harness_id": "harness.blocked-transition-no-unrelated-mutation",
            "expectation": "no unrelated owner fields change",
        }
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0].no_unrelated_mutation" in failure
        and "diff_harness_id must cover transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_fallback_drifts(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    step = transition_sequences[0]["steps"][0]
    step["precondition"] = "stale_snapshot"
    step["expected_result"] = "fallback"
    step["deterministic_fallback"] = {
        "outcome": "restore stable identity",
        "allowed_mutation_scope": "declared transition fields only",
    }
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    runtime_transition_sequences[0]["steps"][0]["deterministic_fallback"][
        "outcome"
    ] = "different outcome"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0]" in failure
        and "runtime steps does not match transition sequence" in failure
        and "different outcome" in failure
        for failure in failures
    )


def test_runtime_transition_sequence_lookup_fails_closed_in_startup_guard() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    header = (repo_root / "include" / "ytnova_appstate_actions.h").read_text(
        encoding="utf-8"
    )
    source = (repo_root / "src" / "core" / "main.c").read_text(encoding="utf-8")

    assert "AppStateTransitionSequenceCount(void)" in header
    assert "AppStateTransitionSequenceAt(size_t index)" in header
    assert "AppStateTransitionSequenceLookup(const char *scenario_id)" in header
    assert "AppStateActionCoverageCount(void)" in header
    assert "AppStateActionCoverageAt(size_t index)" in header
    assert "AppStateActionCoverageLookup(YtreeNovaAction action)" in header
    assert "AppStateTransitionSequenceAt(AppStateTransitionSequenceCount())" in source
    assert "AppStateTransitionSequenceLookup(NULL)" in source
    assert 'AppStateTransitionSequenceLookup("")' in source
    assert 'AppStateTransitionSequenceLookup("sequence.__ytnova_unknown__")' in source
    assert "AppStateActionCoverageAt(AppStateActionCoverageCount())" in source
    assert "AppStateActionCoverageLookup((YtreeNovaAction)-1)" in source
    assert (
        "AppStateActionCoverageLookup((YtreeNovaAction)(ACTION_USER_CMD + 1))"
        in source
    )


def test_runtime_transition_sequence_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateTransitionSequenceCount() != required_sequence_id_count" in source
    assert "sequence == NULL || !NonEmptyString(sequence->scenario_id)" in source
    assert (
        "!NonEmptyString(sequence->category) || !NonEmptyString(sequence->flow)"
        in source
    )
    assert "sequence->steps == NULL" in source
    assert "sequence->step_count == 0" in source
    assert (
        "AppStateTransitionSequenceLookup(sequence->scenario_id) != sequence"
        in source
    )
    assert "previous_index < index" in source
    assert "strcmp(previous->scenario_id, sequence->scenario_id) == 0" in source
    assert (
        "AppStateTransitionSequenceAt(AppStateTransitionSequenceCount()) != NULL"
        in source
    )
    assert "AppStateTransitionSequenceLookup(NULL) != NULL" in source
    assert 'AppStateTransitionSequenceLookup("") != NULL' in source
    assert (
        'AppStateTransitionSequenceLookup("sequence.__ytnova_unknown__") != NULL'
        in source
    )
    assert "!AppStateTransitionSequenceStepReady(sequence, step, step_index," in source
    assert "AppStateTransitionLookup(step->transition_id) == NULL" in source
    assert "AppStateInvariantLookup(step->invariant_ids[ref_index]) == NULL" in source
    assert "AppStateTransitionSequenceStepInvariantCoversTransition(step)" in source
    assert "invariant->transition_ids" in source
    assert "invariant->transition_id_count" in source
    assert (
        "AppStateDiffHarnessLookup(step->diff_harness_ids[ref_index]) == NULL"
        in source
    )
    assert "AppStateTransitionSequenceStepDiffHarnessCoversTransition" in source
    assert "!AppStateTransitionSequenceStepDiffHarnessCoversTransition(step)" in source
    assert "AppStateGenerationDomainLookup(expectation->domain_id) == NULL" in source
    assert "!AppStateTransitionSequencesReady()" in source


def test_runtime_transition_sequence_startup_validates_fallback_no_unrelated_shape() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateTransitionSequenceStepNoUnrelatedMutationReady" in source
    assert 'strcmp(step->expected_result, "fallback") == 0' in source
    assert "step->precondition != NULL" in source
    assert (
        "StringListContains(step->diff_harness_ids, step->diff_harness_id_count,"
        in source
    )
    assert (
        "step->no_unrelated_mutation->diff_harness_id)"
        in source
    )
    assert (
        "StringListContains(harness->transition_ids, harness->transition_id_count,"
        in source
    )
    assert "step->transition_id)" in source


def test_runtime_transition_sequence_startup_requires_documented_scenario_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    sequence_doc, sequence_failures = guard._load_json(
        guard.DEFAULT_TRANSITION_SEQUENCES
    )
    required_ids = guard._collect_string_ids(
        sequence_doc,
        collection_key="scenarios",
        id_field="scenario_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredTransitionSequenceScenarioIds\[\]\s*=\s*"
        r"\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert sequence_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredTransitionSequenceScenarioIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert re.search(
        r"AppStateTransitionSequenceLookup\(\s*"
        r"kAppStateRequiredTransitionSequenceScenarioIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_transition_registry_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateTransitionAt(AppStateTransitionCount()) != NULL" in source
    assert "AppStateTransitionLookup(NULL) != NULL" in source
    assert 'AppStateTransitionLookup("") != NULL' in source
    assert 'AppStateTransitionLookup("transition.__ytnova_unknown__") != NULL' in source
    assert "AppStateTransitionCount() != required_transition_id_count" in source
    assert "metadata == NULL || !NonEmptyString(metadata->id)" in source
    assert "metadata->declared_write_set == NULL" in source
    assert "metadata->declared_write_set_count == 0" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->id, metadata->id) == 0" in source
    assert "if (!NonEmptyString(field))" in source
    assert "AppStateTransitionLookup(kAppStateRequiredTransitionIds[index])" in source
    assert "!AppStateTransitionRegistryReady()" in source


def test_runtime_transition_registry_startup_validates_write_set_owner_fields() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert re.search(
        r"for \(write_index = 0; write_index < "
        r"metadata->declared_write_set_count;\s*write_index\+\+\) \{\s*"
        r"const char \*field = metadata->declared_write_set\[write_index\];\s*"
        r"if \(!NonEmptyString\(field\)\)\s*return 0;\s*"
        r"if \(AppStateOwnerFieldLookup\(field\) == NULL\)\s*return 0;",
        source,
        re.S,
    )


def test_runtime_transition_registry_startup_requires_invariant_write_coverage() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateTransitionWriteHasInvariantCoverage(")
    transition_start = source.index("static int AppStateTransitionRegistryReady(void)")
    generation_start = source.index("static int AppStateGenerationDomainsReady(void)")
    helper_body = source[helper_start:transition_start]
    transition_body = source[transition_start:generation_start]

    assert "AppStateInvariantAt(invariant_index)" in helper_body
    assert re.search(
        r"StringListContains\(invariant->transition_ids,\s*"
        r"invariant->transition_id_count, transition_id\)",
        helper_body,
        re.S,
    )
    assert re.search(
        r"StringListContains\(invariant->protected_fields,\s*"
        r"invariant->protected_field_count, field\)",
        helper_body,
        re.S,
    )
    assert "!AppStateTransitionWriteHasInvariantCoverage(metadata->id, field)" in (
        transition_body
    )


def test_runtime_transition_registry_startup_requires_documented_transition_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    transition_doc, transition_failures = guard._load_json(guard.DEFAULT_TRANSITIONS)
    required_ids = guard._collect_string_ids(
        transition_doc,
        collection_key="transitions",
        id_field="id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredTransitionIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert transition_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredTransitionIds",
    )
    assert table_failures == []
    assert table_ids == [
        transition["id"] for transition in transition_doc["transitions"]
    ]
    assert set(table_ids) == required_ids
    assert len(table_ids) == len(required_ids)
    assert re.search(
        r"AppStateTransitionLookup\(\s*"
        r"kAppStateRequiredTransitionIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_guard_fails_when_required_diff_harness_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0].pop("failure_mode")
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "missing required field(s): failure_mode" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_diff_harness_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[1]["harness_id"] = diff_harness_checks[0]["harness_id"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any("duplicate harness_id" in failure for failure in failures)


def test_guard_fails_when_diff_harness_has_unknown_check_category(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["check_category"] = "unknown_diff_harness_check"
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "unknown check_category" in failure
        and "unknown_diff_harness_check" in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_references_unknown_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["transition_ids"] = ["transition.missing"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "transition_ids references unknown transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_references_unknown_owner_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["owner_field_refs"] = ["field.unknown"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "owner_field_refs references unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_protected_field_lacks_diff_harness_owner_ref(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        harness["owner_field_refs"] = ["field"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "invariant[" in failure
        and "protected field lacks diff harness owner_field_refs coverage" in failure
        and "invariant." in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_references_unknown_invariant(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["invariant_ids"] = ["invariant.missing"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "invariant_ids references unknown invariant id" in failure
        and "invariant.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_references_unknown_generation_domain(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    diff_harness_checks[0]["generation_domain_ids"] = ["domain.missing"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "diff_harness_check[0]" in failure
        and "generation_domain_ids references unknown generation domain id" in failure
        and "domain.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_lacks_diff_harness_transition_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    missing_domain_id = str(generation_domains[0]["domain_id"])
    missing_transition_id = str(generation_domains[0]["advances_on_transition_ids"][0])
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        harness["generation_domain_ids"] = [
            domain_id
            for domain_id in _complete_generation_domain_ids()
            if domain_id != missing_domain_id
        ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        generation_domains=generation_domains,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and missing_domain_id in failure
        and "same-domain/same-transition diff harness coverage" in failure
        and missing_transition_id in failure
        for failure in failures
    )


def test_guard_fails_when_diff_harness_write_coverage_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    missing_transition_id = str(transitions[0]["id"])
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        harness["transition_ids"] = [
            transition_id
            for transition_id in _complete_transition_ids()
            if transition_id != missing_transition_id
        ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, diff_harness_checks=diff_harness_checks
    )

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "lacks diff harness coverage" in failure
        and missing_transition_id in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()[1:]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime diff harness registry missing harness id(s)" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_is_extra(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    extra = dict(runtime_diff_harness_checks[0])
    extra["harness_id"] = "harness.extra"
    runtime_diff_harness_checks.append(extra)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_diff_harness" in failure
        and "harness_id does not match a diff harness id" in failure
        and "harness.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_is_duplicate(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    runtime_diff_harness_checks[1]["harness_id"] = runtime_diff_harness_checks[0][
        "harness_id"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any("duplicate runtime diff harness id" in failure for failure in failures)


def test_guard_fails_when_runtime_diff_harness_drifts_from_docs(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    runtime_diff_harness_checks[0]["expected_behavior"] = "runtime drift"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_diff_harness[0]" in failure
        and "runtime expected_behavior does not match diff harness" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_row_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            '"blocked_transition_no_unrelated_mutation", '
            "kAppStateDiffHarnessSnapshotPhases0",
            '"blocked_transition_no_unrelated_mutation", '
            "kAppStateDiffHarnessMissing0",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_diff_harness[0]" in failure
        and "malformed runtime diff harness registry row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            'static const char *const kAppStateDiffHarnessSnapshotPhases0[] '
            '= {\n  "before",',
            'static const char *const kAppStateDiffHarnessSnapshotPhases0[] '
            '= {\n  ,',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateDiffHarnessSnapshotPhases0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "value", "expected"),
    (
        (
            "transition_ids",
            ["transition.missing"],
            "transition_ids does not match runtime transition registry",
        ),
        (
            "owner_field_refs",
            ["field.missing"],
            "owner_field_refs does not match runtime owner field registry",
        ),
        (
            "invariant_ids",
            ["invariant.missing"],
            "invariant_ids does not match runtime invariant registry",
        ),
        (
            "generation_domain_ids",
            ["domain.missing"],
            "generation_domain_ids does not match runtime generation domain registry",
        ),
    ),
)
def test_guard_fails_on_runtime_diff_harness_invalid_linkage(
    tmp_path: Path, field: str, value: list[str], expected: str
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    runtime_diff_harness_checks[0][field] = value
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_diff_harness[0]" in failure
        and expected in failure
        and value[0] in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_diff_harness_write_coverage_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    missing_transition_id = str(transitions[0]["id"])
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        harness["transition_ids"] = [
            transition_id
            for transition_id in _complete_transition_ids()
            if transition_id != missing_transition_id
        ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition[0]" in failure
        and "lacks diff harness coverage" in failure
        and missing_transition_id in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_field_lacks_diff_harness_owner_ref(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_invariants = copy.deepcopy(_complete_invariants())
    missing_owner_field_ref = "panel.tree_selection_key"
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        owner_field_refs = harness["owner_field_refs"]
        assert isinstance(owner_field_refs, list)
        harness["owner_field_refs"] = [
            owner_field_ref
            for owner_field_ref in owner_field_refs
            if owner_field_ref != missing_owner_field_ref
        ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[0]" in failure
        and "protected field lacks runtime diff harness owner_field_refs coverage" in failure
        and str(runtime_invariants[0]["invariant_id"]) in failure
        and missing_owner_field_ref in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_lacks_diff_harness_transition_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    missing_domain_id = str(runtime_generation_domains[0]["domain_id"])
    missing_transition_id = str(
        runtime_generation_domains[0]["advances_on_transition_ids"][0]
    )
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        harness["generation_domain_ids"] = [
            domain_id
            for domain_id in _complete_generation_domain_ids()
            if domain_id != missing_domain_id
        ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[0]" in failure
        and missing_domain_id in failure
        and "same-domain/same-transition diff harness coverage" in failure
        and missing_transition_id in failure
        for failure in failures
    )


def test_runtime_diff_harness_lookup_fails_closed(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "diff_harness_lookup_probe.c"
    binary = tmp_path / "diff_harness_lookup_probe"
    probe.write_text(
        """
#include "ytnova_appstate_actions.h"

int main(void) {
  const AppStateDiffHarnessMetadata *metadata;

  if (AppStateDiffHarnessCount() == 0)
    return 1;
  if (AppStateDiffHarnessAt(AppStateDiffHarnessCount()) != 0)
    return 2;
  if (AppStateDiffHarnessLookup(0) != 0)
    return 3;
  if (AppStateDiffHarnessLookup("") != 0)
    return 4;
  if (AppStateDiffHarnessLookup("harness.__ytnova_unknown__") != 0)
    return 5;

  metadata = AppStateDiffHarnessAt(0);
  if (metadata == 0)
    return 6;
  if (AppStateDiffHarnessLookup(metadata->harness_id) != metadata)
    return 7;
  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-Iinclude",
            str(probe),
            "src/core/appstate_actions.c",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert build.returncode == 0, build.stdout + build.stderr
    run = subprocess.run(
        [str(binary)],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr


def test_runtime_action_coverage_lookup_fails_closed(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "action_coverage_lookup_probe.c"
    binary = tmp_path / "action_coverage_lookup_probe"
    probe.write_text(
        """
#include "ytnova_appstate_actions.h"

int main(void) {
  const AppStateActionCoverageMetadata *metadata;

  if (AppStateActionCoverageCount() == 0)
    return 1;
  if (AppStateActionCoverageAt(AppStateActionCoverageCount()) != 0)
    return 2;
  if (AppStateActionCoverageLookup((YtreeNovaAction)-1) != 0)
    return 3;
  if (AppStateActionCoverageLookup((YtreeNovaAction)(ACTION_USER_CMD + 1)) != 0)
    return 4;

  metadata = AppStateActionCoverageLookup(ACTION_NONE);
  if (metadata == 0)
    return 5;
  if (metadata != AppStateActionCoverageAt((size_t)ACTION_NONE))
    return 6;
  if (metadata->action != ACTION_NONE)
    return 7;
  if (metadata->action_name == 0 || metadata->action_name[0] == '\\0')
    return 8;
  if (metadata->declared_write_set == 0 || metadata->declared_write_set_count == 0)
    return 9;
  if (metadata->migration_notes == 0 || metadata->migration_note_count == 0)
    return 10;
  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-Iinclude",
            str(probe),
            "src/core/appstate_actions.c",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert build.returncode == 0, build.stdout + build.stderr
    run = subprocess.run(
        [str(binary)],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr


def test_guard_fails_when_required_generation_domain_category_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = [
        domain
        for domain in _complete_generation_domains()
        if domain["category"] != "layout_reflow"
    ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation domains missing required category" in failure
        for failure in failures
    )
    assert any("layout_reflow" in failure for failure in failures)


def test_guard_fails_when_required_generation_domain_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0].pop("restore_boundary")
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "missing required field" in failure
        and "restore_boundary" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_generation_domain_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[1]["domain_id"] = generation_domains[0]["domain_id"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[1]" in failure and "duplicate domain_id" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_has_unknown_category(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["category"] = "unknown_generation_domain"
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "unknown category" in failure
        and "unknown_generation_domain" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_owner_field_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["generation_owner_field"] = "field.unknown"
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "generation_owner_field references unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_identity_field_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["identity_fields"] = ["field.unknown"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "identity_fields references unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_transition_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["advances_on_transition_ids"] = ["transition.missing"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "advances_on_transition_ids references unknown transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_write_lacks_domain_transition_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_id = str(transitions[0]["id"])
    generation_domains = _complete_generation_domains()
    for domain in generation_domains:
        domain["advances_on_transition_ids"] = [
            candidate
            for candidate in _complete_transition_ids()
            if candidate != transition_id
        ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "writes generation owner field without generation domain coverage"
        in failure
        and transition_id in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_advances_is_not_a_list(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["advances_on_transition_ids"] = "transition.keybinding"
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "advances_on_transition_ids must be a list" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_domain_advances_element_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["advances_on_transition_ids"] = [123]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "advances_on_transition_ids[0]" in failure
        and "must be a non-empty string" in failure
        for failure in failures
    )


def test_guard_fails_when_empty_generation_domain_advances_lack_projection_note(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["advances_on_transition_ids"] = []
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert any(
        "generation_domain[0]" in failure
        and "empty advances_on_transition_ids requires" in failure
        for failure in failures
    )


def test_guard_accepts_empty_generation_domain_advances_with_projection_note(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    generation_domains = _complete_generation_domains()
    generation_domains[0]["advances_on_transition_ids"] = []
    generation_domains[0]["migration_notes"] = [
        "read-only/projection-only fixture domain"
    ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert failures == []


def test_guard_fails_when_runtime_generation_domain_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()[1:]
    missing_domain_id = _complete_generation_domains()[0]["domain_id"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime generation domain registry missing domain id(s)" in failure
        and missing_domain_id in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_is_extra(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    runtime_generation_domains.append(
        _generation_domain("panel_generation", "domain.extra")
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain" in failure
        and "domain_id does not match a generation domain id" in failure
        and "domain.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_is_duplicate(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    runtime_generation_domains[1]["domain_id"] = runtime_generation_domains[0][
        "domain_id"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[1]" in failure
        and "duplicate runtime generation domain id" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_row_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            "static const AppStateGenerationDomainMetadata "
            "kAppStateGenerationDomains[] = {\n",
            "static const AppStateGenerationDomainMetadata "
            "kAppStateGenerationDomains[] = {\n"
            '  {"domain.malformed"},\n',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[0]" in failure
        and "malformed runtime generation domain registry row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            'static const char *const kAppStateGenerationDomainIdentityFields0[] '
            '= {\n  "field",\n};',
            "static const char *const kAppStateGenerationDomainIdentityFields0[] "
            "= {\n  123,\n};",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateGenerationDomainIdentityFields0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_owner_link_is_invalid(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    runtime_generation_domains[0]["generation_owner_field"] = "field.missing"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[0]" in failure
        and "generation_owner_field does not match runtime owner field registry"
        in failure
        and "field.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_transition_link_is_invalid(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    runtime_generation_domains[0]["advances_on_transition_ids"] = [
        "transition.missing"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[0]" in failure
        and "advances_on_transition_ids does not match runtime transition registry"
        in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_write_lacks_domain_transition_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_id = str(transitions[0]["id"])
    runtime_generation_domains = _complete_generation_domains()
    for domain in runtime_generation_domains:
        domain["advances_on_transition_ids"] = [
            candidate
            for candidate in _complete_transition_ids()
            if candidate != transition_id
        ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition[0]" in failure
        and "writes generation owner field without generation domain coverage"
        in failure
        and transition_id in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_generation_domain_stale_boundary_fields_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_generation_domains = _complete_generation_domains()
    runtime_generation_domains[0]["stale_snapshot_policy"] = "changed stale policy"
    runtime_generation_domains[0]["fail_closed_fallback"] = "changed fallback"
    runtime_generation_domains[0]["restore_boundary"] = "changed boundary"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_generation_domains=runtime_generation_domains,
    )

    failures = _validate(paths)

    assert any(
        "runtime_generation_domain[0]" in failure
        and "runtime stale_snapshot_policy does not match generation domain"
        in failure
        for failure in failures
    )
    assert any(
        "runtime_generation_domain[0]" in failure
        and "runtime fail_closed_fallback does not match generation domain" in failure
        for failure in failures
    )
    assert any(
        "runtime_generation_domain[0]" in failure
        and "runtime restore_boundary does not match generation domain" in failure
        for failure in failures
    )


def test_guard_fails_when_generation_effect_names_unregistered_generation_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transitions[0]["generation_effect"] = "Increment missing_generation."
    paths = _write_fixture(tmp_path, transitions=transitions)

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "generation_effect names unregistered generation field" in failure
        and "missing_generation" in failure
        for failure in failures
    )


def test_guard_fails_when_required_invariant_category_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = [
        invariant
        for invariant in _complete_invariants()
        if invariant["category"] != "stale_snapshot_fail_closed"
    ]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any("invariants missing required category" in failure for failure in failures)
    assert any("stale_snapshot_fail_closed" in failure for failure in failures)


def test_guard_fails_when_required_invariant_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0].pop("failure_mode")
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "missing required field" in failure
        and "failure_mode" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_invariant_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[1]["invariant_id"] = invariants[0]["invariant_id"]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[1]" in failure and "duplicate invariant_id" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_has_unknown_category(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["category"] = "unknown_invariant"
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "unknown category" in failure
        and "unknown_invariant" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_references_unknown_owner_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["protected_fields"] = ["field.unknown"]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "protected_fields references unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_references_unknown_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["transition_ids"] = ["transition.missing"]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "transition_ids references unknown transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_references_unknown_dispatch_surface(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["dispatch_surface_ids"] = ["surface.missing"]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "dispatch_surface_ids references unknown dispatch surface id" in failure
        and "surface.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_dispatch_surfaces_are_not_a_list(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["dispatch_surface_ids"] = "surface.key_decode_input_dispatch"
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "dispatch_surface_ids must be a list" in failure
        for failure in failures
    )


def test_guard_fails_when_invariant_dispatch_surface_element_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["dispatch_surface_ids"] = [123]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "dispatch_surface_ids[0]" in failure
        and "must be a non-empty string" in failure
        for failure in failures
    )


def test_guard_fails_when_empty_invariant_dispatch_surfaces_lack_cross_cutting_note(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    invariants[0]["dispatch_surface_ids"] = []
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "invariant[0]" in failure
        and "empty dispatch_surface_ids requires a cross-cutting" in failure
        for failure in failures
    )


def test_guard_accepts_empty_invariant_dispatch_surfaces_with_cross_cutting_note(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    moved_surface_ids = list(invariants[0]["dispatch_surface_ids"])
    invariants[1]["dispatch_surface_ids"] = list(
        dict.fromkeys(invariants[1]["dispatch_surface_ids"] + moved_surface_ids)
    )
    invariants[0]["dispatch_surface_ids"] = []
    invariants[0]["migration_notes"] = ["cross-cutting fixture invariant"]
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert failures == []


def test_guard_fails_when_required_dispatch_surface_category_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = [
        surface
        for surface in _complete_dispatch_surfaces()
        if surface["category"] != "watcher_live_refresh"
    ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any("dispatch surfaces missing required category" in failure for failure in failures)
    assert any("watcher_live_refresh" in failure for failure in failures)


def test_guard_fails_when_required_dispatch_surface_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0].pop("source_path")
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "missing required field" in failure
        and "source_path" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_dispatch_surface_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[1]["surface_id"] = dispatch_surfaces[0]["surface_id"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[1]" in failure and "duplicate surface_id" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_has_unknown_category(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["category"] = "unknown_dispatch_surface"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "unknown category" in failure
        and "unknown_dispatch_surface" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_references_unknown_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["transition_id"] = "transition.missing"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "transition_id does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_on_malformed_dispatch_surface_source_path(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["source_path"] = "../src/ui/key_engine.c"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "source_path must be a relative repository path" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_source_path_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["source_path"] = "src/ui/missing_dispatch_surface.c"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "source_path does not exist" in failure
        and "src/ui/missing_dispatch_surface.c" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_source_path_is_outside_src(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["source_path"] = "scripts/check_appstate_contract.py"
    dispatch_surfaces[0]["entry_symbol_or_path"] = "validate_contract"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "source_path must point inside src/" in failure
        and "scripts/check_appstate_contract.py" in failure
        for failure in failures
    )


def test_guard_fails_on_malformed_dispatch_surface_entry_symbol_or_path(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["entry_symbol_or_path"] = "../GetEventOrKey"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "entry_symbol_or_path is malformed" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_entry_symbol_is_not_in_source(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["entry_symbol_or_path"] = "MissingDispatchSurfaceEntry"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "entry_symbol_or_path not found in source_path" in failure
        and "MissingDispatchSurfaceEntry" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_allowed_writes_are_not_a_list(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["allowed_direct_writes"] = "field"
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "allowed_direct_writes must be a list" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_allowed_write_is_unregistered(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["allowed_direct_writes"] = ["field.unknown"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_allowed_write_exceeds_transition_contract(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["allowed_direct_writes"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "allowed_direct_writes[0]" in failure
        and "outside transition declared_write_set" in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_allowed_write_lacks_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    surface_id = str(dispatch_surfaces[0]["surface_id"])
    fallback_surface_id = str(dispatch_surfaces[1]["surface_id"])
    invariants = _complete_invariants()
    for invariant in invariants:
        _remove_dispatch_surface_id(invariant, surface_id, fallback_surface_id)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        dispatch_surfaces=dispatch_surfaces,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and surface_id in failure
        and "allowed_direct_writes[0]" in failure
        and "field" in failure
        and "lacks same-surface invariant coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_allowed_write_splits_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    surface_id = str(dispatch_surfaces[0]["surface_id"])
    fallback_surface_id = str(dispatch_surfaces[1]["surface_id"])
    invariants = _complete_invariants()
    _split_dispatch_surface_invariant_coverage(
        invariants, surface_id, fallback_surface_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        dispatch_surfaces=dispatch_surfaces,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and surface_id in failure
        and "allowed_direct_writes[0]" in failure
        and "field" in failure
        and "lacks same-surface invariant coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_transition_write_splits_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    target_index = next(
        index
        for index, record in enumerate(transitions)
        if record["id"] != "transition.keybinding"
    )
    transition_id = str(transitions[target_index]["id"])
    fallback_transition_id = next(
        str(record["id"]) for record in transitions if record["id"] != transition_id
    )
    invariants = _complete_invariants()
    _split_transition_invariant_coverage(
        invariants, transition_id, fallback_transition_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        f"transition[{target_index}]" in failure
        and "declared_write_set[0]" in failure
        and transition_id in failure
        and "field" in failure
        and "lacks same-transition invariant coverage" in failure
        for failure in failures
    )


def test_guard_accepts_empty_dispatch_surface_allowed_writes(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["allowed_direct_writes"] = []
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert failures == []


def test_guard_fails_when_runtime_dispatch_surface_metadata_drifts(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_dispatch_surfaces = _complete_dispatch_surfaces()
    runtime_dispatch_surfaces[0]["category"] = "runtime_drift"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and "runtime category does not match dispatch surface" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_id_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_dispatch_surfaces = _complete_dispatch_surfaces()[1:]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
    )

    failures = _validate(paths)

    assert any(
        "runtime dispatch surface registry missing surface id(s)" in failure
        and "surface.directory_window_action_dispatch" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_id_is_extra(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_dispatch_surfaces = _complete_dispatch_surfaces()
    extra = dict(runtime_dispatch_surfaces[0])
    extra["surface_id"] = "surface.extra"
    runtime_dispatch_surfaces.append(extra)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface" in failure
        and "does not match a dispatch surface id: surface.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_row_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace('{"surface.key_decode_input_dispatch"', "{123", 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[3]" in failure
        and "malformed runtime dispatch surface registry row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace(
            '"fixture coverage",',
            "123,",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateDispatchSurfaceMigrationNotes0" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_allowed_write_exceeds_transition_contract(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["allowed_direct_writes"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        dispatch_surfaces=dispatch_surfaces,
        runtime_dispatch_surfaces=copy.deepcopy(dispatch_surfaces),
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and "allowed_direct_writes[0]" in failure
        and "outside transition declared_write_set" in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_allowed_write_lacks_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    runtime_dispatch_surfaces = copy.deepcopy(dispatch_surfaces)
    surface_id = str(runtime_dispatch_surfaces[0]["surface_id"])
    fallback_surface_id = str(runtime_dispatch_surfaces[1]["surface_id"])
    runtime_invariants = _complete_invariants()
    for invariant in runtime_invariants:
        _remove_dispatch_surface_id(invariant, surface_id, fallback_surface_id)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        dispatch_surfaces=dispatch_surfaces,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and surface_id in failure
        and "allowed_direct_writes[0]" in failure
        and "field" in failure
        and "lacks same-surface invariant coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_allowed_write_splits_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    runtime_dispatch_surfaces = copy.deepcopy(dispatch_surfaces)
    surface_id = str(runtime_dispatch_surfaces[0]["surface_id"])
    fallback_surface_id = str(runtime_dispatch_surfaces[1]["surface_id"])
    runtime_invariants = _complete_invariants()
    _split_dispatch_surface_invariant_coverage(
        runtime_invariants, surface_id, fallback_surface_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        dispatch_surfaces=dispatch_surfaces,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and surface_id in failure
        and "allowed_direct_writes[0]" in failure
        and "field" in failure
        and "lacks same-surface invariant coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_write_splits_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [dict(record) for record in transitions]
    target_index = next(
        index
        for index, record in enumerate(runtime_transitions)
        if record["id"] != "transition.keybinding"
    )
    transition_id = str(runtime_transitions[target_index]["id"])
    fallback_transition_id = next(
        str(record["id"])
        for record in runtime_transitions
        if record["id"] != transition_id
    )
    runtime_transitions[target_index]["declared_write_set"] = [
        "panel.tree_selection_key"
    ]
    runtime_invariants = _complete_invariants()
    _split_transition_invariant_coverage(
        runtime_invariants,
        transition_id,
        fallback_transition_id,
        field="panel.tree_selection_key",
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        f"runtime_transition[{target_index}]" in failure
        and "declared_write_set[0]" in failure
        and transition_id in failure
        and "panel.tree_selection_key" in failure
        and "lacks same-transition invariant coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_dispatch_surface_transition_is_not_registered(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_dispatch_surfaces = _complete_dispatch_surfaces()
    runtime_dispatch_surfaces[0]["transition_id"] = "transition.missing"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and "transition_id does not match runtime transition registry" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_owner_field_records(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    owner_fields[1]["field"] = owner_fields[0]["field"]
    paths = _write_fixture(tmp_path, transitions=transitions, owner_fields=owner_fields)

    failures = _validate(paths)

    assert any(
        "owner_field[1]" in failure and "duplicate field" in failure for failure in failures
    )


def test_guard_fails_when_required_owner_metadata_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    owner_fields[0].pop("canonical_owner")
    paths = _write_fixture(tmp_path, transitions=transitions, owner_fields=owner_fields)

    failures = _validate(paths)

    assert any(
        "owner_field[0]" in failure
        and "missing required field" in failure
        and "canonical_owner" in failure
        for failure in failures
    )


def test_guard_fails_on_malformed_owner_invariant_checks(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    owner_fields[0]["invariant_checks"] = []
    paths = _write_fixture(tmp_path, transitions=transitions, owner_fields=owner_fields)

    failures = _validate(paths)

    assert any(
        "owner_field[0]" in failure
        and "invariant_checks" in failure
        and "must be non-empty" in failure
        for failure in failures
    )


def test_guard_fails_on_unknown_owner_field_invariant_id(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    owner_fields[0]["invariant_checks"] = ["invariant.missing"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        owner_fields=owner_fields,
        runtime_owner_fields=_complete_owner_fields(),
    )

    failures = _validate(paths)

    assert any(
        "owner_field[0]" in failure
        and "invariant_checks[0] does not match runtime invariant registry"
        and "invariant.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_owner_field_record_is_malformed(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    malformed_owner_fields: list[object] = [123, owner_fields[1]]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        owner_fields=malformed_owner_fields,  # type: ignore[arg-type]
        runtime_owner_fields=_complete_owner_fields(),
    )

    failures = _validate(paths)

    assert any(
        "owner_field[0]" in failure and "record must be an object" in failure
        for failure in failures
    )


def test_guard_fails_when_owner_field_invariant_does_not_protect_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["protected_fields"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        owner_fields=owner_fields,
        invariants=invariants,
        runtime_owner_fields=_complete_owner_fields(),
        runtime_invariants=_complete_invariants(),
    )

    failures = _validate(paths)

    assert any(
        "owner_field[0]" in failure
        and "invariant_checks must include at least one invariant"
        and "owner field field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_metadata_drifts(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_owner_fields = _complete_owner_fields()
    runtime_owner_fields[0]["canonical_owner"] = "runtime drift"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_owner_fields=runtime_owner_fields,
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field[0]" in failure
        and "runtime canonical_owner does not match owner field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_invariant_id_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_owner_fields = _complete_owner_fields()
    runtime_owner_fields[0]["invariant_checks"] = ["invariant.missing"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_owner_fields=runtime_owner_fields,
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field[0]" in failure
        and "invariant_checks[0] does not match runtime invariant registry"
        and "invariant.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_invariant_does_not_protect_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_invariants = _complete_invariants()
    for invariant in runtime_invariants:
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen":
            invariant["protected_fields"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field[0]" in failure
        and "invariant_checks must include at least one invariant"
        and "owner field field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    owner_fields = _complete_owner_fields()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_owner_fields=owner_fields[1:],
    )

    failures = _validate(paths)

    assert any(
        "runtime owner field registry missing field" in failure
        and owner_fields[0]["field"] in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_is_extra(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    extra = _owner_field("field.extra")
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_owner_fields=_complete_owner_fields() + [extra],
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field" in failure
        and "field does not match an owner field: field.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_owner_fields = _complete_owner_fields()
    runtime_owner_fields[1]["field"] = runtime_owner_fields[0]["field"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_owner_fields=runtime_owner_fields,
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field[1]" in failure
        and "duplicate runtime owner field" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_row_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace('{"field",', "{NULL,", 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_owner_field[0]" in failure
        and "malformed runtime owner field registry row" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_owner_field_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace(
            'static const char *const kAppStateOwnerFieldInvariantChecks0[] = {\n'
            '  "invariant.inactive_panel_frozen",',
            "static const char *const kAppStateOwnerFieldInvariantChecks0[] = {\n"
            "  NULL,",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateOwnerFieldInvariantChecks0[0]" in failure
        and "malformed string literal entry" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_fails_on_unknown_transition_declared_write_set(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transitions[0]["declared_write_set"] = ["field.unknown"]
    paths = _write_fixture(tmp_path, transitions=transitions)

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_on_unknown_action_declared_write_set(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["declared_write_set"] = ["field.unknown"]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_on_unknown_event_declared_write_set(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["declared_write_set"] = ["field.unknown"]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "unregistered owner field" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_required_category_is_missing(tmp_path: Path) -> None:
    transitions = [
        _transition(category, "transition.keybinding" if category == "keybinding" else None)
        for category in REQUIRED_CATEGORIES
        if category != "render_reflow"
    ]
    paths = _write_fixture(tmp_path, transitions=transitions)

    failures = _validate(paths)

    assert any("missing required category" in failure for failure in failures)
    assert any("render_reflow" in failure for failure in failures)


def test_guard_fails_when_required_transition_field_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transitions[0].pop("owner")
    paths = _write_fixture(tmp_path, transitions=transitions)

    failures = _validate(paths)

    assert any("missing required field" in failure and "owner" in failure for failure in failures)


def test_guard_fails_when_required_shim_field_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim.pop("removal_trigger")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "missing required field" in failure and "removal_trigger" in failure
        for failure in failures
    )


def test_guard_fails_when_required_shim_owner_field_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim.pop("owner_field_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "missing required field" in failure
        and "owner_field_refs" in failure
        for failure in failures
    )


def test_guard_fails_when_required_shim_generation_domain_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim.pop("generation_domain_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "missing required field" in failure
        and "generation_domain_refs" in failure
        for failure in failures
    )


def test_guard_fails_when_required_shim_diff_harness_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim.pop("diff_harness_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "missing required field" in failure
        and "diff_harness_refs" in failure
        for failure in failures
    )


def test_guard_fails_when_required_action_field_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0].pop("owner")
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure and "missing required field" in failure and "owner" in failure
        for failure in failures
    )


def test_guard_fails_when_required_event_class_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = [
        event for event in _complete_events() if event["event_class"] != "render_reflow"
    ]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any("event coverage missing required event_class" in failure for failure in failures)
    assert any("render_reflow" in failure for failure in failures)


def test_guard_fails_when_event_has_unknown_class(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events() + [_event("unknown_event_class")]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any("unknown event_class" in failure and "unknown_event_class" in failure for failure in failures)


def test_guard_fails_when_event_class_is_duplicated(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[1]["event_class"] = events[0]["event_class"]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any("event[1]" in failure and "duplicate event_class" in failure for failure in failures)


def test_guard_fails_when_required_event_field_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0].pop("source")
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure and "missing required field" in failure and "source" in failure
        for failure in failures
    )


def test_guard_fails_on_duplicate_transition_and_shim_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transitions[1]["id"] = transitions[0]["id"]
    duplicate_shim = _shim()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=[_shim(), duplicate_shim],
    )

    failures = _validate(paths)

    assert any("transition[1]" in failure and "duplicate id" in failure for failure in failures)
    assert any("shim[1]" in failure and "duplicate id" in failure for failure in failures)


def test_guard_fails_on_duplicate_action_records(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[1]["action"] = actions[0]["action"]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any("action[1]" in failure and "duplicate action" in failure for failure in failures)


def test_guard_fails_on_duplicate_event_ids(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[1]["event_id"] = events[0]["event_id"]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any("event[1]" in failure and "duplicate event_id" in failure for failure in failures)


def test_guard_fails_on_empty_required_list_fields(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transitions[0]["declared_write_set"] = []
    shim = _shim()
    shim["invariant_checks"] = []
    actions = _complete_actions()
    actions[0]["declared_write_set"] = []
    events = _complete_events()
    events[0]["trigger_paths"] = []
    paths = _write_fixture(
        tmp_path, transitions=transitions, shims=[shim], actions=actions, events=events
    )

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "declared_write_set" in failure
        and "must be non-empty" in failure
        for failure in failures
    )
    assert any(
        "shim[0]" in failure
        and "invariant_checks" in failure
        and "must be non-empty" in failure
        for failure in failures
    )
    assert any(
        "action[0]" in failure
        and "declared_write_set" in failure
        and "must be non-empty" in failure
        for failure in failures
    )
    assert any(
        "event[0]" in failure
        and "trigger_paths" in failure
        and "must be non-empty" in failure
        for failure in failures
    )


def test_guard_fails_on_non_list_required_list_fields(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    transitions[0]["declared_write_set"] = "field"
    shim = _shim()
    shim["invariant_checks"] = "invariant"
    actions = _complete_actions()
    actions[0]["migration_notes"] = "note"
    events = _complete_events()
    events[0]["declared_write_set"] = "field"
    paths = _write_fixture(
        tmp_path, transitions=transitions, shims=[shim], actions=actions, events=events
    )

    failures = _validate(paths)

    assert any(
        "transition[0]" in failure
        and "declared_write_set" in failure
        and "must be a non-empty list" in failure
        for failure in failures
    )
    assert any(
        "shim[0]" in failure
        and "invariant_checks" in failure
        and "must be a non-empty list" in failure
        for failure in failures
    )
    assert any(
        "action[0]" in failure
        and "migration_notes" in failure
        and "must be a non-empty list" in failure
        for failure in failures
    )
    assert any(
        "event[0]" in failure
        and "declared_write_set" in failure
        and "must be a non-empty list" in failure
        for failure in failures
    )


@pytest.mark.parametrize(("record_type", "field", "label"), REQUIRED_LIST_FIELD_CASES)
@pytest.mark.parametrize("value", ([123], [{"not": "a string"}]))
def test_guard_fails_on_non_string_required_list_elements(
    tmp_path: Path, record_type: str, field: str, label: str, value: object
) -> None:
    paths = _fixture_with_list_field_value(tmp_path, record_type, field, value)

    failures = _validate(paths)

    assert any(
        label in failure
        and f"{field}[0]" in failure
        and "must be a non-empty string" in failure
        for failure in failures
    )


@pytest.mark.parametrize(("record_type", "field", "label"), REQUIRED_LIST_FIELD_CASES)
@pytest.mark.parametrize("value", ([""], ["   "]))
def test_guard_fails_on_blank_required_list_elements(
    tmp_path: Path, record_type: str, field: str, label: str, value: object
) -> None:
    paths = _fixture_with_list_field_value(tmp_path, record_type, field, value)

    failures = _validate(paths)

    assert any(
        label in failure
        and f"{field}[0]" in failure
        and "must be a non-empty string" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_targets_unknown_transition(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=[_shim(target_transition="transition.missing")],
    )

    failures = _validate(paths)

    assert any(
        "target_transition does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_invariant_check_is_not_runtime_registered(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim["invariant_checks"] = ["invariant.not_runtime_registered"]
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "invariant_checks" in failure
        and "runtime invariant registry" in failure
        and "invariant.not_runtime_registered" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_invariant_checks_do_not_cover_target_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(target_transition="transition.render_reflow")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and (
            "invariant_checks must include at least one invariant covering "
            "target_transition transition.render_reflow"
        )
        in failure
        for failure in failures
    )


def test_guard_fails_when_each_shim_invariant_check_lacks_generation_target_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim["invariant_checks"] = [
        "invariant.inactive_panel_frozen",
        "invariant.render_projection_read_only",
    ]
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.render_projection_read_only":
            invariant["transition_ids"] = ["transition.render_reflow"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=[shim],
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "invariant_checks[1] must cover target_transition transition.keybinding"
        in failure
        and "invariant.render_projection_read_only" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_generation_domain_ref_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(generation_domain_refs=["domain.unknown"])
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "generation_domain_refs does not match generation-domain registry"
        in failure
        and "domain.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_generation_domain_ref_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(
        generation_domain_refs=["domain.panel_generation", "domain.panel_generation"]
    )
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "duplicate generation_domain_refs[1]" in failure
        and "domain.panel_generation" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_generation_owner_ref_lacks_matching_domain(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transitions[0]["declared_write_set"] = ["field", "panel.panel_generation"]
    owner_fields = _complete_owner_fields() + [_owner_field("panel.panel_generation")]
    generation_domains = _complete_generation_domains()
    generation_domains[0]["generation_owner_field"] = "panel.panel_generation"
    generation_domains[0]["identity_fields"] = ["panel.panel_generation"]
    shim = _shim(
        owner_field_refs=["panel.panel_generation"],
        generation_domain_refs=["domain.volume_generation"],
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        owner_fields=owner_fields,
        generation_domains=generation_domains,
        shims=[shim],
    )

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "generation_domain_refs must include a domain whose "
        "generation_owner_field is panel.panel_generation" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_owner_field_ref_is_unknown(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["field.unknown"])
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "owner_field_refs does not match owner-field registry" in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_owner_field_ref_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["field", "field"])
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "duplicate owner_field_refs[1]" in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_diff_harness_ref_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(diff_harness_refs=["harness.unknown"])
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "diff_harness_refs references unknown diff harness id" in failure
        and "harness.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_diff_harness_ref_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(
        diff_harness_refs=[
            "harness.transition_before_after_snapshot",
            "harness.transition_before_after_snapshot",
        ]
    )
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "duplicate diff_harness_refs[1]" in failure
        and "harness.transition_before_after_snapshot" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_diff_harness_lacks_target_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        harness["transition_ids"] = ["transition.render_reflow"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "diff_harness_refs must include at least one diff harness covering"
        in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "replacement", "expected_ref"),
    (
        ("owner_field_refs", ["panel.tree_selection_key"], "field"),
        (
            "invariant_ids",
            ["invariant.blocked_transition_determinism"],
            "invariant.inactive_panel_frozen",
        ),
        ("generation_domain_ids", ["domain.volume_generation"], "domain.panel_generation"),
    ),
)
def test_guard_fails_when_shim_diff_harness_union_lacks_declared_coverage(
    tmp_path: Path,
    field: str,
    replacement: list[str],
    expected_ref: str,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        if harness["harness_id"] == "harness.transition_before_after_snapshot":
            harness[field] = replacement
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "lacks referenced diff_harness_refs coverage" in failure
        and expected_ref in failure
        for failure in failures
    )


def test_guard_fails_when_write_capable_shim_owner_field_is_outside_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["panel.tree_selection_key"])
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    "write_permission",
    (
        "Do not write stale mirror directly; write only from transition commit after canonical state changes.",
        "No write should happen before transition commit; write the compatibility mirror after canonical state changes.",
        "Never write before canonical state changes; write during the transition commit.",
        "Read-only before commit; write the compatibility mirror after canonical state changes.",
    ),
)
def test_guard_treats_explicit_write_capability_as_authoritative_over_prose(
    tmp_path: Path, write_permission: str
) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["panel.tree_selection_key"])
    shim["write_permission"] = write_permission
    shim["write_capability"] = "write_capable"
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_write_capability_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim.pop("write_capability")
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "write_capability" in failure
        and "missing required field" in failure
        for failure in failures
    )


def test_guard_fails_when_shim_write_capability_is_unknown(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim["write_capability"] = "sometimes"
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "write_capability must be one of" in failure
        and "sometimes" in failure
        for failure in failures
    )


def test_guard_allows_no_write_shim_owner_field_outside_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["panel.tree_selection_key"])
    shim["write_permission"] = "Never write authoritative selection from this projection."
    shim["write_capability"] = "no_write"
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert not any(
        "shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        for failure in failures
    )


def test_guard_allows_read_only_projection_shim_owner_field_outside_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim(owner_field_refs=["panel.tree_selection_key"])
    shim["write_permission"] = "Read-only projection for render calculations."
    shim["write_capability"] = "read_only_projection"
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert not any(
        "shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_metadata_drifts(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["owner"] = "different owner"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "runtime owner does not match shim" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_owner_field_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(owner_field_refs=["panel.tree_selection_key"])]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "runtime owner_field_refs does not match shim" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_generation_domain_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [
        _shim(generation_domain_refs=["domain.panel_generation", "domain.volume_generation"])
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "runtime generation_domain_refs does not match shim" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_diff_harness_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [
        _shim(
            diff_harness_refs=[
                "harness.transition_before_after_snapshot",
                "harness.declared_write_set_diff",
            ]
        )
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "runtime diff_harness_refs does not match shim" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_diff_harness_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0].pop("diff_harness_refs")
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "diff_harness_refs must be non-empty" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_diff_harness_ref_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(diff_harness_refs=["harness.unknown"])]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "diff_harness_refs references unknown diff harness id" in failure
        and "harness.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_diff_harness_ref_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [
        _shim(
            diff_harness_refs=[
                "harness.transition_before_after_snapshot",
                "harness.transition_before_after_snapshot",
            ]
        )
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "duplicate diff_harness_refs[1]" in failure
        and "harness.transition_before_after_snapshot" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_diff_harness_lacks_target_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        harness["transition_ids"] = ["transition.render_reflow"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "diff_harness_refs must include at least one diff harness covering"
        in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "replacement", "expected_ref"),
    (
        ("owner_field_refs", ["panel.tree_selection_key"], "field"),
        (
            "invariant_ids",
            ["invariant.blocked_transition_determinism"],
            "invariant.inactive_panel_frozen",
        ),
        ("generation_domain_ids", ["domain.volume_generation"], "domain.panel_generation"),
    ),
)
def test_guard_fails_when_runtime_shim_diff_harness_union_lacks_declared_coverage(
    tmp_path: Path,
    field: str,
    replacement: list[str],
    expected_ref: str,
) -> None:
    transitions = _complete_transitions()
    runtime_diff_harness_checks = _complete_diff_harness_checks()
    for harness in runtime_diff_harness_checks:
        if harness["harness_id"] == "harness.transition_before_after_snapshot":
            harness[field] = replacement
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "lacks referenced diff_harness_refs coverage" in failure
        and expected_ref in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_generation_domain_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0].pop("generation_domain_refs")
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "generation_domain_refs must be non-empty" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_generation_domain_ref_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(generation_domain_refs=["domain.unknown"])]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "generation_domain_refs does not match runtime generation domain registry"
        in failure
        and "domain.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_generation_domain_ref_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [
        _shim(
            generation_domain_refs=[
                "domain.panel_generation",
                "domain.panel_generation",
            ]
        )
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "duplicate generation_domain_refs[1]" in failure
        and "domain.panel_generation" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_generation_owner_ref_lacks_matching_domain(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transitions[0]["declared_write_set"] = ["field", "panel.panel_generation"]
    owner_fields = _complete_owner_fields() + [_owner_field("panel.panel_generation")]
    generation_domains = _complete_generation_domains()
    generation_domains[0]["generation_owner_field"] = "panel.panel_generation"
    generation_domains[0]["identity_fields"] = ["panel.panel_generation"]
    runtime_shims = [
        _shim(
            owner_field_refs=["panel.panel_generation"],
            generation_domain_refs=["domain.volume_generation"],
        )
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        owner_fields=owner_fields,
        generation_domains=generation_domains,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "generation_domain_refs must include a domain whose "
        "generation_owner_field is panel.panel_generation" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_owner_field_ref_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(owner_field_refs=["field.unknown"])]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "owner_field_refs does not match runtime owner field registry"
        in failure
        and "field.unknown" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_write_capability_is_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["write_capability"] = ""
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "write_capability must be one of" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_write_capability_is_unknown(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["write_capability"] = "sometimes"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "write_capability must be one of" in failure
        and "sometimes" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    "write_permission",
    (
        "Do not write stale mirror directly; write only from transition commit after canonical state changes.",
        "No write should happen before transition commit; write the compatibility mirror after canonical state changes.",
        "Never write before canonical state changes; write during the transition commit.",
        "Read-only before commit; write the compatibility mirror after canonical state changes.",
    ),
)
def test_guard_runtime_treats_explicit_write_capability_as_authoritative_over_prose(
    tmp_path: Path, write_permission: str
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(owner_field_refs=["panel.tree_selection_key"])]
    runtime_shims[0]["write_permission"] = write_permission
    runtime_shims[0]["write_capability"] = "write_capable"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_write_capable_shim_owner_field_is_outside_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(owner_field_refs=["panel.tree_selection_key"])]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "owner_field_refs must be declared by target_transition write set"
        in failure
        and "panel.tree_selection_key" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_id_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    second_shim = _shim()
    second_shim["id"] = "shim.second"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=[_shim(), second_shim],
        runtime_shims=[_shim()],
    )

    failures = _validate(paths)

    assert any(
        "runtime compatibility shim registry missing shim id" in failure
        and "shim.second" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_id_is_extra(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    extra_shim = _shim()
    extra_shim["id"] = "shim.extra"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=[_shim(), extra_shim],
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[1]" in failure
        and "id does not match a shim id" in failure
        and "shim.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_target_transition_drifts(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(target_transition="transition.render_reflow")]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "runtime target_transition does not match shim" in failure
        and "transition.render_reflow" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_invariant_checks_do_not_cover_target_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim(target_transition="transition.render_reflow")]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and (
            "invariant_checks must include at least one invariant covering "
            "target_transition transition.render_reflow"
        )
        in failure
        for failure in failures
    )


def test_guard_fails_when_each_runtime_shim_invariant_check_lacks_generation_target_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["invariant_checks"] = [
        "invariant.inactive_panel_frozen",
        "invariant.render_projection_read_only",
    ]
    invariants = _complete_invariants()
    for invariant in invariants:
        if invariant["invariant_id"] == "invariant.render_projection_read_only":
            invariant["transition_ids"] = ["transition.render_reflow"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "invariant_checks[1] must cover target_transition transition.keybinding"
        in failure
        and "invariant.render_projection_read_only" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_invariant_checks_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["invariant_checks"] = []
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "invariant_checks must be non-empty" in failure
        for failure in failures
    )


def test_guard_preserves_runtime_shim_invariant_array_parse_failures(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace(
            'static const char *const kAppStateCompatibilityShimInvariantChecks0[] = {\n'
            '  "invariant.inactive_panel_frozen",',
            "static const char *const kAppStateCompatibilityShimInvariantChecks0[] = {\n"
            "  NULL,",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateCompatibilityShimInvariantChecks0[0]" in failure
        and "malformed string literal entry" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_preserves_runtime_shim_generation_domain_array_parse_failures(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace(
            'static const char *const kAppStateCompatibilityShimGenerationDomainRefs0[] = {\n'
            '  "domain.panel_generation",',
            "static const char *const kAppStateCompatibilityShimGenerationDomainRefs0[] = {\n"
            "  NULL,",
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateCompatibilityShimGenerationDomainRefs0[0]" in failure
        and "malformed string literal entry" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_shim_target_transition_is_not_runtime_registered(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [
        record for record in transitions if record["id"] != "transition.keybinding"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "target_transition does not match runtime transition registry" in failure
        and "transition.keybinding" in failure
        for failure in failures
    )

def test_guard_fails_when_runtime_invariant_metadata_drifts(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    runtime_invariants = _complete_invariants()
    runtime_invariants[0]["protected_fields"] = ["field"]
    runtime_invariants[1]["migration_notes"] = ["different note"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[0]" in failure
        and "runtime protected_fields does not match invariant" in failure
        for failure in failures
    )
    assert any(
        "runtime_invariant[1]" in failure
        and "runtime migration_notes does not match invariant" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_id_is_missing(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    missing_id = invariants[-1]["invariant_id"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=invariants[:-1],
    )

    failures = _validate(paths)

    assert any(
        "runtime invariant registry missing invariant id" in failure
        and missing_id in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_id_is_extra(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    extra_invariant = _invariant("inactive_panel_frozen", "invariant.extra")
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=_complete_invariants() + [extra_invariant],
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant" in failure
        and "invariant_id does not match an invariant id" in failure
        and "invariant.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_row_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    malformed_row = (
        '  {NULL, "inactive_panel_frozen", "YtreeNovaPanel(inactive)", '
        "kAppStateInvariantProtectedFields0, "
        "sizeof(kAppStateInvariantProtectedFields0) / "
        "sizeof(kAppStateInvariantProtectedFields0[0]), "
        "kAppStateInvariantTransitionIds0, "
        "sizeof(kAppStateInvariantTransitionIds0) / "
        "sizeof(kAppStateInvariantTransitionIds0[0]), "
        "kAppStateInvariantDispatchSurfaceIds0, "
        "sizeof(kAppStateInvariantDispatchSurfaceIds0) / "
        "sizeof(kAppStateInvariantDispatchSurfaceIds0[0]), "
        '"stale selection", "guard", "contract guard", '
        "kAppStateInvariantMigrationNotes0, "
        "sizeof(kAppStateInvariantMigrationNotes0) / "
        "sizeof(kAppStateInvariantMigrationNotes0[0])},"
    )
    marker = (
        "\n};\n"
        "static const char *const kAppStateTransitionSequenceStepInvariantIds0_0"
    )
    runtime_path.write_text(
        source.replace(marker, f"\n{malformed_row}{marker}", 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[" in failure
        and "malformed runtime invariant registry row" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_list_entry_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    marker = (
        "static const char *const kAppStateInvariantProtectedFields0[] = {\n"
        '  "field",'
    )
    mutated_marker = marker.replace('  "field",', '  NULL,\n  "field",')
    runtime_path.write_text(
        source.replace(marker, mutated_marker, 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateInvariantProtectedFields0[0]" in failure
        and "malformed string literal entry" in failure
        and "NULL" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_id_is_duplicated(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_invariants = _complete_invariants()
    runtime_invariants[1]["invariant_id"] = runtime_invariants[0]["invariant_id"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[1]" in failure
        and "duplicate runtime invariant id" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_transition_ids_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_invariants = _complete_invariants()
    runtime_invariants[0]["transition_ids"] = []
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_invariants=runtime_invariants,
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[0]" in failure
        and "transition_ids must be non-empty" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_invariant_transition_is_not_runtime_registered(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [
        record for record in transitions if record["id"] != "transition.keybinding"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime_invariant[0]" in failure
        and "transition_ids does not match runtime transition registry" in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_is_missing_enum_action(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = [_action("ACTION_NONE"), _action("ACTION_USER_CMD")]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action coverage missing YtreeNovaAction enum member" in failure
        and "ACTION_MOVE_UP" in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_has_extra_unknown_action(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions() + [_action("ACTION_NOT_IN_ENUM")]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "unknown YtreeNovaAction enum member" in failure and "ACTION_NOT_IN_ENUM" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_registry_is_missing_matrix_id(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=transitions[:-1],
    )

    failures = _validate(paths)

    missing_id = transitions[-1]["id"]
    assert any(
        "runtime transition registry missing transition id" in failure
        and missing_id in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_lookup_references_missing_runtime_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [
        record for record in transitions if record["id"] != "transition.keybinding"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action[0]" in failure
        and "transition_id does not match runtime transition registry" in failure
        and "transition.keybinding" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_registry_has_extra_id(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = transitions + [
        _transition("keybinding", "transition.runtime.extra")
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition" in failure
        and "id does not match a transition matrix id" in failure
        and "transition.runtime.extra" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_registry_mismatches_matrix(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [dict(record) for record in transitions]
    runtime_transitions[0]["category"] = "render_reflow"
    runtime_transitions[1]["owner"] = "different owner"
    runtime_transitions[2]["declared_write_set"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime category does not match transition" in failure
        for failure in failures
    )
    assert any(
        "runtime owner does not match transition" in failure for failure in failures
    )
    assert any(
        "runtime declared_write_set does not match transition" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_registry_uses_unknown_write_field(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_transitions = [dict(record) for record in transitions]
    runtime_transitions[0]["declared_write_set"] = ["field.not_registered"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_transitions=runtime_transitions,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition[0]" in failure
        and "declared_write_set references unregistered owner field" in failure
        and "field.not_registered" in failure
        for failure in failures
    )


def test_guard_fails_when_action_references_unknown_transition(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["transition_id"] = "transition.missing"
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "transition_id does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_event_references_unknown_transition(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["transition_id"] = "transition.missing"
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "transition_id does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_action_category_does_not_match_transition(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["category"] = "render_reflow"
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "category does not match transition" in failure and "render_reflow" in failure
        for failure in failures
    )


def test_guard_fails_when_event_category_does_not_match_transition(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["category"] = "render_reflow"
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "category does not match transition" in failure
        and "render_reflow" in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_owner_does_not_match_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["owner"] = "different owner"
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "owner does not match transition" in failure
        and "different owner" in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_owner_does_not_match_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["owner"] = "different owner"
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "owner does not match transition" in failure
        and "different owner" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_owner_does_not_match_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    runtime_action_coverages = _complete_actions()
    actions[0]["owner"] = "different owner"
    runtime_action_coverages[0]["owner"] = "different owner"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "owner does not match transition" in failure
        and "different owner" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_owner_does_not_match_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    runtime_events = _complete_events()
    events[0]["owner"] = "different owner"
    runtime_events[0]["owner"] = "different owner"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        events=events,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "owner does not match transition" in failure
        and "different owner" in failure
        for failure in failures
    )


def test_guard_catches_enum_drift_from_temporary_header(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        enum_actions=FIXTURE_ACTIONS + ["ACTION_NEW_DRIFT"],
    )

    failures = _validate(paths)

    assert any(
        "action coverage missing YtreeNovaAction enum member" in failure
        and "ACTION_NEW_DRIFT" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_lookup_is_missing_enum_action(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_actions = [_action("ACTION_NONE"), _action("ACTION_USER_CMD")]
    paths = _write_fixture(
        tmp_path, transitions=transitions, runtime_actions=runtime_actions
    )

    failures = _validate(paths)

    assert any(
        "runtime action lookup missing YtreeNovaAction enum member" in failure
        and "ACTION_MOVE_UP" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_lookup_has_unknown_action(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_actions = _complete_actions() + [_action("ACTION_NOT_IN_ENUM")]
    paths = _write_fixture(
        tmp_path, transitions=transitions, runtime_actions=runtime_actions
    )

    failures = _validate(paths)

    assert any(
        "unknown YtreeNovaAction enum member" in failure and "ACTION_NOT_IN_ENUM" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_lookup_uses_unknown_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_actions = _complete_actions()
    runtime_actions[0] = _action("ACTION_NONE", transition_id="transition.missing")
    paths = _write_fixture(
        tmp_path, transitions=transitions, runtime_actions=runtime_actions
    )

    failures = _validate(paths)

    assert any(
        "runtime_action[0]" in failure
        and "transition_id does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_lookup_mismatches_action_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_actions = _complete_actions()
    runtime_actions[0] = _action("ACTION_NONE", category="render_reflow")
    paths = _write_fixture(
        tmp_path, transitions=transitions, runtime_actions=runtime_actions
    )

    failures = _validate(paths)

    assert any(
        "runtime category does not match action coverage" in failure
        and "ACTION_NONE" in failure
        and "render_reflow" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_is_missing_enum_action(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = [_action("ACTION_NONE"), _action("ACTION_USER_CMD")]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime action coverage missing YtreeNovaAction enum member" in failure
        and "ACTION_MOVE_UP" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_has_malformed_row(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace(
            '{ACTION_NONE, "ACTION_NONE",',
            '{ACTION_NONE /* malformed */, "ACTION_NONE",',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "malformed runtime action coverage row" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_write_set_is_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace('"panel.tree_selection_key",', "panel.tree_selection_key,", 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateActionCoverageWriteSet0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_migration_notes_are_malformed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(tmp_path, transitions=transitions)
    runtime_path = paths[-1]
    source = runtime_path.read_text(encoding="utf-8")
    runtime_path.write_text(
        source.replace('"fixture action coverage",', "fixture action coverage,", 1),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateActionCoverageMigrationNotes0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_uses_unknown_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0] = _action(
        "ACTION_NONE", transition_id="transition.missing"
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "transition_id does not match a transition id" in failure
        and "transition.missing" in failure
        for failure in failures
    )


def test_guard_fails_when_action_transition_table_drifts_from_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_actions = _complete_actions()
    runtime_actions[0] = _action("ACTION_NONE", category="render_reflow")
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_actions=runtime_actions,
        runtime_action_coverages=_complete_actions(),
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "category does not match runtime action transition table" in failure
        and "ACTION_NONE" in failure
        for failure in failures
    )


def _event_runtime_records_and_failures(runtime_path: Path = guard.DEFAULT_ACTION_RUNTIME):
    return guard._parse_runtime_event_coverage_registry(runtime_path)


def _event_runtime_validation_failures(runtime_path: Path) -> list[str]:
    transitions_doc, transition_failures = guard._load_json(guard.DEFAULT_TRANSITIONS)
    event_doc, event_failures = guard._load_json(guard.DEFAULT_EVENT_COVERAGE)
    runtime_transitions, runtime_transition_failures = guard._parse_runtime_transition_registry(
        runtime_path
    )
    runtime_events, runtime_event_failures = guard._parse_runtime_event_coverage_registry(
        runtime_path
    )
    transition_ids = {
        record["id"]: record for record in transitions_doc.get("transitions", [])
    }
    return (
        transition_failures
        + event_failures
        + runtime_transition_failures
        + runtime_event_failures
        + guard._validate_runtime_event_coverage_registry(
            runtime_records=runtime_events,
            runtime_path=runtime_path,
            event_coverage_doc=event_doc,
            transition_ids=transition_ids,
            runtime_transition_ids={record["id"] for record in runtime_transitions},
        )
    )


def _mutated_event_runtime(tmp_path: Path, old: str, new: str) -> Path:
    runtime_path = tmp_path / "appstate_actions.c"
    source = guard.DEFAULT_ACTION_RUNTIME.read_text(encoding="utf-8")
    assert old in source
    runtime_path.write_text(source.replace(old, new, 1), encoding="utf-8")
    return runtime_path


def test_runtime_event_coverage_registry_matches_docs() -> None:
    records, parse_failures = _event_runtime_records_and_failures()

    assert parse_failures == []
    assert {record["event_id"] for record in records} == guard._collect_string_ids(
        guard._load_json(guard.DEFAULT_EVENT_COVERAGE)[0],
        collection_key="events",
        id_field="event_id",
    )
    assert _event_runtime_validation_failures(guard.DEFAULT_ACTION_RUNTIME) == []


def test_runtime_event_coverage_detects_doc_drift(tmp_path: Path) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        '"event.render-reflow",\n   "render_reflow"',
        '"event.render-reflow-runtime",\n   "render_reflow"',
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("runtime event coverage missing from docs" in failure for failure in failures)
    assert any("runtime event coverage missing event id(s)" in failure for failure in failures)


def test_runtime_event_coverage_detects_duplicate_and_missing_classes(
    tmp_path: Path,
) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        '"event.render-reflow",\n   "render_reflow"',
        '"event.render-reflow",\n   "modal_completion"',
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("duplicate event_class: modal_completion" in failure for failure in failures)
    assert any("missing event_class(es): render_reflow" in failure for failure in failures)


def test_runtime_event_coverage_detects_invalid_transition_linkage(
    tmp_path: Path,
) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        '"event.render-reflow",\n   "render_reflow",\n   "transition.render-reflow.project-state"',
        '"event.render-reflow",\n   "render_reflow",\n   "transition.render-reflow.unknown"',
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("transition_id does not match a transition id" in failure for failure in failures)


def test_runtime_event_coverage_detects_write_set_drift(tmp_path: Path) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        "   kAppStateTransitionWriteSet9,\n   sizeof(kAppStateTransitionWriteSet9) / sizeof(kAppStateTransitionWriteSet9[0]),\n   \"covered_by_transition_record\",\n   kAppStateEventCoverageTriggerPaths8",
        "   kAppStateTransitionWriteSet0,\n   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),\n   \"covered_by_transition_record\",\n   kAppStateEventCoverageTriggerPaths8",
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("declared_write_set does not match transition" in failure for failure in failures)


def test_runtime_event_coverage_detects_malformed_lists(tmp_path: Path) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        '"Signal flag set outside curses work",',
        '"",',
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("trigger_paths" in failure for failure in failures)


def test_runtime_event_coverage_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateEventCoverageAt(AppStateEventCoverageCount()) != NULL" in source
    assert 'AppStateEventCoverageLookup("event.__ytnova_unknown__") != NULL' in source
    assert "!AppStateEventCoverageReady()" in source


def test_runtime_coverage_startup_validates_owner_alignment() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    event_start = source.index("static int AppStateEventCoverageReady(void)")
    action_start = source.index("static int AppStateActionCoverageReady(void)")
    diff_harness_start = source.index(
        "static int AppStateDiffHarnessRegistryReady(void)"
    )

    event_body = source[event_start:action_start]
    action_body = source[action_start:diff_harness_start]

    assert "strcmp(coverage->owner, transition->owner) != 0" in action_body
    assert "strcmp(coverage->owner, transition->owner) != 0" in event_body


def test_runtime_event_coverage_startup_requires_documented_event_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    event_doc, event_failures = guard._load_json(guard.DEFAULT_EVENT_COVERAGE)
    required_ids = guard._collect_string_ids(
        event_doc,
        collection_key="events",
        id_field="event_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredEventIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert event_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredEventIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert "AppStateRequiredEventIdCovered(kAppStateRequiredEventIds[index])" in source


def test_runtime_owner_field_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateOwnerFieldAt(AppStateOwnerFieldCount()) != NULL" in source
    assert "AppStateOwnerFieldLookup(NULL) != NULL" in source
    assert 'AppStateOwnerFieldLookup("") != NULL' in source
    assert 'AppStateOwnerFieldLookup("field.__ytnova_unknown__") != NULL' in source
    assert "AppStateOwnerFieldCount() != required_owner_field_id_count" in source
    assert "metadata == NULL || !NonEmptyString(metadata->field)" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->field, metadata->field) == 0" in source
    assert "AppStateOwnerFieldLookup(kAppStateRequiredOwnerFieldIds[index])" in source
    assert "!AppStateOwnerFieldsReady()" in source


def test_runtime_owner_field_startup_validates_invariant_checks_against_registry() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert re.search(
        r"static int AppStateInvariantProtectsField\(.*?"
        r"metadata = AppStateInvariantLookup\(invariant_id\);\s*"
        r"if \(metadata == NULL\)\s*return 0;\s*"
        r"if \(!NonEmptyStringList\(metadata->protected_fields,\s*"
        r"metadata->protected_field_count\)\)\s*return 0;\s*"
        r"return StringListContains\(metadata->protected_fields,",
        source,
        re.S,
    )
    assert re.search(
        r"for \(invariant_index = 0;\s*"
        r"invariant_index < metadata->invariant_check_count;\s*"
        r"invariant_index\+\+\) \{\s*"
        r"if \(AppStateInvariantLookup\("
        r"metadata->invariant_checks\[invariant_index\]\)\s*==\s*NULL\)\s*"
        r"return 0;",
        source,
        re.S,
    )
    assert "field_protected = 1" in source
    assert re.search(r"if \(!field_protected\)\s*return 0;", source, re.S)


def test_runtime_owner_field_startup_requires_documented_field_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    owner_doc, owner_failures = guard._load_json(guard.DEFAULT_OWNER_FIELDS)
    required_ids = guard._collect_string_ids(
        owner_doc,
        collection_key="owner_fields",
        id_field="field",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredOwnerFieldIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert owner_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredOwnerFieldIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert len(table_ids) == len(required_ids)
    assert re.search(
        r"AppStateOwnerFieldLookup\(\s*"
        r"kAppStateRequiredOwnerFieldIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_generation_domain_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert (
        "AppStateGenerationDomainAt(AppStateGenerationDomainCount()) != NULL"
        in source
    )
    assert "AppStateGenerationDomainLookup(NULL) != NULL" in source
    assert 'AppStateGenerationDomainLookup("") != NULL' in source
    assert (
        'AppStateGenerationDomainLookup("generation.__ytnova_unknown__") != NULL'
        in source
    )
    assert (
        "AppStateGenerationDomainCount() != "
        "required_generation_domain_id_count"
        in source
    )
    assert "metadata == NULL || !NonEmptyString(metadata->domain_id)" in source
    assert "!NonEmptyString(metadata->category)" in source
    assert "!NonEmptyString(metadata->generation_owner_field)" in source
    assert "!NonEmptyString(metadata->stale_snapshot_policy)" in source
    assert "!NonEmptyString(metadata->fail_closed_fallback)" in source
    assert "!NonEmptyString(metadata->restore_boundary)" in source
    assert "!NonEmptyString(metadata->enforcement_status)" in source
    assert "NonEmptyStringList(metadata->identity_fields" in source
    assert "NonEmptyStringList(metadata->advances_on_transition_ids" in source
    assert "NonEmptyStringList(metadata->migration_notes" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->domain_id, metadata->domain_id) == 0" in source
    assert "AppStateOwnerFieldLookup(metadata->generation_owner_field) == NULL" in source
    assert "AppStateOwnerFieldLookup(metadata->identity_fields[field_index])" in source
    assert (
        "AppStateTransitionLookup(\n"
        "              metadata->advances_on_transition_ids[transition_index])"
        in source
    )
    assert (
        "kAppStateRequiredGenerationDomainIds[index]) == NULL"
        in source
    )
    assert "!AppStateGenerationDomainsReady()" in source


def test_runtime_generation_write_startup_validates_runtime_metadata() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    helper_start = source.index("static int AppStateGenerationWriteCovered(")
    transition_start = source.index("static int AppStateTransitionRegistryReady(void)")
    generation_start = source.index("static int AppStateGenerationDomainsReady(void)")
    helper_body = source[helper_start:transition_start]
    transition_body = source[transition_start:generation_start]

    assert "AppStateGenerationDomainCount()" in helper_body
    assert "AppStateGenerationDomainAt(domain_index)" in helper_body
    assert "domain->generation_owner_field" in helper_body
    assert "domain->advances_on_transition_ids" in helper_body
    assert "strcmp(domain_transition_id, transition_id) == 0" in helper_body
    assert "!AppStateGenerationWriteCovered(field, metadata->id)" in transition_body


def test_runtime_generation_domain_startup_requires_documented_domain_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    generation_doc, generation_failures = guard._load_json(
        guard.DEFAULT_GENERATION_DOMAINS
    )
    required_ids = guard._collect_string_ids(
        generation_doc,
        collection_key="generation_domains",
        id_field="domain_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredGenerationDomainIds\[\]\s*=\s*"
        r"\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert generation_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredGenerationDomainIds",
    )
    assert table_failures == []
    assert table_ids == [
        domain["domain_id"] for domain in generation_doc["generation_domains"]
    ]
    assert set(table_ids) == required_ids
    assert len(table_ids) == len(required_ids)
    assert re.search(
        r"AppStateGenerationDomainLookup\(\s*"
        r"kAppStateRequiredGenerationDomainIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_dispatch_surface_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateDispatchSurfaceAt(AppStateDispatchSurfaceCount()) != NULL" in source
    assert 'AppStateDispatchSurfaceLookup("surface.__ytnova_unknown__") != NULL' in source
    assert "AppStateDispatchSurfaceCount() != required_surface_id_count" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->surface_id, metadata->surface_id) == 0" in source
    assert "!AppStateDispatchSurfacesReady()" in source


def test_runtime_dispatch_surface_startup_validates_allowed_write_owner_fields() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert re.search(
        r"if \(metadata->allowed_direct_writes == NULL\)\s*return 0;\s*"
        r"for \(write_index = 0; write_index < "
        r"metadata->allowed_direct_write_count;\s*write_index\+\+\) \{\s*"
        r"const char \*field = metadata->allowed_direct_writes\[write_index\];\s*"
        r"if \(!NonEmptyString\(field\)\)\s*return 0;\s*"
        r"if \(AppStateOwnerFieldLookup\(field\) == NULL\)\s*return 0;",
        source,
        re.S,
    )


def test_runtime_dispatch_surface_startup_validates_allowed_write_contract() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateDispatchSurfaceWritesReady(")
    dispatch_start = source.index("static int AppStateDispatchSurfacesReady(void)")
    helper_body = source[helper_start:dispatch_start]

    assert (
        "const AppStateTransitionMetadata *transition =\n"
        "      AppStateTransitionLookup(metadata->transition_id);"
    ) in helper_body
    assert "transition == NULL" in helper_body
    assert re.search(
        r"StringListContains\(transition->declared_write_set,\s*"
        r"transition->declared_write_set_count, field\)",
        helper_body,
        re.S,
    )


def test_runtime_dispatch_surface_startup_requires_invariant_coverage() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateDispatchSurfaceWritesReady(")
    dispatch_start = source.index("static int AppStateDispatchSurfacesReady(void)")
    helper_body = source[helper_start:dispatch_start]

    assert "AppStateDispatchSurfaceWriteHasInvariantCoverage(" in helper_body
    assert "AppStateInvariantAt(invariant_index)" in source
    assert re.search(
        r"StringListContains\(invariant->dispatch_surface_ids,\s*"
        r"invariant->dispatch_surface_id_count, surface_id\)",
        source,
        re.S,
    )
    assert re.search(
        r"StringListContains\(invariant->protected_fields,\s*"
        r"invariant->protected_field_count, field\)",
        source,
        re.S,
    )


def test_runtime_dispatch_surface_startup_requires_documented_surface_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    dispatch_doc, dispatch_failures = guard._load_json(guard.DEFAULT_DISPATCH_SURFACES)
    required_ids = guard._collect_string_ids(
        dispatch_doc,
        collection_key="dispatch_surfaces",
        id_field="surface_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredDispatchSurfaceIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert dispatch_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredDispatchSurfaceIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert re.search(
        r"AppStateDispatchSurfaceLookup\(\s*"
        r"kAppStateRequiredDispatchSurfaceIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_invariant_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateInvariantAt(AppStateInvariantCount()) != NULL" in source
    assert 'AppStateInvariantLookup("invariant.__ytnova_unknown__") != NULL' in source
    assert "AppStateInvariantCount() != required_invariant_id_count" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->invariant_id, metadata->invariant_id) == 0" in source
    assert "!AppStateInvariantRegistryReady()" in source


def test_runtime_invariant_startup_validates_protected_fields_against_owner_registry() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "NonEmptyStringList(metadata->protected_fields" in source
    assert re.search(
        r"for \(protected_field_index = 0;\s*"
        r"protected_field_index < metadata->protected_field_count;\s*"
        r"protected_field_index\+\+\) \{\s*"
        r"const char \*field = "
        r"metadata->protected_fields\[protected_field_index\];\s*"
        r"if \(AppStateOwnerFieldLookup\(field\) == NULL\)\s*return 0;",
        source,
        re.S,
    )


def test_runtime_invariant_startup_requires_diff_harness_owner_field_coverage() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateDiffHarnessOwnerFieldCovered" in source
    assert re.search(
        r"if \(AppStateOwnerFieldLookup\(field\) == NULL\)\s*return 0;\s*"
        r"if \(!AppStateDiffHarnessOwnerFieldCovered\(field\)\)\s*return 0;",
        source,
        re.S,
    )


def test_runtime_invariant_startup_requires_documented_invariant_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    invariant_doc, invariant_failures = guard._load_json(guard.DEFAULT_INVARIANTS)
    required_ids = guard._collect_string_ids(
        invariant_doc,
        collection_key="invariants",
        id_field="invariant_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredInvariantIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert invariant_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredInvariantIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert re.search(
        r"AppStateInvariantLookup\(\s*"
        r"kAppStateRequiredInvariantIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_shim_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert (
        "AppStateCompatibilityShimAt(AppStateCompatibilityShimCount()) != NULL"
        in source
    )
    assert (
        'AppStateCompatibilityShimLookup("shim.__ytnova_unknown__") != NULL'
        in source
    )
    assert "AppStateCompatibilityShimCount() != required_shim_id_count" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->id, metadata->id) == 0" in source
    assert "!AppStateCompatibilityShimsReady()" in source


def test_runtime_shim_startup_validates_invariant_checks_against_registry() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert re.search(
        r"for \(invariant_index = 0;\s*"
        r"invariant_index < metadata->invariant_check_count;\s*"
        r"invariant_index\+\+\) \{\s*"
        r"if \(!NonEmptyString\(metadata->invariant_checks\[invariant_index\]\)\)\s*"
        r"return 0;\s*"
        r"if \(AppStateInvariantLookup\("
        r"metadata->invariant_checks\[invariant_index\]\)\s*==\s*NULL\)\s*"
        r"return 0;",
        source,
        re.S,
    )


def test_runtime_shim_startup_requires_invariant_to_cover_target_transition() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index(
        "static int AppStateCompatibilityShimInvariantCoversTransition("
    )
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    helper_body = source[helper_start:ready_start]
    ready_body = source[ready_start:action_start]

    assert "metadata->target_transition" in helper_body
    assert (
        "AppStateInvariantLookup(metadata->invariant_checks[invariant_index])"
        in helper_body
    )
    assert "invariant->transition_ids" in helper_body
    assert "invariant->transition_id_count" in helper_body
    assert "!StringListContains(invariant->transition_ids" in helper_body
    assert "!AppStateCompatibilityShimInvariantCoversTransition(metadata)" in ready_body


def test_runtime_shim_startup_requires_owner_field_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    ready_body = source[ready_start:action_start]

    assert "metadata->owner_field_refs" in ready_body
    assert "metadata->owner_field_ref_count" in ready_body
    assert "AppStateOwnerFieldLookup(metadata->owner_field_refs[ref_index])" in ready_body
    assert re.search(
        r"StringListContains\(metadata->owner_field_refs,\s*"
        r"ref_index,\s*metadata->owner_field_refs\[ref_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_shim_startup_requires_generation_domain_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    ready_body = source[ready_start:action_start]

    assert "metadata->generation_domain_refs" in ready_body
    assert "metadata->generation_domain_ref_count" in ready_body
    assert "AppStateGenerationDomainLookup(" in ready_body
    assert re.search(
        r"StringListContains\(metadata->generation_domain_refs,\s*"
        r"generation_index,\s*"
        r"metadata->generation_domain_refs\[generation_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_shim_startup_requires_diff_harness_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    ready_body = source[ready_start:action_start]

    assert "metadata->diff_harness_refs" in ready_body
    assert "metadata->diff_harness_ref_count" in ready_body
    assert "AppStateDiffHarnessLookup(metadata->diff_harness_refs[diff_index])" in ready_body
    assert re.search(
        r"StringListContains\(metadata->diff_harness_refs,\s*"
        r"diff_index,\s*metadata->diff_harness_refs\[diff_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_shim_startup_requires_diff_harness_union_coverage() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index(
        "static int AppStateCompatibilityShimDiffHarnessCoversTransition("
    )
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    helper_body = source[helper_start:ready_start]
    ready_body = source[ready_start:action_start]

    assert "AppStateCompatibilityShimDiffHarnessCoversOwnerField(" in helper_body
    assert "AppStateCompatibilityShimDiffHarnessCoversInvariant(" in helper_body
    assert (
        "AppStateCompatibilityShimDiffHarnessCoversGenerationDomain("
        in helper_body
    )
    assert "harness->transition_ids" in helper_body
    assert "harness->owner_field_refs" in helper_body
    assert "harness->invariant_ids" in helper_body
    assert "harness->generation_domain_ids" in helper_body
    assert "AppStateCompatibilityShimDiffHarnessCoversTransition(metadata)" in ready_body
    assert "AppStateCompatibilityShimDiffHarnessCoversOwnerField(" in ready_body
    assert "AppStateCompatibilityShimDiffHarnessCoversInvariant(" in ready_body
    assert (
        "AppStateCompatibilityShimDiffHarnessCoversGenerationDomain("
        in ready_body
    )


def test_runtime_shim_startup_requires_generation_owner_refs_to_match_domains() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    owner_helper_start = source.index(
        "static int AppStateGenerationOwnerFieldRegistered("
    )
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    helper_body = source[owner_helper_start:ready_start]
    ready_body = source[ready_start:action_start]

    assert "AppStateGenerationDomainAt(domain_index)" in helper_body
    assert "domain->generation_owner_field" in helper_body
    assert "AppStateCompatibilityShimGenerationDomainCoversOwnerField(" in helper_body
    assert "AppStateGenerationDomainLookup(" in helper_body
    assert "metadata->generation_domain_refs[ref_index]" in helper_body
    assert "AppStateGenerationOwnerFieldRegistered(" in ready_body
    assert "AppStateCompatibilityShimGenerationDomainCoversOwnerField(" in ready_body


def test_runtime_shim_startup_requires_write_refs_to_match_target_write_set() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateCompatibilityShimWriteCapable(")
    invariant_start = source.index(
        "static int AppStateCompatibilityShimInvariantCoversTransition("
    )
    ready_start = source.index("static int AppStateCompatibilityShimsReady(void)")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    helper_body = source[helper_start:invariant_start]
    ready_body = source[ready_start:action_start]

    assert "metadata->write_capability" in helper_body
    assert 'strcmp(metadata->write_capability, "write_capable") == 0' in helper_body
    assert "strstr" not in helper_body
    assert "write_permission" not in helper_body
    assert "AppStateCompatibilityShimWriteCapabilityKnown(metadata)" in ready_body
    assert '"read_only_projection"' in helper_body
    assert '"no_write"' in helper_body
    assert "const AppStateTransitionMetadata *transition" in ready_body
    assert re.search(
        r"AppStateCompatibilityShimWriteCapable\(metadata\).*?"
        r"!StringListContains\(transition->declared_write_set,\s*"
        r"transition->declared_write_set_count,\s*"
        r"metadata->owner_field_refs\[ref_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_shim_startup_requires_documented_shim_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    shim_doc, shim_failures = guard._load_json(guard.DEFAULT_SHIMS)
    required_ids = guard._collect_string_ids(
        shim_doc,
        collection_key="shims",
        id_field="id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredShimIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert shim_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredShimIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert re.search(
        r"AppStateCompatibilityShimLookup\(\s*"
        r"kAppStateRequiredShimIds\[index\]\s*\)",
        source,
        re.S,
    )


def test_runtime_diff_harness_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateDiffHarnessInvariantCoversTransition" in source
    assert "AppStateDiffHarnessAt(AppStateDiffHarnessCount()) != NULL" in source
    assert 'AppStateDiffHarnessLookup("harness.__ytnova_unknown__") != NULL' in source
    assert "AppStateDiffHarnessLookup(NULL) != NULL" in source
    assert 'AppStateDiffHarnessLookup("") != NULL' in source
    assert "AppStateDiffHarnessCount() != required_diff_harness_id_count" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->harness_id, metadata->harness_id) == 0" in source
    assert "AppStateTransitionLookup(metadata->transition_ids[ref_index])" in source
    assert "AppStateOwnerFieldLookup(metadata->owner_field_refs[ref_index])" in source
    assert "AppStateInvariantLookup(metadata->invariant_ids[ref_index])" in source
    assert re.search(
        r"!AppStateDiffHarnessInvariantCoversTransition\(\s*"
        r"metadata,\s*metadata->transition_ids\[ref_index\]\)",
        source,
        re.S,
    )
    assert (
        "AppStateGenerationDomainLookup(\n"
        "              metadata->generation_domain_ids[ref_index])"
        in source
    )
    assert "!AppStateDiffHarnessRegistryReady()" in source


def test_runtime_diff_harness_startup_requires_owner_field_invariant() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index(
        "static int AppStateDiffHarnessInvariantProtectsOwnerField("
    )
    ready_start = source.index("static int AppStateDiffHarnessRegistryReady(void)")
    signal_start = source.index("static void SigIntHandler(int sig)")
    helper_body = source[helper_start:ready_start]
    ready_body = source[ready_start:signal_start]

    assert re.search(
        r"if \(metadata == NULL \|\| !NonEmptyString\(owner_field\) \|\|\s*"
        r"!NonEmptyStringList\(metadata->invariant_ids,\s*"
        r"metadata->invariant_id_count\)\)\s*"
        r"return 0;\s*"
        r"for \(ref_index = 0; "
        r"ref_index < metadata->invariant_id_count; ref_index\+\+\) \{\s*"
        r"if \(AppStateInvariantProtectsField\("
        r"metadata->invariant_ids\[ref_index\],\s*owner_field\)\)\s*"
        r"return 1;",
        helper_body,
        re.S,
    )
    assert re.search(
        r"!NonEmptyStringList\(metadata->owner_field_refs,\s*"
        r"metadata->owner_field_ref_count\).*?"
        r"!NonEmptyStringList\(metadata->invariant_ids,\s*"
        r"metadata->invariant_id_count\).*?"
        r"for \(ref_index = 0; ref_index < metadata->owner_field_ref_count;\s*"
        r"ref_index\+\+\) \{\s*"
        r"if \(AppStateOwnerFieldLookup\("
        r"metadata->owner_field_refs\[ref_index\]\) ==\s*NULL\)\s*"
        r"return 0;\s*\}\s*"
        r"for \(ref_index = 0; ref_index < metadata->invariant_id_count;\s*"
        r"ref_index\+\+\) \{\s*"
        r"if \(AppStateInvariantLookup\("
        r"metadata->invariant_ids\[ref_index\]\) == NULL\)\s*"
        r"return 0;\s*\}\s*"
        r"for \(ref_index = 0; ref_index < metadata->owner_field_ref_count;\s*"
        r"ref_index\+\+\) \{\s*"
        r"if \(!AppStateDiffHarnessInvariantProtectsOwnerField\(\s*"
        r"metadata,\s*metadata->owner_field_refs\[ref_index\]\)\)\s*"
        r"return 0;",
        ready_body,
        re.S,
    )


def test_runtime_diff_harness_write_coverage_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateDiffHarnessWriteCovered" in source
    assert re.search(
        r"for \(write_index = 0; write_index < "
        r"transition->declared_write_set_count;\s*write_index\+\+\) \{\s*"
        r"const char \*field = transition->declared_write_set\[write_index\];\s*"
        r"if \(!AppStateDiffHarnessWriteCovered\(field, transition->id\)\)\s*"
        r"return 0;",
        source,
        re.S,
    )


def test_runtime_generation_harness_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index(
        "static int AppStateGenerationAdvanceHasDiffHarnessCoverage("
    )
    invariant_start = source.index(
        "static int AppStateDiffHarnessInvariantCoversTransition("
    )
    ready_start = source.index("static int AppStateDiffHarnessRegistryReady(void)")
    signal_start = source.index("static void SigIntHandler(int sig)")
    helper_body = source[helper_start:invariant_start]
    ready_body = source[ready_start:signal_start]

    assert "harness->generation_domain_ids" in helper_body
    assert "harness->transition_ids" in helper_body
    assert "domain_seen && transition_seen" in helper_body
    assert "AppStateGenerationDomainCount()" in ready_body
    assert "domain->advances_on_transition_ids" in ready_body
    assert re.search(
        r"!AppStateGenerationAdvanceHasDiffHarnessCoverage\(\s*"
        r"domain->domain_id,\s*"
        r"domain->advances_on_transition_ids\[transition_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_diff_harness_startup_requires_documented_harness_ids() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    diff_doc, diff_failures = guard._load_json(guard.DEFAULT_DIFF_HARNESS)
    required_ids = guard._collect_string_ids(
        diff_doc,
        collection_key="diff_harness_checks",
        id_field="harness_id",
    )
    required_table = re.search(
        r"static\s+const\s+char\s+\*const\s+"
        r"kAppStateRequiredDiffHarnessIds\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )

    assert diff_failures == []
    assert required_table is not None
    table_ids, table_failures = guard._parse_string_initializer_array(
        required_table.group("body"),
        "kAppStateRequiredDiffHarnessIds",
    )
    assert table_failures == []
    assert set(table_ids) == required_ids
    assert re.search(
        r"AppStateDiffHarnessLookup\(\s*"
        r"kAppStateRequiredDiffHarnessIds\[index\]\s*\)",
        source,
        re.S,
    )
