import pytest
import time
import re
import pexpect
from helpers_stats import current_file_from_stats as _current_file_from_stats
from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_text as _footer_text
from ytree_keys import Keys
from tui_harness import YtreeTUI

def get_clean_screen(yt):
    try:
        yt.child.expect(pexpect.TIMEOUT, timeout=0.2)
    except:
        pass
    raw = (yt.child.before if isinstance(yt.child.before, str) else "") + \
          (yt.child.after if isinstance(yt.child.after, str) else "")
    clean = re.sub(r'\x1B(?:\[[0-9;]*[a-zA-Z]|\(B|\[\?[0-9]*[a-zA-Z]|\**|[=>])?', '', raw)
    return clean

def sync_state(yt):
    yt.child.expect(r'20\d{2}')

def test_multi_column_rendering_metrics(ytree_binary, tmp_path):
    """
    REGRESSION: Columns overlap when metrics (max_filename_len) are not initialized correctly.
    Verifies that the file window correctly calculates columns even on first entry.
    """
    d = tmp_path / "layout_test"
    d.mkdir()
    # Create 50 files to force multi-column
    for i in range(50):
        (d / f"file_{i:02d}.txt").write_text("test")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))

    # Enter file window via 'S' (Global Mode)
    tui.send_keystroke(Keys.SHOWALL, wait=0.35)

    # Find a mode where short names visibly render in multiple columns.
    found_multi_column = False
    for _ in range(6):
        lines = tui.get_screen_dump()
        for line in lines[2:24]:
            if len(re.findall(r"\bfile_\d{2}\.txt\b", line)) >= 2:
                found_multi_column = True
                break
        if found_multi_column:
            break
        tui.send_keystroke("\x06", wait=0.35)  # Ctrl-F: Toggle Mode

    screen = "\n".join(tui.get_screen_dump())
    assert "FILE" in screen

    assert found_multi_column, "File window did not render short filenames across multiple columns"

    tui.quit()


def _has_two_short_file_columns(lines, split_x):
    if split_x is None:
        return False

    for line in lines[2:24]:
        left = line[:split_x]
        if len(re.findall(r"\b[a-z]\d{3}\.txt\b", left)) >= 2:
            return True
    return False


def _file_index(name):
    if not name:
        return None
    m = re.search(r"a(\d{3})\.txt", name)
    if not m:
        return None
    return int(m.group(1))


def _ensure_multi_column_layout(tui, split_x, max_toggles=5):
    for _ in range(max_toggles + 1):
        lines = tui.get_screen_dump()
        if _has_two_short_file_columns(lines, split_x):
            return lines
        tui.send_keystroke("\x06", wait=0.4)  # Ctrl-F: rotate file mode
    return None


def _detect_panel_split_x(lines):
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


