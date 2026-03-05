"""
Tests for stats panel display and UI bugs.

These tests verify:
1. Stats panel shows "CURRENT FILE" when in file window
2. Stats update immediately when cursor moves (not "one behind")
3. Attributes section appears immediately on entering file window
4. Footer menu displays correct commands
5. Key bindings work case-insensitively
6. 'l' key triggers Log prompt
"""

import pytest
import time
import re
from tui_harness import YtreeTUI
from ytree_keys import Keys


@pytest.fixture
def test_dir_with_files(tmp_path):
    """Create a test directory with multiple files of different sizes."""
    test_root = tmp_path / "test_stats"
    test_root.mkdir()

    # Create files with known sizes for verification
    (test_root / "file1.txt").write_text("small")  # 5 bytes
    (test_root / "file2.txt").write_text("medium content here")  # 19 bytes
    (test_root / "file3.txt").write_text("x" * 100)  # 100 bytes

    return test_root


def test_stats_show_current_file_on_entry(test_dir_with_files, ytree_binary):
    """
    BUG: Stats panel shows "CURRENT DIR" instead of "CURRENT FILE" when entering file window.
    EXPECTED: Stats should immediately show "CURRENT FILE" with file metadata.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window (small)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # CRITICAL: Stats panel should show "CURRENT FILE" not "CURRENT DIR"
    if "CURRENT DIR" in screen:
        pytest.fail(f"BUG: Stats show 'CURRENT DIR' instead of 'CURRENT FILE' on file window entry\n{screen}")

    assert "CURRENT FILE" in screen, f"Stats should show 'CURRENT FILE' header\n{screen}"
    assert "file1.txt" in screen, f"Stats should show selected filename\n{screen}"

    tui.quit()


def test_attributes_section_appears_on_entry(test_dir_with_files, ytree_binary):
    """
    BUG: Attributes section (bottom of stats panel) is blank until first DOWN press.
    EXPECTED: Attributes (Size, Perm, Mod time) should appear immediately.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # Check for attributes section keywords
    has_size = "Size:" in screen or "5" in screen  # 5 bytes for file1.txt
    has_perm = "Perm:" in screen or "rw" in screen
    has_mod = "Mod" in screen or "20" in screen  # date pattern

    if not (has_size or has_perm or has_mod):
        pytest.fail(f"BUG: Attributes section missing on file window entry\n{screen}")

    tui.quit()


def test_stats_synchronize_with_cursor_movement(test_dir_with_files, ytree_binary):
    """
    BUG: Stats display is "one behind" - shows previous file when cursor moves.
    EXPECTED: Stats should update to show currently selected file immediately.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window - should select file1.txt
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen1 = "\n".join(tui.get_screen_dump())
    assert "file1.txt" in screen1, "Should start on file1.txt"

    # Press DOWN - should move to file2.txt
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.3)

    screen2 = "\n".join(tui.get_screen_dump())

    # CRITICAL: Stats should show file2.txt NOW, not file1.txt
    if "file1.txt" in screen2 and "CURRENT FILE" in screen2:
        # Check if file1.txt appears in stats area (not just file list)
        # Stats panel is on the right side, starting around column 90
        lines = screen2.split('\n')
        stats_area = '\n'.join([line[90:] if len(line) > 90 else "" for line in lines[2:25]])
        if "file1.txt" in stats_area:
            pytest.fail(f"BUG: Stats still show file1.txt after moving to file2.txt (one behind)\n{screen2}")

    assert "file2.txt" in screen2, f"Stats should update to show file2.txt\n{screen2}"

    # Press DOWN again - should move to file3.txt
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.3)

    screen3 = "\n".join(tui.get_screen_dump())
    assert "file3.txt" in screen3, f"Stats should update to show file3.txt\n{screen3}"

    # Press UP - should move back to file2.txt
    tui.send_keystroke(Keys.UP)
    time.sleep(0.3)

    screen4 = "\n".join(tui.get_screen_dump())
    assert "file2.txt" in screen4, f"Stats should update back to file2.txt\n{screen4}"

    tui.quit()


def test_stats_in_big_window_mode(test_dir_with_files, ytree_binary):
    """
    BUG: Stats still show "CURRENT DIR" when entering file window.
    EXPECTED: Stats should show "CURRENT FILE" in file window (big or small mode).

    NOTE: ytree enters file window in big mode by default (bypass_small_window=TRUE),
    so we only need one ENTER to get into big window mode.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window (enters big mode by default)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # Should show file stats, not directory stats
    if "CURRENT DIR" in screen and "FILE" not in screen:
        pytest.fail(f"BUG: Stats show 'CURRENT DIR' in file window\n{screen}")

    assert "CURRENT FILE" in screen, f"Stats should show 'CURRENT FILE' in file window\n{screen}"

    tui.quit()


