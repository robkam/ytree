from __future__ import annotations

import importlib.util
import subprocess
from pathlib import Path

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_fuzz_harness_sync.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_fuzz_harness_sync", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def test_source_change_without_harness_change_fails() -> None:
    changed_files = {"src/util/string_utils.c"}
    failures = guard.find_missing_fuzz_updates(changed_files)
    assert len(failures) == 1
    assert "src/util/string_utils.c changed without matching fuzz harness update" in failures[0]


def test_source_change_with_matching_harness_change_passes() -> None:
    changed_files = {"src/util/string_utils.c", "tests/fuzz/fuzz_string_utils.c"}
    failures = guard.find_missing_fuzz_updates(changed_files)
    assert failures == []


def test_each_source_requires_its_own_harness_update() -> None:
    changed_files = {
        "src/util/string_utils.c",
        "tests/fuzz/fuzz_string_utils.c",
        "src/util/path_utils.c",
    }
    failures = guard.find_missing_fuzz_updates(changed_files)
    assert len(failures) == 1
    assert "src/util/path_utils.c changed without matching fuzz harness update" in failures[0]


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_fuzz_harness_sync.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
