import pytest
import shlex
import time
import re
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    return "\n".join(tui.get_screen_dump()[-3:]).lower()


def _detect_stats_split_x(lines):
    if len(lines) < 2:
        return None
    border = lines[1]
    marker = "wqqqqqqq FILTER"
    idx = border.find(marker)
    if idx != -1:
        return idx
    return None


def _current_file_from_stats(lines, split_x):
    for i, line in enumerate(lines):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT FILE" in segment and i + 1 < len(lines):
            next_segment = lines[i + 1][split_x:] if split_x is not None else lines[i + 1]
            m = re.search(r"([A-Za-z0-9._-]+\\.txt)", next_segment)
            if m:
                return m.group(1)
            # Fallback: sometimes the next line capture still includes border
            # glyphs; search the full next row as a last resort.
            m = re.search(r"([A-Za-z0-9._-]+\\.txt)", lines[i + 1])
            if m:
                return m.group(1)
    return None


def _stats_current_dir_contains(lines, marker):
    split_x = _detect_stats_split_x(lines)
    for i, line in enumerate(lines):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT DIR" not in segment:
            continue
        for j in (1, 2):
            idx = i + j
            if idx >= len(lines):
                continue
            candidate = lines[idx][split_x:] if split_x is not None else lines[idx]
            if marker in candidate:
                return True
    return False


def _configure_filediff_capture(tmp_dir):
    log_path = tmp_dir / "filediff_args.log"
    helper_path = tmp_dir / ".capture_filediff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)

    (tmp_dir / ".ytree").write_text(
        f"[GLOBAL]\nFILEDIFF={helper_path}\n",
        encoding="utf-8",
    )
    return log_path


def _wait_for_file(path, timeout=2.0):
    end = time.time() + timeout
    while time.time() < end:
        if path.exists():
            return True
        time.sleep(0.05)
    return False

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


