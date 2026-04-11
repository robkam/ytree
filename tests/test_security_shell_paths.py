import shlex
import time

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
