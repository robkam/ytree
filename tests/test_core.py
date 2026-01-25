import pytest
import time
import os
import pexpect
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
    # Updated to match src/cmd/copy.c: "To Directory:"
    yt.child.expect("To Directory")
    yt.input_text("") # Accept current directory

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
    # Updated to match src/cmd/rename.c: "RENAME TO:"
    # Using partial match "RENAME" for robustness as per task instructions
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
    # Updated to match src/cmd/move.c: "To Directory:"
    yt.child.expect("To Directory")
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

def test_path_copy(controller, sandbox):
    """
    Verifies the PathCopy (Y) command.
    Scenario:
        1. Select 'deep_file.txt' (which is inside deep/nested/).
        2. Perform PathCopy.
        3. Target 'dest' folder using ABSOLUTE path.
        4. Verify that the directory structure 'source/deep/nested/' is recreated in 'dest'.
           (Since root of volume is sandbox, file at sandbox/source/deep/nested is relative as source/deep/nested)
    """
    yt = controller()
    yt.wait_for_startup()

    # 1. Select the nested file
    # select_file uses EXPAND_ALL/SHOWALL/FILTER to find the file.
    yt.select_file("deep_file.txt")

    # 2. Initiate PathCopy
    time.sleep(0.5)
    yt.child.send(Keys.PATHCOPY)

    # 3. Verify Prompt 1 (Filename)
    # Expect "PATHCOPY: " or "AS:"
    yt.child.expect("PATHCOPY")

    # Accept default filename
    yt.child.send(Keys.ENTER)

    # 4. Verify Prompt 2 (Destination Dir)
    # Updated to match src/cmd/copy.c: "To Directory:"
    yt.child.expect("To Directory")

    # 5. Input Destination (ABSOLUTE PATH)
    dest_path = sandbox / "dest"
    yt.input_text(str(dest_path))

    # 6. Handle "Create Directory?" Prompt
    # ytree might ask to create the structure.
    try:
        # Check for "Create" prompt or timeout if it proceeds automatically
        index = yt.child.expect([r"[Cc]reate", pexpect.TIMEOUT], timeout=1.0)
        if index == 0:
            # If prompt appeared, confirm Yes
            time.sleep(0.2)
            yt.child.send(Keys.CONFIRM_YES)
    except Exception:
        pass

    # 7. Verify Logic
    # Since we operate from 'sandbox' root, and file is in 'source/deep/nested',
    # PathCopy should append 'source/deep/nested' to 'dest'.
    expected_path = sandbox / "dest" / "source" / "deep" / "nested" / "deep_file.txt"

    # Wait briefly for FS operations to complete
    start = time.time()
    while not expected_path.exists() and time.time() - start < 2.0:
        time.sleep(0.1)

    # Tripartite Verification
    # System Layer: Does file exist?
    assert expected_path.exists(), f"PathCopy failed: Structure not created at {expected_path}"

    # Data Layer: Is content preserved?
    assert expected_path.read_text(encoding="utf-8") == "deep content"

    yt.quit()