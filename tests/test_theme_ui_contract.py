import re
from pathlib import Path


def _read(path):
    return Path(path).read_text(encoding="utf-8")


def _source_paths(*roots):
    paths = []
    for root in roots:
        paths.extend(Path(root).glob("*.c"))
    return paths


def _command_strip_commands(source, array_name):
    array = re.search(
        rf"static const UICommandStripCommand {array_name}\[\] = \{{(?P<body>.*?)\}};",
        source,
        re.S,
    )
    assert array, f"missing command strip array {array_name}"
    return re.findall(
        r'\{UI_COMMAND_LAYOUT_([A-Z_]+), "([^"]*)", "([^"]*)", (NULL|"([^"]*)")\}',
        array.group("body"),
    )


def _command_strip_text(source, array_name):
    rendered = []
    for layout, label, primary_key, secondary_value, secondary_key in (
        _command_strip_commands(source, array_name)
    ):
        if rendered:
            rendered.append("  ")
        secondary_key = None if secondary_value == "NULL" else secondary_key
        if layout == "MNEMONIC":
            rendered.extend(["(", primary_key, ")", label[1:]])
        elif layout == "KEY_PREFIX":
            rendered.extend(["(", primary_key])
            if secondary_key is not None:
                rendered.extend([")/(", secondary_key])
            rendered.extend([") ", label])
        elif layout == "ALT_MNEMONIC":
            rendered.extend(["(", primary_key, ")/(", secondary_key, ")", label[1:]])
        elif layout == "LABEL_FIRST":
            rendered.extend([label, " (", primary_key])
            if secondary_key is not None:
                rendered.extend([")/(", secondary_key])
            rendered.append(")")
        else:
            raise AssertionError(f"unknown command strip layout {layout}")
    return "".join(rendered)


def _assert_command_strip_uses_full_label_model(source, array_name, labels):
    assert f"static const UICommandStripCommand {array_name}[]" in source
    assert "UICommandStripPart" not in source
    assert "UI_COMMAND_LABEL" not in source
    assert "UI_COMMAND_PUNCT" not in source
    for label in labels:
        assert f'"{label}"' in source


def test_f2_footer_uses_required_theme_command_strip():
    source = _read("src/ui/f2_picker.c")

    assert _command_strip_text(source, "f2_command_strip") == "(L)og  (<)/(>) cycle"
    _assert_command_strip_uses_full_label_model(
        source, "f2_command_strip", ("Log", "cycle")
    )
    assert "UI_RenderCommandStrip" in source
    assert '"[ (L)og (< >) Cycle ]"' not in source


def test_command_strip_key_role_controls_color_styling():
    source = _read("src/ui/display_utils.c")
    color_source = _read("src/ui/color.c")

    assert "UIKeybindAttrForBase(hcolor, ncolor)" in source
    assert "key_attr = COLOR_PAIR(hcolor) | A_BOLD;" not in source
    assert "COLOR_PAIR(color) | A_BOLD" not in source
    assert "init_pair(UI_KEYBIND_BASE_PAIR + (i - 1)," in color_source
    assert "NormalizeColorIndex(UIColorForeground(UI_ROLE_KEYBIND), COLORS)," in color_source
    assert "NormalizeColorIndex(UIColorBackground(i), COLORS));" in color_source


def test_color_supported_roles_do_not_add_bold_attributes():
    offenders = []
    for path in _source_paths("src/ui", "src/core"):
        source = path.read_text(encoding="utf-8")
        if re.search(r"COLOR_PAIR\([^\n]*\) \| A_BOLD", source):
            offenders.append(str(path))
        if re.search(r"WbkgdSet\([^\n]*A_BOLD", source):
            offenders.append(str(path))

    assert offenders == []


def test_render_surfaces_consume_semantic_role_aliases():
    legacy_pair = re.compile(
        r"CPAIR_(TREE_LINES|MARGIN|HIGLOBAL|GLOBAL|FILE|WINFILE|DIR|WINDIR|"
        r"MENU|STATS|WINSTATS|BORDERS|HST|HIHST|WINHST|HELP|HIMENUS|"
        r"ERR|WARN|INFO|DIALOG|DISABLED)"
    )
    offenders = []

    for path in _source_paths("src/ui", "src/core"):
        if path.name == "color.c":
            continue
        source = path.read_text(encoding="utf-8")
        if legacy_pair.search(source):
            offenders.append(str(path))

    assert offenders == []


