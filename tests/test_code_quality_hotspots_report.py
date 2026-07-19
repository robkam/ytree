from __future__ import annotations

import importlib.util
import subprocess
from pathlib import Path

import pytest

REPORT_PATH = (
    Path(__file__).resolve().parents[1]
    / "scripts"
    / "report_code_quality_hotspots.py"
)
REPORT_SPEC = importlib.util.spec_from_file_location(
    "report_code_quality_hotspots",
    REPORT_PATH,
)
assert REPORT_SPEC is not None and REPORT_SPEC.loader is not None
reporter = importlib.util.module_from_spec(REPORT_SPEC)
REPORT_SPEC.loader.exec_module(reporter)


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


def _write_repo_fixture(tmp_path: Path, *, dir_lines: int, file_lines: int) -> None:
    _write(
        tmp_path / "src/ui/ctrl_dir.c",
        _build_long_function("HandleDirWindow", dir_lines),
    )
    _write(
        tmp_path / "src/ui/ctrl_file.c",
        _build_long_function("HandleFileWindow", file_lines),
    )


@pytest.fixture(autouse=True)
def _guarded_surfaces(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        reporter.guard,
        "CONTROLLER_FILE_LINE_BUDGET",
        {
            "src/ui/ctrl_dir.c": 40,
            "src/ui/ctrl_file.c": 40,
        },
    )
    monkeypatch.setattr(
        reporter.guard,
        "CONTROLLER_GOD_FUNCTION_LINE_BUDGET",
        {
            "src/ui/ctrl_dir.c": {"HandleDirWindow": 30},
            "src/ui/ctrl_file.c": {"HandleFileWindow": 30},
        },
    )


def test_build_snapshot_reports_guarded_file_and_function_hotspots(
    tmp_path: Path,
) -> None:
    _write_repo_fixture(tmp_path, dir_lines=8, file_lines=3)

    snapshot = reporter.build_snapshot(tmp_path)

    assert [row["surface"] for row in snapshot["controller_files"]] == [
        "src/ui/ctrl_dir.c",
        "src/ui/ctrl_file.c",
    ]
    assert [row["surface"] for row in snapshot["controller_functions"]] == [
        "src/ui/ctrl_dir.c:HandleDirWindow",
        "src/ui/ctrl_file.c:HandleFileWindow",
    ]
    assert {row["surface"] for row in snapshot["combined"]} == {
        "src/ui/ctrl_dir.c",
        "src/ui/ctrl_file.c",
        "src/ui/ctrl_dir.c:HandleDirWindow",
        "src/ui/ctrl_file.c:HandleFileWindow",
    }


def test_render_markdown_with_baseline_shows_before_after_delta(
    tmp_path: Path,
) -> None:
    _write_repo_fixture(tmp_path, dir_lines=8, file_lines=3)
    baseline = reporter.build_snapshot(tmp_path)

    _write_repo_fixture(tmp_path, dir_lines=4, file_lines=3)
    current = reporter.build_snapshot(tmp_path)

    markdown = reporter.render_markdown(current, baseline=baseline, top=4)
    baseline_rows = {row["surface"]: row for row in baseline["combined"]}
    current_rows = {row["surface"]: row for row in current["combined"]}

    assert "| Rank | Surface | Metric | Before | After | Delta | Budget | Slack Δ |" in markdown
    for surface in ("src/ui/ctrl_dir.c", "src/ui/ctrl_dir.c:HandleDirWindow"):
        baseline_row = baseline_rows[surface]
        current_row = current_rows[surface]
        assert (
            f"| {surface} | {current_row['metric']} | "
            f"{baseline_row['current']} | {current_row['current']} | "
            f"{current_row['current'] - baseline_row['current']:+d} | "
            f"{current_row['budget']} | "
            f"{current_row['slack'] - baseline_row['slack']:+d} |"
        ) in markdown
    assert "-4" in markdown


def test_current_repository_hotspot_report_runs() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        [
            "python3",
            "scripts/report_code_quality_hotspots.py",
            "--format",
            "markdown",
            "--top",
            "4",
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
    assert "src/ui/ctrl_dir.c" in run.stdout
    assert "HandleFileWindow" in run.stdout
