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


def _scroll_help_to_text(tui, text, *, steps=80):
    current = screen_text(tui)
    unchanged_steps = 0

    if text in current:
        return current

    attempts = 0
    while text not in current:
        if attempts >= steps:
            raise AssertionError(
                f"Could not navigate help to {text!r}.\n{current}"
            )
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
        attempts += 1
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

    attempts = 0
    while True:
        if attempts >= steps:
            raise AssertionError(
                f"Could not open help detail {detail_title!r}.\n{screen_text(tui)}"
            )
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
        attempts += 1


def _follow_help_topic(
    tui, label, topic_title, *, direction_key=Keys.RIGHT, timeout=1.0, steps=24
):
    _scroll_help_to_text(tui, label)

    attempts = 0
    while True:
        if attempts >= steps:
            raise AssertionError(
                f"Could not follow help topic {topic_title!r}.\n{screen_text(tui)}"
            )
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        screen = tui.send_and_wait_for_condition(
            direction_key,
            lambda lines: lines if any(topic_title in line for line in lines) else False,
            timeout=timeout,
        )
        if screen:
            return "\n".join(screen)
        current = screen_text(tui)
        if topic_title not in current:
            tui.send_keystroke(Keys.LEFT, wait=0.05)
            current = screen_text(tui)
        if label not in current:
            _scroll_help_to_text(tui, label)
        attempts += 1


