import time
from pathlib import Path

from helpers_ui import footer_lines, screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())


def _spawn_narrow_tui(root):
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))
    tui.child.setwinsize(24, 80)
    tui.screen.resize(24, 80)
    time.sleep(1.0)
    tui._read_output(0.5)
    return tui


def _root_with_file(tmp_path):
    root = tmp_path / "command_strip_visibility"
    root.mkdir()
    (root / "dir1").mkdir()
    (root / "file1.txt").write_text("seed\n", encoding="utf-8")
    return root


def _line_containing(tui, needle):
    for line in tui.get_screen_dump():
        if needle in line:
            return line
    raise AssertionError(f"Could not find {needle!r}.\n{screen_text(tui)}")


def _line_containing_all(tui, *needles):
    for line in tui.get_screen_dump():
        if all(needle in line for needle in needles):
            return line
    raise AssertionError(f"Could not find {needles!r}.\n{screen_text(tui)}")


def _assert_footer_column_alignment(lines, first_row_token, second_row_token):
    assert lines[0].find(first_row_token) == lines[1].find(second_row_token) == lines[2].find("F1 help"), (
        "Footer rows should share one left-aligned command column.\n"
        + "\n".join(lines)
    )


def _assert_single_space_after_nav_glyphs(line, label):
    assert line[3:].startswith(f" {label} F1 help"), (
        "Footer nav row should use exactly one space after the nav glyphs.\n"
        f"{line!r}"
    )
    assert not line[3:].startswith(f"  {label}"), (
        "Footer nav row must not double-space after the nav glyphs.\n"
        f"{line!r}"
    )


def test_narrow_dir_and_file_footers_show_explicit_mnemonic_keys(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_narrow_tui(root)

    try:
        dir_footer = "\n".join(footer_lines(tui))
        assert "1..0 A C D F G I J L M N O" in dir_footer
        assert "^F" not in dir_footer
        assert "Brief" not in dir_footer
        assert "(A)" not in dir_footer
        assert "(M)" not in dir_footer
        assert "(N)" not in dir_footer
        assert "Esc" in dir_footer

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        file_footer = "\n".join(footer_lines(tui))
        assert "1..0 A C/^K D E F H I J L M/^N" in file_footer
        assert "^F" not in file_footer
        assert "Brief" not in file_footer
        assert "N O P Q R S W X Y Z / `" in file_footer
        assert "(A)" not in file_footer
        assert "(E)" not in file_footer
        assert "(M)" not in file_footer
        assert "(Y)" not in file_footer
        assert "Esc" in file_footer
    finally:
        tui.quit()


def test_wide_footer_keeps_space_before_jump_label(tmp_path):
    root = _root_with_file(tmp_path)
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.child.setwinsize(24, 140)
        tui.screen.resize(24, 140)
        time.sleep(1.0)
        tui._read_output(0.5)

        dir_lines = footer_lines(tui)
        dir_footer = "\n".join(dir_lines)
        assert "/ jump" in dir_footer
        assert "/jump" not in dir_footer
        assert "` dotfiles" in dir_footer
        assert "`dotfiles" not in dir_footer
        _assert_single_space_after_nav_glyphs(dir_lines[2], "File")
        _assert_footer_column_alignment(dir_lines, "1..0 dir view", "Pipe")

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        file_lines = footer_lines(tui)
        file_footer = "\n".join(file_lines)
        assert "C/^K copy" in file_footer
        assert "C/^K Copy" not in file_footer
        assert "M/^N move" in file_footer
        assert "M/^N Move" not in file_footer
        _assert_single_space_after_nav_glyphs(file_lines[2], "Tree")
        _assert_footer_column_alignment(file_lines, "1..0 file view", "Newfile")
    finally:
        tui.quit()


def test_picker_menus_show_explicit_close_and_action_keys(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_narrow_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        tui.send_keystroke("k", wait=0.5)
        volume_line = _line_containing(tui, "Select Volume")
        assert "Select Volume" in volume_line
        volume_commands = _line_containing_all(tui, "Up/Down", "Esc")
        assert "select" in volume_commands
        assert "switch" in volume_commands
        assert "quit" in volume_commands
        assert "Esc" in volume_commands
        assert "Delete" in volume_commands
        assert "(D)" not in volume_commands
        tui.send_keystroke(Keys.ESC, wait=0.5)

        tui.send_keystroke(Keys.F9, wait=0.5)
        _line_containing(tui, "Applications")
        app_commands = _line_containing_all(tui, "Select ", "Enter")
        assert "Enter" in app_commands
        assert "Esc" in app_commands
    finally:
        tui.quit()


def test_f2_picker_shows_explicit_log_key(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_narrow_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke("c", wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.F2, wait=0.5)

        f2_line = _line_containing(tui, "cycle")
        assert "Log" in f2_line
        assert "(L)" not in f2_line
        assert "cycle" in f2_line
        assert "<" in f2_line and ">" in f2_line
    finally:
        tui.quit()
