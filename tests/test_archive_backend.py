import subprocess
import tarfile
import zipfile
from pathlib import Path

import pytest

UNSUPPORTED_FORMAT_ERROR = -2


@pytest.fixture(scope="session")
def archive_backend_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "archive_backend_driver.c"
    driver_bin = tmp_path_factory.mktemp("archive_backend") / "archive_backend_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-DHAVE_LIBARCHIVE",
        "-Iinclude",
        str(driver_src),
        "src/fs/archive_write.c",
        "-o",
        str(driver_bin),
        "-larchive",
    ]
    subprocess.run(
        compile_cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return driver_bin


def _run_backend(driver, dest_path, source_paths):
    cmd = [str(driver), str(dest_path)] + [str(path) for path in source_paths]
    completed = subprocess.run(cmd, check=True, capture_output=True, text=True)
    return int(completed.stdout.strip())


def test_archive_create_zip_from_sources(archive_backend_driver, tmp_path):
    alpha = tmp_path / "alpha.txt"
    beta = tmp_path / "beta.txt"
    alpha.write_text("alpha\n", encoding="utf-8")
    beta.write_text("beta\n", encoding="utf-8")

    dest = tmp_path / "bundle.zip"
    rc = _run_backend(
        archive_backend_driver,
        dest.resolve(),
        [alpha.resolve(), beta.resolve()],
    )

    assert rc == 0
    with zipfile.ZipFile(dest, "r") as zf:
        names = sorted(zf.namelist())
        assert names == ["alpha.txt", "beta.txt"]
        assert zf.read("alpha.txt") == b"alpha\n"
        assert zf.read("beta.txt") == b"beta\n"


def test_archive_create_tar_gz_from_sources(archive_backend_driver, tmp_path):
    alpha = tmp_path / "alpha.txt"
    beta = tmp_path / "beta.txt"
    alpha.write_text("alpha\n", encoding="utf-8")
    beta.write_text("beta\n", encoding="utf-8")

    dest = tmp_path / "bundle.tar.gz"
    rc = _run_backend(
        archive_backend_driver,
        dest.resolve(),
        [alpha.resolve(), beta.resolve()],
    )

    assert rc == 0
    with tarfile.open(dest, "r:gz") as tf:
        names = sorted(member.name for member in tf.getmembers())
        assert names == ["alpha.txt", "beta.txt"]
        assert tf.extractfile("alpha.txt").read() == b"alpha\n"
        assert tf.extractfile("beta.txt").read() == b"beta\n"


def test_archive_create_unsupported_extension_returns_error(
    archive_backend_driver, tmp_path
):
    alpha = tmp_path / "alpha.txt"
    alpha.write_text("alpha\n", encoding="utf-8")

    dest = tmp_path / "bundle.unsupported"
    rc = _run_backend(
        archive_backend_driver, dest.resolve(), [alpha.resolve()]
    )

    assert rc == UNSUPPORTED_FORMAT_ERROR
    assert not dest.exists()
