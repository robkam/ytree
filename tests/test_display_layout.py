import pytest
import time
import re
import pexpect
import io
from helpers_stats import current_file_from_stats as _current_file_from_stats
from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_lines as _footer_lines
from helpers_ui import footer_text as _footer_text
from ytnova_keys import Keys
from tui_harness import YtreeNovaTUI


def _select_tree_header_marker(tui, marker, timeout=3.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        lines = tui.get_screen_dump()
        if lines and marker in lines[0]:
            return lines
        lines = tui.send_and_wait_for_screen_change(
            Keys.DOWN, timeout=min(0.5, max(0.05, deadline - time.monotonic()))
        )
        if lines and marker in lines[0]:
            return lines
    screen = "\n".join(tui.get_screen_dump())
    pytest.fail(f"Could not select {marker!r}.\n{screen}")

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

def test_multi_column_rendering_metrics(ytnova_binary, tmp_path):
    """
    REGRESSION: Columns overlap when metrics (max_filename_len) are not initialized correctly.
    Verifies that the file window correctly calculates columns even on first entry.
    """
    d = tmp_path / "layout_test"
    d.mkdir()
    # Create 50 files to force multi-column
    for i in range(50):
        (d / f"file_{i:02d}.txt").write_text("test")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

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
        tui.send_keystroke("\x06", wait=0.35)  # C-f: Toggle Mode

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


def _move_to_file_index(tui, split_x, target_idx, key, timeout=3.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        current_idx = _file_index(_current_file_from_stats(tui.get_screen_dump(), split_x))
        if current_idx == target_idx:
            return current_idx
        lines = tui.send_and_wait_for_screen_change(
            key, timeout=min(0.5, max(0.05, deadline - time.monotonic()))
        )
        if lines:
            current_idx = _file_index(_current_file_from_stats(lines, split_x))
            if current_idx == target_idx:
                return current_idx
    pytest.fail(f"Could not select file index {target_idx}.")


def _ensure_multi_column_layout(tui, split_x, max_toggles=5):
    for _ in range(max_toggles + 1):
        lines = tui.get_screen_dump()
        if _has_two_short_file_columns(lines, split_x):
            return lines
        tui.send_keystroke("\x06", wait=0.4)  # C-f: rotate file mode
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


def test_file_window_column_stride_sync_after_hidden_toggle(ytnova_binary, tmp_path):
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

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir))

    # Make the window wide enough for multi-column rendering of short names.
    tui.child.setwinsize(36, 160)
    tui.screen.resize(36, 160)
    assert tui.wait_for_text(test_dir.name, timeout=2.0), "\n".join(tui.get_screen_dump())

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


def test_split_file_details_do_not_wrap_neighbor_rows_at_120x36(ytnova_binary, tmp_path):
    """
    Regression:
    At 120x36 in split file view, C-f detail modes must clip per-row output.
    Attributes/dates must not spill into adjacent lines (observed around cursor
    and bottom rows).
    """
    d = tmp_path / "split_file_detail_nowrap_120x36"
    d.mkdir()
    for i in range(60):
        # Long names force tighter detail rendering in split panels.
        (d / f"very_long_filename_{i:03d}_for_split_wrap_check.txt").write_text(
            "x", encoding="utf-8"
        )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(36, 120)
    tui.screen.resize(36, 120)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.5)  # file view
    tui.send_keystroke(Keys.F8, wait=0.5)     # split
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke("\x06", wait=0.3)      # C-f once (your repro)

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


