from pathlib import Path
import io
import tarfile

from helpers_source import extract_function_block as _extract_function_block
from helpers_source import read_repo_source as _read_source
from helpers_ui import footer_lines, screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())


def _spawn_help_tui(root, env_extra=None):
    return YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root), env_extra=env_extra)


def _root_with_file(tmp_path, name="help_text_contract"):
    root = tmp_path / name
    root.mkdir()
    (root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (root / "beta.txt").write_text("beta\n", encoding="utf-8")
    return root


def _create_tar(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, data in entries.items():
            payload = data.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))


def _popup_frame(screen, title, footer_hint="Esc/Quit"):
    lines = screen.splitlines()
    title_row = next(i for i, line in enumerate(lines) if title in line)
    title_line = lines[title_row]
    title_col = title_line.index(title)
    left = title_line.rfind("x", 0, title_col)
    right = title_line.find("x", title_col + len(title))
    footer_row = next(i for i, line in enumerate(lines) if footer_hint in line)
    bottom_row = next(
        i
        for i, line in enumerate(lines[footer_row:], start=footer_row)
        if len(line) > right and line[left] == "m" and line[right] == "j"
    )
    return {
        "title_row": title_row,
        "footer_row": footer_row,
        "bottom_row": bottom_row,
        "left": left,
        "right": right,
    }


def _enter_archive_from_selected_file(tui):
    tui.send_keystroke(Keys.ENTER, wait=0.5)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)

    for _ in range(6):
        if tui.wait_for_content("Skipped unsafe archive member path", timeout=0.3):
            tui.send_keystroke(Keys.ENTER, wait=0.3)
        else:
            break


def _wait_for_help(tui, title, key=Keys.F1, timeout=1.5):
    screen = tui.send_and_wait_for_condition(
        key,
        lambda lines: lines
        if any(title in line for line in lines)
        else False,
        timeout=timeout,
    )
    assert screen, screen_text(tui)
    return "\n".join(screen)


def _normalized_help_text(screen):
    return " ".join(screen.replace("`", "").split())


def test_vi_file_footer_uses_runtime_vi_keys(tmp_path):
    root = _root_with_file(tmp_path, "vi_footer_runtime_keys")
    (root / ".ytnova").write_text("[GLOBAL]\nVI_KEYS=1\n", encoding="utf-8")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        footer = "\n".join(footer_lines(tui))

        assert "delete" in footer, footer
        assert "Delete" not in footer, footer
    finally:
        tui.quit()


