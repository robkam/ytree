import time
import zipfile

from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source
from helpers_ui import assert_file_tag_state as _assert_file_tag_state
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _read_ctrl_file_ops_source():
    return read_repo_source("src/ui/ctrl_file_ops.c")


def test_invert_tags_i_and_upper_i_on_mixed_set(ytree_binary, tmp_path):
    work_dir = tmp_path / "tagged_invert_mixed"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")
    (work_dir / "gamma.txt").write_text("gamma", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(work_dir))
    time.sleep(0.5)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)

        tui.send_keystroke("t", wait=0.2)  # alpha.txt
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)  # gamma.txt

        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
        _assert_file_tag_state(tui, "gamma.txt", True)

        tui.send_keystroke("i", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", True)
        _assert_file_tag_state(tui, "gamma.txt", False)

        tui.send_keystroke("I", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
        _assert_file_tag_state(tui, "gamma.txt", True)
    finally:
        tui.quit()


def test_invert_tags_i_and_upper_i_in_directory_window(ytree_binary, tmp_path):
    work_dir = tmp_path / "dir_window_invert_tags"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(work_dir))
    time.sleep(0.5)

    try:
        footer = _footer_text(tui)
        assert "j tree" in footer, f"Expected dir-window footer before invert.\n{footer}"

        tui.send_keystroke("t", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)

        tui.send_keystroke("i", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("I", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)
    finally:
        tui.quit()


def test_invert_tags_i_and_upper_i_in_archive_directory_window(
    ytree_binary, tmp_path
):
    archive_path = tmp_path / "invert_archive.zip"
    with zipfile.ZipFile(archive_path, "w") as zf:
        zf.writestr("alpha.txt", "alpha")
        zf.writestr("beta.txt", "beta")

    tui = YtreeTUI(
        executable=ytree_binary, cwd=str(tmp_path), args=[str(archive_path)]
    )
    time.sleep(0.6)

    try:
        tui.send_keystroke("t", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)

        tui.send_keystroke("i", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", False)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("I", wait=0.25)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", True)
    finally:
        tui.quit()


def test_only_tagged_toggle_o_from_directory_window(ytree_binary, tmp_path):
    work_dir = tmp_path / "dir_window_only_tagged"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")
    (work_dir / "gamma.txt").write_text("gamma", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(work_dir))
    time.sleep(0.5)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke("t", wait=0.2)  # alpha
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)  # gamma
        tui.send_keystroke(Keys.ESC, wait=0.35)

        footer = _footer_text(tui)
        assert "j tree" in footer, f"Expected directory footer before tagged-only toggle.\n{footer}"

        tui.send_keystroke("o", wait=0.35)
        tagged_only_screen = _screen_text(tui)
        assert "alpha.txt" in tagged_only_screen, tagged_only_screen
        assert "gamma.txt" in tagged_only_screen, tagged_only_screen
        assert "beta.txt" not in tagged_only_screen, tagged_only_screen

        tui.send_keystroke("O", wait=0.35)
        full_screen = _screen_text(tui)
        assert "alpha.txt" in full_screen, full_screen
        assert "beta.txt" in full_screen, full_screen
        assert "gamma.txt" in full_screen, full_screen
    finally:
        tui.quit()


def test_handle_tag_file_action_delegates_file_op_hotspot():
    source = _read_ctrl_file_ops_source()
    handle_block = _extract_function_block(source, "BOOL handle_tag_file_action(")

    assert "HandleTaggedFileOpDispatchAction(" in source, (
        "Tagged file-op hotspot helper must exist so command flow is extracted "
        "out of handle_tag_file_action."
    )
    assert "HandleTaggedFileOpDispatchAction(" in handle_block, (
        "handle_tag_file_action must delegate file-op hotspot handling "
        "to the extracted helper."
    )
    assert "HandleTaggedSelectionDispatchAction(" in source, (
        "Tagged selection helper must exist so tagging state transitions "
        "are extracted out of handle_tag_file_action."
    )
    assert "HandleTaggedSelectionDispatchAction(" in handle_block, (
        "handle_tag_file_action must delegate tag selection state handling "
        "to the extracted helper."
    )
    assert "case ACTION_CMD_TAGGED_Y:" not in handle_block, (
        "Tagged copy command branch should be handled in extracted helper."
    )
    assert "case ACTION_CMD_TAGGED_M:" not in handle_block, (
        "Tagged move command branch should be handled in extracted helper."
    )
    assert "case ACTION_CMD_TAGGED_X:" not in handle_block, (
        "Tagged execute command branch should be handled in extracted helper."
    )
    assert "case ACTION_TAG:" not in handle_block, (
        "Single-file tag branch should be handled in extracted helper."
    )
    assert "case ACTION_UNTAG:" not in handle_block, (
        "Single-file untag branch should be handled in extracted helper."
    )


def test_tagged_copy_prompt_cancel_preserves_tagged_state(ytree_binary, tmp_path):
    work_dir = tmp_path / "tagged_copy_prompt"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(work_dir))
    time.sleep(0.5)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke("t", wait=0.2)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("\x03", wait=0.35)  # Ctrl+C (copy tagged)
        assert tui.wait_for_content("COPY: TAGGED FILES", timeout=1.0), _screen_text(tui)

        tui.send_keystroke(Keys.ESC, wait=0.2)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
    finally:
        tui.quit()


def test_tagged_move_prompt_cancel_preserves_tagged_state(ytree_binary, tmp_path):
    work_dir = tmp_path / "tagged_move_prompt"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(work_dir))
    time.sleep(0.5)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke("t", wait=0.2)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)

        tui.send_keystroke("\x0e", wait=0.35)  # Ctrl+N (move tagged)
        assert tui.wait_for_content("MOVE: TAGGED FILES", timeout=1.0), _screen_text(tui)

        tui.send_keystroke(Keys.ESC, wait=0.2)
        _assert_file_tag_state(tui, "alpha.txt", True)
        _assert_file_tag_state(tui, "beta.txt", False)
    finally:
        tui.quit()
