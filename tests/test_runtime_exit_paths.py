import subprocess
from pathlib import Path

import pexpect
import pytest


@pytest.fixture(scope="session")
def runtime_exit_edit_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "runtime_exit_edit_driver.c"
    driver_bin = tmp_path_factory.mktemp("runtime_exit_edit") / "edit_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-Iinclude",
        str(driver_src),
        "src/cmd/edit.c",
        "src/util/path_utils.c",
        "-Wl,--wrap=malloc",
        "-o",
        str(driver_bin),
    ]

    subprocess.run(
        compile_cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return driver_bin


@pytest.fixture(scope="session")
def runtime_exit_normpath_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "runtime_exit_normpath_driver.c"
    driver_bin = tmp_path_factory.mktemp("runtime_exit_normpath") / "normpath_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-Iinclude",
        str(driver_src),
        "src/util/path_utils.c",
        "-Wl,--wrap=strdup",
        "-o",
        str(driver_bin),
    ]

    subprocess.run(
        compile_cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return driver_bin


def test_edit_malloc_failure_returns_control(runtime_exit_edit_driver, tmp_path):
    target_file = tmp_path / "edit-target.txt"
    target_file.write_text("data", encoding="utf-8")

    completed = subprocess.run(
        [str(runtime_exit_edit_driver), str(target_file)],
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 0, (
        "Edit should return control when malloc fails instead of aborting the "
        "process.\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )
    assert "EDIT_RETURN=-1" in completed.stdout


def test_normpath_strdup_failure_returns_control(runtime_exit_normpath_driver):
    completed = subprocess.run(
        [str(runtime_exit_normpath_driver), "./does-not-exist/path"],
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 0, (
        "NormPath should fail safely when strdup fails instead of aborting the "
        "process.\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )
    assert "NORMPATH_EMPTY" in completed.stdout


def test_startup_log_missing_path_exits_without_segv(ytree_binary, tmp_path):
    missing_path = tmp_path / "does-not-exist"
    child = pexpect.spawn(
        ytree_binary,
        args=[str(missing_path)],
        cwd=str(tmp_path),
        env={"TERM": "xterm", "LC_ALL": "C.UTF-8", "HOME": str(tmp_path)},
        encoding="utf-8",
        timeout=5,
        dimensions=(24, 120),
    )

    transcript = ""
    try:
        first = child.expect([r"Can't access", r"AddressSanitizer", pexpect.EOF])
        transcript += child.before or ""
        if isinstance(child.after, str):
            transcript += child.after

        if first == 0:
            child.send("\r")
            child.expect(pexpect.EOF)
            transcript += child.before or ""
    finally:
        child.close(force=True)

    assert "AddressSanitizer" not in transcript, (
        "Logging a missing startup path must not crash with ASan.\n"
        f"Transcript:\n{transcript}"
    )
    assert child.signalstatus is None, (
        "Logging a missing startup path crashed with a fatal signal.\n"
        f"signal={child.signalstatus}, exit={child.exitstatus}\n"
        f"Transcript:\n{transcript}"
    )
    assert child.exitstatus == 1, (
        "Missing startup path should exit cleanly with code 1.\n"
        f"signal={child.signalstatus}, exit={child.exitstatus}\n"
        f"Transcript:\n{transcript}"
    )


def test_scoped_runtime_paths_no_longer_call_exit():
    repo_root = Path(__file__).resolve().parents[1]
    scoped_files = [
        repo_root / "src" / "cmd" / "edit.c",
        repo_root / "src" / "ui" / "ctrl_file_ops.c",
        repo_root / "src" / "util" / "path_utils.c",
    ]

    for scoped_file in scoped_files:
        source = scoped_file.read_text(encoding="utf-8")
        assert "exit(1);" not in source, f"Hard exit remains in {scoped_file}"
