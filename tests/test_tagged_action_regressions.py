import time
import zipfile

from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source
from helpers_ui import assert_file_tag_state as _assert_file_tag_state
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _read_ctrl_file_ops_source():
    return read_repo_source("src/ui/ctrl_file_ops.c")


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






def test_tagged_execute_uses_the_tagged_file_directory_as_its_working_directory():
    source = read_repo_source("src/cmd/execute.c")
    start = source.index("int ExecuteCommand(")
    body = source[start:]

    assert "GetPath(fe_ptr->dir_entry, path)" in body
    assert "fchdir(start_dir_fd)" in body


def test_ctrl_key_dispatch_exposes_only_supported_tagged_operations():
    source = read_repo_source("src/ui/key_engine.c")
    start = source.index("YtreeNovaAction GetKeyAction(")
    end = source.index("\nint WGetch(", start)
    key_action = source[start:end]

    assert "case 0x1C:" not in key_action
    assert "case 0x17:" not in key_action
    assert (
        "case 0x1A:\n    return AppStateValidatedKeyAction(ACTION_CMD_I);"
        in key_action
    )


def test_tagged_attribute_prompt_uses_one_date_action_hint():
    source = read_repo_source("src/ui/attr_actions.c")
    start = source.index("static const UICommandStripCommand attribute_commands_tagged[]")
    end = source.index("static const UICommandStripCommand date_change_hint_commands[]", start)
    tagged_commands = source[start:end]

    assert '"tagged date"' not in tagged_commands
    assert tagged_commands.count('"Date"') == 1
