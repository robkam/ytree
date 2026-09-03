import zipfile

from helpers_ui import assert_file_tag_state as _assert_file_tag_state
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys



def test_invert_tags_i_and_upper_i_on_mixed_set(ytnova_binary, tmp_path):
    work_dir = tmp_path / "tagged_invert_mixed"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")
    (work_dir / "gamma.txt").write_text("gamma", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("alpha.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN + Keys.DOWN, timeout=2.0)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)

        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
        _assert_file_tag_state(tui, "gamma.txt", True)

        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", True)
        _assert_file_tag_state(tui, "gamma.txt", False)

        assert tui.send_and_wait_for_screen_change("I", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
        _assert_file_tag_state(tui, "gamma.txt", True)
    finally:
        tui.quit()


def test_invert_tags_i_and_upper_i_in_directory_window(ytnova_binary, tmp_path):
    work_dir = tmp_path / "dir_window_invert_tags"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(tui)
        footer = _footer_text(tui)

        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)

        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", False)

        assert tui.send_and_wait_for_screen_change("I", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)
    finally:
        tui.quit()


def test_invert_tags_i_and_upper_i_in_archive_directory_window(
    ytnova_binary, tmp_path
):
    archive_path = tmp_path / "invert_archive.zip"
    with zipfile.ZipFile(archive_path, "w") as zf:
        zf.writestr("alpha.txt", "alpha")
        zf.writestr("beta.txt", "beta")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(tmp_path), args=[str(archive_path)]
    )

    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)

        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", False)

        assert tui.send_and_wait_for_screen_change("I", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)
    finally:
        tui.quit()






def test_tagged_copy_prompt_cancel_preserves_tagged_state(ytnova_binary, tmp_path):
    work_dir = tmp_path / "tagged_copy_prompt"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("alpha.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("\x03", wait=0.35)  # Ctrl+C (copy tagged)
        assert tui.wait_for_content("COPY: TAGGED FILES", timeout=1.0), _screen_text(tui)

        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
    finally:
        tui.quit()


def test_tagged_move_prompt_cancel_preserves_tagged_state(ytnova_binary, tmp_path):
    work_dir = tmp_path / "tagged_move_prompt"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("alpha.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("\x0e", wait=0.35)  # Ctrl+N (move tagged)
        assert tui.wait_for_content("MOVE: TAGGED FILES", timeout=1.0), _screen_text(tui)

        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
    finally:
        tui.quit()





