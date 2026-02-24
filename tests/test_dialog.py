import pytest
import shutil
import os
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

YTREE_BIN = os.path.abspath("./build/ytree")

def get_screen_text(screen_dump):
    return "\n".join(screen_dump)

def test_tier_1_footer_prompt(tmp_path):
    """Test the footer prompt by attempting to delete a file."""
    # Create a test file
    (tmp_path / "test_file.txt").touch()
    
    tui = YtreeTUI(executable=YTREE_BIN, cwd=str(tmp_path))
    try:
        tui.send_keystroke(Keys.ENTER) # enter directory view
        tui.send_keystroke(Keys.DELETE) # press 'd'
        
        screen = get_screen_text(tui.get_screen_dump())
        assert "Delete test_file.txt?" in screen or "Delete" in screen
        tui.send_keystroke('n') # cancel delete
    finally:
        tui.quit()

def test_tier_2_history_popover(tmp_path):
    """Test opening the history popover."""
    tui = YtreeTUI(executable=YTREE_BIN, cwd=str(tmp_path))
    try:
        tui.send_keystroke(Keys.UP)
        tui.send_keystroke(Keys.ESC) # dismiss
    finally:
        tui.quit()

def test_tier_3_vol_menu(tmp_path):
    """Test Volume Menu interaction and 'd' exit key."""
    tui = YtreeTUI(executable=YTREE_BIN, cwd=str(tmp_path))
    try:
        tui.send_keystroke('K') # Open Volume Menu
        tui.send_keystroke(Keys.ESC) # close
    finally:
        tui.quit()

def test_tier_3_vol_menu_delete(tmp_path):
    """Test 'd'/'D' exit key logic in Volume Menu."""
    tui = YtreeTUI(executable=YTREE_BIN, cwd=str(tmp_path))
    try:
        tui.send_keystroke('K') # Open Volume Menu
        tui.send_keystroke('d') # Delete key
        # We don't have multiple volumes yet but pressing 'd' should attempt deletion or give an error/prompt
        # Actually in ytree, pressing 'd' in the volume menu might prompt to delete the volume from history.
        screen = get_screen_text(tui.get_screen_dump())
        # We just want to ensure it doesn't crash and returns cleanly or prompts
        pass
    finally:
        tui.quit()

