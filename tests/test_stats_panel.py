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
import subprocess
import re
import os
from helpers_stats import (
    current_file_from_stats as _current_file_from_stats,
    detect_stats_split_x as _detect_stats_split_x,
)
from helpers_ui import footer_text_from_lines as _footer_text_from_lines
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


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


def _screen_lines(screen_or_lines):
    if isinstance(screen_or_lines, str):
        return screen_or_lines.split("\n")
    return list(screen_or_lines)


def _screen_text(screen_or_lines):
    return "\n".join(_screen_lines(screen_or_lines))


def _send_and_wait(tui, keys, timeout=1.0):
    before = tui.get_screen_dump()
    lines = tui.send_and_wait_for_condition(
        keys,
        lambda current_lines: current_lines
        if current_lines != before
        else False,
        timeout=timeout,
    )
    return lines or tui.get_screen_dump()


def _footer_text(screen_or_lines):
    return _footer_text_from_lines(_screen_lines(screen_or_lines))


def _stats_area(screen_or_lines):
    lines = _screen_lines(screen_or_lines)
    split_x = _detect_stats_split_x(lines)
    if split_x is None:
        return []

    stats_lines = []
    for line in lines[:-3]:
        segment = line[split_x:].strip()
        if not segment:
            continue
        stats_lines.append(segment)
    return stats_lines


def _stats_view_value(screen_or_lines):
    view_line = _line_with_text(_stats_area(screen_or_lines), "View:")
    return view_line.split("View:", 1)[1].split("x", 1)[0].strip()


def _current_file_name(screen_or_lines):
    lines = _screen_lines(screen_or_lines)
    return _current_file_from_stats(lines, _detect_stats_split_x(lines))


def _file_lines_with_names(screen_or_lines, *names):
    return [
        line
        for line in _screen_lines(screen_or_lines)
        if any(name in line for name in names)
    ]


def _tagged_file_count(screen_or_lines, *names):
    count = 0
    for line in _file_lines_with_names(screen_or_lines, *names):
        for name in names:
            if name in line and "*" in line.split(name, 1)[0]:
                count += 1
                break
    return count


def test_stats_show_current_file_on_entry(test_dir_with_files, ytnova_binary):
    """
    BUG: Stats panel shows "CURRENT DIR" instead of "CURRENT FILE" when entering file window.
    EXPECTED: Stats should immediately show "CURRENT FILE" with file metadata.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window (small)
    lines = _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    screen = _screen_text(lines)
    stats_text = "\n".join(_stats_area(lines))

    # CRITICAL: Stats panel should show "CURRENT FILE" not "CURRENT DIR"
    if "CURRENT DIR" in stats_text:
        pytest.fail(f"BUG: Stats show 'CURRENT DIR' instead of 'CURRENT FILE' on file window entry\n{screen}")

    assert "CURRENT FILE" in stats_text, f"Stats should show 'CURRENT FILE' header\n{screen}"
    assert _current_file_name(lines) == "file1.txt", (
        f"Stats should show file1.txt as the selected file.\n{screen}"
    )

    tui.quit()


def test_attributes_section_appears_on_entry(test_dir_with_files, ytnova_binary):
    """
    BUG: Attributes section (bottom of stats panel) is blank until first DOWN press.
    EXPECTED: Attributes (Size, Perm, Mod time) should appear immediately.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window
    lines = _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    screen = _screen_text(lines)
    stats_text = "\n".join(_stats_area(lines))

    # Check for attributes section keywords
    has_size = "Size:" in stats_text or "5" in stats_text
    has_perm = "Perm:" in stats_text or "rw" in stats_text
    has_mod = "Mod" in stats_text or "20" in stats_text

    if not (has_size or has_perm or has_mod):
        pytest.fail(f"BUG: Attributes section missing on file window entry\n{screen}")

    tui.quit()


def test_stats_synchronize_with_cursor_movement(test_dir_with_files, ytnova_binary):
    """
    BUG: Stats display is "one behind" - shows previous file when cursor moves.
    EXPECTED: Stats should update to show currently selected file immediately.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window - should select file1.txt
    lines = _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    screen1 = _screen_text(lines)
    assert _current_file_name(lines) == "file1.txt", "Should start on file1.txt"

    # Press DOWN - should move to file2.txt
    lines = tui.send_and_wait_for_condition(
        Keys.DOWN,
        lambda current_lines: current_lines
        if _current_file_name(current_lines) == "file2.txt"
        else False,
        timeout=1.0,
    )
    assert lines, "Stats should update to show file2.txt immediately"
    screen2 = _screen_text(lines)

    # Press DOWN again - should move to file3.txt
    lines = tui.send_and_wait_for_condition(
        Keys.DOWN,
        lambda current_lines: current_lines
        if _current_file_name(current_lines) == "file3.txt"
        else False,
        timeout=1.0,
    )
    assert lines, "Stats should update to show file3.txt immediately"
    screen3 = _screen_text(lines)

    # Press UP - should move back to file2.txt
    lines = tui.send_and_wait_for_condition(
        Keys.UP,
        lambda current_lines: current_lines
        if _current_file_name(current_lines) == "file2.txt"
        else False,
        timeout=1.0,
    )
    assert lines, "Stats should update back to file2.txt immediately"
    screen4 = _screen_text(lines)

    tui.quit()


def test_stats_in_big_window_mode(test_dir_with_files, ytnova_binary):
    """
    BUG: Stats still show "CURRENT DIR" when entering file window.
    EXPECTED: Stats should show "CURRENT FILE" in file window (big or small mode).

    NOTE: ytnova enters file window in big mode by default (bypass_small_window=TRUE),
    so we only need one ENTER to get into big window mode.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window (enters big mode by default)
    lines = _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    screen = _screen_text(lines)
    stats_text = "\n".join(_stats_area(lines))

    # Should show file stats, not directory stats
    if "CURRENT DIR" in stats_text and "FILE" not in stats_text:
        pytest.fail(f"BUG: Stats show 'CURRENT DIR' in file window\n{screen}")

    assert "CURRENT FILE" in stats_text, f"Stats should show 'CURRENT FILE' in file window\n{screen}"

    tui.quit()