def test_file_window_column_stride_sync_after_hidden_toggle(ytree_binary, tmp_path):
    """
    REGRESSION:
    When file-list width/metrics change without a mode switch, RIGHT/LEFT must
    keep using the current column geometry.

    This test uses a hidden extra-long dotfile to force a one-column layout
    after toggling dotfiles visible, then checks that RIGHT no longer jumps
    selection by a stale multi-column stride.
    """
    test_dir = tmp_path / "filewin_stride_sync"
    test_dir.mkdir()

    for i in range(60):
        (test_dir / f"a{i:03d}.txt").write_text("x", encoding="utf-8")

    long_hidden = "." + ("L" * 120) + ".txt"
    (test_dir / long_hidden).write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))

    # Make the window wide enough for multi-column rendering of short names.
    tui.child.setwinsize(36, 160)
    tui.screen.resize(36, 160)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.5)
    lines = tui.get_screen_dump()
    split_x = _detect_stats_split_x(lines)
    assert split_x is not None, "Could not detect file/stats split border"

    assert _has_two_short_file_columns(
        lines, split_x
    ), "Short-name file list should render using multiple columns"

    # Show dotfiles: the long hidden entry forces single-column geometry.
    tui.send_keystroke("`", wait=0.6)
    lines = tui.get_screen_dump()
    assert not _has_two_short_file_columns(
        lines, split_x
    ), "Long filename should collapse visible short-name columns"

    # Move off the long dotfile so we can compare numeric filename indices.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    before = _current_file_from_stats(tui.get_screen_dump(), split_x)
    before_idx = _file_index(before)
    assert before is not None and before_idx is not None, "Could not read selected short filename before RIGHT"

    # In one-column name-only mode, RIGHT/LEFT should behave like PgDn/PgUp.
    tui.send_keystroke(Keys.RIGHT, wait=0.4)
    after_right = _current_file_from_stats(tui.get_screen_dump(), split_x)
    after_right_idx = _file_index(after_right)
    assert (
        after_right is not None and after_right_idx is not None and
        after_right_idx > before_idx
    ), f"RIGHT should page in one-column layout (before={before}, after={after_right})"

    tui.send_keystroke(Keys.LEFT, wait=0.4)
    after_left = _current_file_from_stats(tui.get_screen_dump(), split_x)
    assert (
        after_left == before
    ), f"LEFT should reverse RIGHT paging in one-column layout (before={before}, after_left={after_left})"

    # Hide dotfiles again: short-name multi-column layout must be restored.
    tui.send_keystroke("`", wait=0.6)
    lines = tui.get_screen_dump()
    assert _has_two_short_file_columns(
        lines, split_x
    ), "File window did not restore multi-column layout for short filenames"

    tui.quit()


def test_split_file_details_do_not_wrap_neighbor_rows_at_120x36(ytree_binary, tmp_path):
    """
    Regression:
    At 120x36 in split file view, Ctrl-F detail modes must clip per-row output.
    Attributes/dates must not spill into adjacent lines (observed around cursor
    and bottom rows).
    """
    d = tmp_path / "split_file_detail_nowrap_120x36"
    d.mkdir()
    for i in range(60):
        # Long names force tighter detail rendering in split panes.
        (d / f"very_long_filename_{i:03d}_for_split_wrap_check.txt").write_text(
            "x", encoding="utf-8"
        )

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    tui.child.setwinsize(36, 120)
    tui.screen.resize(36, 120)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.5)  # file view
    tui.send_keystroke(Keys.F8, wait=0.5)     # split
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke("\x06", wait=0.3)      # Ctrl-F once (your repro)

    lines = tui.get_screen_dump()
    split_x = _detect_panel_split_x(lines)
    assert split_x is not None, "Could not detect split-panel separator."

    left_rows = [line[:split_x] for line in lines[2:28]]
    wrapped_rows = [
        row for row in left_rows
        if row.strip()
        and "No Files!" not in row
        and not re.match(r"^x  ", row)
    ]
    assert not wrapped_rows, (
        "File detail output wrapped into adjacent rows in split mode.\n"
        + "\n".join(left_rows)
    )

    tui.quit()


