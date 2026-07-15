from pathlib import Path

from helpers_ui import footer_lines, screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())


def _spawn_task42_tui(root, env_extra=None):
    return YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root), env_extra=env_extra)


def _root_with_file(tmp_path, name="help_text_contract"):
    root = tmp_path / name
    root.mkdir()
    (root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (root / "beta.txt").write_text("beta\n", encoding="utf-8")
    return root


def test_vi_file_footer_uses_runtime_vi_keys(tmp_path):
    root = _root_with_file(tmp_path, "vi_footer_runtime_keys")
    (root / ".ytnova").write_text("[GLOBAL]\nVI_KEYS=1\n", encoding="utf-8")
    tui = _spawn_task42_tui(root)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        footer = "\n".join(footer_lines(tui))

        assert "delete" in footer, footer
        assert "Delete" not in footer, footer
    finally:
        tui.quit()


def test_execute_prompt_f1_help_explains_placeholder_and_tagged_repeat(tmp_path):
    root = _root_with_file(tmp_path, "execute_prompt_help")
    tui = _spawn_task42_tui(root)

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
    tui = _spawn_task42_tui(root)

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


def test_archive_prompt_f1_help_explains_suffixes_and_selection_scope(tmp_path):
    root = _root_with_file(tmp_path, "archive_prompt_help")
    tui = _spawn_task42_tui(root)

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