def test_lowercase_l_key_triggers_log(test_dir_with_files, ytnova_binary):
    """
    BUG: Lowercase 'l' key does nothing, only uppercase 'L' works.
    EXPECTED: Both 'l' and 'L' should trigger Log prompt.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Press lowercase 'l'
    lines = _send_and_wait(tui, 'l', timeout=0.5)
    screen = _screen_text(lines)

    # Should show "Log Path:" prompt
    if "Log" not in screen and "log" not in screen and "Path" not in screen:
        pytest.fail(f"BUG: Lowercase 'l' did not trigger Log prompt\n{screen}")

    _send_and_wait(tui, Keys.ESC, timeout=0.3)
    tui.quit()


def test_lowercase_k_key_opens_volume_menu(test_dir_with_files, ytnova_binary):
    """
    BUG: Lowercase 'k' should open volume menu (case-insensitive).
    EXPECTED: Both 'k' and 'K' should open volume menu.

    NOTE: VI keys are runtime-configurable (`VI_KEYS=0/1` in profile). With
    default `VI_KEYS=0`, lowercase bindings remain case-insensitive.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Press lowercase 'k' (should work with VI keys disabled)
    lines = _send_and_wait(tui, 'k', timeout=0.5)
    screen = _screen_text(lines)

    # Should show volume menu
    if "Volume" not in screen and "volume" not in screen:
        pytest.fail(f"BUG: Lowercase 'k' did not open volume menu\n{screen}")

    _send_and_wait(tui, Keys.ESC, timeout=0.3)
    tui.quit()


def test_case_insensitive_tag_untag(test_dir_with_files, ytnova_binary):
    """
    BUG: 'T' tags all instead of single file, 'U' untags all instead of single.
    EXPECTED:
      - 't' or 'T' = tag single file
      - 'u' or 'U' = untag single file
      - Ctrl+T = tag all
      - Ctrl+U = untag all
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    # Press lowercase 't' to tag single file
    _send_and_wait(tui, 't', timeout=0.3)

    lines = tui.get_screen_dump()
    assert _tagged_file_count(lines, "file1.txt") == 1, "file1.txt should be tagged"

    # Press DOWN to move to file2.txt
    _send_and_wait(tui, Keys.DOWN, timeout=0.3)

    # Press uppercase 'T' to tag file2.txt (should tag SINGLE, not all)
    _send_and_wait(tui, 'T', timeout=0.3)

    _send_and_wait(tui, Keys.DOWN, timeout=0.3)
    screen3 = tui.get_screen_dump()
    tag_count = _tagged_file_count(screen3, "file1.txt", "file2.txt", "file3.txt")

    # Should have exactly 2 tagged files (file1, file2), not 3
    if tag_count >= 3:
        pytest.fail(
            "BUG: Uppercase 'T' tagged all files instead of a single file "
            f"(found {tag_count} tags)\n{_screen_text(screen3)}"
        )

    # Press lowercase 'u' to untag file3 (which should not be tagged)
    _send_and_wait(tui, 'u', timeout=0.3)

    tui.quit()


def test_ctrl_t_tags_all(test_dir_with_files, ytnova_binary):
    """
    Verify Ctrl+T tags all files (not just uppercase T).
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    # Press Ctrl+T to tag all
    screen = _send_and_wait(tui, '\x14', timeout=0.5)

    # All three files should be tagged
    tag_count = _tagged_file_count(screen, "file1.txt", "file2.txt", "file3.txt")
    if tag_count < 3:
        pytest.fail(
            f"BUG: Ctrl+T did not tag all files (found {tag_count} tags)\n{_screen_text(screen)}"
        )

    tui.quit()


