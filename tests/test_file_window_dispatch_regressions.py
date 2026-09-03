
from helpers_stats import current_file_from_stats
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _current_file_from_stats(tui):
    return current_file_from_stats(tui.get_screen_dump())


def test_navigation_dispatch_updates_current_file_stats(ytnova_binary, tmp_path):
    root = tmp_path / "dispatch_navigation_stats"
    root.mkdir()
    (root / "aa_one.txt").write_text("one\n", encoding="utf-8")
    (root / "bb_two.txt").write_text("two\n", encoding="utf-8")
    (root / "cc_three.txt").write_text("three\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        first_file = _current_file_from_stats(tui)
        assert first_file == "aa_one.txt", _screen_text(tui)

        tui.send_keystroke(Keys.DOWN, wait=0.25)
        second_file = _current_file_from_stats(tui)
        assert second_file == "bb_two.txt", _screen_text(tui)

        tui.send_keystroke(Keys.UP, wait=0.25)
        up_file = _current_file_from_stats(tui)
        assert up_file == "aa_one.txt", _screen_text(tui)
    finally:
        tui.quit()


def test_make_file_prompt_dispatch_creates_file(ytnova_binary, tmp_path):
    root = tmp_path / "dispatch_make_file_prompt"
    root.mkdir()
    (root / "seed.txt").write_text("seed\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.MAKE_FILE, wait=0.25)
        assert tui.wait_for_content("MAKE FILE:", timeout=1.0), _screen_text(tui)

        created = root / "created_from_prompt.txt"
        tui.send_keystroke("created_from_prompt.txt" + Keys.ENTER, wait=0)
        assert tui.wait_for_condition(
            lambda _lines: created.exists(),
            timeout=2.0,
            description="created file",
        )
    finally:
        tui.quit()


def test_split_and_tab_dispatch_keeps_file_mode_footer(ytnova_binary, tmp_path):
    root = tmp_path / "dispatch_split_switch_footer"
    root.mkdir()
    (root / "left.txt").write_text("left\n", encoding="utf-8")
    (root / "right.txt").write_text("right\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)

        tui.send_keystroke(Keys.F8, wait=0.45)
        tui.send_keystroke(Keys.TAB, wait=0.45)

        footer = _footer_text(tui)
    finally:
        tui.quit()