def test_file_window_left_right_edge_no_wrap(ytree_binary, tmp_path):
    """
    REGRESSION:
    LEFT/RIGHT must keep row semantics. At horizontal edges with no same-row
    target, cursor must stay put (no jump to first/last item).
    """
    test_dir = tmp_path / "filewin_left_right_edges"
    test_dir.mkdir()
    total_files = 50
    for i in range(total_files):
        (test_dir / f"a{i:03d}.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))
    tui.child.setwinsize(48, 160)
    tui.screen.resize(48, 160)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.5)
    lines = tui.get_screen_dump()
    split_x = _detect_stats_split_x(lines)
    assert split_x is not None, "Could not detect file/stats split border"

    lines = _ensure_multi_column_layout(tui, split_x)
    assert lines is not None, "Could not reach a multi-column file layout"

    # Move within first column, then verify LEFT at edge does nothing.
    for _ in range(5):
        tui.send_keystroke(Keys.DOWN, wait=0.12)
    left_edge_before = _current_file_from_stats(tui.get_screen_dump(), split_x)
    assert left_edge_before is not None, "Could not read file selection before LEFT-edge check"
    tui.send_keystroke(Keys.LEFT, wait=0.2)
    left_edge_after = _current_file_from_stats(tui.get_screen_dump(), split_x)
    assert (
        left_edge_after == left_edge_before
    ), f"LEFT at first column should not wrap (before={left_edge_before}, after={left_edge_after})"

    # Move to second column on the same row and capture the stride.
    tui.send_keystroke(Keys.RIGHT, wait=0.2)
    second_col_name = _current_file_from_stats(tui.get_screen_dump(), split_x)
    idx_first_col = _file_index(left_edge_after)
    idx_second_col = _file_index(second_col_name)
    assert idx_first_col is not None and idx_second_col is not None, "Could not parse file index from selection"
    assert idx_second_col > idx_first_col, "RIGHT should move to the next column on the same row"
    x_step = idx_second_col - idx_first_col

    # Choose a row that exists in column 2 but not column 3 (right edge).
    row_min = max(0, total_files - 2 * x_step)
    row_max = min(x_step - 1, total_files - x_step - 1)
    assert row_min <= row_max, "Test setup did not produce a right-edge row in second column"
    target_row = row_max
    target_idx = x_step + target_row

    if idx_second_col < target_idx:
        for _ in range(target_idx - idx_second_col):
            tui.send_keystroke(Keys.DOWN, wait=0.08)
    elif idx_second_col > target_idx:
        for _ in range(idx_second_col - target_idx):
            tui.send_keystroke(Keys.UP, wait=0.08)

    right_edge_before_name = _current_file_from_stats(tui.get_screen_dump(), split_x)
    right_edge_before = _file_index(right_edge_before_name)
    assert right_edge_before == target_idx, f"Expected selection index {target_idx}, got {right_edge_before}"

    tui.send_keystroke(Keys.RIGHT, wait=0.2)
    right_edge_after_name = _current_file_from_stats(tui.get_screen_dump(), split_x)
    right_edge_after = _file_index(right_edge_after_name)
    assert (
        right_edge_after == right_edge_before
    ), f"RIGHT at row edge should not jump/wrap (before={right_edge_before_name}, after={right_edge_after_name})"

    tui.quit()


def test_file_detail_rows_do_not_wrap_attributes_into_next_line(ytree_binary, tmp_path):
    """
    Regression guard:
    In narrow layouts, file-detail modes must clip row content instead of
    wrapping attributes/dates into the next visual line.
    """
    d = tmp_path / "file_detail_clip"
    d.mkdir()
    for i in range(12):
        (d / f"f{i:03d}.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    tui.child.setwinsize(32, 84)
    tui.screen.resize(32, 84)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.5)

    lines = tui.get_screen_dump()
    stats_split_x = _detect_stats_split_x(lines)
    assert stats_split_x is not None, "Could not detect file/stats split border"

    # Rotate into a detail-heavy mode that shows dates on file rows.
    for _ in range(6):
        screen = "\n".join(tui.get_screen_dump())
        if re.search(r"\d{4}-\d{2}-\d{2}", screen):
            break
        tui.send_keystroke("\x06", wait=0.35)  # Ctrl-F

    dump = tui.get_screen_dump()
    candidate_lines = [line[:stats_split_x] for line in dump[2:24]]
    date_no_name = [
        line for line in candidate_lines
        if re.search(r"\d{4}-\d{2}-\d{2}", line) and ".txt" not in line
    ]
    assert not date_no_name, (
        "Detail attributes/dates wrapped into continuation lines in narrow file view.\n"
        + "\n".join(candidate_lines)
    )

    tui.quit()


