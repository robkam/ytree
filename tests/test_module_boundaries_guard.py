from __future__ import annotations

import importlib.util
import subprocess
import textwrap
from pathlib import Path

import pytest

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_module_boundaries.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_module_boundaries", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _build_long_function(name: str, body_lines: int) -> str:
    body = "\n".join("    x += 1;" for _ in range(body_lines))
    return (
        f"int {name}(void)\n"
        "{\n"
        "    int x = 0;\n"
        f"{body}\n"
        "    return x;\n"
        "}\n"
    )


@pytest.mark.parametrize(
    ("relpath", "guarded_name"),
    [
        ("src/ui/ctrl_dir.c", "HandleDirWindow"),
        ("src/ui/ctrl_file.c", "HandleFileWindow"),
    ],
)
def test_new_controller_top_level_function_is_rejected(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, relpath: str, guarded_name: str
) -> None:
    repo_root = tmp_path
    other_relpath = (
        "src/ui/ctrl_file.c" if relpath.endswith("ctrl_dir.c") else "src/ui/ctrl_dir.c"
    )
    other_guarded = (
        "HandleFileWindow" if guarded_name == "HandleDirWindow" else "HandleDirWindow"
    )

    _write(
        repo_root / relpath,
        textwrap.dedent(
            f"""\
            int ExistingDispatch(void) {{ return 0; }}
            int {guarded_name}(void) {{ return ExistingDispatch(); }}
            int NewlyAddedHelper(void) {{ return 1; }}
            """
        ),
    )
    _write(
        repo_root / other_relpath,
        textwrap.dedent(
            f"""\
            int ExistingDispatch(void) {{ return 0; }}
            int {other_guarded}(void) {{ return ExistingDispatch(); }}
            """
        ),
    )

    monkeypatch.setattr(
        guard,
        "CONTROLLER_TOP_LEVEL_ALLOWLIST",
        {
            "src/ui/ctrl_dir.c": {"ExistingDispatch", "HandleDirWindow"},
            "src/ui/ctrl_file.c": {"ExistingDispatch", "HandleFileWindow"},
        },
    )

    failures = guard.check_controller_allowlists(repo_root)
    assert any("NewlyAddedHelper" in failure for failure in failures)
    assert any(
        "allowlist updates require explicit architecture approval" in failure
        for failure in failures
    )


def test_god_function_growth_is_rejected(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    repo_root = tmp_path
    _write(
        repo_root / "src/ui/ctrl_dir.c",
        _build_long_function("HandleDirWindow", body_lines=8),
    )
    _write(
        repo_root / "src/ui/ctrl_file.c",
        _build_long_function("HandleFileWindow", body_lines=8),
    )

    monkeypatch.setattr(
        guard,
        "CONTROLLER_GOD_FUNCTION_LINE_BUDGET",
        {
            "src/ui/ctrl_dir.c": {"HandleDirWindow": 6},
            "src/ui/ctrl_file.c": {"HandleFileWindow": 6},
        },
    )

    failures = guard.check_controller_god_function_budgets(repo_root)
    assert any("HandleDirWindow line budget exceeded" in failure for failure in failures)
    assert any("HandleFileWindow line budget exceeded" in failure for failure in failures)


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_module_boundaries.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
