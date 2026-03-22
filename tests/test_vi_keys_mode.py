import time

import pytest

from tui_harness import YtreeTUI
from ytree_keys import Keys


@pytest.fixture
def vi_mode_test_dir(tmp_path):
    root = tmp_path / "vi_mode_keys"
    root.mkdir()
    (root / ".ytree").write_text("[GLOBAL]\nVI_KEYS=1\n", encoding="utf-8")
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")
    (root / "c.txt").write_text("c", encoding="utf-8")
    return root


def test_vi_uppercase_u_untags_all(ytree_binary, vi_mode_test_dir):
    tui = YtreeTUI(executable=ytree_binary, cwd=str(vi_mode_test_dir))
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


def test_vi_uppercase_d_deletes_tagged(ytree_binary, vi_mode_test_dir):
    tui = YtreeTUI(executable=ytree_binary, cwd=str(vi_mode_test_dir))
    time.sleep(1.0)

    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.4)

    tui.send_keystroke("t")
    time.sleep(0.2)
    tui.send_keystroke("t")
    time.sleep(0.2)

    tui.send_keystroke("D")
    time.sleep(0.2)
    tui.send_keystroke("y")
    time.sleep(0.2)
    tui.send_keystroke("n")
    time.sleep(0.7)

    tui.quit()

    assert not (vi_mode_test_dir / "a.txt").exists()
    assert not (vi_mode_test_dir / "b.txt").exists()
    assert (vi_mode_test_dir / "c.txt").exists()
