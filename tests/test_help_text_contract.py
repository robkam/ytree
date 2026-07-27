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
        assert "{} expands to the selected file path." in help_screen, help_screen
        assert "^X reruns the command for each tagged file." in help_screen, help_screen

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
        assert "Enter text only; ytnova runs grep -i -- PATTERN {}." in help_screen, help_screen
        assert "Only tagged files are searched, and non-matches are untagged." in help_screen, help_screen

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
        assert "Use normal glob-like patterns such as `*.c`" in filter_help, filter_help
        assert "Extended selectors such as `:r`, `:x`, `>2023-01-01`, and `>1M`" in filter_help, filter_help

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
        assert "Use .tar, .tar.gz/.tgz, .tar.bz2/.tbz2, .tar.xz/.txz, or .zip." in help_screen, help_screen
        assert "Tagged files win; otherwise ytnova archives the current selection." in help_screen, help_screen

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
        assert "A (Attributes):" in help_screen, help_screen
        assert "Directory help explains the live directory footer commands" not in help_screen, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "F8 Split Help")
        assert "Tab" in help_screen, help_screen
        assert "inactive panel" in help_screen, help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.F8)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        help_screen = _wait_for_help(tui, "File Help")
        assert "pathcopy" in help_screen.lower(), help_screen

        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F7)
        preview_screen = _wait_for_help(tui, "F7 Preview Help")
        assert "^P/^N" in preview_screen, preview_screen
        assert "Shift+PgUp/PgDn" in preview_screen, preview_screen
    finally:
        tui.quit()


def test_showall_help_links_to_global_help_via_generated_footer_navigation(tmp_path):
    root = _root_with_file(tmp_path, "showall_global_help_navigation")
    tui = _spawn_help_tui(root)

    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke("s", wait=0.4)

        showall_help = _wait_for_help(tui, "Showall Help")
        assert "single-volume aggregated file view" in showall_help, showall_help
        assert "Press `Esc` to return to the previously selected directory." in showall_help, showall_help

        navigation_help_screen = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("Navigation" in line for line in lines) else False,
            timeout=1.5,
        )
        assert navigation_help_screen, screen_text(tui)
        navigation_help = "\n".join(navigation_help_screen)
        assert "Arrow keys, paging keys, `Home`, `End`, and `Enter`" in navigation_help

        showall_again = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Showall Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert showall_again, screen_text(tui)

        navigation_again = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("Navigation" in line for line in lines) else False,
            timeout=1.5,
        )
        assert navigation_again, screen_text(tui)

        showall_again = tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any("Showall Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert showall_again, screen_text(tui)

        global_help_screen = tui.send_and_wait_for_condition(
            "g",
            lambda lines: lines if any("Global Help" in line for line in lines) else False,
            timeout=1.5,
        )
        assert global_help_screen, screen_text(tui)
        global_help = "\n".join(global_help_screen)
        assert "multi-volume aggregated file view" in global_help, global_help
        assert "different logged volume root" in global_help, global_help

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
        assert "Raw writes content without frame headings." in format_help, format_help
        assert "Page break inserts a separator" in format_help, format_help
        assert "between successive files" in format_help, format_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Format:", timeout=1.0), screen_text(tui)

        tui.send_keystroke("P", wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        separator_help = _wait_for_help(tui, "Output Separator Help")
        assert "accept the default triple-backtick fence" in separator_help, separator_help
        tui.send_keystroke(Keys.ESC, wait=0.2)
        assert tui.wait_for_content("Page break separator", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)

        assert tui.wait_for_content("Destination:", timeout=1.0), screen_text(tui)
        destination_help = _wait_for_help(tui, "Output Destination Help")
        assert "goes to a file path" in destination_help, destination_help
        assert "external" in destination_help, destination_help
        assert "command." in destination_help, destination_help
        assert "final target" in destination_help, destination_help
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
        assert "A (Attributes):" in help_screen, help_screen

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
        assert "preserves its current in-memory state" in volume_screen, volume_screen
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("Select Volume", timeout=1.0), screen_text(tui)
        tui.send_keystroke(Keys.ESC)
        assert tui.wait_for_content("alpha.txt", timeout=1.0), screen_text(tui)

        tui.send_keystroke(Keys.F9)
        assert tui.wait_for_content("Applications", timeout=1.0), screen_text(tui)
        applications_screen = _wait_for_help(tui, "Applications Help")
        assert "Enter closes the menu today." in applications_screen, applications_screen
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
        for label in (
            "delete",
            "filter",
            "global",
            "compare",
            "volume",
            "log",
            "makedir",
            "pipe",
            "quit",
            "rename",
            "showall",
            "tag",
            "untag",
        ):
            assert label in dir_help, help_screen
        assert "root" in dir_help or "exit" in dir_help, help_screen
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
        for label in (
            "copy",
            "delete",
            "filter",
            "hex",
            "invert",
            "compare",
            "volume",
            "log",
            "move",
            "quit",
            "rename",
            "sort",
            "tag",
            "untag",
            "view",
            "pipe",
            "pathcopy",
        ):
            assert label in file_help, help_screen
        assert "execute" not in file_help, help_screen
        assert "write" not in file_help, help_screen
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
        assert "verschieben" in lower_help, help_screen
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
    assert "split_help_commands" in display_source

    runtime_help_source = _read_source("src/ui/runtime_help.c")
    assert "generated_help_topics.h" in runtime_help_source
    assert "FindGeneratedTopicByContext" in runtime_help_source

    history_source = _read_source("src/ui/history_dialog.c")
    assert "case KEY_F(1):" in history_source

    volume_source = _read_source("src/ui/volume_menu.c")
    assert "ShowVolumeHelpPopup" in volume_source

    applications_source = _read_source("src/ui/application_menu.c")
    assert "ShowApplicationsHelpPopup" in applications_source
