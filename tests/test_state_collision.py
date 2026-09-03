from helpers_ui import drive_action_until
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

def test_state_collision_cursor_pos(dual_panel_sandbox, ytnova_binary):
    """
    Demonstrates state collision for cursor_pos/current_dir_entry in split screen.
    If state is global, moving the cursor in one panel will affect the other.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(dual_panel_sandbox))
    assert tui.wait_for_content("right_dir", timeout=2.0)
    assert drive_action_until(
        tui,
        Keys.DOWN,
        lambda lines: lines if "right_dir" in next(iter(lines), "") else False,
        max_actions=128,
    ), "Could not select the right_dir fixture entry."

    assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)

    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: lines
        if any(line.startswith("Path:") and "right_dir" in line for line in lines)
        else False,
        timeout=2.0,
    )
    assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)

    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda screen: screen
        if any(line.startswith("Path:") and "right_dir" in line for line in screen)
        and any(" 0 " in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "Left panel selection no longer opens the right_dir fixture files."
    tui.quit()
