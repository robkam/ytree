import shlex
import time
import re
from pathlib import Path

from helpers_ui import footer_lines, screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())
DISPLAY_SOURCE = Path(__file__).resolve().parents[1] / "src" / "ui" / "display.c"
_FOOTER_COMMAND_COLUMN = len("COMMANDS ")


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


def _configure_filediff_capture(root):
    log_path = root / "filediff_args.log"
    helper_path = root / ".capture_filediff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)
    (root / ".ytnova").write_text(
        f"[GLOBAL]\nFILEDIFF={helper_path}\n",
        encoding="utf-8",
    )
    return log_path


def _run_file_compare(tui, target, wait=0.5):
    tui.send_keystroke(Keys.CTRL_U + target + Keys.ENTER, wait=wait)
    tui.send_keystroke(Keys.ENTER, wait=0.35)


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


def _footer_command_text(line):
    return line[_FOOTER_COMMAND_COLUMN:].rstrip()


def _footer_entries(lines, row_indexes=(0, 1)):
    entries = []
    for index in row_indexes:
        entries.extend(
            [entry for entry in _footer_command_text(lines[index]).split("  ") if entry]
        )
    return entries


def _assert_balanced_top_footer_rows(lines):
    row0 = _footer_command_text(lines[0])
    row1 = _footer_command_text(lines[1])
    entries0 = [entry for entry in row0.split("  ") if entry]
    entries1 = [entry for entry in row1.split("  ") if entry]
    actual_delta = abs(len(row0) - len(row1))

    if len(entries0) > 1:
        alt0 = "  ".join(entries0[:-1])
        alt1 = "  ".join([entries0[-1], *entries1])
        assert actual_delta <= abs(len(alt0) - len(alt1)), (
            "Footer top rows should keep the chosen split at least as balanced "
            "as moving the last first-row entry down.\n"
            + "\n".join(lines)
        )

    if len(entries1) > 1:
        alt0 = "  ".join([*entries0, entries1[0]])
        alt1 = "  ".join(entries1[1:])
        assert actual_delta <= abs(len(alt0) - len(alt1)), (
            "Footer top rows should keep the chosen split at least as balanced "
            "as moving the first second-row entry up.\n"
            + "\n".join(lines)
        )


def _assert_no_duplicate_footer_entries(lines):
    entries = _footer_entries(lines)
    duplicates = []
    seen = set()
    for entry in entries:
        if entry in seen and entry not in duplicates:
            duplicates.append(entry)
        seen.add(entry)
    assert not duplicates, (
        "Footer top rows must not repeat command labels within the same footer.\n"
        f"duplicates={duplicates}\n" + "\n".join(lines)
    )


def _footer_array_duplicates(source_path, array_type):
    duplicates = {}
    source = source_path.read_text(encoding="utf-8")
    for match in re.finditer(
        rf"static const {array_type}\s+(\w+)\[\]\s*=\s*\{{(.*?)\}};",
        source,
        re.S,
    ):
        current_name = match.group(1)
        body = match.group(2)
        if array_type == "FooterCommandSpec":
            labels = re.findall(
                r'FOOTER_(?:ACTIONS?|STATIC)\(\s*[^,]+,\s*"([^"]+)"',
                body,
                re.S,
            )
        else:
            labels = [
                pair[0]
                for pair in re.findall(
                    r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}',
                    body,
                    re.S,
                )
            ]

        seen = set()
        dup = []
        for label in labels:
            if label in seen and label not in dup:
                dup.append(label)
            seen.add(label)
        if dup:
            duplicates[current_name] = dup

    return duplicates


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


def test_footer_specs_and_help_overrides_do_not_repeat_labels():
    footer_dups = _footer_array_duplicates(DISPLAY_SOURCE, "FooterCommandSpec")
    override_dups = _footer_array_duplicates(DISPLAY_SOURCE, "HelpLabelOverrideSpec")
    assert not footer_dups, footer_dups
    assert not override_dups, override_dups


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
        _assert_balanced_top_footer_rows(dir_lines)
        _assert_no_duplicate_footer_entries(dir_lines)
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
        assert "C/^Copy" in file_footer
        assert "M/^Nove" in file_footer
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
        _assert_balanced_top_footer_rows(file_lines)
        _assert_no_duplicate_footer_entries(file_lines)
        assert "Output" in file_footer, file_footer
        assert "Write" not in file_footer, file_footer
        assert "O/^Output" in file_footer, file_footer
        assert file_lines[2].find("F9 apps") < file_lines[2].find("F10 config"), file_lines[2]
        assert file_lines[2].rstrip().endswith("Esc cancel"), file_lines[2]
        assert "1..9 file view" in file_lines[0], "\n".join(file_lines)
        assert "Only tagged" not in file_footer, "\n".join(file_lines)
        assert "F1 help" in file_lines[2], "\n".join(file_lines)
    finally:
        tui.quit()


