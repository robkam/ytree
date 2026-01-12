import pytest
import os
import tarfile
from ytree_control import YtreeController

@pytest.fixture(scope="session")
def ytree_binary():
    """Locates the ytree binary in build/ or current directory."""
    paths = ["build/ytree", "./ytree"]
    for p in paths:
        if os.path.exists(p) and os.access(p, os.X_OK):
            return os.path.abspath(p)
    pytest.fail("ytree binary not found. Please run 'make'.")

@pytest.fixture
def sandbox(tmp_path):
    """
    Creates a fresh, isolated filesystem structure for each test.
    Structure:
      sandbox/
        source/
          root_file.txt
          deep/
            nested/
              deep_file.txt
        dest/
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
def archive_sandbox(sandbox):
    """
    Creates a 'source.tar.gz' archive containing the 'source' directory structure.
    Returns the absolute path to the archive file.
    """
    archive_path = sandbox / "source.tar.gz"

    # Create tarball relative to sandbox root so 'source' is the top-level element
    with tarfile.open(archive_path, "w:gz") as tar:
        tar.add(sandbox / "source", arcname="source")

    return str(archive_path)

@pytest.fixture
def controller(ytree_binary, sandbox):
    """
    Factory fixture to create a YtreeController instance.
    The controller runs inside the 'sandbox' directory and sets HOME to that directory.
    """
    def _create(args=None):
        if args is None:
            args = []
        return YtreeController(ytree_binary, str(sandbox))
    return _create