def test_ctrl_u_untags_all(test_dir_with_files, ytnova_binary):
    """
    BUG: Ctrl+U moves cursor up instead of untagging all.
    EXPECTED: Ctrl+U should untag all files.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window and tag all
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    screen1 = _send_and_wait(tui, '\x14', timeout=0.5)
    assert _tagged_file_count(screen1, "file1.txt", "file2.txt", "file3.txt") >= 3, (
        "All files should be tagged"
    )

    # Press Ctrl+U to untag all
    screen2 = _send_and_wait(tui, '\x15', timeout=0.5)
    tagged_count = _tagged_file_count(screen2, "file1.txt", "file2.txt", "file3.txt")
    if tagged_count > 0:
        pytest.fail(
            f"BUG: Ctrl+U did not untag files (found {tagged_count} tagged rows)\n"
            f"{_screen_text(screen2)}"
        )

    tui.quit()


def test_footer_shows_fileinfo_band(test_dir_with_files, ytnova_binary):
    """
    BUG: Footer can drift away from the unified numeric FileInfo band.
    EXPECTED: Footer should show "1..0 file view" and should not show Brief/About.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    footer = _footer_text(lines)

    if "1..0 file view" not in footer:
        pytest.fail(f"BUG: Footer missing unified FileInfo band\nFooter:\n{footer}\n\nFull screen:\n{screen}")

    if "brief" in footer or "compact" in footer:
        pytest.fail(f"BUG: Footer still shows obsolete Brief command\nFooter:\n{footer}")

    if "about" in footer:
        pytest.fail(f"BUG: Footer shows obsolete 'About' command\nFooter:\n{footer}")

    tui.quit()


def _line_with_text(lines, needle):
    for line in lines:
        if needle in line:
            return line
    raise AssertionError(f"Missing line containing {needle!r}\n" + "\n".join(lines))


def _line_tokens(line):
    return tuple(re.findall(r"[A-Za-z0-9._:/+-]+", line))


def _stats_without_view(lines):
    return "\n".join(
        line
        for line in lines
        if "View:" not in line
        and not re.fullmatch(r"\d{2}-\d{2}-\d{4}\s+\d{2}:\d{2}:\d{2}", line.strip())
    )


