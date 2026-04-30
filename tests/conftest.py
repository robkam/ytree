import pytest
import os
import tarfile
from ytree_control import YtreeController

@pytest.fixture(scope="session")
def ytree_binary():
    """Locates the ytree binary in build/ or current directory."""
    override = os.environ.get("YTREE_TEST_BIN")
    if override:
        if os.path.exists(override) and os.access(override, os.X_OK):
            return os.path.abspath(override)
        pytest.fail(f"YTREE_TEST_BIN is not executable: {override}")

    paths = ["build/ytree", "./ytree"]
    for p in paths:
        if os.path.exists(p) and os.access(p, os.X_OK):
            return os.path.abspath(p)
    pytest.fail("ytree binary not found. Please run 'make'.")

@pytest.fixture
def sandbox(tmp_path):
    """
    Creates a fresh, isolated filesystem structure for each test.
    """
    root = tmp_path / "sandbox"
    root.mkdir()

    source = root / "source"
    source.mkdir()
    (source / "root_file.txt").write_text("root content", encoding="utf-8")

    deep = source / "deep" / "nested"
    deep.mkdir(parents=True)
    (deep / "deep_file.txt").write_text("deep content", encoding="utf-8")

    dest = root / "dest"
    dest.mkdir()

    return root

@pytest.fixture
def dual_panel_sandbox(tmp_path):
    """
    Creates a specific structure for testing F7/F8 isolation.
    Structure:
      /left_dir/
         very_long_filename_alpha_numeric_extension_test.txt (content: "left")
      /right_dir/
         0 (content: "0")
         1 (content: "1")
         ...
         9 (content: "9")
    """
    root = tmp_path / "dual_panel_root"
    root.mkdir()

    left = root / "left_dir"
    left.mkdir()
    long_name = "very_long_filename_alpha_numeric_extension_test.txt"
    (left / long_name).write_text("left content", encoding="utf-8")

    right = root / "right_dir"
    right.mkdir()
    for i in range(10):
        (right / str(i)).write_text(f"content of {i}", encoding="utf-8")

    return root

@pytest.fixture
def controller(ytree_binary, sandbox):
    """
    Factory fixture to create a YtreeController instance.
    """
    def _create(cwd=None):
        work_dir = cwd if cwd else str(sandbox)
        return YtreeController(ytree_binary, work_dir)
    return _create