def test_startup_recreates_windows_once_after_theme_load():
    source = _read("src/core/init.c")
    load_done = source.index('DEBUG_LOG("Init: LoadTheme done")')
    reinit_done = source.index('DEBUG_LOG("Init: ReinitColorPairs done")')
    recreate_after_theme = source.index(
        'DEBUG_LOG("Init: ReCreateWindows after theme done")'
    )

    assert source.count("ReCreateWindows(ctx);") == 1
    assert load_done < reinit_done < recreate_after_theme
    assert "CoreInitCreateThemedStartupWindows(ctx);" in (
        source[load_done:recreate_after_theme]
    )
    assert "static void CoreInitCreateThemedStartupWindows(ViewContext *ctx) {" in source


def test_startup_only_recreates_windows_after_theme_load():
    source = _read("src/core/init.c")
    init_start = source.index("int Init(ViewContext *ctx, const char *configuration_file,")
    init_body = source[init_start:]

    assert init_body.count("CoreInitCreateThemedStartupWindows(ctx);") == 1
    assert init_body.index('DEBUG_LOG("Init: LoadTheme done")') < init_body.index(
        "CoreInitCreateThemedStartupWindows(ctx);"
    )


def test_startup_defers_normal_window_creation_to_themed_helper():
    source = _read("src/core/init.c")
    helper_start = source.index(
        "static void CoreInitCreateThemedStartupWindows(ViewContext *ctx) {"
    )
    helper_end = source.index("\nvoid ShutdownCurses(", helper_start)
    helper_body = source[helper_start:helper_end]

    assert "CoreInitWbkgdSet(ctx, stdscr, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in helper_body
    assert "werase(stdscr);" in helper_body
    assert "ReCreateWindows(ctx);" in helper_body

    init_start = source.index("int Init(ViewContext *ctx, const char *configuration_file,")
    init_body = source[init_start:]

    assert init_body.count("CoreInitCreateThemedStartupWindows(ctx);") == 1
    assert init_body.index('DEBUG_LOG("Init: LoadTheme done")') < init_body.index(
        "CoreInitCreateThemedStartupWindows(ctx);"
    )


def test_f10_bootstrap_defers_starter_files_until_explicit_edit_choice():
    source = _read("src/ui/ui_edit_config.c")
    helper_start = source.index("static int EnsureConfigStarterFile(")
    helper_end = source.index("static int ReloadConfigAndTheme(")
    helper_body = source[helper_start:helper_end]
    ui_open = source[source.index("void UI_OpenConfigProfile(") :]

    assert "static int EnsureConfigStarterFile(" in helper_body
    assert "static int EnsureThemesStarterFile(" in helper_body
    assert "default_profile_template" in helper_body
    assert "default_theme_catalog" in helper_body
    assert 'WriteStarterFile(ctx, profile_path, default_profile_template, "config")' in (
        helper_body
    )
    assert 'WriteStarterFile(ctx, themes_path, default_theme_catalog, "themes")' in (
        helper_body
    )
    assert "mkstemp" not in helper_body
    assert "link(temp_path" not in helper_body
    assert ui_open.index("InputChoiceCommandStrip") < ui_open.index("case 'C':")
    pre_switch = ui_open[: ui_open.index("switch (term) {")]
    assert "EnsureConfigStarterFile" not in pre_switch
    assert "EnsureThemesStarterFile" not in pre_switch


def test_semantic_roles_are_canonical_runtime_color_model():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")

    assert "UI_ROLE_DYNAMIC_TEXT = 1" in defs_source
    assert "UI_ROLE_DYNAMIC_TEXT = CPAIR_FILE" not in defs_source
    assert "enum UI_COLOR_PAIRS" not in defs_source
    assert '{"dynamic_text", UI_ROLE_DYNAMIC_TEXT, 7, 0}' in color_source
    assert "LegacyColorAlias" not in color_source
    assert "legacy_color_aliases" not in color_source
    assert '"FILE_COLOR"' not in color_source
    assert "ApplySemanticRole(ctx, roles[i].name, fg, bg)" in theme_source
    assert "ApplyMigrationRoleShim(ctx, roles[i].name, fg, bg)" not in theme_source