def test_stats_stay_human_readable_without_wrapping_when_fileinfo_size_toggles(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "stats_fileinfo_toggle"
    test_root.mkdir()
    big_file = test_root / "000-big.bin"
    with big_file.open("wb") as handle:
        handle.truncate(12_345_678_901)
    (test_root / "zzz.txt").write_text("tail\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    _send_and_wait(tui, "2", timeout=0.4)

    screen1 = tui.get_screen_dump()
    stats_lines1 = _stats_area(screen1)
    stats_text1 = "\n".join(stats_lines1)
    file_row_before = _line_with_text(
        _file_lines_with_names(screen1, "000-big.bin"), "000-big.bin"
    )

    assert "11.5G" in stats_text1, f"Stats should stay human-readable.\n{stats_text1}"
    assert not any(
        re.fullmatch(r"\s*[0-9][0-9,\\.]*\s*", line.strip())
        for line in stats_lines1
        if line.strip()
    ), f"Stats should not wrap numeric spillover onto a separate line.\n{stats_text1}"

    _send_and_wait(tui, "6", timeout=0.4)

    screen2 = tui.get_screen_dump()
    stats_lines2 = _stats_area(screen2)
    stats_text2 = "\n".join(stats_lines2)
    file_row_after = _line_with_text(
        _file_lines_with_names(screen2, "000-big.bin"), "000-big.bin"
    )

    assert _stats_without_view(stats_lines2) == _stats_without_view(stats_lines1), (
        "FileInfo size toggle should not change stats rendering.\n"
        f"Before:\n{stats_text1}\n\nAfter:\n{stats_text2}"
    )
    assert _line_tokens(file_row_after) != _line_tokens(file_row_before), (
        "FileInfo size toggle should still change the file-row projection."
    )

    tui.quit()


def test_file_window_starts_in_name_only_view_by_default(tmp_path, ytnova_binary):
    test_root = tmp_path / "default_filemode_plain"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")

    assert file_lines, f"Expected file rows after entering the file window.\n{_screen_text(screen)}"
    assert all("-rw" not in line for line in file_lines), (
        "Default file window should start plain with filename.ext only, not long rows.\n"
        + "\n".join(file_lines)
    )

    tui.quit()


def test_default_fileinfo_view_is_combined_across_dir_and_file(tmp_path, ytnova_binary):
    test_root = tmp_path / "combined_fileinfo_default"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, "2", timeout=0.4)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")

    assert file_lines, f"Expected file rows after entering the file window.\n{_screen_text(screen)}"
    assert any("-rw" in line for line in file_lines), (
        "Default FileInfo behavior should be combined: pressing 2 in the dir view should carry into the file view.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Attributes", (
        "Combined FileInfo behavior should keep the stats label in sync across dir -> file transitions.\n"
        + _screen_text(screen)
    )

    tui.quit()


def test_separate_dir_file_views_can_be_opted_into(tmp_path, ytnova_binary):
    test_root = tmp_path / "separate_fileinfo_opt_in"
    test_root.mkdir()
    (test_root / ".ytnova").write_text(
        "[GLOBAL]\nSEPARATE_DIR_FILE_VIEWS=1\nFILEMODE=1\n",
        encoding="utf-8",
    )
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, "2", timeout=0.4)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")

    assert file_lines, f"Expected file rows after entering the file window.\n{_screen_text(screen)}"
    assert all("-rw" not in line for line in file_lines), (
        "SEPARATE_DIR_FILE_VIEWS=1 should keep the file window on its own plain startup view.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Name", (
        "With SEPARATE_DIR_FILE_VIEWS=1, the file window should still report View: Name here.\n"
        + _screen_text(screen)
    )

    tui.quit()


def test_stats_show_named_fileinfo_view_state(tmp_path, ytnova_binary):
    test_root = tmp_path / "fileinfo_view_summary"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta_long_name.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = tui.get_screen_dump()
    assert _stats_view_value(screen) == "Name", (
        "Plain startup should say View: Name in the stats panel.\n" + _screen_text(screen)
    )

    _send_and_wait(tui, "2", timeout=0.4)
    screen = tui.get_screen_dump()
    assert _stats_view_value(screen) == "Attributes", (
        "Pressing 2 should switch both the rows and the stats label to Attributes.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, "7", timeout=0.4)
    screen = tui.get_screen_dump()
    assert _stats_view_value(screen) == "Mini preview", (
        "Pressing 7 should expose a named Mini preview FileInfo mode, not a stacked or stale label.\n"
        + _screen_text(screen)
    )

    tui.quit()


def test_startup_ignores_legacy_filemode_config_and_stays_plain(
    tmp_path, ytnova_binary
):
    home = tmp_path / "home"
    config_dir = home / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "ytnova.conf").write_text("[GLOBAL]\nFILEMODE=2\n", encoding="utf-8")

    test_root = home / "work"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(test_root),
        env_extra={"HOME": str(home)},
    )

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")
    assert file_lines, f"Expected embedded file rows on startup.\n{_screen_text(screen)}"
    assert all("-rw" not in line for line in file_lines), (
        "Legacy FILEMODE configs should not override the always-plain startup view.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Name", (
        "Plain startup should keep the stats label at View: Name even when an old FILEMODE=2 config exists.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")
    assert file_lines, f"Expected file rows after entering file focus.\n{_screen_text(screen)}"
    assert all("-rw" not in line for line in file_lines), (
        "Entering file focus after startup should still stay in the plain Name view.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Name", (
        "File focus should still report View: Name after the always-plain startup.\n"
        + _screen_text(screen)
    )

    tui.quit()


def test_dir_focus_view_changes_redraw_embedded_file_window_when_shared(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "shared_embedded_redraw"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))

    _send_and_wait(tui, "2", timeout=0.4)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")
    assert file_lines, f"Expected embedded file rows after pressing 2 in dir focus.\n{_screen_text(screen)}"
    assert any("-rw" in line for line in file_lines), (
        "Shared 1..4 views should redraw the embedded file window immediately when dir focus switches to Attributes.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Attributes", (
        "The stats label should stay in sync with the shared Attributes view.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, "1", timeout=0.4)

    screen = tui.get_screen_dump()
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta.txt")
    assert file_lines, f"Expected embedded file rows after pressing 1 in dir focus.\n{_screen_text(screen)}"
    assert all("-rw" not in line for line in file_lines), (
        "Switching back to Name from dir focus should redraw the embedded file window immediately.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Name", (
        "The stats label should return to View: Name with the shared plain view.\n"
        + _screen_text(screen)
    )

    tui.quit()


def test_attributes_view_controls_symlink_targets_in_small_file_window(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "attributes_symlink_small_window"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")
    (test_root / "alpha-link").symlink_to("alpha.txt")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))

    screen = tui.get_screen_dump()
    symlink_line = _line_with_text(_screen_lines(screen), "alpha-link")
    assert " -> " not in symlink_line, (
        "Plain startup should keep symlink rows in the simple Name view.\n"
        f"{symlink_line}\n\nFull screen:\n{_screen_text(screen)}"
    )

    _send_and_wait(tui, "6", timeout=0.4)

    screen = tui.get_screen_dump()
    symlink_line = _line_with_text(_screen_lines(screen), "alpha-link")
    assert " -> " not in symlink_line, (
        "Key 6 should no longer own symlink rendering once that detail moves into Attributes view.\n"
        f"{symlink_line}\n\nFull screen:\n{_screen_text(screen)}"
    )

    _send_and_wait(tui, "2", timeout=0.4)

    screen = tui.get_screen_dump()
    symlink_line = _line_with_text(_screen_lines(screen), "alpha-link")
    assert "alpha-link -> alpha.txt" in symlink_line, (
        "Attributes view should now own symlink target rendering in the embedded small file window.\n"
        f"{symlink_line}\n\nFull screen:\n{_screen_text(screen)}"
    )
    assert _stats_view_value(screen) == "Attributes", (
        "The stats label should stay aligned with the Attributes base view.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, "2", timeout=0.4)

    screen = tui.get_screen_dump()
    symlink_line = _line_with_text(_screen_lines(screen), "alpha-link")
    assert " -> " not in symlink_line, (
        "Leaving Attributes view should restore plain symlink labels.\n"
        f"{symlink_line}\n\nFull screen:\n{_screen_text(screen)}"
    )

    tui.quit()


def test_tree_focus_git_view_uses_single_column_overlay_rows(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "tree_focus_git_overlay"
    test_root.mkdir()
    names = [
        "alpha.c",
        "beta.c",
        "gamma.c",
        "delta.c",
        "echo.c",
        "foxtrot.c",
        "golf.c",
        "hotel.c",
    ]
    for name in names:
        (test_root / name).write_text(f"{name}\n", encoding="utf-8")

    subprocess.run(["git", "init"], cwd=test_root, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "config", "user.email", "codex@example.com"],
        cwd=test_root,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        ["git", "config", "user.name", "Codex"],
        cwd=test_root,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(["git", "add", "."], cwd=test_root, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "commit", "-m", "seed repo"],
        cwd=test_root,
        check=True,
        capture_output=True,
        text=True,
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))

    screen = tui.get_screen_dump()
    assert _file_lines_with_names(screen, *names), (
        f"Expected embedded file rows on startup.\n{_screen_text(screen)}"
    )

    _send_and_wait(tui, "9", timeout=0.8)

    screen = tui.get_screen_dump()
    after_lines = _file_lines_with_names(screen, *names)
    assert after_lines, f"Expected embedded file rows after enabling Git view.\n{_screen_text(screen)}"
    assert all("[clean]" in line for line in after_lines), (
        "Tree-focus Git view should show a Git status on every visible small-window row.\n"
        + "\n".join(after_lines)
    )
    assert max(sum(name in line for name in names) for line in after_lines) == 1, (
        "Tree-focus Git view should collapse the small file window to one detail row per file.\n"
        + "\n".join(after_lines)
    )
    assert _stats_view_value(screen) == "Git", (
        "The stats label should switch to View: Git when the embedded Git file projection is active.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, "5", timeout=0.4)

    screen = tui.get_screen_dump()
    assert _stats_view_value(screen) == "Compact", (
        "Compact should become the named active view when 5 is enabled on top of Git.\n"
        + _screen_text(screen)
    )

    _send_and_wait(tui, "1", timeout=0.4)

    screen = tui.get_screen_dump()
    after_lines = _file_lines_with_names(screen, *names)
    assert _stats_view_value(screen) == "Name", (
        "Pressing 1 should clear extra toggles and return the named view to Name.\n"
        + _screen_text(screen)
    )
    assert after_lines and all("[clean]" not in line for line in after_lines), (
        "Resetting extras should remove the Git overlay from the small file window.\n"
        + "\n".join(after_lines)
    )

    tui.quit()


def test_git_view_marks_modified_files_inside_repo_subdirectory(
    tmp_path, ytnova_binary
):
    repo_root = tmp_path / "git_subdir_status"
    repo_root.mkdir()
    subdir = repo_root / "etc"
    subdir.mkdir()
    (subdir / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (subdir / "beta.txt").write_text("beta\n", encoding="utf-8")

    subprocess.run(["git", "init"], cwd=repo_root, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "config", "user.email", "codex@example.com"],
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        ["git", "config", "user.name", "Codex"],
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(["git", "add", "."], cwd=repo_root, check=True, capture_output=True, text=True)
    subprocess.run(
        ["git", "commit", "-m", "seed repo"],
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )

    (subdir / "alpha.txt").write_text("alpha changed\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(subdir))

    _send_and_wait(tui, "9", timeout=0.8)

    screen = "\n".join(tui.get_screen_dump())
    alpha_line = _line_with_text(screen.split("\n"), "alpha.txt")
    beta_line = _line_with_text(screen.split("\n"), "beta.txt")

    assert "[modified]" in alpha_line, (
        "Git view should report modified files in the current repo subdirectory as modified.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )
    assert "[clean]" in beta_line, (
        "Git view should still report unchanged sibling files as clean.\n"
        f"{beta_line}\n\nFull screen:\n{screen}"
    )
    assert _stats_view_value(screen) == "Git", (
        "The stats label should still report View: Git in a repo subdirectory.\n"
        + screen
    )

    tui.quit()


def test_compact_view_adds_columns_in_small_file_window(tmp_path, ytnova_binary):
    test_root = tmp_path / "compact_small_window"
    test_root.mkdir()
    for idx in range(70):
        (test_root / f"{idx:02d}_very_long_filename_sample.txt").write_text(
            "x\n", encoding="utf-8"
        )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = "\n".join(tui.get_screen_dump())
    before_row = _line_with_text(screen.split("\n"), "00_very_long_filename_sample")
    before_columns = len(re.findall(r"\b\d{2}_very_long_filename", before_row))
    assert before_columns >= 2, (
        "Precondition failed: expected the small file window to start with at least two visible columns.\n"
        + screen
    )

    _send_and_wait(tui, "5", timeout=0.4)

    screen = "\n".join(tui.get_screen_dump())
    after_row = _line_with_text(screen.split("\n"), "00_very_long_filename")
    after_columns = len(re.findall(r"\b\d{2}_very_long_filename", after_row))
    assert after_columns > before_columns, (
        "Compact view should fit more filename columns in the small file window, not just shorten the existing ones.\n"
        f"Before row: {before_row}\nAfter row:  {after_row}\nFull screen:\n{screen}"
    )

    tui.quit()


def test_selecting_base_views_clears_compact_named_state(tmp_path, ytnova_binary):
    test_root = tmp_path / "compact_named_state_reset"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    for key, expected in (("2", "Attributes"), ("3", "Owner"), ("4", "Times")):
        _send_and_wait(tui, "5", timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        assert _stats_view_value(screen) == "Compact", (
            "Precondition failed: 5 should first name the active view Compact.\n"
            + screen
        )

        _send_and_wait(tui, key, timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        assert _stats_view_value(screen) == expected, (
            f"Pressing {key} while compact is active should switch the named view to {expected}, not stay stuck on Compact.\n"
            + screen
        )

        _send_and_wait(tui, "1", timeout=0.4)

    tui.quit()


def test_compact_key_is_ignored_in_dense_file_views(tmp_path, ytnova_binary):
    test_root = tmp_path / "compact_dense_file_noop"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    for key, expected in (("2", "Attributes"), ("3", "Owner"), ("4", "Times")):
        _send_and_wait(tui, key, timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        before_line = _line_with_text(screen.split("\n"), "alpha.txt")
        assert _stats_view_value(screen) == expected, (
            f"Precondition failed: key {key} should first select {expected}.\n"
            + screen
        )

        _send_and_wait(tui, "5", timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        after_line = _line_with_text(screen.split("\n"), "alpha.txt")
        assert _stats_view_value(screen) == expected, (
            f"Key 5 should do nothing while {expected} is active in file focus.\n"
            + screen
        )
        assert _line_tokens(after_line) == _line_tokens(before_line), (
            f"Key 5 should not change the visible file-row tokens while {expected} is active.\n"
            f"Before: {before_line}\nAfter:  {after_line}\n\nFull screen:\n{screen}"
        )

        _send_and_wait(tui, "1", timeout=0.4)

    tui.quit()


def test_compact_key_is_ignored_in_dense_dir_views(tmp_path, ytnova_binary):
    test_root = tmp_path / "compact_dense_dir_noop"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))

    for key, expected in (("2", "Attributes"), ("3", "Owner"), ("4", "Times")):
        _send_and_wait(tui, key, timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        before_line = _line_with_text(screen.split("\n"), "alpha.txt")
        assert _stats_view_value(screen) == expected, (
            f"Precondition failed: key {key} should first select {expected} in tree focus.\n"
            + screen
        )

        _send_and_wait(tui, "5", timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        after_line = _line_with_text(screen.split("\n"), "alpha.txt")
        assert _stats_view_value(screen) == expected, (
            f"Key 5 should do nothing while {expected} is active in tree focus.\n"
            + screen
        )
        assert _line_tokens(after_line) == _line_tokens(before_line), (
            f"Key 5 should not change the small-window row tokens while {expected} is active in tree focus.\n"
            f"Before: {before_line}\nAfter:  {after_line}\n\nFull screen:\n{screen}"
        )

        _send_and_wait(tui, "1", timeout=0.4)

    tui.quit()


def test_zero_is_unused_and_one_resets_back_to_name(tmp_path, ytnova_binary):
    test_root = tmp_path / "fileinfo_zero_unused"
    test_root.mkdir()
    with (test_root / "alpha.bin").open("wb") as handle:
        handle.truncate(12_345)
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    _send_and_wait(tui, "5", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    compact_line = _line_with_text(screen.split("\n"), "alpha.bin")
    assert _stats_view_value(screen) == "Compact", (
        "Precondition failed: key 5 should first enable Compact.\n" + screen
    )

    _send_and_wait(tui, "0", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    unchanged_line = _line_with_text(screen.split("\n"), "alpha.bin")
    assert _stats_view_value(screen) == "Compact", (
        "Key 0 should currently be a silent no-op.\n"
        + screen
    )
    assert _line_tokens(unchanged_line) == _line_tokens(compact_line), (
        "Key 0 should not change the visible file-row tokens.\n"
        f"Before: {compact_line}\nAfter:  {unchanged_line}\n\nFull screen:\n{screen}"
    )

    _send_and_wait(tui, "1", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    reset_line = _line_with_text(screen.split("\n"), "alpha.bin")
    assert _stats_view_value(screen) == "Name", (
        "Key 1 should reset the active view back to Name.\n" + screen
    )
    assert "12.1K" not in reset_line and "12,345" not in reset_line, (
        "Resetting to Name should clear the extra compact/detail state from the visible row.\n"
        f"{reset_line}\n\nFull screen:\n{screen}"
    )

    tui.quit()


def test_repeated_numeric_view_keys_reset_back_to_name(tmp_path, ytnova_binary):
    test_root = tmp_path / "repeat_view_reset"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (test_root / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    for key, expected in (("2", "Attributes"), ("3", "Owner"), ("4", "Times")):
        _send_and_wait(tui, key, timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        assert _stats_view_value(screen) == expected, (
            f"Pressing {key} once should select {expected}.\n{screen}"
        )

        _send_and_wait(tui, key, timeout=0.4)
        screen = "\n".join(tui.get_screen_dump())
        assert _stats_view_value(screen) == "Name", (
            f"Pressing {key} again should reset back to Name.\n{screen}"
        )

    tui.quit()


def test_rich_fileinfo_overlay_shows_text_snippet(tmp_path, ytnova_binary):
    test_root = tmp_path / "fileinfo_overlay_snippet"
    test_root.mkdir()
    (test_root / "alpha.txt").write_text(
        "alpha headline\nsecond line\n", encoding="utf-8"
    )
    (test_root / "beta_long_name.txt").write_text("beta body\n", encoding="utf-8")
    (test_root / "gamma.txt").write_text("gamma tail\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root))
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    _send_and_wait(tui, "7", timeout=0.4)

    screen = "\n".join(tui.get_screen_dump())
    file_lines = _file_lines_with_names(screen, "alpha.txt", "beta_long_name.txt", "gamma.txt")
    alpha_line = _line_with_text(file_lines, "alpha.txt")
    beta_line = _line_with_text(file_lines, "beta_long_name.txt")
    gamma_line = _line_with_text(file_lines, "gamma.txt")

    assert "alpha headline" in alpha_line, (
        "Mini preview FileInfo view should show the start of the selected text file contents.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )
    assert "beta body" in beta_line and "gamma tail" in gamma_line, (
        "Mini preview mode should add a text snippet to every visible file row.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "Mini preview", (
        "The stats label should report View: Mini preview while the snippet overlay is active.\n"
        + screen
    )

    tui.quit()


def test_summary_fileinfo_overlay_uses_file_command_output(tmp_path, ytnova_binary):
    test_root = tmp_path / "fileinfo_overlay_file_command"
    bin_dir = tmp_path / "bin"
    file_stub = bin_dir / "file"

    test_root.mkdir()
    bin_dir.mkdir()
    (test_root / "alpha.txt").write_text("alpha headline\n", encoding="utf-8")
    (test_root / "script.sh").write_text("#!/bin/sh\necho hi\n", encoding="utf-8")
    file_stub.write_text(
        "#!/bin/sh\n"
        'case \"$3\" in\n'
        '  *alpha.txt) echo \"ASCII text, with alpha payload\" ;;\n'
        '  *script.sh) echo \"POSIX shell script, ASCII text executable\" ;;\n'
        "  *) echo \"unknown\" ;;\n"
        "esac\n",
        encoding="utf-8",
    )
    file_stub.chmod(0o755)

    env_extra = {"PATH": f"{bin_dir}:{os.environ.get('PATH', '')}"}
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root), env_extra=env_extra)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    _send_and_wait(tui, "8", timeout=0.4)

    screen = "\n".join(tui.get_screen_dump())
    file_lines = _file_lines_with_names(screen, "alpha.txt", "script.sh")
    alpha_line = _line_with_text(file_lines, "alpha.txt")
    script_line = _line_with_text(file_lines, "script.sh")

    assert "ASCII text, with alpha payload" in alpha_line, (
        "File FileInfo view should use file-command-style output for the row detail.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )
    assert "POSIX shell script, ASCII text executable" in script_line, (
        "File mode should add file-command detail to every visible file row.\n"
        + "\n".join(file_lines)
    )
    assert _stats_view_value(screen) == "File", (
        "The stats label should report View: File while the type overlay is active.\n"
        + screen
    )

    tui.quit()


def test_long_filename_does_not_hide_preview_or_file_overlays(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "overlay_long_filename_guard"
    bin_dir = tmp_path / "bin"
    file_stub = bin_dir / "file"
    long_name = "z" * 96 + ".txt"

    test_root.mkdir()
    bin_dir.mkdir()
    (test_root / "alpha.sh").write_text("#!/bin/sh\necho alpha\n", encoding="utf-8")
    (test_root / long_name).write_text("long payload\n", encoding="utf-8")
    file_stub.write_text(
        "#!/bin/sh\n"
        'case \"$3\" in\n'
        '  *alpha.sh) echo \"POSIX shell script, ASCII text executable\" ;;\n'
        "  *) echo \"ASCII text\" ;;\n"
        "esac\n",
        encoding="utf-8",
    )
    file_stub.chmod(0o755)

    env_extra = {"PATH": f"{bin_dir}:{os.environ.get('PATH', '')}"}
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root), env_extra=env_extra)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    _send_and_wait(tui, "7", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    alpha_line = _line_with_text(screen.split("\n"), "alpha.sh")
    assert "#!/bin/sh echo alpha" in alpha_line, (
        "A very long sibling filename should not push Mini preview detail off-screen for every row.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )
    assert _stats_view_value(screen) == "Mini preview", (
        "The stats label should still report View: Mini preview.\n" + screen
    )

    _send_and_wait(tui, "8", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    alpha_line = _line_with_text(screen.split("\n"), "alpha.sh")
    assert "POSIX shell script, ASCII text executable" in alpha_line, (
        "A very long sibling filename should not hide File detail rows.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )
    assert _stats_view_value(screen) == "File", (
        "The stats label should still report View: File.\n" + screen
    )

    tui.quit()


def test_compact_view_yields_to_visible_rich_and_summary_overlays(
    tmp_path, ytnova_binary
):
    test_root = tmp_path / "compact_overlay_visibility"
    bin_dir = tmp_path / "bin"
    file_stub = bin_dir / "file"

    test_root.mkdir()
    bin_dir.mkdir()
    (test_root / "alpha.txt").write_text(
        "alpha headline\nsecond line\n", encoding="utf-8"
    )
    (test_root / "beta.txt").write_text("beta body\n", encoding="utf-8")
    file_stub.write_text(
        "#!/bin/sh\n"
        'case \"$3\" in\n'
        '  *alpha.txt) echo \"ASCII text, with alpha payload\" ;;\n'
        '  *beta.txt) echo \"ASCII text, with beta payload\" ;;\n'
        "  *) echo \"unknown\" ;;\n"
        "esac\n",
        encoding="utf-8",
    )
    file_stub.chmod(0o755)

    env_extra = {"PATH": f"{bin_dir}:{os.environ.get('PATH', '')}"}
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_root), env_extra=env_extra)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    _send_and_wait(tui, "5", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    assert _stats_view_value(screen) == "Compact", (
        "Precondition failed: key 5 should first enable Compact.\n" + screen
    )

    _send_and_wait(tui, "7", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    alpha_line = _line_with_text(screen.split("\n"), "alpha.txt")
    assert _stats_view_value(screen) == "Mini preview", (
        "Pressing 7 after Compact should switch to a visible Mini preview overlay.\n"
        + screen
    )
    assert "alpha headline" in alpha_line, (
        "Mini preview overlay should no longer stay hidden behind Compact mode.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )

    _send_and_wait(tui, "8", timeout=0.4)
    screen = "\n".join(tui.get_screen_dump())
    alpha_line = _line_with_text(screen.split("\n"), "alpha.txt")
    assert _stats_view_value(screen) == "File", (
        "Pressing 8 after Mini preview should switch to a visible File overlay.\n"
        + screen
    )
    assert "ASCII text, with alpha payload" in alpha_line, (
        "File overlay should no longer stay hidden behind Compact mode.\n"
        f"{alpha_line}\n\nFull screen:\n{screen}"
    )

    tui.quit()


def test_footer_after_execute_escape(test_dir_with_files, ytnova_binary):
    """
    BUG: After pressing 'x' (execute) then ESC, second footer line shows only 'C'.
    EXPECTED: Footer should be fully restored after canceling command.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    # Capture normal footer
    screen1 = "\n".join(tui.get_screen_dump())
    lines1 = screen1.split('\n')
    footer1 = '\n'.join(lines1[-3:])

    # Press 'x' for execute
    _send_and_wait(tui, 'x', timeout=0.5)

    # Press ESC to cancel
    _send_and_wait(tui, Keys.ESC, timeout=0.5)

    # Check footer is restored
    screen2 = "\n".join(tui.get_screen_dump())
    lines2 = screen2.split('\n')
    footer2 = '\n'.join(lines2[-3:])

    # Footer should not be truncated to just 'C'
    footer2_text = footer2.strip()
    if len(footer2_text) < 10:
        pytest.fail(f"BUG: Footer truncated after Execute ESC\nBefore:\n{footer1}\n\nAfter:\n{footer2}")

    tui.quit()


def test_footer_visible_in_big_window(test_dir_with_files, ytnova_binary):
    """
    BUG (FIXED?): Footer was blank in big window mode.
    EXPECTED: Footer menu should be visible when zoomed.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(test_dir_with_files))

    # Enter file window and zoom
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    screen = "\n".join(tui.get_screen_dump())
    footer = _footer_text(screen)

    # Footer should have some command text
    if len(footer.strip()) < 10:
        pytest.fail(f"Footer blank in big window mode:\n{footer}\n\n{screen}")

    tui.quit()

def test_pipe_command_in_big_file_window(test_dir_with_files, ytnova_binary):
    """
    BUG E: The Pipe ('p') command is non-responsive in the big file window.
    EXPECTED: Pressing 'p' prompts the user for a Pipe command.
    """
    # Start with SMALLWINDOWSKIP=0
    ytnova_cfg = test_dir_with_files.parent / ".ytnova"
    ytnova_cfg.write_text("SMALLWINDOWSKIP=0\n")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(test_dir_with_files.parent)
    )
    # Move down to highlight test_dir_with_files
    _send_and_wait(tui, Keys.DOWN, timeout=0.5)

    # Enter small file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)
    # Enter big file window
    _send_and_wait(tui, Keys.ENTER, timeout=0.5)

    # Press 'p' for Pipe
    _send_and_wait(tui, 'p', timeout=0.5)

    screen = "\n".join(tui.get_screen_dump()).lower()

    # Assert that the Pipe prompt appears. Look for "PIPE" or "Pipe" on a single line with ":"
    # "Pipe" in the footer is not the prompt. The prompt row usually looks like "pipe-command:"
    prompt_found = "pipe-command" in screen

    if not prompt_found:
        pytest.fail(f"BUG: Pipe command ('p') did nothing in big file window. Prompt not found.\nScreen dump:\n{screen}")
    tui.quit()