def test_file_window_one_column_edges_preserve_row(ytree_binary, tmp_path):
    """
    REGRESSION:
    In one-column (long-name) mode, LEFT/RIGHT page navigation must preserve
    the current row at top/bottom boundaries and must not snap to first/last
    file.
    """
    test_dir = tmp_path / "filewin_one_column_edges"
    test_dir.mkdir()

    total_files = 61
    for i in range(total_files):
        (test_dir / f"a{i:03d}.txt").write_text("x", encoding="utf-8")
    long_hidden = "." + ("L" * 120) + ".txt"
    (test_dir / long_hidden).write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))
    tui.child.setwinsize(36, 160)
    tui.screen.resize(36, 160)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.5)
    lines = tui.get_screen_dump()
    split_x = _detect_stats_split_x(lines)
    assert split_x is not None, "Could not detect file/stats split border"

    # Force one-column geometry by showing the long hidden dotfile.
    tui.send_keystroke("`", wait=0.6)
    lines = tui.get_screen_dump()
    assert not _has_two_short_file_columns(
        lines, split_x
    ), "Long filename should collapse visible short-name columns"

    # Move off the long dotfile and onto row 5.
    for _ in range(6):
        tui.send_keystroke(Keys.DOWN, wait=0.12)
    start_name = _current_file_from_stats(tui.get_screen_dump(), split_x)
    start_idx = _file_index(start_name)
    assert start_idx == 5, f"Expected start index 5, got {start_name}"

    # LEFT at top boundary must keep the same row/index (no snap to first file).
    tui.send_keystroke(Keys.LEFT, wait=0.3)
    after_left_top = _current_file_from_stats(tui.get_screen_dump(), split_x)
    after_left_top_idx = _file_index(after_left_top)
    assert (
        after_left_top_idx == start_idx
    ), f"LEFT at top boundary should preserve row (before={start_name}, after={after_left_top})"

    # Page to the bottom boundary with RIGHT until selection no longer changes.
    prev_idx = after_left_top_idx
    for _ in range(12):
        tui.send_keystroke(Keys.RIGHT, wait=0.25)
        cur_name = _current_file_from_stats(tui.get_screen_dump(), split_x)
        cur_idx = _file_index(cur_name)
        assert cur_idx is not None, "Could not parse selection index during RIGHT paging"
        if cur_idx == prev_idx:
            break
        prev_idx = cur_idx

    # At bottom boundary, one-column paging keeps row and does not snap to last.
    assert prev_idx < total_files - 1, (
        f"RIGHT paging in one-column mode snapped to last file ({prev_idx}); "
        "expected row-preserving boundary behavior"
    )
    tui.send_keystroke(Keys.RIGHT, wait=0.25)
    after_right_bottom = _current_file_from_stats(tui.get_screen_dump(), split_x)
    after_right_bottom_idx = _file_index(after_right_bottom)
    assert (
        after_right_bottom_idx == prev_idx
    ), f"RIGHT at bottom boundary should preserve row (before_idx={prev_idx}, after={after_right_bottom})"

    tui.quit()


def test_global_repeat_key_is_noop_in_global_view(ytree_binary, tmp_path):
    root = tmp_path / "global_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    tui.send_keystroke("g", wait=0.5)
    footer = _footer_text(tui)
    assert "global off" not in footer
    assert "to dir" in footer

    tui.send_keystroke("g", wait=0.5)
    footer = _footer_text(tui)
    assert "to dir" in footer, "G in global mode should be a no-op."

    tui.send_keystroke("\\", wait=0.5)
    footer = _footer_text(tui)
    assert "to dir" not in footer

    tui.quit()


