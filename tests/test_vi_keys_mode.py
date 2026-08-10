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
    time.sleep(1.0)

    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.4)

    tui.send_keystroke("\x14")  # Ctrl+T = tag all
    time.sleep(0.4)

    screen_before = "\n".join(tui.get_screen_dump())
    assert "* a.txt" in screen_before
    assert "* b.txt" in screen_before
    assert "* c.txt" in screen_before

    tui.send_keystroke("U")
    time.sleep(0.4)

    screen_after = "\n".join(tui.get_screen_dump())
    assert "* a.txt" not in screen_after
    assert "* b.txt" not in screen_after
    assert "* c.txt" not in screen_after

    tui.quit()


def test_vi_uppercase_d_deletes_tagged_after_single_confirmation(
    ytnova_binary, vi_mode_test_dir
):
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(vi_mode_test_dir))
    time.sleep(1.0)

    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.4)

    tui.send_keystroke("t")
    time.sleep(0.2)
    tui.send_keystroke("t")
    time.sleep(0.2)

    tui.send_keystroke("D")
    assert tui.wait_for_text("Delete 2 tagged files", timeout=1.5), "\n".join(
        tui.get_screen_dump()
    )
    prompt_screen = "\n".join(tui.get_screen_dump())
    assert "Delete 2 tagged files" in prompt_screen, prompt_screen
    assert "Ask for confirmation for each file" not in prompt_screen, prompt_screen

    raw_output = io.StringIO()
    tui.child.logfile_read = raw_output
    try:
        tui.send_keystroke("y")
        deleted = tui.wait_for_condition(
            lambda _lines: (
                not (vi_mode_test_dir / "a.txt").exists()
                and not (vi_mode_test_dir / "b.txt").exists()
            ),
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
