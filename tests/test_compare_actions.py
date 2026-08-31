import time
import shlex
import os

from helpers_files import wait_for_file as _wait_for_file
from helpers_ui import (
    assert_file_tag_state as _assert_file_tag_state,
    footer_lines as _footer_lines,
    footer_text as _footer_text,
    screen_text as _screen_text,
)
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _has_border_glyphs(tui):
    screen = _screen_text(tui)
    for ch in ("│", "─", "┌", "┐", "└", "┘", "├", "┤", "┼"):
        if ch in screen:
            return True
    # Some PTY/terminfo combinations surface ACS line drawing as ASCII fallback.
    if "lqq" in screen and "\nx" in screen:
        return True
    return False


def _assert_no_footer_artifacts(tui):
    footer_lines = _footer_lines(tui)
    footer_text = "\n".join(footer_lines).lower()
    assert "pipe" in footer_text, f"Footer action line missing.\nFooter:\n{footer_text}"
    artifact_lines = [
        line for line in footer_lines if line.strip() and len(line.strip()) == 1 and line.strip().isalpha()
    ]
    assert not artifact_lines, f"Footer contained lone alphabetic artifact(s): {artifact_lines}\nFooter:\n{footer_text}"


def _configure_filediff_capture(tmp_dir, use_placeholders=False):
    log_path = tmp_dir / "filediff_args.log"
    helper_path = tmp_dir / ".capture_filediff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)

    filediff_cmd = str(helper_path)
    if use_placeholders:
        filediff_cmd = f"{filediff_cmd} %1 %2"

    (tmp_dir / ".ytnova").write_text(
        f"[GLOBAL]\nFILEDIFF={filediff_cmd}\n",
        encoding="utf-8",
    )
    return log_path


def _configure_dirdiff_capture(tmp_dir):
    log_path = tmp_dir / "dirdiff_args.log"
    helper_path = tmp_dir / ".capture_dirdiff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)

    (tmp_dir / ".ytnova").write_text(
        f"[GLOBAL]\nDIRDIFF={helper_path}\nTREEDIFF={helper_path}\n",
        encoding="utf-8",
    )
    return log_path


def _configure_dirdiff_only_capture(tmp_dir):
    log_path = tmp_dir / "dirdiff_args.log"
    helper_path = tmp_dir / ".capture_dirdiff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)

    (tmp_dir / ".ytnova").write_text(
        f"[GLOBAL]\nDIRDIFF={helper_path}\n",
        encoding="utf-8",
    )
    return log_path


def _run_file_compare(tui, target=None, wait=0.5):
    if target is None:
        tui.send_keystroke(Keys.ENTER, wait=wait)
    else:
        tui.send_keystroke(Keys.CTRL_U + target + Keys.ENTER, wait=wait)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue after helper exits


def _open_directory_compare_prompt(tui):
    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)


def _open_compare_prompt_help(tui):
    assert tui.send_and_wait_for_screen_change(Keys.F1, timeout=1.5), (
        "Compare-target help did not replace the active prompt.\n"
        f"Screen:\n{_screen_text(tui)}"
    )


def _cycle_directory_compare_scope(tui, count=1):
    for _ in range(count):
        tui.send_keystroke(Keys.F3, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)


def _cycle_directory_compare_basis(tui, count=1):
    for _ in range(count):
        tui.send_keystroke(Keys.F4, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)


def _cycle_directory_compare_tag(tui, count=1):
    for _ in range(count):
        tui.send_keystroke(Keys.F5, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)


def test_compare_footer_entries_by_view(ytnova_binary, tmp_path):
    d = tmp_path / "compare_footer"
    d.mkdir()
    (d / "a.txt").write_text("a", encoding="utf-8")
    (d / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), (
        "File compare action should be reachable via J."
    )
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.quit()