def test_showall_repeat_key_sorts_in_showall_view(ytree_binary, tmp_path):
    root = tmp_path / "showall_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    footer = _footer_text(tui)
    assert "showall off" not in footer
    assert "to dir" in footer

    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    footer = _footer_text(tui)
    assert "sort by" in footer, "S in showall mode should trigger sort."
    tui.send_keystroke(Keys.ESC, wait=0.5)
    footer = _footer_text(tui)
    assert "to dir" in footer, "After dismissing sort prompt, stay in file view."

    tui.send_keystroke("\\", wait=0.5)
    footer = _footer_text(tui)
    assert "to dir" not in footer

    tui.quit()


def test_mutating_action_repeat_is_not_undo(ytree_binary, tmp_path):
    root = tmp_path / "mkdir_repeat_action"
    root.mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    tui.send_keystroke("M", wait=0.25)
    tui.send_keystroke("first_dir\r", wait=0.6)
    tui.send_keystroke("M", wait=0.25)
    tui.send_keystroke("second_dir\r", wait=0.6)
    tui.quit()

    assert (root / "first_dir").is_dir(), (
        "First mkdir action should persist after repeating the key."
    )
    assert (root / "second_dir").is_dir(), (
        "Second mkdir keypress must execute another create action, not undo."
    )


def test_showall_repeat_stays_in_showall_context(ytree_binary, tmp_path):
    root = tmp_path / "showall_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    # Move from root to alpha in tree mode and remember this start context.
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke("beta_only.txt\r", wait=0.5)

    # Repeat S in showall: should sort, not leave showall mode.
    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    footer = _footer_text(tui)
    assert "sort by" in footer
    tui.send_keystroke(Keys.ESC, wait=0.5)
    footer = _footer_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert "to dir" in footer
    assert "beta_only.txt" in screen

    tui.quit()


def test_global_repeat_stays_in_global_context(ytree_binary, tmp_path):
    root = tmp_path / "global_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    # Move from root to alpha in tree mode and remember this start context.
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.send_keystroke("g", wait=0.5)
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke("beta_only.txt\r", wait=0.5)

    # Repeat G in global-all-volumes mode: should be a no-op.
    tui.send_keystroke("g", wait=0.5)
    footer = _footer_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert "to dir" in footer
    assert "beta_only.txt" in screen

    tui.quit()


