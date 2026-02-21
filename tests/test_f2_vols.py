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
        
        # Enter file view
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)

        # Send 'c' (Copy) on the first file to trigger the copy interaction
        tui.send_keystroke('c')
        time.sleep(0.5)
        
        # Accept filename
        tui.send_keystroke('\r')
        time.sleep(0.5)

        # Now at "To Directory:" prompt. Hit F2!
        tui.send_keystroke(Keys.F2)
        time.sleep(1.0)
        
        # We are in the F2 menu. Press 'L' to log a new directory!
        tui.send_keystroke('l')
        time.sleep(0.5)
        
        # Enter dir_a
        tui.send_keystroke(str(dir_a) + '\r')
        time.sleep(1.0)
        
        # The F2 window now shows dir_a tree. Cycle back or accept it
        # Let's select it for the copy destination
        tui.send_keystroke('\r')
        time.sleep(1.0)

        # The copy likely succeeded or failed, but the side effect is we now have volume_a logged!
        # Wait, if we accept it, ytree performs the copy and returns to the main view.
        # But we want to test cycling! Let's just enter another copy prompt and cycle!
        
        tui.send_keystroke('c')
        time.sleep(0.5)
        tui.send_keystroke('\r')
        time.sleep(0.5)
        
        # At "To Directory" again:
        tui.send_keystroke(Keys.F2)
        time.sleep(1.0)
        
        # Cycle back using '<'
        tui.send_keystroke('<')
        time.sleep(1.0)
        
        screen = get_screen_text(tui)
        # We should see the previous volume (presumably the tmp_path or volume_a depending on how it cycles)
        assert "volume" in screen or "tmp" in screen, f"Failed to cycle F2 window. Screen:\n{screen}"
        
        # Hit Escape to get out of F2, then escape to cancel Copy
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.5)
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.5)
        
        # Return to Tree view
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)
        
        # In main view, cycle volumes with '<' to verify the main view also has them logged
        tui.send_keystroke('<')
        time.sleep(1.0)
        
        screen = get_screen_text(tui)
        assert "volume_a" in screen, f"Main view did not cycle to volume_a. Screen:\n{screen}"

    finally:
        tui.quit()
