import pytest
import time
import re
import pexpect
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

def test_multi_column_rendering_metrics(controller, tmp_path):
    """
    REGRESSION: Columns overlap when metrics (max_filename_len) are not initialized correctly.
    Verifies that the file window correctly calculates columns even on first entry.
    """
    d = tmp_path / "layout_test"
    d.mkdir()
    # Create 50 files to force multi-column
    for i in range(50):
        (d / f"file_{i:02d}.txt").write_text("test")

    yt = controller(cwd=str(d))
    yt.wait_for_startup()
    
    # Enter file window via 'S' (Global Mode)
    yt.child.send(Keys.SHOWALL)
    sync_state(yt)
    
    # Cycle to MODE_3 (Name only)
    # MODE_0 -> MODE_1 -> MODE_2 -> MODE_3
    for _ in range(3):
        yt.child.send("\x06") # Ctrl-F: Toggle Mode
        time.sleep(0.5)
        sync_state(yt)

    screen = get_clean_screen(yt)
    assert "FILE" in screen
    
    # In MODE_3, 50 files with 120 width should definitely show file_49.txt
    # (around 4-5 columns)
    assert "file_49.txt" in screen
    
    yt.quit()


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
    if split_x is None:
        return None

    for i, line in enumerate(lines):
        right = line[split_x:]
        if "CURRENT FILE" in right and i + 1 < len(lines):
            next_right = lines[i + 1][split_x:]
            m = re.search(r"x\s+([^\s]+)", next_right)
            if m:
                return m.group(1)
            return None
    return None


def _has_two_short_file_columns(lines, split_x):
    if split_x is None:
        return False

    for line in lines[2:24]:
        left = line[:split_x]
        if len(re.findall(r"\ba\d{3}\.txt\b", left)) >= 2:
            return True
    return False


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
    before = _current_file_from_stats(lines, split_x)
    assert before is not None, "Could not read selected filename from stats panel"
    assert not _has_two_short_file_columns(
        lines, split_x
    ), "Long filename should collapse visible short-name columns"

    # In one-column mode, RIGHT should not jump to a different file.
    tui.send_keystroke(Keys.RIGHT, wait=0.4)
    after = _current_file_from_stats(tui.get_screen_dump(), split_x)
    assert (
        after == before
    ), f"RIGHT changed selection in one-column layout (before={before}, after={after})"

    # Hide dotfiles again: short-name multi-column layout must be restored.
    tui.send_keystroke("`", wait=0.6)
    lines = tui.get_screen_dump()
    assert _has_two_short_file_columns(
        lines, split_x
    ), "File window did not restore multi-column layout for short filenames"

    tui.quit()
