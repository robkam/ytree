import pytest
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    return "\n".join(tui.get_screen_dump()[-3:]).lower()

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

    # If split occurred from file mode, the peer panel may also be in file view.
    # Drop to tree view before navigating to right_dir.
    if "hex j compare" in _footer_text(tui):
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.3)

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


def test_split_from_file_keeps_file_focus_on_tab(tmp_path, ytree_binary):
    root = tmp_path / "split_file_focus_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    footer = _footer_text(tui)
    assert "hex j compare" in footer, (
        "Switching panels after splitting from file view should keep file mode.\n"
        f"{footer}"
    )

    tui.quit()


def test_split_from_file_preserves_inactive_panel_file_state(tmp_path, ytree_binary):
    root = tmp_path / "split_file_focus_inactive_state"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_unique_123.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_unique_456.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    assert screen.count("alpha_unique_123.txt") >= 2, (
        "Inactive panel did not retain its file-window state after split/tab.\n"
        f"{screen}"
    )

    tui.quit()


def test_split_tab_back_preserves_selected_file_index(tmp_path, ytree_binary):
    root = tmp_path / "split_file_selection_persistence"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    # Select alpha_2.txt in the left panel.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    # Do work in other panel: navigate to beta and enter file view.
    if "hex j compare" in _footer_text(tui):
        tui.send_keystroke(Keys.ESC, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    # Return to original panel and verify selected file is unchanged.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("SOURCE: alpha_2.txt", timeout=1.0)

    tui.quit()


def test_inactive_panel_stays_file_focused_after_tab_away(tmp_path, ytree_binary):
    root = tmp_path / "inactive_panel_file_focus"
    root.mkdir()
    left = root / "left_focus_dir_A"
    right = root / "right_focus_dir_B"
    left.mkdir()
    right.mkdir()
    (right / "right_focus_file_0.txt").write_text("x\n", encoding="utf-8")
    (right / "right_focus_file_1.txt").write_text("y\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    # Move right panel to right_focus_dir_B and enter file view.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    # Switch away. Inactive right panel should remain file-focused visually,
    # not revert to tree.
    tui.send_keystroke(Keys.TAB, wait=0.4)

    screen = "\n".join(tui.get_screen_dump())
    assert "right_focus_file_0.txt" in screen
    assert "right_focus_file_1.txt" in screen
    assert screen.count("right_focus_dir_B") <= 1, (
        "Inactive panel reverted to tree view after tab away.\n"
        f"{screen}"
    )

    tui.quit()

def test_f8_active_header_sync(dual_panel_sandbox, ytree_binary):
    """
    BUG 4: Verifies the top 'Path:' header updates to match the ACTIVE panel's volume path.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    # Split screen
    tui.send_keystroke(Keys.F8)
    time.sleep(0.5)

    # Tab to Right Panel
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)

    # Move down to 'right_dir' and ENTER
    screen = "\n".join(tui.get_screen_dump())
    if "child" in screen: print("CHILD ALREADY VISIBLE")
    tui.send_keystroke(Keys.DOWN + Keys.DOWN)
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # The header should show the active panel's path (right_dir)
    # Extract the first line (header)
    header_line = screen.split('\n')[0]

    if "right_dir" not in header_line:
        pytest.fail(f"HEADER SYNC BUG: Expected 'right_dir' in header, but got:\n{header_line}")

    tui.quit()

def test_navigation_does_not_expand(tmp_path, ytree_binary):
    """
    BUG 2: Verifies that pressing DOWN arrow merely moves the cursor,
    and does NOT automatically scan/expand subdirectories.
    """
    # Create nested structure: test_root/parent/child/file.txt
    root = tmp_path / "test_root"
    child_dir = root / "parent" / "child"
    child_dir.mkdir(parents=True)
    (child_dir / "file.txt").touch()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(1.0)

    # Press DOWN to highlight 'parent'. It should NOT expand to show 'child'.
    screen = "\n".join(tui.get_screen_dump())
    if "child" in screen: print("CHILD ALREADY VISIBLE")
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    if "child" in screen:
        pytest.fail(f"AUTO-EXPAND BUG: Pressing DOWN automatically expanded the directory. 'child' is visible:\n{screen}")

    tui.quit()

def test_header_path_clearing(dual_panel_sandbox, ytree_binary):
    """BUG 6: Header doesn't clear old long paths when moving to short paths."""
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    # 1. Navigate deep to a long path
    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    # 2. Navigate back up to a short path (ESC returns from file window to tree view)
    tui.send_keystroke(Keys.ESC)
    time.sleep(0.5)
    tui.send_keystroke(Keys.LEFT)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    # The header should NOT contain the old directory name
    assert "left_dir" not in screen.splitlines()[0], "Header path was not cleared properly!"

def test_dialog_screen_wiping(dual_panel_sandbox, ytree_binary):
    """BUG 4: Returning from a dialog leaves the screen missing separator lines."""
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    # Trigger a dialog (Makedir) and cancel out
    tui.send_keystroke("M")
    time.sleep(0.5)
    tui.send_keystroke(Keys.ESC)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    # VT100 mode renders separator lines as 'q' characters (ACS_HLINE).
    # If the screen was wiped, the horizontal line above the menu will be missing.
    assert "qqq" in screen, "Separator lines were wiped from the background!"

def test_negative_filter_logic(dual_panel_sandbox, ytree_binary):
    """BUG 5: Negative filter (-*.o) hides everything instead of just .o files."""
    # Create a mixed directory
    (dual_panel_sandbox / "code.c").touch()
    (dual_panel_sandbox / "code.o").touch()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    tui.send_keystroke("f")
    tui.send_keystroke("\x15")  # Ctrl+U to clear the pre-filled "*"
    tui.send_keystroke("-*.o\r")
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    assert "code.c" in screen, "Negative filter hid files that should be visible!"
    assert "code.o" not in screen, "Negative filter failed to hide the target file!"

def test_split_screen_memory_isolation(dual_panel_sandbox, ytree_binary):
    """BUG 1: Inactive panel displays garbage/forgets state when active panel scrolls."""
    (dual_panel_sandbox / "left_dir" / "target_file.txt").touch()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    # Left Panel: Enter left_dir to see target_file.txt
    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    # Split & Tab to Right Panel
    tui.send_keystroke(Keys.F8)
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)

    # Right Panel: Scroll and collapse
    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.LEFT)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    # If memory is corrupted or state is lost, target_file.txt will turn into garbage (e.g., *?^X)
    assert "target_file.txt" in screen, "Inactive panel lost its memory state or was overwritten by garbage!"

