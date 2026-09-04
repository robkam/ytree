from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_test_contract_resilience.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_test_contract_resilience", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
sys.modules[GUARD_SPEC.name] = guard
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def test_scan_records_each_required_brittle_pattern_family(tmp_path: Path) -> None:
    _write(
        tmp_path / "tests" / "test_example.py",
        """import time

def test_example(screen):
    time.sleep(0.1)
    for _ in range(4):
        screen.send_keystroke("down")
    while time.monotonic() < 3:
        pass
    for attempt in range(3):
        pass
    screen.resize(rows=24, cols=80)
    assert screen.lines[2] == "command strip"
    assert 80 == screen.columns
    assert "private branch" in Path("src/ui/example.c").read_text()
    assert "private branch" in _read_source("ignored")
""",
    )

    patterns = {match.pattern_id for match in guard.scan(tmp_path)}

    assert {
        "direct-time-sleep",
        "fixed-navigation-loop",
        "polling-or-retry-loop",
        "screen-slice-or-grid",
        "terminal-geometry",
        "source-read",
        "implementation-string-assertion",
        "exact-prose-assertion",
    } <= patterns


def test_guard_rejects_unreviewed_and_incomplete_rows(tmp_path: Path) -> None:
    _write(tmp_path / "tests" / "test_example.py", "import time\ntime.sleep(0.1)\n")
    document = {"schema_version": guard.SCHEMA_VERSION, "matches": []}

    failures = guard.validate_baseline(document, tmp_path)

    assert any("unreviewed match" in failure for failure in failures)


def test_guard_rejects_blank_exception_accountability_fields(tmp_path: Path) -> None:
    _write(tmp_path / "tests" / "test_example.py", "import time\ntime.sleep(0.1)\n")
    document = guard.build_baseline(tmp_path)
    document["matches"][0]["owner"] = ""
    document["matches"][0]["reason"] = ""
    document["matches"][0]["removal_condition"] = ""

    failures = guard.validate_baseline(document, tmp_path)

    assert any("owner must be non-empty" in failure for failure in failures)
    assert any("reason must be non-empty" in failure for failure in failures)
    assert any("removal_condition must be non-empty" in failure for failure in failures)


def test_guard_rejects_rows_that_do_not_match_scanned_evidence(tmp_path: Path) -> None:
    _write(tmp_path / "tests" / "test_example.py", "import time\ntime.sleep(0.1)\n")
    document = guard.build_baseline(tmp_path)
    document["matches"][0]["evidence"] = "rewritten evidence"

    failures = guard.validate_baseline(document, tmp_path)

    assert any("does not match scanned evidence" in failure for failure in failures)


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_test_contract_resilience.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr


def test_baseline_is_json_object() -> None:
    baseline = Path(__file__).with_name("contract_resilience_baseline.json")
    document = json.loads(baseline.read_text(encoding="utf-8"))

    assert document["schema_version"] == guard.SCHEMA_VERSION
    assert document["contract"] == "Reviewed test-contract exception allowlist for Python tests"


def test_exception_allowlist_rows_identify_their_accountability() -> None:
    baseline = Path(__file__).with_name("contract_resilience_baseline.json")
    document = json.loads(baseline.read_text(encoding="utf-8"))

    for row in document["matches"]:
        assert isinstance(row["path"], str) and row["path"].startswith("tests/")
        assert isinstance(row["pattern_id"], str) and row["pattern_id"]
        assert isinstance(row["reason"], str) and row["reason"].strip()
        assert isinstance(row["owner"], str) and row["owner"].strip()
        assert isinstance(row["removal_condition"], str) and row["removal_condition"].strip()


def test_reviewed_semantic_wait_exceptions_survive_baseline_generation() -> None:
    preview = guard._baseline_row(
        guard.Match(
            "polling-or-retry-loop",
            "tests/test_f7_preview.py",
            "test_f7_file_name_clipping_at_boundaries",
            309,
            "for y in range(BORDER_MIN_Y, BORDER_MAX_Y + 1):",
        )
    )
    harness = guard._baseline_row(
        guard.Match(
            "polling-or-retry-loop",
            "tests/tui_harness.py",
            "wait_for_condition",
            91,
            "while True:",
        )
    )

    assert preview["owner"] == "geometry and presentation remediation"
    assert harness["disposition"] == "retained"
