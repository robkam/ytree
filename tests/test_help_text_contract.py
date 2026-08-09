from pathlib import Path
import io
import os
import re
import tarfile

from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source
from helpers_ui import footer_lines, screen_text
import pexpect
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())


def _spawn_help_tui(root, env_extra=None):
    return YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root), env_extra=env_extra)


def _root_with_file(tmp_path, name="help_text_contract"):
    root = tmp_path / name
    root.mkdir()
    (root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (root / "beta.txt").write_text("beta\n", encoding="utf-8")
    return root


def _create_tar(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, data in entries.items():
            payload = data.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))


def _popup_frame(screen, title, footer_hint="Esc/Quit"):
    lines = screen.splitlines()
    title_row = next(i for i, line in enumerate(lines) if title in line)
    title_line = lines[title_row]
    title_col = title_line.index(title)
    left = title_line.rfind("x", 0, title_col)
    right = title_line.find("x", title_col + len(title))
    footer_row = next(i for i, line in enumerate(lines) if footer_hint in line)
    bottom_row = next(
        i
        for i, line in enumerate(lines[footer_row:], start=footer_row)
        if len(line) > right and line[left] == "m" and line[right] == "j"
    )
    return {
        "title_row": title_row,
        "footer_row": footer_row,
        "bottom_row": bottom_row,
        "left": left,
        "right": right,
    }


def _enter_archive_from_selected_file(tui):
    tui.send_keystroke(Keys.ENTER, wait=0.5)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)

    for _ in range(6):
        if tui.wait_for_content("Skipped unsafe archive member path", timeout=0.3):
            tui.send_keystroke(Keys.ENTER, wait=0.3)
        else:
            break


def _wait_for_help(tui, title, key=Keys.F1, timeout=1.5):
    screen = tui.send_and_wait_for_condition(
        key,
        lambda lines: lines
        if any(title in line for line in lines)
        else False,
        timeout=timeout,
    )
    assert screen, screen_text(tui)
    return "\n".join(screen)


def _normalized_help_text(screen):
    return " ".join(screen.replace("`", "").split())


def _scroll_help_to_text(tui, text, *, steps=48):
    current = screen_text(tui)
    unchanged_steps = 0

    if text in current:
        return current

    for _ in range(steps):
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        next_screen = screen_text(tui)
        if text in next_screen:
            return next_screen
        if next_screen == current:
            unchanged_steps += 1
            if unchanged_steps >= 2:
                tui.send_keystroke(Keys.PGDN, wait=0.05)
                next_screen = screen_text(tui)
                if text in next_screen:
                    return next_screen
                unchanged_steps = 0
        else:
            unchanged_steps = 0
        current = next_screen
    return current


def _send_help_key_until_text(tui, key, text, *, timeout=1.0):
    screen = tui.send_and_wait_for_condition(
        key,
        lambda lines: lines if any(text in line for line in lines) else False,
        timeout=timeout,
    )
    assert screen, screen_text(tui)
    return "\n".join(screen)


def _open_help_detail(
    tui, label, detail_text, *, direction_key=Keys.RIGHT, timeout=1.0, steps=24
):
    detail_title = label[:-1] if label.endswith(":") else label
    _scroll_help_to_text(tui, label)

    for _ in range(steps):
        before = screen_text(tui)
        tui.send_keystroke(direction_key, wait=0.05)
        current = screen_text(tui)
        if current != before:
            title_row = current.splitlines()[3]
            if (
                detail_title in title_row
                and f"{detail_title}:" not in title_row
                and "Help" not in title_row
            ):
                return current
            tui.send_keystroke(Keys.LEFT, wait=0.05)
            current = screen_text(tui)
        if label not in current:
            _scroll_help_to_text(tui, label)
        tui.send_keystroke(Keys.DOWN, wait=0.05)

    assert False, screen_text(tui)


def _follow_help_topic(
    tui, label, topic_title, *, direction_key=Keys.RIGHT, timeout=1.0, steps=24
):
    _scroll_help_to_text(tui, label)

    for _ in range(steps):
        before = screen_text(tui)
        screen = tui.send_and_wait_for_condition(
            direction_key,
            lambda lines: lines if any(topic_title in line for line in lines) else False,
            timeout=timeout,
        )
        if screen:
            return "\n".join(screen)
        current = screen_text(tui)
        if current != before and topic_title not in current:
            tui.send_keystroke(Keys.LEFT, wait=0.05)
            current = screen_text(tui)
        if label not in current:
            current = _scroll_help_to_text(tui, label)
        tui.send_keystroke(Keys.DOWN, wait=0.05)

    assert False, screen_text(tui)