def test_j_opens_directory_compare_prompt(ytnova_binary, tmp_path):
    d = tmp_path / "compare_prompt_open"
    d.mkdir()
    (d / "child").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_compare_prompt_scope_cycle_preserves_target_prompt(ytnova_binary, tmp_path):
    d = tmp_path / "compare_prompt_scope_cycle"
    d.mkdir()
    (d / "left").mkdir()
    (d / "right").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)
    assert "directory | size+date | different" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.ESC, wait=0.2)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui)
    assert "logged tree" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.quit()


def test_compare_prompt_cycles_to_external_dirdiff_without_extra_prompts(
    ytnova_binary, tmp_path
):
    d = tmp_path / "compare_prompt_external_dir"
    d.mkdir()
    alpha = d / "alpha"
    beta = d / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "a.txt").write_text("a", encoding="utf-8")
    (beta / "b.txt").write_text("b", encoding="utf-8")
    log_path = _configure_dirdiff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # alpha

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=2)
    assert "external directory" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.CTRL_U + str(beta) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "DIRDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(alpha), str(beta)], (
        "DIRDIFF should receive source and target directory paths.\n"
        f"Args: {logged}"
    )
    assert not tui.wait_for_content("COMPARE TARGET [", timeout=0.4), (
        "External directory compare should launch directly from the compare target prompt."
    )

    tui.quit()


def test_compare_prompt_cycles_to_external_treediff_without_extra_prompts(
    ytnova_binary, tmp_path
):
    source_root = tmp_path / "compare_prompt_external_tree_source"
    target_root = tmp_path / "compare_prompt_external_tree_target"
    source_root.mkdir()
    target_root.mkdir()
    (source_root / "left.txt").write_text("left", encoding="utf-8")
    (target_root / "left.txt").write_text("right", encoding="utf-8")
    log_path = _configure_dirdiff_capture(source_root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(source_root))

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=3)
    assert "external tree" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.CTRL_U + str(target_root) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "TREEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source_root), str(target_root)], (
        "TREEDIFF should receive source and target logged-root paths.\n"
        f"Args: {logged}"
    )
    assert not tui.wait_for_content("COMPARE TARGET [", timeout=0.4), (
        "External tree compare should launch directly from the compare target prompt."
    )

    tui.quit()


def test_external_dirdiff_return_restores_full_ncurses_frame(ytnova_binary, tmp_path):
    d = tmp_path / "compare_prompt_external_redraw"
    d.mkdir()
    alpha = d / "alpha"
    beta = d / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "only_left.txt").write_text("left\n", encoding="utf-8")
    (beta / "only_right.txt").write_text("right\n", encoding="utf-8")
    (d / ".ytnova").write_text(
        "[GLOBAL]\nDIRDIFF=diff -ru\nTREEDIFF=diff -ru\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # alpha

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=2)
    assert "external directory" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.CTRL_U + str(beta) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert tui.wait_for_content("alpha", timeout=1.0), (
        "The source directory did not return after external compare.\n"
        f"Screen:\n{_screen_text(tui)}"
    )
    tui.quit()


def test_non_f8_compare_target_prompting_for_file_dir_tree(ytnova_binary, tmp_path):
    d = tmp_path / "compare_non_f8_prompt"
    d.mkdir()
    (d / "file1.txt").write_text("x", encoding="utf-8")
    (d / "dir_a").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    file_target_prompt = _screen_text(tui).lower()
    assert "f1 help" in file_target_prompt
    assert "[f2]" not in file_target_prompt
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui)
    assert "logged tree" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.quit()