def test_split_refresh_updates_inactive_tree_file_list_without_tab(
    tmp_path, ytree_binary
):
    root = tmp_path / "split_inactive_refresh_updates_tree_file_list"
    root.mkdir()
    left = root / "left_dir"
    right = root / "right_dir"
    left.mkdir()
    right.mkdir()
    (left / "left_old.txt").write_text("left\n", encoding="utf-8")
    (right / "right_old.txt").write_text("right\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    # Right pane: select right_dir in tree mode so its small file list is visible.
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    assert "right_old.txt" in "\n".join(tui.get_screen_dump())

    # Keep right pane inactive while refreshing from the left pane.
    tui.send_keystroke(Keys.TAB, wait=0.35)
    (right / "right_new.txt").write_text("new\n", encoding="utf-8")
    time.sleep(0.1)
    tui.send_keystroke(Keys.CTRL_L, wait=0.9)

    screen = "\n".join(tui.get_screen_dump())
    assert "right_new.txt" in screen, (
        "Inactive split panel did not refresh its file list after external change.\n"
        "The new file only appeared after switching panels.\n"
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

    log_path = _configure_filediff_capture(root)

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
    tui.send_keystroke(Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_2.txt"), (
        "Split-tab round-trip changed selected source file.\n"
        f"Expected source: {alpha / 'alpha_2.txt'}\nActual source: {logged[0]}"
    )

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


def test_volume_cycle_restores_prior_directory_selection(tmp_path, ytree_binary):
    vol_rich = tmp_path / "vol_rich_restore_selection"
    vol_sparse_b = tmp_path / "vol_sparse_b_restore_selection"
    vol_sparse_c = tmp_path / "vol_sparse_c_restore_selection"
    vol_rich.mkdir()
    vol_sparse_b.mkdir()
    vol_sparse_c.mkdir()

    for i in range(6):
        d = vol_rich / f"r_dir_{i}"
        d.mkdir()
        (d / f"r_file_{i}_unique.txt").write_text(f"{i}\n", encoding="utf-8")

    (vol_sparse_b / "b_dir_0").mkdir()
    (vol_sparse_b / "b_dir_0" / "b_file_0_unique.txt").write_text("b\n", encoding="utf-8")
    (vol_sparse_c / "c_dir_0").mkdir()
    (vol_sparse_c / "c_dir_0" / "c_file_0_unique.txt").write_text("c\n", encoding="utf-8")

    tui = YtreeTUI(
        executable=ytree_binary,
        cwd=str(tmp_path),
        args=[str(vol_rich), str(vol_sparse_b), str(vol_sparse_c)],
    )
    time.sleep(1.0)

    def active_volume_name():
        header = tui.get_screen_dump()[0]
        for name in (
            "vol_rich_restore_selection",
            "vol_sparse_b_restore_selection",
            "vol_sparse_c_restore_selection",
        ):
            if name in header:
                return name
        return None

    # Normalize to the rich volume first so we can choose a non-trivial index.
    for _ in range(12):
        if active_volume_name() == "vol_rich_restore_selection":
            break
        tui.send_keystroke(">", wait=0.35)
    assert active_volume_name() == "vol_rich_restore_selection", (
        "Could not switch to rich volume for selection restore test.\n"
        + "\n".join(tui.get_screen_dump())
    )
    start_vol = "vol_rich_restore_selection"

    # Move off the default/root selection before capturing target.
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)
    assert "hex j compare" in _footer_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    expected_file = None
    for candidate in (
        "r_file_0_unique.txt",
        "r_file_1_unique.txt",
        "r_file_2_unique.txt",
        "r_file_3_unique.txt",
        "r_file_4_unique.txt",
        "r_file_5_unique.txt",
    ):
        if candidate in screen:
            expected_file = candidate
            break
    assert expected_file is not None, (
        "Failed to detect selected file on initial volume before cycling.\n" + screen
    )

    tui.send_keystroke(Keys.ESC, wait=0.35)
    assert "dir      attributes brief compare" in _footer_text(tui)
    lines = tui.get_screen_dump()
    expected_dir = expected_file.replace("r_file_", "r_dir_").replace("_unique.txt", "")
    assert _stats_current_dir_contains(lines, expected_dir), (
        "Baseline check failed: selected directory was not retained after leaving file view.\n"
        f"Expected selected dir marker: {expected_dir}\n" + "\n".join(lines)
    )

    # Cycle away and back.
    seen_other = False
    returned = False
    for _ in range(18):
        tui.send_keystroke(">", wait=0.45)
        current = active_volume_name()
        if current and current != start_vol:
            seen_other = True
        if seen_other and current == start_vol:
            returned = True
            break

    assert returned, "Did not return to start volume while cycling loaded volumes."

    lines = tui.get_screen_dump()
    screen = "\n".join(lines)
    assert _stats_current_dir_contains(lines, expected_dir), (
        "Directory selection was not restored after cycling volumes away/back.\n"
        f"Expected selected dir marker: {expected_dir}\n{screen}"
    )

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
    # Start with SMALLWINDOWSKIP=0 to trigger the bug on the small->big transition
    ytree_cfg = dual_panel_sandbox / ".ytree"
    ytree_cfg.write_text("SMALLWINDOWSKIP=0\n")

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

    log_path = _configure_filediff_capture(root)

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
    tui.send_keystroke(Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_1.txt"), (
        "Active-panel mode changes mutated inactive panel file selection/state."
    )

    tui.quit()


def test_split_from_file_keeps_inactive_file_selection_independent(tmp_path, ytree_binary):
    root = tmp_path / "split_file_independent_scroll_state"
    root.mkdir()
    alpha = root / "alpha"
    alpha.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (alpha / "alpha_3.txt").write_text("3\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)  # Enter alpha
    assert "hex j compare" in _footer_text(tui)

    # Set split baseline to alpha_1.
    tui.send_keystroke(Keys.DOWN, wait=0.2)

    tui.send_keystroke(Keys.F8, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    # Move active panel to alpha_3; inactive panel should remain on alpha_1.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)

    tui.send_keystroke(Keys.TAB, wait=0.4)
    if "hex j compare" not in _footer_text(tui):
        tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_1.txt"), (
        "Inactive panel file selection tracked active scrolling after split-from-file."
    )

    tui.quit()


def test_log_new_volume_from_file_view_resets_focus_and_selection(tmp_path, ytree_binary):
    root = tmp_path / "file_log_new_volume_resets_focus"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("0\n", encoding="utf-8")
    (beta / "beta_1.txt").write_text("1\n", encoding="utf-8")
    (beta / "beta_2.txt").write_text("2\n", encoding="utf-8")
    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Enter alpha file view and select a non-first file.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    assert "hex j compare" in _footer_text(tui)

    # Log a new volume directly from file view.
    tui.send_keystroke(Keys.LOG, wait=0.2)
    tui.send_keystroke(Keys.CTRL_U + str(beta) + Keys.ENTER, wait=0.8)

    # Must return to directory mode first (no implicit file-window entry).
    footer = _footer_text(tui)
    assert "dir      attributes brief compare" in footer, (
        "Logging a new volume from file view should return to directory mode."
    )

    # Enter file view explicitly and ensure selection starts at first file.
    tui.send_keystroke(Keys.ENTER, wait=0.5)
    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U + str(compare_target) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(beta / "beta_0.txt"), (
        "Newly logged volume file selection should reset to first entry."
    )

    tui.quit()


def test_log_second_volume_from_file_view_keeps_tree_on_root(tmp_path, ytree_binary):
    root = tmp_path / "file_log_second_volume_starts_at_root"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (root / "root_mode_anchor.txt").write_text("anchor\n", encoding="utf-8")
    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_root_file.txt").write_text("root\n", encoding="utf-8")
    (beta / "aa_probe_dir").mkdir()
    (beta / "bb_probe_dir").mkdir()
    (beta / "aa_probe_dir" / "aa_only.txt").write_text("aa\n", encoding="utf-8")
    (beta / "bb_probe_dir" / "bb_only.txt").write_text("bb\n", encoding="utf-8")
    _configure_filediff_capture(root)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Enter file view first to exercise the same path that regressed.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.LOG, wait=0.2)
    tui.send_keystroke(Keys.CTRL_U + str(beta) + Keys.ENTER, wait=0.9)

    footer = _footer_text(tui)
    assert "dir      attributes brief compare" in footer, (
        "Logging a second volume from file view should return to directory mode."
    )

    lines = tui.get_screen_dump()
    screen = "\n".join(lines)
    assert not _stats_current_dir_contains(lines, "aa_probe_dir"), (
        "Newly logged second volume should select root, not the first child dir.\n"
        f"{screen}"
    )
    assert not _stats_current_dir_contains(lines, "bb_probe_dir"), (
        "Newly logged second volume should select root, not a child dir.\n"
        f"{screen}"
    )

    tui.quit()


def test_volume_menu_cancel_restores_dir_footer_immediately(tmp_path, ytree_binary):
    root = tmp_path / "volume_menu_cancel_dir_footer"
    root.mkdir()
    (root / "a.txt").write_text("a\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke("k", wait=0.3)
    assert tui.wait_for_content("Select Volume", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.3)

    footer = _footer_text(tui)
    assert "dir      attributes brief compare" in footer, (
        "Cancelling volume menu from dir view should immediately restore dir footer."
    )
    assert "f1 help" in footer, (
        "Cancelling volume menu from dir view left footer/help blank until next key."
    )

    tui.quit()


def test_volume_menu_cancel_restores_file_footer_immediately(tmp_path, ytree_binary):
    root = tmp_path / "volume_menu_cancel_file_footer"
    root.mkdir()
    (root / "a.txt").write_text("a\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert "hex j compare" in _footer_text(tui)

    tui.send_keystroke("k", wait=0.3)
    assert tui.wait_for_content("Select Volume", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.3)

    footer = _footer_text(tui)
    assert "hex j compare" in footer, (
        "Cancelling volume menu from file view should immediately restore file footer."
    )
    assert "f1 help" in footer, (
        "Cancelling volume menu from file view left footer/help blank until next key."
    )

    tui.quit()
