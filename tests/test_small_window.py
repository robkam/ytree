import pytest
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

@pytest.fixture
def test_dir_with_files(tmp_path):
    """Create a test directory with a file."""
    test_root = tmp_path / "test_small_win"
    test_root.mkdir()
    (test_root / "file1.txt").write_text("small")
    return test_root

def test_small_window_transition(test_dir_with_files, ytree_binary):
    """
    Test NOSMALLWINDOW=0 mode transitions through states:
    DIR window → SMALL file window → BIG file window → back to DIR
    """
    # Create .ytree config
    ytree_cfg = test_dir_with_files.parent / ".ytree"
    ytree_cfg.write_text("[GLOBAL]\nNOSMALLWINDOW=0\n")
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files.parent))
    time.sleep(1.0)
    
    # Navigate to test_small_win directory
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.5)
    
    # STATE 1: DIR window (initial state - already here)
    
    # Transition to SMALL window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.8)  # Increase sleep for reliability
    
    screen_small = "\n".join(tui.get_screen_dump())
    
    # Verify SMALL window state
    assert "FILE" in screen_small, "SMALL window should show FILE footer"
    assert "test_small_win" in screen_small, "Dir name should still be visible"
    
    # Transition to BIG window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.8)
    
    screen_big = "\n".join(tui.get_screen_dump())
    
    # Verify BIG window state
    assert "FILE" in screen_big, "BIG window should show FILE footer"
    
    # Transition back to DIR window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.8)
    
    screen_dir = "\n".join(tui.get_screen_dump())
    
    # Verify back in DIR window (footer should NOT show FILE)
    assert "test_small_win" in screen_dir, "Should be back in DIR view"
    assert "FILE" not in screen_dir, "Should NOT show FILE footer in DIR view"
    
    tui.quit()

