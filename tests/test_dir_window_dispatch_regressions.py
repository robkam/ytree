from pathlib import Path
import re
import time

from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _dir_navigation_action_case_source(action_name):
    source = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    switch_start = source.find("switch (action)")
    case_start = source.find(f"case {action_name}:", switch_start)
    assert case_start >= 0, (
        f"Missing expected {action_name} branch in HandleDirWindow switch"
    )

    next_case = source.find("\n    case ", case_start + 1)
    if next_case < 0:
        next_case = len(source)
    return source[case_start:next_case]


def _split_screen_action_case_source():
    source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    case_start = source.find("case ACTION_SPLIT_SCREEN:")
    case_end = source.find("case ACTION_SWITCH_PANEL:", case_start)
    assert case_start >= 0 and case_end > case_start, (
        "Could not isolate ACTION_SPLIT_SCREEN case in "
        "HandleDirWindowPanelAction (src/ui/dir_ops.c)"
    )
    return source[case_start:case_end]


def _restore_panel_tree_selection_source():
    source = Path("src/cmd/log.c").read_text(encoding="utf-8")
    func_start = source.find("static void RestorePanelTreeSelection(")
    assert func_start >= 0, "Missing RestorePanelTreeSelection in src/cmd/log.c"

    next_func = source.find("\nstatic ", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate RestorePanelTreeSelection in src/cmd/log.c"
    )
    return source[func_start:next_func]


def test_tree_viewport_stable_restore_preserves_visible_selection():
    restore_source = _restore_panel_tree_selection_source()
    visible_guard = (
        r"selected_index\s*>=\s*panel->disp_begin_pos"
        r"[\s\S]*selected_index\s*<\s*panel->disp_begin_pos\s*\+\s*win_height"
        r"[\s\S]*panel->cursor_pos\s*=\s*selected_index\s*-\s*panel->disp_begin_pos"
        r"[\s\S]*return\s*;"
    )

    assert re.search(visible_guard, restore_source), (
        "RestorePanelTreeSelection must preserve disp_begin_pos when the saved "
        "tree selection is already visible; Enter/log round-trips should not "
        f"bottom-align an in-view row.\n{restore_source}"
    )


def _first_visible_edge_tree_row(tui):
    for line in tui.get_screen_dump():
        if not line.startswith("x"):
            continue
        match = re.search(r"dir_\d{2}_edge_scroll\b", line[:60])
        if match:
            return match.group(0)
    return None


