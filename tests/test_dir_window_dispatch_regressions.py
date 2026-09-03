from pathlib import Path
import re

from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_source import extract_function_block as _extract_function_block
from helpers_ui import footer_text as _footer_text
from helpers_ui import line_marks_file_as_tagged as _line_marks_file_as_tagged
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


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
    return Path("include/ytnova_defs.h").read_text(encoding="utf-8")


def _log_source():
    return Path("src/cmd/log.c").read_text(encoding="utf-8")


def _log_disk_source():
    source = _log_source()
    func_start = source.find("int LogDisk(")
    assert func_start >= 0, "Missing LogDisk in src/cmd/log.c"

    next_func = source.find("\nint GetNewLogPath(", func_start + 1)
    assert next_func > func_start, "Could not isolate LogDisk in src/cmd/log.c"
    return source[func_start:next_func]


def _panel_anchor_file_source():
    return Path("src/ui/panel_anchor.c").read_text(encoding="utf-8")


def _reset_panel_tree_viewport_snapshot_source():
    source = _panel_anchor_file_source()
    func_start = source.find("void ResetPanelTreeViewportSnapshot(")
    assert func_start >= 0, (
        "Missing ResetPanelTreeViewportSnapshot in src/ui/panel_anchor.c"
    )

    next_func = source.find("\nint FindDirIndexByPath(", func_start + 1)
    assert next_func > func_start, (
        "Could not isolate ResetPanelTreeViewportSnapshot in "
        "src/ui/panel_anchor.c"
    )
    return source[func_start:next_func]


def _dir_ops_source():
    return Path("src/ui/dir_ops.c").read_text(encoding="utf-8")


def _appstate_visibility_source():
    return Path("src/ui/appstate_visibility.c").read_text(encoding="utf-8")


