from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source


def test_severity_modals_route_only_info_warn_error_pairs():
    error_source = _read_source("src/ui/error.c")

    severity_mapper = _extract_function_block(
        error_source, "static short ModalSeverityColorPair(ModalSeverity severity) {"
    )
    assert "return CPAIR_INFO;" in severity_mapper
    assert "return CPAIR_WARN;" in severity_mapper
    assert "return CPAIR_ERR;" in severity_mapper
    assert "CPAIR_DIALOG" not in severity_mapper, (
        "Severity mapper must stay bound to info/warn/error tiers only."
    )


def test_compare_help_popup_uses_neutral_dialog_palette():
    compare_source = _read_source("src/ui/compare_request.c")
    popup_block = _extract_function_block(
        compare_source,
        "static void ShowCompareHelpPopup(ViewContext *ctx, CompareHelpTopic topic)",
    )

    assert "WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_DIALOG));" in popup_block
    assert (
        "PrintMenuOptions(win, height - 2, 2, (char *)close_prompt, CPAIR_DIALOG,"
        in popup_block
    )
    assert "COLOR_PAIR(CPAIR_WARN)" not in popup_block
    assert "COLOR_PAIR(CPAIR_ERR)" not in popup_block


def test_prompt_and_volume_dialogs_use_neutral_dialog_palette():
    input_source = _read_source("src/ui/input_line.c")
    input_block = _extract_function_block(
        input_source,
        "static int UI_ReadStringInternal(ViewContext *ctx, YtreeNovaPanel *panel,",
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_DIALOG));" in input_block
    assert "PrintMenuOptions(win, hints_row, 1, (char *)hints, CPAIR_DIALOG," in input_block
    assert "COLOR_PAIR(CPAIR_WARN)" not in input_block
    assert "COLOR_PAIR(CPAIR_ERR)" not in input_block

    volume_source = _read_source("src/ui/volume_menu.c")
    volume_block = _extract_function_block(
        volume_source, "int SelectLoadedVolume(ViewContext *ctx, int *return_key)"
    )
    assert "WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_DIALOG));" in volume_block
    assert "COLOR_PAIR(CPAIR_WARN)" not in volume_block
    assert "COLOR_PAIR(CPAIR_ERR)" not in volume_block