def test_volume_menu_uses_required_theme_command_strip():
    source = _read("src/ui/volume_menu.c")

    assert (
        _command_strip_text(source, "volume_command_strip")
        == "(Up)/(Down) select  (Enter) switch  (Esc) quit  (D)elete"
    )
    _assert_command_strip_uses_full_label_model(
        source, "volume_command_strip", ("select", "switch", "quit", "Delete")
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

    assert "UI_ROLE_HELP" in defs_source
    assert "CPAIR_HELP" not in defs_source
    assert "UI_ROLE_KEYBIND" in defs_source
    assert '{"help", UI_ROLE_HELP, 7, 0}' in color_source
    assert '"help"' in theme_source
    assert "ctx->ctx_menu_window, COLOR_PAIR(UI_ROLE_HELP)" in init_source
    assert "UI_RenderCommandStrip(ctx->ctx_menu_window, y, prefix_width," in display_source
    assert "static const UICommandStripCommand dir_help_disk_mode_0_commands[]" in (
        display_source
    )
    assert "static const UICommandStripCommand file_help_disk_mode_0_commands[]" in (
        display_source
    )
    assert "static const UICommandStripCommand history_help_commands[]" in display_source
    assert "PrintMenuOptions(ctx->ctx_menu_window, i, 0, dir_help" not in display_source
    assert "PrintMenuOptions(ctx->ctx_menu_window, i, 0, file_help" not in display_source
    assert "PrintSpecialString(ctx->ctx_menu_window, y, 0, (char *)strip->prefix," in display_source
    assert "mvwaddstr(ctx->ctx_menu_window, y, 0, strip->prefix);" not in display_source
    assert 'strncmp(strip->prefix, "9-4 ", 4) == 0' in display_source
    assert 'PrintSpecialString(ctx->ctx_menu_window, y, 0, "9-4", UI_ROLE_KEYBIND);' in display_source
    assert 'GetProfileValue)(ctx, "DIR1")' not in display_source
    assert 'GetProfileValue)(ctx, "DIR2")' not in display_source
    assert 'GetProfileValue)(ctx, "FILE1")' not in display_source
    assert 'GetProfileValue)(ctx, "FILE2")' not in display_source
    assert "COLOR_PAIR(CPAIR_HELP) | A_BOLD" not in display_source
    assert "COLOR_PAIR(color) | A_BOLD" not in display_source
    assert '(char *)"History   (P)in/unpin' not in display_source
    assert '{{ "DIR      ", dir_help_disk_mode_0_commands,' in display_source
    assert '{ "COMMANDS ", file_help_disk_mode_1_commands,' in display_source
    assert '"" , history_help_commands' not in display_source
    assert '        "", history_help_commands,' in display_source
    assert "Updated:" not in display_source
    assert "COLOR_PAIR(UI_ROLE_HELP)" in compare_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in compare_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in compare_source


def test_task_sixty_touched_surfaces_use_structured_command_strips():
    app_menu_source = _read("src/ui/application_menu.c")
    compare_source = _read("src/ui/compare_request.c")
    display_source = _read("src/ui/display.c")
    input_line_source = _read("src/ui/input_line.c")
    tagged_source = _read("src/ui/tagged_view.c")
    internal_view_source = _read("src/ui/view_internal.c")

    assert "static const HelpCommandStrip dir_help_nav_builtin[]" in display_source
    assert "static const HelpCommandStrip file_help_nav_builtin[]" in display_source
    assert "static const UICommandStripCommand compare_status_commands[]" in compare_source
    assert "static const UICommandStripCommand compare_basis_commands[]" in compare_source
    assert "static const UICommandStripCommand compare_tag_result_commands[]" in compare_source
    assert "static const UICommandStripCommand compare_scope_commands[]" in compare_source
    assert "static const UICommandStripCommand compare_external_scope_commands[]" in compare_source
    assert "static const UICommandStripCommand compare_help_close_commands[]" in compare_source
    assert "static const UICommandStripCommand applications_menu_commands[]" in app_menu_source
    assert "static const UICommandStripCommand read_string_path_hint_commands[]" in input_line_source
    assert "static const UICommandStripCommand read_string_history_hint_commands[]" in input_line_source
    assert "UI_RenderCommandStrip(ctx->ctx_border_window, ctx->layout.prompt_y, prompt_x," in compare_source
    assert "UI_RenderCommandStrip(" in tagged_source
    assert "tagged_view_message_commands" in tagged_source
    assert "tagged_view_prompt_commands" in tagged_source
    assert "view_edit_prompt_commands" in internal_view_source
    assert "view_readonly_prompt_commands" in internal_view_source
    assert "view_navigation_commands" in internal_view_source
    assert "DisplayBuiltInHelpLine(ctx, 2, nav_strip);" in display_source
    assert "PrintMenuOptions(ctx->ctx_border_window, ctx->layout.status_y, 1," not in compare_source
    assert '"9-4 File "' in display_source
    assert '"9-4 Tree "' in display_source
    assert '"(F1)/(Esc) close help"' not in compare_source
    assert '"(F2) browse  (Up) history  (Enter) OK  (Esc) cancel"' not in input_line_source
    assert '"(Up) history  (Enter) OK  (Esc) cancel"' not in input_line_source
    assert "PrintOptions(stdscr, ctx->layout.message_y, 0," not in tagged_source
    assert "PrintOptions(stdscr, ctx->layout.prompt_y, 0," not in tagged_source
    assert "PrintOptions(stdscr, geom->prompt_y, 0," not in internal_view_source
    assert "PrintOptions(stdscr, geom->status_y, 0," not in internal_view_source


def test_picker_surfaces_use_picker_and_selection_roles():
    app_menu_source = _read("src/ui/application_menu.c")
    completion_source = _read("src/ui/completion_dialog.c")
    history_source = _read("src/ui/history_dialog.c")
    volume_source = _read("src/ui/volume_menu.c")
    render_dir_source = _read("src/ui/render_dir.c")
    display_source = _read("src/ui/display.c")

    assert "WbkgdSet(ctx, ctx->ctx_matches_window, COLOR_PAIR(color));" not in (
        completion_source
    )
    assert "ctx->ctx_matches_window, COLOR_PAIR(UI_ROLE_PICKER)" in completion_source
    assert "ctx->ctx_history_window, COLOR_PAIR(UI_ROLE_PICKER)" in history_source
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in app_menu_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in app_menu_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in app_menu_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in app_menu_source
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in volume_source
    assert "COLOR_PAIR(CPAIR_HST) | A_BOLD" not in volume_source
    assert "win == ctx->ctx_f2_window" in render_dir_source
    assert "color = UI_ROLE_PICKER;" in render_dir_source
    assert "tree_line_color = UI_ROLE_PICKER;" in render_dir_source
    assert "UIOverlayAttrForBase(UI_ROLE_TREE_LINES, UI_ROLE_PICKER)" not in render_dir_source
    assert "waddch(win, (chtype)ch | ((win == ctx->ctx_f2_window) ? 0 : A_BOLD));" in (
        render_dir_source
    )
    assert "waddch(win, (chtype)ch | A_BOLD);" not in render_dir_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in render_dir_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in render_dir_source
    assert "box(ctx->ctx_f2_window, 0, 0);" not in display_source


def test_picker_family_selection_can_fall_back_to_inverse_of_picker_base():
    app_menu_source = _read("src/ui/application_menu.c")
    color_source = _read("src/ui/color.c")
    history_source = _read("src/ui/history_dialog.c")
    completion_source = _read("src/ui/completion_dialog.c")
    volume_source = _read("src/ui/volume_menu.c")

    assert "UISelectionAttrForBase" in color_source
    assert "UIColorForeground(UI_ROLE_SELECTION) == UIColorForeground(base_role)" in (
        color_source
    )
    assert "UIColorBackground(UI_ROLE_SELECTION) == UIColorBackground(base_role)" in (
        color_source
    )
    assert "return COLOR_PAIR(base_role) | A_REVERSE;" in color_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in app_menu_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in history_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in completion_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in volume_source
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in _read("src/ui/render_dir.c")


def test_navigation_help_lists_f9_apps_between_split_and_config():
    display_source = _read("src/ui/display.c")

    assert '{UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL}' in display_source


def test_f9_applications_menu_is_wired_in_tree_and_file_controllers():
    dir_source = _read("src/ui/ctrl_dir.c")
    file_source = _read("src/ui/ctrl_file.c")

    assert "if (ch == KEY_F(9)) {" in dir_source
    assert "UI_OpenApplicationsMenu(ctx);" in dir_source
    assert "if (ch == KEY_F(9)) {" in file_source
    assert "UI_OpenApplicationsMenu(ctx);" in file_source


def test_f2_footer_row_is_cleared_before_redrawing_command_strip():
    source = _read("src/ui/f2_picker.c")

    assert "mvwhline(ctx->ctx_f2_window, win_height - 1, 0, ' ', win_width);" in source
    clear_row = source.index(
        "mvwhline(ctx->ctx_f2_window, win_height - 1, 0, ' ', win_width);"
    )
    render_strip = source.index("UI_RenderCommandStrip(")
    assert clear_row < render_strip


def test_active_file_and_tree_selection_use_selection_role_pairs():
    dir_source = _read("src/ui/render_dir.c")
    file_source = _read("src/ui/render_file.c")

    assert "highlight_color = UI_ROLE_SELECTION;" in dir_source
    assert "highlight_color = UI_ROLE_SELECTION;" in dir_source
    assert "COLOR_PAIR(highlight_color)" in dir_source
    assert "full_line_highlight =" in dir_source
    assert "win != ctx->ctx_f2_window" in dir_source
    assert "inactive_full_line_attr = (hilight && full_line_highlight && !is_active)" in dir_source
    assert "wattron(win, A_BOLD | A_UNDERLINE);" in dir_source

    assert "highlight_color_pair = UI_ROLE_SELECTION;" in file_source
    assert "COLOR_PAIR(highlight_color_pair)" in file_source
    assert "inactive_highlight_attr = A_BOLD | A_UNDERLINE;" in file_source
    assert "if (hilight && !is_active_panel)" in file_source
    assert "A_REVERSE" not in file_source


def test_tree_lines_and_margin_use_dedicated_theme_roles():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")
    dir_source = _read("src/ui/render_dir.c")

    assert "UI_ROLE_TREE_LINES" in defs_source
    assert "UI_ROLE_MARGIN" in defs_source
    assert "CPAIR_TREE_LINES" not in defs_source
    assert "CPAIR_MARGIN" not in defs_source
    assert '{"tree_lines", UI_ROLE_TREE_LINES, 7, 0}' in color_source
    assert '{"margin", UI_ROLE_MARGIN, 7, 0}' in color_source
    assert '"tree_lines"' in theme_source
    assert '"margin"' in theme_source
    assert "margin_color = UI_ROLE_MARGIN;" in dir_source
    assert "tree_line_color = UI_ROLE_TREE_LINES;" in dir_source
    assert "wattrset(win, margin_attr);" in dir_source
    assert "wattrset(win, tree_line_attr);" in dir_source
    assert "GetFileTypeColor" not in dir_source


def test_incremental_jump_uses_slash_without_f12_alias():
    key_source = _read("src/ui/key_engine.c")
    man_source = _read("etc/ytnova.1.md")
    usage_source = _read("docs/USAGE.md")
    spec_source = _read("docs/SPECIFICATION.md")

    assert "case '/':" in key_source
    assert "ACTION_LIST_JUMP" in key_source
    assert "KEY_F(12)" not in key_source
    assert "**/** (or **F12**)" not in man_source
    assert "**/** (or **F12**)" not in usage_source
    assert "**`F12`**: incremental jump" not in spec_source
    assert "Legacy alias for `/`" not in spec_source


def test_disabled_role_is_not_advertised_in_starter_theme_contract():
    starter_theme_source = _read("etc/ytnova.themes")
    spec_source = _read("docs/SPECIFICATION.md")
    roadmap_source = _read("docs/ROADMAP.md")

    assert "disabled =" not in starter_theme_source
    assert "`disabled`" not in spec_source
    assert "`disabled`: inactive or unavailable commands/options." not in roadmap_source


def test_header_path_uses_dynamic_text_role():
    defs_source = _read("include/ytnova_defs.h")
    display_source = _read("src/ui/display.c")

    assert "UI_ROLE_STATIC_TEXT" in defs_source
    assert "DisplayHeaderPath" in display_source
    assert "WbkgdSet(ctx, ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in (
        display_source
    )
    assert "wattrset(ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in (
        display_source
    )
    assert "COLOR_PAIR(UI_ROLE_STATIC_TEXT) | A_BOLD" not in display_source
    assert "COLOR_PAIR(UI_ROLE_STATIC_TEXT)" in display_source