def test_f8_big_window_footer_and_separator_lost(dual_panel_sandbox, ytree_binary):
    """
    BUG D: Entering a big file window via F8 causes the footer and the horizontal panel separator in the inactive panel to disappear.
    EXPECTED: The horizontal separator (e.g., qqqq or ---) stays, and the footer correctly appears.
    """
    # Start with NOSMALLWINDOW=0 to trigger the bug on the small->big transition
    ytree_cfg = dual_panel_sandbox / ".ytree"
    ytree_cfg.write_text("NOSMALLWINDOW=0\n")

    tui = YtreeTUI(
        executable=ytree_binary, 
        cwd=str(dual_panel_sandbox)
    )
    time.sleep(1.0)
    # 1. Split screen
    tui.send_keystroke(Keys.F8)
    time.sleep(0.5)
    
    # 2. Swap to right panel
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)
    
    # 3. Move down to right_dir (which has files)
    tui.send_keystroke(Keys.DOWN + Keys.DOWN)
    time.sleep(0.5)
    
    # 4. Press Enter to drop into small window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    # 5. Press Enter to drop into big file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    
    screen = "\n".join(tui.get_screen_dump())
    lines = screen.split('\n')
    footer = '\n'.join(lines[-3:]).lower()
    # Verify horizontal separator lines are present in the inactive panel
    # We'll check for the horizontal border characters 'qqq' or '---'
    has_separator = "qqq" in screen or "---" in screen
    
    if not has_separator:
        pytest.fail(f"BUG: Horizontal panel separator lost in inactive panel after entering big window from F8 split.\n{screen}")
        
    # Verify the footer isn't wiped out
    if "attribute" not in footer and "delete" not in footer:
        pytest.fail(f"BUG: Footer disappeared after entering big window from F8 split.\nFooter dump:\n{footer}")
    tui.quit()


