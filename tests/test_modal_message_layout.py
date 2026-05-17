from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source


def test_wrapping_prefers_word_boundaries_for_modal_body():
    error_source = _read_source("src/ui/error.c")
    wrap_block = _extract_function_block(
        error_source,
        "static int GetWrappedChunk(const char *segment, int segment_len, int line_start,\n"
        "                           int body_width, int *chunk_offset, int *chunk_len) {",
    )

    assert (
        "while (wrap_end > line_start && !isspace((unsigned char)segment[wrap_end]))"
        in wrap_block
    ), "Wrapping should back up to whitespace before breaking the line."
    assert "while (next_start < segment_len &&" in wrap_block
    assert "isspace((unsigned char)segment[next_start]))" in wrap_block


def test_long_single_word_falls_back_to_safe_hard_break():
    error_source = _read_source("src/ui/error.c")
    wrap_block = _extract_function_block(
        error_source,
        "static int GetWrappedChunk(const char *segment, int segment_len, int line_start,\n"
        "                           int body_width, int *chunk_offset, int *chunk_len) {",
    )

    assert "if (wrap_end == line_start)" in wrap_block
    assert "wrap_end = line_start + body_width;" in wrap_block
    assert "next_start = wrap_end;" in wrap_block


def test_modal_header_and_prompt_center_contract_is_preserved():
    error_source = _read_source("src/ui/error.c")

    map_modal_block = _extract_function_block(
        error_source,
        "static void MapModalWindow(ViewContext *ctx, char *header, char *prompt,\n"
        "                           ModalSeverity severity) {",
    )

    assert "PrintErrorLine(ctx, 1, header);" in map_modal_block
    assert (
        "MvWAddStr(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 2, 1, prompt);"
        in map_modal_block
    )

    display_block = _extract_function_block(
        error_source,
        "static void DisplayMessage(ViewContext *ctx, const char *msg) {",
    )
    assert "center_body = (total_lines == 1);" in display_block
    assert "if (center_body)" in display_block
    assert "PrintErrorLine(ctx, y, buffer);" in display_block
    assert "MvWAddStr(ctx->ctx_error_window, y, 1, buffer);" in display_block
    assert "if (msg[i] == '*' || msg[i] == '\\0')" in display_block

    ui_message_block = _extract_function_block(error_source, "int UI_Message(")
    ui_warning_block = _extract_function_block(error_source, "int UI_Warning(")
    ui_error_block = _extract_function_block(error_source, "int UI_Error(")

    assert '"             PRESS ENTER              "' in ui_message_block
    assert '"             PRESS ENTER              "' in ui_warning_block
    assert '"             PRESS ENTER              "' in ui_error_block