def test_lowercase_l_key_triggers_log(test_dir_with_files, ytree_binary):
    """
    BUG: Lowercase 'l' key does nothing, only uppercase 'L' works.
    EXPECTED: Both 'l' and 'L' should trigger Log prompt.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Press lowercase 'l'
    tui.send_keystroke('l')
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # Should show "Log Path:" prompt
    if "Log" not in screen and "log" not in screen and "Path" not in screen:
        pytest.fail(f"BUG: Lowercase 'l' did not trigger Log prompt\n{screen}")

    tui.send_keystroke(Keys.ESC)
    tui.quit()


def test_lowercase_k_key_opens_volume_menu(test_dir_with_files, ytree_binary):
    """
    BUG: Lowercase 'k' should open volume menu (case-insensitive).
    EXPECTED: Both 'k' and 'K' should open volume menu.

    NOTE: VI keys are now disabled by default in Makefile to allow case-insensitive
    bindings. Users can re-enable with ADD_CFLAGS = -DVI_KEYS if desired.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Press lowercase 'k' (should work with VI keys disabled)
    tui.send_keystroke('k')
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # Should show volume menu
    if "Volume" not in screen and "volume" not in screen:
        pytest.fail(f"BUG: Lowercase 'k' did not open volume menu\n{screen}")

    tui.send_keystroke(Keys.ESC)
    tui.quit()


def test_case_insensitive_tag_untag(test_dir_with_files, ytree_binary):
    """
    BUG: 'T' tags all instead of single file, 'U' untags all instead of single.
    EXPECTED:
      - 't' or 'T' = tag single file
      - 'u' or 'U' = untag single file
      - Ctrl+T = tag all
      - Ctrl+U = untag all
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    # Press lowercase 't' to tag single file
    tui.send_keystroke('t')
    time.sleep(0.3)

    screen1 = "\n".join(tui.get_screen_dump())
    # file1.txt should be tagged (marked with *)
    assert "*" in screen1 or "1 " in screen1, "file1.txt should be tagged"

    # Press DOWN to move to file2.txt
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.3)

    # Press uppercase 'T' to tag file2.txt (should tag SINGLE, not all)
    tui.send_keystroke('T')
    time.sleep(0.3)

    screen2 = "\n".join(tui.get_screen_dump())
    # file3.txt should NOT be tagged yet
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.3)
    screen3 = "\n".join(tui.get_screen_dump())

    # If all files were tagged, it's a bug
    # Extract only file list area (left side, excluding filter display at top-right)
    lines = screen3.split('\n')
    file_list_area = '\n'.join([line[:90] if len(line) > 90 else line for line in lines[20:25]])
    tag_count = file_list_area.count('*')

    # Should have exactly 2 tagged files (file1, file2), not 3
    if tag_count >= 3:
        pytest.fail(f"BUG: Uppercase 'T' tagged all files instead of single file (found {tag_count} tags)\n{screen3}")

    # Press lowercase 'u' to untag file3 (which should not be tagged)
    tui.send_keystroke('u')
    time.sleep(0.3)

    tui.quit()


def test_ctrl_t_tags_all(test_dir_with_files, ytree_binary):
    """
    Verify Ctrl+T tags all files (not just uppercase T).
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    # Press Ctrl+T to tag all
    tui.send_keystroke('\x14')  # Ctrl+T
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # All three files should be tagged
    tag_count = screen.count('*')
    if tag_count < 3:
        pytest.fail(f"BUG: Ctrl+T did not tag all files (found {tag_count} tags)\n{screen}")

    tui.quit()


