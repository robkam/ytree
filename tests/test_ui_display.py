"""Stable display behaviour contracts."""
from pathlib import Path

from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _root(tmp_path):
    root = tmp_path / "display"
    root.mkdir()
    for name in ("alpha.txt", "beta.txt", "gamma.txt"):
        (root / name).write_text(name, encoding="utf-8")
    return root


def _spawn(root):
    return YtreeNovaTUI(executable=str(Path(__file__).parent.parent / "build" / "ytnova"), cwd=str(root))


def test_file_selection_remains_discoverable_after_navigation(tmp_path):
    tui = _spawn(_root(tmp_path))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5)
        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=1.5)
        assert tui.wait_for_content("beta.txt", timeout=1.5)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=1.5)
        assert tui.wait_for_content("gamma.txt", timeout=1.0)
    finally:
        tui.quit()


def test_file_window_round_trip_preserves_a_usable_directory_view(tmp_path):
    tui = _spawn(_root(tmp_path))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5)
        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=1.5)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5)
        assert tui.wait_for_content("alpha.txt", timeout=1.0)
    finally:
        tui.quit()


def test_rename_prompt_cancels_to_the_invoking_file_view(tmp_path):
    tui = _spawn(_root(tmp_path))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=1.5)
        assert tui.send_and_wait_for_screen_change("r", timeout=1.5)
        assert tui.wait_for_content("RENAME TO:", timeout=1.0)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5)
        assert tui.wait_for_content("alpha.txt", timeout=1.0)
    finally:
        tui.quit()
