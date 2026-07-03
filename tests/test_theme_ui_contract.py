import re
from pathlib import Path


def _read(path):
    return Path(path).read_text(encoding="utf-8")


def _command_strip_text(source, array_name):
    array = re.search(
        rf"static const UICommandStripPart {array_name}\[\] = \{{(?P<body>.*?)\}};",
        source,
        re.S,
    )
    assert array, f"missing command strip array {array_name}"
    return "".join(
        re.findall(r'\{UI_COMMAND_(?:PUNCT|LABEL|KEY), "([^"]*)"\}', array.group("body"))
    )


def test_f2_footer_uses_required_theme_command_strip():
    source = _read("src/ui/f2_picker.c")

    assert _command_strip_text(source, "f2_command_strip") == "(L)og  (<)/(>) Cycle"
    assert "UI_RenderCommandStrip" in source
    assert '"[ (L)og (< >) Cycle ]"' not in source


def test_volume_menu_uses_required_theme_command_strip():
    source = _read("src/ui/volume_menu.c")

    assert (
        _command_strip_text(source, "volume_command_strip")
        == "Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete"
    )
    assert "UI_RenderCommandStrip" in source
    assert "Use UP/DOWN to select" not in source


def test_help_surfaces_use_help_role():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")
    init_source = _read("src/core/init.c")
    display_source = _read("src/ui/display.c")
    compare_source = _read("src/ui/compare_request.c")

    assert "CPAIR_HELP" in defs_source
    assert '{"HELP_COLOR", CPAIR_HELP, 7, 0},' in color_source
    assert '{"help", {"HELP_COLOR", NULL}},' in theme_source
    assert "ctx->ctx_menu_window, COLOR_PAIR(CPAIR_HELP)" in init_source
    assert "lo_color = CPAIR_HELP;" in display_source
    assert "PrintMenuOptions(ctx->ctx_menu_window, i, 0," in display_source
    assert "dir_help[ctx->view_mode][i]," in display_source
    assert "file_help[ctx->view_mode][i]," in display_source
    assert "CPAIR_HELP, CPAIR_HIMENUS" in display_source
    assert '(char *)"History   (P)in/unpin' in display_source
    assert "COLOR_PAIR(CPAIR_HELP)" in compare_source


def test_picker_surfaces_use_picker_and_selection_roles():
    completion_source = _read("src/ui/completion_dialog.c")
    volume_source = _read("src/ui/volume_menu.c")
    render_dir_source = _read("src/ui/render_dir.c")

    assert "WbkgdSet(ctx, ctx->ctx_matches_window, COLOR_PAIR(color));" not in (
        completion_source
    )
    assert "ctx->ctx_matches_window, COLOR_PAIR(CPAIR_WINHST)" in completion_source
    assert "WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINHST));" in volume_source
    assert "COLOR_PAIR(CPAIR_HIHST)" in volume_source
    assert "win == ctx->ctx_f2_window" in render_dir_source
    assert "color = CPAIR_HST;" in render_dir_source


def test_active_file_and_tree_selection_use_selection_role_pairs():
    dir_source = _read("src/ui/render_dir.c")
    file_source = _read("src/ui/render_file.c")

    assert "highlight_color = CPAIR_HIDIR;" in dir_source
    assert "highlight_color = CPAIR_HIHST;" in dir_source
    assert "COLOR_PAIR(highlight_color)" in dir_source
    assert "inactive_full_line_attr = (hilight && ctx->highlight_full_line && !is_active)" in dir_source
    assert "wattron(win, A_BOLD | A_UNDERLINE);" in dir_source

    assert "highlight_color_pair = CPAIR_HIFILE;" in file_source
    assert "COLOR_PAIR(highlight_color_pair)" in file_source
    assert "inactive_highlight_attr = A_BOLD | A_UNDERLINE;" in file_source
    assert "if (hilight && !is_active_panel)" in file_source
    assert "A_REVERSE" not in file_source


def test_tree_lines_and_margin_use_dedicated_theme_roles():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")
    dir_source = _read("src/ui/render_dir.c")

    assert "CPAIR_TREE_LINES" in defs_source
    assert "CPAIR_MARGIN" in defs_source
    assert '{"TREE_LINES_COLOR", CPAIR_TREE_LINES, 7, 0},' in color_source
    assert '{"MARGIN_COLOR", CPAIR_MARGIN, 7, 0},' in color_source
    assert '{"tree_lines", {"TREE_LINES_COLOR", NULL}},' in theme_source
    assert '{"margin", {"MARGIN_COLOR", NULL}},' in theme_source
    assert "margin_color = CPAIR_MARGIN;" in dir_source
    assert "tree_line_color = CPAIR_TREE_LINES;" in dir_source
    assert "wattrset(win, margin_attr);" in dir_source
    assert "wattrset(win, tree_line_attr);" in dir_source
    assert "GetFileTypeColor" not in dir_source


def test_disabled_role_projects_to_runtime_pair():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")

    assert "CPAIR_DISABLED" in defs_source
    assert '{"DISABLED_COLOR", CPAIR_DISABLED, 8, 0}' in color_source
    assert '{"disabled", {"DISABLED_COLOR", NULL}},' in theme_source


def test_header_path_uses_dynamic_text_role():
    display_source = _read("src/ui/display.c")

    assert "DisplayHeaderPath" in display_source
    assert "WbkgdSet(ctx, ctx->ctx_path_window, COLOR_PAIR(CPAIR_FILE));" in (
        display_source
    )
    assert "wattrset(ctx->ctx_path_window, COLOR_PAIR(CPAIR_FILE));" in (
        display_source
    )