@pytest.mark.parametrize("mode_key", [Keys.SHOWALL, "g"])
def test_backslash_to_dir_in_showall_and_global(ytree_binary, tmp_path, mode_key):
    """
    REGRESSION:
    In Show All / Global file list modes, '\\' exits the mode and re-anchors
    the tree/file cursors to the selected file inside its owner directory.
    """
    root = tmp_path / "to_dir_mode"
    owner_dir = root / "owner_dir"
    other_dir = root / "other_dir"
    owner_dir.mkdir(parents=True)
    other_dir.mkdir(parents=True)

    target_name = "jump_target.txt"
    (owner_dir / target_name).write_text("x", encoding="utf-8")
    (other_dir / "other_file.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(mode_key, wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert "to dir" in screen, "Show All/Global footer should include '\\ to dir'"

    # Select the target file deterministically via filter.
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke(f"{target_name}\r", wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert target_name in screen, "Target file should be selected in global file list"

    tui.send_keystroke("\\", wait=0.7)
    lines = tui.get_screen_dump()
    screen = "\n".join(lines)
    split_x = _detect_stats_split_x(lines)
    jumped_current = _current_file_from_stats(lines, split_x)
    assert "DIR" in screen, "Expected to return to tree mode after '\\'"
    assert owner_dir.name in screen, "Expected header/tree context to include owner directory"
    assert target_name in screen, "Expected file cursor to land on selected file in owner directory"
    assert (
        jumped_current == target_name
    ), f"Expected CURRENT FILE to stay on selected target after '\\' (got {jumped_current})"

    tui.quit()


def test_footer_fkeys_render_as_text_in_dir_and_showall(ytree_binary, tmp_path):
    """
    REGRESSION:
    Footer command rows must render function key labels as text (F7/F8/F10/F1),
    not ACS glyph substitutions.
    """
    d = tmp_path / "footer_fkeys"
    d.mkdir()
    (d / "a.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    screen = "\n".join(tui.get_screen_dump())
    assert "F7" in screen and "F8" in screen and "F10" in screen and "F1" in screen
    assert "Treespec" not in screen
    assert "Tree  F1 help" in screen
    assert "Dir   F1 help" not in screen
    assert "jump" in screen
    assert "dotfiles" in screen
    assert "eXecute" in screen

    tui.send_keystroke(Keys.SHOWALL, wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert "F7" in screen and "F8" in screen and "F10" in screen and "F1" in screen
    assert "to dir" in screen
    assert "Dir   F1 help" in screen
    assert "jump" in screen
    assert "dotfiles" in screen
    assert "eXecute" in screen

    tui.quit()


def test_sort_prompt_uses_full_footer_without_bleed(ytree_binary, tmp_path):
    """
    REGRESSION:
    Sort prompt must fully occupy the footer area in file mode. The previous
    file footer row must not bleed through above SORT/COMMANDS lines.
    """
    d = tmp_path / "sort_footer_bleed"
    d.mkdir()
    (d / "b.txt").write_text("x", encoding="utf-8")
    (d / "a.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    # Enter file mode and open sort prompt.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("s", wait=0.4)

    lines = tui.get_screen_dump()
    footer = lines[-3:]

    assert "SORT by" in footer[0], "Footer line 1 should be owned by sort prompt"
    assert "COMMANDS" in footer[1], "Footer line 2 should be owned by sort prompt"
    assert "FILE" not in footer[0], "File footer must not bleed into sort prompt"
    assert "Attribute" not in footer[0], "File footer hints must not bleed into sort prompt"

    # Exit sort prompt cleanly.
    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()


@pytest.mark.parametrize(
    "action_key,new_name,confirm_text",
    [
        ("c", "dir_copy_out", "Copy directory now"),
        ("v", "dir_move_out", "Move directory now"),
    ],
)
def test_dir_copy_move_keeps_full_frame_after_command(
    ytree_binary, tmp_path, action_key, new_name, confirm_text
):
    root = tmp_path / "dir_ops_frame"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    # Select src_dir (first child of logged root in this fixture).
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    screen = "\n".join(tui.get_screen_dump())
    assert "src_dir" in screen

    tui.child.send(action_key)
    tui.child.expect("COPY:|MOVE:")
    tui.child.send("\x15")
    tui.child.send(f"{new_name}\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send(".\r")
    tui.child.expect(confirm_text)
    tui.child.send("Y")
    tui.send_keystroke("", wait=0.8)

    out_dir = root / new_name
    assert out_dir.exists() and out_dir.is_dir()
    assert (out_dir / "nested" / "payload.txt").exists()
    if action_key == "v":
        assert not src.exists()

    post = "\n".join(tui.get_screen_dump())
    assert "Tree  F1 help" in post, "Footer/help row disappeared after dir copy/move"
    assert "Path:" in post, "Header/border row disappeared after dir copy/move"

    tui.quit()


def test_jump_prompt_uses_footer_without_bleed(ytree_binary, tmp_path):
    """
    REGRESSION:
    List-jump prompt ('/') must render in the footer area without stale file
    footer text bleeding through.
    """
    d = tmp_path / "jump_footer_bleed"
    d.mkdir()
    (d / "alpha.txt").write_text("x", encoding="utf-8")
    (d / "beta.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    time.sleep(1.0)
    tui.send_keystroke("", wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("/", wait=0.4)

    lines = tui.get_screen_dump()
    footer = lines[-3:]
    footer_text = "\n".join(footer)

    assert "Jump to:" in footer_text, "Jump prompt should be visible in footer"
    assert "FILE" not in footer_text, "File footer should not bleed into jump prompt"
    assert "Attributes" not in footer_text, "Old footer hints should not bleed into jump prompt"

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()
