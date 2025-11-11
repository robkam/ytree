import pytest
import shutil
import subprocess
from pathlib import Path

@pytest.fixture(scope="session")
def ytree_env(tmp_path_factory):
    """
    Pytest fixture to set up and tear down the test environment for ytree.

    This fixture is session-scoped, meaning it runs once for the entire test
    session, creating the necessary files and directories for all tests to use.

    This fixture performs the following actions:
    1.  **Setup (before tests):**
        - Defines a root test directory inside pytest's temporary directory.
        - Cleans up any artifacts from previous runs (handled by tmp_path_factory).
        - Creates a nested directory structure as per the test plan.
        - Creates various test files with specific content.
        - Sets permissions on one file to be unreadable.
        - Creates a symbolic link.
        - Creates `test.tar.gz` and `test.zip` archives from the test data.

    2.  **Yield:**
        - Yields a `pathlib.Path` object pointing to the root of the test
          directory (`ytree_test`).

    3.  **Teardown (after tests):**
        - pytest's `tmp_path_factory` automatically handles the cleanup of all
          created directories and files.
    """
    # Use pytest's temporary directory factory for robust cleanup
    base_dir = tmp_path_factory.getbasetemp()
    test_root = base_dir / "ytree_test"
    tar_archive = base_dir / "test.tar.gz"
    zip_archive = base_dir / "test.zip"

    # --- Setup Phase ---
    # 1. Create Directory Structure
    d1_1 = test_root / "d1" / "d1_1"
    d2 = test_root / "d2"
    empty_dir = test_root / "empty_dir"
    d1_1.mkdir(parents=True, exist_ok=True)
    d2.mkdir(parents=True, exist_ok=True)
    empty_dir.mkdir(parents=True, exist_ok=True)

    # Create files
    (test_root / "rootfile.txt").write_text("root content")
    (test_root / "d1" / "file_d1.txt").write_text("content d1\n")
    (test_root / "d1" / "d1_1" / "file_d1_1.log").write_text("content d1_1\n")

    # Create unreadable file
    unreadable = d1_1 / "unreadable_file"
    unreadable.touch()
    unreadable.chmod(0o000)

    # Create symbolic link
    (d1_1 / "link_to_rootfile").symlink_to("../../rootfile.txt")

    # 2. Create Archive Files
    # TAR archive
    subprocess.run(
        ["tar", "-czf", str(tar_archive), "d1", "rootfile.txt"],
        cwd=test_root,
        check=True,
        capture_output=True
    )
    # ZIP archive
    subprocess.run(
        ["zip", "-r", str(zip_archive), "d1", "rootfile.txt"],
        cwd=test_root,
        check=True,
        capture_output=True
    )

    # Yield the path to the test environment root
    yield test_root
    
    # Teardown of the unreadable file's permissions to allow automatic cleanup
    if unreadable.exists():
        unreadable.chmod(0o644)