def test_split_top_borders_show_current_filter(ytnova_binary, tmp_path):
    d = tmp_path / "split_filter_header"
    d.mkdir()
    for name in ("alpha.c", "beta.h", "gamma.txt"):
        (d / name).write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    assert tui.wait_for_text("Path:", timeout=2.0), "Initial header did not render."

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("f", wait=0.2)
    assert tui.wait_for_content("FILTER:", timeout=1.0), "Filter prompt missing."
    tui.send_keystroke(Keys.CTRL_U + "*.c,*.h" + Keys.ENTER, wait=0.5)

    lines = tui.send_and_wait_for_condition(
        Keys.F8,
        lambda dump: (
            dump
            if len(dump) > 1
            and _detect_panel_split_x(dump) is not None
            and "<*.c,*.h>" in dump[1][:_detect_panel_split_x(dump)]
            and "<*.c,*.h>" in dump[1][_detect_panel_split_x(dump) + 1 :]
            else False
        ),
        timeout=2.0,
    )
    assert lines, "Split header did not show the current filter on both borders."

    tui.quit()


def test_file_window_left_right_edge_no_wrap(ytnova_binary, tmp_path):
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

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir))
    tui.child.setwinsize(48, 160)
    tui.screen.resize(48, 160)
    assert tui.wait_for_text(test_dir.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.5)
    lines = tui.get_screen_dump()
    split_x = _detect_stats_split_x(lines)
    assert split_x is not None, "Could not detect file/stats split border"

    lines = _ensure_multi_column_layout(tui, split_x)
    assert lines is not None, "Could not reach a multi-column file layout"

    # Move within first column, then verify LEFT at edge does nothing.
    _move_to_file_index(tui, split_x, 5, Keys.DOWN)
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
        _move_to_file_index(tui, split_x, target_idx, Keys.DOWN)
    elif idx_second_col > target_idx:
        _move_to_file_index(tui, split_x, target_idx, Keys.UP)

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


def test_file_detail_rows_do_not_wrap_attributes_into_next_line(ytnova_binary, tmp_path):
    """
    Regression guard:
    In narrow layouts, file-detail modes must clip row content instead of
    wrapping attributes/dates into the next visual line.
    """
    d = tmp_path / "file_detail_clip"
    d.mkdir()
    for i in range(12):
        (d / f"f{i:03d}.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(32, 84)
    tui.screen.resize(32, 84)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.5)

    lines = tui.get_screen_dump()
    stats_split_x = _detect_stats_split_x(lines)
    assert stats_split_x is not None, "Could not detect file/stats split border"

    # Rotate into a detail-heavy mode that shows dates on file rows.
    for _ in range(6):
        screen = "\n".join(tui.get_screen_dump())
        if re.search(r"\d{4}-\d{2}-\d{2}", screen):
            break
        tui.send_keystroke("\x06", wait=0.35)  # C-f

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


def test_file_window_one_column_edges_preserve_row(ytnova_binary, tmp_path):
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

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir))
    tui.child.setwinsize(36, 160)
    tui.screen.resize(36, 160)
    assert tui.wait_for_text(test_dir.name, timeout=2.0), "\n".join(tui.get_screen_dump())

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
    _move_to_file_index(tui, split_x, 5, Keys.DOWN)
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

    # At the explicit bottom boundary, RIGHT must not wrap selection.
    lines = tui.send_and_wait_for_screen_change("\033OF", timeout=2.0)
    assert lines, "End did not select the bottom boundary"
    prev_idx = _file_index(_current_file_from_stats(lines, split_x))
    assert prev_idx is not None, "Could not parse selection index at bottom boundary"
    tui.send_keystroke(Keys.RIGHT, wait=0.25)
    after_right_bottom = _current_file_from_stats(tui.get_screen_dump(), split_x)
    after_right_bottom_idx = _file_index(after_right_bottom)
    assert (
        after_right_bottom_idx == prev_idx
    ), f"RIGHT at bottom boundary should preserve row (before_idx={prev_idx}, after={after_right_bottom})"

    tui.quit()


def test_global_repeat_key_is_noop_in_global_view(ytnova_binary, tmp_path):
    root = tmp_path / "global_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
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


def test_showall_repeat_key_sorts_in_showall_view(ytnova_binary, tmp_path):
    root = tmp_path / "showall_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
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