def test_clock_uses_dynamic_text_role():
    init_source = _read("src/core/init.c")
    clock_source = _read("src/core/clock.c")

    assert (
        "ctx->ctx_time_window, COLOR_PAIR(CPAIR_FILE)" in init_source
    )
    assert "COLOR_PAIR(CPAIR_WINDIR | A_BOLD)" not in init_source
    assert "wattrset(ctx->ctx_time_window, COLOR_PAIR(CPAIR_FILE));" in (
        clock_source
    )
    assert "COLOR_PAIR(CPAIR_MENU)" not in clock_source


def test_stats_rendering_splits_static_dynamic_and_border_roles():
    stats_source = _read("src/ui/stats.c")
    theme_source = _read("src/cmd/theme.c")

    assert "static void SetStatsStaticColor" in stats_source
    assert "static void SetStatsDynamicColor" in stats_source
    assert "static void SetStatsBorderColor" in stats_source
    assert "COLOR_PAIR(CPAIR_MENU)" in stats_source
    assert "COLOR_PAIR(CPAIR_STATS)" in stats_source
    assert "COLOR_PAIR(CPAIR_BORDERS)" in stats_source
    assert "COLOR_PAIR(color)" not in stats_source
    assert '{"static_text", {"MENU_COLOR", NULL}},' in theme_source
    assert '{"box_lines", {"BORDERS_COLOR", NULL}},' in theme_source
    assert (
        '"DIR_COLOR", "WINDIR_COLOR", "FILE_COLOR", "WINFILE_COLOR"'
        in theme_source
    )
    assert '"STATS_COLOR", "WINSTATS_COLOR", NULL' in theme_source


def test_viewer_frame_uses_border_role_not_directory_fill_role():
    internal_source = _read("src/ui/view_internal.c")
    tagged_source = _read("src/ui/tagged_view.c")

    assert "ctx->viewer.view, COLOR_PAIR(CPAIR_WINDIR)" in internal_source
    assert "ctx->viewer.view, COLOR_PAIR(CPAIR_WINDIR)" in tagged_source
    assert "ctx->viewer.border, COLOR_PAIR(CPAIR_BORDERS)" in internal_source
    assert "ctx->viewer.border, COLOR_PAIR(CPAIR_BORDERS)" in tagged_source


def test_viewer_file_headers_use_dynamic_text_role():
    internal_source = _read("src/ui/view_internal.c")
    tagged_source = _read("src/ui/tagged_view.c")

    assert 'Print(stdscr, geom->header_y, 0, "File: ", CPAIR_MENU);' in (
        internal_source
    )
    assert "CutPathname(str, file_path, ctx->viewer.wcols - 5), CPAIR_FILE" in (
        internal_source
    )
    assert 'Print(stdscr, ctx->layout.header_y, 0, "File: ", CPAIR_MENU);' in (
        tagged_source
    )
    assert "CutPathname(clipped_header, header_buf, available), CPAIR_FILE" in (
        tagged_source
    )


def test_preview_content_resets_search_hit_to_base_role_pair():
    preview_source = _read("src/ui/view_preview.c")

    assert "WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINFILE));" in preview_source
    assert "wattrset(win, COLOR_PAIR(CPAIR_FILE));" in preview_source
    assert "wattrset(win, COLOR_PAIR(CPAIR_HIGLOBAL));" in preview_source
    assert "wattroff(win, COLOR_PAIR(CPAIR_HIGLOBAL));" not in preview_source


def test_modal_prompt_keeps_severity_role_pair():
    error_source = _read("src/ui/error.c")

    assert "COLOR_PAIR(color_pair) | A_BOLD" in error_source
    assert "A_REVERSE | A_BLINK" not in error_source
    assert "wattrset(ctx->ctx_error_window, COLOR_PAIR(color_pair));" in error_source


def test_theme_docs_capture_role_routing_invariants():
    spec_source = _read("docs/SPECIFICATION.md")
    arch_source = _read("docs/ARCHITECTURE.md")

    assert "F1/context help surfaces use the `help` role" in spec_source
    assert (
        "F2, history, completion, and volume selection surfaces use the `picker` role"
        in spec_source
    )
    assert "`WINERR_COLOR` is a migration-only alias for `ERR_COLOR`" in spec_source
    assert "Set a window background once per refresh path" in arch_source
    assert "stats titles and fixed labels use `static_text`" in arch_source
    assert "changing stats values use `dynamic_text`" in arch_source
    assert "MUST NOT use raw reverse/blink styling" in spec_source
    assert "tree guide glyphs use `tree_lines`" in spec_source
    assert "File-type palette rules do not style directory tree rows" in spec_source
    assert "tree status columns use `margin`" in arch_source
    assert "Preview/search-hit highlighting uses `search_hit`" in spec_source
    assert "Frame/Fill Separation" in arch_source
    assert "search-hit spans use `search_hit`" in arch_source


def test_f10_surface_uses_required_command_strip_and_enter_default():
    source = _read("src/ui/ui_edit_config.c")
    key_source = _read("src/ui/key_engine.c")
    header_source = _read("include/ytnova_ui.h")

    assert (
        _command_strip_text(source, "config_command_strip")
        == "(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"
    )
    assert "InputChoiceCommandStrip" in source
    assert "InputChoiceCommandStrip" in key_source
    assert "InputChoiceCommandStrip" in header_source
    assert '"(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"' not in source
    assert 'case CR:' in source
    assert 'case LF:' in source
    assert 'case \'C\':' in source
    assert 'case \'T\':' in source
    assert 'case \'R\':' in source


def test_reload_failures_use_status_line_without_success_message():
    source = _read("src/ui/ui_edit_config.c")

    assert "UI_ShowStatusLineError" in source
    assert "Reload failed: can't read config" in source
    assert "Reload failed: can't load theme" in source
    assert "Reloaded" not in source
    assert "reload successful" not in source.lower()