def test_clock_uses_dynamic_text_role():
    init_source = _read("src/core/init.c")
    clock_source = _read("src/core/clock.c")

    assert (
        "ctx->ctx_time_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in init_source
    )
    assert "COLOR_PAIR(CPAIR_WINDIR | A_BOLD)" not in init_source
    assert "wattrset(ctx->ctx_time_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in (
        clock_source
    )
    assert "COLOR_PAIR(UI_ROLE_STATIC_TEXT)" not in clock_source


def test_stats_rendering_splits_static_dynamic_and_border_roles():
    stats_source = _read("src/ui/stats.c")
    theme_source = _read("src/cmd/theme.c")

    assert "static void SetStatsStaticColor" in stats_source
    assert "static void SetStatsDynamicColor" in stats_source
    assert "static void SetStatsBorderColor" in stats_source
    assert "COLOR_PAIR(UI_ROLE_STATIC_TEXT)" in stats_source
    assert "COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in stats_source
    assert "COLOR_PAIR(UI_ROLE_BOX_LINES)" in stats_source
    assert "COLOR_PAIR(color)" not in stats_source
    assert '"static_text"' in theme_source
    assert '"box_lines"' in theme_source
    assert "ApplySemanticRole(ctx, roles[i].name, fg, bg)" in theme_source


