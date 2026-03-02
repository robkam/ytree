import pytest
import os
import time
from tui_harness import YtreeTUI

def test_ui_layout_box_drawing(ytree_binary, tmp_path):
    """
    REGRESSION: Verify that box-drawing is active and uses Ncurses graphics (ACS or Unicode).
    We check for the presence of drawing components like borders and tree connectors.
    Since pyte might represent ACS (vt100) characters in their ASCII-equivalent form (like 'lq' or 'mq')
    or as Unicode characters depending on the environment, we check for both.
    """
    # Create a test directory with a sub-directory
    test_dir = tmp_path / "ui_layout_test"
    test_dir.mkdir()
    (test_dir / "new_dir").mkdir()
    
    # Launch ytree using the TUI harness
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))
    
    # Wait for the screen to stabilize and ensure content is loaded
    assert tui.wait_for_content("new_dir", timeout=5.0), "Directory 'new_dir' not found on screen after scan"
    
    screen = tui.get_screen_dump()
    
    # 1. Search for Border Components
    # Top-left corner: 'l' (ACS_ULCORNER), '+' (ASCII), or '┌' (Unicode)
    found_border = False
    for line in screen:
        if any(corner in line for corner in ["lq", "┌─", "+-"]):
            found_border = True
            break
    assert found_border, "UI Top Border not detected using any supported graphics mode"

    # 2. Search for Tree Connectors
    # 'mq' (LLCORNER + HLINE) is the standard vt100 ACS for a tree root/leaf
    # '└─' is the Unicode equivalent
    found_connector = False
    for line in screen:
        if any(conn in line for conn in ["mq", "└─", "+-"]):
            found_connector = True
            break
    assert found_connector, "Tree root/leaf connector not detected using any supported graphics mode"
        
    # 2. Ensure "DIR Attribute" menu item is visible (bottom menu check)
    found_dir_attr = any("DIR Attribute" in line for line in screen)
    assert found_dir_attr, "'DIR Attribute' menu hint not found on screen"
    
    # 3. Ensure FILTER header is present in the stats column area
    found_filter = any("FILTER" in line for line in screen)
    assert found_filter, "'FILTER' header not found in statistics area"

    tui.quit()

def test_ui_layout_dynamic_resizing(ytree_binary, tmp_path):
    """
    Verifies that the UI remains stable during terminal resize.
    """
    test_dir = tmp_path / "resize_test"
    test_dir.mkdir()
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))
    assert tui.wait_for_content("resize_test", timeout=5.0), "Test directory not found on screen"
    
    # Resize to something different
    new_cols, new_lines = 100, 30
    tui.child.setwinsize(new_lines, new_cols)
    tui.screen.resize(new_lines, new_cols)
    
    # Wait for ytree to handle SIGWINCH and redraw
    time.sleep(1.0)
    
    screen = tui.get_screen_dump()
    
    # Basic sanity: Path should still be at top
    found_path = any("Path: " in line for line in screen)
    assert found_path, "Header 'Path: ' lost after resize"
    
    # Commands should still be at bottom
    found_commands = any("COMMANDS" in line for line in screen)
    assert found_commands, "Bottom menu 'COMMANDS' lost after resize"
    
    tui.quit()
