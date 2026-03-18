import time

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _footer_lines(tui):
    return tui.get_screen_dump()[-3:]


def _footer_text(tui):
    return "\n".join(_footer_lines(tui)).lower()


def _assert_no_footer_artifacts(tui):
    footer_lines = _footer_lines(tui)
    footer_text = "\n".join(footer_lines).lower()
    assert "commands" in footer_text, f"COMMANDS footer line missing.\nFooter:\n{footer_text}"
    artifact_lines = [
        line for line in footer_lines if line.strip() and len(line.strip()) == 1 and line.strip().isalpha()
    ]
    assert not artifact_lines, f"Footer contained lone alphabetic artifact(s): {artifact_lines}\nFooter:\n{footer_text}"


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


def test_c_opens_compare_submenu(ytree_binary, tmp_path):
    d = tmp_path / "compare_submenu_open"
    d.mkdir()
    (d / "child").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)

    tui.send_keystroke("C", wait=0.25)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_compare_submenu_d_and_t_choices_prompt_expected_targets(ytree_binary, tmp_path):
    d = tmp_path / "compare_submenu_choices"
    d.mkdir()
    (d / "left").mkdir()
    (d / "right").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.6)

    tui.send_keystroke("C", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)
    tui.send_keystroke("D", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke("C", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)
    tui.send_keystroke("T", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.quit()


def test_non_f8_compare_target_prompting_for_file_dir_tree(ytree_binary, tmp_path):
    d = tmp_path / "compare_non_f8_prompt"
    d.mkdir()
    (d / "file1.txt").write_text("x", encoding="utf-8")
    (d / "dir_a").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    file_target_prompt = _screen_text(tui).lower()
    assert "f1 help" in file_target_prompt
    assert "[f2]" not in file_target_prompt
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("D", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("T", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.quit()


def test_compare_target_prompt_reuses_shared_edit_navigation_behavior(ytree_binary, tmp_path):
    d = tmp_path / "compare_prompt_history"
    d.mkdir()
    (d / "a.txt").write_text("a", encoding="utf-8")
    (d / "b.txt").write_text("b", encoding="utf-8")
    remembered_target = str(d / "remembered_target.txt")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U + remembered_target + Keys.ENTER, wait=0.4)
    assert tui.wait_for_content("Compare execution not implemented yet.", timeout=1.0)
    assert tui.wait_for_content("TARGET: remembered_target.txt", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.2)

    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke("\x10", wait=0.3)  # Ctrl-P history-up
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()


def test_f8_file_compare_uses_inactive_panel_default_target(ytree_binary, tmp_path):
    d = tmp_path / "compare_f8_file_default"
    d.mkdir()
    left_dir = d / "left"
    right_dir = d / "right"
    left_dir.mkdir()
    right_dir.mkdir()
    (left_dir / "left.txt").write_text("left", encoding="utf-8")
    right_file = right_dir / "right.txt"
    right_file.write_text("right", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.7)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.F8, wait=0.4)

    tui.send_keystroke(Keys.TAB, wait=0.3)
    if tui.wait_for_content("Hex J compare", timeout=0.4):
        tui.send_keystroke(Keys.ESC, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke(Keys.TAB, wait=0.3)
    assert tui.wait_for_content("Hex J compare", timeout=1.0)
    tui.send_keystroke("J", wait=0.3)

    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    assert tui.wait_for_content("right.txt", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("Compare execution not implemented yet.", timeout=1.0)
    assert tui.wait_for_content("TARGET: right.txt", timeout=1.0)

    tui.send_keystroke(Keys.ENTER, wait=0.2)
    tui.quit()


def test_f8_directory_compare_uses_inactive_panel_default_target(ytree_binary, tmp_path):
    d = tmp_path / "compare_f8_dir_default"
    d.mkdir()
    alpha = d / "alpha"
    beta = d / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "a.txt").write_text("a", encoding="utf-8")
    (beta / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.7)

    tui.send_keystroke(Keys.DOWN, wait=0.2)  # alpha
    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)  # beta
    tui.send_keystroke(Keys.TAB, wait=0.3)

    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("D", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    assert tui.wait_for_content("beta", timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)
    tui.send_keystroke("S", wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    tui.send_keystroke("F", wait=0.3)
    assert tui.wait_for_content("Compare execution not implemented yet.", timeout=1.0)
    assert tui.wait_for_content("TARGET: beta", timeout=1.0)

    tui.send_keystroke(Keys.ENTER, wait=0.2)
    tui.quit()


def test_f8_tree_compare_uses_inactive_panel_logged_root_default(ytree_binary, tmp_path):
    main_root = tmp_path / "compare_f8_tree_main"
    other_root = tmp_path / "compare_f8_tree_other"
    main_root.mkdir()
    other_root.mkdir()
    (main_root / "main_dir").mkdir()
    (other_root / "other_dir").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(main_root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.3)
    tui.send_keystroke("L", wait=0.3)
    assert tui.wait_for_content("Log Path:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U, wait=0.2)
    tui.send_keystroke(str(other_root) + Keys.ENTER, wait=0.7)

    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("T", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    assert tui.wait_for_content(str(other_root.name), timeout=1.0)
    tui.send_keystroke(Keys.ENTER, wait=0.3)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)
    tui.send_keystroke("S", wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    tui.send_keystroke("F", wait=0.3)

    assert tui.wait_for_content("Compare execution not implemented yet.", timeout=1.0)
    assert tui.wait_for_content("TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.ENTER, wait=0.2)
    tui.quit()


def test_compare_flow_cancel_is_safe_and_footer_remains_clean(ytree_binary, tmp_path):
    d = tmp_path / "compare_cancel_safety"
    d.mkdir()
    (d / "x").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.6)

    tui.send_keystroke("C", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Compare execution not implemented yet.", timeout=0.5)

    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("D", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Compare execution not implemented yet.", timeout=0.5)

    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("D", wait=0.2)
    tui.send_keystroke("." + Keys.ENTER, wait=0.2)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Compare execution not implemented yet.", timeout=0.5)

    tui.send_keystroke("C", wait=0.2)
    tui.send_keystroke("D", wait=0.2)
    tui.send_keystroke("." + Keys.ENTER, wait=0.2)
    tui.send_keystroke("S", wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert not tui.wait_for_content("Compare execution not implemented yet.", timeout=0.5)

    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_compare_prompt_labels_and_result_text(ytree_binary, tmp_path):
    d = tmp_path / "compare_prompt_labels"
    d.mkdir()
    (d / "alpha").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.6)

    tui.send_keystroke("C", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)
    screen_lower = _screen_text(tui).lower()
    assert "logged tree" in screen_lower
    assert "directory + logged tree" not in screen_lower
    tui.send_keystroke("D", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    target_prompt = _screen_text(tui).lower()
    assert "f1 help" in target_prompt
    assert "f2 browse" in target_prompt
    assert "[f2]" not in target_prompt
    tui.send_keystroke("." + Keys.ENTER, wait=0.2)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)
    tui.send_keystroke("S", wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    screen_lower = _screen_text(tui).lower()
    assert "fifferent" not in screen_lower
    assert "left-only" not in screen_lower
    assert "unique" in screen_lower
    assert "type-mismatch" in screen_lower
    assert "error" in screen_lower
    tui.send_keystroke(Keys.ESC, wait=0.2)
    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_compare_help_f1_open_close_and_prompt_restore(ytree_binary, tmp_path):
    d = tmp_path / "compare_help_cycle"
    d.mkdir()
    (d / "alpha").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.6)

    tui.send_keystroke("C", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)

    tui.send_keystroke(Keys.F1, wait=0.3)
    scope_help = _screen_text(tui).lower()
    assert "compare help" in scope_help
    assert "directory only compares the current directory" in scope_help
    assert "logged tree compares the current logged tree recursively" in scope_help

    tui.send_keystroke("H", wait=0.2)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0)
    tui.send_keystroke("D", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.F1, wait=0.3)
    target_help = _screen_text(tui).lower()
    assert "compare help" in target_help
    assert "current directory is the compare source" in target_help
    assert "(f2) browse" in target_help
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke("." + Keys.ENTER, wait=0.2)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)

    tui.send_keystroke(Keys.F1, wait=0.3)
    basis_help = _screen_text(tui).lower()
    assert "compare help" in basis_help
    assert "size checks file length" in basis_help
    assert "date checks the last-modified time" in basis_help
    assert "hash opens both files" in basis_help
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("COMPARE BASIS:", timeout=1.0)

    tui.send_keystroke("S", wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    tui.send_keystroke(Keys.F1, wait=0.3)
    result_help = _screen_text(tui).lower()
    assert "compare help" in result_help
    assert "choose which compare result to tag in the active file list" in result_help
    assert "different tags basis mismatches" in result_help
    assert "unique tags source-only entries" in result_help
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("TAG FILE LIST:", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)

    _assert_no_footer_artifacts(tui)
    tui.quit()


def test_file_compare_target_help_is_file_specific_and_f2_browse_keeps_footer_clean(
    ytree_binary, tmp_path
):
    d = tmp_path / "compare_file_target_help"
    d.mkdir()
    (d / "alpha.txt").write_text("a", encoding="utf-8")
    (d / "browse_here").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(0.6)

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke("J", wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.F1, wait=0.3)
    help_text = _screen_text(tui).lower()
    assert "current file is the compare source" in help_text
    assert "current directory is the compare source" not in help_text
    assert "current logged tree is the compare source" not in help_text
    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)

    tui.send_keystroke(Keys.F2, wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    footer = "\n".join(_footer_lines(tui)).lower()
    assert "file     attributes" not in footer, f"File footer bled into compare target prompt:\n{footer}"

    tui.send_keystroke(Keys.ESC, wait=0.2)
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

    screen = _screen_text(tui)
    assert "COPY: TAGGED FILES" in screen, f"Ctrl-K in file view should keep tagged-copy behavior:\n{screen}"

    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.quit()