def test_viewer_frame_uses_box_lines_only_for_frame_glyphs():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    internal_source = _read("src/ui/view_internal.c")
    tagged_source = _read("src/ui/tagged_view.c")

    assert "#define UI_VIEWER_FRAME_PAIR NUM_UI_COLOR_PAIRS" in defs_source
    assert "ctx->viewer.view, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in internal_source
    assert "ctx->viewer.view, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in tagged_source
    assert "ctx->viewer.border, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in internal_source
    assert "ctx->viewer.border, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in tagged_source
    assert "UIColorForeground(UI_ROLE_BOX_LINES)" in color_source
    assert "UIColorBackground(UI_ROLE_DYNAMIC_TEXT)" in color_source
    assert "init_pair(UI_VIEWER_FRAME_PAIR," in color_source
    assert "wattron(ctx->viewer.border, COLOR_PAIR(UI_VIEWER_FRAME_PAIR));" in (
        internal_source
    )
    assert "wattroff(ctx->viewer.border, COLOR_PAIR(UI_VIEWER_FRAME_PAIR));" in (
        internal_source
    )
    assert "wattron(ctx->viewer.border, COLOR_PAIR(UI_VIEWER_FRAME_PAIR));" in (
        tagged_source
    )
    assert "wattroff(ctx->viewer.border, COLOR_PAIR(UI_VIEWER_FRAME_PAIR));" in (
        tagged_source
    )
    assert "wattron(ctx->viewer.border, COLOR_PAIR(UI_ROLE_BOX_LINES));" not in (
        internal_source
    )
    assert "wattron(ctx->viewer.border, COLOR_PAIR(UI_ROLE_BOX_LINES));" not in (
        tagged_source
    )


