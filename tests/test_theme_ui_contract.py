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
    assert "if (!is_active)\n      wattron(win, A_BOLD | A_UNDERLINE);" in dir_source

    assert "highlight_color_pair = CPAIR_HIFILE;" in file_source
    assert "COLOR_PAIR(highlight_color_pair)" in file_source
    assert "if (hilight && !is_active_panel)" in file_source


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
    assert (
        '{"dynamic_text", {"DIR_COLOR", "FILE_COLOR", "STATS_COLOR", NULL}},'
        in theme_source
    )


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


def test_f10_surface_uses_required_command_strip_and_enter_default():
    source = _read("src/ui/ui_edit_config.c")

    assert '"(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"' in source
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
