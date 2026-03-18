import pytest
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    return "\n".join(tui.get_screen_dump()[-3:]).lower()

def _detect_split_column(lines):
    if len(lines) < 3:
        return None

    top = lines[1]
    for ch in ("w", "┬", "+"):
        idx = top.find(ch, 1)
        if idx != -1:
            return idx

    counts = {}
    for row in lines[2:-4]:
        for x, ch in enumerate(row):
            if ch in ("x", "|"):
                counts[x] = counts.get(x, 0) + 1

    if not counts:
        return None
    return max(counts, key=counts.get)

def _assert_split_column_continuous(lines, label):
    split_col = _detect_split_column(lines)
    assert split_col is not None, f"Could not detect split column ({label}).\n" + "\n".join(lines)

    for y in range(2, max(2, len(lines) - 4)):
        row = lines[y]
        if split_col >= len(row):
            continue
        assert row[split_col] != " ", (
            f"Split separator has a gap at row {y} ({label}).\n" + "\n".join(lines)
        )

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

def test_split_from_dir_immediately_renders_peer_panel(tmp_path, ytree_binary):
    root = tmp_path / "split_dir_immediate_render"
    root.mkdir()
    (root / "alpha_peer_dir").mkdir()
    (root / "beta_peer_dir").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.F8, wait=0.4)

    screen = "\n".join(tui.get_screen_dump())
    assert screen.count("alpha_peer_dir") >= 2, (
        "Split from dir view did not render peer panel until next keypress.\n"
        f"{screen}"
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

def test_split_from_file_immediate_peer_mirror_not_blank(tmp_path, ytree_binary):
    root = tmp_path / "split_file_immediate_mirror"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_immediate_uniq.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_immediate_uniq.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)

    screen = "\n".join(tui.get_screen_dump())
    assert screen.count("alpha_immediate_uniq.txt") >= 2, (
        "Peer panel stayed blank after splitting from file view.\n"
        f"{screen}"
    )

    tui.quit()


def test_split_mirror_stays_on_active_volume_after_volume_cycle(tmp_path, ytree_binary):
    vol_a = tmp_path / "vol_a"
    vol_b = tmp_path / "vol_b"
    vol_c = tmp_path / "vol_c"
    vol_a.mkdir()
    vol_b.mkdir()
    vol_c.mkdir()
    (vol_a / "a_only.txt").write_text("a\n", encoding="utf-8")
    (vol_b / "b_only.txt").write_text("b\n", encoding="utf-8")
    (vol_c / "c_only.txt").write_text("c\n", encoding="utf-8")

    tui = YtreeTUI(
        executable=ytree_binary,
        cwd=str(vol_a),
        args=[str(vol_b), str(vol_c)],
    )
    time.sleep(1.0)

    vol_to_file = {
        "vol_a": "a_only.txt",
        "vol_b": "b_only.txt",
        "vol_c": "c_only.txt",
    }

    def active_volume_name():
        header = tui.get_screen_dump()[0]
        for vol_name in vol_to_file:
            if vol_name in header:
                return vol_name
        return None

    start_vol = active_volume_name()
    assert start_vol is not None, "Could not detect starting volume in header."

    tui.send_keystroke(Keys.ENTER, wait=0.5)
    assert "hex j compare" in _footer_text(tui)
    start_file = vol_to_file[start_vol]
    assert start_file in "\n".join(tui.get_screen_dump())

    # Seed right-panel cache on the current volume, then unsplit.
    tui.send_keystroke(Keys.F8, wait=0.5)
    tui.send_keystroke(Keys.F8, wait=0.5)

    # Cycle until a different volume becomes active.
    target_vol = None
    for _ in range(12):
        current = active_volume_name()
        if current and current != start_vol:
            target_vol = current
            break
        tui.send_keystroke("<", wait=0.5)
    assert target_vol is not None, "Failed to cycle to a different volume."

    if "hex j compare" not in _footer_text(tui):
        tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    target_file = vol_to_file[target_vol]
    screen = "\n".join(tui.get_screen_dump())
    assert target_file in screen

    # Regression guard: initial split mirror must not show stale file list from
    # another volume until TAB.
    tui.send_keystroke(Keys.F8, wait=0.5)
    screen = "\n".join(tui.get_screen_dump())
    assert screen.count(target_file) >= 2, (
        "Split mirror reused stale file list from another volume.\n"
        f"{screen}"
    )
    assert start_file not in screen, (
        "Split mirror leaked prior volume file list on first draw.\n"
        f"{screen}"
    )

    tui.quit()


def test_inactive_dir_focus_survives_tab_away_and_back(tmp_path, ytree_binary):
    root = tmp_path / "inactive_dir_focus_survives_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (beta / "beta_focus_file.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)
    assert "dir      attributes brief compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    assert "dir      attributes brief compare" in _footer_text(tui)

    # Move to right panel and enter file mode there.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    # Return to left panel. It must still be in dir mode.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    assert "dir      attributes brief compare" in _footer_text(tui), (
        "Inactive panel lost dir focus after tab-away from dir mode."
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

def test_split_separator_stays_continuous_during_file_tree_toggle(tmp_path, ytree_binary):
    root = tmp_path / "split_separator_continuity"
    root.mkdir()
    left = root / "left_sep_dir"
    right = root / "right_sep_dir"
    left.mkdir()
    right.mkdir()
    (left / "left_a.txt").write_text("a\n", encoding="utf-8")
    (right / "right_a.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "right active file / left inactive tree")

    tui.send_keystroke(Keys.TAB, wait=0.4)
    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "left active tree / right inactive file")

    tui.send_keystroke(Keys.TAB, wait=0.4)
    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "right active file after second tab")

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


def test_active_mode_toggles_do_not_mutate_inactive_file_state(tmp_path, ytree_binary):
    root = tmp_path / "split_independent_mode_toggles"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("0\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Left panel: enter alpha file view and select alpha_1.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    assert "hex j compare" in _footer_text(tui)

    # Split and move to right panel.
    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    # Right panel: toggle between dir/file/small-big transitions.
    if "hex j compare" in _footer_text(tui):
        tui.send_keystroke(Keys.ESC, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)  # file (small)
    tui.send_keystroke(Keys.ENTER, wait=0.4)  # file (big)
    tui.send_keystroke(Keys.ESC, wait=0.4)    # back to tree
    tui.send_keystroke(Keys.ENTER, wait=0.4)  # file again

    # Left panel must still have file focus and selected alpha_1.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    assert "hex j compare" in _footer_text(tui)
    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("SOURCE: alpha_1.txt", timeout=1.0), (
        "Active-panel mode changes mutated inactive panel file selection/state."
    )

    tui.quit()
