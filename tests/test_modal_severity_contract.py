from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source


def test_modal_message_severity_routes_info_warn_error_pairs():
    error_source = _read_source("src/ui/error.c")

    assert "static short ModalSeverityColorPair(" in error_source, (
        "Modal rendering must route severity through one centralized color-tier mapper."
    )
    assert "CPAIR_INFO" in error_source, "Info-tier modal color must be wired."
    assert "CPAIR_WARN" in error_source, "Warning-tier modal color must be wired."
    assert "CPAIR_ERR" in error_source, "Error-tier modal color must be wired."

    ui_message_block = _extract_function_block(error_source, "int UI_Message(")
    ui_warning_block = _extract_function_block(error_source, "int UI_Warning(")
    ui_error_block = _extract_function_block(error_source, "int UI_Error(")

    assert "MODAL_SEVERITY_INFO" in ui_message_block, (
        "UI_Message must route through informational modal severity."
    )
    assert "MODAL_SEVERITY_WARNING" in ui_warning_block, (
        "UI_Warning must route through warning modal severity."
    )
    assert "MODAL_SEVERITY_ERROR" in ui_error_block, (
        "UI_Error must route through error modal severity."
    )


def test_directory_compare_completion_uses_informational_modal_path():
    compare_source = _read_source("src/ui/dir_compare.c")

    flat_compare_block = _extract_function_block(
        compare_source, "void DirCompare_RunInternalDirectory("
    )
    logged_tree_block = _extract_function_block(
        compare_source, "void DirCompare_RunInternalLoggedTree("
    )

    assert "Directory compare complete." in flat_compare_block
    assert "UI_Message(" in flat_compare_block, (
        "Directory compare completion summary should use the informational modal path."
    )

    assert "Logged-tree compare complete." in logged_tree_block
    assert "UI_Message(" in logged_tree_block, (
        "Logged-tree completion summary should use the informational modal path."
    )


def test_modal_window_background_rebinds_per_severity_tier():
    error_source = _read_source("src/ui/error.c")
    map_modal_block = _extract_function_block(
        error_source,
        "static void MapModalWindow(ViewContext *ctx, char *header, char *prompt,\n"
        "                           ModalSeverity severity) {",
    )

    assert "WbkgdSet(ctx, ctx->ctx_error_window, COLOR_PAIR(color_pair));" in map_modal_block, (
        "Modal rendering must rebind the error window background to the mapped "
        "severity tier before drawing."
    )

    background_rebind = map_modal_block.find(
        "WbkgdSet(ctx, ctx->ctx_error_window, COLOR_PAIR(color_pair));"
    )
    erase_call = map_modal_block.find("werase(ctx->ctx_error_window);")
    assert background_rebind >= 0 and erase_call > background_rebind, (
        "Modal background must be rebound before clearing so refreshed content "
        "uses the selected severity tier."
    )