def test_viewer_file_headers_use_dynamic_text_role():
    internal_source = _read("src/ui/view_internal.c")
    tagged_source = _read("src/ui/tagged_view.c")

    assert 'Print(stdscr, geom->header_y, 0, "File: ", UI_ROLE_STATIC_TEXT);' in (
        internal_source
    )
    assert "CutPathname(str, file_path, ctx->viewer.wcols - 5), UI_ROLE_DYNAMIC_TEXT" in (
        internal_source
    )
    assert 'Print(stdscr, ctx->layout.header_y, 0, "File: ", UI_ROLE_STATIC_TEXT);' in (
        tagged_source
    )
    assert "CutPathname(clipped_header, header_buf, available), UI_ROLE_DYNAMIC_TEXT" in (
        tagged_source
    )


def test_preview_content_resets_search_hit_to_base_role_pair():
    preview_source = _read("src/ui/view_preview.c")

    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in preview_source
    assert "wattrset(win, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in preview_source
    assert "wattrset(win, COLOR_PAIR(UI_ROLE_SEARCH_HIT));" in preview_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_SEARCH_HIT));" not in preview_source


def test_modal_prompt_keeps_severity_role_pair():
    error_source = _read("src/ui/error.c")

    assert "wattrset(ctx->ctx_error_window, COLOR_PAIR(color_pair));" in error_source
    assert "COLOR_PAIR(color_pair) | A_BOLD" not in error_source
    assert "A_REVERSE | A_BLINK" not in error_source