def test_compare_target_prompt_reuses_shared_edit_navigation_behavior(ytnova_binary, tmp_path):
    d = tmp_path / "compare_prompt_history"
    d.mkdir()
    (d / "a.txt").write_text("a", encoding="utf-8")
    (d / "b.txt").write_text("b", encoding="utf-8")
    remembered_target = str(d / "remembered_target.txt")
    (d / "remembered_target.txt").write_text("remembered", encoding="utf-8")
    log_path = _configure_filediff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _run_file_compare(tui, remembered_target, wait=0.55)
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) == 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[1] == remembered_target, (
        f"FILEDIFF target mismatch.\nExpected: {remembered_target}\nActual: {logged[1]}"
    )

    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke("\x10", wait=0.3)  # C-p history-up
    assert tui.wait_for_content("Pin/unpin", timeout=1.0)
    assert "Delete" in _screen_text(tui), _screen_text(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()


def test_f8_file_compare_uses_inactive_panel_default_target(ytnova_binary, tmp_path):
    d = tmp_path / "compare_f8_file_default"
    d.mkdir()
    left_dir = d / "left"
    right_dir = d / "right"
    left_dir.mkdir()
    right_dir.mkdir()
    (left_dir / "left.txt").write_text("left", encoding="utf-8")
    right_file = right_dir / "right.txt"
    right_file.write_text("right", encoding="utf-8")
    log_path = _configure_filediff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.F8, wait=0.4)

    tui.send_keystroke(Keys.TAB, wait=0.3)
    if tui.wait_for_content("J compare", timeout=0.4):
        tui.send_keystroke(Keys.ESC, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke(Keys.TAB, wait=0.3)
    assert tui.wait_for_content("J compare", timeout=1.0)
    tui.send_keystroke("J", wait=0.3)

    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    assert tui.wait_for_content("right.txt", timeout=1.0)
    _run_file_compare(tui, wait=0.55)
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) == 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0].endswith("left.txt"), f"Unexpected source arg: {logged[0]}"
    assert logged[1] == str(right_file), f"Unexpected target arg: {logged[1]}"
    tui.quit()


