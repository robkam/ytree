from __future__ import annotations

import importlib.util
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_c_unsafe_apis.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_c_unsafe_apis", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
sys.modules[GUARD_SPEC.name] = guard
GUARD_SPEC.loader.exec_module(guard)


FORMER_LEGACY_SNIPPETS = {
    "src/core/quit.c": 'if (system("stty sane")) {',
    "src/cmd/system.c": 'result = system(command_line);',
    "src/cmd/system.c#shell": 'execl("/bin/sh", "sh", "-c", command_line, (char *)NULL);',
    "src/cmd/print_ops.c": 'out_fp = popen(dest, "w");',
    "src/ui/ctrl_file_ops.c": 'popen(filepath, "w")) == NULL) {',
    "src/ui/fileinfo_git.c": 'pipe_fp = popen(command, "r");',
    "src/ui/render_file.c": 'pipe_fp = popen(command, "r");',
}


RUNTIME_LAUNCH_INVARIANT = (
    "Runtime-launch security invariant: the source tree contains no injection-capable "
    "process-launch API."
)


def _assert_invariant(condition: bool) -> None:
    assert condition, (
        f"{RUNTIME_LAUNCH_INVARIANT} Runtime execution cannot safely prove the global "
        "absence of every forbidden launch path."
    )


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


@pytest.mark.parametrize(
    ("snippet", "expected_call"),
    [
        ('int Demo(void) { return system("ls"); }\n', "system("),
        ('int Demo(void) { return popen("ls", "r") != NULL; }\n', "popen("),
        (
            'int Demo(void) { return execl("/bin/sh", "sh", "-c", "ls", (char *)NULL); }\n',
            "execl(",
        ),
        ('int Demo(void) { return execv(path, argv); }\n', "execv("),
        (
            'int Demo(void) { return posix_spawn(&pid, path, NULL, NULL, argv, envp); }\n',
            "posix_spawn(",
        ),
    ],
)
def test_guard_rejects_runtime_launch_denylist(
    tmp_path: Path, snippet: str, expected_call: str
) -> None:
    _write(tmp_path / "src/ui/example.c", snippet)

    violations = guard.iter_violations(tmp_path)

    _assert_invariant(violations == [("src/ui/example.c", 1, expected_call)])


def test_find_banned_calls_skips_comments_and_strings() -> None:
    findings = guard.find_banned_calls(
        textwrap.dedent(
            '''\
            const char *system_name = "system(";
            // popen(
            /* execl( */
            int ok(void)
            {
                return 0;
            }
            '''
        )
    )

    _assert_invariant(findings == [])


def test_guard_no_longer_grandfathers_runtime_launch_debt() -> None:
    _assert_invariant(guard.ALLOWLIST == {})


def test_guard_rejects_former_legacy_runtime_debt_snippets(tmp_path: Path) -> None:
    _write(
        tmp_path / "src/core/quit.c",
        FORMER_LEGACY_SNIPPETS["src/core/quit.c"] + "\n",
    )
    _write(
        tmp_path / "src/cmd/system.c",
        "\n".join(
            [
                FORMER_LEGACY_SNIPPETS["src/cmd/system.c"],
                FORMER_LEGACY_SNIPPETS["src/cmd/system.c#shell"],
            ]
        )
        + "\n",
    )
    _write(
        tmp_path / "src/cmd/print_ops.c",
        FORMER_LEGACY_SNIPPETS["src/cmd/print_ops.c"] + "\n",
    )
    _write(
        tmp_path / "src/ui/ctrl_file_ops.c",
        FORMER_LEGACY_SNIPPETS["src/ui/ctrl_file_ops.c"] + "\n",
    )
    _write(
        tmp_path / "src/ui/fileinfo_git.c",
        FORMER_LEGACY_SNIPPETS["src/ui/fileinfo_git.c"] + "\n",
    )
    _write(
        tmp_path / "src/ui/render_file.c",
        FORMER_LEGACY_SNIPPETS["src/ui/render_file.c"] + "\n",
    )

    _assert_invariant(guard.iter_violations(tmp_path) == [
        ("src/cmd/print_ops.c", 1, "popen("),
        ("src/cmd/system.c", 1, "system("),
        ("src/cmd/system.c", 2, "execl("),
        ("src/core/quit.c", 1, "system("),
        ("src/ui/ctrl_file_ops.c", 1, "popen("),
        ("src/ui/fileinfo_git.c", 1, "popen("),
        ("src/ui/render_file.c", 1, "popen("),
    ])


def test_guard_rejects_runtime_launch_violation_inside_historic_path(tmp_path: Path) -> None:
    _write(
        tmp_path / "src/cmd/system.c",
        "\n".join(
            [
                FORMER_LEGACY_SNIPPETS["src/cmd/system.c"],
                'result = system("id");',
            ]
        )
        + "\n",
    )

    _assert_invariant(guard.iter_violations(tmp_path) == [
        ("src/cmd/system.c", 1, "system("),
        ("src/cmd/system.c", 2, "system("),
    ])


def test_current_repository_baseline_passes() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    run = subprocess.run(
        ["python3", "scripts/check_c_unsafe_apis.py"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    _assert_invariant(run.returncode == 0)
