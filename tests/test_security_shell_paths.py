import shlex
import time

from helpers_files import wait_for_file as _wait_for_file
from helpers_ui import screen_text
from helpers_source import read_repo_source
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


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
    (tmp_dir / ".ytnova").write_text("\n".join(body) + "\n", encoding="utf-8")


def _send_and_wait_for_transition(tui, keys, timeout=2.0):
    lines = tui.send_and_wait_for_screen_change(keys, timeout=timeout)
    assert lines, screen_text(tui)
    return lines


def _read_ctrl_file_ops_source():
    return read_repo_source("src/ui/ctrl_file_ops.c")


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
    ytnova_binary, tmp_path
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

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(source_root))
    assert tui.wait_for_content(source_name, timeout=2.0), screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # tree -> file view
    _send_and_wait_for_transition(tui, "J")
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _send_and_wait_for_transition(tui, Keys.CTRL_U + str(target_path) + Keys.ENTER)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # HitReturnToContinue

    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source_path), str(target_path)], (
        "Compare placeholder expansion must pass literal source/target paths without "
        "shell splitting or interpretation.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_view_launch_passes_metacharacter_path_as_single_literal_argument(
    ytnova_binary, tmp_path
):
    root = tmp_path / "view_shell_literal_path"
    root.mkdir()

    filename = "view ; and& back\\slash 'quote with space.txt"
    file_path = root / filename
    file_path.write_text("payload", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "view_args.log")
    _write_global_profile(root, [("PAGER", str(helper_path))])

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content(filename, timeout=2.0), screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # tree -> file view
    _send_and_wait_for_transition(tui, "v")

    assert _wait_for_file(tui, log_path, timeout=2.0), "PAGER helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(file_path)], (
        "View command must pass the selected path as one literal argv entry.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_execute_command_placeholder_preserves_metacharacter_filename_literal(
    ytnova_binary, tmp_path
):
    root = tmp_path / "execute_shell_literal_path"
    root.mkdir()

    filename = "exec ; and& back\\slash 'quote with space.sh"
    file_path = root / filename
    file_path.write_text("echo payload\n", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "exec_args.log")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content(filename, timeout=2.0), screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # tree -> file view
    _send_and_wait_for_transition(tui, "x")
    assert tui.wait_for_content(
        "COMMAND ({} inserts selected path):", timeout=1.0
    )
    tui.send_keystroke(
        "\x05" + Keys.CTRL_U + str(helper_path) + " {}" + Keys.ENTER,
        wait=0.55,
    )

    assert _wait_for_file(tui, log_path, timeout=2.0), "Execute helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [filename], (
        "Execute {} placeholder expansion must preserve filename as a single "
        "literal argument.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_execute_placeholder_in_user_quotes_does_not_enable_shell_injection(
    ytnova_binary, tmp_path
):
    root = tmp_path / "execute_shell_quote_injection_guard"
    root.mkdir()

    marker_name = "task71_injected_marker"
    marker_path = root / marker_name
    filename = f"exec ; touch {marker_name}"
    file_path = root / filename
    file_path.write_text("echo payload\n", encoding="utf-8")

    helper_path, log_path = _configure_capture_helper(root, "exec_quoted_placeholder.log")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content(filename, timeout=2.0), screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # tree -> file view
    _send_and_wait_for_transition(tui, "x")
    assert tui.wait_for_content(
        "COMMAND ({} inserts selected path):", timeout=1.0
    )
    tui.send_keystroke(
        "\x05" + Keys.CTRL_U + str(helper_path) + " '{}'" + Keys.ENTER,
        wait=0.55,
    )

    assert _wait_for_file(tui, log_path, timeout=2.0), "Execute helper did not run."
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


def test_file_execute_prefill_omits_executable_path(
    ytnova_binary, tmp_path
):
    root = tmp_path / "execute_default_prefill_prompt"
    root.mkdir()

    script_path = root / "blob.sh"
    script_path.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    script_path.chmod(0o755)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content(script_path.name, timeout=2.0), screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # tree -> file view
    _send_and_wait_for_transition(tui, "x")

    screen = screen_text(tui)
    assert "COMMAND ({} inserts selected path):  {}" in screen, screen
    assert "COMMAND: ./'blob.sh'" not in screen, screen

    tui.quit()


def test_query_system_call_reinitializes_clock_and_dialog_refresh_after_continue():
    src = read_repo_source("src/ui/interactions.c")
    start = src.index("int QuerySystemCall(")
    end = src.index("\nint UI_ReadFilter(", start)
    body = src[start:end]

    assert "HitReturnToContinue();" in body, (
        "QuerySystemCall should keep the raw-terminal continue prompt after the "
        "external command returns."
    )
    assert "ctx->hook_init_clock(ctx);" in body, (
        "QuerySystemCall must restore the clock/curses state immediately after "
        "the continue prompt returns."
    )
    assert "UI_Dialog_RefreshAll(ctx);" in body, (
        "QuerySystemCall must restage the full window stack after restoring the "
        "clock so footer, borders, path, and stats redraw immediately."
    )
    assert "ClockHandler(ctx, 0);" in body, (
        "QuerySystemCall must repaint the live clock/calendar after restoring "
        "the dialog stack."
    )

    continue_idx = body.index("HitReturnToContinue();")
    clock_idx = body.index("ctx->hook_init_clock(ctx);")
    dialog_idx = body.index("UI_Dialog_RefreshAll(ctx);")
    clock_draw_idx = body.index("ClockHandler(ctx, 0);")
    doupdate_idx = body.index("doupdate();")

    assert continue_idx < clock_idx < dialog_idx < clock_draw_idx < doupdate_idx, (
        "QuerySystemCall must reinitialize curses, refresh the dialog stack, "
        "redraw the clock, and only then flush the redraw after the continue "
        "prompt."
    )


def test_runtime_launch_debt_surfaces_use_shared_runtime_launch_helpers():
    expected_helpers = {
        "src/cmd/system.c": [
            "RuntimeLaunchExecShellChild(",
            "RuntimeLaunchRunShell(",
            "RuntimeLaunchWait(",
        ],
        "src/cmd/pipe.c": [
            "RuntimeLaunchStartArgvWriter(",
            "RuntimeLaunchCloseWriter(",
        ],
        "src/cmd/print_ops.c": [
            "RuntimeLaunchStartShellWriter(",
            "RuntimeLaunchCloseWriter(",
        ],
        "src/ui/ctrl_file_ops.c": [
            "RuntimeLaunchStartShellWriter(",
            "RuntimeLaunchCloseWriter(",
        ],
        "src/ui/fileinfo_git.c": ["RuntimeLaunchCaptureShellOutput("],
        "src/ui/render_file.c": ["RuntimeLaunchCaptureShellOutput("],
        "src/core/quit.c": ["RuntimeLaunchRunShell("],
    }

    for rel_path, snippets in expected_helpers.items():
        src = read_repo_source(rel_path)
        assert "system(" not in src, f"{rel_path} must not use system()."
        assert "popen(" not in src, f"{rel_path} must not use popen()."
        for snippet in snippets:
            assert snippet in src, f"{rel_path} must route through {snippet}."


def test_shared_runtime_launch_module_owns_execvp_and_waitpid():
    src = read_repo_source("src/cmd/runtime_launch.c")

    assert "execvp(" in src, "Shared runtime launch module must own execvp()."
    assert "waitpid(" in src, "Shared runtime launch module must own waitpid()."
    assert "system(" not in src, "Shared runtime launch module must not regress to system()."
    assert "popen(" not in src, "Shared runtime launch module must not regress to popen()."
