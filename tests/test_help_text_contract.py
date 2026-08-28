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


def _spawn_help_tui(root, env_extra=None, dimensions=(36, 120)):
    return YtreeNovaTUI(
        executable=YTNOVA_BIN,
        cwd=str(root),
        env_extra=env_extra,
        dimensions=dimensions,
    )


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


def _popup_frame(screen, title, footer_hint="Esc/Q quit"):
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


def _scroll_help_to_text(tui, text, *, steps=80):
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


def _open_tagged_help_from_index(tui):
    _send_help_key_until_text(tui, Keys.END, "Tagged:")
    tracked_labels = ("Tagged:", "Tagged Viewer:", "Theming:", "Vi Keys:", "K volume:")

    for _ in range(12):
        tagged_style = _visible_cell_style(tui, "Tagged:")
        viewer_style = _visible_cell_style(tui, "Tagged Viewer:")
        theming_style = _visible_cell_style(tui, "Theming:")
        if viewer_style != tagged_style and viewer_style != theming_style:
            before = tuple(_visible_cell_style(tui, label) for label in tracked_labels)
            tui.child.send(Keys.UP)
            tui.wait_for_condition(
                lambda lines: lines
                if tuple(_visible_cell_style(tui, label) for label in tracked_labels)
                != before
                else False,
                timeout=0.5,
            )
            opened = tui.send_and_wait_for_condition(
                Keys.RIGHT,
                lambda lines: lines
                if any("Tags select several files" in line for line in lines)
                else False,
                timeout=1.0,
            )
            assert opened, screen_text(tui)
            return "\n".join(opened)

        before = tuple(_visible_cell_style(tui, label) for label in tracked_labels)
        tui.child.send(Keys.UP)
        tui.wait_for_condition(
            lambda lines: lines
            if tuple(_visible_cell_style(tui, label) for label in tracked_labels)
            != before
            else False,
            timeout=0.5,
        )

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
        assert "1: Name only." in help_screen, help_screen
        selected_style = _visible_cell_style(tui, "1: Name only.")

        tui.send_keystroke(csi_down, wait=0.05)
        assert (
            _selected_visible_help_label(
                tui, ["1: Name only.", "2: Attributes.", "Copy:"], selected_style
            )
            != "1: Name only."
        ), screen_text(tui)

        title_row, detail_screen = _open_current_help_detail_title(tui, csi_right)
        assert title_row and "Directory Help" not in title_row, detail_screen

        returned = tui.send_and_wait_for_condition(
            csi_left,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert returned, screen_text(tui)
        assert any(
            "Copy:" in line or "2: Attributes." in line for line in returned
        ), "\n".join(returned)
    finally:
        tui.quit()


def test_contextual_help_accepts_application_arrow_sequences(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_app_arrows")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen
        selected_style = _visible_cell_style(tui, "1: Name only.")

        tui.send_keystroke(Keys.DOWN, wait=0.05)
        assert (
            _selected_visible_help_label(
                tui, ["1: Name only.", "2: Attributes.", "Copy:"], selected_style
            )
            != "1: Name only."
        ), screen_text(tui)

        title_row, detail_screen = _open_current_help_detail_title(tui, Keys.RIGHT)
        assert title_row and "Directory Help" not in title_row, detail_screen

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert returned, screen_text(tui)
        assert any(
            "Copy:" in line or "2: Attributes." in line for line in returned
        ), "\n".join(returned)
    finally:
        tui.quit()


def test_help_popup_does_not_turn_fragmented_arrow_prefixes_into_escape():
    source = _read_source("src/ui/key_engine.c")
    normalizer = _extract_function_block(
        source,
        "static int NormalizeEscSequenceForWindow(WINDOW *win, int ch)",
    )

    assert "nodelay(win, TRUE)" not in normalizer
    assert "wtimeout(win, ESC_SEQUENCE_TIMEOUT_MS)" in normalizer


def test_contextual_help_down_arrow_advances_hidden_active_link(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_active_link")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen
        selected_style = _visible_cell_style(tui, "1: Name only.")

        tui.send_keystroke(Keys.DOWN, wait=0.05)
        assert (
            _selected_visible_help_label(
                tui, ["1: Name only.", "2: Attributes.", "Copy:"], selected_style
            )
            != "1: Name only."
        ), screen_text(tui)

        title_row, detail_screen = _open_current_help_detail_title(tui, Keys.RIGHT)
        assert title_row and "Directory Help" not in title_row, detail_screen
    finally:
        tui.quit()


def test_contextual_help_down_arrow_skips_plain_rows_then_scrolls(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_down_scroll")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen

        moved = tui.send_and_wait_for_condition(
            Keys.DOWN,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert moved, screen_text(tui)

        copy_detail = _follow_help_topic(tui, "Copy:", "Copy/Move Targets", timeout=1.0)
        assert "Copy/Move Targets" in copy_detail, copy_detail

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
        scrolled = _scroll_help_to_text(tui, "eXecute:")
        assert "eXecute:" in scrolled, scrolled
        assert "1: Name only." not in scrolled, scrolled
    finally:
        tui.quit()


def test_contextual_help_down_arrow_scrolls_past_write_to_lower_commands(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_scroll_lower_commands")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen

        lower_commands = tui.wait_for_condition(
            lambda lines: lines if any("eXecute:" in line for line in lines) else False,
            timeout=2.0,
            poll_interval=0.02,
        )
        assert not lower_commands, screen_text(tui)

        for _ in range(64):
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
        assert "1: Name only." in help_screen, help_screen

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


def test_contextual_help_wraps_long_directory_rows_to_fit_popup(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_wraps_long_rows")
    tui = _spawn_help_tui(root, dimensions=(36, 90))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        frame = _popup_frame(help_screen, "Directory Help")
        popup_lines = [
            line[frame["left"] + 1 : frame["right"]]
            for line in help_screen.splitlines()[frame["title_row"] + 1 : frame["footer_row"]]
        ]
        wrapped_index = next(
            i for i, line in enumerate(popup_lines) if "Tree versus file window:" in line
        )

        wrapped_text = " ".join(
            line.strip()
            for line in popup_lines[wrapped_index : wrapped_index + 2]
        )
        assert "Tree versus file window: In directory focus, 5, 7, 8, and 9" in wrapped_text, help_screen
        assert "do not change the tree rows." in popup_lines[wrapped_index + 1], help_screen
        assert popup_lines[wrapped_index + 2].strip() == "", help_screen
    finally:
        tui.quit()


def test_help_index_uses_the_width_reserved_by_its_footer(tmp_path):
    root = _root_with_file(tmp_path, "help_index_footer_width")
    tui = _spawn_help_tui(root, dimensions=(36, 70))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        contents = _send_help_key_until_text(tui, "i", "Help Index")

        assert "Archive Directory: The archive-directory footer acts" in contents, contents
    finally:
        tui.quit()


def test_help_index_reopens_cleanly_after_terminal_resize(tmp_path):
    root = _root_with_file(tmp_path, "help_index_terminal_resize")
    tui = _spawn_help_tui(root, dimensions=(36, 120))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")

        tui.child.setwinsize(24, 70)
        tui.screen.resize(24, 70)
        resized = tui.wait_for_condition(
            lambda lines: lines
            if any("Help Index" in line for line in lines)
            and any("Esc/Q quit" in line for line in lines)
            else False,
            timeout=1.5,
        )

        assert resized, screen_text(tui)
        frame = _popup_frame("\n".join(resized), "Help Index")
        assert frame["right"] < 70
        assert any("COMMANDS" in line for line in resized), screen_text(tui)

        restored = tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda lines: lines
            if any("COMMANDS" in line for line in lines)
            and not any("Esc/Q quit" in line for line in lines)
            else False,
            timeout=1.5,
        )
        assert restored, screen_text(tui)
        assert any("FILTER" in line for line in restored), screen_text(tui)
    finally:
        tui.quit()


def test_ytnova_navigation_opens_inline_help_links(tmp_path):
    root = _root_with_file(tmp_path, "ytnova_navigation_related_help")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")
        navigation = _follow_help_topic(tui, "Navigation:", "YtreeNova Navigation")

        normalized_navigation = _normalized_help_text(navigation)
        assert "YtreeNova is built for keyboard use." in normalized_navigation, navigation
        assert "Mouse effects may occur" in normalized_navigation, navigation
        assert "rather than designed controls." in normalized_navigation, navigation

        navigation = _send_help_key_until_text(tui, Keys.END, "F8 split")
        assert "/ jump" in navigation, navigation
        assert "F7 preview" in navigation, navigation
        assert "F8 split" in navigation, navigation
        assert "topic:" not in navigation, navigation
        assert "Related help" not in navigation, navigation

        tui.send_keystroke(Keys.DOWN, wait=0.05)
        linked = _send_help_key_until_text(tui, Keys.RIGHT, "List Jump")
        assert "Type letters" in linked, linked

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        _send_help_key_until_text(tui, Keys.RIGHT, "List Jump")

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        _send_help_key_until_text(tui, Keys.END, "F8 split")
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        linked = _send_help_key_until_text(tui, Keys.RIGHT, "F7 Preview Help")
        assert "Keep moving the selected file" in _normalized_help_text(linked), linked

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        tui.send_keystroke(Keys.UP, wait=0.05)
        linked = _send_help_key_until_text(tui, Keys.RIGHT, "F8 Split")
        assert "active panel" in _normalized_help_text(linked), linked
    finally:
        tui.quit()


def test_tagged_help_renders_and_opens_inline_topic_links(tmp_path):
    root = _root_with_file(tmp_path, "tagged_inline_help")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")
        tagged = _open_tagged_help_from_index(tui)

        assert "Tags select several files" in tagged, tagged
        assert "topic:" not in tagged, tagged
        assert "[copy]" not in tagged, tagged

        target = "searched tagged files"
        for _ in range(60):
            current = screen_text(tui)
            if target in current:
                break
            changed = tui.send_and_wait_for_condition(
                Keys.DOWN,
                lambda lines: lines
                if any(target in line for line in lines)
                or "\n".join(lines) != current
                else False,
                timeout=0.4,
            )
            if changed and any(target in line for line in changed):
                break

        tagged = screen_text(tui)
        assert target in tagged, tagged
        assert "(topic:" not in tagged, tagged
        assert any(target in line for line in tagged.splitlines()), tagged

        link_style = _visible_cell_style(tui, target)
        prose_style = _visible_cell_style(tui, "untags files")
        assert link_style != prose_style, tagged
        assert not link_style[3], tagged

        for _ in range(16):
            if _visible_cell_style(tui, target) != link_style:
                break
            tui.child.send(Keys.DOWN)
            tui.wait_for_condition(
                lambda lines: lines
                if _visible_cell_style(tui, target) != link_style
                else False,
                timeout=0.4,
            )

        assert _visible_cell_style(tui, target) != link_style, screen_text(tui)
        opened = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("Enter plain search text only" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert opened, screen_text(tui)
        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines
            if any(target in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
    finally:
        tui.quit()


def test_related_help_list_does_not_leave_links_below_the_popup_footer(tmp_path):
    root = _root_with_file(tmp_path, "related_help_list_fits")
    tui = _spawn_help_tui(root, dimensions=(43, 106))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")
        _follow_help_topic(tui, "Navigation:", "YtreeNova Navigation")

        navigation = ""
        for _ in range(20):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            navigation = screen_text(tui)
            if "/ jump" in navigation:
                break

        assert "/ jump" in navigation, navigation
        assert "F7 preview" in navigation, navigation
        assert "F8 split" in navigation, navigation
    finally:
        tui.quit()


def test_directory_help_shows_explainer_links_as_selectable_related_help(tmp_path):
    root = _root_with_file(tmp_path, "directory_related_help")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        directory = _send_help_key_until_text(tui, Keys.END, "Related help")

        assert "Navigation" in directory, directory
        assert "Copy" in directory, directory
    finally:
        tui.quit()


def test_contextual_help_keeps_blank_gap_above_footer_while_scrolled(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_footer_gap")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen

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


def test_contextual_help_keeps_blank_gap_above_footer_at_bottom(tmp_path):
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

        assert frame["footer_row"] - last_content_row >= 2, help_screen
    finally:
        tui.quit()


def test_contextual_help_up_arrow_reselects_visible_links_when_scrolling_back(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_up_reselects")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen

        reached_execute = False
        for _ in range(80):
            tui.send_keystroke(Keys.DOWN, wait=0.05)
            current = screen_text(tui)
            if "eXecute:" in current:
                reached_execute = True
                break

        assert reached_execute, screen_text(tui)

        restored = False
        for _ in range(80):
            tui.send_keystroke(Keys.UP, wait=0.05)
            current = screen_text(tui)
            if "J compare:" in current:
                restored = True
                break

        assert restored, screen_text(tui)
        for _ in range(8):
            if _visible_cell_style(tui, "J compare:") != _visible_cell_style(
                tui, "K volume:"
            ):
                break
            tui.send_keystroke(Keys.UP, wait=0.05)
        assert _visible_cell_style(tui, "J compare:") != _visible_cell_style(
            tui, "K volume:"
        ), screen_text(tui)
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
        help_screen = _scroll_help_to_text(tui, "C/^Copy:")
        assert "C/^Copy:" in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1: Name only.")
        first_detail = _follow_help_topic(
            tui, "C/^Copy:", "Copy/Move Targets", timeout=1.0
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

        assert "D/^Delete" in footer, footer
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
        assert "The prompt starts with {} for the selected file path" in normalized, help_screen
        assert "C-x" in help_screen, help_screen
        assert "repeat the same command once per tagged file" in normalized, help_screen

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
        assert "Type one or more filter terms for the current file list." in normalized, filter_help
        assert "* means show everything" in normalized, filter_help
        assert "*.c matches by name" in normalized, filter_help
        assert "Separate terms with commas so they all apply together." in normalized, filter_help
        assert "switch between all files and tagged files" in normalized, filter_help

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
        assert "1: Name only." in directory_help_screen, directory_help_screen
        assert "2: Attributes." in directory_help_screen, directory_help_screen
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
            line for line in help_screen.splitlines() if "Esc/Q quit" in line
        )
        assert "Enter/Right open link" in footer_line, footer_line
        assert "Index" in footer_line, footer_line
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
            i for i, line in enumerate(help_lines) if "1: Name only." in line
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
        assert "1: Name only." in help_screen, help_screen
        assert "F8:" in help_screen, help_screen
        assert "Leave split:" not in help_screen, help_screen
        help_screen = _scroll_help_to_text(tui, "J compare:")
        assert "J compare:" in help_screen, help_screen
        help_screen = _scroll_help_to_text(tui, "eXecute:")
        assert "eXecute:" in help_screen, help_screen
        assert "Left Arrow:" not in help_screen, help_screen
        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1: Name only.")
        help_screen = _scroll_help_to_text(tui, "Copy:")
        _follow_help_topic(
            tui, "Copy:", "Copy/Move Targets", timeout=1.0
        )
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
        assert "1: Name only." in help_screen, help_screen
        assert "Left Arrow:" not in help_screen, help_screen
        assert "Right Arrow:" not in help_screen, help_screen
        assert "Enter:" not in help_screen, help_screen
        for label, stale_label in (
            ("C/^Copy:", "Copy tagged:"),
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
        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1: Name only.")
        help_screen = _scroll_help_to_text(tui, "C/^Copy:")
        _follow_help_topic(
            tui, "C/^Copy:", "Copy/Move Targets", timeout=1.0
        )
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
            "C/^Copy:",
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
            "C-p and C-n",
            "Shift-PgUp and Shift-PgDn",
            ):
                assert nav_label not in preview_screen, preview_screen
        assert preview_frame == split_frame, preview_screen
    finally:
        tui.quit()


def test_showall_help_keeps_scope_details_and_returns(tmp_path):
    root = _root_with_file(tmp_path, "showall_global_help_navigation")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke("s", wait=0.4)

        showall_help = _wait_for_help(tui, "Showall Help")
        assert "Showall lists every file inside the current logged volume only." in showall_help, showall_help
        assert "Return to the previously selected directory." in showall_help, showall_help
        assert "owner directory" in showall_help, showall_help

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

        assert tui.wait_for_content("Output to:", timeout=1.0), screen_text(tui)
        destination_help = _wait_for_help(tui, "Output Destination Help")
        destination_normalized = _normalized_help_text(destination_help)
        destination_lower = destination_normalized.lower()
        assert "file output writes exported text to a path" in destination_lower, destination_help
        assert "cwd" in destination_lower and "current working directory" in destination_lower, destination_help
        assert "hardcopy sends raw exported text" in destination_lower, destination_help
        assert "cat > /dev/lp1" in destination_lower, destination_help
        assert "f3" in destination_lower and "file destination prompt" in destination_lower, destination_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Output to:", timeout=1.0), screen_text(tui)

        tui.send_keystroke("F", wait=0.2)
        assert tui.wait_for_content("Output file [Raw]", timeout=1.0), screen_text(tui)
        destination_help = _wait_for_help(tui, "Output Destination Help")
        destination_normalized = _normalized_help_text(destination_help)
        destination_lower = destination_normalized.lower()
        assert "press" in destination_lower and "f3" in destination_lower, destination_help
        assert "page break" in destination_lower and "separator" in destination_lower, destination_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Output file [Raw]", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F3, wait=0.2)
        assert tui.wait_for_content("Frame separator", timeout=1.0), screen_text(tui)
        separator_help = _wait_for_help(tui, "Output Separator Help")
        assert "default triple-backtick fence" in _normalized_help_text(separator_help), separator_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Frame separator", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        assert tui.wait_for_content("Output file [Framed]", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.F3, wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        tui.send_and_wait_for_screen_change("---SEP---" + Keys.ENTER, timeout=1.5)
        assert tui.wait_for_content("Output file [Page break]", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_integrated_help_directory_and_file_modes_do_not_crash(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_scope_lifetime")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1: Name only." in help_screen, help_screen
        assert "2: Attributes." in help_screen, help_screen
        footer_line = next(
            line for line in help_screen.splitlines() if "Esc/Q quit" in line
        )
        assert "open" in footer_line, footer_line
        assert "Index" in footer_line, footer_line
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


def test_help_index_opens_from_contextual_help_and_returns_to_origin(tmp_path):
    root = _root_with_file(tmp_path, "help_index")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        _wait_for_help(tui, "Directory Help")
        contents_screen = tui.send_and_wait_for_condition(
            "i",
            lambda lines: lines
            if any("Help Index" in line for line in lines)
            and any("Applications" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert contents_screen, screen_text(tui)
        contents_text = "\n".join(contents_screen)
        footer_line = next(
            line for line in contents_text.splitlines() if "Esc/Q quit" in line
        )
        assert "open" in footer_line, footer_line
        assert "Navigation" in footer_line, footer_line
        assert "Applications" in contents_text, contents_text
        assert "Archive Directory" in contents_text, contents_text

        selected_style = _visible_cell_style(tui, "Applications")
        reached_copy_targets = False
        for _ in range(24):
            if (
                _selected_visible_help_label(
                    tui, ["Copy/Move Targets"], selected_style
                )
                == "Copy/Move Targets"
            ):
                reached_copy_targets = True
                break
            tui.send_keystroke(Keys.DOWN, wait=0.05)

        assert reached_copy_targets, screen_text(tui)
        contents_text = screen_text(tui)
        assert "Copy/Move Targets" in contents_text, contents_text

        detail_screen = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines
            if any("Copy/Move Targets" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert detail_screen, screen_text(tui)
        detail_text = "\n".join(detail_screen)
        assert "Copy/Move Targets" in detail_text, detail_text

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        returned = tui.wait_for_condition(
            lambda lines: lines
            if any("Help Index" in line for line in lines)
            else False,
            timeout=1.0,
            poll_interval=0.05,
        )
        assert returned, screen_text(tui)
        returned_text = "\n".join(returned)
        assert "Copy/Move Targets" in returned_text, returned_text
        assert (
            _selected_visible_help_label(
                tui, ["Copy/Move Targets"], selected_style
            )
            == "Copy/Move Targets"
        ), returned_text

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("Directory Help", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC, wait=0.05)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_return_restores_scrolled_related_link(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_related_link_history")
    tui = _spawn_help_tui(root, dimensions=(40, 118))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, Keys.PGDN, "Makedir")
        source = _send_help_key_until_text(tui, Keys.PGDN, "Related help")
        assert "Navigation" in source, source

        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=1.0)
        _send_help_key_until_text(tui, Keys.RIGHT, "YtreeNova Navigation")

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines
            if any("Directory Help" in line for line in lines)
            and any("Related help" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
        assert "Navigation" in "\n".join(returned), returned

        _send_help_key_until_text(tui, Keys.RIGHT, "YtreeNova Navigation")
    finally:
        tui.quit()


def test_contextual_help_detail_footer_uses_left_back_and_i_index(tmp_path):
    root = _root_with_file(tmp_path, "help_detail_footer_contract")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        _wait_for_help(tui, "Directory Help")
        detail_screen = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("Esc/Q quit" in line and "Left back" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert detail_screen, screen_text(tui)
        detail_text = "\n".join(detail_screen)
        footer_line = next(
            line for line in detail_text.splitlines() if "Esc/Q quit" in line
        )
        assert "Left back" in footer_line, footer_line
        assert "Index" in footer_line, footer_line
        assert "Navigation" in footer_line, footer_line
        assert "I back" not in footer_line, footer_line
        assert "Directory mode" not in footer_line, footer_line
        assert "File mode" not in footer_line, footer_line
        assert "Archive file" not in footer_line, footer_line
        assert "F8 split" not in footer_line, footer_line

        contents_screen = tui.send_and_wait_for_condition(
            "i",
            lambda lines: lines
            if any("Applications:" in line for line in lines)
            and any("Esc/Q quit" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert contents_screen, screen_text(tui)
        contents_text = "\n".join(contents_screen)
        assert (
            "Change the active panel's directory and file presentation."
            not in contents_text
        ), contents_text
        footer_line = next(
            line for line in contents_text.splitlines() if "Esc/Q quit" in line
        )
        assert "open" in footer_line, footer_line
    finally:
        tui.quit()


def test_help_popup_plain_key_tokens_use_help_footer_style(tmp_path):
    root = _root_with_file(tmp_path, "help_popup_plain_key_style")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "ytnova.conf").write_text(
        "[GLOBAL]\nTHEME=sample\nSMALLWINDOWSKIP=1\n",
        encoding="utf-8",
    )
    (config_dir / "themes.conf").write_text(
        """
[theme sample]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = magenta on white
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_footer = white on magenta
help_keybind = yellow on cyan
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""".strip()
        + "\n",
        encoding="utf-8",
    )

    tui = _spawn_help_tui(root)

    try:
        help_screen = _wait_for_help(tui, "Directory Help")
        assert "Enter/Right open link" in help_screen, help_screen
        footer_style = _visible_cell_style(tui, "open link")
        enter_style = _visible_cell_style(tui, "Enter/Right")
        quit_style = _visible_cell_style(tui, "Esc/Q quit")
        assert enter_style == footer_style, (
            "Help popup plain-text key tokens must use help_footer styling.\n"
            f"Enter/Right={enter_style} footer={footer_style}\n\n{screen_text(tui)}"
        )
        assert quit_style == footer_style, (
            "Help popup Esc/Q quit token must use help_footer styling.\n"
            f"Esc/Q quit={quit_style} footer={footer_style}\n\n{screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_main_footer_signpost_keybind_uses_footer_background(tmp_path):
    root = _root_with_file(tmp_path, "footer_signpost_theme_contract")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "ytnova.conf").write_text(
        "[GLOBAL]\nTHEME=sample\nSMALLWINDOWSKIP=1\n",
        encoding="utf-8",
    )
    (config_dir / "themes.conf").write_text(
        """
[theme sample]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = magenta on white
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_keybind = yellow
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""".strip()
        + "\n",
        encoding="utf-8",
    )

    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        nav_style = _visible_cell_style(tui, "←qj")
        footer_style = _visible_cell_style(tui, "File F1 help")
        assert nav_style[1] == footer_style[1], (
            "Footer navigation signpost must inherit the configured footer "
            "background instead of keeping the main surface background.\n"
            f"Nav={nav_style} Footer={footer_style}\n\n{screen_text(tui)}"
        )
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
        assert (
            "After launch, ytnova keeps running and the application continues on its own."
            in normalized
        ), applications_screen
        assert (
            "Use E to edit the applications catalog that backs application presets."
            in normalized
        ), applications_screen
        assert "Use Esc to cancel the menu." in normalized, applications_screen
        assert (
            "Use {} for the file or folder currently selected in ytnova."
            in normalized
        ), applications_screen
        assert (
            "Use {input} for the text you type when the preset asks for extra input."
            in normalized
        ), applications_screen
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
            "1: Name only.",
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
        for label in ("1: name only.", "global:", "j compare:", "k volume:", "log:", "showall:", "/ jump:"):
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
        seen_screens = [help_screen]
        for label in (
            "1: Name only.",
            "C/^Copy:",
            "Delete:",
            "Filter:",
            "Hex:",
            "J compare:",
            "K volume:",
            "M/^N move:",
        ):
            help_screen = _scroll_help_to_text(tui, label)
            seen_screens.append(help_screen)
        file_help = "\n".join(seen_screens).lower()
        for label in ("1: name only.", "c/^copy:", "delete:", "filter:", "hex:", "j compare:", "k volume:", "m/^n move:"):
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
        assert "1: Name only." in help_screen, help_screen

        help_screen = _send_help_key_until_text(tui, Keys.END, "` dotfiles:")
        assert "` dotfiles:" in help_screen, help_screen

        unchanged = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert unchanged, screen_text(tui)
        assert not any("` dotfiles" == line.strip() for line in unchanged), "\n".join(unchanged)

        help_screen = _send_help_key_until_text(tui, Keys.HOME, "1: Name only.")
        assert "1: Name only." in help_screen, help_screen

        help_screen = screen_text(tui)
        for _ in range(4):
            if "moVedir:" in help_screen:
                break
            tui.send_keystroke(Keys.PGDN, wait=0.05)
            help_screen = screen_text(tui)
        assert "moVedir:" in help_screen, help_screen

        help_screen = screen_text(tui)
        for _ in range(4):
            if "1: Name only." in help_screen:
                break
            tui.send_keystroke(Keys.PGUP, wait=0.05)
            help_screen = screen_text(tui)
        assert "1: Name only." in help_screen, help_screen
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


def test_help_popup_styles_inline_topic_links_as_links():
    source = _read_source("src/ui/help_popup.c")
    renderer = _extract_function_block(
        source,
        "static void RenderHelpInlineText(WINDOW *win, int y, int column, int max_width,",
    )

    assert "UI_HELP_POPUP_SPAN_LINK" in renderer
    assert "UI_ROLE_HELP_LINK" in renderer
    assert "UI_ROLE_HELP_LINK_SELECTION" in renderer
    assert "selected_link_index" in renderer
    assert "A_UNDERLINE" not in renderer
    assert "wattrset(win, COLOR_PAIR(base_role));\n      size_t rendered" in renderer


def test_runtime_help_history_restores_the_popup_viewport():
    source = _read_source("src/ui/runtime_help.c")
    popup_source = _read_source("src/ui/help_popup.c")

    assert "int scroll_line_offset;" in source
    assert "state.visible_row_offset = current_view.scroll_line_offset;" in source
    assert "current_view.scroll_line_offset = state.visible_row_offset;" in source
    assert "footer_spec->final_scroll_line" in popup_source


def test_runtime_help_strips_inline_topic_links_before_wrapping():
    source = _read_source("src/ui/runtime_help.c")
    build_rows = _extract_function_block(
        source,
        "static size_t BuildTextRows(RuntimeHelpPopupState *state,",
    )

    assert "ParseHelpMarkdown(state, line, &parsed_line)" in build_rows
    assert build_rows.index("ParseHelpMarkdown(state, line, &parsed_line)") < build_rows.index(
        "AppendWrappedParsedHelpLine(state, &row_count, &line_index,"
    )

    parser = _extract_function_block(
        source,
        "static void ParseHelpMarkdown(RuntimeHelpPopupState *state, const char *source,",
    )
    assert '"](topic:"' in parser
    assert "UI_HELP_POPUP_SPAN_LINK" in parser
    assert "target_topic_id" in parser

    wrapper = _extract_function_block(
        source,
        "static size_t NextWrappedParsedChunk(const ParsedHelpLine *line,",
    )
    assert "StrVisualLength(segment)" in wrapper
    assert "VisualPositionToBytePosition(segment, wrap_width)" in wrapper
    assert "UI_HELP_POPUP_SPAN_LINK" in wrapper


def test_final_inline_help_link_remains_selected_at_navigation_boundary():
    source = _read_source("src/ui/runtime_help.c")
    handler = _extract_function_block(
        source,
        "static int HandleGeneratedHelpFooterKey(ViewContext *ctx, int ch,",
    )

    boundary = handler.index("if (next_index == GENERATED_HELP_NO_SELECTION)")
    assert "return 0;" in handler[boundary:]
    assert "SelectInlineHelpLink(state, next_index)" in handler
    assert "FindVisibleInlineHelpLink(state, reverse)" in handler


def test_runtime_help_marks_inline_topic_links_for_rendering():
    source = _read_source("src/ui/runtime_help.c")
    parser = _extract_function_block(
        source,
        "static void ParseHelpMarkdown(RuntimeHelpPopupState *state, const char *source,",
    )
    header = _read_source("include/ytnova_ui.h")

    assert "UIHelpPopupSpan" in header
    assert "const UIHelpPopupSpan *spans" in header
    assert "AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_LINK" in parser
    assert "GENERATED_HELP_LINK_MARKER" not in source
