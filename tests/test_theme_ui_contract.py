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

    assert _command_strip_text(source, "f2_command_strip") == "(L)og  (<)/(>) Cycle"
    _assert_command_strip_uses_full_label_model(
        source, "f2_command_strip", ("Log", "Cycle")
    )
    assert "UI_RenderCommandStrip" in source
    assert '"[ (L)og (< >) Cycle ]"' not in source


def test_command_strip_key_role_controls_color_styling():
    source = _read("src/ui/display_utils.c")

    assert "key_attr = COLOR_PAIR(hcolor);" in source
    assert "key_attr = COLOR_PAIR(hcolor) | A_BOLD;" not in source
    assert "COLOR_PAIR(color) | A_BOLD" not in source


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


def test_startup_initializes_loaded_theme_before_recreating_windows():
    source = _read("src/core/init.c")
    load_done = source.index('DEBUG_LOG("Init: LoadTheme done")')
    reinit_done = source.index('DEBUG_LOG("Init: ReinitColorPairs done")')
    recreate_after_theme = source.index(
        'DEBUG_LOG("Init: ReCreateWindows after theme done")'
    )

    assert load_done < reinit_done < recreate_after_theme
    assert "CoreInitWbkgdSet(ctx, stdscr, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));" in (
        source[load_done:recreate_after_theme]
    )
    assert "werase(stdscr);" in source[load_done:recreate_after_theme]


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
        == "Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete"
    )
    _assert_command_strip_uses_full_label_model(
        source, "volume_command_strip", ("Select", "Switch", "Quit", "Delete")
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
    assert "lo_color = UI_ROLE_HELP;" in display_source
    assert "PrintMenuOptions(ctx->ctx_menu_window, i, 0," in display_source
    assert "dir_help[ctx->view_mode][i]," in display_source
    assert "file_help[ctx->view_mode][i]," in display_source
    assert "UI_ROLE_HELP, UI_ROLE_KEYBIND" in display_source
    assert "COLOR_PAIR(CPAIR_HELP) | A_BOLD" not in display_source
    assert "COLOR_PAIR(color) | A_BOLD" not in display_source
    assert '(char *)"History   (P)in/unpin' in display_source
    assert "COLOR_PAIR(UI_ROLE_HELP)" in compare_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in compare_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in compare_source


def test_picker_surfaces_use_picker_and_selection_roles():
    completion_source = _read("src/ui/completion_dialog.c")
    volume_source = _read("src/ui/volume_menu.c")
    render_dir_source = _read("src/ui/render_dir.c")
    display_source = _read("src/ui/display.c")

    assert "WbkgdSet(ctx, ctx->ctx_matches_window, COLOR_PAIR(color));" not in (
        completion_source
    )
    assert "ctx->ctx_matches_window, COLOR_PAIR(UI_ROLE_PICKER)" in completion_source
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_source
    assert "COLOR_PAIR(UI_ROLE_SELECTION)" in volume_source
    assert "COLOR_PAIR(CPAIR_HST) | A_BOLD" not in volume_source
    assert "win == ctx->ctx_f2_window" in render_dir_source
    assert "color = UI_ROLE_PICKER;" in render_dir_source
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in render_dir_source
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in render_dir_source
    assert "box(ctx->ctx_f2_window, 0, 0);" not in display_source


def test_active_file_and_tree_selection_use_selection_role_pairs():
    dir_source = _read("src/ui/render_dir.c")
    file_source = _read("src/ui/render_file.c")

    assert "highlight_color = UI_ROLE_SELECTION;" in dir_source
    assert "highlight_color = UI_ROLE_SELECTION;" in dir_source
    assert "COLOR_PAIR(highlight_color)" in dir_source
    assert "inactive_full_line_attr = (hilight && ctx->highlight_full_line && !is_active)" in dir_source
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


def test_disabled_role_projects_to_runtime_pair():
    defs_source = _read("include/ytnova_defs.h")
    color_source = _read("src/ui/color.c")
    theme_source = _read("src/cmd/theme.c")

    assert "UI_ROLE_DISABLED" in defs_source
    assert "CPAIR_DISABLED" not in defs_source
    assert '{"disabled", UI_ROLE_DISABLED, 8, 0}' in color_source
    assert '"disabled"' in theme_source


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


def test_viewer_frame_uses_border_role_not_directory_fill_role():
    internal_source = _read("src/ui/view_internal.c")
    tagged_source = _read("src/ui/tagged_view.c")

    assert "ctx->viewer.view, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in internal_source
    assert "ctx->viewer.view, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT)" in tagged_source
    assert "ctx->viewer.border, COLOR_PAIR(UI_ROLE_BOX_LINES)" in internal_source
    assert "ctx->viewer.border, COLOR_PAIR(UI_ROLE_BOX_LINES)" in tagged_source


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


def test_theme_editor_uses_preferred_path_with_legacy_fallback():
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
    assert "char legacy_path[PATH_LENGTH + 1];" in source
    assert "access(themes_path, F_OK) != 0" in source
    assert "access(legacy_path, F_OK) == 0" in source
    assert "THEME_CONFIG_HOME_PATH" in source
    assert "THEME_FILENAME" in source
    assert 'snprintf(themes_path, themes_path_size, "%s", THEME_FILENAME);' not in source
    assert "Can't resolve themes file path" in source
    assert "EditMissingThemesFromDefault(ctx, dir_entry, themes_path)" in source
    assert "default_theme_catalog" in source
    assert "link(temp_path, themes_path)" in source
    assert "ResolveSeedThemePath" in theme_source
    assert "SeedConfiguredThemePath" in theme_source
    assert "ReadCompiledThemeCatalog" in theme_source
    assert '"etc/ytnova.themes"' not in theme_source
    assert '"etc/ytnova.themes"' not in source
    assert "[theme classic-blue]" in default_theme_source


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
