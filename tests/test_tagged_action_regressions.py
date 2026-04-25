import time
from pathlib import Path

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _read_ctrl_file_ops_source():
    repo_root = Path(__file__).resolve().parents[1]
    return (repo_root / "src/ui/ctrl_file_ops.c").read_text(encoding="utf-8")


def _extract_function_block(source, signature):
    start = source.find(signature)
    assert start >= 0, f"Could not find function signature: {signature}"

    open_brace = source.find("{", start)
    assert open_brace >= 0, f"Could not find opening brace for: {signature}"

    depth = 0
    for idx in range(open_brace, len(source)):
        ch = source[idx]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source[start : idx + 1]

    raise AssertionError(f"Could not find closing brace for: {signature}")


def _find_line_with_text(tui, needle):
    for line in tui.get_screen_dump():
        if needle in line:
            return line
    return None


def _line_marks_file_as_tagged(line, filename):
    idx = line.find(filename)
    if idx <= 0:
        return False
    return "*" in line[:idx]


def _assert_file_tag_state(tui, filename, expected_tagged):
    line = _find_line_with_text(tui, filename)
    assert line is not None, (
        f"Could not find file row for {filename!r}.\nScreen:\n{_screen_text(tui)}"
    )
    is_tagged = _line_marks_file_as_tagged(line, filename)
    assert is_tagged == expected_tagged, (
        f"Unexpected tag state for {filename!r}. "
        f"Expected tagged={expected_tagged}, got {is_tagged}.\n"
        f"Row: {line}\nScreen:\n{_screen_text(tui)}"
    )


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