def test_tree_viewport_edge_scroll_after_end_up_keeps_top_visible_row(
    ytree_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_edge_scroll"
    root.mkdir()
    for idx in range(45):
        (root / f"dir_{idx:02d}_edge_scroll").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke("\033OF", wait=0.35)
    assert tui.wait_for_content(
        "dir_44_edge_scroll", timeout=1.0
    ), _screen_text(tui)
    top_after_end = _first_visible_edge_tree_row(tui)
    assert top_after_end is not None, _screen_text(tui)

    tui.send_keystroke(Keys.UP, wait=0.35)

    screen = _screen_text(tui)
    assert "dir_43_edge_scroll" in screen, (
        "UP after END should move selection to the previous tree row.\n" f"{screen}"
    )
    assert _first_visible_edge_tree_row(tui) == top_after_end, (
        "UP after END should keep the tree viewport origin stable while the "
        "new selection remains inside the visible window.\n"
        f"top_after_end={top_after_end!r}\n{screen}"
    )

    for _ in range(16):
        tui.send_keystroke(Keys.UP, wait=0.05)

    screen = _screen_text(tui)
    assert "dir_27_edge_scroll" in screen, (
        "Selection should be able to move to the top visible row without "
        "moving the tree viewport.\n"
        f"{screen}"
    )
    assert _first_visible_edge_tree_row(tui) == top_after_end, screen

    tui.send_keystroke(Keys.UP, wait=0.2)

    screen = _screen_text(tui)
    assert _first_visible_edge_tree_row(tui) == "dir_26_edge_scroll", (
        "Once UP moves past the top visible row, the tree should scroll by "
        "one row.\n"
        f"{screen}"
    )

    tui.quit()


def test_dir_window_sibling_navigation_sets_help_refresh_flag():
    success_guard = "if (target != NULL && target != dir_entry) {"
    for action_name in ("ACTION_MOVE_SIBLING_NEXT", "ACTION_MOVE_SIBLING_PREV"):
        case_source = _dir_navigation_action_case_source(action_name)
        guard_start = case_source.find(success_guard)
        assert guard_start >= 0, (
            "Sibling navigation must guard refresh behavior on successful target "
            f"selection in {action_name}.\n{case_source}"
        )
        guard_end = case_source.find("\n      }\n", guard_start)
        assert guard_end > guard_start, (
            "Could not isolate successful sibling-selection block in "
            f"{action_name}.\n{case_source}"
        )
        success_block = case_source[guard_start:guard_end]

        assert "DirOps_SelectVisibleDirAndRefresh" in success_block, success_block
        assert (
            "*need_dsp_help_ptr = TRUE;" in success_block
            or "need_dsp_help = TRUE;" in success_block
        ), (
            "Sibling navigation should request a directory-help/footer refresh "
            f"inside successful selection handling in {action_name}.\n"
            f"{success_block}"
        )


def test_dir_window_navigation_selects_expected_directory(ytree_binary, tmp_path):
    root = tmp_path / "dir_dispatch_nav"
    root.mkdir()
    alpha = root / "alpha_dir_dispatch"
    beta = root / "beta_dir_dispatch"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only_file.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_only_file.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = _screen_text(tui)
    footer = _footer_text(tui)
    assert "beta_only_file.txt" in screen, (
        "Tree navigation + enter should open the selected directory's file list.\n"
        f"{screen}"
    )
    assert "hex invert j compare" in footer, (
        "Expected file-window footer after entering the selected directory.\n"
        f"{footer}"
    )

    tui.quit()


def test_dir_window_compare_prompt_round_trip(ytree_binary, tmp_path):
    root = tmp_path / "dir_dispatch_compare_prompt"
    root.mkdir()
    (root / "left").mkdir()
    (root / "right").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.6)

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE SCOPE:", timeout=1.0), _screen_text(tui)

    tui.send_keystroke(Keys.ESC, wait=0.25)
    footer = _footer_text(tui)
    assert "j compare" in footer and "j tree" in footer, (
        "Exiting compare scope prompt should restore directory footer hints.\n"
        f"{footer}"
    )

    tui.quit()


def test_dir_window_split_unsplit_branch_requires_peer_panel_guards():
    case_source = _split_screen_action_case_source()
    assert re.search(
        r"if\s*\(\s*ctx->is_split_screen\s*&&\s*ctx->active\s*==\s*ctx->right"
        r"\s*&&\s*ctx->left\s*&&\s*ctx->right\s*\)",
        case_source,
    ), (
        "ACTION_SPLIT_SCREEN unsplit path must guard peer panels before state "
        f"copy from right to left.\n{case_source}"
    )


def test_dir_window_split_and_tab_keeps_file_focus(ytree_binary, tmp_path):
    root = tmp_path / "dir_dispatch_split_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_split_focus.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_split_focus.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)
    assert "hex invert j compare" in _footer_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    footer = _footer_text(tui)
    assert "hex invert j compare" in footer, (
        "Split + panel switch from file view should preserve file focus footer.\n"
        f"{footer}"
    )

    tui.quit()


def test_dir_right_arrow_drills_into_first_child_when_already_expanded(
    ytree_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_right_drill"
    root.mkdir()
    parent = root / "parent_dir_dispatch"
    child = parent / "child_dir_dispatch"
    child.mkdir(parents=True)
    (child / "child_only_marker.txt").write_text("child\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.25)  # select parent_dir_dispatch
    tui.send_keystroke(Keys.RIGHT, wait=0.35)  # expand parent
    tui.send_keystroke(Keys.RIGHT, wait=0.35)  # drill into first child

    screen = _screen_text(tui)
    assert "parent_dir_dispatch/child_dir_dispatch" in screen, (
        "RIGHT on an already-expanded node should move selection to its first "
        "child and update the active path.\n"
        f"{screen}"
    )

    tui.quit()
