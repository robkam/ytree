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
    assert tui.wait_for_condition(
        lambda lines: any("F1 help" in line for line in lines[-3:]),
        timeout=2.0,
        poll_interval=0.05,
    ), screen_text(tui)
    return tui


def _spawn_sized_tui(root, cols):
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))
    tui.child.setwinsize(24, cols)
    tui.screen.resize(24, cols)
    assert tui.wait_for_condition(
        lambda lines: any("F1 help" in line for line in lines[-3:]),
        timeout=2.0,
        poll_interval=0.05,
    ), screen_text(tui)
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


def _prompt_lines(tui):
    return tui.get_screen_dump()[-3:]


def _assert_single_space_after_nav_glyphs(line, label, first_command):
    assert line[3:].startswith(f" {label} {first_command}"), (
        "Footer nav row should use exactly one space after the nav glyphs.\n"
        f"{line!r}"
    )
    assert not line[3:].startswith(f"  {label}"), (
        "Footer nav row must not double-space after the nav glyphs.\n"
        f"{line!r}"
    )


def test_narrow_dir_and_file_footers_keep_full_labels_until_resize(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_narrow_tui(root)

    try:
        dir_lines = footer_lines(tui)
        dir_footer = "\n".join(dir_lines)
        assert "1..9 A C D F G I J L M N O" not in dir_footer
        assert any("dir view" in line for line in dir_lines[:2]), dir_footer
        assert dir_lines[1].rstrip().endswith("..."), dir_footer
        assert "^F" not in dir_footer
        assert "Brief" not in dir_footer
        assert "(A)" not in dir_footer
        assert "(M)" not in dir_footer
        assert "(N)" not in dir_footer
        assert dir_lines[2].rstrip().endswith("..."), dir_footer
        assert "F9 apps" in dir_lines[2], dir_footer
        assert "F10" in dir_lines[2], dir_footer

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        file_lines = footer_lines(tui)
        file_footer = "\n".join(file_lines)
        assert "1..9 A C/^K D E F H I J L M/^N" not in file_footer
        assert any("file view" in line for line in file_lines[:2]), file_footer
        assert file_lines[1].rstrip().endswith("..."), file_footer
        assert "^F" not in file_footer
        assert "Brief" not in file_footer
        assert "(A)" not in file_footer
        assert "(E)" not in file_footer
        assert "(M)" not in file_footer
        assert "(Y)" not in file_footer
        assert file_lines[2].rstrip().endswith("..."), file_footer
        assert "F9 apps" in file_lines[2], file_footer
        assert "F10" in file_lines[2], file_footer
    finally:
        tui.quit()


def test_wide_footer_keeps_space_before_jump_label(tmp_path):
    root = _root_with_file(tmp_path)
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.child.setwinsize(24, 140)
        tui.screen.resize(24, 140)
        assert tui.wait_for_condition(
            lambda lines: any("F1 help" in line for line in lines[-3:]),
            timeout=2.0,
            poll_interval=0.05,
        ), screen_text(tui)

        dir_lines = footer_lines(tui)
        dir_footer = "\n".join(dir_lines)
        assert "/ jump" in dir_footer
        assert "/jump" not in dir_footer
        assert "` dotfiles" in dir_footer
        assert "`dotfiles" not in dir_footer
        _assert_single_space_after_nav_glyphs(dir_lines[2], "File", "F1 help")
        assert dir_lines[1].startswith("COMMANDS "), (
            "Wide dir footer second row should remain a balanced command overflow row.\n"
            + "\n".join(dir_lines)
        )
        assert "K volume" in dir_footer, dir_footer
        assert "Output" in dir_footer, dir_footer
        assert "Write" not in dir_footer, dir_footer
        assert dir_footer.index("Output") < dir_footer.index("eXecute"), dir_footer
        assert dir_lines[2].find("F9 apps") < dir_lines[2].find("F10 config"), dir_lines[2]
        assert dir_lines[2].rstrip().endswith("Esc cancel"), dir_lines[2]
        assert "1..9 dir view" in dir_lines[0], "\n".join(dir_lines)
        assert "Pipe" in dir_lines[1], "\n".join(dir_lines)
        assert "Only tagged" not in dir_footer, "\n".join(dir_lines)
        assert "F1 help" in dir_lines[2], "\n".join(dir_lines)

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        file_lines = footer_lines(tui)
        file_footer = "\n".join(file_lines)
        assert "C/^K copy" in file_footer
        assert "C/^K Copy" not in file_footer
        assert "M/^N move" in file_footer
        assert "M/^N Move" not in file_footer
        assert "K volume" in file_footer, file_footer
        assert "Tag" in file_footer, file_footer
        assert "Untag" in file_footer, file_footer
        assert "View" in file_footer, file_footer
        _assert_single_space_after_nav_glyphs(file_lines[2], "Tree", "F1 help")
        assert "Pipe" in file_lines[1], (
            "Wide file footer should balance later key-ordered actions onto the second row.\n"
            + "\n".join(file_lines)
        )
        assert file_lines[1].startswith("COMMANDS "), (
            "Wide file footer second row should begin with the balanced overflow command row.\n"
            + "\n".join(file_lines)
        )
        assert "Output" in file_footer, file_footer
        assert "Write" not in file_footer, file_footer
        assert file_footer.index("Output") < file_footer.index("eXecute"), file_footer
        assert file_lines[2].find("F9 apps") < file_lines[2].find("F10 config"), file_lines[2]
        assert file_lines[2].rstrip().endswith("Esc cancel"), file_lines[2]
        assert "1..9 file view" in file_lines[0], "\n".join(file_lines)
        assert "Only tagged" not in file_footer, "\n".join(file_lines)
        assert "F1 help" in file_lines[2], "\n".join(file_lines)
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


def test_narrow_compare_target_prompt_uses_truncation_not_mid_token_clipping(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_sized_tui(root, cols=48)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke("J", wait=0.3)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), screen_text(tui)

        lines = _prompt_lines(tui)
        hint_line = lines[-1]
        prompt_text = "\n".join(lines)

        assert "F1 help" in hint_line and "F2 browse" in hint_line, prompt_text
        assert hint_line.rstrip().endswith("..."), (
            "Narrow compare prompt should truncate the final command entry with an ellipsis.\n"
            + prompt_text
        )
        assert "Esc c" not in hint_line, (
            "Narrow compare prompt should not clip the cancel command mid-token.\n"
            + prompt_text
        )
    finally:
        tui.quit()


def test_narrow_sort_and_viewer_takeovers_use_ellipsis_for_overflow(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_sized_tui(root, cols=40)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        tui.send_keystroke("s", wait=0.3)
        assert tui.wait_for_condition(
            lambda lines: any("SORT by" in line for line in lines[-3:]),
            timeout=1.0,
            poll_interval=0.05,
        ), screen_text(tui)

        sort_lines = _prompt_lines(tui)
        sort_text = "\n".join(sort_lines)
        assert any(line.rstrip().endswith("...") for line in sort_lines[:2]), (
            "Narrow sort prompt should truncate overflow with an ellipsis instead of clipping key labels.\n"
            + sort_text
        )

        tui.send_keystroke(Keys.ESC, wait=0.2)
        tui.send_keystroke("h", wait=0.6)

        viewer_lines = _prompt_lines(tui)
        viewer_text = "\n".join(viewer_lines)
        assert viewer_lines[-1].rstrip().endswith("..."), (
            "Narrow viewer navigation strip should truncate the final command with an ellipsis.\n"
            + viewer_text
        )
    finally:
        tui.quit()
