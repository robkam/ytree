from pathlib import Path
import re
import time

from helpers_source import extract_function_block as _extract_function_block
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


def _split_transition_source():
    return Path("src/ui/split_transition.c").read_text(encoding="utf-8")


def _restore_panel_tree_selection_source():
    source = Path("src/cmd/log.c").read_text(encoding="utf-8")
    func_start = source.find("static void RestorePanelTreeSelection(")
    assert func_start >= 0, "Missing RestorePanelTreeSelection in src/cmd/log.c"

    next_func = source.find("\nstatic ", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate RestorePanelTreeSelection in src/cmd/log.c"
    )
    return source[func_start:next_func]


def _capture_panel_anchor_source():
    source = Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")
    func_start = source.find("BOOL CapturePanelAnchorPath(")
    assert func_start >= 0, "Missing CapturePanelAnchorPath in src/ui/panel_anchor.c"

    next_func = source.find("\nint FindDirIndexByPath(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate CapturePanelAnchorPath in src/ui/panel_anchor.c"
    )
    return source[func_start:next_func]


def _resolve_panel_file_anchor_source():
    source = Path("src/ui/display.c").read_text(encoding="utf-8")
    func_start = source.find("static DirEntry *ResolvePanelFileAnchor(")
    assert func_start >= 0, "Missing ResolvePanelFileAnchor in src/ui/display.c"

    next_func = source.find(
        "\nstatic DirEntry *ResolvePanelFileAnchorForRender(", func_start + 1
    )
    assert next_func > func_start, (
        "Could not isolate ResolvePanelFileAnchor in src/ui/display.c"
    )
    return source[func_start:next_func]


def _restore_panel_file_selection_source():
    source = Path("src/cmd/log.c").read_text(encoding="utf-8")
    func_start = source.find("static void RestorePanelFileSelection(")
    assert func_start >= 0, "Missing RestorePanelFileSelection in src/cmd/log.c"

    next_func = source.find("\nstatic void SavePanelTreeSelection(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate RestorePanelFileSelection in src/cmd/log.c"
    )
    return source[func_start:next_func]


def _rebind_active_file_panel_selection_source():
    source = Path("src/ui/ctrl_file_ops.c").read_text(encoding="utf-8")
    func_start = source.find("BOOL RebindActiveFilePanelSelection(")
    assert func_start >= 0, (
        "Missing RebindActiveFilePanelSelection in src/ui/ctrl_file_ops.c"
    )

    next_func = source.find("\nstatic void DebugLogFilePanelState(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate RebindActiveFilePanelSelection in "
        "src/ui/ctrl_file_ops.c"
    )
    return source[func_start:next_func]


def _resolve_panel_anchor_target_source():
    source = Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")
    func_start = source.find("static BOOL PanelAnchorTargetIsVisible(")
    assert func_start >= 0, (
        "Missing ResolvePanelAnchorTarget support helpers in "
        "src/ui/panel_anchor.c"
    )

    next_func = source.find("\nDirEntry *FindDirByPathInTree(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate panel anchor restore helpers in src/ui/panel_anchor.c"
    )
    return source[func_start:next_func]


def _handle_switch_window_source():
    source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    func_start = source.find("void HandleSwitchWindow(")
    assert func_start >= 0, "Missing HandleSwitchWindow in src/ui/dir_ops.c"

    next_func = source.find("\nvoid SyncActivePanelWindows(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate HandleSwitchWindow in src/ui/dir_ops.c"
    )
    return source[func_start:next_func]


def _panel_selected_file_path_source():
    source = Path("src/ui/interactions_panel_paths.c").read_text(
        encoding="utf-8"
    )
    func_start = source.find("int UI_GetPanelSelectedFilePath(")
    assert func_start >= 0, (
        "Missing UI_GetPanelSelectedFilePath in src/ui/interactions_panel_paths.c"
    )

    return source[func_start:]


def _defs_source():
    return Path("include/ytree_defs.h").read_text(encoding="utf-8")


def _log_source():
    return Path("src/cmd/log.c").read_text(encoding="utf-8")


def _panel_anchor_file_source():
    return Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")


def _dir_ops_source():
    return Path("src/ui/dir_ops.c").read_text(encoding="utf-8")


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


def test_panel_restore_paths_use_canonical_selection_identity_only():
    capture_source = _capture_panel_anchor_source()
    resolve_source = _resolve_panel_file_anchor_source()
    restore_source = _restore_panel_file_selection_source()
    rebind_source = _rebind_active_file_panel_selection_source()
    switch_source = _handle_switch_window_source()
    panel_file_path_source = _panel_selected_file_path_source()

    assert "assert(!panel->vol || panel->vol == vol);" in capture_source, (
        "CapturePanelAnchorPath must fail fast on non-owner volume access.\n"
        f"{capture_source}"
    )
    assert "file_dir_entry" not in capture_source, (
        "CapturePanelAnchorPath must not recover file anchor authority from a "
        "raw file_dir_entry alias.\n"
        f"{capture_source}"
    )
    assert "file_dir_entry" not in resolve_source, (
        "ResolvePanelFileAnchor must use the canonical selection path, not the "
        "raw file_dir_entry pointer, as its restore authority.\n"
        f"{resolve_source}"
    )
    assert "saved_file_dir_path" not in restore_source, (
        "RestorePanelFileSelection must not fall back to the pointer-derived "
        "saved_file_dir_path breadcrumb when the canonical selection path is "
        "available.\n"
        f"{restore_source}"
    )
    assert "ResolvePanelAnchorTarget(panel, panel->vol," in rebind_source, (
        "RebindActiveFilePanelSelection must resolve the active file panel by "
        "canonical anchor path identity.\n"
        f"{rebind_source}"
    )
    assert "panel->file_selection_dir_path" in rebind_source, (
        "RebindActiveFilePanelSelection must resolve by the saved canonical "
        "selection path.\n"
        f"{rebind_source}"
    )
    assert "GetPanelDirEntry(panel)" not in rebind_source, (
        "RebindActiveFilePanelSelection must not fall back to raw tree-row "
        "authority when canonical anchor resolution fails.\n"
        f"{rebind_source}"
    )
    assert "file_dir_entry == dir_entry" not in switch_source, (
        "HandleSwitchWindow must key file-window restore off the canonical "
        "selection path, not a file_dir_entry alias comparison.\n"
        f"{switch_source}"
    )
    assert "file_dir_entry == dir_entry" not in panel_file_path_source, (
        "UI_GetPanelSelectedFilePath must not infer file ownership from the "
        "raw file_dir_entry pointer alias.\n"
        f"{panel_file_path_source}"
    )


