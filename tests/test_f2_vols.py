import pytest
import os
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

YTREE_BIN = "/home/rob/ytree/build/ytree"

def get_screen_text(tui):
    return "\n".join(tui.get_screen_dump())

def test_f2_log_and_cycle_volumes(tmp_path):
    """
    Test that F2 can log new volumes, cycle through them, and update the main views.
    """
    # 1. Setup two distinct directory structures
    dir_a = tmp_path / "volume_a"
    dir_a.mkdir()
    (dir_a / "file_a.txt").touch()

    dir_b = tmp_path / "volume_b"
    dir_b.mkdir()
    (dir_b / "file_b.txt").touch()

    # Start ytree in the base path
    tui = YtreeTUI(executable=YTREE_BIN, cwd=str(tmp_path))
    
    try:
        # Give it a moment to startup
        time.sleep(1.0)
        
        # 2. Log volume_a
        tui.send_keystroke(Keys.F2)
        time.sleep(0.5)
        
        tui.send_keystroke('l') # 'l' for Log
        time.sleep(0.5)
        
        tui.send_keystroke(str(dir_a) + '\r')
        time.sleep(1.0)
        
        # We should now be viewing volume_a
        screen = get_screen_text(tui)
        assert "volume_a" in screen, f"Failed to log and switch to volume_a. Screen:\n{screen}"
        assert "file_a.txt" in screen, f"file_a.txt not visible after logging volume_a. Screen:\n{screen}"

        # 3. Log volume_b
        tui.send_keystroke(Keys.F2)
        time.sleep(0.5)
        
        tui.send_keystroke('l') # log new
        time.sleep(0.5)
        tui.send_keystroke(str(dir_b) + '\r')
        time.sleep(1.0)
        
        screen = get_screen_text(tui)
        assert "volume_b" in screen, f"Failed to log and switch to volume_b. Screen:\n{screen}"
        assert "file_b.txt" in screen, f"file_b.txt not visible after logging volume_b. Screen:\n{screen}"

        # 4. Cycle back to volume_a using F2
        tui.send_keystroke(Keys.F2)
        time.sleep(0.5)
        
        # Test cycling with '<' (previous volume)
        tui.send_keystroke('<')
        time.sleep(0.5)
        
        # The F2 window should now display volume_a context
        screen = get_screen_text(tui)
        assert "volume_a" in screen, f"F2 window did not cycle to volume_a. Screen:\n{screen}"

        # Select it
        tui.send_keystroke(Keys.ENTER)
        time.sleep(1.0)
        
        # 5. Verify the main windows updated to show trees and files for volume_a
        screen = get_screen_text(tui)
        assert "volume_a" in screen, f"Main view did not update to volume_a. Screen:\n{screen}"
        assert "file_a.txt" in screen, f"Main view files did not update to file_a.txt. Screen:\n{screen}"
        assert "file_b.txt" not in screen, f"Main view still incorrectly showing volume_b's files. Screen:\n{screen}"

    finally:
        tui.quit()
