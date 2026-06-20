from __future__ import annotations

import copy
import importlib.util
import json
import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[1]
GUARD_PATH = REPO_ROOT / "scripts" / "check_appstate_contract.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_appstate_contract", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


REQUIRED_CATEGORIES = sorted(guard.REQUIRED_TRANSITION_CATEGORIES)
REQUIRED_EVENT_CLASSES = sorted(guard.REQUIRED_EVENT_CLASSES)
REQUIRED_DISPATCH_SURFACE_CATEGORIES = [
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
    "command_completion_dispatch",
    "volume_menu_selection",
    "rebuild_rebind_callback",
]
REQUIRED_INVARIANT_CATEGORIES = sorted(guard.REQUIRED_INVARIANT_CATEGORIES)
REQUIRED_GENERATION_DOMAIN_CATEGORIES = sorted(
    guard.REQUIRED_GENERATION_DOMAIN_CATEGORIES
)
REQUIRED_DIFF_HARNESS_CATEGORIES = sorted(guard.REQUIRED_DIFF_HARNESS_CATEGORIES)
REQUIRED_SEQUENCE_FLOWS = sorted(guard.REQUIRED_SEQUENCE_FLOWS)
FIXTURE_ACTIONS = [
    "ACTION_NONE",
    "ACTION_MOVE_UP",
    "ACTION_VOL_MENU",
    "ACTION_USER_CMD",
]
REQUIRED_LIST_FIELD_CASES = [
    ("action", "declared_write_set", "action[0]"),
    ("action", "owner_field_refs", "action[0]"),
    ("action", "generation_domain_refs", "action[0]"),
    ("action", "diff_harness_refs", "action[0]"),
    ("action", "invariant_refs", "action[0]"),
    ("action", "migration_notes", "action[0]"),
    ("event", "declared_write_set", "event[0]"),
    ("event", "owner_field_refs", "event[0]"),
    ("event", "generation_domain_refs", "event[0]"),
    ("event", "diff_harness_refs", "event[0]"),
    ("event", "invariant_refs", "event[0]"),
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
    ("dispatch_surface", "transition_sequence_refs", "dispatch_surface[0]"),
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
    declared_write_set = ["field"]
    generation_effect = "generation"
    if category == "render_reflow":
        declared_write_set = ["panel.tree_selection_key"]
        generation_effect = "projection_only"
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
        "declared_write_set": declared_write_set,
        "generation_effect": generation_effect,
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
    if action == "ACTION_VOL_MENU" and transition_id == "transition.keybinding":
        transition_id = "transition.menu_action"
        category = "menu_action"
    return {
        "action": action,
        "transition_id": transition_id,
        "category": category,
        "owner": "owner",
        "declared_write_set": ["panel.tree_selection_key"],
        "owner_field_refs": ["panel.tree_selection_key"],
        "transition_sequence_refs": _sequence_refs_for_transition(transition_id),
        "dispatch_surface_refs": _dispatch_surface_refs_for_transition(
            transition_id, action=action
        ),
        "generation_domain_refs": _generation_domain_refs_for_transition(
            transition_id
        ),
        "diff_harness_refs": _diff_harness_refs_for_transition(transition_id),
        "invariant_refs": _invariant_refs_for_transition(transition_id),
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
    resolved_transition_id = transition_id or f"transition.{resolved_category}"
    declared_write_set = ["field"]
    if resolved_category == "render_reflow":
        declared_write_set = ["panel.tree_selection_key"]
    return {
        "event_id": f"event.{event_class}",
        "event_class": event_class,
        "transition_id": resolved_transition_id,
        "category": resolved_category,
        "source": "fixture source",
        "owner": "owner",
        "declared_write_set": declared_write_set,
        "owner_field_refs": list(declared_write_set),
        "transition_sequence_refs": _sequence_refs_for_transition(
            resolved_transition_id
        ),
        "dispatch_surface_refs": _dispatch_surface_refs_for_transition(
            resolved_transition_id,
            event_id=f"event.{event_class}",
        ),
        "generation_domain_refs": _generation_domain_refs_for_transition(
            resolved_transition_id
        ),
        "diff_harness_refs": _diff_harness_refs_for_transition(
            resolved_transition_id
        ),
        "invariant_refs": _invariant_refs_for_transition(
            resolved_transition_id
        ),
        "boundary_status": "test",
        "trigger_paths": ["fixture trigger"],
        "migration_notes": ["fixture event coverage"],
    }


def _sequence_refs_for_transition(transition_id: str) -> list[str]:
    return {
        "transition.command_completion": ["sequence.search_jump"],
        "transition.filesystem_mutation_result": [
            "sequence.filesystem_mutation_result"
        ],
        "transition.keybinding": ["sequence.split_toggle_f8"],
        "transition.menu_action": ["sequence.volume_menu_select"],
        "transition.modal_action": ["sequence.esc_modal_dismissal"],
        "transition.rebuild_rebind_callback": ["sequence.refresh_rebuild"],
        "transition.refresh_rebuild": ["sequence.refresh_rebuild"],
        "transition.render_reflow": ["sequence.render_reflow_projection"],
        "transition.terminal_signal_or_resize": ["sequence.terminal_resize_reflow"],
        "transition.volume_operation": ["sequence.volume_cycling_release"],
    }.get(transition_id, ["sequence.split_toggle_f8"])


def _dispatch_surface_refs_for_transition(
    transition_id: str,
    *,
    action: str | None = None,
    event_id: str | None = None,
) -> list[str]:
    if transition_id == "transition.keybinding":
        return [
            "surface.key_decode_input_dispatch",
            "surface.directory_window_action_dispatch",
            "surface.file_window_action_dispatch",
        ]
    if transition_id == "transition.refresh_rebuild":
        if event_id == "event.watcher_live_refresh":
            return ["surface.watcher_live_refresh"]
        return ["surface.refresh_rebuild_rebind"]
    if transition_id == "transition.rebuild_rebind_callback":
        return ["surface.rebuild_rebind_callback"]
    if transition_id == "transition.command_completion":
        return ["surface.command_completion_dispatch"]
    if transition_id == "transition.menu_action" and action == "ACTION_VOL_MENU":
        return ["surface.volume_menu_selection"]
    return {
        "transition.filesystem_mutation_result": ["surface.filesystem_mutation_result"],
        "transition.modal_action": ["surface.menu_modal_completion"],
        "transition.render_reflow": ["surface.render_reflow_projection"],
        "transition.terminal_signal_or_resize": ["surface.resize_signal_handling"],
        "transition.volume_operation": ["surface.volume_operation"],
    }.get(transition_id, ["surface.key_decode_input_dispatch"])


def _invariant_refs_for_transition(transition_id: str) -> list[str]:
    return {
        "transition.command_completion": ["invariant.blocked_transition_determinism"],
        "transition.filesystem_mutation_result": [
            "invariant.blocked_transition_determinism"
        ],
        "transition.keybinding": ["invariant.inactive_panel_frozen"],
        "transition.menu_action": ["invariant.shared_state_panel_local_isolation"],
        "transition.modal_action": [
            "invariant.panel_local_focus_restore",
            "invariant.blocked_transition_determinism",
        ],
        "transition.rebuild_rebind_callback": [
            "invariant.hidden_entry_visible_navigation",
            "invariant.viewport_identity_rebind",
        ],
        "transition.refresh_rebuild": ["invariant.hidden_entry_visible_navigation"],
        "transition.render_reflow": ["invariant.render_projection_read_only"],
        "transition.terminal_signal_or_resize": [
            "invariant.render_projection_read_only"
        ],
        "transition.volume_operation": [
            "invariant.shared_state_panel_local_isolation"
        ],
    }.get(transition_id, ["invariant.inactive_panel_frozen"])


def _generation_domain_refs_for_transition(transition_id: str) -> list[str]:
    return {
        "transition.command_completion": ["domain.modal_command_target"],
        "transition.filesystem_mutation_result": [
            "domain.panel_generation",
            "domain.volume_generation",
            "domain.directory_identity",
            "domain.file_identity",
            "domain.topology_state",
            "domain.file_payload_state",
        ],
        "transition.keybinding": [
            "domain.panel_generation",
            "domain.focus_shape",
            "domain.visibility_filter_state",
        ],
        "transition.menu_action": [
            "domain.panel_generation",
            "domain.focus_shape",
            "domain.volume_lifecycle",
        ],
        "transition.modal_action": [
            "domain.panel_generation",
            "domain.focus_shape",
            "domain.modal_command_target",
        ],
        "transition.rebuild_rebind_callback": [
            "domain.panel_generation",
            "domain.directory_identity",
            "domain.file_identity",
            "domain.focus_shape",
            "domain.visibility_filter_state",
            "domain.file_payload_state",
        ],
        "transition.refresh_rebuild": [
            "domain.panel_generation",
            "domain.volume_generation",
            "domain.directory_identity",
            "domain.file_identity",
            "domain.visibility_filter_state",
            "domain.topology_state",
            "domain.file_payload_state",
            "domain.volume_lifecycle",
        ],
        "transition.render_reflow": ["domain.layout_reflow"],
        "transition.terminal_signal_or_resize": [
            "domain.panel_generation",
            "domain.layout_reflow",
        ],
        "transition.volume_operation": [
            "domain.panel_generation",
            "domain.volume_generation",
            "domain.directory_identity",
            "domain.volume_lifecycle",
        ],
    }.get(transition_id, ["domain.panel_generation"])


def _diff_harness_refs_for_transition(transition_id: str) -> list[str]:
    return {
        "transition.command_completion": [
            "harness.declared_write_set_diff",
            "harness.blocked_transition_no_unrelated_mutation",
        ],
        "transition.filesystem_mutation_result": [
            "harness.transition_before_after_snapshot",
            "harness.generation_mismatch_check",
            "harness.blocked_transition_no_unrelated_mutation",
        ],
        "transition.keybinding": [
            "harness.transition_before_after_snapshot",
            "harness.declared_write_set_diff",
            "harness.blocked_transition_no_unrelated_mutation",
        ],
        "transition.menu_action": ["harness.declared_write_set_diff"],
        "transition.modal_action": [
            "harness.declared_write_set_diff",
            "harness.blocked_transition_no_unrelated_mutation",
        ],
        "transition.rebuild_rebind_callback": [
            "harness.transition_before_after_snapshot",
            "harness.generation_mismatch_check",
        ],
        "transition.refresh_rebuild": [
            "harness.transition_before_after_snapshot",
            "harness.generation_mismatch_check",
        ],
        "transition.render_reflow": ["harness.render_projection_read_only_diff"],
        "transition.terminal_signal_or_resize": [
            "harness.generation_mismatch_check"
        ],
        "transition.volume_operation": [
            "harness.transition_before_after_snapshot",
            "harness.blocked_transition_no_unrelated_mutation",
        ],
    }.get(transition_id, ["harness.transition_before_after_snapshot"])


def _wrong_diff_harness_ref(transition_id: str) -> str:
    valid_refs = set(_diff_harness_refs_for_transition(transition_id))
    for harness in _complete_diff_harness_checks():
        harness_id = str(harness["harness_id"])
        if harness_id not in valid_refs:
            return harness_id
    raise AssertionError(f"missing mismatched diff harness fixture for {transition_id}")


def _diff_harness_checks_with_transition_mismatch(
    harness_id: str, transition_id: str
) -> list[dict[str, object]]:
    diff_harness_checks = _complete_diff_harness_checks()
    fallback_transition_id = next(
        candidate
        for candidate in _complete_transition_ids()
        if candidate != transition_id
    )
    for harness in diff_harness_checks:
        if harness["harness_id"] == harness_id:
            harness["transition_ids"] = [fallback_transition_id]
            return diff_harness_checks
    raise AssertionError(f"missing diff harness fixture for {harness_id}")


def _wrong_generation_domain_ref(transition_id: str) -> str:
    valid_refs = set(_generation_domain_refs_for_transition(transition_id))
    for domain in _complete_generation_domains():
        domain_id = str(domain["domain_id"])
        if domain_id not in valid_refs:
            return domain_id
    raise AssertionError(
        f"missing mismatched generation domain fixture for {transition_id}"
    )


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
        "command_completion_dispatch": "transition.command_completion",
        "volume_menu_selection": "transition.menu_action",
        "rebuild_rebind_callback": "transition.rebuild_rebind_callback",
    }
    sequence_by_surface_category = {
        "key_decode_input_dispatch": "sequence.split_toggle_f8",
        "directory_window_action_dispatch": "sequence.split_toggle_f8",
        "file_window_action_dispatch": "sequence.file_small_big_transitions",
        "menu_modal_completion": "sequence.esc_modal_dismissal",
        "resize_signal_handling": "sequence.terminal_resize_reflow",
        "refresh_rebuild_rebind": "sequence.refresh_rebuild",
        "filesystem_mutation_result": "sequence.filesystem_mutation_result",
        "volume_operation": "sequence.volume_cycling_release",
        "watcher_live_refresh": "sequence.refresh_rebuild",
        "render_reflow_projection": "sequence.render_reflow_projection",
        "command_completion_dispatch": "sequence.search_jump",
        "volume_menu_selection": "sequence.volume_menu_select",
        "rebuild_rebind_callback": "sequence.refresh_rebuild",
    }
    resolved_transition_id = transition_id or transition_by_surface_category[category]
    allowed_direct_writes = ["field"]
    if resolved_transition_id == "transition.render_reflow":
        allowed_direct_writes = ["panel.tree_selection_key"]
    return {
        "surface_id": surface_id or f"surface.{category}",
        "category": category,
        "source_path": "src/ui/key_engine.c",
        "entry_symbol_or_path": "GetEventOrKey",
        "transition_id": resolved_transition_id,
        "boundary_status": "test",
        "allowed_direct_writes": allowed_direct_writes,
        "transition_sequence_refs": [sequence_by_surface_category[category]],
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
    coverage_transition_ids: list[str] | None = None,
    advances_on_transition_ids: list[str] | None = None,
) -> dict[str, object]:
    resolved_coverage_transition_ids = (
        coverage_transition_ids
        if coverage_transition_ids is not None
        else ["transition.keybinding"]
    )
    resolved_advances_on_transition_ids = (
        advances_on_transition_ids
        if advances_on_transition_ids is not None
        else ["transition.keybinding"]
    )
    return {
        "domain_id": domain_id or f"domain.{category}",
        "category": category,
        "owner_region": "panel-local state",
        "generation_owner_field": "field",
        "identity_fields": ["field"],
        "coverage_transition_ids": resolved_coverage_transition_ids,
        "advances_on_transition_ids": resolved_advances_on_transition_ids,
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
    action_id: str | None = "ACTION_NONE",
    event_id: str | None = None,
    invariant_ids: list[str] | None = None,
    diff_harness_ids: list[str] | None = None,
    action_coverage_refs: list[str] | None = None,
    event_coverage_refs: list[str] | None = None,
    generation_domain_id: str = "domain.panel_generation",
    expected_result: str = "allowed",
) -> dict[str, object]:
    stimulus: dict[str, object] = {}
    if action_id is not None:
        stimulus["action_id"] = action_id
    if event_id is not None:
        stimulus["event_id"] = event_id
    return {
        "ordinal": ordinal,
        "step_id": f"step.{ordinal}",
        "transition_id": transition_id,
        "stimulus": stimulus,
        "action_coverage_refs": (
            action_coverage_refs
            if action_coverage_refs is not None
            else ([action_id] if action_id is not None else [])
        ),
        "event_coverage_refs": (
            event_coverage_refs
            if event_coverage_refs is not None
            else ([event_id] if event_id is not None else [])
        ),
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
    category_by_flow = {
        "esc_modal_dismissal": "modal_command",
        "filesystem_mutation_result": "filesystem_mutation",
        "volume_menu_select": "menu_action",
        "refresh_rebuild": "refresh_rebuild",
        "render_reflow_projection": "render_reflow",
        "terminal_resize_reflow": "terminal_resize",
        "volume_cycling_release": "volume_lifecycle",
    }
    step_by_flow = {
        "esc_modal_dismissal": _sequence_step(
            transition_id="transition.modal_action",
            action_id=None,
            event_id="event.modal_completion",
            invariant_ids=["invariant.blocked_transition_determinism"],
            diff_harness_ids=["harness.declared_write_set_diff"],
        ),
        "filesystem_mutation_result": _sequence_step(
            transition_id="transition.filesystem_mutation_result",
            action_id=None,
            event_id="event.filesystem_mutation_result",
            invariant_ids=["invariant.blocked_transition_determinism"],
        ),
        "search_jump": _sequence_step(
            transition_id="transition.command_completion",
            action_id=None,
            event_id="event.command_completion",
            invariant_ids=["invariant.blocked_transition_determinism"],
            diff_harness_ids=["harness.declared_write_set_diff"],
            generation_domain_id="domain.modal_command_target",
        ),
        "volume_menu_select": _sequence_step(
            transition_id="transition.menu_action",
            action_id="ACTION_VOL_MENU",
            invariant_ids=["invariant.shared_state_panel_local_isolation"],
            diff_harness_ids=["harness.declared_write_set_diff"],
        ),
        "refresh_rebuild": _sequence_step(
            transition_id="transition.refresh_rebuild",
            action_id=None,
            event_id="event.refresh_rebuild",
            invariant_ids=[
                "invariant.blocked_transition_determinism",
                "invariant.hidden_entry_visible_navigation",
            ],
        ),
        "rebuild_rebind_callback": _sequence_step(
            ordinal=2,
            transition_id="transition.rebuild_rebind_callback",
            action_id=None,
            event_id="event.rebuild_rebind_callback",
            invariant_ids=[
                "invariant.blocked_transition_determinism",
                "invariant.hidden_entry_visible_navigation",
            ],
        ),
        "render_reflow_projection": _sequence_step(
            transition_id="transition.render_reflow",
            action_id=None,
            event_id="event.render_reflow",
            invariant_ids=["invariant.render_projection_read_only"],
            diff_harness_ids=["harness.render_projection_read_only_diff"],
            generation_domain_id="domain.layout_reflow",
        ),
        "terminal_resize_reflow": _sequence_step(
            transition_id="transition.terminal_signal_or_resize",
            action_id=None,
            event_id="event.terminal_resize_signal",
            invariant_ids=["invariant.render_projection_read_only"],
            diff_harness_ids=["harness.generation_mismatch_check"],
            generation_domain_id="domain.layout_reflow",
        ),
        "volume_cycling_release": _sequence_step(
            transition_id="transition.volume_operation",
            action_id=None,
            event_id="event.volume_lifecycle",
            invariant_ids=["invariant.shared_state_panel_local_isolation"],
        ),
    }
    return [
        _transition_sequence(
            flow,
            category=category_by_flow.get(
                flow,
                "layout_split" if flow.startswith("split_") else "panel_navigation",
            ),
            steps=(
                [
                    copy.deepcopy(step_by_flow["refresh_rebuild"]),
                    copy.deepcopy(step_by_flow["rebuild_rebind_callback"]),
                ]
                if flow == "refresh_rebuild"
                else [copy.deepcopy(step_by_flow.get(flow, _sequence_step()))]
            ),
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
    transition_side_effects = []
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
        side_effects = record.get("side_effects")
        if not isinstance(side_effects, list):
            side_effects = []
        side_effect_rows = "\n".join(
            f'  "{effect}",' for effect in side_effects if isinstance(effect, str)
        )
        transition_side_effects.append(
            "static const char *const kAppStateTransitionSideEffects"
            f"{index}[] = {{\n{side_effect_rows}\n}};\n"
        )
        transition_rows.append(
            f'  {{"{record.get("id", "")}", "{record.get("category", "")}", '
            f'"{record.get("source_state", "")}", "{record.get("event", "")}", '
            f'"{record.get("guard", "")}", "{record.get("allowed_result", "")}", '
            f'"{record.get("blocked_result", "")}", '
            f'"{record.get("target_state", "")}", "{record.get("owner", "")}", '
            f'"{record.get("generation_effect", "")}", '
            f"kAppStateTransitionSideEffects{index}, "
            f"sizeof(kAppStateTransitionSideEffects{index}) / "
            f"sizeof(kAppStateTransitionSideEffects{index}[0]), "
            f'"{record.get("render_invalidation", "")}", '
            f'"{record.get("boundary_status", "")}", '
            f'"{record.get("notes_follow_up", "")}", '
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
        owner_field_refs = record.get("owner_field_refs")
        if not isinstance(owner_field_refs, list):
            owner_field_refs = []
        owner_ref_rows = "\n".join(
            f'  "{field}",' for field in owner_field_refs if isinstance(field, str)
        )
        owner_refs_table = f"kAppStateActionCoverageOwnerFieldRefs{index}"
        action_coverage_arrays.append(
            f"static const char *const {owner_refs_table}[] = "
            f"{{\n{owner_ref_rows}\n}};\n"
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
        transition_sequence_refs = record.get("transition_sequence_refs")
        if not isinstance(transition_sequence_refs, list):
            transition_sequence_refs = []
        sequence_ref_rows = "\n".join(
            f'  "{ref}",' for ref in transition_sequence_refs if isinstance(ref, str)
        )
        sequence_refs_table = f"kAppStateActionCoverageTransitionSequenceRefs{index}"
        action_coverage_arrays.append(
            f"static const char *const {sequence_refs_table}[] = "
            f"{{\n{sequence_ref_rows}\n}};\n"
        )
        dispatch_surface_refs = record.get("dispatch_surface_refs")
        if not isinstance(dispatch_surface_refs, list):
            dispatch_surface_refs = []
        dispatch_surface_ref_rows = "\n".join(
            f'  "{ref}",' for ref in dispatch_surface_refs if isinstance(ref, str)
        )
        dispatch_surface_refs_table = (
            f"kAppStateActionCoverageDispatchSurfaceRefs{index}"
        )
        action_coverage_arrays.append(
            f"static const char *const {dispatch_surface_refs_table}[] = "
            f"{{\n{dispatch_surface_ref_rows}\n}};\n"
        )
        invariant_refs = record.get("invariant_refs")
        if not isinstance(invariant_refs, list):
            invariant_refs = []
        invariant_ref_rows = "\n".join(
            f'  "{ref}",' for ref in invariant_refs if isinstance(ref, str)
        )
        invariant_refs_table = f"kAppStateActionCoverageInvariantRefs{index}"
        action_coverage_arrays.append(
            f"static const char *const {invariant_refs_table}[] = "
            f"{{\n{invariant_ref_rows}\n}};\n"
        )
        generation_domain_refs = record.get("generation_domain_refs")
        if not isinstance(generation_domain_refs, list):
            generation_domain_refs = []
        generation_domain_ref_rows = "\n".join(
            f'  "{ref}",'
            for ref in generation_domain_refs
            if isinstance(ref, str)
        )
        generation_domain_refs_table = (
            f"kAppStateActionCoverageGenerationDomainRefs{index}"
        )
        action_coverage_arrays.append(
            f"static const char *const {generation_domain_refs_table}[] = "
            f"{{\n{generation_domain_ref_rows}\n}};\n"
        )
        diff_harness_refs = record.get("diff_harness_refs")
        if not isinstance(diff_harness_refs, list):
            diff_harness_refs = []
        diff_harness_ref_rows = "\n".join(
            f'  "{ref}",' for ref in diff_harness_refs if isinstance(ref, str)
        )
        diff_harness_refs_table = f"kAppStateActionCoverageDiffHarnessRefs{index}"
        action_coverage_arrays.append(
            f"static const char *const {diff_harness_refs_table}[] = "
            f"{{\n{diff_harness_ref_rows}\n}};\n"
        )
        action = record.get("action", "")
        action_coverage_rows.append(
            f'  {{{action}, "{action}", "{record.get("transition_id", "")}", '
            f'"{record.get("category", "")}", "{record.get("owner", "")}", '
            f"{write_set_table}, sizeof({write_set_table}) / "
            f"sizeof({write_set_table}[0]), "
            f"{owner_refs_table}, sizeof({owner_refs_table}) / "
            f"sizeof({owner_refs_table}[0]), "
            f"{sequence_refs_table}, sizeof({sequence_refs_table}) / "
            f"sizeof({sequence_refs_table}[0]), "
            f"{dispatch_surface_refs_table}, sizeof({dispatch_surface_refs_table}) / "
            f"sizeof({dispatch_surface_refs_table}[0]), "
            f"{invariant_refs_table}, sizeof({invariant_refs_table}) / "
            f"sizeof({invariant_refs_table}[0]), "
            f"{generation_domain_refs_table}, "
            f"sizeof({generation_domain_refs_table}) / "
            f"sizeof({generation_domain_refs_table}[0]), "
            f"{diff_harness_refs_table}, "
            f"sizeof({diff_harness_refs_table}) / "
            f"sizeof({diff_harness_refs_table}[0]), "
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
        transition_sequence_refs = record.get("transition_sequence_refs")
        if not isinstance(transition_sequence_refs, list):
            transition_sequence_refs = []
        sequence_ref_rows = "\n".join(
            f'  "{ref}",' for ref in transition_sequence_refs if isinstance(ref, str)
        )
        sequence_refs_table = f"kAppStateEventCoverageTransitionSequenceRefs{index}"
        event_coverage_arrays.append(
            f"static const char *const {sequence_refs_table}[] = "
            f"{{\n{sequence_ref_rows}\n}};\n"
        )
        dispatch_surface_refs = record.get("dispatch_surface_refs")
        if not isinstance(dispatch_surface_refs, list):
            dispatch_surface_refs = []
        dispatch_surface_ref_rows = "\n".join(
            f'  "{ref}",' for ref in dispatch_surface_refs if isinstance(ref, str)
        )
        dispatch_surface_refs_table = (
            f"kAppStateEventCoverageDispatchSurfaceRefs{index}"
        )
        event_coverage_arrays.append(
            f"static const char *const {dispatch_surface_refs_table}[] = "
            f"{{\n{dispatch_surface_ref_rows}\n}};\n"
        )
        invariant_refs = record.get("invariant_refs")
        if not isinstance(invariant_refs, list):
            invariant_refs = []
        invariant_ref_rows = "\n".join(
            f'  "{ref}",' for ref in invariant_refs if isinstance(ref, str)
        )
        invariant_refs_table = f"kAppStateEventCoverageInvariantRefs{index}"
        event_coverage_arrays.append(
            f"static const char *const {invariant_refs_table}[] = "
            f"{{\n{invariant_ref_rows}\n}};\n"
        )
        generation_domain_refs = record.get("generation_domain_refs")
        if not isinstance(generation_domain_refs, list):
            generation_domain_refs = []
        generation_domain_ref_rows = "\n".join(
            f'  "{ref}",'
            for ref in generation_domain_refs
            if isinstance(ref, str)
        )
        generation_domain_refs_table = (
            f"kAppStateEventCoverageGenerationDomainRefs{index}"
        )
        event_coverage_arrays.append(
            f"static const char *const {generation_domain_refs_table}[] = "
            f"{{\n{generation_domain_ref_rows}\n}};\n"
        )
        diff_harness_refs = record.get("diff_harness_refs")
        if not isinstance(diff_harness_refs, list):
            diff_harness_refs = []
        diff_harness_ref_rows = "\n".join(
            f'  "{ref}",' for ref in diff_harness_refs if isinstance(ref, str)
        )
        diff_harness_refs_table = f"kAppStateEventCoverageDiffHarnessRefs{index}"
        event_coverage_arrays.append(
            f"static const char *const {diff_harness_refs_table}[] = "
            f"{{\n{diff_harness_ref_rows}\n}};\n"
        )
        owner_field_refs = record.get("owner_field_refs")
        if not isinstance(owner_field_refs, list):
            owner_field_refs = []
        owner_ref_rows = "\n".join(
            f'  "{field}",' for field in owner_field_refs if isinstance(field, str)
        )
        owner_refs_table = f"kAppStateEventCoverageOwnerFieldRefs{index}"
        event_coverage_arrays.append(
            f"static const char *const {owner_refs_table}[] = "
            f"{{\n{owner_ref_rows}\n}};\n"
        )
        transition_index = transition_index_by_id.get(str(record.get("transition_id", "")), 0)
        write_set_table = f"kAppStateTransitionWriteSet{transition_index}"
        event_coverage_rows.append(
            f'  {{"{record.get("event_id", "")}", "{record.get("event_class", "")}", '
            f'"{record.get("transition_id", "")}", "{record.get("category", "")}", '
            f'"{record.get("source", "")}", "{record.get("owner", "")}", '
            f"{write_set_table}, sizeof({write_set_table}) / "
            f"sizeof({write_set_table}[0]), "
            f"{owner_refs_table}, sizeof({owner_refs_table}) / "
            f"sizeof({owner_refs_table}[0]), "
            f'"{record.get("boundary_status", "")}", '
            f"{trigger_table}, sizeof({trigger_table}) / sizeof({trigger_table}[0]), "
            f"{sequence_refs_table}, sizeof({sequence_refs_table}) / "
            f"sizeof({sequence_refs_table}[0]), "
            f"{dispatch_surface_refs_table}, sizeof({dispatch_surface_refs_table}) / "
            f"sizeof({dispatch_surface_refs_table}[0]), "
            f"{invariant_refs_table}, sizeof({invariant_refs_table}) / "
            f"sizeof({invariant_refs_table}[0]), "
            f"{generation_domain_refs_table}, "
            f"sizeof({generation_domain_refs_table}) / "
            f"sizeof({generation_domain_refs_table}[0]), "
            f"{diff_harness_refs_table}, "
            f"sizeof({diff_harness_refs_table}) / "
            f"sizeof({diff_harness_refs_table}[0]), "
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
        transition_sequence_refs = record.get("transition_sequence_refs")
        if not isinstance(transition_sequence_refs, list):
            transition_sequence_refs = []
        sequence_ref_rows = "\n".join(
            f'  "{ref}",' for ref in transition_sequence_refs if isinstance(ref, str)
        )
        sequence_refs_table = f"kAppStateDispatchSurfaceTransitionSequenceRefs{index}"
        dispatch_surface_arrays.append(
            f"static const char *const {sequence_refs_table}[] = "
            f"{{\n{sequence_ref_rows}\n}};\n"
        )
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
            f"{sequence_refs_table}, sizeof({sequence_refs_table}) / "
            f"sizeof({sequence_refs_table}[0]), "
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
        ("coverage_transition_ids", "CoverageTransitionIds"),
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
            f"kAppStateGenerationDomainCoverageTransitionIds{index}, "
            f"sizeof(kAppStateGenerationDomainCoverageTransitionIds{index}) / "
            f"sizeof(kAppStateGenerationDomainCoverageTransitionIds{index}[0]), "
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
            action_coverage_refs = step.get("action_coverage_refs")
            if isinstance(action_coverage_refs, list) and action_coverage_refs:
                action_sequence_ref_rows = "\n".join(
                    f'  "{ref}",'
                    for ref in action_coverage_refs
                    if isinstance(ref, str)
                )
                action_coverage_table = (
                    "kAppStateTransitionSequenceStepActionCoverageRefs"
                    f"{sequence_index}_{step_index}"
                )
                transition_sequence_arrays.append(
                    f"static const char *const {action_coverage_table}[] = "
                    f"{{\n{action_sequence_ref_rows}\n}};\n"
                )
                action_coverage_ref_expr = (
                    f"{action_coverage_table}, sizeof({action_coverage_table}) / "
                    f"sizeof({action_coverage_table}[0])"
                )
            else:
                action_coverage_ref_expr = "NULL, 0"
            event_coverage_refs = step.get("event_coverage_refs")
            if isinstance(event_coverage_refs, list) and event_coverage_refs:
                event_sequence_ref_rows = "\n".join(
                    f'  "{ref}",'
                    for ref in event_coverage_refs
                    if isinstance(ref, str)
                )
                event_coverage_table = (
                    "kAppStateTransitionSequenceStepEventCoverageRefs"
                    f"{sequence_index}_{step_index}"
                )
                transition_sequence_arrays.append(
                    f"static const char *const {event_coverage_table}[] = "
                    f"{{\n{event_sequence_ref_rows}\n}};\n"
                )
                event_coverage_ref_expr = (
                    f"{event_coverage_table}, sizeof({event_coverage_table}) / "
                    f"sizeof({event_coverage_table}[0])"
                )
            else:
                event_coverage_ref_expr = "NULL, 0"
            precondition = step.get("precondition")
            precondition_expr = (
                f'"{precondition}"' if isinstance(precondition, str) else "NULL"
            )
            step_rows.append(
                f'  {{{step.get("ordinal", 0)}, "{step.get("step_id", "")}", '
                f'"{step.get("transition_id", "")}", {action_expr}, {event_expr}, '
                f"{action_coverage_ref_expr}, {event_coverage_ref_expr}, "
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
        + "".join(transition_side_effects)
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


def _wrong_dispatch_surface_ref(transition_id: str) -> str:
    for surface in _complete_dispatch_surfaces():
        surface_transition_id = str(surface["transition_id"])
        if surface_transition_id != transition_id:
            return str(surface["surface_id"])
    raise AssertionError(f"missing mismatched dispatch surface fixture for {transition_id}")


def _wrong_invariant_ref(transition_id: str) -> str:
    for invariant in _complete_invariants():
        transition_ids = invariant.get("transition_ids")
        if isinstance(transition_ids, list) and transition_id not in transition_ids:
            return str(invariant["invariant_id"])
    raise AssertionError(f"missing mismatched invariant fixture for {transition_id}")


def _event_index(event_id: str) -> int:
    for index, event in enumerate(_complete_events()):
        if event["event_id"] == event_id:
            return index
    raise AssertionError(f"missing fixture event {event_id}")


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
    transition_ids_by_category = {
        "inactive_panel_frozen": [
            "transition.keybinding",
            "transition.menu_action",
            "transition.modal_action",
            "transition.refresh_rebuild",
            "transition.terminal_signal_or_resize",
        ],
        "render_projection_read_only": [
            "transition.render_reflow",
            "transition.terminal_signal_or_resize",
        ],
        "hidden_entry_visible_navigation": [
            "transition.keybinding",
            "transition.refresh_rebuild",
            "transition.rebuild_rebind_callback",
        ],
        "panel_local_focus_restore": [
            "transition.keybinding",
            "transition.menu_action",
            "transition.modal_action",
            "transition.rebuild_rebind_callback",
        ],
        "viewport_identity_rebind": [
            "transition.refresh_rebuild",
            "transition.terminal_signal_or_resize",
            "transition.filesystem_mutation_result",
            "transition.rebuild_rebind_callback",
        ],
        "shared_state_panel_local_isolation": [
            "transition.menu_action",
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.filesystem_mutation_result",
        ],
        "stale_snapshot_fail_closed": [
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.filesystem_mutation_result",
            "transition.rebuild_rebind_callback",
        ],
        "blocked_transition_determinism": _complete_transition_ids(),
    }
    return [
        _invariant(
            category,
            transition_ids=transition_ids_by_category[category],
            dispatch_surface_ids=dispatch_surface_ids_by_category[category],
        )
        for category in REQUIRED_INVARIANT_CATEGORIES
    ]


def _complete_generation_domains() -> list[dict[str, object]]:
    coverage_transition_ids_by_category = {
        "panel_generation": [
            "transition.keybinding",
            "transition.menu_action",
            "transition.modal_action",
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.terminal_signal_or_resize",
            "transition.rebuild_rebind_callback",
            "transition.filesystem_mutation_result",
        ],
        "volume_generation": [
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.filesystem_mutation_result",
        ],
        "directory_identity": [
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.filesystem_mutation_result",
            "transition.rebuild_rebind_callback",
        ],
        "file_identity": [
            "transition.refresh_rebuild",
            "transition.filesystem_mutation_result",
            "transition.rebuild_rebind_callback",
        ],
        "focus_shape": [
            "transition.keybinding",
            "transition.menu_action",
            "transition.modal_action",
            "transition.rebuild_rebind_callback",
        ],
        "modal_command_target": [
            "transition.modal_action",
            "transition.command_completion",
        ],
        "visibility_filter_state": [
            "transition.keybinding",
            "transition.refresh_rebuild",
            "transition.rebuild_rebind_callback",
        ],
        "topology_state": [
            "transition.refresh_rebuild",
            "transition.volume_operation",
            "transition.filesystem_mutation_result",
        ],
        "file_payload_state": [
            "transition.refresh_rebuild",
            "transition.filesystem_mutation_result",
            "transition.rebuild_rebind_callback",
        ],
        "volume_lifecycle": [
            "transition.menu_action",
            "transition.refresh_rebuild",
            "transition.volume_operation",
        ],
        "layout_reflow": [
            "transition.terminal_signal_or_resize",
            "transition.render_reflow",
        ],
    }
    advances_on_transition_ids_by_category = {
        **coverage_transition_ids_by_category,
        "layout_reflow": ["transition.terminal_signal_or_resize"],
    }
    return [
        _generation_domain(
            category,
            "domain.panel_generation" if category == "panel_generation" else None,
            coverage_transition_ids_by_category[category],
            advances_on_transition_ids_by_category[category],
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
                "invariant.hidden_entry_visible_navigation",
                "invariant.panel_local_focus_restore",
                "invariant.render_projection_read_only",
                "invariant.shared_state_panel_local_isolation",
                "invariant.viewport_identity_rebind",
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


def test_current_generation_domain_docs_keep_projection_transitions_out_of_advances() -> None:
    generation_domains = json.loads(
        Path("docs/appstate_generation_domains.json").read_text(encoding="utf-8")
    )["generation_domains"]
    projection_domain = next(
        record
        for record in generation_domains
        if record["domain_id"] == "reflow.layout.projection"
    )

    assert "transition.render-reflow.project-state" not in projection_domain[
        "advances_on_transition_ids"
    ]
    assert "transition.render-reflow.project-state" in projection_domain[
        "coverage_transition_ids"
    ]


def test_runtime_generation_domain_ref_readiness_uses_coverage_transitions() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateGenerationDomainRefsReady(")
    action_start = source.index("static int AppStateActionTransitionsReady(void)")
    helper_body = source[helper_start:action_start]

    assert "domain->coverage_transition_ids" in helper_body
    assert "domain->coverage_transition_id_count" in helper_body
    assert "domain->advances_on_transition_ids" not in helper_body


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


@pytest.mark.parametrize(
    ("mutation", "expected"),
    (
        ("missing_action", "action_coverage_refs must be a list"),
        ("malformed_action", "action_coverage_refs must be a list"),
        ("duplicate_action", "duplicate action_coverage_refs"),
        ("unknown_action", "unknown action coverage record"),
        ("wrong_kind_action", "event_coverage_refs must be empty"),
        ("stimulus_mismatch_action", "does not match stimulus ACTION_NONE"),
        ("mixed_action", "does not match stimulus ACTION_NONE"),
        ("missing_event", "event_coverage_refs must be a list"),
        ("malformed_event", "event_coverage_refs must be a list"),
        ("duplicate_event", "duplicate event_coverage_refs"),
        ("unknown_event", "unknown event coverage record"),
        ("wrong_kind_event", "action_coverage_refs must be empty"),
        ("stimulus_mismatch_event", "does not match stimulus event.modal_completion"),
        ("mixed_event", "does not match stimulus event.modal_completion"),
    ),
)
def test_guard_fails_on_transition_sequence_step_coverage_refs(
    tmp_path: Path, mutation: str, expected: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    action_step = transition_sequences[0]["steps"][0]
    event_step = transition_sequences[0]["steps"][0] = _sequence_step(
        transition_id="transition.modal_action",
        action_id=None,
        event_id="event.modal_completion",
        invariant_ids=["invariant.blocked_transition_determinism"],
    )
    target_step = action_step

    if mutation == "missing_action":
        action_step.pop("action_coverage_refs")
        target_step = action_step
    elif mutation == "malformed_action":
        action_step["action_coverage_refs"] = "ACTION_NONE"
        target_step = action_step
    elif mutation == "duplicate_action":
        action_step["action_coverage_refs"] = ["ACTION_NONE", "ACTION_NONE"]
        target_step = action_step
    elif mutation == "unknown_action":
        action_step["action_coverage_refs"] = ["ACTION_MISSING"]
        target_step = action_step
    elif mutation == "wrong_kind_action":
        action_step["event_coverage_refs"] = ["event.modal_completion"]
        target_step = action_step
    elif mutation == "stimulus_mismatch_action":
        action_step["action_coverage_refs"] = ["ACTION_VOL_MENU"]
        target_step = action_step
    elif mutation == "mixed_action":
        action_step["action_coverage_refs"] = ["ACTION_NONE", "ACTION_VOL_MENU"]
        target_step = action_step
    elif mutation == "missing_event":
        event_step.pop("event_coverage_refs")
    elif mutation == "malformed_event":
        event_step["event_coverage_refs"] = "event.modal_completion"
    elif mutation == "duplicate_event":
        event_step["event_coverage_refs"] = [
            "event.modal_completion",
            "event.modal_completion",
        ]
    elif mutation == "unknown_event":
        event_step["event_coverage_refs"] = ["event.missing"]
    elif mutation == "wrong_kind_event":
        event_step["action_coverage_refs"] = ["ACTION_NONE"]
    elif mutation == "stimulus_mismatch_event":
        event_step["event_coverage_refs"] = ["event.refresh_rebuild"]
    elif mutation == "mixed_event":
        event_step["event_coverage_refs"] = [
            "event.modal_completion",
            "event.refresh_rebuild",
        ]
    else:
        raise AssertionError(mutation)

    if mutation.endswith("action"):
        transition_sequences[0]["steps"][0] = target_step

    paths = _write_fixture(
        tmp_path, transitions=transitions, transition_sequences=transition_sequences
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure and expected in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("record_type", "expected"),
    (
        ("action", "action_coverage_refs[0] transition_id does not match"),
        ("event", "event_coverage_refs[0] transition_id does not match"),
    ),
)
def test_guard_fails_when_sequence_step_coverage_ref_transition_mismatches(
    tmp_path: Path, record_type: str, expected: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    actions = _complete_actions()
    events = _complete_events()
    if record_type == "action":
        for action in actions:
            if action["action"] == "ACTION_NONE":
                action["transition_id"] = "transition.refresh_rebuild"
                break
    else:
        transition_sequences[0]["steps"][0] = _sequence_step(
            transition_id="transition.modal_action",
            action_id=None,
            event_id="event.modal_completion",
            invariant_ids=["invariant.blocked_transition_determinism"],
        )
        for event in events:
            if event["event_id"] == "event.modal_completion":
                event["transition_id"] = "transition.refresh_rebuild"
                break
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        events=events,
        transition_sequences=transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure and expected in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("coverage_field", "replacement_refs", "expected"),
    (
        (
            "invariant_refs",
            ["invariant.blocked_transition_determinism"],
            "invariant_refs must overlap step invariant_ids",
        ),
        (
            "diff_harness_refs",
            ["harness.generation_mismatch_check"],
            "diff_harness_refs must overlap step diff_harness_ids",
        ),
        (
            "generation_domain_refs",
            ["domain.focus_shape"],
            "generation_domain_refs must overlap step generation_domain_expectations",
        ),
    ),
)
def test_guard_fails_when_sequence_step_action_coverage_lacks_semantic_overlap(
    tmp_path: Path,
    coverage_field: str,
    replacement_refs: list[str],
    expected: str,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    for action in actions:
        if action["action"] == "ACTION_NONE":
            action[coverage_field] = replacement_refs
            break
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        transition_sequences=_complete_transition_sequences(),
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "action_coverage_refs[0] ACTION_NONE" in failure
        and expected in failure
        for failure in failures
    )


def test_guard_fails_when_mixed_sequence_step_coverage_lacks_semantic_overlap(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    for action in actions:
        if action["action"] == "ACTION_NONE":
            action["transition_id"] = "transition.modal_action"
            action["category"] = "modal_action"
            action["invariant_refs"] = ["invariant.blocked_transition_determinism"]
            action["diff_harness_refs"] = ["harness.declared_write_set_diff"]
            action["generation_domain_refs"] = ["domain.panel_generation"]
            break
    events = _complete_events()
    for event in events:
        if event["event_id"] == "event.modal_completion":
            event["invariant_refs"] = ["invariant.inactive_panel_frozen"]
            break
    transition_sequences = _complete_transition_sequences()
    transition_sequences[0]["steps"][0] = _sequence_step(
        transition_id="transition.modal_action",
        action_id="ACTION_NONE",
        event_id="event.modal_completion",
        invariant_ids=["invariant.blocked_transition_determinism"],
        diff_harness_ids=["harness.declared_write_set_diff"],
        generation_domain_id="domain.panel_generation",
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        events=events,
        transition_sequences=transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "transition_sequence[0].step[0]" in failure
        and "event_coverage_refs[0] event.modal_completion" in failure
        and "invariant_refs must overlap step invariant_ids" in failure
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


def test_guard_fails_when_runtime_transition_sequence_coverage_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    runtime_transition_sequences[0]["steps"][0]["action_coverage_refs"] = [
        "ACTION_VOL_MENU"
    ]
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
        and "ACTION_VOL_MENU" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_coverage_ref_is_malformed(
    tmp_path: Path,
) -> None:
    paths = _write_fixture(tmp_path, transitions=_complete_transitions())
    runtime_path = paths[-1]
    runtime_path.write_text(
        runtime_path.read_text(encoding="utf-8").replace(
            'static const char *const kAppStateTransitionSequenceStepActionCoverageRefs0_0[] = {\n'
            '  "ACTION_NONE",',
            'static const char *const kAppStateTransitionSequenceStepActionCoverageRefs0_0[] = {\n'
            "  NULL,\n"
            '  "ACTION_NONE",',
            1,
        ),
        encoding="utf-8",
    )

    failures = _validate(paths)

    assert any(
        "kAppStateTransitionSequenceStepActionCoverageRefs0_0[0]" in failure
        and "malformed string literal entry" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_transition_sequence_coverage_ref_mismatches_stimulus(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    for sequences in (transition_sequences, runtime_transition_sequences):
        sequences[0]["steps"][0]["action_coverage_refs"] = ["ACTION_VOL_MENU"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and "action_coverage_refs[0] ACTION_VOL_MENU does not match stimulus ACTION_NONE"
        in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field", "value", "expected"),
    (
        (
            "invariant_ids",
            ["invariant.blocked_transition_determinism"],
            "invariant_refs must overlap step invariant_ids",
        ),
        (
            "diff_harness_ids",
            ["harness.generation_mismatch_check"],
            "diff_harness_refs must overlap step diff_harness_ids",
        ),
        (
            "generation_domain_expectations",
            [{"domain_id": "domain.modal_command_target", "expectation": "fixture"}],
            "generation_domain_refs must overlap step generation_domain_expectations",
        ),
    ),
)
def test_guard_fails_when_runtime_transition_sequence_coverage_lacks_semantic_overlap(
    tmp_path: Path, field: str, value: object, expected: str
) -> None:
    transitions = _complete_transitions()
    transition_sequences = _complete_transition_sequences()
    runtime_transition_sequences = copy.deepcopy(transition_sequences)
    for sequences in (transition_sequences, runtime_transition_sequences):
        sequences[0]["steps"][0][field] = value
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        transition_sequences=transition_sequences,
        runtime_transition_sequences=runtime_transition_sequences,
    )

    failures = _validate(paths)

    assert any(
        "runtime_transition_sequence[0].step[0]" in failure
        and "action_coverage_refs[0] ACTION_NONE" in failure
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
    assert "AppStateActionCoverageIdLookup(step->action_coverage_refs[ref_index])" in source
    assert "AppStateEventCoverageLookup(step->event_coverage_refs[ref_index])" in source
    assert "AppStateTransitionSequenceStepCoverageOverlaps" in source
    assert "StringListsOverlap(step->invariant_ids, step->invariant_id_count" in source
    assert "StringListsOverlap(step->diff_harness_ids, step->diff_harness_id_count" in source
    assert "AppStateTransitionSequenceStepGenerationDomainOverlaps" in source
    assert "coverage->generation_domain_refs" in source
    assert "step->action_coverage_ref_count" in source
    assert "step->event_coverage_ref_count" in source
    assert (
        "step->action_coverage_refs != NULL ||\n"
        "             step->action_coverage_ref_count != 0"
    ) in source
    assert (
        "step->event_coverage_refs != NULL ||\n"
        "             step->event_coverage_ref_count != 0"
    ) in source
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
    assert "!NonEmptyString(metadata->guard)" in source
    assert "!NonEmptyString(metadata->blocked_result)" in source
    assert "!NonEmptyStringList(metadata->side_effects" in source
    assert "!NonEmptyString(metadata->render_invalidation)" in source
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


def test_runtime_generation_domain_validation_requires_covered_advances(
    tmp_path: Path,
) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "generation_domain_validation_probe.c"
    binary = tmp_path / "generation_domain_validation_probe"
    probe.write_text(
        """
#include "src/core/appstate_actions.c"

int main(void) {
  AppStateGenerationDomainMetadata mismatched_domain;
  AppStateGenerationDomainMetadata missing_transition;
  AppStateGenerationDomainMetadata uncovered_advance;
  AppStateGenerationDomainMetadata missing_notes;
  static const char *const missing_transition_ids[] = {
      "transition.__ytnova_missing__",
  };
  static const char *const uncovered_advance_ids[] = {
      "transition.command-completion.user-command",
  };

  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return 1;
  if (!AppStateValidatedGenerationDomain("generation.volume.shared-authority"))
    return 2;
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return 3;
  if (AppStateValidatedGenerationDomain(NULL))
    return 4;
  if (AppStateValidatedGenerationDomain(""))
    return 5;
  if (AppStateValidatedGenerationDomain("generation.__ytnova_missing__"))
    return 6;

  mismatched_domain =
      *AppStateGenerationDomainLookup("generation.panel.local-authority");
  mismatched_domain.domain_id = "generation.__ytnova_mismatch__";
  if (AppStateValidateGenerationDomain("generation.panel.local-authority",
                                       &mismatched_domain))
    return 7;

  missing_transition =
      *AppStateGenerationDomainLookup("generation.panel.local-authority");
  missing_transition.coverage_transition_ids = missing_transition_ids;
  missing_transition.coverage_transition_id_count = 1;
  if (AppStateValidateGenerationDomain("generation.panel.local-authority",
                                       &missing_transition))
    return 8;

  uncovered_advance =
      *AppStateGenerationDomainLookup("generation.panel.local-authority");
  uncovered_advance.advances_on_transition_ids = uncovered_advance_ids;
  uncovered_advance.advances_on_transition_id_count = 1;
  if (AppStateValidateGenerationDomain("generation.panel.local-authority",
                                       &uncovered_advance))
    return 9;

  missing_notes =
      *AppStateGenerationDomainLookup("generation.panel.local-authority");
  missing_notes.migration_notes = 0;
  missing_notes.migration_note_count = 0;
  if (AppStateValidateGenerationDomain("generation.panel.local-authority",
                                       &missing_notes))
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
            "-I.",
            "-Iinclude",
            str(probe),
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
  if (metadata->owner_field_refs == 0 || metadata->owner_field_ref_count == 0)
    return 10;
  if (metadata->dispatch_surface_refs == 0 || metadata->dispatch_surface_ref_count == 0)
    return 11;
  if (metadata->migration_notes == 0 || metadata->migration_note_count == 0)
    return 12;
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


def test_runtime_key_action_validation_requires_coverage_and_transition(
    tmp_path: Path,
) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "key_action_validation_probe.c"
    binary = tmp_path / "key_action_validation_probe"
    probe.write_text(
        """
#include "src/core/appstate_actions.c"

int main(void) {
  AppStateActionCoverageMetadata invalid_owner_refs;
  AppStateActionCoverageMetadata mismatched_owner;
  AppStateActionCoverageMetadata missing_transition;
  static const char *const missing_owner_field_refs[] = {
      "panel.__ytnova_missing__",
  };

  if (AppStateValidatedKeyAction(ACTION_ENTER) != ACTION_ENTER)
    return 1;
  if (AppStateValidatedKeyAction(ACTION_NONE) != ACTION_NONE)
    return 2;
  if (AppStateValidatedKeyAction((YtreeNovaAction)(ACTION_USER_CMD + 1)) !=
      ACTION_NONE)
    return 3;

  missing_transition = *AppStateActionCoverageLookup(ACTION_ENTER);
  missing_transition.transition_id = "transition.__ytnova_missing__";
  if (AppStateValidateKeyActionCoverage(ACTION_ENTER, &missing_transition) !=
      ACTION_NONE)
    return 4;

  invalid_owner_refs = *AppStateActionCoverageLookup(ACTION_ENTER);
  invalid_owner_refs.owner_field_refs = missing_owner_field_refs;
  invalid_owner_refs.owner_field_ref_count = 1;
  if (AppStateValidateKeyActionCoverage(ACTION_ENTER, &invalid_owner_refs) !=
      ACTION_NONE)
    return 5;

  mismatched_owner = *AppStateActionCoverageLookup(ACTION_ENTER);
  mismatched_owner.owner = "window.__ytnova_mismatch__";
  if (AppStateValidateKeyActionCoverage(ACTION_ENTER, &mismatched_owner) !=
      ACTION_NONE)
    return 6;

  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-I.",
            "-Iinclude",
            str(probe),
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


def test_runtime_dispatch_surface_validation_requires_registry_and_transition(
    tmp_path: Path,
) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "dispatch_surface_validation_probe.c"
    binary = tmp_path / "dispatch_surface_validation_probe"
    probe.write_text(
        """
#include "src/core/appstate_actions.c"

int main(void) {
  AppStateDispatchSurfaceMetadata mismatched_surface;
  AppStateDispatchSurfaceMetadata missing_transition;
  AppStateDispatchSurfaceMetadata blank_source_path;
  AppStateDispatchSurfaceMetadata unknown_allowed_write;
  AppStateDispatchSurfaceMetadata outside_allowed_write;
  AppStateDispatchSurfaceMetadata missing_sequences;
  AppStateDispatchSurfaceMetadata wrong_sequence;
  AppStateDispatchSurfaceMetadata missing_notes;
  static const char *const unknown_allowed_writes[] = {
      "field.__ytnova_missing__",
  };
  static const char *const outside_allowed_writes[] = {
      "ctx.command_state",
  };
  static const char *const wrong_sequence_refs[] = {
      "sequence.volume-cycling-release",
  };

  if (!AppStateValidatedDispatchSurface("surface.key-decode-input-dispatch"))
    return 1;
  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return 2;
  if (!AppStateValidatedDispatchSurface("surface.resize-signal-handling"))
    return 6;
  if (!AppStateValidatedDispatchSurface("surface.command-completion-dispatch"))
    return 8;
  if (!AppStateValidatedDispatchSurface("surface.watcher-live-refresh"))
    return 15;
  if (!AppStateValidatedDispatchSurface("surface.render-reflow-projection"))
    return 9;
  if (!AppStateValidatedDispatchSurface("surface.volume-menu-selection"))
    return 10;
  if (!AppStateValidatedDispatchSurface("surface.volume-operation"))
    return 11;
  if (!AppStateValidatedDispatchSurface("surface.file-window-action-dispatch"))
    return 12;
  if (!AppStateValidatedDispatchSurface("surface.directory-window-action-dispatch"))
    return 13;
  if (!AppStateValidatedDispatchSurface("surface.panel-anchor-rebind"))
    return 14;
  if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))
    return 16;
  if (AppStateValidatedDispatchSurface(NULL))
    return 3;
  if (AppStateValidatedDispatchSurface(""))
    return 4;
  if (AppStateValidatedDispatchSurface("surface.__ytnova_missing__"))
    return 5;

  mismatched_surface =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  mismatched_surface.surface_id = "surface.__ytnova_mismatch__";
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &mismatched_surface))
    return 6;

  missing_transition =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  missing_transition.transition_id = "transition.__ytnova_missing__";
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &missing_transition))
    return 7;

  blank_source_path =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  blank_source_path.source_path = "";
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &blank_source_path))
    return 17;

  unknown_allowed_write =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  unknown_allowed_write.allowed_direct_writes = unknown_allowed_writes;
  unknown_allowed_write.allowed_direct_write_count = 1;
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &unknown_allowed_write))
    return 18;

  outside_allowed_write =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  outside_allowed_write.allowed_direct_writes = outside_allowed_writes;
  outside_allowed_write.allowed_direct_write_count = 1;
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &outside_allowed_write))
    return 19;

  missing_sequences =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  missing_sequences.transition_sequence_refs = 0;
  missing_sequences.transition_sequence_ref_count = 0;
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &missing_sequences))
    return 20;

  wrong_sequence =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  wrong_sequence.transition_sequence_refs = wrong_sequence_refs;
  wrong_sequence.transition_sequence_ref_count = 1;
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &wrong_sequence))
    return 21;

  missing_notes =
      *AppStateDispatchSurfaceLookup("surface.key-decode-input-dispatch");
  missing_notes.migration_notes = 0;
  missing_notes.migration_note_count = 0;
  if (AppStateValidateDispatchSurface("surface.key-decode-input-dispatch",
                                      &missing_notes))
    return 22;

  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-I.",
            "-Iinclude",
            str(probe),
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


def test_input_choice_modal_completion_fails_closed_on_invalid_surface() -> None:
    source = Path("src/ui/key_engine.c").read_text(encoding="utf-8")

    assert 'include "ytnova_appstate_actions.h"' in source
    for function_name in ("InputChoice", "InputChoiceLiteral"):
        start = source.index(f"int {function_name}(")
        next_function = source.index("\nint ", start + 1)
        body = source[start:next_function]
        surface_validation = (
            'if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))'
        )
        event_validation = 'if (!AppStateValidatedEvent("event.modal-completion"))'
        boundary_calls = [
            "ClearHelp(ctx);",
            "curs_set(1);",
            "leaveok(ctx->ctx_border_window, FALSE);",
            "mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);",
            "PrintMenuOptions(" if function_name == "InputChoice" else "Print(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)msg,",
            "wnoutrefresh(ctx->ctx_border_window);",
            "doupdate();",
            "WGetch(ctx, ctx->ctx_border_window);",
        ]

        assert surface_validation in body
        assert event_validation in body
        assert "return ERR;" in body
        surface_idx = body.index(surface_validation)
        event_idx = body.index(event_validation, surface_idx)
        event_return_idx = body.index("return ERR;", event_idx)

        assert surface_idx < event_idx
        assert surface_idx < body.index("return ERR;", surface_idx) < event_idx
        assert event_idx < event_return_idx < body.index("ClearHelp(ctx);")
        for call in boundary_calls:
            assert event_idx < body.index(call)


def test_wgetch_resize_signal_handling_fails_closed_before_mutation() -> None:
    source = Path("src/ui/key_engine.c").read_text(encoding="utf-8")
    start = source.index("int WGetch(")
    end = source.index("\nint Getch(", start)
    body = source[start:end]
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.resize-signal-handling"))'
    )

    assert validation in body
    assert "return ERR;" in body
    assert body.index(validation) < body.index("ctx->resize_request = TRUE;")
    assert body.index("return ERR;") < body.index("ctx->resize_request = TRUE;")


def test_get_key_action_routes_decoded_actions_through_appstate_boundary() -> None:
    source = Path("src/ui/key_engine.c").read_text(encoding="utf-8")
    start = source.index("YtreeNovaAction GetKeyAction(")
    end = source.index("\nint WGetch(", start)
    body = source[start:end]

    assert 'include "ytnova_appstate_actions.h"' in source
    assert (
        'if (!AppStateValidatedDispatchSurface("surface.key-decode-input-dispatch"))'
        in body
    )
    assert body.count("return ACTION_NONE;") == 1
    assert body.index("return ACTION_NONE;") < body.index("switch (ch)")
    assert "AppStateValidatedKeyAction(" in body
    assert not re.search(r"return\s+ACTION_(?!NONE;)", body)


def test_command_completion_dispatch_fails_closed_before_command_work() -> None:
    source = Path("src/ui/ctrl_file_ops.c").read_text(encoding="utf-8")
    start = source.index("BOOL handle_file_window_command_action(")
    end = source.index("\nBOOL handle_file_window_misc_dispatch_action(", start)
    body = source[start:end]
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.command-completion-dispatch"))'
    )
    event_validation = 'if (!AppStateValidatedEvent("event.command-completion"))'
    early_return = "return FALSE;"
    boundary_calls = [
        "switch (action)",
        "GetActivePanelSelectedFile(",
        "GetCopyParameter(",
        "GetMoveParameter(",
        "InputChoice(",
        "CopyFile(",
        "MoveFile(",
        "DeleteFile(",
        "RenameFile(",
        "GetPipeCommand(",
        "GetCommandLine(",
        "*dir_entry_ptr =",
    ]

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert event_validation in body
    assert early_return in body
    validation_index = body.index(validation)
    event_validation_index = body.index(event_validation)
    switch_index = body.index("switch (action)")
    assert validation_index < event_validation_index
    assert body.index(early_return, validation_index) < event_validation_index
    assert body.index(early_return, event_validation_index) < switch_index
    for call in boundary_calls:
        assert validation_index < body.index(call)
        assert event_validation_index < body.index(call)


def test_refresh_view_render_reflow_projection_fails_closed_before_render_work() -> None:
    source = Path("src/ui/display.c").read_text(encoding="utf-8")
    start = source.index("void RefreshView(")
    body = source[start:]
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.render-reflow-projection"))'
    )
    event_validation = 'if (!AppStateValidatedEvent("event.render-reflow"))'
    early_return = "return;"
    boundary_calls = [
        "Layout_Recalculate(",
        "ReCreateWindows(",
        "DisplayMenu(",
        "DisplayDiskStatistic(",
        "UpdateStatsPanel(",
        "DisplayHeaderPath(",
        "DisplayTree(",
        "DisplayFileWindow(",
        "RenderInactivePanel(",
        "UI_Dialog_RefreshAll(",
        "ClockHandler(",
        "doupdate(",
    ]

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert event_validation in body
    validation_idx = body.index(validation)
    event_validation_idx = body.index(event_validation, validation_idx)
    early_return_idx = body.index(early_return, event_validation_idx)
    assert validation_idx < event_validation_idx < early_return_idx
    assert early_return_idx < body.index("&ctx->active->vol->vol_stats")
    for call in boundary_calls:
        assert body.index(validation) < body.index(call)
        assert early_return_idx < body.index(call)


def test_handle_file_window_dispatch_fails_closed_before_file_work() -> None:
    source = Path("src/ui/ctrl_file.c").read_text(encoding="utf-8")
    start = source.index("int HandleFileWindow(")
    body = source[start:]
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.file-window-action-dispatch"))'
    )
    early_return = "return ESC;"
    boundary_calls = [
        'DEBUG_LOG("HandleFileWindow ENTERED',
        "ctx->focused_window = FOCUS_FILE;",
        "ctx->active->saved_focus = FOCUS_FILE;",
        "ctx->active->saved_big_file_view =",
        "BuildFileEntryList(",
        "RefreshView(",
        "GetEventOrKey(",
        "GetKeyAction(",
    ]

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert body.index(validation) < body.index(early_return)
    for call in boundary_calls:
        assert body.index(validation) < body.index(call)
        assert body.index(early_return) < body.index(call)


def test_handle_file_window_revalidates_remapped_action_before_file_dispatch() -> None:
    source = Path("src/ui/ctrl_file.c").read_text(encoding="utf-8")
    start = source.index("int HandleFileWindow(")
    end = source.index("\nfile_window_done:", start)
    body = source[start:end]
    get_key_action = "action = GetKeyAction(ctx, ch);"
    mode_remap = "if (GetPanelFileMode(ctx->active) == MODE_1)"
    preview_filter = "action = FilterPreviewAction(action);"
    validation = "action = AppStateValidatedKeyAction(action);"
    first_dispatch_derivation = "if (FileNav_GetXStep(ctx) == 1 &&"
    switch_dispatch = "switch (action)"

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert (
        body.index(get_key_action)
        < body.index(mode_remap)
        < body.index(preview_filter)
        < body.index(validation)
        < body.index(first_dispatch_derivation)
        < body.index(switch_dispatch)
    )


def test_handle_dir_window_dispatch_fails_closed_before_directory_work() -> None:
    source = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    start = source.index("HandleDirWindow(")
    body = source[start:]
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.directory-window-action-dispatch"))'
    )
    early_return = "return ESC;"
    boundary_calls = [
        'DEBUG_LOG("HandleDirWindow:',
        "Layout_Recalculate(",
        "DisplayMenu(",
        "ctx->focused_window =",
        "SyncActivePanelWindows(",
        "ctx->preview_mode = FALSE;",
        "BuildDirEntryList(",
        "RefreshView(",
        "GetEventOrKey(",
        "GetKeyAction(",
    ]

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert body.index(validation) < body.index(early_return)
    for call in boundary_calls:
        assert body.index(validation) < body.index(call)
        assert body.index(early_return) < body.index(call)


def test_handle_dir_window_revalidates_decoded_action_before_directory_dispatch() -> None:
    source = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    start = source.index("HandleDirWindow(")
    body = source[start:]
    get_key_action = "action = GetKeyAction(ctx, ch);"
    validation = "action = AppStateValidatedKeyAction(action);"
    debug_boundary = 'DebugLogDirLoopState("before_dispatch"'
    switch_dispatch = "switch (action)"

    assert 'include "ytnova_appstate_actions.h"' in source
    assert validation in body
    assert (
        body.index(get_key_action)
        < body.index(validation)
        < body.index(debug_boundary)
        < body.index(switch_dispatch)
    )


def test_panel_anchor_rebind_fails_closed_before_anchor_state_work() -> None:
    source = Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.panel-anchor-rebind"))'
    )
    event_validation = (
        'if (!AppStateValidatedEvent("event.rebuild-rebind-callback"))'
    )
    early_return = "return;"

    assert 'include "ytnova_appstate_actions.h"' in source

    restore_start = source.index("void RestorePanelAnchorPath(")
    restore_end = source.index("\nstatic void FreePanelVolumeFileState(", restore_start)
    restore_body = source[restore_start:restore_end]
    restore_boundary_calls = [
        "CapturePanelViewportSnapshot(",
        "ResolvePanelAnchorTarget(",
        "RestorePanelViewportSnapshot(",
        "PositionPanelAtIndex(",
        "panel->file_dir_entry =",
    ]

    assert validation in restore_body
    assert event_validation in restore_body
    validation_idx = restore_body.index(validation)
    event_validation_idx = restore_body.index(event_validation)
    early_return_idx = restore_body.index(early_return, event_validation_idx)
    assert validation_idx < event_validation_idx < early_return_idx
    for call in restore_boundary_calls:
        assert event_validation_idx < restore_body.index(call)
        assert early_return_idx < restore_body.index(call)

    ensure_start = source.index("void EnsurePanelAnchorVisible(")
    ensure_end = source.index("\nvoid DebugLogDirLoopState(", ensure_start)
    ensure_body = source[ensure_start:ensure_end]
    ensure_boundary_calls = [
        "FindDirByPathInTree(",
        "FindDirByPathOrAncestor(",
        "BuildDirEntryList(",
        "ResolvePanelAnchorTarget(",
        "PositionPanelAtIndex(",
        "panel->file_dir_entry =",
    ]

    assert validation in ensure_body
    assert event_validation in ensure_body
    validation_idx = ensure_body.index(validation)
    event_validation_idx = ensure_body.index(event_validation)
    early_return_idx = ensure_body.index(early_return, event_validation_idx)
    assert validation_idx < event_validation_idx < early_return_idx
    for call in ensure_boundary_calls:
        assert event_validation_idx < ensure_body.index(call)
        assert early_return_idx < ensure_body.index(call)


def test_refresh_tree_safe_fails_closed_before_tree_refresh_work() -> None:
    source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.refresh-rebuild-rebind"))'
    )
    event_validation = 'if (!AppStateValidatedEvent("event.refresh-rebuild"))'

    assert 'include "ytnova_appstate_actions.h"' in source

    start = source.index("DirEntry *RefreshTreeSafe(")
    end = source.index("\nint RefreshDirWindow(", start)
    body = source[start:end]
    boundary_calls = [
        "CapturePanelViewportSnapshot(",
        "InvalidateVolumePanels(",
        "RescanDir(",
        "BuildDirEntryList(",
        "RestorePanelViewportSnapshot(",
        "DisplayTree(",
        "DisplayFileWindow(",
    ]

    assert validation in body
    validation_idx = body.index(validation)
    assert event_validation in body
    event_validation_idx = body.index(event_validation, validation_idx)
    dispatch_return_idx = body.index("return entry;", validation_idx)
    event_return_idx = body.index("return entry;", event_validation_idx)
    assert validation_idx < dispatch_return_idx < event_validation_idx
    assert event_validation_idx < event_return_idx
    for call in boundary_calls:
        assert event_return_idx < body.index(call, event_return_idx)


def test_appstate_shim_lookup_fails_closed_through_runtime_metadata() -> None:
    header = Path("include/ytnova_appstate_actions.h").read_text(encoding="utf-8")
    source = Path("src/core/appstate_actions.c").read_text(encoding="utf-8")
    validation = "int AppStateValidatedCompatibilityShim(const char *shim_id)"
    body_start = source.index("static int AppStateValidateCompatibilityShim(")
    body_end = source.index("\nconst AppStateEventCoverageMetadata", body_start)
    body = source[body_start:body_end]

    assert validation in header
    assert "static int AppStateValidateCompatibilityShim(" in source
    assert "AppStateCompatibilityShimLookup(shim_id)" in body
    assert "AppStateTransitionLookup(metadata->target_transition)" in body
    assert "AppStateValidateTransition(metadata->target_transition, transition)" in body
    assert "metadata->write_capability" in body
    assert '"read_only_projection"' in body
    assert "AppStateTransitionFieldsRegistered(metadata->owner_field_refs" in body
    assert "strcmp(metadata->id, shim_id)" in body


def test_compatibility_shim_boundaries_fail_closed_before_legacy_writes() -> None:
    dir_ops = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    ctrl_dir = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    ctrl_file = Path("src/ui/ctrl_file.c").read_text(encoding="utf-8")
    panel_anchor = Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")
    display = Path("src/ui/display.c").read_text(encoding="utf-8")

    assert 'include "ytnova_appstate_actions.h"' in dir_ops
    assert 'include "ytnova_appstate_actions.h"' in ctrl_dir
    assert 'include "ytnova_appstate_actions.h"' in ctrl_file
    assert 'include "ytnova_appstate_actions.h"' in panel_anchor
    assert 'include "ytnova_appstate_actions.h"' in display

    toggle_start = dir_ops.index("void ToggleDotFiles(")
    toggle_end = dir_ops.index("\nDirEntry *RefreshTreeSafe(", toggle_start)
    toggle_body = dir_ops[toggle_start:toggle_end]
    hidden_validation = (
        'if (!AppStateValidatedCompatibilityShim("shim.viewcontext-hide-dot-files"))'
    )
    assert hidden_validation in toggle_body
    assert toggle_body.index(hidden_validation) < toggle_body.index("p->hide_dot_files =")
    assert toggle_body.index(hidden_validation) < toggle_body.index("ctx->hide_dot_files =")

    volume_start = dir_ops.index("HandleDirWindowVolumeAction(")
    volume_end = dir_ops.index("\nint RefreshDirWindow(", volume_start)
    volume_body = dir_ops[volume_start:volume_end]
    volume_validation = (
        'if (!AppStateValidatedCompatibilityShim("shim.volume-saved-tree-index"))'
    )
    assert volume_validation in volume_body
    assert volume_body.index(volume_validation) < volume_body.index(
        "ctx->active->vol->saved_tree_index ="
    )

    restore_start = panel_anchor.index("BOOL RestorePanelViewportSnapshot(")
    restore_end = panel_anchor.index("\nvoid RestorePanelAnchorPath(", restore_start)
    restore_body = panel_anchor[restore_start:restore_end]
    assert volume_validation in restore_body
    assert restore_body.index(volume_validation) < restore_body.index(
        "ResolvePanelAnchorTarget("
    )
    assert restore_body.index(volume_validation) < restore_body.index(
        "panel->disp_begin_pos ="
    )

    focus_validation = (
        'if (!AppStateValidatedCompatibilityShim("shim.focused-window-session-flag"))'
    )
    dir_start = ctrl_dir.index("HandleDirWindow(")
    dir_body = ctrl_dir[dir_start:]
    assert focus_validation in dir_body
    assert dir_body.index(focus_validation) < dir_body.index("ctx->focused_window =")

    file_start = ctrl_file.index("int HandleFileWindow(")
    file_body = ctrl_file[file_start:]
    assert focus_validation in file_body
    assert file_body.index(focus_validation) < file_body.index("ctx->focused_window =")

    refresh_start = display.index("void RefreshView(")
    refresh_body = display[refresh_start:]
    render_validation = (
        'if (!AppStateValidatedCompatibilityShim("shim-render-derived-row-position"))'
    )
    assert render_validation in refresh_body
    assert refresh_body.index(render_validation) < refresh_body.index("Layout_Recalculate(")
    assert refresh_body.index(render_validation) < refresh_body.index("DisplayTree(")


def test_directory_mutation_result_handlers_fail_closed_before_commit_work() -> None:
    source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    validation = (
        'if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))'
    )
    event_validation = (
        'if (!AppStateValidatedEvent("event.filesystem-mutation-result"))'
    )

    assert 'include "ytnova_appstate_actions.h"' in source

    make_file_start = source.index("BOOL HandleDirMakeFile(")
    make_dir_start = source.index("void HandleDirMakeDirectory(", make_file_start)
    make_file_body = source[make_file_start:make_dir_start]
    assert validation in make_file_body
    validation_idx = make_file_body.index(validation)
    assert event_validation in make_file_body
    event_validation_idx = make_file_body.index(event_validation, validation_idx)
    surface_return_idx = make_file_body.index("return FALSE;", validation_idx)
    event_return_idx = make_file_body.index("return FALSE;", event_validation_idx)
    assert validation_idx < surface_return_idx < event_validation_idx < event_return_idx
    for call in ["ClearHelp(ctx);", "MakeFile(ctx,", "DisplayFileWindow(", "RefreshView(", "MESSAGE("]:
        assert event_return_idx < make_file_body.index(call)

    delete_dir_start = source.index("DirEntry *HandleDirDeleteDirectory(", make_dir_start)
    make_dir_body = source[make_dir_start:delete_dir_start]
    assert validation in make_dir_body
    validation_idx = make_dir_body.index(validation)
    assert event_validation in make_dir_body
    event_validation_idx = make_dir_body.index(event_validation, validation_idx)
    surface_return_idx = make_dir_body.index("return;", validation_idx)
    event_return_idx = make_dir_body.index("return;", event_validation_idx)
    assert validation_idx < surface_return_idx < event_validation_idx < event_return_idx
    for call in [
        'DebugLogSplitState("HandleDirMakeDirectory:entry", ctx);',
        "GetPath(",
        "ClearHelp(ctx);",
        "CaptureInactiveFallback(",
        "MakeDirectory(ctx,",
        "volume_generation++",
        "BuildDirEntryList(",
        "ReanchorPanelToDir(",
        "RefreshView(",
        "wmove(ctx->ctx_border_window",
    ]:
        assert event_return_idx < make_dir_body.index(call)

    rename_dir_start = source.index("DirEntry *HandleDirRenameDirectory(", delete_dir_start)
    delete_dir_body = source[delete_dir_start:rename_dir_start]
    assert validation in delete_dir_body
    validation_idx = delete_dir_body.index(validation)
    assert event_validation in delete_dir_body
    event_validation_idx = delete_dir_body.index(event_validation, validation_idx)
    surface_return_idx = delete_dir_body.index("return dir_entry;", validation_idx)
    event_return_idx = delete_dir_body.index("return dir_entry;", event_validation_idx)
    assert validation_idx < surface_return_idx < event_validation_idx < event_return_idx
    for call in [
        "CaptureInactiveFallbackSnapshot(",
        "CapturePanelViewportSnapshot(",
        "DeleteDirectory(ctx,",
        "volume_generation++",
        "BuildDirEntryList(",
        "RestorePanelViewportSnapshot(",
        "ReanchorPanelToDir(",
        "RefreshView(",
    ]:
        assert event_return_idx < delete_dir_body.index(call)

    show_all_start = source.index("void HandleShowAll(", rename_dir_start)
    rename_dir_body = source[rename_dir_start:show_all_start]
    assert validation in rename_dir_body
    validation_idx = rename_dir_body.index(validation)
    assert event_validation in rename_dir_body
    event_validation_idx = rename_dir_body.index(event_validation, validation_idx)
    surface_return_idx = rename_dir_body.index("return dir_entry;", validation_idx)
    event_return_idx = rename_dir_body.index("return dir_entry;", event_validation_idx)
    assert validation_idx < surface_return_idx < event_validation_idx < event_return_idx
    for call in [
        "GetRenameParameter(",
        "RenameDirectory(ctx,",
        "volume_generation++",
        "BuildDirEntryList(",
        "RefreshView(",
    ]:
        assert event_return_idx < rename_dir_body.index(call)


def test_select_loaded_volume_validates_volume_surfaces_before_menu_work() -> None:
    source = Path("src/ui/volume_menu.c").read_text(encoding="utf-8")
    start = source.index("int SelectLoadedVolume(")
    body = source[start:]
    validations = [
        'if (!AppStateValidatedDispatchSurface("surface.volume-menu-selection"))',
        'if (!AppStateValidatedDispatchSurface("surface.volume-operation"))',
    ]
    event_validation = 'if (!AppStateValidatedEvent("event.volume-lifecycle"))'
    boundary_calls = [
        "ClearHelp(ctx);",
        "xmalloc(",
        "newwin(",
        "LogDisk(",
        "Volume_Delete(",
        "BuildDirEntryList(",
        "EnsurePanelsReferenceActiveVolume(",
        "*return_key =",
    ]

    assert 'include "ytnova_appstate_actions.h"' in source
    validation_positions = []
    for validation in validations:
        validation_idx = body.index(validation)
        early_return_idx = body.index("return -1;", validation_idx)
        validation_positions.append(early_return_idx)

        assert validation_idx < early_return_idx
        for call in boundary_calls:
            assert validation_idx < body.index(call)
            assert early_return_idx < body.index(call)

    event_validation_idx = body.index(event_validation)
    event_return_idx = body.index("return -1;", event_validation_idx)
    assert max(validation_positions) < event_validation_idx < event_return_idx
    for call in boundary_calls:
        assert event_validation_idx < body.index(call)
        assert event_return_idx < body.index(call)


def test_runtime_event_boundary_validation_requires_coverage_and_transition(
    tmp_path: Path,
) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "event_validation_probe.c"
    binary = tmp_path / "event_validation_probe"
    probe.write_text(
        """
#include "src/core/appstate_actions.c"

int main(void) {
  AppStateEventCoverageMetadata mismatched_event;
  AppStateEventCoverageMetadata mismatched_owner;
  AppStateEventCoverageMetadata invalid_write_set;
  AppStateEventCoverageMetadata missing_transition;

  if (!AppStateValidatedEvent("event.terminal-resize-signal"))
    return 1;
  if (!AppStateValidatedEvent("event.watcher-live-refresh"))
    return 2;
  if (AppStateValidatedEvent(NULL))
    return 3;
  if (AppStateValidatedEvent(""))
    return 4;
  if (AppStateValidatedEvent("event.__ytnova_missing__"))
    return 5;
  if (!AppStateValidatedEvent("event.render-reflow"))
    return 6;
  if (!AppStateValidatedEvent("event.refresh-rebuild"))
    return 7;
  if (!AppStateValidatedEvent("event.filesystem-mutation-result"))
    return 8;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return 9;
  if (!AppStateValidatedEvent("event.command-completion"))
    return 12;
  if (!AppStateValidatedEvent("event.volume-lifecycle"))
    return 13;

  mismatched_event =
      *AppStateEventCoverageLookup("event.terminal-resize-signal");
  mismatched_event.event_id = "event.__ytnova_mismatch__";
  if (AppStateValidateEventCoverage("event.terminal-resize-signal",
                                    &mismatched_event))
    return 10;

  missing_transition =
      *AppStateEventCoverageLookup("event.terminal-resize-signal");
  missing_transition.transition_id = "transition.__ytnova_missing__";
  if (AppStateValidateEventCoverage("event.terminal-resize-signal",
                                    &missing_transition))
    return 11;

  mismatched_owner =
      *AppStateEventCoverageLookup("event.terminal-resize-signal");
  mismatched_owner.owner = "window.__ytnova_mismatch__";
  if (AppStateValidateEventCoverage("event.terminal-resize-signal",
                                    &mismatched_owner))
    return 14;

  invalid_write_set =
      *AppStateEventCoverageLookup("event.terminal-resize-signal");
  invalid_write_set.declared_write_set = 0;
  invalid_write_set.declared_write_set_count = 0;
  if (AppStateValidateEventCoverage("event.terminal-resize-signal",
                                    &invalid_write_set))
    return 15;

  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-I.",
            "-Iinclude",
            str(probe),
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


def test_runtime_registry_boundary_validation_requires_registered_metadata(
    tmp_path: Path,
) -> None:
    repo_root = Path(__file__).resolve().parents[1]
    probe = tmp_path / "registry_boundary_validation_probe.c"
    binary = tmp_path / "registry_boundary_validation_probe"
    probe.write_text(
        """
#include "src/core/appstate_actions.c"

int main(void) {
  AppStateTransitionMetadata invalid_transition;
  AppStateOwnerFieldMetadata invalid_owner_field;
  AppStateInvariantMetadata invalid_invariant;
  AppStateGenerationDomainMetadata invalid_generation_domain;
  AppStateDiffHarnessMetadata invalid_diff_harness;
  AppStateTransitionSequenceMetadata invalid_sequence;
  AppStateCompatibilityShimMetadata invalid_shim;
  static const char *const missing_invariant_refs[] = {
      "invariant.__ytnova_missing__",
  };

  if (!AppStateValidatedTransition("transition.keybinding.navigate-tree"))
    return 1;
  if (!AppStateValidatedOwnerField("panel.tree_selection_key"))
    return 2;
  if (!AppStateValidatedInvariant("invariant.inactive-panel-frozen"))
    return 3;
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return 4;
  if (!AppStateValidatedDiffHarness("harness.transition-before-after-snapshot"))
    return 5;
  if (!AppStateValidatedTransitionSequence("sequence.split-toggle-f8"))
    return 6;
  if (!AppStateValidatedCompatibilityShim("shim-render-derived-row-position"))
    return 7;

  if (AppStateValidatedTransition(NULL))
    return 8;
  if (AppStateValidatedOwnerField(""))
    return 9;
  if (AppStateValidatedInvariant("invariant.__ytnova_missing__"))
    return 10;
  if (AppStateValidatedGenerationDomain("generation.__ytnova_missing__"))
    return 11;
  if (AppStateValidatedDiffHarness("harness.__ytnova_missing__"))
    return 12;
  if (AppStateValidatedTransitionSequence("sequence.__ytnova_missing__"))
    return 13;

  invalid_transition =
      *AppStateTransitionLookup("transition.keybinding.navigate-tree");
  invalid_transition.owner = "";
  if (AppStateValidateTransition("transition.keybinding.navigate-tree",
                                 &invalid_transition))
    return 14;

  invalid_owner_field = *AppStateOwnerFieldLookup("panel.tree_selection_key");
  invalid_owner_field.invariant_checks = missing_invariant_refs;
  invalid_owner_field.invariant_check_count = 1;
  if (AppStateValidateOwnerField("panel.tree_selection_key",
                                 &invalid_owner_field))
    return 15;

  invalid_invariant =
      *AppStateInvariantLookup("invariant.inactive-panel-frozen");
  invalid_invariant.transition_ids = NULL;
  invalid_invariant.transition_id_count = 0;
  if (AppStateValidateInvariant("invariant.inactive-panel-frozen",
                                &invalid_invariant))
    return 16;

  invalid_generation_domain =
      *AppStateGenerationDomainLookup("generation.panel.local-authority");
  invalid_generation_domain.generation_owner_field = "panel.__ytnova_missing__";
  if (AppStateValidateGenerationDomain("generation.panel.local-authority",
                                       &invalid_generation_domain))
    return 17;

  invalid_diff_harness =
      *AppStateDiffHarnessLookup("harness.transition-before-after-snapshot");
  invalid_diff_harness.generation_domain_ids = NULL;
  invalid_diff_harness.generation_domain_id_count = 0;
  if (AppStateValidateDiffHarness("harness.transition-before-after-snapshot",
                                  &invalid_diff_harness))
    return 18;

  invalid_sequence =
      *AppStateTransitionSequenceLookup("sequence.split-toggle-f8");
  invalid_sequence.steps = NULL;
  invalid_sequence.step_count = 0;
  if (AppStateValidateTransitionSequence("sequence.split-toggle-f8",
                                         &invalid_sequence))
    return 19;

  invalid_shim =
      *AppStateCompatibilityShimLookup("shim.focused-window-session-flag");
  invalid_shim.write_capability = "read_only_projection";
  if (AppStateValidateCompatibilityShim("shim.focused-window-session-flag",
                                        &invalid_shim))
    return 20;

  return 0;
}
""",
        encoding="utf-8",
    )

    build = subprocess.run(
        [
            "gcc",
            "-std=c99",
            "-I.",
            "-Iinclude",
            str(probe),
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


def test_get_event_or_key_event_boundary_routes_synthetic_events() -> None:
    source = Path("src/ui/key_engine.c").read_text(encoding="utf-8")
    start = source.index("int GetEventOrKey(")
    end = source.index("\nint UI_AskConflict(", start)
    body = source[start:end]

    resize_blocks = re.findall(
        r'if \(ctx && ctx->resize_request\) \{\n(?P<body>.*?\n  })',
        body,
        flags=re.S,
    )
    assert len(resize_blocks) == 3
    for block in resize_blocks:
        compact = re.sub(r"\s+", "", block)
        assert re.search(
            r'AppStateValidatedDispatchSurface\(\s*"surface\.resize-signal-handling"\s*\)',
            block,
        )
        assert re.search(
            r'AppStateValidatedEvent\(\s*"event\.terminal-resize-signal"\s*\)',
            block,
        )
        assert (
            compact.index('AppStateValidatedDispatchSurface("surface.resize-signal-handling")')
            < compact.index('AppStateValidatedEvent("event.terminal-resize-signal")')
            < compact.index("returnKEY_RESIZE;")
        )

    assert re.search(
        r'if \(Watcher_ProcessEvents\(ctx\)\) \{\s+'
        r'if \(!AppStateValidatedDispatchSurface\(\s*"surface\.watcher-live-refresh"\s*\)\)\s+'
        r'return ERR;\s+'
        r'if \(!AppStateValidatedEvent\("event\.watcher-live-refresh"\)\)\s+'
        r"return ERR;\s+return KEY_F\(5\);",
        body,
    )


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
    generation_domains.append(
        _generation_domain(
            "layout_reflow",
            "domain.projection_only_unused",
            ["transition.render_reflow"],
            [],
        )
    )
    generation_domains[-1]["migration_notes"] = [
        "read-only/projection-only fixture domain"
    ]
    paths = _write_fixture(
        tmp_path, transitions=transitions, generation_domains=generation_domains
    )

    failures = _validate(paths)

    assert failures == []


def test_guard_accepts_projection_only_generation_domain_refs_with_split_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    generation_domains = _complete_generation_domains()
    render_domain = next(
        record
        for record in generation_domains
        if record["domain_id"] == "domain.layout_reflow"
    )

    assert "transition.render_reflow" in render_domain["coverage_transition_ids"]
    assert "transition.render_reflow" not in render_domain["advances_on_transition_ids"]

    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        events=events,
        generation_domains=generation_domains,
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


def test_guard_fails_when_runtime_dispatch_surface_validation_callsite_is_missing(
    tmp_path: Path,
) -> None:
    shutil.copytree(REPO_ROOT / "src", tmp_path / "src")
    source_path = tmp_path / "src" / "ui" / "ctrl_file.c"
    original = source_path.read_text(encoding="utf-8")
    mutated = original.replace(
        'AppStateValidatedDispatchSurface("surface.file-window-action-dispatch")',
        'AppStateValidatedDispatchSurface("surface.file-window-action-dispatch-missing")',
        1,
    )
    assert mutated != original
    source_path.write_text(mutated, encoding="utf-8")

    failures = guard.validate_contract(
        guard.DEFAULT_TRANSITIONS,
        guard.DEFAULT_SHIMS,
        guard.DEFAULT_ACTION_COVERAGE,
        guard.DEFAULT_ACTION_HEADER,
        guard.DEFAULT_EVENT_COVERAGE,
        guard.DEFAULT_OWNER_FIELDS,
        guard.DEFAULT_DISPATCH_SURFACES,
        guard.DEFAULT_INVARIANTS,
        guard.DEFAULT_GENERATION_DOMAINS,
        guard.DEFAULT_DIFF_HARNESS,
        guard.DEFAULT_TRANSITION_SEQUENCES,
        guard.DEFAULT_ACTION_RUNTIME,
        repository_root=tmp_path,
    )

    assert any(
        "surface.file-window-action-dispatch" in failure
        and "missing runtime validation callsite" in failure
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


def test_guard_fails_when_dispatch_surface_sequence_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0].pop("transition_sequence_refs")
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "missing required field" in failure
        and "transition_sequence_refs" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        (["sequence.missing"], "references unknown transition sequence"),
        (["sequence.split_toggle_f8", "sequence.split_toggle_f8"], "duplicate transition_sequence_refs"),
        ([123], "transition_sequence_refs[0] must be a non-empty string"),
    ],
)
def test_guard_fails_on_malformed_dispatch_surface_sequence_refs(
    tmp_path: Path, refs: list[object], expected: str
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["transition_sequence_refs"] = refs
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure and expected in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_sequence_misses_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    dispatch_surfaces = _complete_dispatch_surfaces()
    dispatch_surfaces[0]["transition_sequence_refs"] = ["sequence.refresh_rebuild"]
    paths = _write_fixture(
        tmp_path, transitions=transitions, dispatch_surfaces=dispatch_surfaces
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "must include at least one step covering transition_id transition.keybinding"
        in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_sequence_lacks_diff_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    diff_harness_checks = _complete_diff_harness_checks()
    transition_harness = next(
        harness
        for harness in diff_harness_checks
        if harness["harness_id"] == "harness.transition_before_after_snapshot"
    )
    transition_harness["owner_field_refs"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "lacks transition-sequence diff harness coverage" in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_dispatch_surface_sequence_lacks_invariant_coverage(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    inactive_invariant = next(
        invariant
        for invariant in invariants
        if invariant["invariant_id"] == "invariant.inactive_panel_frozen"
    )
    inactive_invariant["protected_fields"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        "dispatch_surface[0]" in failure
        and "lacks transition-sequence invariant coverage" in failure
        and "field" in failure
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


def test_dispatch_surface_records_document_new_write_authority() -> None:
    dispatch_doc, doc_failures = guard._load_json(guard.DEFAULT_DISPATCH_SURFACES)
    runtime_records, runtime_failures = guard._parse_runtime_dispatch_surface_registry(
        guard.DEFAULT_ACTION_RUNTIME
    )

    assert doc_failures == []
    assert runtime_failures == []

    expected_writes = {
        "surface.volume-menu-selection": [
            "ctx.active",
            "panel.volume_key",
            "panel.restore_snapshot",
            "panel.panel_generation",
        ],
        "surface.panel-anchor-rebind": [
            "panel.tree_selection_key",
            "panel.file_selection_key",
            "panel.tree_cursor_pos",
            "panel.tree_viewport_origin",
            "panel.file_viewport_origin",
            "panel.panel_generation",
        ],
    }
    docs_by_id = {
        record["surface_id"]: record for record in dispatch_doc["dispatch_surfaces"]
    }
    runtime_by_id = {record["surface_id"]: record for record in runtime_records}

    for surface_id, writes in expected_writes.items():
        assert docs_by_id[surface_id]["allowed_direct_writes"] == writes
        assert runtime_by_id[surface_id]["allowed_direct_writes"] == writes


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


def test_guard_fails_when_runtime_dispatch_surface_sequence_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_dispatch_surfaces = _complete_dispatch_surfaces()
    runtime_dispatch_surfaces[0]["transition_sequence_refs"] = [
        "sequence.tab_panel_switch"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_dispatch_surfaces=runtime_dispatch_surfaces,
    )

    failures = _validate(paths)

    assert any(
        "runtime_dispatch_surface[0]" in failure
        and "runtime transition_sequence_refs does not match dispatch surface"
        in failure
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
        and "surface.key_decode_input_dispatch" in failure
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
        "runtime_dispatch_surface[0]" in failure
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


def test_guard_fails_when_read_only_projection_shim_owner_field_is_in_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    shim = _shim()
    shim["write_permission"] = "Read-only projection for render calculations."
    shim["write_capability"] = "read_only_projection"
    paths = _write_fixture(tmp_path, transitions=transitions, shims=[shim])

    failures = _validate(paths)

    assert any(
        "shim[0]" in failure
        and "read_only_projection owner_field_refs must stay outside "
        "target_transition write set" in failure
        and "field" in failure
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


def test_guard_fails_when_runtime_read_only_projection_owner_field_is_in_write_set(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_shims = [_shim()]
    runtime_shims[0]["write_permission"] = (
        "Read-only projection for render calculations."
    )
    runtime_shims[0]["write_capability"] = "read_only_projection"
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_shims=runtime_shims,
    )

    failures = _validate(paths)

    assert any(
        "runtime_shim[0]" in failure
        and "read_only_projection owner_field_refs must stay outside "
        "target_transition write set" in failure
        and "field" in failure
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
    runtime_transitions[3]["guard"] = "different guard"
    runtime_transitions[4]["side_effects"] = ["different side effect"]
    runtime_transitions[5]["boundary_status"] = "different status"
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
    assert any(
        "runtime guard does not match transition" in failure for failure in failures
    )
    assert any(
        "runtime side_effects does not match transition" in failure
        for failure in failures
    )
    assert any(
        "runtime boundary_status does not match transition" in failure
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


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "owner_field_refs must be non-empty"),
        ("field", "owner_field_refs must be a non-empty list"),
        ([123], "owner_field_refs[0] must be a non-empty string"),
        (["field", "field"], "duplicate owner_field_refs[1]"),
        (["field.unknown"], "owner_field_refs does not match owner-field registry"),
        (["panel.tree_selection_key"], "must be declared by declared_write_set"),
    ],
)
def test_guard_fails_when_event_coverage_owner_field_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["owner_field_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any("event[0]" in failure and expected in failure for failure in failures)


def test_guard_fails_when_event_coverage_owner_field_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0].pop("owner_field_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "missing required field" in failure
        and "owner_field_refs" in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_owner_field_refs_mix_valid_and_wrong_write(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["owner_field_refs"] = ["field", "panel.tree_selection_key"]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "owner_field_refs[1] must be declared by declared_write_set: panel.tree_selection_key"
        in failure
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


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "owner_field_refs must be non-empty"),
        ("panel.tree_selection_key", "owner_field_refs must be a non-empty list"),
        ([123], "owner_field_refs[0] must be a non-empty string"),
        (
            ["panel.tree_selection_key", "panel.tree_selection_key"],
            "duplicate owner_field_refs[1]",
        ),
        (["field.unknown"], "owner_field_refs does not match owner-field registry"),
        (["field"], "must be declared by declared_write_set"),
    ],
)
def test_guard_fails_when_action_coverage_owner_field_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["owner_field_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any("action[0]" in failure and expected in failure for failure in failures)


def test_guard_fails_when_action_coverage_owner_field_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0].pop("owner_field_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "missing required field" in failure
        and "owner_field_refs" in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_owner_field_refs_miss_declared_write(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["owner_field_refs"] = ["field", "panel.tree_selection_key"]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "owner_field_refs[0] must be declared by declared_write_set: field"
        in failure
        for failure in failures
    )


def test_guard_requires_empty_action_write_set_owner_field_rationale(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["declared_write_set"] = []
    actions[0]["owner_field_refs"] = []
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "empty declared_write_set requires an owner_field_refs rationale"
        in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "transition_sequence_refs must be a non-empty list"),
        ("sequence.split_toggle_f8", "transition_sequence_refs must be a non-empty list"),
        ([123], "transition_sequence_refs[0] must be a non-empty string"),
        (
            ["sequence.split_toggle_f8", "sequence.split_toggle_f8"],
            "duplicate transition_sequence_refs[1]",
        ),
        (
            ["sequence.__missing__"],
            "transition_sequence_refs references unknown transition sequence",
        ),
        (
            ["sequence.refresh_rebuild"],
            "transition_sequence_refs must include at least one step covering transition_id transition.keybinding",
        ),
        (
            ["sequence.split_toggle_f8", "sequence.refresh_rebuild"],
            "transition_sequence_refs[1]",
        ),
    ],
)
def test_guard_fails_when_action_coverage_sequence_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["transition_sequence_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any("action[0]" in failure and expected in failure for failure in failures)


def test_guard_fails_when_action_coverage_dispatch_surface_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0].pop("dispatch_surface_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "missing required field" in failure
        and "dispatch_surface_refs" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "dispatch_surface_refs must be non-empty"),
        ("surface.key_decode_input_dispatch", "dispatch_surface_refs must be a non-empty list"),
        ([123], "dispatch_surface_refs[0] must be a non-empty string"),
        (
            ["surface.key_decode_input_dispatch", "surface.key_decode_input_dispatch"],
            "duplicate dispatch_surface_refs[1]",
        ),
        (["surface.__missing__"], "references unknown dispatch surface"),
        (["surface.menu_modal_completion"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_action_coverage_dispatch_surface_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["dispatch_surface_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any("action[0]" in failure and expected in failure for failure in failures)


def test_guard_fails_when_action_coverage_dispatch_surface_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    transition_id = str(actions[0]["transition_id"])
    valid_ref = str(actions[0]["dispatch_surface_refs"][0])
    wrong_ref = _wrong_dispatch_surface_ref(transition_id)
    actions[0]["dispatch_surface_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and f"dispatch_surface_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "invariant_refs must be non-empty"),
        ("invariant.inactive_panel_frozen", "invariant_refs must be a non-empty list"),
        ([123], "invariant_refs[0] must be a non-empty string"),
        (
            ["invariant.inactive_panel_frozen", "invariant.inactive_panel_frozen"],
            "invariant_refs[1] duplicates invariant.inactive_panel_frozen",
        ),
        (["invariant.__missing__"], "invariant_refs[0] does not match invariant registry"),
        (["invariant.render_projection_read_only"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_action_coverage_invariant_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["invariant_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any("action[0]" in failure and expected in failure for failure in failures)


def test_guard_fails_when_action_coverage_invariant_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    transition_id = str(actions[0]["transition_id"])
    valid_ref = str(actions[0]["invariant_refs"][0])
    wrong_ref = _wrong_invariant_ref(transition_id)
    actions[0]["invariant_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and f"invariant_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_invariant_refs_do_not_cover_declared_writes(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    invariants = _complete_invariants()
    _split_transition_invariant_coverage(
        invariants,
        "transition.keybinding",
        "transition.refresh_rebuild",
        "panel.tree_selection_key",
    )
    paths = _write_fixture(tmp_path, transitions=transitions, invariants=invariants)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "invariant_refs lack collective coverage" in failure
        and "panel.tree_selection_key" in failure
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


@pytest.mark.parametrize(
    ("refs", "expected_fragment"),
    [
        ([], "transition_sequence_refs must be a non-empty list"),
        ("sequence.refresh_rebuild", "transition_sequence_refs must be a non-empty list"),
        ([123], "transition_sequence_refs[0] must be a non-empty string"),
        (
            ["sequence.refresh_rebuild", "sequence.refresh_rebuild"],
            "duplicate transition_sequence_refs[1]",
        ),
        (
            ["sequence.__missing__"],
            "transition_sequence_refs references unknown transition sequence",
        ),
        (
            ["sequence.split_toggle_f8"],
            "transition_sequence_refs must include at least one step covering transition_id",
        ),
        (
            ["sequence.search_jump", "sequence.split_toggle_f8"],
            "transition_sequence_refs[1]",
        ),
    ],
)
def test_guard_fails_when_event_coverage_sequence_refs_are_invalid(
    tmp_path: Path, refs: object, expected_fragment: str
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["transition_sequence_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure and expected_fragment in failure for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected"),
    [
        ([], "invariant_refs must be non-empty"),
        ("invariant.inactive_panel_frozen", "invariant_refs must be a non-empty list"),
        ([123], "invariant_refs[0] must be a non-empty string"),
        (
            ["invariant.render_projection_read_only", "invariant.render_projection_read_only"],
            "invariant_refs[1] duplicates invariant.render_projection_read_only",
        ),
        (["invariant.__missing__"], "invariant_refs[0] does not match invariant registry"),
        (["invariant.inactive_panel_frozen"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_event_coverage_invariant_refs_are_invalid(
    tmp_path: Path, refs: object, expected: str
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    event_index = _event_index("event.render_reflow")
    events[event_index]["invariant_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        f"event[{event_index}]" in failure and expected in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_invariant_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    event_index = _event_index("event.render_reflow")
    transition_id = str(events[event_index]["transition_id"])
    valid_ref = str(events[event_index]["invariant_refs"][0])
    wrong_ref = _wrong_invariant_ref(transition_id)
    events[event_index]["invariant_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        f"event[{event_index}]" in failure
        and f"invariant_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_invariant_refs_do_not_cover_declared_writes(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    event_index = _event_index("event.render_reflow")
    invariants = _complete_invariants()
    _split_transition_invariant_coverage(
        invariants,
        "transition.render_reflow",
        "transition.keybinding",
        "field",
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        events=events,
        invariants=invariants,
    )

    failures = _validate(paths)

    assert any(
        f"event[{event_index}]" in failure
        and "invariant_refs lack collective coverage" in failure
        and "field" in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_dispatch_surface_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0].pop("dispatch_surface_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "missing required field" in failure
        and "dispatch_surface_refs" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected_fragment"),
    [
        ([], "dispatch_surface_refs must be non-empty"),
        ("surface.resize_signal_handling", "dispatch_surface_refs must be a non-empty list"),
        ([123], "dispatch_surface_refs[0] must be a non-empty string"),
        (
            ["surface.resize_signal_handling", "surface.resize_signal_handling"],
            "duplicate dispatch_surface_refs[1]",
        ),
        (["surface.__missing__"], "references unknown dispatch surface"),
        (["surface.menu_modal_completion"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_event_coverage_dispatch_surface_refs_are_invalid(
    tmp_path: Path, refs: object, expected_fragment: str
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["dispatch_surface_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure and expected_fragment in failure for failure in failures
    )


def test_guard_fails_when_event_coverage_dispatch_surface_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    transition_id = str(events[0]["transition_id"])
    valid_ref = str(events[0]["dispatch_surface_refs"][0])
    wrong_ref = _wrong_dispatch_surface_ref(transition_id)
    events[0]["dispatch_surface_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and f"dispatch_surface_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("record_type", "refs", "expected_fragment"),
    [
        ("action", [], "diff_harness_refs must be non-empty"),
        ("action", "harness.transition_before_after_snapshot", "diff_harness_refs must be a non-empty list"),
        ("action", [123], "diff_harness_refs[0] must be a non-empty string"),
        (
            "action",
            [
                "harness.transition_before_after_snapshot",
                "harness.transition_before_after_snapshot",
            ],
            "duplicate diff_harness_refs[1]",
        ),
        ("action", ["harness.__missing__"], "references unknown diff harness id"),
        ("action", ["harness.render_projection_read_only_diff"], "transition_id does not match"),
        ("event", [], "diff_harness_refs must be non-empty"),
        ("event", "harness.generation_mismatch_check", "diff_harness_refs must be a non-empty list"),
        ("event", [123], "diff_harness_refs[0] must be a non-empty string"),
        (
            "event",
            ["harness.generation_mismatch_check", "harness.generation_mismatch_check"],
            "duplicate diff_harness_refs[1]",
        ),
        ("event", ["harness.__missing__"], "references unknown diff harness id"),
        ("event", ["harness.render_projection_read_only_diff"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_coverage_diff_harness_refs_are_invalid(
    tmp_path: Path, record_type: str, refs: object, expected_fragment: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    events = _complete_events()
    if record_type == "action":
        actions[0]["diff_harness_refs"] = refs
    else:
        events[0]["diff_harness_refs"] = refs
    diff_harness_checks = None
    if (
        isinstance(refs, list)
        and len(refs) == 1
        and refs[0] == "harness.render_projection_read_only_diff"
    ):
        record = actions[0] if record_type == "action" else events[0]
        diff_harness_checks = _diff_harness_checks_with_transition_mismatch(
            str(refs[0]), str(record["transition_id"])
        )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        events=events,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    label = "action[0]" if record_type == "action" else "event[0]"
    assert any(
        label in failure and expected_fragment in failure for failure in failures
    )


@pytest.mark.parametrize("record_type", ["action", "event"])
def test_guard_fails_when_coverage_diff_harness_refs_mix_valid_and_wrong_transition(
    tmp_path: Path, record_type: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    events = _complete_events()
    record = actions[0] if record_type == "action" else events[0]
    transition_id = str(record["transition_id"])
    valid_ref = str(record["diff_harness_refs"][0])
    wrong_ref = _wrong_diff_harness_ref(transition_id)
    record["diff_harness_refs"] = [valid_ref, wrong_ref]
    diff_harness_checks = _diff_harness_checks_with_transition_mismatch(
        wrong_ref, transition_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        events=events,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    label = "action[0]" if record_type == "action" else "event[0]"
    assert any(
        label in failure
        and f"diff_harness_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("field_name", "expected_fragment"),
    [
        (
            "owner_field_refs",
            "owner_field_refs[0] lacks referenced diff_harness_refs coverage",
        ),
        (
            "invariant_ids",
            "invariant_refs[0] lacks referenced diff_harness_refs coverage",
        ),
        (
            "generation_domain_ids",
            "generation_domain_refs[0] lacks referenced diff_harness_refs coverage",
        ),
    ],
)
@pytest.mark.parametrize("record_type", ["action", "event"])
def test_guard_fails_when_coverage_diff_harness_refs_lack_overlap_coverage(
    tmp_path: Path,
    record_type: str,
    field_name: str,
    expected_fragment: str,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    events = _complete_events()
    record = actions[0] if record_type == "action" else events[0]
    record_field = {
        "owner_field_refs": "owner_field_refs",
        "invariant_ids": "invariant_refs",
        "generation_domain_ids": "generation_domain_refs",
    }[field_name]
    record_refs = {str(ref) for ref in record[record_field]}
    replacement = [
        candidate
        for candidate in {
            "owner_field_refs": ["field", "panel.tree_selection_key"],
            "invariant_ids": [
                "invariant.blocked_transition_determinism",
                "invariant.inactive_panel_frozen",
                "invariant.render_projection_read_only",
            ],
            "generation_domain_ids": [
                "domain.panel_generation",
                "domain.layout_reflow",
                "domain.volume_generation",
            ],
        }[field_name]
        if candidate not in record_refs
    ][:1]
    assert replacement
    referenced_harnesses = set(record["diff_harness_refs"])
    diff_harness_checks = _complete_diff_harness_checks()
    for harness in diff_harness_checks:
        if harness["harness_id"] in referenced_harnesses:
            harness[field_name] = replacement
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        events=events,
        diff_harness_checks=diff_harness_checks,
    )

    failures = _validate(paths)

    label = "action[0]" if record_type == "action" else "event[0]"
    assert any(
        label in failure and expected_fragment in failure for failure in failures
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


def test_guard_fails_when_runtime_action_coverage_owner_field_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["owner_field_refs"] = ["field"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "owner_field_refs does not match action coverage"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_sequence_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["transition_sequence_refs"] = [
        "sequence.refresh_rebuild"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "runtime transition_sequence_refs does not match action coverage"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_sequence_refs_are_mixed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["transition_sequence_refs"] = [
        "sequence.split_toggle_f8",
        "sequence.refresh_rebuild",
    ]
    runtime_action_coverages = copy.deepcopy(actions)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        actions=actions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "transition_sequence_refs[1]" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_dispatch_surface_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["dispatch_surface_refs"] = ["surface.menu_modal_completion"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "dispatch_surface_refs does not match action coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_dispatch_surface_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    transition_id = str(runtime_action_coverages[0]["transition_id"])
    valid_ref = str(runtime_action_coverages[0]["dispatch_surface_refs"][0])
    wrong_ref = _wrong_dispatch_surface_ref(transition_id)
    runtime_action_coverages[0]["dispatch_surface_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and f"dispatch_surface_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_generation_domain_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0].pop("generation_domain_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and "missing required field" in failure
        and "generation_domain_refs" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected_fragment"),
    [
        ([], "generation_domain_refs must be non-empty"),
        ("domain.panel_generation", "generation_domain_refs must be a non-empty list"),
        ([123], "generation_domain_refs[0] must be a non-empty string"),
        (
            ["domain.panel_generation", "domain.panel_generation"],
            "duplicate generation_domain_refs[1]",
        ),
        (["domain.__missing__"], "references unknown generation domain"),
        (["domain.modal_command_target"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_action_coverage_generation_domain_refs_are_invalid(
    tmp_path: Path, refs: object, expected_fragment: str
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    actions[0]["generation_domain_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure and expected_fragment in failure for failure in failures
    )


def test_guard_fails_when_action_coverage_generation_domain_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions()
    transition_id = str(actions[0]["transition_id"])
    valid_ref = str(actions[0]["generation_domain_refs"][0])
    wrong_ref = _wrong_generation_domain_ref(transition_id)
    actions[0]["generation_domain_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action[0]" in failure
        and f"generation_domain_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_generation_domain_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["generation_domain_refs"] = [
        str(runtime_action_coverages[0]["generation_domain_refs"][0])
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "generation_domain_refs does not match action coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_generation_domain_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    transition_id = str(runtime_action_coverages[0]["transition_id"])
    valid_ref = str(runtime_action_coverages[0]["generation_domain_refs"][0])
    wrong_ref = _wrong_generation_domain_ref(transition_id)
    runtime_action_coverages[0]["generation_domain_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and f"generation_domain_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_diff_harness_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["diff_harness_refs"] = [
        str(runtime_action_coverages[0]["diff_harness_refs"][0])
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "diff_harness_refs does not match action coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_diff_harness_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    transition_id = str(runtime_action_coverages[0]["transition_id"])
    valid_ref = str(runtime_action_coverages[0]["diff_harness_refs"][0])
    wrong_ref = _wrong_diff_harness_ref(transition_id)
    runtime_action_coverages[0]["diff_harness_refs"] = [valid_ref, wrong_ref]
    runtime_diff_harness_checks = _diff_harness_checks_with_transition_mismatch(
        wrong_ref, transition_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and f"diff_harness_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_invariant_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    runtime_action_coverages[0]["invariant_refs"] = [
        "invariant.blocked_transition_determinism"
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and "runtime invariant_refs does not match action coverage" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_action_coverage_invariant_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_action_coverages = _complete_actions()
    transition_id = str(runtime_action_coverages[0]["transition_id"])
    valid_ref = str(runtime_action_coverages[0]["invariant_refs"][0])
    wrong_ref = _wrong_invariant_ref(transition_id)
    runtime_action_coverages[0]["invariant_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_action_coverages=runtime_action_coverages,
    )

    failures = _validate(paths)

    assert any(
        "runtime_action_coverage[0]" in failure
        and f"invariant_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
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


def test_guard_fails_when_runtime_event_coverage_owner_field_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    runtime_events[0]["owner_field_refs"] = ["panel.tree_selection_key"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "owner_field_refs does not match docs"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_dispatch_surface_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    runtime_events[0]["dispatch_surface_refs"] = ["surface.menu_modal_completion"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "dispatch_surface_refs does not match docs" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_dispatch_surface_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    transition_id = str(runtime_events[0]["transition_id"])
    valid_ref = str(runtime_events[0]["dispatch_surface_refs"][0])
    wrong_ref = _wrong_dispatch_surface_ref(transition_id)
    runtime_events[0]["dispatch_surface_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and f"dispatch_surface_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_event_coverage_generation_domain_refs_are_missing(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0].pop("generation_domain_refs")
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and "missing required field" in failure
        and "generation_domain_refs" in failure
        for failure in failures
    )


@pytest.mark.parametrize(
    ("refs", "expected_fragment"),
    [
        ([], "generation_domain_refs must be non-empty"),
        ("domain.panel_generation", "generation_domain_refs must be a non-empty list"),
        ([123], "generation_domain_refs[0] must be a non-empty string"),
        (
            ["domain.panel_generation", "domain.panel_generation"],
            "duplicate generation_domain_refs[1]",
        ),
        (["domain.__missing__"], "references unknown generation domain"),
        (["domain.modal_command_target"], "transition_id does not match"),
    ],
)
def test_guard_fails_when_event_coverage_generation_domain_refs_are_invalid(
    tmp_path: Path, refs: object, expected_fragment: str
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    if refs == ["domain.modal_command_target"]:
        refs = [_wrong_generation_domain_ref(str(events[0]["transition_id"]))]
    events[0]["generation_domain_refs"] = refs
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure and expected_fragment in failure for failure in failures
    )


def test_guard_fails_when_event_coverage_generation_domain_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    transition_id = str(events[0]["transition_id"])
    valid_ref = str(events[0]["generation_domain_refs"][0])
    wrong_ref = _wrong_generation_domain_ref(transition_id)
    events[0]["generation_domain_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(tmp_path, transitions=transitions, events=events)

    failures = _validate(paths)

    assert any(
        "event[0]" in failure
        and f"generation_domain_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_generation_domain_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    event_index = _event_index("event.terminal_resize_signal")
    runtime_events[event_index]["generation_domain_refs"] = [
        str(runtime_events[event_index]["generation_domain_refs"][0])
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        f"runtime_event_coverage[{event_index}]" in failure
        and "generation_domain_refs does not match docs" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_generation_domain_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    transition_id = str(runtime_events[0]["transition_id"])
    valid_ref = str(runtime_events[0]["generation_domain_refs"][0])
    wrong_ref = _wrong_generation_domain_ref(transition_id)
    runtime_events[0]["generation_domain_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and f"generation_domain_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_diff_harness_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    runtime_events[0]["diff_harness_refs"] = [
        str(runtime_events[0]["diff_harness_refs"][0])
    ]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "diff_harness_refs does not match docs" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_diff_harness_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    transition_id = str(runtime_events[0]["transition_id"])
    valid_ref = str(runtime_events[0]["diff_harness_refs"][0])
    wrong_ref = _wrong_diff_harness_ref(transition_id)
    runtime_events[0]["diff_harness_refs"] = [valid_ref, wrong_ref]
    runtime_diff_harness_checks = _diff_harness_checks_with_transition_mismatch(
        wrong_ref, transition_id
    )
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
        runtime_diff_harness_checks=runtime_diff_harness_checks,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and f"diff_harness_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_invariant_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    event_index = _event_index("event.render_reflow")
    runtime_events[event_index]["invariant_refs"] = ["invariant.inactive_panel_frozen"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        f"runtime_event_coverage[{event_index}]" in failure
        and "invariant_refs does not match docs" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_invariant_refs_mix_valid_and_wrong_transition(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    event_index = _event_index("event.render_reflow")
    transition_id = str(runtime_events[event_index]["transition_id"])
    valid_ref = str(runtime_events[event_index]["invariant_refs"][0])
    wrong_ref = _wrong_invariant_ref(transition_id)
    runtime_events[event_index]["invariant_refs"] = [valid_ref, wrong_ref]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        f"runtime_event_coverage[{event_index}]" in failure
        and f"invariant_refs[1] transition_id does not match {transition_id}: {wrong_ref}"
        in failure
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


def test_guard_fails_when_runtime_event_coverage_sequence_refs_drift(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    runtime_events = _complete_events()
    runtime_events[0]["transition_sequence_refs"] = ["sequence.split_toggle_f8"]
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "transition_sequence_refs does not match docs" in failure
        for failure in failures
    )


def test_guard_fails_when_runtime_event_coverage_sequence_refs_are_mixed(
    tmp_path: Path,
) -> None:
    transitions = _complete_transitions()
    events = _complete_events()
    events[0]["transition_sequence_refs"] = [
        "sequence.search_jump",
        "sequence.split_toggle_f8",
    ]
    runtime_events = copy.deepcopy(events)
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        events=events,
        runtime_events=runtime_events,
    )

    failures = _validate(paths)

    assert any(
        "runtime_event_coverage[0]" in failure
        and "transition_sequence_refs[1]" in failure
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
        "WriteSet" in failure
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
    owner_doc, owner_failures = guard._load_json(guard.DEFAULT_OWNER_FIELDS)
    invariant_doc, invariant_failures = guard._load_json(guard.DEFAULT_INVARIANTS)
    runtime_transitions, runtime_transition_failures = guard._parse_runtime_transition_registry(
        runtime_path
    )
    runtime_dispatch_surfaces, runtime_dispatch_surface_failures = (
        guard._parse_runtime_dispatch_surface_registry(runtime_path)
    )
    runtime_events, runtime_event_failures = guard._parse_runtime_event_coverage_registry(
        runtime_path
    )
    runtime_sequences, runtime_sequence_failures = (
        guard._parse_runtime_transition_sequence_registry(runtime_path)
    )
    runtime_generation_domains, runtime_generation_domain_failures = (
        guard._parse_runtime_generation_domain_registry(runtime_path)
    )
    runtime_invariants, runtime_invariant_failures = (
        guard._parse_runtime_invariant_registry(runtime_path)
    )
    runtime_diff_harnesses, runtime_diff_harness_failures = (
        guard._parse_runtime_diff_harness_registry(runtime_path)
    )
    transition_ids = {
        record["id"]: record for record in transitions_doc.get("transitions", [])
    }
    return (
        transition_failures
        + event_failures
        + owner_failures
        + invariant_failures
        + runtime_transition_failures
        + runtime_dispatch_surface_failures
        + runtime_event_failures
        + runtime_sequence_failures
        + runtime_generation_domain_failures
        + runtime_invariant_failures
        + runtime_diff_harness_failures
        + guard._validate_runtime_event_coverage_registry(
            runtime_records=runtime_events,
            runtime_path=runtime_path,
            event_coverage_doc=event_doc,
            transition_ids=transition_ids,
            runtime_transition_sequence_records=runtime_sequences,
            runtime_dispatch_surface_records=runtime_dispatch_surfaces,
            runtime_generation_domain_records=runtime_generation_domains,
            registered_owner_fields=guard._collect_string_ids(
                owner_doc, collection_key="owner_fields", id_field="field"
            ),
            runtime_transition_ids={record["id"] for record in runtime_transitions},
            runtime_invariant_ids={
                record["invariant_id"] for record in runtime_invariants
            },
            runtime_invariant_transition_ids=guard._invariant_transition_ids_by_invariant(
                runtime_invariants
            ),
            runtime_invariant_protected_fields=guard._invariant_protected_fields_by_invariant(
                runtime_invariants
            ),
            runtime_diff_harness_ids={
                record["harness_id"] for record in runtime_diff_harnesses
            },
            runtime_diff_harness_transition_ids=guard._diff_harness_transition_ids_by_harness(
                runtime_diff_harnesses
            ),
            runtime_diff_harness_owner_field_refs=guard._diff_harness_string_refs_by_harness(
                runtime_diff_harnesses,
                "owner_field_refs",
            ),
            runtime_diff_harness_invariant_ids=guard._diff_harness_string_refs_by_harness(
                runtime_diff_harnesses,
                "invariant_ids",
            ),
            runtime_diff_harness_generation_domain_ids=guard._diff_harness_string_refs_by_harness(
                runtime_diff_harnesses,
                "generation_domain_ids",
            ),
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
        "   kAppStateTransitionWriteSet9,\n   sizeof(kAppStateTransitionWriteSet9) / sizeof(kAppStateTransitionWriteSet9[0]),\n   kAppStateEventCoverageOwnerFieldRefs7,\n   sizeof(kAppStateEventCoverageOwnerFieldRefs7) / sizeof(kAppStateEventCoverageOwnerFieldRefs7[0]),\n   \"covered_by_transition_record\",\n   kAppStateEventCoverageTriggerPaths8",
        "   kAppStateTransitionWriteSet0,\n   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),\n   kAppStateEventCoverageOwnerFieldRefs7,\n   sizeof(kAppStateEventCoverageOwnerFieldRefs7) / sizeof(kAppStateEventCoverageOwnerFieldRefs7[0]),\n   \"covered_by_transition_record\",\n   kAppStateEventCoverageTriggerPaths8",
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("declared_write_set does not match transition" in failure for failure in failures)


def test_runtime_event_coverage_detects_dispatch_surface_ref_drift(
    tmp_path: Path,
) -> None:
    runtime_path = _mutated_event_runtime(
        tmp_path,
        'static const char *const kAppStateEventCoverageDispatchSurfaceRefs8[] = {\n  "surface.render-reflow-projection",',
        'static const char *const kAppStateEventCoverageDispatchSurfaceRefs8[] = {\n  "surface.menu-modal-completion",',
    )

    failures = _event_runtime_validation_failures(runtime_path)

    assert any("dispatch_surface_refs does not match docs" in failure for failure in failures)


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
    assert "coverage->transition_sequence_refs" in source
    assert "coverage->transition_sequence_ref_count" in source
    assert "coverage->dispatch_surface_refs" in source
    assert "coverage->dispatch_surface_ref_count" in source
    assert "coverage->owner_field_refs" in source
    assert "coverage->owner_field_ref_count" in source
    assert "AppStateCoverageOwnerFieldRefsReady(" in source
    assert "coverage->invariant_refs" in source
    assert "coverage->invariant_ref_count" in source
    assert "coverage->diff_harness_refs" in source
    assert "coverage->diff_harness_ref_count" in source
    assert "AppStateDiffHarnessRefsReady(" in source
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
    assert "AppStateDispatchSurfaceRefsReady(coverage->dispatch_surface_refs" in action_body
    assert "AppStateDispatchSurfaceRefsReady(coverage->dispatch_surface_refs" in event_body
    assert "AppStateCoverageOwnerFieldRefsReady(" in action_body
    assert "AppStateCoverageOwnerFieldRefsReady(" in event_body
    assert "AppStateInvariantRefsReady(coverage->invariant_refs" in action_body
    assert "AppStateInvariantRefsReady(coverage->invariant_refs" in event_body
    assert "AppStateDiffHarnessRefsReady(" in action_body
    assert "AppStateDiffHarnessRefsReady(" in event_body


def test_runtime_coverage_startup_rejects_owner_field_ref_mismatch() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateCoverageOwnerFieldRefsReady(")
    required_event_start = source.index("static int AppStateRequiredEventClassCovered(")
    helper_body = source[helper_start:required_event_start]

    assert "AppStateOwnerFieldLookup(owner_field) == NULL" in helper_body
    assert re.search(
        r"StringListContains\(owner_field_refs,\s*ref_index,\s*owner_field\)",
        helper_body,
        re.S,
    )
    assert re.search(
        r"!StringListContains\(declared_write_set,\s*"
        r"declared_write_set_count,\s*owner_field\)",
        helper_body,
        re.S,
    )
    assert re.search(
        r"!StringListContains\(owner_field_refs,\s*"
        r"owner_field_ref_count,\s*declared_write_set\[write_index\]\)",
        helper_body,
        re.S,
    )


def test_runtime_coverage_startup_rejects_bad_diff_harness_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateDiffHarnessRefsReady(")
    event_start = source.index("static int AppStateEventCoverageReady(void)")
    helper_body = source[helper_start:event_start]

    assert "AppStateDiffHarnessLookup(diff_harness_refs[ref_index])" in helper_body
    assert re.search(
        r"StringListContains\(harness->transition_ids,\s*"
        r"harness->transition_id_count,\s*transition_id\)",
        helper_body,
        re.S,
    )
    assert re.search(
        r"strcmp\(diff_harness_refs\[previous_index\],\s*"
        r"diff_harness_refs\[ref_index\]\) == 0",
        helper_body,
        re.S,
    )
    assert "harness->owner_field_refs" in helper_body
    assert "harness->invariant_ids" in helper_body
    assert "harness->generation_domain_ids" in helper_body


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


def test_runtime_generation_domain_startup_separates_coverage_from_advances() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    generation_start = source.index("static int AppStateGenerationDomainsReady(void)")
    dispatch_start = source.index(
        "static int AppStateDispatchSurfaceWriteHasInvariantCoverage("
    )
    ready_body = source[generation_start:dispatch_start]

    assert "metadata->coverage_transition_ids" in ready_body
    assert "metadata->coverage_transition_id_count" in ready_body
    assert "metadata->advances_on_transition_ids" in ready_body
    assert re.search(
        r"StringListContains\(metadata->coverage_transition_ids,\s*"
        r"metadata->coverage_transition_id_count,\s*"
        r"metadata->advances_on_transition_ids\s*\[transition_index\]\)",
        ready_body,
        re.S,
    )


def test_runtime_dispatch_surface_startup_checks_fail_closed() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")

    assert "AppStateDispatchSurfaceAt(AppStateDispatchSurfaceCount()) != NULL" in source
    assert 'AppStateDispatchSurfaceLookup("surface.__ytnova_unknown__") != NULL' in source
    assert "AppStateDispatchSurfaceCount() != required_surface_id_count" in source
    assert "previous_index < index" in source
    assert "strcmp(previous->surface_id, metadata->surface_id) == 0" in source
    assert "AppStateDispatchSurfaceSequenceRefsReady(metadata)" in source
    assert "AppStateTransitionSequenceRefsReady(" in source
    assert "metadata->transition_sequence_ref_count" in source
    assert "AppStateTransitionSequenceLookup(refs[ref_index])" in source
    assert (
        "strcmp(refs[previous_index], refs[ref_index]) == 0"
    ) in source
    assert "!AppStateDispatchSurfacesReady()" in source


def test_runtime_action_coverage_startup_validates_sequence_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    action_start = source.index("static int AppStateActionCoverageReady(void)")
    diff_harness_start = source.index(
        "static int AppStateDiffHarnessWriteCovered"
    )
    action_body = source[action_start:diff_harness_start]

    assert "coverage->transition_sequence_refs" in action_body
    assert "coverage->transition_sequence_ref_count" in action_body
    assert "AppStateTransitionSequenceRefsReady(" in action_body
    assert "coverage->diff_harness_refs" in action_body
    assert "coverage->diff_harness_ref_count" in action_body


def test_runtime_action_coverage_startup_rejects_mixed_sequence_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateTransitionSequenceRefsReady(")
    dispatch_start = source.index("static int AppStateDispatchSurfaceSequenceRefsReady(")
    helper_body = source[helper_start:dispatch_start]

    assert re.search(
        r"for \(ref_index = 0; ref_index < ref_count; ref_index\+\+\) \{\s*"
        r"const AppStateTransitionSequenceMetadata \*sequence =\s*"
        r"AppStateTransitionSequenceLookup\(refs\[ref_index\]\);\s*"
        r"int transition_step_found = 0;",
        helper_body,
        re.S,
    )
    assert re.search(
        r"if \(!transition_step_found\)\s*return 0;",
        helper_body,
        re.S,
    )


def test_runtime_action_coverage_startup_rejects_mixed_dispatch_surface_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    action_start = source.index("static int AppStateActionCoverageReady(void)")
    diff_harness_start = source.index(
        "static int AppStateDiffHarnessWriteCovered"
    )
    helper_start = source.index("static int AppStateDispatchSurfaceRefsReady(")
    helper_end = source.index("static int AppStateDispatchSurfaceSequenceRefsReady(")
    action_body = source[action_start:diff_harness_start]
    helper_body = source[helper_start:helper_end]

    assert "coverage->dispatch_surface_refs" in action_body
    assert "coverage->dispatch_surface_ref_count" in action_body
    assert re.search(
        r"for \(ref_index = 0; ref_index < ref_count; ref_index\+\+\) \{\s*"
        r"const AppStateDispatchSurfaceMetadata \*surface =\s*"
        r"AppStateDispatchSurfaceLookup\(refs\[ref_index\]\);",
        helper_body,
        re.S,
    )
    assert re.search(
        r"if \(!NonEmptyString\(surface->transition_id\) \|\|\s*"
        r"strcmp\(surface->transition_id, transition_id\) != 0\)\s*return 0;",
        helper_body,
        re.S,
    )


def test_runtime_event_coverage_startup_rejects_mixed_sequence_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    event_start = source.index("static int AppStateEventCoverageReady(void)")
    action_start = source.index("static int AppStateActionCoverageReady(void)")
    event_body = source[event_start:action_start]

    assert "coverage->transition_sequence_refs" in event_body
    assert "coverage->transition_sequence_ref_count" in event_body
    assert "AppStateTransitionSequenceRefsReady(" in event_body


def test_runtime_event_coverage_startup_rejects_mixed_dispatch_surface_refs() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    event_start = source.index("static int AppStateEventCoverageReady(void)")
    action_start = source.index("static int AppStateActionCoverageReady(void)")
    helper_start = source.index("static int AppStateDispatchSurfaceRefsReady(")
    helper_end = source.index("static int AppStateDispatchSurfaceSequenceRefsReady(")
    event_body = source[event_start:action_start]
    helper_body = source[helper_start:helper_end]

    assert "coverage->dispatch_surface_refs" in event_body
    assert "coverage->dispatch_surface_ref_count" in event_body
    assert re.search(
        r"for \(ref_index = 0; ref_index < ref_count; ref_index\+\+\) \{\s*"
        r"const AppStateDispatchSurfaceMetadata \*surface =\s*"
        r"AppStateDispatchSurfaceLookup\(refs\[ref_index\]\);",
        helper_body,
        re.S,
    )
    assert re.search(
        r"if \(!NonEmptyString\(surface->transition_id\) \|\|\s*"
        r"strcmp\(surface->transition_id, transition_id\) != 0\)\s*return 0;",
        helper_body,
        re.S,
    )


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


def test_runtime_dispatch_surface_startup_requires_sequence_field_coverage() -> None:
    source = Path("src/core/main.c").read_text(encoding="utf-8")
    helper_start = source.index("static int AppStateDispatchSurfaceWritesReady(")
    dispatch_start = source.index("static int AppStateDispatchSurfacesReady(void)")
    helper_body = source[helper_start:dispatch_start]

    assert "AppStateDispatchSurfaceSequenceRefsCoverField(metadata, field)" in helper_body
    assert "AppStateTransitionSequenceStepDiffHarnessCoversField(" in source
    assert "AppStateTransitionSequenceStepInvariantCoversField(" in source
    assert re.search(
        r"StringListContains\(harness->owner_field_refs,\s*"
        r"harness->owner_field_ref_count, field\)",
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
