from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source


def test_severity_modals_route_only_info_warn_error_pairs():
    error_source = _read_source("src/ui/error.c")

    severity_mapper = _extract_function_block(
        error_source, "static short ModalSeverityColorPair(ModalSeverity severity) {"
    )
    assert "return UI_ROLE_INFO;" in severity_mapper
    assert "return UI_ROLE_WARNING;" in severity_mapper
    assert "return UI_ROLE_ERROR;" in severity_mapper
    assert "UI_ROLE_DIALOG" not in severity_mapper, (
        "Severity mapper must stay bound to info/warn/error tiers only."
    )


def test_shared_help_popup_uses_help_palette():
    help_source = _read_source("src/ui/help_popup.c")
    popup_block = _extract_function_block(
        help_source,
        "static int ShowHelpPopupInternal(ViewContext *ctx, const char *title,",
    )
    footer_block = _extract_function_block(
        help_source,
        "static void RenderHelpPopupFooter(",
    )

    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_HELP));" in popup_block
    assert "wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));" in popup_block
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));" in popup_block
    assert "RenderHelpPopupFooter(win, height - 2," in popup_block
    assert "FillHelpPopupBlankLine(win, y, start_x, footer_width," in footer_block
    assert "UI_RenderCommandStrip(win, y, start_x, commands, command_count," in footer_block
    assert "UI_ROLE_HELP_FOOTER, UI_ROLE_HELP_KEYBIND" in footer_block
    assert "UI_ROLE_HELP_KEYBIND" in footer_block
    assert "UI_ROLE_HELP_HEADING" in help_source
    assert "UI_ROLE_HELP_TOPIC" in help_source
    assert "UI_ROLE_HELP_ATTENTION" in help_source
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in popup_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in popup_block

    compare_source = _read_source("src/ui/compare_request.c")
    compare_block = _extract_function_block(
        compare_source,
        "static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {",
    )
    assert "UI_ShowGeneratedContextHelp(ctx, spec->context_id, NULL, 0);" in compare_block

    prompt_source = _read_source("src/ui/interactions.c")
    prompt_block = _extract_function_block(
        prompt_source,
        "static void ShowPromptHelpPopup(ViewContext *ctx, PromptHelpTopic topic) {",
    )
    assert "UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0);" in prompt_block


def test_prompt_uses_dialog_and_volume_uses_picker_palette():
    input_source = _read_source("src/ui/input_line.c")
    input_block = _extract_function_block(
        input_source,
        "static int UI_ReadStringInternal(ViewContext *ctx, YtreeNovaPanel *panel,",
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DIALOG));" in input_block
    assert "UI_RenderAdaptiveCommandStrip(win, hints_row, 1, hints, hint_count," in input_block
    assert "UI_ROLE_DIALOG, UI_ROLE_KEYBIND);" in input_block
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in input_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in input_block

    volume_source = _read_source("src/ui/volume_menu.c")
    volume_block = _extract_function_block(
        volume_source, "int SelectLoadedVolume(ViewContext *ctx, int *return_key)"
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_block
    assert "wattron(win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_block
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_block
    assert "COLOR_PAIR(UI_ROLE_BOX_LINES)" not in volume_block
    assert "UI_RenderCommandStrip" in volume_block
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in volume_source
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in volume_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in volume_block

    app_source = _read_source("src/ui/application_menu.c")
    app_block = _extract_function_block(
        app_source, "int UI_OpenApplicationsMenu(ViewContext *ctx) {"
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in app_block
    assert "wattron(win, COLOR_PAIR(UI_ROLE_PICKER));" in app_block
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_PICKER));" in app_block
    assert "COLOR_PAIR(UI_ROLE_BOX_LINES)" not in app_block
    assert "UI_RenderCommandStrip" in app_block
    assert "UISelectionAttrForBase(ctx, UI_ROLE_PICKER)" in app_source
    assert "UI_Dialog_Close(ctx, win);" in app_block
    assert "RefreshView(ctx, GetSelectedDirEntry(ctx, ctx->active->vol));" in app_block
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in app_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in app_block