def test_tree_viewport_stable_restore_preserves_visible_selection():
    panel_anchor_source = _panel_anchor_file_source()
    restore_start = panel_anchor_source.index("BOOL RestorePanelTreeViewportSnapshot(")
    restore_end = panel_anchor_source.index("\nvoid RestorePanelAnchorPath(", restore_start)
    restore_source = panel_anchor_source[restore_start:restore_end]
    visible_guard = (
        r"selected_index\s*>=\s*begin"
        r"[\s\S]*selected_index\s*<\s*begin\s*\+\s*win_height"
        r"[\s\S]*cursor\s*=\s*selected_index\s*-\s*begin"
        r"[\s\S]*AppStateCommitPanelTreeViewport\(\s*panel,\s*begin,\s*cursor\s*\)"
        r"[\s\S]*return\s+(?:TRUE|FALSE);"
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
    panel_helper_source = Path("src/ui/appstate_panel.c").read_text(encoding="utf-8")
    dir_ops_source = _dir_ops_source()
    visibility_source = _appstate_visibility_source()

    for needle in (
        "unsigned int saved_panel_generation;",
        "unsigned int saved_volume_generation;",
        "unsigned int saved_tree_volume_generation;",
        "unsigned int volume_generation;",
        "unsigned int panel_generation;",
    ):
        assert needle in defs_source, (
            f"Missing generation field declaration: {needle}\n{defs_source}"
        )

    assert "AppStateCommitPanelVolumeFileSnapshot(" in log_source
    assert "panel->panel_generation, panel->vol->volume_generation" in log_source
    assert "state->saved_panel_generation != panel->panel_generation" in log_source
    assert "state->saved_volume_generation != vol->volume_generation" in log_source
    assert "generation_valid =" in panel_anchor_source
    assert (
        "saved_tree_volume_generation == panel->vol->volume_generation"
        in panel_anchor_source
    )
    assert (
        "saved_tree_panel_generation == panel->panel_generation"
        in panel_anchor_source
    )

    assert 'include "ytnova_appstate_panel.h"' in panel_anchor_source
    for signature in (
        "BOOL AppStateCommitPanelTreeSelection(",
        "BOOL AppStateCommitPanelFileViewport(",
        "BOOL AppStateCommitPanelFileAnchor(",
        "BOOL AppStateCommitPanelTreeViewport(",
    ):
        start = panel_helper_source.index(signature)
        end = panel_helper_source.find("\nBOOL ", start + 1)
        body = (
            panel_helper_source[start:]
            if end < 0
            else panel_helper_source[start:end]
        )
        assert 'AppStateValidatedOwnerField("panel.panel_generation")' in body
        assert "return AppStateCommitPanelGeneration(panel);" in body
    assert "AppStateRestorePanelGeneration(dst, src->panel_generation)" in panel_anchor_source
    assert "AppStateCommitPanelGeneration(panel)" not in panel_anchor_source

    assert "AppStateCommitPanelVisibilityFilter(p, !p->hide_dot_files)" in dir_ops_source
    assert "panel->panel_generation++;" in visibility_source, visibility_source
    assert "AppStateCommitVolumeGeneration(panel->vol)" in visibility_source
    assert "AppStateCommitVolumeGeneration(ctx->active->vol)" in dir_ops_source


def test_volume_tree_restore_uses_panel_path_snapshot_before_index_breadcrumb():
    defs_source = _defs_source()
    log_source = _log_source()
    panel_anchor_source = _panel_anchor_file_source()
    save_start = panel_anchor_source.index("void SavePanelTreeViewportSnapshot(")
    save_end = panel_anchor_source.index("\nint FindDirIndexByPath(", save_start)
    save_source = panel_anchor_source[save_start:save_end]
    restore_start = panel_anchor_source.index("BOOL RestorePanelTreeViewportSnapshot(")
    restore_end = panel_anchor_source.index("\nvoid RestorePanelAnchorPath(", restore_start)
    restore_source = panel_anchor_source[restore_start:restore_end]
    file_restore_source = _restore_panel_file_selection_source()

    for needle in (
        "unsigned int saved_tree_panel_generation;",
        "unsigned int saved_tree_volume_generation;",
        "char saved_tree_selected_dir_path[PATH_LENGTH + 1];",
        "char saved_tree_top_dir_path[PATH_LENGTH + 1];",
    ):
        assert needle in defs_source, (
            "Panel volume restore state must own path-scoped tree snapshot "
            f"metadata: {needle}\n{defs_source}"
        )

    assert "CapturePanelViewportSnapshot(panel, panel->vol, &snapshot);" in save_source
    assert (
        "AppStateCommitPanelVolumeTreeViewportSnapshot("
        in save_source
    )
    assert (
        "snapshot.has_selected_dir_path, snapshot.selected_dir_path,"
        in save_source
    ), save_source

    path_restore = restore_source.find("RestorePanelViewportSnapshot(")
    legacy_index = restore_source.find("panel->vol->saved_tree_index")
    assert path_restore >= 0, restore_source
    assert legacy_index < 0 or path_restore < legacy_index, (
        "RestorePanelTreeSelection must try the panel-local path snapshot "
        "before any legacy index breadcrumb.\n"
        f"{restore_source}"
    )
    assert "state->saved_tree_panel_generation == panel->panel_generation" in (
        restore_source
    )
    assert (
        "state->saved_tree_volume_generation == panel->vol->volume_generation"
        in restore_source
    )
    assert "selected_index = panel->vol->saved_tree_index;" not in restore_source
    assert "vol->saved_tree_index = resolved_index;" not in file_restore_source


def test_log_disk_restore_does_not_use_volume_tree_breadcrumbs():
    log_disk_source = _log_disk_source()
    reset_snapshot_source = _reset_panel_tree_viewport_snapshot_source()

    assert "AppStateRestorePanelGeneration(" in log_disk_source
    assert "state->saved_tree_panel_generation" in log_disk_source
    assert "panel->panel_generation = state->saved_tree_panel_generation;" not in (
        log_disk_source
    )
    assert "panel->vol->saved_tree_generation" not in log_disk_source
    assert "panel->vol->saved_tree_index" not in log_disk_source
    assert "if (reload_requested)" in log_disk_source
    assert "AppStateCommitPanelTreeViewport(panel, 0, 0)" in log_disk_source
    assert "panel->disp_begin_pos = 0;" not in log_disk_source
    assert "panel->cursor_pos = 0;" not in log_disk_source
    assert "ResetPanelTreeViewportSnapshot(panel);" in log_disk_source
    assert (
        "AppStateCommitPanelVolumeTreeViewportSnapshot("
        in reset_snapshot_source
    )
    assert "NULL, FALSE, NULL" in reset_snapshot_source
    assert "saved_tree_index" not in reset_snapshot_source
    assert "saved_tree_generation" not in reset_snapshot_source


def test_volume_action_restore_uses_panel_tree_snapshot_not_index_breadcrumb():
    dir_ops_source = _dir_ops_source()
    volume_start = dir_ops_source.index("HandleDirWindowVolumeAction(")
    volume_end = dir_ops_source.index("\nint RefreshDirWindow(", volume_start)
    volume_source = dir_ops_source[volume_start:volume_end]

    save_snapshot = volume_source.find("SavePanelTreeViewportSnapshot(ctx->active);")
    select_loaded = volume_source.find("SelectLoadedVolume(ctx, NULL)")
    cycle_loaded = volume_source.find("CycleLoadedVolume(ctx, ctx->active,")
    restore_snapshot = volume_source.find(
        "RestorePanelTreeViewportSnapshot(ctx, ctx->active)"
    )

    assert save_snapshot >= 0, volume_source
    assert select_loaded > save_snapshot, volume_source
    assert cycle_loaded > save_snapshot, volume_source
    assert restore_snapshot > save_snapshot, volume_source
    assert "ctx->active->vol->saved_tree_index =" not in volume_source
    assert (
        "ctx->active->disp_begin_pos = ctx->active->vol->saved_tree_index"
        not in volume_source
    )


def test_f2_picker_return_uses_panel_tree_snapshot_not_index_breadcrumb():
    f2_source = Path("src/ui/f2_picker.c").read_text(encoding="utf-8")
    original_vol = f2_source.index("original_vol = ctx->active->vol;")
    restore_block = f2_source.index("if (ctx->active->vol != original_vol)")

    save_snapshot = f2_source.find("SavePanelTreeViewportSnapshot(ctx->active);")
    restore_snapshot = f2_source.find(
        "RestorePanelTreeViewportSnapshot(ctx, ctx->active)", restore_block
    )

    assert 'include "ytnova_panel_anchor.h"' in f2_source
    assert save_snapshot > original_vol, f2_source
    assert save_snapshot < restore_block, f2_source
    assert restore_snapshot > restore_block, f2_source
    assert "ctx->active->vol->saved_tree_index" not in f2_source


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
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_edge_scroll"
    root.mkdir()
    for idx in range(45):
        (root / f"dir_{idx:02d}_edge_scroll").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

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

    steps = 0
    while "dir_27_edge_scroll" not in tui.get_screen_dump()[0]:
        if steps >= 45:
            raise AssertionError(
                "Could not select dir_27_edge_scroll at the viewport edge.\n"
                f"{_screen_text(tui)}"
            )
        tui.send_keystroke(Keys.UP, wait=0.05)
        steps += 1

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


def test_dir_window_navigation_selects_expected_directory(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_nav"
    root.mkdir()
    alpha = root / "alpha_dir_dispatch"
    beta = root / "beta_dir_dispatch"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only_file.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_only_file.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = _screen_text(tui)
    footer = _footer_text(tui)
    assert "beta_only_file.txt" in screen, (
        "Tree navigation + enter should open the selected directory's file list.\n"
        f"{screen}"
    )

    tui.quit()


def test_tab_wraps_to_the_next_visible_tree_sibling(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    root = tmp_path / "tab_visible_sibling_wrap"
    home.mkdir()
    root.mkdir()
    (home / ".ytnova").write_text(
        "[GLOBAL]\nHIDEDOTFILES=1\nTREEDEPTH=1\nSMALLWINDOWSKIP=1\n",
        encoding="utf-8",
    )
    for name in (".hidden_first_sibling", "visible_first_sibling", "visible_last_sibling"):
        (root / name).mkdir()

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), env_extra={"HOME": str(home)}
    )
    try:
        assert tui.wait_for_content("visible_last_sibling", timeout=1.5), _screen_text(tui)
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        reached_last = tui.send_and_wait_for_condition(
            Keys.DOWN,
            lambda lines: lines
            if any("Path:" in line and "visible_last_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert reached_last, _screen_text(tui)

        wrapped = tui.send_and_wait_for_condition(
            Keys.TAB,
            lambda lines: lines
            if any("Path:" in line and "visible_first_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert wrapped, _screen_text(tui)

        reverse_wrapped = tui.send_and_wait_for_condition(
            Keys.SHIFT_TAB,
            lambda lines: lines
            if any("Path:" in line and "visible_last_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert reverse_wrapped, _screen_text(tui)
    finally:
        tui.quit()


def test_dir_window_compare_prompt_round_trip(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_compare_prompt"
    root.mkdir()
    (root / "left").mkdir()
    (root / "right").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)

    tui.send_keystroke(Keys.ESC, wait=0.25)
    footer = _footer_text(tui)

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
        r"\s*&&\s*ctx->left\s*&&\s*ctx->right\s*&&\s*!preserve_left_file_state"
        r"\s*\)",
        split_source,
    ), (
        "ACTION_SPLIT_SCREEN unsplit path must guard peer panels and avoid "
        f"overwriting the surviving left file anchor.\n{split_source}"
    )
    assert (
        "CapturePanelSelectionAnchor(ctx, ctx->left, left_dir_entry);"
        in split_source
    ), (
        "Split transition ownership must recapture the surviving left file "
        f"anchor before closing the split.\n{split_source}"
    )
    assert "RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);" in (
        split_source
    ), (
        "Split transition ownership must rebind panel-local file selection "
        f"through the canonical restore helper.\n{split_source}"
    )


def test_dir_window_hidden_toggle_uses_visible_selection_authority():
    toggle_case = _dir_navigation_action_case_source("ACTION_TOGGLE_HIDDEN")
    post_toggle = toggle_case.split("ToggleDotFiles(ctx, ctx->active);", 1)[1]

    assert "ResolveActiveDirEntry(ctx, s)" in post_toggle, (
        "Hidden-file visibility toggles must restore selection through the "
        f"canonical visible active-dir resolver.\n{toggle_case}"
    )
    assert "->dir_entry_list[" not in post_toggle, (
        "Hidden-file visibility toggles must not synthesize restore authority "
        f"from raw directory row indexes after changing visibility.\n{toggle_case}"
    )


def test_dir_window_post_dispatch_refresh_uses_visible_selection_authority():
    source = Path("src/ui/ctrl_dir.c").read_text(encoding="utf-8")
    marker = "DebugLogDirLoopState(\"after_dispatch\""
    log_start = source.find(marker)
    assert log_start >= 0, "Missing after-dispatch debug marker"

    switch_end = source.rfind("    } /* switch */", 0, log_start)
    assert switch_end >= 0, "Missing HandleDirWindow switch terminator"
    refresh_source = source[switch_end:log_start]

    assert "ResolveActiveDirEntry(ctx, s)" in refresh_source, (
        "Directory-window post-dispatch refresh must restore through the "
        f"canonical visible active-dir resolver.\n{refresh_source}"
    )
    assert "disp_begin_pos + ctx->active->cursor_pos" not in refresh_source, (
        "Directory-window post-dispatch refresh must not synthesize selection "
        f"from raw row math.\n{refresh_source}"
    )
    assert "->dir_entry_list[" not in refresh_source, (
        "Directory-window post-dispatch refresh must not index the raw "
        f"directory row list directly.\n{refresh_source}"
    )


def test_file_window_preview_return_uses_visible_selection_authority():
    source = Path("src/ui/ctrl_file_ops.c").read_text(encoding="utf-8")
    preview_source = _extract_function_block(
        source, "BOOL handle_file_window_preview_action("
    )
    return_source = preview_source.split(
        "AppStateCommitPreviewMode(ctx, !ctx->preview_mode)", 1
    )[1].split("RefreshView(ctx, dir_entry);", 1)[0]

    assert "ResolveActiveDirEntry(ctx, stats_local)" in return_source, (
        "File-window preview return must restore the active directory through "
        f"the canonical visible active-dir resolver.\n{return_source}"
    )
    assert "disp_begin_pos + ctx->active->cursor_pos" not in return_source, (
        "File-window preview return must not synthesize selection from raw row "
        f"math.\n{return_source}"
    )
    assert "->dir_entry_list[" not in return_source, (
        "File-window preview return must not index the raw directory row list "
        f"directly.\n{return_source}"
    )


def test_dir_mutation_results_use_visible_selection_authority():
    source = Path("src/ui/dir_ops.c").read_text(encoding="utf-8")

    delete_source = _extract_function_block(
        source, "DirEntry *HandleDirDeleteDirectory("
    )
    delete_rebuild = delete_source.split(
        "BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);",
        1,
    )[1]
    rename_source = _extract_function_block(
        source, "DirEntry *HandleDirRenameDirectory("
    )
    rename_rebuild = rename_source.split(
        "BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);",
        1,
    )[1]

    for label, post_rebuild in (
        ("directory delete", delete_rebuild),
        ("directory rename", rename_rebuild),
    ):
        assert "ResolveActiveDirEntry(ctx," in post_rebuild, (
            f"{label} rebuild result must restore through the canonical visible "
            f"active-dir resolver.\n{post_rebuild}"
        )
        assert "disp_begin_pos +\n" not in post_rebuild, (
            f"{label} rebuild result must not synthesize selection from raw "
            f"row math.\n{post_rebuild}"
        )
        assert "->dir_entry_list[" not in post_rebuild, (
            f"{label} rebuild result must not index the raw directory row list "
            f"directly.\n{post_rebuild}"
        )


def test_dir_window_split_and_tab_keeps_file_focus(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_split_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_split_focus.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_split_focus.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    footer = _footer_text(tui)

    tui.quit()


def test_split_tab_refresh_rejects_stale_file_restore_snapshot(
    ytnova_binary, tmp_path
):
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in (
        "build",
        "coverage",
        "docs",
        "etc",
        "include",
        "infra",
        "src",
        "tests",
    ):
        (repo / name).mkdir()
    src_cmd = repo / "src" / "cmd"
    src_cmd.mkdir()
    tests_dir = repo / "tests"
    (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")

    for idx in range(3):
        (src_cmd / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
    for idx in range(4):
        (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(repo), env_extra={"HOME": str(home)}
    )

    try:
        def stats_current_dir_contains(marker):
            lines = tui.get_screen_dump()
            split_x = _detect_stats_split_x(lines)
            for i, line in enumerate(lines):
                segment = line[split_x:] if split_x is not None else line
                if "CURRENT DIR" not in segment:
                    continue
                for j in (1, 2):
                    idx = i + j
                    if idx >= len(lines):
                        continue
                    candidate = (
                        lines[idx][split_x:] if split_x is not None else lines[idx]
                    )
                    if marker in candidate:
                        return True
            return False

        steps = 0
        while not stats_current_dir_contains("src"):
            if steps >= 80:
                raise AssertionError(
                    f"Could not focus the src fixture directory.\n{_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.DOWN, wait=0.12)
            steps += 1

        tui.send_keystroke(Keys.RIGHT, wait=0.25)

        steps = 0
        while not stats_current_dir_contains("cmd"):
            if steps >= 80:
                raise AssertionError(
                    f"Could not focus the cmd fixture directory.\n{_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.DOWN, wait=0.12)
            steps += 1

        tui.send_keystroke(Keys.RIGHT, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.45)

        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        selected_name = "src_file_2.c"
        pre_screen = _screen_text(tui)
        pre_tag_state = {}
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = next((line for line in pre_screen.splitlines() if name in line), None)
            assert line is not None, pre_screen
            pre_tag_state[name] = _line_marks_file_as_tagged(line, name)
        assert any(pre_tag_state.values()), (
            "Precondition failed: no tagged source files before split flow.\n"
            f"{pre_screen}"
        )

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content(f"COPY: {selected_name}", timeout=1.0), (
            _screen_text(tui)
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)

        tui.send_keystroke(Keys.TAB, wait=0.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)

        after_tab = _screen_text(tui)

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content(f"COPY: {selected_name}", timeout=1.0), (
            _screen_text(tui)
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)

        for name, expected_tagged in pre_tag_state.items():
            line = next((line for line in after_tab.splitlines() if name in line), None)
            assert line is not None, (
                "Source panel lost file rows after split restore.\n"
                f"{after_tab}"
            )
            assert _line_marks_file_as_tagged(line, name) == expected_tagged, (
                "Source panel tag state changed after in-app generation bump.\n"
                f"Expected tagged={expected_tagged} Row: {line}\n"
                f"{after_tab}"
            )
    finally:
        tui.quit()


def test_dir_right_arrow_drills_into_first_child_when_already_expanded(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_right_drill"
    root.mkdir()
    parent = root / "parent_dir_dispatch"
    child = parent / "child_dir_dispatch"
    child.mkdir(parents=True)
    (child / "child_only_marker.txt").write_text("child\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

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
