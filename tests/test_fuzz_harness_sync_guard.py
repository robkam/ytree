from __future__ import annotations

import importlib.util
import subprocess
from pathlib import Path


def _assert_invariant(condition: bool, invariant: str) -> None:
    assert condition, (
        f"{invariant} Runtime execution cannot safely prove that every changed "
        "source file retains its matching fuzz-harness update."
    )


FUZZ_SYNC_INVARIANT = "Fuzz-coverage invariant: the source-to-harness change mapping remains complete."


GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_fuzz_harness_sync.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_fuzz_harness_sync", GUARD_PATH)
_assert_invariant(
    GUARD_SPEC is not None and GUARD_SPEC.loader is not None,
    FUZZ_SYNC_INVARIANT,
)
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def test_source_change_without_harness_change_fails() -> None:
    changed_files = {"src/util/string_utils.c"}
    failures = guard.find_missing_fuzz_updates(changed_files)
    _assert_invariant(len(failures) == 1, FUZZ_SYNC_INVARIANT)
    _assert_invariant(
        "src/util/string_utils.c changed without matching fuzz harness update" in failures[0],
        FUZZ_SYNC_INVARIANT,
    )


def test_source_change_with_matching_harness_change_passes() -> None:
    changed_files = {"src/util/string_utils.c", "tests/fuzz/fuzz_string_utils.c"}
    failures = guard.find_missing_fuzz_updates(changed_files)
    _assert_invariant(failures == [], FUZZ_SYNC_INVARIANT)


def test_each_source_requires_its_own_harness_update() -> None:
    changed_files = {
        "src/util/string_utils.c",
        "tests/fuzz/fuzz_string_utils.c",
        "src/util/path_utils.c",
    }
    failures = guard.find_missing_fuzz_updates(changed_files)
    _assert_invariant(len(failures) == 1, FUZZ_SYNC_INVARIANT)
    _assert_invariant(
        "src/util/path_utils.c changed without matching fuzz harness update" in failures[0],
        FUZZ_SYNC_INVARIANT,
    )


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_fuzz_harness_sync.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    _assert_invariant(run.returncode == 0, FUZZ_SYNC_INVARIANT)


def test_invalid_base_sha_falls_back_to_head_show() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        [
            "python3",
            "scripts/check_fuzz_harness_sync.py",
            "--base",
            "f3f9637177947a0bdafb95540bbc0bf34179a3c0",
            "--head",
            "HEAD",
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    _assert_invariant(run.returncode == 0, FUZZ_SYNC_INVARIANT)
