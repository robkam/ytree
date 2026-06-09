from __future__ import annotations

import importlib.util
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
    ("shim", "invariant_checks", "shim[0]"),
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


def _shim(target_transition: str = "transition.keybinding") -> dict[str, object]:
    return {
        "id": "shim.test",
        "owner": "owner",
        "old_authority_path": "legacy.path",
        "read_permission": "read",
        "write_permission": "write",
        "invariant_checks": ["invariant"],
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
        "owner": "YtreePanel(active)",
        "declared_write_set": ["panel.tree_selection_key"],
        "boundary_status": "test",
        "migration_notes": ["fixture coverage"],
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
        "owner": "fixture owner",
        "declared_write_set": ["field"],
        "boundary_status": "test",
        "trigger_paths": ["fixture trigger"],
        "migration_notes": ["fixture coverage"],
    }


def _owner_field(field: str = "field") -> dict[str, object]:
    return {
        "field": field,
        "owner_region": "panel-local state",
        "canonical_owner": "YtreePanel(fixture)",
        "runtime_carrier": "YtreePanel fixture carrier",
        "mutation_rule": "Fixture transitions may mutate only declared fields.",
        "migration_status": "test",
        "invariant_checks": ["fixture invariant"],
    }


def _write_fixture(
    tmp_path: Path,
    *,
    transitions: list[dict[str, object]],
    shims: list[dict[str, object]] | None = None,
    actions: list[dict[str, object]] | None = None,
    events: list[dict[str, object]] | None = None,
    owner_fields: list[dict[str, object]] | None = None,
    required_event_classes: list[str] | None = None,
    enum_actions: list[str] | None = None,
) -> tuple[Path, Path, Path, Path, Path, Path]:
    transitions_path = tmp_path / "transitions.json"
    shims_path = tmp_path / "shims.json"
    action_coverage_path = tmp_path / "action_coverage.json"
    event_coverage_path = tmp_path / "event_coverage.json"
    owner_fields_path = tmp_path / "owner_fields.json"
    actions_header_path = tmp_path / "ytree_defs.h"
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
    _write(actions_header_path, _enum_header(enum_actions or FIXTURE_ACTIONS))
    return (
        transitions_path,
        shims_path,
        action_coverage_path,
        actions_header_path,
        event_coverage_path,
        owner_fields_path,
    )


def _jsonish(value: object) -> str:
    import json

    return json.dumps(value, indent=2)


def _enum_header(actions: list[str]) -> str:
    members = "\n".join(f"  {action}," for action in actions)
    return f"typedef enum {{\n{members}\n}} YtreeAction;\n"


def _complete_transitions() -> list[dict[str, object]]:
    return [
        _transition(category, "transition.keybinding" if category == "keybinding" else None)
        for category in REQUIRED_CATEGORIES
    ]


def _complete_actions() -> list[dict[str, object]]:
    return [_action(action) for action in FIXTURE_ACTIONS]


def _complete_events() -> list[dict[str, object]]:
    return [_event(event_class) for event_class in REQUIRED_EVENT_CLASSES]


def _complete_owner_fields() -> list[dict[str, object]]:
    return [_owner_field("field"), _owner_field("panel.tree_selection_key")]


def _validate(paths: tuple[Path, Path, Path, Path, Path, Path]) -> list[str]:
    return guard.validate_contract(*paths)


def _fixture_with_list_field_value(
    tmp_path: Path, record_type: str, field: str, value: object
) -> tuple[Path, Path, Path, Path, Path, Path]:
    transitions = _complete_transitions()
    shims = [_shim()]
    actions = _complete_actions()
    events = _complete_events()
    owner_fields = _complete_owner_fields()
    if record_type == "action":
        actions[0][field] = value
    elif record_type == "event":
        events[0][field] = value
    elif record_type == "owner_field":
        owner_fields[0][field] = value
    elif record_type == "transition":
        transitions[0][field] = value
    elif record_type == "shim":
        shims[0][field] = value
    else:
        raise AssertionError(f"unknown record type: {record_type}")
    return _write_fixture(
        tmp_path,
        transitions=transitions,
        shims=shims,
        actions=actions,
        events=events,
        owner_fields=owner_fields,
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


def test_guard_fails_when_action_coverage_is_missing_enum_action(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = [_action("ACTION_NONE"), _action("ACTION_USER_CMD")]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "action coverage missing YtreeAction enum member" in failure
        and "ACTION_MOVE_UP" in failure
        for failure in failures
    )


def test_guard_fails_when_action_coverage_has_extra_unknown_action(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    actions = _complete_actions() + [_action("ACTION_NOT_IN_ENUM")]
    paths = _write_fixture(tmp_path, transitions=transitions, actions=actions)

    failures = _validate(paths)

    assert any(
        "unknown YtreeAction enum member" in failure and "ACTION_NOT_IN_ENUM" in failure
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


def test_guard_catches_enum_drift_from_temporary_header(tmp_path: Path) -> None:
    transitions = _complete_transitions()
    paths = _write_fixture(
        tmp_path,
        transitions=transitions,
        enum_actions=FIXTURE_ACTIONS + ["ACTION_NEW_DRIFT"],
    )

    failures = _validate(paths)

    assert any(
        "action coverage missing YtreeAction enum member" in failure
        and "ACTION_NEW_DRIFT" in failure
        for failure in failures
    )
