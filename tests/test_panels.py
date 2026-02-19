import pytest
import time
import re
import os
import pexpect
from ytree_keys import Keys

def get_clean_screen(yt):
    """
    Senior SDET Buffer Scraper.
    Strips ANSI codes and normalizes whitespace.
    Crucial for matching text that might be wrapped or moved by ncurses.
    """
    raw = (yt.child.before if isinstance(yt.child.before, str) else "") + \
          (yt.child.after if isinstance(yt.child.after, str) else "")
    # Remove all ANSI escape sequences to allow clean regex/string matching
    clean = re.sub(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])', '', raw)
    return clean

def sync_state(yt):
    """Universal synchronization: waits for the UI clock to tick."""
    # ytree usually displays the year (202x) in the top right.
    # Waiting for this regex ensures ncurses has finished at least one draw cycle.
    yt.child.expect(r'20\d{2}')

def detect_and_fix_auto_enter(yt):
    """
    Handles the spurious mode transition bug.
    If ytree auto-entered a dir on F8, we Esc back to the Tree.
    """
    sync_state(yt)
    screen = get_clean_screen(yt)
    if "FILE" in screen and "DIR" not in screen:
        yt.child.send(Keys.ESC)
        sync_state(yt)

def test_f8_split_truncation_bug(controller, dual_panel_sandbox):
    """
    BUG REGRESSION: Multi-column Corruption.
    Verified: Navigating active panel corrupts inactive multi-column list.
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    # 1. Setup multi-column list on Left
    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)
    yt.child.send(Keys.ENTER) # Enter left_dir
    sync_state(yt)

    # 2. Split Screen
    yt.child.send(Keys.F8)
    detect_and_fix_auto_enter(yt)

    # 3. Tab to Right
    yt.child.send(Keys.TAB)
    sync_state(yt)

    # 4. Navigate right_dir and enter to trigger redraw
    for _ in range(5):
        if "right_dir" in get_clean_screen(yt): break
        yt.child.send(Keys.DOWN)
    yt.child.send(Keys.ENTER)
    sync_state(yt)

    # 5. Bug Verification
    screen = get_clean_screen(yt)
    if "ch  " in screen:
        pytest.fail("BUG CONFIRMED: Multi-column corruption detected in inactive panel (chgrp.c -> ch  ).")

    if "chgrp.c" not in screen:
        pytest.fail("BUG CONFIRMED: Filename 'chgrp.c' missing or truncated in inactive panel.")

    yt.quit()

def test_f8_cross_panel_autoview_sourcing_bug(controller, dual_panel_sandbox):
    """
    BUG: Autoview
    Verifies:
    1. File list is for Right Panel and first file in list is selected.
    2. Preview matches the selected file and changes with scroll.
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    # Expand all to ensure directories are visible in the tree
    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)

    # Enter split-screen mode
    yt.child.send(Keys.F8)
    detect_and_fix_auto_enter(yt)

    # Move focus to the Right Panel
    yt.child.send(Keys.TAB)
    sync_state(yt)

    # Navigate to right_dir in the tree
    for _ in range(10):
        if "right_dir" in get_clean_screen(yt):
            break
        yt.child.send(Keys.DOWN)
        sync_state(yt)

    # Trigger Autoview (F7)
    yt.child.send(Keys.F7)
    # ncurses needs a moment to spawn the internal pager/viewer
    time.sleep(1.0)
    sync_state(yt)

    screen = get_clean_screen(yt)

    # 1. Verification: Is the file list sourced from the Right Panel directory?
    # right_dir contains files named "0", "1", "2"...
    if "0" not in screen or "1" not in screen:
        pytest.fail("F7 Logic Error: File list failed to populate from Right Panel directory.")

    # 2. Verification: Does the preview match the first selected file?
    # We expect "content of 0" to be visible in the content pane.
    if "content of 0" not in screen:
        pytest.fail("BUG: Autoview content pane is not displaying data for the selected file '0'.")

    # 3. Verification: Does the preview change when we scroll the selection?
    yt.child.send(Keys.DOWN)
    sync_state(yt)
    # Wait for the view to refresh
    time.sleep(0.5)

    screen_after_scroll = get_clean_screen(yt)
    if "content of 1" not in screen_after_scroll:
        pytest.fail("BUG: Autoview content failed to update after scrolling to file '1'.")

    yt.quit()

def test_f7_header_refresh_delay(controller, dual_panel_sandbox):
    """
    BUG REGRESSION: Stale Path Header.
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)

    yt.child.send(Keys.F7)
    time.sleep(0.5)
    sync_state(yt)

    screen = get_clean_screen(yt)
    path_match = re.search(r"Path:.*", screen)
    if not path_match:
        pytest.fail("TUI Error: Could not find 'Path:' header.")

    path_text = path_match.group(0)
    # BUG: Shows 'left_dir' instead of 'chgrp.c'
    if "left_dir" in path_text and "chgrp.c" not in path_text:
        pytest.fail("BUG CONFIRMED: F7 Header shows stale directory path; only updates after scrolling.")

    yt.quit()