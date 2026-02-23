import pytest
import time
import os
import shutil
import tempfile
from tui_harness import YtreeTUI
from ytree_keys import Keys

@pytest.fixture
def filter_env(ytree_binary):
    test_base_dir = tempfile.mkdtemp(prefix="ytree_filter_")
    for f in ["file1.c", "file2.c", "file3.txt"]:
        with open(os.path.join(test_base_dir, f), "w") as fd:
            fd.write("test")
    yield test_base_dir, ytree_binary
    shutil.rmtree(test_base_dir)

def test_filter_stats_recalculation(filter_env):
    cwd, binary = filter_env
    tui = YtreeTUI(executable=binary, cwd=cwd)
    time.sleep(1.0) # Wait for scan
    
    # Check initial match 3
    screen = "\n".join(tui.get_screen_dump())
    assert "Mat: 3" in screen.replace(" ", "") or "Mat:3" in screen.replace(" ", "")
    
    # Filter for *.c
    tui.send_keystroke(Keys.FILTER)
    time.sleep(0.2)
    # The prompt might already have something, let's clear it
    tui.send_keystroke("\x15") # Ctrl-U
    tui.send_keystroke("*.c\r")
    time.sleep(0.5)
    
    # Check for recalculation to 2
    screen = "\n".join(tui.get_screen_dump())
    assert "Mat: 2" in screen.replace(" ", "") or "Mat:2" in screen.replace(" ", "")
    
    # Verify Global Mode (S) works
    tui.send_keystroke(Keys.SHOWALL)
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    assert "FILE" in screen
    assert "file1.c" in screen
    assert "file2.c" in screen
    assert "file3.txt" not in screen
    
    tui.quit()

def test_show_all_no_matching_files(filter_env):
    cwd, binary = filter_env
    tui = YtreeTUI(executable=binary, cwd=cwd)
    time.sleep(1.0)
    
    # Filter for non-existent
    tui.send_keystroke(Keys.FILTER)
    time.sleep(0.2)
    tui.send_keystroke("\x15")
    tui.send_keystroke("*.java\r")
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    assert "Mat: 0" in screen.replace(" ", "") or "Mat:0" in screen.replace(" ", "")
    
    # Try 'S'
    tui.send_keystroke(Keys.SHOWALL)
    time.sleep(0.5)
    
    # Should stay in DIR view
    screen = "\n".join(tui.get_screen_dump())
    assert "DIR" in screen
    assert "FILE" not in screen
    
    tui.quit()

def test_multi_pattern_filter(ytree_binary, tmp_path):
    """
    REGRESSION: Filter with multiple patterns (e.g. *.c,*.h) fails.
    """
    d = tmp_path / "filter_multi"
    d.mkdir()
    (d / "file1.c").write_text("c")
    (d / "file2.h").write_text("h")
    (d / "file3.txt").write_text("txt")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0) # Wait for scan

    # Apply multi-filter
    tui.send_keystroke(Keys.FILTER)
    time.sleep(0.2)
    tui.send_keystroke("\x15") # Clear line
    tui.send_keystroke("*.c,*.h\r")
    time.sleep(0.5)
    
    # Check stats for 2 files
    screen = "\n".join(tui.get_screen_dump())
    # Use replace(" ", "") because spacing might vary
    assert "Mat:2" in screen.replace(" ", "") or "Mat: 2" in screen.replace(" ", "")
    
    # Verify Global Mode
    tui.send_keystroke(Keys.SHOWALL)
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    assert "file1.c" in screen
    assert "file2.h" in screen
    assert "file3.txt" not in screen

    # Test with extra spaces
    tui.send_keystroke(Keys.FILTER)
    time.sleep(0.2)
    tui.send_keystroke("\x15") # Clear line
    tui.send_keystroke(" *.c , *.h \r")
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    assert "file1.c" in screen
    assert "file2.h" in screen
    
    tui.quit()
