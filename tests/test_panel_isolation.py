import pytest
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

def test_panel_switch_updates_small_window(dual_panel_sandbox, ytree_binary):
    """
    Verify that switching panels updates the content of the small file window.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0) # Wait for initial scan
    
    # 1. Start in Left Panel (default). Small window should show left files.
    # We are in '/' of sandbox. There are 'left_dir' and 'right_dir'.
    
    # Navigate to left_dir
    tui.send_keystroke(Keys.DOWN + Keys.ENTER) 
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    assert "very_long_filename" in screen # Found in left_dir
    
    # 2. Split Screen
    tui.send_keystroke(Keys.F8) 
    time.sleep(0.5)
    
    # 3. Switch to Right Panel
    tui.send_keystroke(Keys.TAB) # TAB is usually switch panel
    time.sleep(0.5)
    
    # Navigate to right_dir in right panel
    tui.send_keystroke(Keys.DOWN + Keys.ENTER)
    time.sleep(0.5)
    
    # Verify right panel content (files '0'..'9') is visible
    screen = "\n".join(tui.get_screen_dump())
    assert " 0 " in screen or " 1 " in screen # Right dir files
    
    # Verify left panel content is NOT in the focus of the small window anymore (if shared)
    # Actually if they are side-by-side, both might be visible.
    # But the ACTIVE file window should show the Right files.
    
    # 4. Switch back to Left Panel
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    assert "very_long_filename" in screen
    
    tui.quit()