def test_f8_inactive_selection_moves_to_parent_on_mirrored_collapse(tmp_path, ytree_binary):
    """
    Regression:
    When both panes share the same tree and active pane collapses a branch,
    an inactive selection inside that branch must re-anchor to the parent.
    """
    root = tmp_path / "f8_mirrored_collapse_root"
    root.mkdir()

    parent_dir = root / "parent_dir"
    child_dir = parent_dir / "child_dir"
    child_dir.mkdir(parents=True)

    (parent_dir / "parent_file.txt").write_text("parent\n", encoding="utf-8")
    (child_dir / "child_file.txt").write_text("child\n", encoding="utf-8")

    sibling_dir = root / "sibling_dir"
    sibling_dir.mkdir()
    (sibling_dir / "sibling_file.txt").write_text("sibling\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(1.0)

    # Expand tree so parent/child are visible in both panes.
    tui.send_keystroke(Keys.EXPAND_ALL)
    time.sleep(0.5)

    # Left pane (active): move to parent_dir.
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.5)

    # Split and move to right pane.
    tui.send_keystroke(Keys.F8)
    time.sleep(0.5)
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)

    # Right pane: move inside collapsed target branch (parent -> child).
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.5)

    # Back to left pane and collapse parent_dir.
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)
    tui.send_keystroke(Keys.LEFT)
    time.sleep(0.6)

    # Return to right pane and enter selected directory.
    # Expected: selection was re-anchored to parent_dir.
    tui.send_keystroke(Keys.TAB)
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.6)

    screen = "\n".join(tui.get_screen_dump())
    if "parent_file.txt" not in screen:
        tui.quit()
        pytest.fail(
            "Inactive selection did not move to parent after mirrored collapse.\n"
            f"Screen:\n{screen}"
        )
    if "child_file.txt" in screen:
        tui.quit()
        pytest.fail(
            "Inactive pane still entered child after parent collapse.\n"
            f"Screen:\n{screen}"
        )

    tui.quit()


def test_split_file_focus_survives_tab_round_trip(tmp_path, ytree_binary):
    root = tmp_path / "split_file_focus_round_trip"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "alpha.txt" in "\n".join(tui.get_screen_dump())
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    screen = "\n".join(tui.get_screen_dump())
    assert "alpha.txt" in screen, f"Left panel lost its file selection after split/tab round-trip.\n{screen}"
    assert "hex j compare" in _footer_text(tui), f"Left panel lost file footer after split/tab round-trip.\n{screen}"

    tui.quit()


def test_split_panels_keep_independent_file_focus_states(tmp_path, ytree_binary):
    root = tmp_path / "split_file_focus_independent"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "alpha.txt" in "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    if "hex j compare" in _footer_text(tui):
        tui.send_keystroke(Keys.ESC, wait=0.3)

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "beta.txt" in "\n".join(tui.get_screen_dump())
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.TAB, wait=0.4)

    screen = "\n".join(tui.get_screen_dump())
    assert "alpha.txt" in screen, f"Returning to the left panel did not restore its file view.\n{screen}"
    assert "hex j compare" in _footer_text(tui), f"Returning to the left panel did not restore file footer/help.\n{screen}"

    tui.quit()