def test_file_compare_placeholder_expansion_passes_source_and_target(ytnova_binary, tmp_path):
    d = tmp_path / "compare_placeholder_expansion"
    d.mkdir()
    source = d / "source.txt"
    target = d / "target.txt"
    source.write_text("left", encoding="utf-8")
    target.write_text("right", encoding="utf-8")
    log_path = _configure_filediff_capture(d, use_placeholders=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _run_file_compare(tui, str(target), wait=0.55)

    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source), str(target)], (
        "FILEDIFF %1/%2 placeholder expansion should pass source then target.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_log_then_cycle_back_preserves_file_selection_across_two_volumes(
    ytnova_binary, tmp_path
):
    root = tmp_path / "compare_two_volume_file_selection"
    root.mkdir()
    vol_a = root / "vol_a"
    vol_b = root / "vol_b"
    vol_a.mkdir()
    vol_b.mkdir()
    (vol_a / "a_0.txt").write_text("0\n", encoding="utf-8")
    (vol_a / "a_1.txt").write_text("1\n", encoding="utf-8")
    (vol_a / "a_2.txt").write_text("2\n", encoding="utf-8")
    (vol_a / "a_3.txt").write_text("3\n", encoding="utf-8")
    (vol_b / "b_0.txt").write_text("b0\n", encoding="utf-8")
    (vol_b / "b_1.txt").write_text("b1\n", encoding="utf-8")
    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(vol_a), str(vol_b)],
    )

    def active_volume_name():
        screen = _screen_text(tui)
        if "a_0.txt" in screen:
            return "vol_a"
        if "b_0.txt" in screen:
            return "vol_b"
        return None

    def cycle_to_volume(name):
        fixture_name = {"vol_a": "a_0.txt", "vol_b": "b_0.txt"}[name]
        for _ in range(8):
            if active_volume_name() == name:
                return
            if tui.send_and_wait_for_condition(
                ">",
                lambda lines: lines
                if any(fixture_name in line for line in lines)
                else False,
                timeout=2.0,
                poll_interval=0.05,
            ):
                return
        raise AssertionError(
            f"Failed to select the {name} fixture volume.\n{_screen_text(tui)}"
        )

    def ensure_file_compare_prompt_available():
        tui.send_keystroke("J", wait=0.25)
        if tui.wait_for_content("COMPARE TARGET:", timeout=0.8):
            tui.send_keystroke(Keys.ESC, wait=0.2)
            return
        if tui.wait_for_content("COMPARE TARGET [", timeout=0.8):
            tui.send_keystroke(Keys.ESC, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("J", wait=0.25)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), (
            "Expected file compare prompt after entering file mode."
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)

    cycle_to_volume("vol_a")

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    ensure_file_compare_prompt_available()
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _run_file_compare(tui, str(compare_target), wait=0.55)
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    initial_source = log_path.read_text(encoding="utf-8").splitlines()[0]

    tui.send_keystroke("L", wait=0.3)
    assert tui.wait_for_content("Log Path:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U + str(vol_b) + Keys.ENTER, wait=0.8)

    ensure_file_compare_prompt_available()

    cycle_to_volume("vol_a")

    ensure_file_compare_prompt_available()

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _run_file_compare(tui, str(compare_target), wait=0.55)
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2
    assert logged[0] == initial_source, (
        "File selection was not preserved after log+cycle with two volumes.\n"
        f"Expected source: {initial_source}\nActual source: {logged[0]}"
    )

    tui.quit()


def test_f8_directory_compare_uses_inactive_panel_default_target(ytnova_binary, tmp_path):
    d = tmp_path / "compare_f8_dir_default"
    d.mkdir()
    alpha = d / "alpha"
    beta = d / "beta"
    alpha.mkdir()
    beta.mkdir()
    diff_name = "diff.txt"
    match_name = "match.txt"
    alpha_diff = alpha / diff_name
    beta_diff = beta / diff_name
    alpha_match = alpha / match_name
    beta_match = beta / match_name
    alpha_diff.write_text("alpha-different-size\n", encoding="utf-8")
    beta_diff.write_text("b\n", encoding="utf-8")
    alpha_match.write_text("same-content\n", encoding="utf-8")
    beta_match.write_text("same-content\n", encoding="utf-8")
    (alpha / "alpha_only.txt").write_text("alpha-only\n", encoding="utf-8")
    (beta / "beta_only.txt").write_text("beta-only\n", encoding="utf-8")

    older_time = int(time.time()) - 3600
    newer_time = int(time.time())
    # Keep match.txt fully matching under size+date.
    os.utime(alpha_match, (older_time, older_time))
    os.utime(beta_match, (older_time, older_time))
    # Keep diff.txt mtime equal so size+date mismatch is driven by size.
    os.utime(alpha_diff, (newer_time, newer_time))
    os.utime(beta_diff, (newer_time, newer_time))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    tui.send_keystroke(Keys.DOWN, wait=0.2)  # alpha
    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # beta
    tui.send_keystroke(Keys.TAB, wait=0.3)

    _open_directory_compare_prompt(tui)
    assert tui.wait_for_content("beta", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("Tagged different: 1", timeout=1.0), _screen_text(tui)
    assert tui.wait_for_content("basis size+date", timeout=1.0), _screen_text(tui)
    assert not tui.wait_for_content("Directory compare complete.", timeout=0.3)

    # Active/source side (alpha) should have compare tags.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    _assert_no_footer_artifacts(tui)
    _assert_file_tag_state(tui, diff_name, True)
    _assert_file_tag_state(tui, match_name, False)

    tui.quit()


def test_f8_tree_compare_uses_inactive_panel_logged_root_default(ytnova_binary, tmp_path):
    main_root = tmp_path / "compare_f8_tree_main"
    other_root = tmp_path / "compare_f8_tree_other"
    main_root.mkdir()
    other_root.mkdir()
    (main_root / "main_dir").mkdir()
    (other_root / "main_dir").mkdir()
    (main_root / "main_dir" / "tree_diff.txt").write_text("left-tree", encoding="utf-8")
    (other_root / "main_dir" / "tree_diff.txt").write_text("right-tree", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(main_root))

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.3)
    tui.send_keystroke("L", wait=0.3)
    assert tui.wait_for_content("Log Path:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U, wait=0.2)
    tui.send_keystroke(str(other_root) + Keys.ENTER, wait=0.7)

    tui.send_keystroke(Keys.TAB, wait=0.4)
    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui)
    assert tui.wait_for_content(str(other_root.name), timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("Tagged different: 1", timeout=1.0), _screen_text(tui)
    assert tui.wait_for_content("basis size+date", timeout=1.0), _screen_text(tui)
    assert not tui.wait_for_content("Logged-tree compare complete.", timeout=0.3)

    # Tree compare reports differences but does not auto-apply file tags.
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # main_dir
    _assert_no_footer_artifacts(tui)
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    _assert_file_tag_state(tui, "tree_diff.txt", False)

    # Inactive/target side must not be tagged by compare.
    tui.send_keystroke(Keys.TAB, wait=0.35)
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # main_dir
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    assert tui.wait_for_content("tree_diff.txt", timeout=1.0), _screen_text(tui)
    tui.send_keystroke(Keys.F8, wait=0.4)
    _assert_file_tag_state(tui, "tree_diff.txt", False)

    tui.quit()


def test_tree_compare_logged_only_relative_path_and_skipped_unlogged_reporting(
    ytnova_binary, tmp_path
):
    source_root = tmp_path / "compare_tree_logged_only_source"
    target_root = tmp_path / "compare_tree_logged_only_target"
    source_root.mkdir()
    target_root.mkdir()
    (source_root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (target_root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (source_root / "top").mkdir()
    (target_root / "top").mkdir()
    (source_root / "top" / "deep").mkdir()
    (target_root / "top" / "deep").mkdir()

    source_visible = source_root / "top" / "visible_diff.txt"
    target_visible = target_root / "top" / "visible_diff.txt"
    source_visible.write_text("left", encoding="utf-8")
    target_visible.write_text("right-longer", encoding="utf-8")
    target_mtime = source_visible.stat().st_mtime + 120
    os.utime(target_visible, (target_mtime, target_mtime))

    # This file should be skipped because '+' subtree remains unlogged in source.
    (source_root / "top" / "deep" / "unlogged_diff.txt").write_text("source", encoding="utf-8")
    (target_root / "top" / "deep" / "unlogged_diff.txt").write_text("target-change", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(source_root))

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui)
    tui.send_keystroke(Keys.CTRL_U + str(target_root) + Keys.ENTER, wait=0.35)

    assert tui.wait_for_content("Tagged different: 1", timeout=1.0), _screen_text(tui)
    assert tui.wait_for_content("basis size+date", timeout=1.0), _screen_text(tui)
    assert tui.wait_for_content("skipped unlogged: source=", timeout=1.0), _screen_text(tui)
    assert not tui.wait_for_content("Logged-tree compare complete.", timeout=0.3)

    tui.send_keystroke(Keys.DOWN, wait=0.2)  # top
    _assert_no_footer_artifacts(tui)
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    _assert_file_tag_state(tui, "visible_diff.txt", False)
    tui.quit()


def test_compare_flow_cancel_is_safe_and_footer_remains_clean(ytnova_binary, tmp_path):
    d = tmp_path / "compare_cancel_safety"
    d.mkdir()
    (d / "x").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Directory compare complete.", timeout=0.5)
    assert not tui.wait_for_content("Logged-tree compare complete.", timeout=0.5)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui)
    assert "logged tree" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Logged-tree compare complete.", timeout=0.5)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_basis(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Directory compare complete.", timeout=0.5)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_tag(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Directory compare complete.", timeout=0.5)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=2)
    assert "external directory" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Directory compare complete.", timeout=0.5)

    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_file_compare_cancel_path_is_safe(ytnova_binary, tmp_path):
    d = tmp_path / "compare_file_cancel_safe"
    d.mkdir()
    (d / "alpha.txt").write_text("a", encoding="utf-8")
    log_path = _configure_filediff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    assert not log_path.exists(), "Cancelling file compare prompt should not launch FILEDIFF."
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_file_compare_rejects_same_file_target(ytnova_binary, tmp_path):
    d = tmp_path / "compare_same_file_reject"
    d.mkdir()
    (d / "same.txt").write_text("x", encoding="utf-8")
    log_path = _configure_filediff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.35)

    assert tui.wait_for_content("Can't compare: source and target", timeout=1.0), (
        "Selecting the source file as compare target should be rejected."
    )
    assert not log_path.exists(), "Same-file compare rejection should not run helper."

    tui.send_keystroke(Keys.ENTER, wait=0.2)  # Close message dialog
    tui.quit()


def test_compare_prompt_labels_and_result_text(ytnova_binary, tmp_path):
    d = tmp_path / "compare_prompt_labels"
    d.mkdir()
    (d / "alpha").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)
    screen_lower = _screen_text(tui).lower()
    assert "directory | size+date | different" in screen_lower
    target_prompt = _screen_text(tui).lower()
    assert "f1 help" in target_prompt
    assert "f2 browse" in target_prompt
    assert "f3 scope" in target_prompt
    assert "f4 basis" in target_prompt
    assert "f5 tag" in target_prompt
    assert "[f2]" not in target_prompt
    _cycle_directory_compare_scope(tui)
    assert "logged tree | size+date | different" in _screen_text(tui).lower()
    _cycle_directory_compare_basis(tui)
    assert "logged tree | size | different" in _screen_text(tui).lower()
    _cycle_directory_compare_tag(tui)
    assert "logged tree | size | match" in _screen_text(tui).lower()
    screen_lower = _screen_text(tui).lower()
    assert "fifferent" not in screen_lower
    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_compare_help_f1_open_close_and_prompt_restore(ytnova_binary, tmp_path):
    d = tmp_path / "compare_help_cycle"
    d.mkdir()
    (d / "alpha").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)

    _open_compare_prompt_help(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_split_filemode_toggle_truncates_footer_without_wrapping(ytnova_binary, tmp_path):
    d = tmp_path / "split_footer_clip_filemode"
    d.mkdir()
    (d / "a.txt").write_text("a", encoding="utf-8")
    (d / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    # Numeric FileInfo mode selection must clip footer text in-place, not wrap.
    tui.send_keystroke("2", wait=0.4)
    footer_lines = _footer_lines(tui)
    footer_text = "\n".join(footer_lines).lower()

    assert "commands" in footer_lines[1].lower(), (
        "Footer commands line was corrupted after file mode toggle.\n" + footer_text
    )
    assert "f1 help" in footer_lines[2].lower(), (
        "Footer nav/help line was corrupted after file mode toggle.\n" + footer_text
    )
    assert "jump" not in footer_lines[2].lower(), (
        "Footer text wrapped into nav/help line after file mode toggle.\n" + footer_text
    )
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_file_compare_target_help_is_file_specific_and_f2_browse_keeps_footer_clean(
    ytnova_binary, tmp_path
):
    d = tmp_path / "compare_file_target_help"
    d.mkdir()
    (d / "alpha.txt").write_text("a", encoding="utf-8")
    (d / "browse_here").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    _open_compare_prompt_help(tui)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.F2, wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()


def test_file_view_ctrl_k_remains_tagged_copy(ytnova_binary, tmp_path):
    d = tmp_path / "compare_ctrl_k"
    d.mkdir()
    (d / "f1.txt").write_text("x", encoding="utf-8")
    (d / "f2.txt").write_text("y", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("t", wait=0.2)       # tag current file
    tui.send_keystroke("\x0b", wait=0.35)   # C-k in file view

    screen = _screen_text(tui)
    assert "COPY: TAGGED FILES" in screen, f"C-k in file view should keep tagged-copy behavior:\n{screen}"

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()


def test_file_compare_j_flow_uses_current_file_source_and_prompt_behavior(
    ytnova_binary, tmp_path
):
    d = tmp_path / "compare_j_current_file_source"
    d.mkdir()
    source_a = d / "a_source.txt"
    source_b = d / "b_source.txt"
    target = d / "z_target.txt"
    source_a.write_text("a", encoding="utf-8")
    source_b.write_text("b", encoding="utf-8")
    target.write_text("target", encoding="utf-8")
    log_path = _configure_filediff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # b_source.txt
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    prompt_screen = _screen_text(tui).lower()
    assert "f1 help" in prompt_screen
    assert "[f2]" not in prompt_screen

    _run_file_compare(tui, str(target), wait=0.55)
    assert _wait_for_file(log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source_b), str(target)], (
        "J compare should use current file as source and entered target as destination.\n"
        f"Args: {logged}"
    )

    tui.quit()


def test_compare_external_tree_falls_back_to_dirdiff_when_treediff_unset(
    ytnova_binary, tmp_path
):
    source_root = tmp_path / "compare_external_tree_fallback_source"
    target_root = tmp_path / "compare_external_tree_fallback_target"
    source_root.mkdir()
    target_root.mkdir()
    (source_root / "left.txt").write_text("left", encoding="utf-8")
    (target_root / "left.txt").write_text("right", encoding="utf-8")
    log_path = _configure_dirdiff_only_capture(source_root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(source_root))

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=3)
    assert "external tree" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.CTRL_U + str(target_root) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "DIRDIFF fallback helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert logged == [str(source_root), str(target_root)], (
        "When TREEDIFF is unset, J->X->T should fall back to DIRDIFF.\n"
        f"Args: {logged}"
    )
    assert not tui.wait_for_content("COMPARE TARGET [", timeout=0.4), (
        "External tree compare fallback should launch directly from the compare target prompt."
    )

    tui.quit()