def _open_current_help_detail_title(tui, direction_key=Keys.RIGHT):
    before = screen_text(tui)
    tui.send_keystroke(direction_key, wait=0.05)
    after = screen_text(tui)

    if after == before:
        return None, after
    return after.splitlines()[3], after


def _visible_cell_style(tui, needle):
    for y, line in enumerate(tui.peek_screen_dump()):
        if needle not in line:
            continue
        x = line.index(needle)
        cell = tui.screen.buffer[y][x]
        return (cell.fg, cell.bg, cell.bold, cell.reverse, cell.underscore)
    raise AssertionError(f"Could not find visible cell for {needle!r}.\n{screen_text(tui)}")


def _selected_visible_help_label(tui, labels, selected_style):
    for label in labels:
        try:
            if _visible_cell_style(tui, label) == selected_style:
                return label
        except AssertionError:
            continue
    return None


def _capture_help_scroll_output(root, *, down_presses=12, rows=40, cols=142):
    raw_output = io.BytesIO()
    child = pexpect.spawn(
        YTNOVA_BIN,
        [str(root)],
        cwd=str(root),
        env={**os.environ, "TERM": "xterm-256color"},
        dimensions=(rows, cols),
        encoding=None,
        timeout=5,
    )
    try:
        child.logfile_read = raw_output
        child.expect(b"alpha.txt")
        child.send(Keys.F1.encode("ascii"))
        child.expect(b"Directory Help")
        raw_output.seek(0)
        raw_output.truncate(0)
        for _ in range(down_presses):
            child.send(Keys.DOWN.encode("ascii"))
        child.send(b"q")
        child.expect(b"alpha.txt")
        child.send(b"q")
        child.expect(pexpect.EOF)
    finally:
        child.close(force=True)
    return raw_output.getvalue(), rows


