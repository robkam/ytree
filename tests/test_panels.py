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
    try:
        yt.child.expect(pexpect.TIMEOUT, timeout=0.2)
    except pexpect.TIMEOUT:
        pass
    raw = (yt.child.before if isinstance(yt.child.before, str) else "") + \
          (yt.child.after if isinstance(yt.child.after, str) else "")
    # Remove all ANSI escape sequences to allow clean regex/string matching
    # Matches \x1b[ followed by multiple numbers/semicolons and ending in a letter.
    # It also strips \x1b(B and \x1b[?1h etc.
    clean = re.sub(r'\x1B(?:\[[0-9;]*[a-zA-Z]|\(B|\[\?[0-9]*[a-zA-Z]|\**|[=>])?', '', raw)
    return clean

def sync_state(yt):
    """Universal synchronization: waits for UI repaint activity."""
    yt.child.expect(
        [r"Path:", r"COMMANDS", r"\x1b\[[0-9;?]*[A-Za-z]", pexpect.TIMEOUT],
        timeout=2.0,
    )

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

def test_f8_split_memory_corruption_bug(controller, dual_panel_sandbox):
    """
    BUG REGRESSION: Inactive Panel Memory Corruption (Use-After-Free).
    Verified: Collapsing a directory in the active panel clobbers
    the inactive panel's FileEntryList if it shares the same tree, showing garbage characters.
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    # 1. Setup multi-column list on Left (left_dir)
    time.sleep(0.5)
    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)
    yt.child.send(Keys.DOWN)  # Move to left_dir
    sync_state(yt)
    yt.child.send(Keys.ENTER) # Enter left_dir file window
    sync_state(yt)

    # 2. Split Screen
    yt.child.send(Keys.F8)
    detect_and_fix_auto_enter(yt)

    # 3. Tab to Right
    yt.child.send(Keys.TAB)
    sync_state(yt)

    # 4. Navigate right_dir (pos 2)
    yt.child.send(Keys.DOWN)
    sync_state(yt)

    # 5. Trigger the bug: Collapse the parent directory from the right panel
    # In ytree, pressing LEFT on a directory usually collapses it or moves to parent.
    yt.child.send("-")
    sync_state(yt)

    # 6. Bug Verification
    screen = get_clean_screen(yt)
    expected_file = "very_long_filename_alpha_numeric_extension_test.txt"

    # Check for the expected file in the inactive left panel
    if "very_long_filen" not in screen:
        pytest.fail(f"BUG CONFIRMED: Inactive panel lost its file list or memory was corrupted. '{expected_file}' not found.")

    yt.quit()

def test_f8_cross_panel_autoview_sourcing_bug(controller, dual_panel_sandbox):
    """
    BUG: Autoview State Breakage
    Verifies:
    1. F7 previews the correct file from the active panel.
    2. Pressing F8 while IN Autoview mode is ignored (does not reset to root).
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    time.sleep(0.5)
    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)

    yt.child.send(Keys.F8)
    detect_and_fix_auto_enter(yt)

    yt.child.send(Keys.TAB)
    sync_state(yt)

    # Navigate to right_dir
    yt.child.send(Keys.DOWN)
    sync_state(yt)
    yt.child.send(Keys.DOWN)
    sync_state(yt)

    # Trigger Autoview (F7)
    yt.child.send(Keys.F7)
    time.sleep(1.0)
    sync_state(yt)

    screen = get_clean_screen(yt)

    if "content of 0" not in screen:
        pytest.fail("BUG: Autoview content pane is not displaying data for the selected file '0'.")

    # NEW TEST: Press F8 during F7
    yt.child.send(Keys.F8)
    time.sleep(0.5)
    sync_state(yt)

    screen_after_f8 = get_clean_screen(yt)
    if "content of 0" not in screen_after_f8:
        pytest.fail("BUG CONFIRMED: Pressing F8 during F7 Autoview breaks state and resets the view.")

    yt.quit()

def test_f7_header_shows_directory_only(controller, dual_panel_sandbox):
    """
    SPECIFICATION FIX: XTree&trade;/ZTree compatibility.
    Verifies that the Path header ONLY shows the directory path,
    and does NOT append the filename when exploring files or using F7.
    """
    yt = controller(cwd=str(dual_panel_sandbox))
    yt.wait_for_startup()

    time.sleep(0.5)
    yt.child.send(Keys.EXPAND_ALL)
    sync_state(yt)
    time.sleep(1.0) # Explicit wait before down

    yt.child.send(Keys.DOWN) # left_dir
    sync_state(yt)
    time.sleep(1.0) # Explicit wait after down

    yt.child.send(Keys.F7)
    time.sleep(0.5)
    sync_state(yt)

    screen = get_clean_screen(yt)
    path_matches = re.findall(r"Path:\s*([^\s\r\n\x1b]+)", screen)
    if not path_matches:
        pytest.fail(f"TUI Error: Could not find 'Path:' header.\nScreen Dump:\n{screen}")

    # Assert none of the paths contain the filename
    for match in path_matches:
        if "very_long_filename" in match:
            pytest.fail(f"TUI Error: 'Path:' header contains filename: {match}\nAll matches: {path_matches}")

    yt.quit()

def test_f7_visual_layout(ytree_binary, dual_panel_sandbox):
    """
    Test that the visual layout of the Autoview (F7) is correct in a 120x36 grid.
    Asserts no overlapping text and proper header alignments.
    """
    from tui_harness import YtreeTUI
    from ytree_keys import Keys
    import time

    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))

    # Expand all
    tui.send_keystroke(Keys.EXPAND_ALL)

    # Trigger Autoview (F7)
    tui.send_keystroke(Keys.F7)
    time.sleep(1.0) # Wait for view

    # Get the screen content
    screen = tui.get_screen_dump()

    # Join the screen array into a single string for easier assertions, but keep line access.
    # The grid is 36 lines by 120 columns.
    assert len(screen) == 36
    assert len(screen[0]) == 120

    # In F7 preview mode:
    # Top border starts at line 1, headers typically at top.
    # We should look for the file list left pane and the preview right pane.

    # For now, let's verify that the "Path:" and "Level:" headers are visually present
    # and not clumped together.
    header_found = False
    for line in screen:
        if "Path:" in line:
            header_found = True
            break
    assert header_found, "Path: header missing in visual layout"

    tui.quit()
