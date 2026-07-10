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


def test_compare_help_popup_uses_help_palette():
    compare_source = _read_source("src/ui/compare_request.c")
    popup_block = _extract_function_block(
        compare_source,
        "static void ShowCompareHelpPopup(ViewContext *ctx, CompareHelpTopic topic)",
    )

    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_HELP));" in popup_block
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in popup_block
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in popup_block
    assert "UI_RenderCommandStrip(win, height - 2, 2, compare_help_close_commands," in popup_block
    assert "UI_ROLE_HELP, UI_ROLE_KEYBIND);" in popup_block
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in popup_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in popup_block


def test_prompt_uses_dialog_and_volume_uses_picker_palette():
    input_source = _read_source("src/ui/input_line.c")
    input_block = _extract_function_block(
        input_source,
        "static int UI_ReadStringInternal(ViewContext *ctx, YtreeNovaPanel *panel,",
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DIALOG));" in input_block
    assert "UI_RenderCommandStrip(win, hints_row, 1, hints, hint_count," in input_block
    assert "UI_ROLE_DIALOG, UI_ROLE_KEYBIND);" in input_block
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in input_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in input_block

    volume_source = _read_source("src/ui/volume_menu.c")
    volume_block = _extract_function_block(
        volume_source, "int SelectLoadedVolume(ViewContext *ctx, int *return_key)"
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));" in volume_block
    assert "wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_block
    assert "wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));" in volume_block
    assert "UI_RenderCommandStrip" in volume_block
    assert "COLOR_PAIR(UI_ROLE_SELECTION)" in volume_block
    assert "COLOR_PAIR(UI_ROLE_WARNING)" not in volume_block
    assert "COLOR_PAIR(UI_ROLE_ERROR)" not in volume_block