def _open_tagged_help_from_index(tui):
    return _follow_help_topic(tui, "Tagged", "Tagged", timeout=1.0)


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

        _wait_for_help(tui, "Directory Help")
        for sequence in (csi_down, csi_right, csi_left):
            retained = tui.send_and_wait_for_condition(
                sequence,
                lambda lines: lines
                if any("Directory Help" in line for line in lines)
                else False,
                timeout=1.5,
            )
            assert retained, screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_accepts_application_arrow_sequences(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_app_arrows")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        _wait_for_help(tui, "Directory Help")
        for key in (Keys.DOWN, Keys.RIGHT, Keys.LEFT):
            retained = tui.send_and_wait_for_condition(
                key,
                lambda lines: lines
                if any("Directory Help" in line for line in lines)
                else False,
                timeout=1.5,
            )
            assert retained, screen_text(tui)
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


def test_contextual_help_down_arrow_keeps_the_popup_open(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_inline_link")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        retained = tui.send_and_wait_for_condition(
            Keys.DOWN, lambda lines: lines if any("Directory Help" in line for line in lines) else False, timeout=1.0
        )
        assert retained, screen_text(tui)
    finally:
        tui.quit()

def test_contextual_help_down_arrow_skips_plain_rows_then_scrolls(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_generic_scroll")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")

        scrolled = _scroll_help_to_text(tui, "Z archive")
        assert "Z archive" in scrolled, scrolled
        assert "`Z archive`" not in scrolled, scrolled
    finally:
        tui.quit()

def test_contextual_help_down_arrow_scrolls_past_write_to_lower_commands(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_scroll_lower_commands")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        before = _wait_for_help(tui, "Directory Help")
        after = tui.send_and_wait_for_condition(Keys.DOWN, lambda lines: lines if any("Directory Help" in line for line in lines) else False, timeout=1.0)
        assert after, before
    finally:
        tui.quit()

def test_contextual_help_down_arrow_eventually_scrolls_visible_page(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_generic_wrap")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        for _ in range(4):
            retained = tui.send_and_wait_for_condition(Keys.DOWN, lambda lines: lines if any("Directory Help" in line for line in lines) else False, timeout=1.0)
            assert retained, screen_text(tui)
    finally:
        tui.quit()

def test_contextual_help_uses_the_generated_footer_on_narrow_terminals(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_wraps_long_rows")
    tui = _spawn_help_tui(root, dimensions=(36, 90))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "Directory Help")
        assert "Right/Enter follow" in help_screen, help_screen
    finally:
        tui.quit()

def test_help_index_uses_the_width_reserved_by_its_footer(tmp_path):
    root = _root_with_file(tmp_path, "help_index_footer_width")
    tui = _spawn_help_tui(root, dimensions=(36, 70))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        contents = _send_help_key_until_text(tui, "i", "Help Index")

        footer_line = next(line for line in contents.splitlines() if "Esc/Q quit" in line)
        assert "Right/Enter follow" in footer_line, footer_line
        assert "Index" not in footer_line, footer_line
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
        index = _send_help_key_until_text(tui, "i", "Help Index")
        assert "Navigation" in index, index
        navigation = _send_help_key_until_text(tui, "n", "Help Navigation")
        assert "Related help" not in navigation, navigation
        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Help Index" in line for line in lines) else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
    finally:
        tui.quit()


def test_tagged_help_opens_from_the_help_index(tmp_path):
    root = _root_with_file(tmp_path, "tagged_inline_help")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")
        tagged = _open_tagged_help_from_index(tui)
        assert "Tagged" in tagged, tagged
    finally:
        tui.quit()
def test_help_index_keeps_authored_links_above_the_popup_footer(tmp_path):
    root = _root_with_file(tmp_path, "related_help_list_fits")
    tui = _spawn_help_tui(root, dimensions=(43, 106))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        _send_help_key_until_text(tui, "i", "Help Index")
        index = _send_help_key_until_text(tui, "i", "Help Index")
        assert "Related help" not in index, index
        footer_line = next(line for line in index.splitlines() if "Esc/Q quit" in line)
        assert "Left back" in footer_line, footer_line
        assert "Right/Enter follow" in footer_line, footer_line
    finally:
        tui.quit()


def test_directory_help_exposes_generated_navigation_commands(tmp_path):
    root = _root_with_file(tmp_path, "directory_related_help")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        directory = _wait_for_help(tui, "Directory Help")
        assert "Index" in directory and "Navigation" in directory, directory
    finally:
        tui.quit()

def test_contextual_help_footer_is_present_after_opening(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_footer_gap")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "Directory Help")
        assert "Esc/Q quit" in help_screen, help_screen
    finally:
        tui.quit()

def test_contextual_help_footer_remains_present_after_page_navigation(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_single_footer_gap")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "Directory Help")
        paged = _send_help_key_until_text(tui, Keys.END, "Esc/Q quit")
        assert "Esc/Q quit" in paged, paged
    finally:
        tui.quit()

def test_contextual_help_up_arrow_reselects_visible_links_when_scrolling_back(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_up_reselects")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        assert tui.send_and_wait_for_condition(Keys.UP, lambda lines: lines if any("Directory Help" in line for line in lines) else False, timeout=1.0), screen_text(tui)
    finally:
        tui.quit()

def test_split_file_help_uses_the_split_file_context_and_generated_strip(tmp_path):
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
        footer_line = next(line for line in help_screen.splitlines() if "Esc/Q quit" in line)
        assert "Left back" in footer_line, footer_line
        assert "Right/Enter follow" in footer_line, footer_line
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

        _wait_for_help(tui, "Execute File Help")

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

        _wait_for_help(tui, "Search Tagged Help")

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

        _wait_for_help(tui, "Filter Help")

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

        _wait_for_help(tui, "Create Archive Help")

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Create archive", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_main_f1_help_tracks_directory_and_file_contexts(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_main_contexts")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        directory_help = _wait_for_help(tui, "Directory Help")
        assert "Index" in directory_help and "Navigation" in directory_help, directory_help
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "File Help")
    finally:
        tui.quit()

def test_showall_help_keeps_scope_details_and_returns(tmp_path):
    root = _root_with_file(tmp_path, "showall_global_help_navigation")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke("s", wait=0.4)

        _wait_for_help(tui, "Showall Help")

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
        _wait_for_help(tui, "Output Destination Help")
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Output to:", timeout=1.0), screen_text(tui)

        tui.send_keystroke("F", wait=0.2)
        assert tui.wait_for_content("Output file [Raw]", timeout=1.0), screen_text(tui)
        _wait_for_help(tui, "Output Destination Help")
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Output file [Raw]", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F3, wait=0.2)
        assert tui.wait_for_content("Frame separator", timeout=1.0), screen_text(tui)
        _wait_for_help(tui, "Output Separator Help")
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
        _wait_for_help(tui, "Directory Help")
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "File Help")
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
            else False,
            timeout=1.0,
        )
        assert contents_screen, screen_text(tui)
        contents_text = "\n".join(contents_screen)
        footer_line = next(
            line for line in contents_text.splitlines() if "Esc/Q quit" in line
        )
        assert "Right/Enter follow" in footer_line, footer_line
        assert "Navigation" in footer_line, footer_line
        navigation = tui.send_and_wait_for_condition(
            "n",
            lambda lines: lines
            if any("Help Navigation" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert navigation, screen_text(tui)

        tui.send_keystroke(Keys.LEFT, wait=0.05)
        returned = tui.wait_for_condition(
            lambda lines: lines
            if any("Help Index" in line for line in lines)
            else False,
            timeout=1.0,
            poll_interval=0.05,
        )
        assert returned, screen_text(tui)
        tui.send_keystroke(Keys.LEFT, wait=0.05)
        assert tui.wait_for_content("Directory Help", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC, wait=0.05)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_returns_to_the_link_origin(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_inline_link_history")
    tui = _spawn_help_tui(root, dimensions=(40, 118))

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        tagged = _follow_help_topic(tui, "tag and untag", "Tagged", timeout=1.0)
        assert "Tagged" in tagged, tagged

        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
    finally:
        tui.quit()

def test_contextual_help_inline_links_keep_full_generated_footer(tmp_path):
    root = _root_with_file(tmp_path, "help_inline_link_footer_contract")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")

        tagged_help = _follow_help_topic(tui, "tag and untag", "Tagged", timeout=1.0)
        assert "Tagged" in tagged_help, tagged_help
        footer_line = next(
            line for line in tagged_help.splitlines() if "Esc/Q quit" in line
        )
        assert "Left back" in footer_line, footer_line
        assert "Right/Enter follow" in footer_line, footer_line
        assert "Index" in footer_line, footer_line
        returned = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Directory Help" in line for line in lines) else False,
            timeout=1.0,
        )
        assert returned, screen_text(tui)
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
        assert "Right/Enter follow" in help_screen, help_screen
        footer_style = _visible_cell_style(tui, "follow")
        enter_style = _visible_cell_style(tui, "Right/Enter")
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
        _wait_for_help(tui, "Volume Help")
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F9)
        assert tui.wait_for_content("Applications", timeout=1.0), screen_text(tui)
        _wait_for_help(tui, "Applications Help")
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
        _wait_for_help(tui, "F2 Picker Help")
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
        assert "Right/Enter follow" in help_screen, help_screen
        assert _send_help_key_until_text(tui, Keys.END, "Esc/Q quit"), screen_text(tui)

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("inside_dir", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("inside.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Archive File Help")
        assert "Right/Enter follow" in help_screen, help_screen
        assert _send_help_key_until_text(tui, Keys.END, "Esc/Q quit"), screen_text(tui)
    finally:
        tui.quit()


def test_contextual_help_uses_page_keys_and_only_links_complex_commands(tmp_path):
    root = _root_with_file(tmp_path, "contextual_help_page_keys")
    tui = _spawn_help_tui(root)
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _wait_for_help(tui, "Directory Help")
        for key in (Keys.HOME, Keys.END, Keys.PGDN, Keys.PGUP):
            assert tui.send_and_wait_for_condition(key, lambda lines: lines if any("Directory Help" in line for line in lines) else False, timeout=1.0), screen_text(tui)
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


def test_contextual_help_dispatches_locale_authored_strip_keys(tmp_path):
    root = _root_with_file(tmp_path, "locale_authored_help_strip")
    cases = (
        (None, "Index", "I", "Help Index", "Navigation", "N", "Help Navigation"),
        (
            {"LC_ALL": "de_DE.UTF-8", "LANG": "de_DE.UTF-8", "LANGUAGE": "de"},
            "Inhalt",
            "H",
            "Hilfeindex",
            "Navigation",
            "W",
            "Hilfe-Navigation",
        ),
    )

    for (
        env_extra,
        index_label,
        index_key,
        index_title,
        navigation_label,
        navigation_key,
        navigation_title,
    ) in cases:
        for label, key, title in (
            (index_label, index_key, index_title),
            (navigation_label, navigation_key, navigation_title),
        ):
            tui = _spawn_help_tui(root, env_extra=env_extra)
            try:
                help_screen = _wait_for_help(tui, "Directory Help")
                assert label.casefold() in help_screen.casefold(), help_screen
                opened = tui.send_and_wait_for_condition(
                    key,
                    lambda lines: lines if any(title in line for line in lines) else False,
                    timeout=1.5,
                )
                assert opened, screen_text(tui)
            finally:
                tui.quit()
