from __future__ import annotations

import importlib.util
import subprocess
import textwrap
from pathlib import Path

import pytest

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_dead_history_comments.py"
GUARD_SPEC = importlib.util.spec_from_file_location(
    "check_dead_history_comments", GUARD_PATH
)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None

guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(textwrap.dedent(content), encoding="utf-8")


@pytest.mark.parametrize(
    "comment",
    [
        "/* Helper used to normalize the active path before rendering. */",
        "/* This helper is used to normalize the active path before rendering. */",
        "/* The cursor is moved to the first visible match after wrapping. */",
        "/* The selected entry is moved to the active panel after reopening. */",
        "/* Ignore obsolete cache entries emitted by the current scanner. */",
        "/* Ignore obsolete state files emitted by the current scanner. */",
        "/* The panel removed from the layout keeps its cached state for redraw. */",
    ],
)
def test_present_day_rationale_comments_with_ambiguous_phrases_are_allowed(
    tmp_path: Path, comment: str
) -> None:
    repo_root = tmp_path
    path = repo_root / "src" / "demo.c"
    _write(
        path,
        f"""\
        int demo(void) {{
            {comment}
            return 0;
        }}
        """,
    )

    assert guard.check_path(path, repo_root) == []


@pytest.mark.parametrize(
    "comment",
    [
        "/* This helper used to rebuild the old footer buffer. */",
        "/* GetSortPrompt moved to prompts.c during the controller split. */",
        "/* Removed obsolete link/unlink fallback after the earlier archive flow. */",
    ],
)
def test_clear_dead_history_comments_are_rejected(tmp_path: Path, comment: str) -> None:
    repo_root = tmp_path
    path = repo_root / "src" / "demo.c"
    _write(
        path,
        f"""\
        int demo(void) {{
            {comment}
            return 0;
        }}
        """,
    )

    failures = guard.check_path(path, repo_root)
    assert failures


@pytest.mark.parametrize(
    "comment",
    [
        "/* This helper formerly rebuilt the old footer buffer. */",
        "/* Original code kept a second prompt path here. */",
        "/* This fallback is no longer used after the old archive flow. */",
        "/* This compatibility shim is now a no-op. */",
    ],
)
def test_strong_explicit_dead_history_markers_are_rejected(
    tmp_path: Path, comment: str
) -> None:
    repo_root = tmp_path
    path = repo_root / "src" / "demo.c"
    _write(
        path,
        f"""\
        int demo(void) {{
            {comment}
            return 0;
        }}
        """,
    )

    failures = guard.check_path(path, repo_root)
    assert failures


@pytest.mark.parametrize(
    ("comment", "snippet"),
    [
        ("# The instruction said to keep the old selector here.", "instruction said"),
        (
            "# For now just keep the old helper; we can comment it out later.",
            "for now just keep",
        ),
    ],
)
def test_instruction_transcript_comment_is_rejected_in_python(
    tmp_path: Path, comment: str, snippet: str
) -> None:
    repo_root = tmp_path
    path = repo_root / "tests" / "demo.py"
    _write(
        path,
        f"""\
        def test_demo() -> None:
            {comment}
            assert True
        """,
    )

    failures = guard.check_path(path, repo_root)
    assert any(snippet in failure.lower() for failure in failures)


def test_commented_out_declaration_is_rejected(tmp_path: Path) -> None:
    repo_root = tmp_path
    path = repo_root / "include" / "demo.h"
    _write(
        path,
        """\
        /* static void OldHelper(void); */
        int demo(void);
        """,
    )

    failures = guard.check_path(path, repo_root)
    assert any("commented-out declaration" in failure for failure in failures)


def test_commented_out_code_block_is_rejected(tmp_path: Path) -> None:
    repo_root = tmp_path
    path = repo_root / "src" / "demo.c"
    _write(
        path,
        """\
        int demo(void) {
            /*
            if (legacy_mode) {
                return OldHelper();
            }
            */
            return 0;
        }
        """,
    )

    failures = guard.check_path(path, repo_root)
    assert any("commented-out code block" in failure for failure in failures)


def test_vendor_header_is_excluded(tmp_path: Path) -> None:
    repo_root = tmp_path
    path = repo_root / "include" / "uthash.h"
    _write(
        path,
        """\
        /* These used to be expressed differently upstream. */
        #define UTHASH_VERSION 1
        """,
    )

    assert guard.check_path(path, repo_root) == []


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_dead_history_comments.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
