import subprocess
from pathlib import Path

import pytest


def _deep_component_path(component_count=257):
    return "/" + "/".join(["seg"] * component_count)


@pytest.fixture(scope="session")
def normpath_fail_safe_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "normpath_fail_safe_driver.c"
    driver_bin = tmp_path_factory.mktemp("normpath_fail_safe") / "normpath_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-Iinclude",
        str(driver_src),
        "src/util/path_utils.c",
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


def test_normpath_rejects_deep_component_stack(normpath_fail_safe_driver):
    deep_path = _deep_component_path(component_count=257)
    completed = subprocess.run(
        [str(normpath_fail_safe_driver), deep_path],
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 1, (
        "NormPath must fail safely when component depth exceeds its internal "
        "stack limit instead of silently truncating.\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )
    assert "NORMPATH_FAIL" in completed.stdout
