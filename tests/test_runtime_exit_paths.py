import subprocess
from pathlib import Path

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
