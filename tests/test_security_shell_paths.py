import shlex
import time
from pathlib import Path

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _wait_for_file(path, timeout=2.0):
    end = time.time() + timeout
    while time.time() < end:
        if path.exists():
            return True
        time.sleep(0.05)
    return False


def _configure_capture_helper(tmp_dir, log_name):
    log_path = tmp_dir / log_name
    helper_path = tmp_dir / f".capture_{log_name}.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)
    return helper_path, log_path


def _write_global_profile(tmp_dir, entries):
    body = ["[GLOBAL]"]
    for key, value in entries:
        body.append(f"{key}={value}")
    (tmp_dir / ".ytree").write_text("\n".join(body) + "\n", encoding="utf-8")


def _read_ctrl_file_ops_source():
    repo_root = Path(__file__).resolve().parents[1]
    return (repo_root / "src/ui/ctrl_file_ops.c").read_text(encoding="utf-8")


def _extract_case_block(source, case_label, next_case_label):
    start = source.find(case_label)
    assert start >= 0, f"Could not find case label: {case_label}"
    end = source.find(next_case_label, start)
    assert end >= 0, f"Could not find next case label: {next_case_label}"
    return source[start:end]


def test_tagged_command_long_uses_command_line_length_contract():
    src = _read_ctrl_file_ops_source()
    block = _extract_case_block(
        src, "case ACTION_CMD_TAGGED_X:", "case ACTION_TOGGLE_TAGGED_MODE:"
    )

    assert "malloc(COLS + 1)" not in block, (
        "Tagged execute command buffer must not be sized by COLS for long input."
    )
    assert "malloc(COMMAND_LINE_LENGTH + 1)" in block, (
        "Tagged execute command buffer must use COMMAND_LINE_LENGTH + 1."
    )


def test_tagged_search_long_uses_command_line_length_contract():
    src = _read_ctrl_file_ops_source()
    block = _extract_case_block(
        src, "case ACTION_CMD_TAGGED_S:", "case ACTION_CMD_TAGGED_X:"
    )

    assert "malloc(COLS + 1)" not in block, (
        "Tagged search command buffer must not be sized by COLS for long input."
    )
    assert "malloc(COMMAND_LINE_LENGTH + 1)" in block, (
        "Tagged search command buffer must use COMMAND_LINE_LENGTH + 1."
    )
    assert "COMMAND_LINE_LENGTH + sizeof(\" > /dev/null 2>&1\")" in block, (
        "Tagged silent command must size from COMMAND_LINE_LENGTH contract."
    )


def test_tagged_search_normalization_uses_command_line_length_contract():
    src = _read_ctrl_file_ops_source()
    block = _extract_case_block(
        src, "case ACTION_CMD_TAGGED_S:", "case ACTION_CMD_TAGGED_X:"
    )

    assert "NormalizeQuotedExecPlaceholders(command_line, (size_t)COLS + 1U);" not in block, (
        "Tagged search normalization must not be bounded by COLS."
    )
    assert "NormalizeQuotedExecPlaceholders(" in block, (
        "Tagged search path must normalize quoted placeholders."
    )
    assert "(size_t)COMMAND_LINE_LENGTH + 1U" in block, (
        "Tagged search normalization must use COMMAND_LINE_LENGTH + 1."
    )


def test_tagged_execute_normalization_uses_command_line_length_contract():
    src = _read_ctrl_file_ops_source()
    block = _extract_case_block(
        src, "case ACTION_CMD_TAGGED_X:", "case ACTION_TOGGLE_TAGGED_MODE:"
    )

    assert "NormalizeQuotedExecPlaceholders(command_line, (size_t)COLS + 1U);" not in block, (
        "Tagged execute normalization must not be bounded by COLS."
    )
    assert "NormalizeQuotedExecPlaceholders(" in block, (
        "Tagged execute path must normalize quoted placeholders."
    )
    assert "(size_t)COMMAND_LINE_LENGTH + 1U" in block, (
        "Tagged execute normalization must use COMMAND_LINE_LENGTH + 1."
    )


def test_compare_placeholder_expansion_preserves_metacharacter_paths(
    ytree_binary, tmp_path
):
    source_root = tmp_path / "compare_shell_literal_source"
    target_root = tmp_path / "compare_shell_literal_target"
    source_root.mkdir()
    target_root.mkdir()

    source_name = "source ; and& back\\slash 'quote with space.txt"
    target_name = "target ; and& back\\slash 'quote with space.txt"
    source_path = source_root / source_name
    target_path = target_root / target_name
    source_path.write_text("left", encoding="utf-8")
    target_path.write_text("right", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(source_root, "filediff_args.log")
    _write_global_profile(source_root, [("FILEDIFF", f"{helper_path} %1 %2")])

    tui = YtreeTUI(executable=ytree_binary, cwd=str(source_root))
    time.sleep(0.6)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # tree -> file view
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U + str(target_path) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source_path), str(target_path)], (
        "Compare placeholder expansion must pass literal source/target paths without "
        "shell splitting or interpretation.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_view_launch_passes_metacharacter_path_as_single_literal_argument(
    ytree_binary, tmp_path
):
    root = tmp_path / "view_shell_literal_path"
    root.mkdir()

    filename = "view ; and& back\\slash 'quote with space.txt"
    file_path = root / filename
    file_path.write_text("payload", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "view_args.log")
    _write_global_profile(root, [("PAGER", str(helper_path))])

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # tree -> file view
    tui.send_keystroke("v", wait=0.55)

    assert _wait_for_file(log_path, timeout=2.0), "PAGER helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(file_path)], (
        "View command must pass the selected path as one literal argv entry.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_execute_command_placeholder_preserves_metacharacter_filename_literal(
    ytree_binary, tmp_path
):
    root = tmp_path / "execute_shell_literal_path"
    root.mkdir()

    filename = "exec ; and& back\\slash 'quote with space.sh"
    file_path = root / filename
    file_path.write_text("echo payload\n", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "exec_args.log")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # tree -> file view
    tui.send_keystroke("x", wait=0.35)
    assert tui.wait_for_content("COMMAND:", timeout=1.0)
    tui.send_keystroke(
        Keys.CTRL_U + str(helper_path) + " {}" + Keys.ENTER,
        wait=0.55,
    )

    assert _wait_for_file(log_path, timeout=2.0), "Execute helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [filename], (
        "Execute {} placeholder expansion must preserve filename as a single "
        "literal argument.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_execute_placeholder_in_user_quotes_does_not_enable_shell_injection(
    ytree_binary, tmp_path
):
    root = tmp_path / "execute_shell_quote_injection_guard"
    root.mkdir()

    marker_name = "task71_injected_marker"
    marker_path = root / marker_name
    filename = f"exec ; touch {marker_name}"
    file_path = root / filename
    file_path.write_text("echo payload\n", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "exec_quoted_placeholder.log")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # tree -> file view
    tui.send_keystroke("x", wait=0.35)
    assert tui.wait_for_content("COMMAND:", timeout=1.0)
    tui.send_keystroke(
        Keys.CTRL_U + str(helper_path) + " '{}'" + Keys.ENTER,
        wait=0.55,
    )

    assert _wait_for_file(log_path, timeout=2.0), "Execute helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [filename], (
        "Quoted {} placeholder expansion must still pass the filename as one "
        "literal argument.\n"
        f"Args: {logged}"
    )
    assert not marker_path.exists(), (
        "Shell metacharacters from filename must not execute unintended commands "
        "when {} is used inside user quotes."
    )

    tui.quit()
