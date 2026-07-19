from __future__ import annotations

import importlib.util
import subprocess
import sys
import textwrap
from pathlib import Path

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_clean_code.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_clean_code", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
sys.modules[GUARD_SPEC.name] = guard
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _empty_allowlist() -> dict[str, object]:
    return {
        "single_letter_identifier_exceptions": [],
        "function_size_exceptions": [],
        "long_argument_exceptions": [],
        "flag_argument_exceptions": [],
        "magic_number_exceptions": [],
        "test_fixture_scope_exceptions": [],
        "test_mutable_global_exceptions": [],
    }


def test_guard_rejects_single_letter_parameter_without_exception(tmp_path: Path) -> None:
    _write(
        tmp_path / "src/ui/example.c",
        "int Example(int s) { return s; }\n",
    )

    findings = list(guard.iter_findings(tmp_path, _empty_allowlist()))

    assert any(f.category == "single-letter-identifier" for f in findings)


def test_guard_accepts_allowlisted_single_letter_parameter(tmp_path: Path) -> None:
    _write(
        tmp_path / "src/ui/example.c",
        "int Example(int s) { return s; }\n",
    )
    allowlist = _empty_allowlist()
    allowlist["single_letter_identifier_exceptions"] = [
        {
            "path_regex": r"^src/ui/example\.c$",
            "symbol_regex": r"^Example$",
            "identifiers": ["s"],
        }
    ]

    findings = list(guard.iter_findings(tmp_path, allowlist))

    assert not findings


def test_guard_rejects_long_function_and_long_parameter_list(tmp_path: Path) -> None:
    body = "\n".join(f"    total += {index};" for index in range(220))
    _write(
        tmp_path / "src/ui/example.c",
        (
            "int Example(int a, int b, int c, int d, int e, int f, int g, int h)\n"
            "{\n"
            "    int total = 0;\n"
            f"{body}\n"
            "    return total;\n"
            "}\n"
        ),
    )

    findings = list(guard.iter_findings(tmp_path, _empty_allowlist()))
    categories = {finding.category for finding in findings}

    assert "function-size" in categories
    assert "long-parameter-list" in categories


def test_guard_rejects_flag_argument_and_magic_number_without_exception(
    tmp_path: Path,
) -> None:
    _write(
        tmp_path / "src/ui/example.c",
        textwrap.dedent(
            """\
            int Example(int allow_refresh)
            {
                if (allow_refresh < 10) {
                    return allow_refresh;
                }
                return 0;
            }
            """
        ),
    )

    findings = list(guard.iter_findings(tmp_path, _empty_allowlist()))
    categories = {finding.category for finding in findings}

    assert "flag-argument" in categories
    assert "magic-number" in categories


def test_guard_accepts_allowlisted_flag_argument_and_magic_number(tmp_path: Path) -> None:
    _write(
        tmp_path / "src/ui/example.c",
        textwrap.dedent(
            """\
            int Example(int allow_refresh)
            {
                if (allow_refresh < 10) {
                    return allow_refresh;
                }
                return 0;
            }
            """
        ),
    )
    allowlist = _empty_allowlist()
    allowlist["flag_argument_exceptions"] = [
        {
            "path_regex": r"^src/ui/example\.c$",
            "symbol_regex": r"^Example$",
            "parameters": ["allow_refresh"],
        }
    ]
    allowlist["magic_number_exceptions"] = [
        {
            "path_regex": r"^src/ui/example\.c$",
            "line_regex": r"allow_refresh < 10",
            "literals": ["10"],
        }
    ]

    findings = list(guard.iter_findings(tmp_path, allowlist))

    assert not findings


def test_guard_rejects_session_scoped_test_fixture_without_exception(
    tmp_path: Path,
) -> None:
    _write(
        tmp_path / "tests/test_example.py",
        textwrap.dedent(
            """\
            import pytest

            @pytest.fixture(scope="session")
            def sample_fixture():
                return 1

            def test_example(sample_fixture):
                assert sample_fixture == 1
            """
        ),
    )

    findings = list(guard.iter_findings(tmp_path, _empty_allowlist()))

    assert any(f.category == "test-fixture-scope" for f in findings)


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_clean_code.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    assert run.returncode == 0, run.stdout + run.stderr