def test_external_compare_launch_does_not_modify_file_tag_state(ytnova_binary, tmp_path):
    d = tmp_path / "compare_external_no_tag_mutation"
    d.mkdir()
    alpha = d / "alpha"
    beta = d / "beta"
    alpha.mkdir()
    beta.mkdir()
    diff_name = "diff.txt"
    alpha_diff = alpha / diff_name
    beta_diff = beta / diff_name
    alpha_diff.write_text("left", encoding="utf-8")
    beta_diff.write_text("right", encoding="utf-8")
    log_path = _configure_dirdiff_capture(d)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # alpha

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_scope(tui, count=2)
    assert "external directory" in _screen_text(tui).lower()
    tui.send_keystroke(Keys.CTRL_U + str(beta) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue

    assert _wait_for_file(log_path, timeout=2.0), "DIRDIFF helper did not run."

    tui.send_keystroke(Keys.ENTER, wait=0.35)
    _assert_file_tag_state(tui, diff_name, False)
    tui.send_keystroke(Keys.ESC, wait=0.25)

    tui.send_keystroke(Keys.DOWN, wait=0.2)  # beta
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    _assert_file_tag_state(tui, diff_name, False)

    tui.quit()


def test_compare_help_close_with_f1_and_esc_returns_to_same_prompt(
    ytnova_binary, tmp_path
):
    d = tmp_path / "compare_help_f1_esc_roundtrip"
    d.mkdir()
    (d / "alpha").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))

    _open_directory_compare_prompt(tui)

    _open_compare_prompt_help(tui)
    assert tui.send_and_wait_for_screen_change(Keys.F1, timeout=1.5)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)

    _open_directory_compare_prompt(tui)
    _cycle_directory_compare_basis(tui)
    _open_compare_prompt_help(tui)
    assert tui.send_and_wait_for_screen_change(Keys.F1, timeout=1.5)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)
    tui.quit()
