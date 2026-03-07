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
    Test the NOSMALLWINDOW=0 flow:
    - Press ENTER on a dir -> brings up small window overlay
    - Press ENTER again -> brings up big file window
    """
    ytree_cfg = test_dir_with_files.parent / ".ytree"
    ytree_cfg.write_text("NOSMALLWINDOW=0\n")

    tui = YtreeTUI(
        executable=ytree_binary, 
        cwd=str(test_dir_with_files.parent)
    )
    time.sleep(1.0)
    
    # Move down to highlight test_small_win
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.5)
    
    # 1. Press Enter to enter small file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    
    screen_small = "\n".join(tui.get_screen_dump())
    
    # Verify small window is active (directory list is still partially visible, footer says FILE)
    assert "FILE" in screen_small, "Footer should show FILE commands in small window"
    assert "test_small_win" in screen_small, "Directory name should still be visible"
    
    # 2. Press Enter to go to big file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    
    screen_big = "\n".join(tui.get_screen_dump())
    
    # In big window, the directory structure (like ├── or similar volume tree) might vanish, 
    # but the footer should remain "FILE"
    assert "FILE" in screen_big, "Footer should remain FILE in big window"
    
    tui.quit()