def test_restore_snapshots_validate_generation_before_reuse():
    defs_source = _defs_source()
    log_source = _log_source()
    panel_anchor_source = _panel_anchor_file_source()
    dir_ops_source = _dir_ops_source()

    for needle in (
        "unsigned int saved_panel_generation;",
        "unsigned int saved_volume_generation;",
        "unsigned int saved_tree_generation;",
        "unsigned int saved_tree_volume_generation;",
        "unsigned int volume_generation;",
        "unsigned int panel_generation;",
    ):
        assert needle in defs_source, (
            f"Missing generation field declaration: {needle}\n{defs_source}"
        )

    assert "state->saved_panel_generation = panel->panel_generation;" in log_source
    assert "state->saved_volume_generation = panel->vol->volume_generation;" in log_source
    assert "state->saved_panel_generation != panel->panel_generation" in log_source
    assert "state->saved_volume_generation != vol->volume_generation" in log_source
    assert "panel->vol->saved_tree_generation = panel->panel_generation;" in log_source
    assert (
        "panel->vol->saved_tree_volume_generation = panel->vol->volume_generation;"
        in log_source
    )
    assert "generation_valid =" in log_source
    assert "saved_tree_volume_generation == panel->vol->volume_generation" in log_source
    assert "saved_tree_generation == panel->panel_generation" in log_source

    assert "panel->panel_generation++;" in panel_anchor_source, panel_anchor_source
    assert "dst->panel_generation = src->panel_generation;" in panel_anchor_source

    assert "p->panel_generation++;" in dir_ops_source, dir_ops_source
    assert "p->vol->volume_generation++;" in dir_ops_source, dir_ops_source
    assert "ctx->active->vol->volume_generation++;" in dir_ops_source, dir_ops_source


def test_panel_anchor_restore_follows_exact_fallback_order():
    source = _resolve_panel_anchor_target_source()

    exact = source.find("FindDirByPathInTree(vol->vol_stats.tree, anchor_path)")
    ancestor = source.find("FindDirByPathOrAncestor(vol, anchor_path)")
    visible_ancestor = source.find("PanelAnchorFindVisibleAncestor(panel, vol, ancestor)")
    sibling_helper = source.find("PanelAnchorFindVisibleSibling(panel, vol, sibling_base)")
    root = source.find("return vol->vol_stats.tree;")

    assert exact >= 0, f"ResolvePanelAnchorTarget must resolve exact identity first.\n{source}"
    assert ancestor > exact, (
        "ResolvePanelAnchorTarget must fall back to a visible ancestor after "
        f"exact identity fails.\n{source}"
    )
    assert visible_ancestor > ancestor, (
        "ResolvePanelAnchorTarget must check the visible-ancestor helper before "
        f"trying sibling fallback.\n{source}"
    )
    assert sibling_helper > visible_ancestor, (
        "ResolvePanelAnchorTarget must try visible siblings after ancestor "
        f"fallback.\n{source}"
    )
    assert root > sibling_helper, (
        "ResolvePanelAnchorTarget must end with the root visible node fallback.\n"
        f"{source}"
    )
    assert "FindDirIndexByPathOrAncestor" not in source, (
        "ResolvePanelAnchorTarget must not use the ancestor-only fallback helper "
        "as its restore authority.\n"
        f"{source}"
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


def test_dir_window_split_transition_owner_path_is_canonical():
    split_source = _split_transition_source()
    dir_ops_source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")
    ctrl_source = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    handle_block = _extract_function_block(
        dir_ops_source, "DirWindowDispatchResult\nHandleDirWindowPanelAction("
    )

    assert "ACTION_SPLIT_SCREEN" not in handle_block, (
        "HandleDirWindowPanelAction should no longer own split transitions.\n"
        f"{handle_block}"
    )
    assert "ACTION_SWITCH_PANEL" not in handle_block, (
        "HandleDirWindowPanelAction should no longer own panel switching.\n"
        f"{handle_block}"
    )
    assert "SplitTransition_HandleDirWindowAction(" in ctrl_source, (
        "HandleDirWindow must dispatch split transitions through the owner API.\n"
        f"{ctrl_source}"
    )
    assert re.search(
        r"if\s*\(\s*ctx->is_split_screen\s*&&\s*ctx->active\s*==\s*ctx->right"
        r"\s*&&\s*ctx->left\s*&&\s*ctx->right\s*\)",
        split_source,
    ), (
        "ACTION_SPLIT_SCREEN unsplit path must guard peer panels before state "
        f"copy from right to left.\n{split_source}"
    )
    assert "DonatePanelState(ctx->left, ctx->right);" in split_source, (
        "Split transition ownership must donate the right panel state before "
        f"closing the split.\n{split_source}"
    )
    assert "RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);" in (
        split_source
    ), (
        "Split transition ownership must rebind panel-local file selection "
        f"through the canonical restore helper.\n{split_source}"
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