def test_mutating_action_repeat_is_not_undo(ytnova_binary, tmp_path):
    root = tmp_path / "mkdir_repeat_action"
    root.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        for name in ("first_dir", "second_dir"):
            created = root / name
            assert tui.send_and_wait_for_screen_change("M", timeout=2.0)
            assert tui.send_and_wait_for_condition(
                name + "\r",
                lambda lines: lines if created.is_dir() else False,
                timeout=2.0,
            ), f"mkdir action did not create {name}"
    finally:
        tui.quit()

    assert (root / "first_dir").is_dir(), (
        "First mkdir action should persist after repeating the key."
    )
    assert (root / "second_dir").is_dir(), (
        "Second mkdir keypress must execute another create action, not undo."
    )


def test_showall_repeat_stays_in_showall_context(ytnova_binary, tmp_path):
    root = tmp_path / "showall_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
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


def test_global_repeat_stays_in_global_context(ytnova_binary, tmp_path):
    root = tmp_path / "global_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
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
def test_backslash_to_dir_in_showall_and_global(ytnova_binary, tmp_path, mode_key):
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

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(mode_key, wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert "to dir" in screen, "Show All/Global footer should include '\\ to dir'"
    footer_rows = _footer_lines(tui)
    assert "/ jump" in footer_rows[1], (
        "Show All/Global file footer should keep / jump on the commands row.\n"
        f"{footer_rows[1]!r}"
    )
    assert "` dotfiles" in footer_rows[1], (
        "Show All/Global file footer should keep ` dotfiles on the commands row.\n"
        f"{footer_rows[1]!r}"
    )
    assert "\\ to dir" in footer_rows[2], (
        "Show All/Global footer should render backslash to dir with a separator space.\n"
        f"{footer_rows[2]!r}"
    )
    assert "\\to dir" not in footer_rows[2], (
        "Show All/Global footer must not collapse the backslash-to-dir label.\n"
        f"{footer_rows[2]!r}"
    )
    assert footer_rows[2][3:].startswith(" Tree F1 help"), (
        "Show All/Global footer should use a single space between the nav glyphs and Tree.\n"
        f"{footer_rows[2]!r}"
    )
    assert not footer_rows[2][3:].startswith("  Tree"), (
        "Show All/Global footer must not double-space after the nav glyphs.\n"
        f"{footer_rows[2]!r}"
    )
    assert "1..9 file view" in footer_rows[0], "\n".join(footer_rows)
    assert "Newfile" in "\n".join(footer_rows), "\n".join(footer_rows)
    assert "F1 help" in footer_rows[2], "\n".join(footer_rows)

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


def test_footer_fkeys_render_as_text_in_dir_and_showall(ytnova_binary, tmp_path):
    """
    REGRESSION:
    Footer command rows must render function key labels as text (F7/F8/F10/F1),
    not ACS glyph substitutions.
    """
    d = tmp_path / "footer_fkeys"
    d.mkdir()
    (d / "a.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    screen = "\n".join(tui.get_screen_dump())
    assert "F7" in screen and "F8" in screen and "F10" in screen and "F1" in screen
    assert "Treespec" not in screen
    assert "File F1 help" in screen
    assert "Tree F1 help" not in screen
    assert "jump" in screen
    assert "dotfiles" in screen

    tui.send_keystroke(Keys.SHOWALL, wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert "F7" in screen and "F8" in screen and "F10" in screen and "F1" in screen
    assert "to dir" in screen
    assert "Tree F1 help" in screen
    assert "jump" in screen
    assert "dotfiles" in screen

    tui.quit()


def test_sort_prompt_uses_full_footer_without_bleed(ytnova_binary, tmp_path):
    """
    REGRESSION:
    Sort prompt must fully occupy the footer area in file mode. The previous
    file footer row must not bleed through above SORT/COMMANDS lines.
    """
    d = tmp_path / "sort_footer_bleed"
    d.mkdir()
    (d / "b.txt").write_text("x", encoding="utf-8")
    (d / "a.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

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
    "action_key,new_name,stale_confirm_text",
    [
        ("c", "dir_copy_out", "Copy directory now"),
        ("v", "dir_move_out", "Move directory now"),
    ],
)
def test_dir_copy_move_keeps_full_frame_after_command(
    ytnova_binary, tmp_path, action_key, new_name, stale_confirm_text
):
    root = tmp_path / "dir_ops_frame"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

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
    raw_output = io.StringIO()
    tui.child.logfile_read = raw_output
    tui.child.send(".\r")
    out_dir = root / new_name
    created = tui.wait_for_condition(
        lambda _screen: out_dir if out_dir.exists() and out_dir.is_dir() else False,
        timeout=2.0,
    )
    assert created, "\n".join(tui.get_screen_dump())
    command_output = raw_output.getvalue()
    tui.child.logfile_read = None
    assert "\x1b[?1049l" not in command_output, command_output
    assert "\x1b[?1049h" not in command_output, command_output
    assert "\x1b[H\x1b[2J" not in command_output, command_output
    assert stale_confirm_text not in "\n".join(tui.get_screen_dump())
    assert (out_dir / "nested" / "payload.txt").exists()
    if action_key == "v":
        assert not src.exists()

    restored = tui.wait_for_condition(
        lambda lines: lines
        if "Path:" in "\n".join(lines) and "File F1 help" in "\n".join(lines)
        else False,
        timeout=2.0,
    )
    assert restored, "\n".join(tui.get_screen_dump())
    post = "\n".join(restored)
    if action_key == "c":
        assert new_name in post, (
            "Directory copy did not keep the new directory visible in-session.\n"
            f"{post}"
        )
    assert "File F1 help" in post, "Footer keybinding row disappeared after dir copy/move"
    assert "Path:" in post, "Header/border row disappeared after dir copy/move"

    tui.quit()


def test_dir_copy_to_missing_destination_decline_reopens_prompt_without_frame_corruption(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_copy_missing_dest_no"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("./new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("N")

    settled_lines = tui.wait_for_condition(
        lambda lines: lines
        if (
            "To Directory:" in "\n".join(lines)
            or "F1 help" in "\n".join(lines)
            or "Enter OK" in "\n".join(lines)
        )
        else False,
        timeout=1.5,
    )
    assert settled_lines, "\n".join(tui.get_screen_dump())

    assert not (root / "new_parent").exists()
    assert (root / "src_dir" / "nested" / "payload.txt").exists()

    post = "\n".join(settled_lines)
    footer = _footer_text(tui).lower()
    assert "Path:" in post, "Header/path row disappeared after canceling create prompt"
    prompt_reopened = "To Directory:" in post
    prompt_footer_ok = "enter ok" in footer and "esc cancel" in footer
    file_footer_ok = "tree" in footer and "help" in footer
    assert prompt_footer_ok or file_footer_ok, (
        "Footer keybinding row was corrupted after canceling create prompt.\n"
        f"Footer:\n{footer}\n\nScreen:\n{post}"
    )
    if prompt_reopened:
        assert prompt_footer_ok, (
            "Destination prompt returned without its expected footer controls.\n"
            f"Footer:\n{footer}\n\nScreen:\n{post}"
        )
    else:
        assert file_footer_ok, (
            "File view returned without its expected footer keybinding row.\n"
            f"Footer:\n{footer}\n\nScreen:\n{post}"
        )

    tui.quit()


def test_dir_copy_to_missing_destination_create_yes_copies_and_restores_frame(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_copy_missing_dest_yes"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("./new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete after confirming create"
    assert "Copy directory now" not in "\n".join(tui.get_screen_dump())

    post = "\n".join(tui.get_screen_dump())
    footer = _footer_text(tui).lower()
    assert "Path:" in post, "Header/path row disappeared after create+copy flow"
    assert "tree" in footer and "f1" in footer and "help" in footer, (
        "Footer keybinding row was not restored after create+copy flow.\n"
        f"Footer:\n{footer}\n\nScreen:\n{post}"
    )

    tui.quit()


def test_dir_copy_prompt_shows_source_and_as_target(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_prompt_as"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect(r"COPY:\s+src_dir\s+AS:", timeout=2.0)
    tui.child.send(Keys.ESC)
    tui.send_keystroke("", wait=0.2)
    tui.quit()


def test_file_move_prompt_shows_source_and_as_target(ytnova_binary, tmp_path):
    root = tmp_path / "file_move_prompt_as"
    root.mkdir()
    (root / "src.txt").write_text("payload\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_condition(
        lambda lines: any("1..9 file view" in line for line in lines[-3:]),
        timeout=1.5,
        poll_interval=0.05,
    ), "\n".join(tui.get_screen_dump())
    tui.child.send("m")
    tui.child.expect(r"MOVE:\s+src\.txt\s+AS:", timeout=2.0)
    tui.child.send(Keys.ESC)
    tui.send_keystroke("", wait=0.2)
    tui.quit()


def test_dir_copy_refreshes_destination_branch_without_relog(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_cross_branch_refresh"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    # root -> source_bucket -> src_dir
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("../target_bucket/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete"
    assert "Copy directory now" not in "\n".join(tui.get_screen_dump())

    # Move to target_bucket and verify the copied branch is visible immediately.
    _select_tree_header_marker(tui, "target_bucket")

    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_target_enter = "\n".join(tui.get_screen_dump())
    assert "new_parent" in after_target_enter, (
        "Created destination directory is not visible in-session after copy "
        "(requires relog today).\n"
        f"{after_target_enter}"
    )

    _select_tree_header_marker(tui, "new_parent")

    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_new_parent_enter = "\n".join(tui.get_screen_dump())
    assert "copied_src" in after_new_parent_enter, (
        "Copied subtree is not visible immediately after destination creation.\n"
        f"{after_new_parent_enter}"
    )

    tui.quit()


def test_dir_copy_delete_created_destination_updates_in_session(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_delete_created_destination"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("../target_bucket/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete"
    assert "Copy directory now" not in "\n".join(tui.get_screen_dump())

    # Navigate to created destination directory and delete it.
    _select_tree_header_marker(tui, "target_bucket")
    tui.send_keystroke(Keys.ENTER, wait=0.6)

    _select_tree_header_marker(tui, "new_parent")

    tui.child.send(Keys.DELETE)
    tui.child.expect(r"(Delete this directory|PRUNE)", timeout=2.0)
    tui.child.send("Y")
    tui.send_keystroke("", wait=0.8)

    assert not (root / "target_bucket" / "new_parent").exists(), (
        "Deleting the created destination directory had no filesystem effect."
    )
    after_delete = "\n".join(tui.get_screen_dump())
    assert "new_parent" not in after_delete, (
        "Created destination directory still rendered after in-session delete.\n"
        f"{after_delete}"
    )

    tui.quit()


def test_dir_copy_absolute_destination_refreshes_without_relog(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_absolute_destination_refresh"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send(f"{target_bucket}/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy to absolute destination did not complete"
    assert "Copy directory now" not in "\n".join(tui.get_screen_dump())

    _select_tree_header_marker(tui, "target_bucket")
    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_target_enter = "\n".join(tui.get_screen_dump())
    assert "new_parent" in after_target_enter, (
        "Absolute-path destination not visible in-session after copy.\n"
        f"{after_target_enter}"
    )

    tui.quit()


def test_jump_prompt_uses_footer_without_bleed(ytnova_binary, tmp_path):
    """
    REGRESSION:
    List-jump prompt ('/') must render in the footer area without stale file
    footer text bleeding through.
    """
    d = tmp_path / "jump_footer_bleed"
    d.mkdir()
    (d / "alpha.txt").write_text("x", encoding="utf-8")
    (d / "beta.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

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