def test_theme_docs_capture_role_routing_invariants():
    spec_source = _read("docs/SPECIFICATION.md")
    arch_source = _read("docs/ARCHITECTURE.md")

    assert "F1/context help surfaces use the `help` role" in spec_source
    assert (
        "F2, history, completion, and volume selection surfaces use the `picker` role"
        in spec_source
    )
    assert "CPAIR_" not in spec_source
    assert "WINERR_COLOR" not in spec_source
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
    assert "startup and F10 reload commit paths" in arch_source


def test_theme_editor_tracks_active_path_and_bootstraps_xdg_for_defaults():
    defs_source = _read("include/ytnova_defs.h")
    source = _read("src/ui/ui_edit_config.c")
    theme_source = _read("src/cmd/theme.c")
    default_theme_source = _read("src/core/default_theme_catalog.h")

    assert '#define THEME_CONFIG_HOME_PATH ".config/ytnova/themes.conf"' in (
        defs_source
    )
    assert '#define THEME_FILENAME ".ytnova.themes"' in defs_source
    assert "THEME_CONFIG_HOME_PATH" in theme_source
    assert "THEME_FILENAME" in theme_source
    assert "char theme_file_path[PATH_LENGTH + 1];" in defs_source
    assert "SetThemeFilePath(ctx, NULL);" in theme_source
    assert "SetThemeFilePath(ctx, path);" in theme_source
    assert "EnsureConfigHomeDirectory(home) == 0" in source
    assert "THEME_CONFIG_HOME_PATH" in source
    assert "THEME_FILENAME" in source
    assert "ctx->theme_file_path[0] != '\\0'" in source
    assert 'snprintf(themes_path, themes_path_size, "%s", THEME_FILENAME);' in source
    assert "Can't resolve themes file path" in source
    assert "EnsureThemesStarterFile(ctx, themes_path)" in source
    assert "EnsureConfigStarterFiles" not in source
    assert "default_theme_catalog" in source
    assert "WriteStarterFile" in source
    assert "link(temp_path, themes_path)" not in source
    assert "ResolveSeedThemePath" not in theme_source
    assert "SeedConfiguredThemePath" not in theme_source
    assert "ReadCompiledThemeCatalog" in theme_source
    assert '"etc/ytnova.themes"' not in theme_source
    assert '"etc/ytnova.themes"' not in source
    assert "[theme classic-blue]" in default_theme_source


def test_f10_config_and_reload_do_not_require_theme_path_resolution():
    source = _read("src/ui/ui_edit_config.c")
    ui_open = source[source.index("void UI_OpenConfigProfile(") :]
    pre_switch = ui_open[: ui_open.index("switch (term) {")]
    theme_case = ui_open[ui_open.index("case 'T':") : ui_open.index("case 'R':")]

    assert "ResolveThemesPath(ctx, themes_path, sizeof(themes_path))" not in pre_switch
    assert "ResolveThemesPath(ctx, themes_path, sizeof(themes_path))" in theme_case
    assert 'MESSAGE(ctx, "Can\'t resolve themes file path")' in theme_case


def test_f10_surface_uses_required_command_strip_and_enter_default():
    source = _read("src/ui/ui_edit_config.c")
    key_source = _read("src/ui/key_engine.c")
    header_source = _read("include/ytnova_ui.h")

    assert (
        _command_strip_text(source, "config_command_strip")
        == "(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"
    )
    _assert_command_strip_uses_full_label_model(
        source, "config_command_strip", ("Config", "Themes", "Reload", "Quit")
    )
    assert "InputChoiceCommandStrip" in source
    assert "InputChoiceCommandStrip" in key_source
    assert "InputChoiceCommandStrip" in header_source
    assert "UICommandStripPart" not in header_source
    assert "UI_COMMAND_LABEL" not in header_source
    assert "UI_COMMAND_PUNCT" not in header_source
    assert '"(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"' not in source
    assert 'case CR:' in source
    assert 'case LF:' in source
    assert 'case \'C\':' in source
    assert 'case \'T\':' in source
    assert 'case \'R\':' in source


