import pytest
import time
import os
from ytree_keys import Keys

def test_simple_copy(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Copy
    time.sleep(0.5)
    yt.child.send(Keys.COPY)
    yt.child.expect("COPY")

    # 3. Input New Name
    yt.input_text("copy.txt")

    # 4. Input Destination
    yt.child.expect("TO")
    yt.input_text("")

    # 5. Verify
    assert (sandbox / "source" / "copy.txt").exists()

    yt.quit()

def test_rename(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Rename
    time.sleep(0.5)
    yt.child.send(Keys.RENAME)
    yt.child.expect("RENAME")

    # 3. Input New Name
    yt.input_text("renamed.txt")

    # 4. Verify
    assert (sandbox / "source" / "renamed.txt").exists()
    assert not (sandbox / "source" / "root_file.txt").exists()

    yt.quit()

def test_move(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Move
    time.sleep(0.5)
    yt.child.send(Keys.MOVE)
    yt.child.expect("MOVE")

    # 3. Input New Name
    yt.input_text("moved.txt")

    # 4. Input Destination
    yt.child.expect("TO")
    yt.input_text("../dest")

    # 5. Verify
    assert (sandbox / "dest" / "moved.txt").exists()
    assert not (sandbox / "source" / "root_file.txt").exists()

    yt.quit()

def test_wildcard_rename(controller, sandbox):
    """
    Regression: Rename root_file.txt to new.* (should preserve extension .txt)
    """
    yt = controller()
    yt.wait_for_startup()

    yt.select_file("root_file.txt")

    time.sleep(0.5)
    yt.child.send(Keys.RENAME)
    yt.child.expect("RENAME")
    yt.input_text("new.*")

    expected = sandbox / "source" / "new.txt"
    bad_result = sandbox / "source" / "new.root_file.txt"

    # Poll for file existence
    for _ in range(5):
        if expected.exists(): break
        time.sleep(0.2)

    if bad_result.exists():
        pytest.fail("Regression: 'new.*' resulted in 'new.root_file.txt'")

    assert expected.exists(), "Rename failed (new.txt not found)"
    yt.quit()