import time

from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source
from helpers_stats import current_file_from_stats
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _current_file_from_stats(tui):
    return current_file_from_stats(tui.get_screen_dump())


def test_navigation_dispatch_updates_current_file_stats(ytree_binary, tmp_path):
    root = tmp_path / "dispatch_navigation_stats"
    root.mkdir()
    (root / "aa_one.txt").write_text("one\n", encoding="utf-8")
    (root / "bb_two.txt").write_text("two\n", encoding="utf-8")
    (root / "cc_three.txt").write_text("three\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        first_file = _current_file_from_stats(tui)
        assert first_file == "aa_one.txt", _screen_text(tui)

        tui.send_keystroke(Keys.DOWN, wait=0.25)
        second_file = _current_file_from_stats(tui)
        assert second_file == "bb_two.txt", _screen_text(tui)

        tui.send_keystroke(Keys.UP, wait=0.25)
        up_file = _current_file_from_stats(tui)
        assert up_file == "aa_one.txt", _screen_text(tui)
    finally:
        tui.quit()


def test_make_file_prompt_dispatch_creates_file(ytree_binary, tmp_path):
    root = tmp_path / "dispatch_make_file_prompt"
    root.mkdir()
    (root / "seed.txt").write_text("seed\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.MAKE_FILE, wait=0.25)
        assert tui.wait_for_content("MAKE FILE:", timeout=1.0), _screen_text(tui)

        tui.send_keystroke("created_from_prompt.txt" + Keys.ENTER, wait=0.45)
        assert (root / "created_from_prompt.txt").exists()
    finally:
        tui.quit()


def test_split_and_tab_dispatch_keeps_file_mode_footer(ytree_binary, tmp_path):
    root = tmp_path / "dispatch_split_switch_footer"
    root.mkdir()
    (root / "left.txt").write_text("left\n", encoding="utf-8")
    (root / "right.txt").write_text("right\n", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        assert "hex invert j compare" in _footer_text(tui), _screen_text(tui)

        tui.send_keystroke(Keys.F8, wait=0.45)
        tui.send_keystroke(Keys.TAB, wait=0.45)

        footer = _footer_text(tui)
        assert "hex invert j compare" in footer, _screen_text(tui)
    finally:
        tui.quit()


def test_HandleFileWindow_delegates_misc_dispatch_hotspot():
    ctrl_source = _read_source("src/ui/ctrl_file.c")
    ops_source = _read_source("src/ui/ctrl_file_ops.c")
    handle_block = _extract_function_block(ctrl_source, "int HandleFileWindow(")

    assert "BOOL handle_file_window_misc_dispatch_action(" in ops_source, (
        "File-window misc action dispatch helper must exist in ctrl_file_ops.c "
        "so hotspot branches are extracted out of HandleFileWindow."
    )
    assert "handle_file_window_misc_dispatch_action(" in handle_block, (
        "HandleFileWindow must delegate misc action handling to the extracted "
        "non-controller helper."
    )
    assert 'UI_ReadString(ctx, ctx->active, "MAKE FILE:"' not in handle_block, (
        "Make-file prompt branch body should be handled in extracted misc helper."
    )
    assert "UI_ReadFilter(ctx) == 0" not in handle_block, (
        "Filter branch body should be handled in extracted misc helper."
    )
    assert "GetNewLogPath(ctx, ctx->active" not in handle_block, (
        "Log branch body should be handled in extracted misc helper."
    )


def test_PrintFileEntry_uses_inactive_split_highlight_style():
    render_source = _read_source("src/ui/render_file.c")
    print_file_entry_block = _extract_function_block(
        render_source, "void PrintFileEntry("
    )

    assert "ctx->is_split_screen && panel != ctx->active" in print_file_entry_block, (
        "PrintFileEntry must detect inactive split-panel rows instead of treating "
        "all highlighted rows as active."
    )
    assert "A_BOLD | A_UNDERLINE" in print_file_entry_block, (
        "Inactive split-panel file-row highlight must use underline+bold styling."
    )