def test_contextual_help_scroll_does_not_emit_partial_scroll_regions(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_no_partial_scroll_regions")

    raw_output, rows = _capture_help_scroll_output(root)
    scroll_regions = re.findall(rb"\x1b\[(\d+;\d+)r", raw_output)
    disallowed = [
        region for region in scroll_regions if region != f"1;{rows}".encode("ascii")
    ]

    assert not disallowed, raw_output.decode("latin1", errors="replace")


def test_contextual_help_accepts_csi_arrow_sequences(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_csi_arrows")
    tui = _spawn_help_tui(root)

    csi_down = "\033[B"
    csi_right = "\033[C"
    csi_left = "\033[D"

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        tui.send_keystroke(csi_down, wait=0.05)

        copy_detail = tui.send_and_wait_for_condition(
            csi_right,
            lambda lines: lines
            if any("Copy/Move Targets" in line for line in lines)
            else False,
            timeout=1.5,
        )
        assert copy_detail, screen_text(tui)

        returned = tui.send_and_wait_for_condition(
            csi_left,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert returned, screen_text(tui)
        assert any("Copy:" in line for line in returned), "\n".join(returned)
    finally:
        tui.quit()


def test_contextual_help_accepts_application_arrow_sequences(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_app_arrows")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        tui.send_keystroke(Keys.DOWN, wait=0.05)

        copy_detail = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines
            if any("Copy/Move Targets" in line for line in lines)
            else False,
            timeout=1.5,
        )
        assert copy_detail, screen_text(tui)

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert returned, screen_text(tui)
        assert any("Copy:" in line for line in returned), "\n".join(returned)
    finally:
        tui.quit()


def test_contextual_help_down_arrow_advances_hidden_active_link(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_active_link")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        tui.send_keystroke(Keys.DOWN, wait=0.05)
        moved = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines
            if any("Copy/Move Targets" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert moved, screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_down_arrow_skips_plain_rows_then_scrolls(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_down_scroll")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        moved = tui.send_and_wait_for_condition(
            Keys.DOWN,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert moved, screen_text(tui)

        copy_detail = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines
            if any("Copy/Move Targets" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert copy_detail, screen_text(tui)

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
        scrolled = _scroll_help_to_text(tui, "eXecute:")
        assert "eXecute:" in scrolled, scrolled
        assert "1..9 view:" not in scrolled, scrolled
    finally:
        tui.quit()


def test_contextual_help_down_arrow_scrolls_past_write_to_lower_commands(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_scroll_lower_commands")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        lower_commands = tui.wait_for_condition(
            lambda lines: lines if any("eXecute:" in line for line in lines) else False,
            timeout=2.0,
            poll_interval=0.02,
        )
        assert not lower_commands, screen_text(tui)

        for _ in range(30):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            current = tui.peek_screen_dump()
            if any("eXecute:" in line for line in current):
                break

        assert any("eXecute:" in line for line in tui.peek_screen_dump()), screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_down_arrow_eventually_scrolls_visible_page(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_single_line_scroll")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        scrolled = _scroll_help_to_text(tui, "Z archive:")
        assert "Z archive:" in scrolled, scrolled

        advanced = None
        for _ in range(8):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            current = screen_text(tui)
            if "/ jump:" in current or "` dotfiles:" in current:
                advanced = current
                break

        assert advanced, screen_text(tui)
        assert "Directory Help" in advanced, advanced
    finally:
        tui.quit()


def test_contextual_help_keeps_blank_gap_above_footer_while_scrolled(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_footer_gap")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        help_screen = _scroll_help_to_text(tui, "` dotfiles:")
        assert "` dotfiles:" in help_screen, help_screen

        frame = _popup_frame(help_screen, "Directory Help")
        help_lines = help_screen.splitlines()
        footer_gap = help_lines[frame["footer_row"] - 1][
            frame["left"] + 1 : frame["right"]
        ]

        assert footer_gap.strip() == "", help_screen
    finally:
        tui.quit()


def test_contextual_help_keeps_exactly_one_blank_line_above_footer_at_bottom(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_single_footer_gap")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        help_screen = _scroll_help_to_text(tui, "F10:")
        assert "F10:" in help_screen, help_screen

        current = help_screen
        for _ in range(8):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            next_screen = screen_text(tui)
            if next_screen == current:
                break
            current = next_screen
        help_screen = current

        frame = _popup_frame(help_screen, "Directory Help")
        help_lines = help_screen.splitlines()
        last_content_row = max(
            i
            for i in range(frame["title_row"] + 1, frame["footer_row"])
            if help_lines[i][frame["left"] + 1 : frame["right"]].strip() != ""
        )

        assert frame["footer_row"] - last_content_row == 2, help_screen
    finally:
        tui.quit()


def test_contextual_help_up_arrow_reselects_visible_links_when_scrolling_back(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_up_reselects")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        selected_style = _visible_cell_style(tui, "1..9 view:")

        reached_execute = False
        for _ in range(48):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            current = screen_text(tui)
            if "eXecute:" in current:
                reached_execute = True
                break

        assert reached_execute, screen_text(tui)

        restored = False
        for _ in range(48):
            tui.send_keystroke(Keys.UP, wait=0.05)
            current = screen_text(tui)
            if "J compare:" in current:
                restored = True
                break

        assert restored, screen_text(tui)
        assert _visible_cell_style(tui, "J compare:") == selected_style, screen_text(tui)

        restored_top = False
        for _ in range(48):
            tui.send_keystroke(Keys.UP, wait=0.05)
            current = screen_text(tui)
            if "1..9 view:" in current:
                restored_top = True
                break

        assert restored_top, screen_text(tui)
        assert _visible_cell_style(tui, "1..9 view:") == selected_style, screen_text(tui)

        tui.send_keystroke(Keys.UP, wait=0.05)
        assert _visible_cell_style(tui, "1..9 view:") == selected_style, screen_text(tui)
    finally:
        tui.quit()


def test_split_file_help_arrows_follow_rows_without_wrapping(tmp_path):
    root = _root_with_file(tmp_path, "split_file_help_arrow_boundaries")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.F1)
        assert tui.wait_for_content("F8 Split Directory Help", timeout=1.5), screen_text(
            tui
        )
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "F8 Split File Help")
        assert "C/^K copy:" in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1..9 view:")
        first_detail = _follow_help_topic(
            tui, "C/^K copy:", "Copy/Move Targets", timeout=1.0
        )
        assert "Copy/Move Targets" in first_detail, first_detail
        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("F8 Split File Help", timeout=1.0), screen_text(tui)

        compare_detail = _follow_help_topic(
            tui, "J compare:", "Compare Help", timeout=1.0
        )
        assert "Compare Help" in compare_detail, compare_detail
        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("F8 Split File Help", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.END, wait=0.05)
        current = screen_text(tui)
        assert "F10:" in current, current

        unchanged = tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.5)
        assert not unchanged, screen_text(tui)
    finally:
        tui.quit()


def test_vi_file_footer_uses_runtime_vi_keys(tmp_path):
    root = _root_with_file(tmp_path, "vi_footer_runtime_keys")
    (root / ".ytnova").write_text("[GLOBAL]\nVI_KEYS=1\n", encoding="utf-8")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        footer = "\n".join(footer_lines(tui))

        assert "delete" in footer, footer
        assert "Delete" not in footer, footer
    finally:
        tui.quit()


def test_execute_prompt_f1_help_explains_placeholder_and_tagged_repeat(tmp_path):
    root = _root_with_file(tmp_path, "execute_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("x", wait=0.2)

        assert tui.wait_for_content("COMMAND", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "{} where the selected file path should be inserted" in normalized, help_screen
        assert "Ctrl-X" in help_screen, help_screen
        assert "rerun the command for each tagged file" in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("COMMAND", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_search_tagged_prompt_f1_help_explains_tag_scope(tmp_path):
    root = _root_with_file(tmp_path, "search_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke("\x13", wait=0.2)

        assert tui.wait_for_content("SEARCH TAGGED", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "Enter plain search text only." in normalized, help_screen
        assert "ytnova builds grep -i -- PATTERN {} for you." in normalized, help_screen
        assert "Only tagged files are searched, and non-matches are untagged." in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("SEARCH TAGGED", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_filter_prompt_f1_help_uses_generated_runtime_topic(tmp_path):
    root = _root_with_file(tmp_path, "filter_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("f", wait=0.2)

        assert tui.wait_for_content("FILTER:", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        filter_help = _wait_for_help(tui, "Filter Help")
        normalized = _normalized_help_text(filter_help)
        assert "Type one or more filter terms." in normalized, filter_help
        assert "The prompt starts with *, which means all files." in normalized, filter_help
        assert "Terms can be stacked by separating them with commas." in normalized, filter_help
        assert "All terms apply together to the current file-list scope." in normalized, filter_help

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("FILTER:", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_archive_prompt_f1_help_explains_suffixes_and_selection_scope(tmp_path):
    root = _root_with_file(tmp_path, "archive_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("z", wait=0.2)

        assert tui.wait_for_content("Create archive", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "Use .tar, .tar.gz or .tgz, .tar.bz2 or .tbz2, .tar.xz or .txz, or .zip." in normalized, help_screen
        assert "the tagged set wins" in normalized, help_screen
        assert "current file or directory selection" in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Create archive", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_main_f1_help_tracks_directory_file_preview_and_split_contexts(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_main_contexts")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        directory_help_screen = help_screen
        assert "1..9 view:" in directory_help_screen, directory_help_screen
        assert "Attributes:" in directory_help_screen, directory_help_screen
        assert "A (Attributes):" not in directory_help_screen, directory_help_screen
        assert "Directory commands" not in directory_help_screen, directory_help_screen
        assert "Tree navigation" not in directory_help_screen, directory_help_screen
        for nav_label in ("Enter:", "Collapse:", "Left Arrow:", "Right Arrow:", "Plus:", "Asterisk:"):
            assert nav_label not in directory_help_screen, directory_help_screen
        for label, stale_label in (
            ("J compare:", "Compare:"),
            ("K volume:", "Volume:"),
            ("moVedir:", "MoveDir:"),
            ("eXecute:", "Execute:"),
            ("Z archive:", "Archive:"),
            ("/ jump:", "Jump:"),
            ("` dotfiles:", "Dotfiles:"),
            ("F10:", None),
        ):
            help_screen = _scroll_help_to_text(tui, label)
            assert label in help_screen, help_screen
            if stale_label is not None:
                assert stale_label not in help_screen, help_screen
        assert "Directory help explains the live directory footer commands" not in help_screen, help_screen
        footer_line = next(
            line for line in help_screen.splitlines() if "Esc/Quit" in line
        )
        assert "Open" in footer_line, footer_line
        assert "Contents" in footer_line, footer_line
        assert "Navigation" in footer_line, footer_line
        assert "Shared commands" not in footer_line, footer_line
        assert "F8 split" not in footer_line, footer_line
        directory_frame = _popup_frame(directory_help_screen, "Directory Help")
        assert footer_line.index("Enter/Right") - directory_frame["left"] <= 3, footer_line
        assert directory_frame["bottom_row"] == directory_frame["footer_row"] + 1, directory_help_screen
        help_lines = directory_help_screen.splitlines()
        title_gap = help_lines[directory_frame["title_row"] + 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert title_gap.strip() == "", directory_help_screen
        first_help_row = next(
            i for i, line in enumerate(help_lines) if "1..9 view:" in line
        )
        blank_gap = help_lines[first_help_row + 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert blank_gap.strip() == "", directory_help_screen
        footer_gap = help_lines[directory_frame["footer_row"] - 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert footer_gap.strip() == "", directory_help_screen

        tui.send_keystroke("q")
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "F8 Split Directory Help")
        assert "Tab" in help_screen, help_screen
        assert "inactive panel" in help_screen, help_screen
        assert "1..9 view:" in help_screen, help_screen
        assert "J compare:" in help_screen, help_screen
        assert "F8:" in help_screen, help_screen
        assert "Leave split:" not in help_screen, help_screen
        help_screen = _scroll_help_to_text(tui, "eXecute:")
        assert "eXecute:" in help_screen, help_screen
        assert "Left Arrow:" not in help_screen, help_screen
        help_screen = _send_help_key_until_text(tui, Keys.HOME, "Copy:")
        split_copy_detail = _follow_help_topic(
            tui, "Copy:", "Copy/Move Targets", timeout=1.0
        )
        assert "wildcard rename pattern" in _normalized_help_text(split_copy_detail), split_copy_detail
        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("F8 Split Directory Help", timeout=1.0), screen_text(
            tui
        )
        split_frame = _popup_frame(help_screen, "F8 Split Directory Help")
        split_body = "\n".join(
            line[split_frame["left"] + 1 : split_frame["right"]]
            for line in help_screen.splitlines()[
                split_frame["title_row"] + 1 : split_frame["footer_row"]
            ]
        )
        assert "DIR1..9 dir view" not in split_body, help_screen
        assert "COMMANDS" not in split_body, help_screen
        assert "File F1 help" not in split_body, help_screen
        assert split_frame["title_row"] == directory_frame["title_row"], help_screen
        assert split_frame["footer_row"] == directory_frame["footer_row"], help_screen
        assert split_frame["left"] == directory_frame["left"], help_screen
        assert split_frame["right"] >= directory_frame["right"], help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "F8 Split File Help")
        assert "File commands" not in help_screen, help_screen
        assert "1..9 view:" in help_screen, help_screen
        assert "Left Arrow:" not in help_screen, help_screen
        assert "Right Arrow:" not in help_screen, help_screen
        assert "Enter:" not in help_screen, help_screen
        for label, stale_label in (
            ("C/^K copy:", "Copy tagged:"),
            ("J compare:", "Compare:"),
            ("K volume:", "Volume:"),
            ("M/^N move:", "Move tagged:"),
            ("eXecute:", "Execute:"),
            ("pathcopY:", "Pathcopy:"),
            ("Z archive:", "Archive:"),
            ("/ jump:", "Jump:"),
            ("` dotfiles:", "Dotfiles:"),
        ):
            help_screen = _scroll_help_to_text(tui, label)
            assert label in help_screen, help_screen
            assert stale_label not in help_screen, help_screen
        help_screen = _send_help_key_until_text(tui, Keys.HOME, "C/^K copy:")
        copy_detail = _follow_help_topic(
            tui, "C/^K copy:", "Copy/Move Targets", timeout=1.0
        )
        normalized_copy_detail = _normalized_help_text(copy_detail)
        assert "destination directory" in normalized_copy_detail, copy_detail
        assert "local mode page still owns which key copies or moves" in normalized_copy_detail, copy_detail
        assert "wildcard rename pattern" in normalized_copy_detail, copy_detail
        assert "*.bak" not in normalized_copy_detail, copy_detail
        assert "Ctrl-K copies the tagged set" not in normalized_copy_detail, copy_detail
        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("F8 Split File Help", timeout=1.0), screen_text(tui)
        help_screen = screen_text(tui)
        assert _popup_frame(help_screen, "F8 Split File Help") == split_frame, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F7)
        preview_screen = _wait_for_help(tui, "F7 Preview Help")
        assert "F8: Split does nothing while preview is active." in preview_screen, preview_screen
        assert "F7:" in preview_screen, preview_screen
        preview_frame = _popup_frame(preview_screen, "F7 Preview Help")
        preview_body = "\n".join(
            line[preview_frame["left"] + 1 : preview_frame["right"]]
            for line in preview_screen.splitlines()[
                preview_frame["title_row"] + 1 : preview_frame["footer_row"]
            ]
        )
        assert "PREVIEW" not in preview_body, preview_screen
        assert "COMMANDS" not in preview_body, preview_screen
        for label in (
            "Attributes:",
            "C/^K copy:",
            "Filter:",
            "J compare:",
            "M/^N move:",
            "eXecute:",
            "pathcopY:",
            "Z archive:",
            "/ jump:",
            "` dotfiles:",
        ):
            preview_screen = _scroll_help_to_text(tui, label)
            assert label in preview_screen, preview_screen
        assert "File commands:" not in preview_screen, preview_screen
        for nav_label in (
            "Select file:",
            "Preview lines:",
            "Preview pages:",
            "Ctrl-P and Ctrl-N",
            "Shift-PgUp and Shift-PgDn",
            ):
                assert nav_label not in preview_screen, preview_screen
        assert preview_frame == split_frame, preview_screen
    finally:
        tui.quit()


def test_showall_help_opens_scope_explainer_and_returns(tmp_path):
    root = _root_with_file(tmp_path, "showall_global_help_navigation")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke("s", wait=0.4)

        showall_help = _wait_for_help(tui, "Showall Help")
        assert "Showall lists every file inside the current logged volume only." in showall_help, showall_help
        assert "Return to the previously selected directory." in showall_help, showall_help
        assert "owner directory" in showall_help, showall_help

        scope_help_screen = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines
            if any("Scope" in line for line in lines)
            and not any("Showall Help" in line for line in lines)
            else False,
            timeout=1.5,
        )
        assert scope_help_screen, screen_text(tui)
        scope_help = "\n".join(scope_help_screen)
        assert "Showall lists every file inside the current logged volume only." in scope_help, scope_help

        showall_again = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Showall Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert showall_again, screen_text(tui)

        showall_help = _scroll_help_to_text(tui, "Sort:")
        assert "Repeating S changes sort" in showall_help, showall_help

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_output_prompt_f1_help_uses_generated_runtime_topics(tmp_path):
    root = _root_with_file(tmp_path, "output_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("o", wait=0.2)

        assert tui.wait_for_content("Format:", timeout=1.0), screen_text(tui)
        format_help = _wait_for_help(tui, "Output Format Help")
        format_normalized = _normalized_help_text(format_help)
        assert "Raw writes content with no extra framing." in format_normalized, format_help
        assert "Page break inserts a separator between files" in format_normalized, format_help
        assert "skips a trailing separator at the end." in format_normalized, format_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Format:", timeout=1.0), screen_text(tui)

        tui.send_keystroke("P", wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        separator_help = _wait_for_help(tui, "Output Separator Help")
        assert "default triple-backtick fence" in _normalized_help_text(separator_help), separator_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)

        assert tui.wait_for_content("Output to:", timeout=1.0), screen_text(tui)
        destination_help = _wait_for_help(tui, "Output Destination Help")
        destination_normalized = _normalized_help_text(destination_help)
        destination_lower = destination_normalized.lower()
        assert "file output writes exported text to a path" in destination_lower, destination_help
        assert "hardcopy sends exported text to the chosen printer command" in destination_lower, destination_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Output to:", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_integrated_help_directory_and_file_modes_do_not_crash(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_scope_lifetime")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen
        assert "Attributes:" in help_screen, help_screen
        footer_line = next(
            line for line in help_screen.splitlines() if "Esc/Quit" in line
        )
        assert "Open" in footer_line, footer_line
        assert "Contents" in footer_line, footer_line
        assert "Shared commands" not in footer_line, footer_line
        assert "F8 split" not in footer_line, footer_line

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "File Help")
        assert "pathcopy" in help_screen.lower(), help_screen
    finally:
        tui.quit()


def test_picker_dialog_f1_help_covers_volume_and_applications(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_picker_dialogs")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        tui.send_keystroke("K")
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        volume_screen = _wait_for_help(tui, "Volume Help")
        volume_normalized = _normalized_help_text(volume_screen)
        assert "Use Up and Down to choose a loaded volume." in volume_normalized, volume_screen
        assert "Use Enter to switch to it." in volume_normalized, volume_screen
        assert "Use D to release it, unless it is the last one." in volume_normalized, volume_screen
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F9)
        assert tui.wait_for_content("Applications", timeout=1.0), screen_text(tui)
        applications_screen = _wait_for_help(tui, "Applications Help")
        normalized = _normalized_help_text(applications_screen)
        assert "Use Enter to select the highlighted preset." in normalized, applications_screen
        assert "Use E to edit the commands catalog that backs application presets." in normalized, applications_screen
        assert "Use Esc to cancel the menu." in normalized, applications_screen
        assert "placeholder surface" in normalized, applications_screen
    finally:
        tui.quit()


def test_picker_dialog_f1_help_covers_f2_dotfiles_and_local_actions(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_f2_picker")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_condition(
            lambda lines: lines if any("file view" in line.lower() for line in lines[-3:]) else False,
            timeout=1.0,
            poll_interval=0.05,
        ), screen_text(tui)
        tui.send_keystroke("c")
        assert tui.wait_for_content("COPY:", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("To Directory:", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.F2)
        assert tui.wait_for_content("cycle", timeout=1.0), screen_text(tui)
        f2_screen = _normalized_help_text(_wait_for_help(tui, "F2 Picker Help"))
        assert "Use < and > to cycle loaded volumes." in f2_screen, f2_screen
        assert "Use L to log a new path." in f2_screen, f2_screen
        assert "toggle dotfiles." in f2_screen, f2_screen
    finally:
        tui.quit()


def test_copy_prompt_f1_help_describes_name_then_destination_exception(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_copy_target_prompt")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_condition(
            lambda lines: lines if any("file view" in line.lower() for line in lines[-3:]) else False,
            timeout=1.0,
            poll_interval=0.05,
        ), screen_text(tui)
        tui.send_keystroke("c")
        assert tui.wait_for_content("COPY:", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui)
        assert "F1 help  F1 help" not in prompt_screen, prompt_screen
        assert "F1 help" in prompt_screen and "F2 browse" not in prompt_screen, prompt_screen

        help_screen = _normalized_help_text(_wait_for_help(tui, "Copy/Move Targets"))
        lower_help = help_screen.lower()
        assert "destination directory" in lower_help, help_screen
        assert "full replacement name" in lower_help, help_screen
        assert "wildcard rename pattern" in lower_help, help_screen
    finally:
        tui.quit()


def test_archive_f1_help_uses_archive_specific_context_titles(tmp_path):
    root = tmp_path / "integrated_help_archive_contexts"
    root.mkdir()
    archive_path = root / "inside.tar"
    _create_tar(archive_path, {"inside_dir/inside.txt": "inside payload"})
    tui = _spawn_help_tui(root)

    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0), screen_text(tui)

        help_screen = _wait_for_help(tui, "Archive Directory Help")
        seen_screens = [help_screen]
        for label in (
            "1..9 view:",
            "Global:",
            "J compare:",
            "K volume:",
            "Log:",
            "Showall:",
            "/ jump:",
        ):
            help_screen = _scroll_help_to_text(tui, label)
            seen_screens.append(help_screen)
        dir_help = "\n".join(seen_screens).lower()
        for label in ("1..9 view:", "global:", "j compare:", "k volume:", "log:", "showall:", "/ jump:"):
            assert label in dir_help, dir_help
        for nav_label in ("enter:", "left arrow:", "right arrow:"):
            assert nav_label not in dir_help, help_screen
        assert "archive directory help only covers" not in dir_help, help_screen
        assert "see dir for the normal directory/tree baseline" not in dir_help, help_screen
        assert "copy" not in dir_help, help_screen
        assert "movedir" not in dir_help, help_screen
        assert "newfile" not in dir_help, help_screen
        assert "write" not in dir_help, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("inside_dir", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("inside.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Archive File Help")
        file_help = help_screen.lower()
        for label in ("1..9 view:", "c/^k copy:", "delete:", "filter:", "hex:", "j compare:", "k volume:", "m/^n move:"):
            assert label in file_help, help_screen
        assert "archive file help only covers" not in file_help, help_screen
        assert "see file for the normal file-mode baseline" not in file_help, help_screen
        assert "through archive-aware extract/copy paths" in file_help, help_screen
        assert "copy tagged:" not in file_help, help_screen
        assert "move tagged:" not in file_help, help_screen
        assert "not available in archive file mode" not in file_help, help_screen
    finally:
        tui.quit()


def test_contextual_help_uses_page_keys_and_only_links_complex_commands(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_page_keys")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.END, "` dotfiles:")
        assert "` dotfiles:" in help_screen, help_screen

        unchanged = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert unchanged, screen_text(tui)
        assert not any("` dotfiles" == line.strip() for line in unchanged), "\n".join(unchanged)

        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1..9 view:")
        assert "1..9 view:" in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.PGDN, "moVedir:")
        assert "moVedir:" in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.PGUP, "1..9 view:")
        assert "1..9 view:" in help_screen, help_screen
    finally:
        tui.quit()


def test_command_preset_help_layers_packaged_selection_under_local_overrides(tmp_path):
    root = _root_with_file(tmp_path, "command_preset_help_layering")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text(
        "preset = de\n"
        "[FILE]\n"
        "C | C | Copy | ACTION_CMD_C |\n",
        encoding="utf-8",
    )
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "File Help")
        lower_help = help_screen.lower()
        assert "copy" in lower_help, help_screen
        assert "datei loeschen" in lower_help, help_screen
        assert "kopieren" not in lower_help, help_screen
    finally:
        tui.quit()


def test_invalid_command_preset_aborts_startup(tmp_path):
    root = _root_with_file(tmp_path, "invalid_command_preset_startup")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text("preset = missing-preset\n", encoding="utf-8")
    tui = _spawn_help_tui(root)

    try:
        screen = tui.wait_for_condition(
            lambda lines: lines
            if (not tui.child.isalive())
            and any("LoadCommands failed" in line for line in lines)
            else False,
            timeout=2.0,
        )
        assert screen, screen_text(tui)
    finally:
        tui.quit()


def test_integrated_help_source_covers_archive_showall_and_history_surfaces():
    display_source = _read_source("src/ui/display.c")
    integrated_block = _extract_function_block(
        display_source, "int UI_ShowIntegratedHelp(ViewContext *ctx, const DirEntry *dir_entry) {"
    )
    assert "ShowGeneratedHelpForPlan" in integrated_block
    assert "UI_ShowGeneratedContextHelpWithOverrides" in display_source
    assert '"overlay.f7-dir"' in integrated_block
    assert '"overlay.f8-dir"' in integrated_block
    assert '"main.global"' in integrated_block
    assert '"Showall/Global File Help"' not in integrated_block
    assert "AppendPopupStripRow" not in integrated_block
    assert "AppendResolvedFooterRows" not in integrated_block

    runtime_help_source = _read_source("src/ui/runtime_help.c")
    assert "generated_help_topics.h" in runtime_help_source
    assert "FindGeneratedTopicByContext" in runtime_help_source

    compare_source = _read_source("src/ui/compare_request.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, spec->context_id, NULL, 0)' in compare_source
    assert "UI_HELP_POPUP_COMMAND_STRIP" not in _extract_function_block(
        compare_source,
        "static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {",
    )

    interactions_source = _read_source("src/ui/interactions.c")
    assert "UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0)" in interactions_source

    history_display_source = _read_source("src/ui/display.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.history", NULL, 0)' in history_display_source

    history_source = _read_source("src/ui/history_dialog.c")
    assert "case KEY_F(1):" in history_source

    volume_source = _read_source("src/ui/volume_menu.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.volume-menu", NULL, 0)' in volume_source

    applications_source = _read_source("src/ui/application_menu.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.applications", NULL, 0)' in applications_source

    f2_source = _read_source("src/ui/f2_picker.c")
    assert "case ACTION_HELP:" in f2_source
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.f2-picker", NULL, 0)' in f2_source
