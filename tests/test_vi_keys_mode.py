import io
import time

import pytest

from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


@pytest.fixture
def vi_mode_test_dir(tmp_path):
    root = tmp_path / "vi_mode_keys"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nVI_KEYS=1\n", encoding="utf-8")
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")
    (root / "c.txt").write_text("c", encoding="utf-8")
    return root


def test_vi_uppercase_u_untags_all(ytnova_binary, vi_mode_test_dir):
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(vi_mode_test_dir))
    try:
        assert tui.wait_for_text("a.txt", timeout=2.0), "\n".join(tui.get_screen_dump())
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current: current
            if any("a.txt" in line for line in current)
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())

        lines = tui.send_and_wait_for_condition(
            "\x14",
            lambda current: current
            if all(f"* {name}" in "\n".join(current) for name in ("a.txt", "b.txt", "c.txt"))
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())

        lines = tui.send_and_wait_for_condition(
            "U",
            lambda current: current
            if all(f"* {name}" not in "\n".join(current) for name in ("a.txt", "b.txt", "c.txt"))
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())
    finally:
        tui.quit()


def test_vi_uppercase_d_deletes_tagged_after_single_confirmation(
    ytnova_binary, vi_mode_test_dir
):
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(vi_mode_test_dir))
    try:
        assert tui.wait_for_text("a.txt", timeout=2.0), "\n".join(tui.get_screen_dump())
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current: current
            if any("a.txt" in line for line in current)
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        lines = tui.send_and_wait_for_condition(
            "t",
            lambda current: current
            if "* a.txt" in "\n".join(current) and "* b.txt" in "\n".join(current)
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())

        lines = tui.send_and_wait_for_condition(
            "D",
            lambda current: current
            if any("Delete 2 tagged files" in line for line in current)
            else False,
            timeout=2.0,
        )
        assert lines, "\n".join(tui.get_screen_dump())
        prompt_screen = "\n".join(lines)
        assert "Ask for confirmation for each file" not in prompt_screen, prompt_screen

        raw_output = io.StringIO()
        tui.child.logfile_read = raw_output
        tui.send_keystroke("y")
        deleted = tui.wait_for_condition(
            lambda _lines: not (vi_mode_test_dir / "a.txt").exists()
            and not (vi_mode_test_dir / "b.txt").exists(),
            timeout=2.0,
        )
        assert deleted, "\n".join(tui.get_screen_dump())
        assert "Ask for confirmation for each file" not in raw_output.getvalue()
    finally:
        tui.child.logfile_read = None
        tui.quit()

    assert not (vi_mode_test_dir / "a.txt").exists()
    assert not (vi_mode_test_dir / "b.txt").exists()
    assert (vi_mode_test_dir / "c.txt").exists()