def test_reload_failures_use_status_line_without_success_message():
    source = _read("src/ui/ui_edit_config.c")
    error_source = _read("src/ui/error.c")

    assert "UI_ShowStatusLineError" in source
    assert "Reload failed: can't read config" in source
    assert "Reload failed: malformed config" in source
    assert "Reload failed: can't load theme" in source
    assert "ValidateProfileFile(ctx, profile_path)" in source
    assert "ValidateProfileFile(ctx, profile_path)" in source[
        : source.index("ctx->core_init_ops.read_profile(ctx, profile_path)")
    ]
    assert "PrintMenuOptions(ctx->ctx_menu_window, 2, 0, ctx->status_line_error_text" not in error_source
    assert "ctx->status_line_error_text, UI_ROLE_ERROR" in error_source
    assert "UIColorSnapshot_Create" in source
    assert "UIColorSnapshot_Restore" in source
    assert "Reloaded" not in source
    assert "reload successful" not in source.lower()


def test_f10_reload_owns_canonical_repaint_for_tree_and_file_focus():
    reload_source = _read("src/ui/ui_edit_config.c")
    tree_source = _read("src/ui/ctrl_dir.c")
    file_source = _read("src/ui/ctrl_file_ops.c")

    assert (
        "static int ReloadConfigAndTheme(ViewContext *ctx, DirEntry *dir_entry,"
        in reload_source
    )
    reload_tail = reload_source[
        reload_source.index("ctx->core_init_ops.reinit_color_pairs(ctx);") :
    ]
    assert "ctx->core_init_ops.wbkgd_set(ctx, stdscr," in reload_tail
    assert "werase(stdscr);" in reload_tail
    assert "ReCreateWindows(ctx);" in reload_tail
    assert "RefreshView(ctx, dir_entry);" in reload_tail

    tree_case = tree_source[
        tree_source.index("case ACTION_EDIT_CONFIG:") : tree_source.index(
            "case ACTION_TOGGLE_STATS:"
        )
    ]
    assert "UI_OpenConfigProfile(ctx, dir_entry);" in tree_case
    assert "RefreshView(ctx, dir_entry);" not in tree_case

    file_case = file_source[
        file_source.index("case ACTION_EDIT_CONFIG:") : file_source.index(
            "case ACTION_RESIZE:"
        )
    ]
    assert "UI_OpenConfigProfile(ctx, dir_entry);" in file_case
    assert "RefreshView(ctx, dir_entry);" not in file_case


def test_f10_reload_reapplies_runtime_profile_settings():
    source = _read("src/ui/ui_edit_config.c")
    filter_start = source.index("static int ApplyPanelVisibilityFilterIfAvailable(")
    helper_start = source.index("static int ApplyReloadableProfileSettings(")
    helper_end = source.index("static void RestoreReloadableProfileState(", helper_start)
    filter_body = source[filter_start:helper_start]
    helper_body = source[helper_start:helper_end]
    reload_start = source.index("static int ReloadConfigAndTheme(")
    reload_end = source.index("static void EditConfigProfile(", reload_start)
    reload_body = source[reload_start:reload_end]

    assert "ApplyReloadableProfileSettings(ctx, dir_entry)" in reload_body
    assert "if (panel == NULL)" in filter_body
    assert "if (panel->vol == NULL)" in filter_body
    assert "AppStateSeedPanelVisibilityFilter(panel, hide_dot_files)" in filter_body
    assert "AppStateCommitPanelVisibilityFilter(panel, hide_dot_files)" in filter_body
    assert "AppStateCommitSmallWindowBypass(" in helper_body
    assert "AppStateCommitFullLineHighlight(" in helper_body
    assert "ApplyPanelVisibilityFilterIfAvailable(ctx->left" in helper_body
    assert "ApplyPanelVisibilityFilterIfAvailable(ctx->right" in helper_body
    assert 'ctx->animation_method =' in helper_body
    assert "AppStateCommitRefreshMode(" in helper_body or "ApplyRefreshMode(" in helper_body
