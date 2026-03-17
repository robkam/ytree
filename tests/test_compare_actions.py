import time

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    lines = tui.get_screen_dump()
    return "\n".join(lines[-3:]).lower()


def test_compare_footer_entries_by_view(ytree_binary, tmp_path):
    d = tmp_path / "compare_footer"
    d.mkdir()
    (d / "a.txt").write_text("a", encoding="utf-8")
    (d / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)

    dir_footer = _footer_text(tui)
    assert "brief compare delete" in dir_footer, f"Directory footer missing C compare entry:\n{dir_footer}"
    assert "tree compare" not in dir_footer, f"Directory footer should not show top-level tree compare:\n{dir_footer}"

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    file_footer = _footer_text(tui)
    assert "hex j compare log" in file_footer, f"File footer missing J compare entry:\n{file_footer}"

    tui.quit()


def test_directory_compare_key_and_no_direct_ctrl_k_tree_compare(ytree_binary, tmp_path):
    d = tmp_path / "compare_dir_keys"
    d.mkdir()
    (d / "x.txt").write_text("x", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)

    tui.send_keystroke("C", wait=0.25)
    assert tui.wait_for_content("Directory compare not implemented yet.", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.2)

    tui.send_keystroke("c", wait=0.25)
    assert tui.wait_for_content("Directory compare not implemented yet.", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.2)

    tui.send_keystroke("\x0b", wait=0.25)  # Ctrl-K should no longer trigger tree-compare directly
    assert not tui.wait_for_content("Tree compare not implemented yet.", timeout=0.5)

    tui.quit()


def test_file_compare_j_and_J_behavior(ytree_binary, tmp_path):
    d = tmp_path / "compare_file_keys"
    d.mkdir()
    (d / "f1.txt").write_text("x", encoding="utf-8")
    (d / "f2.txt").write_text("y", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("File compare not implemented yet.", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.2)

    tui.send_keystroke("j", wait=0.25)
    assert tui.wait_for_content("File compare not implemented yet.", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.2)

    tui.quit()


def test_file_view_ctrl_k_remains_tagged_copy(ytree_binary, tmp_path):
    d = tmp_path / "compare_ctrl_k"
    d.mkdir()
    (d / "f1.txt").write_text("x", encoding="utf-8")
    (d / "f2.txt").write_text("y", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("t", wait=0.2)       # tag current file
    tui.send_keystroke("\x0b", wait=0.35)   # Ctrl-K in file view

    screen = "\n".join(tui.get_screen_dump())
    assert "COPY: TAGGED FILES" in screen, f"Ctrl-K in file view should keep tagged-copy behavior:\n{screen}"

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()
