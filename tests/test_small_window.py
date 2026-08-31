import pytest
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

@pytest.fixture
def test_dir_with_files(tmp_path):
    """Create a test directory with a file."""
    test_root = tmp_path / "test_small_win"
    test_root.mkdir()
    (test_root / "file1.txt").write_text("small")
    return test_root

def test_small_window_transition(test_dir_with_files, ytnova_binary):
    """
    Test SMALLWINDOWSKIP=0 mode transitions through states:
    DIR window → SMALL file window → BIG file window → back to DIR
    """
    # Create .ytnova config
    ytnova_cfg = test_dir_with_files.parent / ".ytnova"
    ytnova_cfg.write_text("[GLOBAL]\nSMALLWINDOWSKIP=0\n")
    
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files.parent))
    assert tui.send_and_wait_for_screen_change(Keys.DOWN)
    
    # STATE 1: DIR window (initial state - already here)
    
    # Transition to SMALL window
    assert tui.send_and_wait_for_screen_change(Keys.ENTER)  # Increase sleep for reliability
    
    screen_small = "\n".join(tui.get_screen_dump())
    
    # Verify SMALL window state
    assert "FILE" in screen_small, "SMALL window should show FILE footer"
    assert "test_small_win" in screen_small, "Dir name should still be visible"
    
    # Transition to BIG window
    assert tui.send_and_wait_for_screen_change(Keys.ENTER)
    
    screen_big = "\n".join(tui.get_screen_dump())
    
    # Verify BIG window state
    assert "FILE" in screen_big, "BIG window should show FILE footer"
    
    # Transition back to DIR window
    assert tui.send_and_wait_for_screen_change(Keys.ENTER)
    
    screen_dir = "\n".join(tui.get_screen_dump())
    
    # Verify back in DIR window (footer should NOT show FILE)
    assert "test_small_win" in screen_dir, "Should be back in DIR view"
    assert "FILE" not in screen_dir, "Should NOT show FILE footer in DIR view"
    
    tui.quit()
