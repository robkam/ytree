from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _open_file_window(tui, filename):
    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda current_lines: current_lines
        if any(filename in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)


def _open_attribute_prompt(tui, activation_key):
    lines = tui.send_and_wait_for_condition(
        activation_key,
        lambda current_lines: current_lines
        if any("ATTRIBUTES:" in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)
    return lines
