import os
from helpers_ui import drive_action_until
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

def test_sort_help_missing_options(ytnova_binary, sandbox):
    """
    Verifies that the sort help menu contains all required options.
    Specifically, (N)ame, (S)ize, and o(W)ner should be present.
    """
    # Set ASAN options to log to a file
    asan_log = sandbox / "asan.log"
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = f"log_path={asan_log}"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(sandbox), env_extra=env)
    assert tui.wait_for_content("source", timeout=2.0)
    assert drive_action_until(
        tui,
        Keys.DOWN,
        lambda lines: lines if "source" in next(iter(lines), "") else False,
        max_actions=128,
    ), "Could not select the source fixture entry."
    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: lines if any("root_file.txt" in line for line in lines) else False,
        timeout=2.0,
    ), "Source fixture file list did not become available after Enter."
    assert tui.send_and_wait_for_condition(
        "S",
        lambda lines: lines if any("SORT by" in line for line in lines) else False,
        timeout=2.0,
    ), "Sort prompt did not become available after S."

    prompt = "\n".join(tui.get_screen_dump())
    missing = [option for option in ("Name", "Size", "OWner") if option not in prompt]
    assert not missing, f"Missing sort capabilities: {', '.join(missing)}"

    tui.quit()
