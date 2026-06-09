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
REQUIRED_DISPATCH_SURFACE_CATEGORIES = sorted(guard.REQUIRED_DISPATCH_SURFACE_CATEGORIES)
REQUIRED_INVARIANT_CATEGORIES = sorted(guard.REQUIRED_INVARIANT_CATEGORIES)
REQUIRED_GENERATION_DOMAIN_CATEGORIES = sorted(
    guard.REQUIRED_GENERATION_DOMAIN_CATEGORIES
)
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
    dispatch_surfaces: list[dict[str, object]] | None = None,
    invariants: list[dict[str, object]] | None = None,
    generation_domains: list[dict[str, object]] | None = None,
    required_event_classes: list[str] | None = None,
    enum_actions: list[str] | None = None,
) -> tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path]:
    transitions_path = tmp_path / "transitions.json"
    shims_path = tmp_path / "shims.json"
    action_coverage_path = tmp_path / "action_coverage.json"
    event_coverage_path = tmp_path / "event_coverage.json"
    owner_fields_path = tmp_path / "owner_fields.json"
    dispatch_surfaces_path = tmp_path / "dispatch_surfaces.json"
    invariants_path = tmp_path / "invariants.json"
    generation_domains_path = tmp_path / "generation_domains.json"
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
    _write(actions_header_path, _enum_header(enum_actions or FIXTURE_ACTIONS))
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


def _complete_dispatch_surfaces() -> list[dict[str, object]]:
    return [
        _dispatch_surface(category)
        for category in REQUIRED_DISPATCH_SURFACE_CATEGORIES
    ]


def _complete_invariants() -> list[dict[str, object]]:
    return [_invariant(category) for category in REQUIRED_INVARIANT_CATEGORIES]


def _complete_generation_domains() -> list[dict[str, object]]:
    return [
        _generation_domain(
            category,
            "domain.panel_generation" if category == "panel_generation" else None,
        )
        for category in REQUIRED_GENERATION_DOMAIN_CATEGORIES
    ]


def _complete_owner_fields() -> list[dict[str, object]]:
    return [_owner_field("field"), _owner_field("panel.tree_selection_key")]


def _validate(
    paths: tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path],
) -> list[str]:
    return guard.validate_contract(*paths)


def _fixture_with_list_field_value(
    tmp_path: Path, record_type: str, field: str, value: object
) -> tuple[Path, Path, Path, Path, Path, Path, Path, Path, Path]:
    transitions = _complete_transitions()
    shims = [_shim()]
    actions = _complete_actions()
    events = _complete_events()
    owner_fields = _complete_owner_fields()
    dispatch_surfaces = _complete_dispatch_surfaces()
    invariants = _complete_invariants()
    generation_domains = _complete_generation_domains()
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