def test_picker_menus_show_supported_actions_and_truthful_labels(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_narrow_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        tui.send_keystroke("k", wait=0.5)
        volume_line = _line_containing(tui, "Select Volume")
        assert "Select Volume" in volume_line
        volume_commands = _line_containing_all(tui, "F1 help", "D release", "Enter switch", "Esc cancel")
        assert "Up/Down" not in volume_commands
        assert "Delete" not in volume_commands
        assert "(D)" not in volume_commands
        tui.send_keystroke(Keys.ESC, wait=0.5)

        tui.send_keystroke(Keys.F9, wait=0.5)
        _line_containing(tui, "Applications")
        app_commands = _line_containing_all(
            tui, "F1 help", "Enter select", "Edit", "Esc cancel"
        )
        assert "Close" not in app_commands
        assert "Up/Down" not in app_commands
    finally:
        tui.quit()


def test_f2_picker_shows_explicit_log_key(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_sized_tui(root, 120)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke("c", wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.F2, wait=0.5)

        f2_line = _line_containing_all(
            tui, "F1 help", "Log", "cycle", "` dotfiles", "Enter select", "Esc cancel"
        )
        assert "(L)" not in f2_line
        assert "<" in f2_line and ">" in f2_line
        assert f2_line.index("F1 help") < f2_line.index("Log"), f2_line
        assert f2_line.index("Log") < f2_line.index("cycle"), f2_line
        assert f2_line.index("cycle") < f2_line.index("` dotfiles"), f2_line
        assert f2_line.index("` dotfiles") < f2_line.index("Enter select"), f2_line
        assert f2_line.index("Enter select") < f2_line.index("Esc cancel"), f2_line
        assert "` dotfiles  Enter select" in f2_line, f2_line
    finally:
        tui.quit()


def test_history_dialog_keeps_prompt_footer_and_uses_local_chooser_order(tmp_path):
    root = _root_with_file(tmp_path)
    remembered_target = str(root / "remembered_target.txt")
    Path(remembered_target).write_text("remembered\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)
    tui = _spawn_sized_tui(root, 120)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke("J", wait=0.4)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), screen_text(tui)
        _run_file_compare(tui, remembered_target, wait=0.55)
        assert tui.wait_for_condition(
            lambda _lines: log_path.exists(), timeout=2.0, poll_interval=0.05
        ), screen_text(tui)

        tui.send_keystroke("J", wait=0.4)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), screen_text(tui)

        prompt_footer = _line_containing_all(tui, "F2 browse", "Up history", "Enter OK", "Esc cancel")

        tui.send_keystroke(Keys.UP, wait=0.5)

        history_footer = _line_containing_all(
            tui, "F1 help", "Delete", "Pin/unpin", "Enter select", "Esc cancel"
        )
        assert "Up/Down" not in history_footer
        assert "Left/Right" not in history_footer
        assert history_footer.index("F1 help") < history_footer.index("Delete"), history_footer
        assert history_footer.index("Delete") < history_footer.index("Pin/unpin"), history_footer
        assert history_footer.index("Pin/unpin") < history_footer.index("Enter select"), history_footer
        assert history_footer.index("Enter select") < history_footer.index("Esc cancel"), history_footer

        assert prompt_footer in "\n".join(_prompt_lines(tui)), screen_text(tui)
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


def test_extra_wide_footers_advertise_tagged_variants(tmp_path):
    root = _root_with_file(tmp_path)
    tui = _spawn_sized_tui(root, cols=240)

    try:
        dir_footer = "\n".join(footer_lines(tui))
        assert "T/^Tag" in dir_footer, dir_footer
        assert "U/^Untag" in dir_footer, dir_footer

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        file_footer = "\n".join(footer_lines(tui))
        for variant in (
            "A/^Attributes",
            "C/^Copy",
            "D/^Delete",
            "M/^Nove",
            "O/^Output",
            "P/^Pipe",
            "R/^Rename",
            "^Search",
            "T/^Tag",
            "U/^Untag",
            "V/^View",
            "eX/^Xecute",
            "pathcopY/^Y",
            "Z/^Z archive",
        ):
            assert variant in file_footer, file_footer
        assert file_footer.index("Sort") < file_footer.index("^Search")
        assert file_footer.index("^Search") < file_footer.index("T/^Tag")
    finally:
        tui.quit()