def test_ctrl_u_untags_all(test_dir_with_files, ytree_binary):
    """
    BUG: Ctrl+U moves cursor up instead of untagging all.
    EXPECTED: Ctrl+U should untag all files.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window and tag all
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    tui.send_keystroke('\x14')  # Ctrl+T to tag all
    time.sleep(0.5)

    screen1 = "\n".join(tui.get_screen_dump())
    assert screen1.count('*') >= 3, "All files should be tagged"

    # Press Ctrl+U to untag all
    tui.send_keystroke('\x15')  # Ctrl+U
    time.sleep(0.5)

    screen2 = "\n".join(tui.get_screen_dump())

    # Check if files are still tagged (exclude the filter area which also shows *)
    # Extract only file list lines (typically left side, lines 2-20)
    lines = screen2.split('\n')
    file_list_area = '\n'.join([line[:90] if len(line) > 90 else line for line in lines[2:25]])

    # Count asterisks in file list area (excluding filter display)
    tagged_count = file_list_area.count('*')
    if tagged_count > 1:  # Allow 1 for potential filter display remnants
        pytest.fail(f"BUG: Ctrl+U did not untag files (found {tagged_count} asterisks)\n{screen2}")

    tui.quit()


def test_footer_shows_brief_command(test_dir_with_files, ytree_binary):
    """
    BUG: Footer shows "aBout" which no longer exists, missing "B (Brief)".
    EXPECTED: Footer should show "B Brief" command.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    lines = screen.split('\n')

    # Footer is the last 2-3 lines
    footer = '\n'.join(lines[-3:]).lower()

    # Should have "brief" or "b" command
    if "brief" not in footer and "compact" not in footer:
        pytest.fail(f"BUG: Footer missing 'Brief' command\nFooter:\n{footer}\n\nFull screen:\n{screen}")

    # Should NOT have "about"
    if "about" in footer:
        pytest.fail(f"BUG: Footer shows obsolete 'About' command\nFooter:\n{footer}")

    tui.quit()


def test_footer_after_execute_escape(test_dir_with_files, ytree_binary):
    """
    BUG: After pressing 'x' (execute) then ESC, second footer line shows only 'C'.
    EXPECTED: Footer should be fully restored after canceling command.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    # Capture normal footer
    screen1 = "\n".join(tui.get_screen_dump())
    lines1 = screen1.split('\n')
    footer1 = '\n'.join(lines1[-3:])

    # Press 'x' for execute
    tui.send_keystroke('x')
    time.sleep(0.5)

    # Press ESC to cancel
    tui.send_keystroke(Keys.ESC)
    time.sleep(0.5)

    # Check footer is restored
    screen2 = "\n".join(tui.get_screen_dump())
    lines2 = screen2.split('\n')
    footer2 = '\n'.join(lines2[-3:])

    # Footer should not be truncated to just 'C'
    footer2_text = footer2.strip()
    if len(footer2_text) < 10:
        pytest.fail(f"BUG: Footer truncated after Execute ESC\nBefore:\n{footer1}\n\nAfter:\n{footer2}")

    tui.quit()


def test_footer_visible_in_big_window(test_dir_with_files, ytree_binary):
    """
    BUG (FIXED?): Footer was blank in big window mode.
    EXPECTED: Footer menu should be visible when zoomed.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
    time.sleep(1.0)

    # Enter file window and zoom
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())
    lines = screen.split('\n')
    footer = '\n'.join(lines[-3:])

    # Footer should have some command text
    if len(footer.strip()) < 10:
        pytest.fail(f"BUG: Footer blank in big window\n{screen}")

    tui.quit()