def test_execute_prompt_f1_help_explains_placeholder_and_tagged_repeat(tmp_path):
    root = _root_with_file(tmp_path, "execute_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("x", wait=0.2)

        assert tui.wait_for_content("COMMAND", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "{} where the selected file path should be inserted" in normalized, help_screen
        assert "Ctrl-X" in help_screen, help_screen
        assert "rerun the command for each tagged file" in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("COMMAND", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_search_tagged_prompt_f1_help_explains_tag_scope(tmp_path):
    root = _root_with_file(tmp_path, "search_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke("\x13", wait=0.2)

        assert tui.wait_for_content("SEARCH TAGGED", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "Enter plain search text only." in normalized, help_screen
        assert "ytnova builds grep -i -- PATTERN {} for you." in normalized, help_screen
        assert "Only tagged files are searched, and non-matches are untagged." in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("SEARCH TAGGED", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_filter_prompt_f1_help_uses_generated_runtime_topic(tmp_path):
    root = _root_with_file(tmp_path, "filter_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("f", wait=0.2)

        assert tui.wait_for_content("FILTER:", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        filter_help = _wait_for_help(tui, "Filter Help")
        normalized = _normalized_help_text(filter_help)
        assert "Type a glob such as *.c" in normalized, filter_help
        assert ":r and :x" in normalized, filter_help
        assert ">2023-01-01" in normalized, filter_help
        assert ">1M" in normalized, filter_help

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("FILTER:", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_archive_prompt_f1_help_explains_suffixes_and_selection_scope(tmp_path):
    root = _root_with_file(tmp_path, "archive_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("z", wait=0.2)

        assert tui.wait_for_content("Create archive", timeout=1.0), screen_text(tui)
        prompt_screen = screen_text(tui).lower()
        assert "f1 help" in prompt_screen, prompt_screen

        tui.send_keystroke(Keys.F1, wait=0.3)
        help_screen = screen_text(tui)
        normalized = _normalized_help_text(help_screen)
        assert "Use .tar, .tar.gz or .tgz, .tar.bz2 or .tbz2, .tar.xz or .txz, or .zip." in normalized, help_screen
        assert "the tagged set wins" in normalized, help_screen
        assert "current file or directory selection" in normalized, help_screen

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Create archive", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_main_f1_help_tracks_directory_file_preview_and_split_contexts(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_main_contexts")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen
        assert "Attributes:" in help_screen, help_screen
        assert "A (Attributes):" not in help_screen, help_screen
        assert "Directory commands" not in help_screen, help_screen
        assert "Tree navigation" not in help_screen, help_screen
        assert "Left Arrow:" in help_screen, help_screen
        assert "Directory help explains the live directory footer commands" not in help_screen, help_screen
        footer_line = next(
            line for line in help_screen.splitlines() if "Esc/Quit" in line
        )
        assert "Contents" in footer_line, footer_line
        assert "Navigation" in footer_line, footer_line
        assert "Shared commands" not in footer_line, footer_line
        assert "F8 split" not in footer_line, footer_line
        directory_frame = _popup_frame(help_screen, "Directory Help")
        assert footer_line.index("Contents") - directory_frame["left"] <= 3, footer_line
        assert directory_frame["bottom_row"] == directory_frame["footer_row"] + 1, help_screen
        help_lines = help_screen.splitlines()
        title_gap = help_lines[directory_frame["title_row"] + 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert title_gap.strip() == "", help_screen
        first_help_row = next(
            i for i, line in enumerate(help_lines) if "1..9 view:" in line
        )
        blank_gap = help_lines[first_help_row + 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert blank_gap.strip() == "", help_screen
        footer_gap = help_lines[directory_frame["footer_row"] - 1][
            directory_frame["left"] + 1 : directory_frame["right"]
        ]
        assert footer_gap.strip() == "", help_screen

        tui.send_keystroke("q")
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "F8 Split Help")
        assert "Tab" in help_screen, help_screen
        assert "inactive panel" in help_screen, help_screen
        split_frame = _popup_frame(help_screen, "F8 Split Help")
        split_body = "\n".join(
            line[split_frame["left"] + 1 : split_frame["right"]]
            for line in help_screen.splitlines()[
                split_frame["title_row"] + 1 : split_frame["footer_row"]
            ]
        )
        assert "DIR1..9 dir view" not in split_body, help_screen
        assert "COMMANDS" not in split_body, help_screen
        assert "File F1 help" not in split_body, help_screen
        assert split_frame["title_row"] == directory_frame["title_row"], help_screen
        assert split_frame["footer_row"] == directory_frame["footer_row"], help_screen
        assert split_frame["left"] == directory_frame["left"], help_screen
        assert split_frame["right"] >= directory_frame["right"], help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "File Help")
        assert "File commands" not in help_screen, help_screen
        assert "Left Arrow:" in help_screen, help_screen
        assert "pathcopy" in help_screen.lower(), help_screen
        assert _popup_frame(help_screen, "File Help") == directory_frame, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F7)
        preview_screen = _wait_for_help(tui, "F7 Preview Help")
        preview_frame = _popup_frame(preview_screen, "F7 Preview Help")
        preview_body = "\n".join(
            line[preview_frame["left"] + 1 : preview_frame["right"]]
            for line in preview_screen.splitlines()[
                preview_frame["title_row"] + 1 : preview_frame["footer_row"]
            ]
        )
        assert "PREVIEW" not in preview_body, preview_screen
        assert "COMMANDS" not in preview_body, preview_screen
        assert "Copy:" in preview_screen, preview_screen
        assert "Filter:" in preview_screen, preview_screen
        assert "Ctrl-P and Ctrl-N" in preview_screen, preview_screen
        assert preview_frame == split_frame, preview_screen
        assert "Shift-PgUp and Shift-PgDn" in preview_screen, preview_screen
        assert "F8 split does nothing" in preview_screen, preview_screen
    finally:
        tui.quit()


def test_showall_help_opens_scope_explainer_and_returns(tmp_path):
    root = _root_with_file(tmp_path, "showall_global_help_navigation")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke("s", wait=0.4)

        showall_help = _wait_for_help(tui, "Showall Help")
        assert "Showall lists every file inside the current logged volume only." in showall_help, showall_help
        assert "Return to the previously selected directory." in showall_help, showall_help
        assert "owner directory" in showall_help, showall_help
        assert "Repeating S changes sort" in showall_help, showall_help

        scope_help_screen = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("Scope" in line for line in lines) else False,
            timeout=1.5,
        )
        assert scope_help_screen, screen_text(tui)
        scope_help = "\n".join(scope_help_screen)
        assert "Showall lists every file inside the current logged volume only." in scope_help, scope_help

        showall_again = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Showall Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert showall_again, screen_text(tui)

        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_output_prompt_f1_help_uses_generated_runtime_topics(tmp_path):
    root = _root_with_file(tmp_path, "output_prompt_help")
    tui = _spawn_help_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("w", wait=0.2)

        assert tui.wait_for_content("Format:", timeout=1.0), screen_text(tui)
        format_help = _wait_for_help(tui, "Output Format Help")
        format_normalized = _normalized_help_text(format_help)
        assert "Raw writes content with no extra framing." in format_normalized, format_help
        assert "Page break inserts a separator between files" in format_normalized, format_help
        assert "skips a trailing separator at the end." in format_normalized, format_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Format:", timeout=1.0), screen_text(tui)

        tui.send_keystroke("P", wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        separator_help = _wait_for_help(tui, "Output Separator Help")
        assert "default triple-backtick fence" in _normalized_help_text(separator_help), separator_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)

        assert tui.wait_for_content("Destination:", timeout=1.0), screen_text(tui)
        destination_help = _wait_for_help(tui, "Output Destination Help")
        destination_normalized = _normalized_help_text(destination_help)
        assert "file path or a command line" in destination_normalized, destination_help
        assert "final destination exactly as you want it used" in destination_normalized, destination_help
        assert "cancel and return without writing" in destination_normalized, destination_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Destination:", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_integrated_help_directory_and_file_modes_do_not_crash(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_scope_lifetime")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Directory Help")
        assert "1..9 view:" in help_screen, help_screen
        assert "Attributes:" in help_screen, help_screen
        footer_line = next(
            line for line in help_screen.splitlines() if "Esc/Quit" in line
        )
        assert "Contents" in footer_line, footer_line
        assert "Shared commands" not in footer_line, footer_line
        assert "F8 split" not in footer_line, footer_line

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "File Help")
        assert "pathcopy" in help_screen.lower(), help_screen
    finally:
        tui.quit()


def test_picker_dialog_f1_help_covers_volume_and_applications(tmp_path):
    root = _root_with_file(tmp_path, "integrated_help_picker_dialogs")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        tui.send_keystroke("K")
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        volume_screen = _wait_for_help(tui, "Volume Help")
        volume_normalized = _normalized_help_text(volume_screen)
        assert "Use Up and Down to choose a loaded volume." in volume_normalized, volume_screen
        assert "Use Enter to switch to it." in volume_normalized, volume_screen
        assert "Use D to release it, unless it is the last one." in volume_normalized, volume_screen
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F9)
        assert tui.wait_for_content("Applications", timeout=1.0), screen_text(tui)
        applications_screen = _wait_for_help(tui, "Applications Help")
        normalized = _normalized_help_text(applications_screen)
        assert "Use Enter to accept it." in normalized, applications_screen
        assert "placeholder surface" in normalized, applications_screen
    finally:
        tui.quit()


def test_archive_f1_help_uses_archive_specific_context_titles(tmp_path):
    root = tmp_path / "integrated_help_archive_contexts"
    root.mkdir()
    archive_path = root / "inside.tar"
    _create_tar(archive_path, {"inside_dir/inside.txt": "inside payload"})
    tui = _spawn_help_tui(root)

    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0), screen_text(tui)

        help_screen = _wait_for_help(tui, "Archive Directory Help")
        dir_help = help_screen.lower()
        for label in ("enter:", "left arrow:", "right arrow:", "root:", "exit archive:", "global:", "compare:"):
            assert label in dir_help, help_screen
        assert "archive directory help only covers" not in dir_help, help_screen
        assert "see dir for the normal directory/tree baseline" not in dir_help, help_screen
        assert "jumps to archive root when you are below it" in dir_help, help_screen
        assert "copy" not in dir_help, help_screen
        assert "movedir" not in dir_help, help_screen
        assert "newfile" not in dir_help, help_screen
        assert "write" not in dir_help, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("inside_dir", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("inside.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "Archive File Help")
        file_help = help_screen.lower()
        for label in ("1..9 view:", "copy:", "copy tagged:", "delete:", "filter:", "hex:"):
            assert label in file_help, help_screen
        assert "archive file help only covers" not in file_help, help_screen
        assert "see file for the normal file-mode baseline" not in file_help, help_screen
        assert "through archive-aware extract/copy paths" in file_help, help_screen
        assert "not available in archive file mode" not in file_help, help_screen
    finally:
        tui.quit()


def test_command_preset_help_layers_packaged_selection_under_local_overrides(tmp_path):
    root = _root_with_file(tmp_path, "command_preset_help_layering")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text(
        "preset = de\n"
        "[FILE]\n"
        "C | C | Copy | ACTION_CMD_C |\n",
        encoding="utf-8",
    )
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)

        help_screen = _wait_for_help(tui, "File Help")
        lower_help = help_screen.lower()
        assert "copy" in lower_help, help_screen
        assert "datei loeschen" in lower_help, help_screen
        assert "kopieren" not in lower_help, help_screen
    finally:
        tui.quit()


def test_invalid_command_preset_aborts_startup(tmp_path):
    root = _root_with_file(tmp_path, "invalid_command_preset_startup")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text("preset = missing-preset\n", encoding="utf-8")
    tui = _spawn_help_tui(root)

    try:
        screen = tui.wait_for_condition(
            lambda lines: lines
            if (not tui.child.isalive())
            and any("LoadCommands failed" in line for line in lines)
            else False,
            timeout=2.0,
        )
        assert screen, screen_text(tui)
    finally:
        tui.quit()


def test_integrated_help_source_covers_archive_showall_and_history_surfaces():
    display_source = _read_source("src/ui/display.c")
    integrated_block = _extract_function_block(
        display_source, "int UI_ShowIntegratedHelp(ViewContext *ctx, const DirEntry *dir_entry) {"
    )
    assert "UI_ShowGeneratedContextHelp" in integrated_block
    assert '"overlay.f7-dir"' in integrated_block
    assert '"overlay.f8-dir"' in integrated_block
    assert '"main.global"' in integrated_block
    assert '"Showall/Global File Help"' not in integrated_block
    assert "AppendPopupStripRow" not in integrated_block
    assert "AppendResolvedFooterRows" not in integrated_block

    runtime_help_source = _read_source("src/ui/runtime_help.c")
    assert "generated_help_topics.h" in runtime_help_source
    assert "FindGeneratedTopicByContext" in runtime_help_source

    compare_source = _read_source("src/ui/compare_request.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, spec->context_id, NULL, 0)' in compare_source
    assert "UI_HELP_POPUP_COMMAND_STRIP" not in _extract_function_block(
        compare_source,
        "static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {",
    )

    interactions_source = _read_source("src/ui/interactions.c")
    assert "UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0)" in interactions_source

    history_display_source = _read_source("src/ui/display.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.history", NULL, 0)' in history_display_source

    history_source = _read_source("src/ui/history_dialog.c")
    assert "case KEY_F(1):" in history_source

    volume_source = _read_source("src/ui/volume_menu.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.volume-menu", NULL, 0)' in volume_source

    applications_source = _read_source("src/ui/application_menu.c")
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.applications", NULL, 0)' in applications_source

    f2_source = _read_source("src/ui/f2_picker.c")
    assert "case ACTION_HELP:" in f2_source
    assert 'UI_ShowGeneratedContextHelp(ctx, "dialog.f2-picker", NULL, 0)' in f2_source
