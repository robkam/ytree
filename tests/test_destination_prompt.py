from pathlib import Path
import io
import os
import re

import pexpect
import pytest

from tui_harness import YtreeNovaTUI
from ytnova_control import YtreeNovaController
from ytnova_keys import Keys


def _screen_text(lines: list[str]) -> str:
    return "\n".join(lines)


def _expect_missing_destination_prompt(child: pexpect.spawn, destination: Path) -> None:
    child.expect(r"Create missing directory\?", timeout=2.0)


def _cell_style_at(tui: YtreeNovaTUI, needle: str, offset: int = 0):
    for y, line in enumerate(tui.peek_screen_dump()):
        if needle not in line:
            continue
        x = line.index(needle) + offset
        cell = tui.screen.buffer[y][x]
        return (cell.fg, cell.bg, cell.bold, cell.reverse, cell.underscore)
    raise AssertionError(
        f"Could not find {needle!r}.\n{_screen_text(tui.get_screen_dump())}"
    )


def test_copy_name_to_destination_prompt_does_not_redraw_working_view(
    ytnova_binary, tmp_path
):
    root = tmp_path / "copy_prompt_transition_no_flicker"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(
            tui.get_screen_dump()
        )
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_text("alpha.txt", timeout=1.5), _screen_text(
            tui.get_screen_dump()
        )
        tui.child.send(Keys.COPY)
        tui.child.expect("COPY:", timeout=1.5)

        raw_output = io.StringIO()
        tui.child.logfile_read = raw_output
        tui.child.send("alpha-copy.txt\r")
        tui.child.expect("To Directory:", timeout=1.5)
        captured = raw_output.getvalue()

        assert "\x1b[J" not in captured, captured
        assert "COMMANDS " not in captured, captured
        assert "FILE     1..9 file view" not in captured, captured
        assert "Path:" not in captured, captured
        tui.child.send(Keys.ESC)
    finally:
        tui.child.logfile_read = None
        tui.quit()


def test_missing_destination_choice_highlights_default_answer(
    ytnova_binary, tmp_path
):
    root = tmp_path / "missing_destination_choice_highlight"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    destination = root / "new_parent"

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), env_extra={"TERM": "xterm-256color"}
    )
    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(
            tui.get_screen_dump()
        )
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_text("alpha.txt", timeout=1.5), _screen_text(
            tui.get_screen_dump()
        )
        tui.child.send(Keys.COPY)
        tui.child.expect("COPY:", timeout=1.5)
        tui.child.send("alpha-copy.txt\r")
        tui.child.expect("To Directory:", timeout=1.5)
        raw_output = io.StringIO()
        tui.child.logfile_read = raw_output
        tui.child.send(f"{destination}\r")
        assert tui.wait_for_text("Create missing directory?", timeout=1.5), (
            _screen_text(tui.get_screen_dump())
        )
        assert "y/N" in _screen_text(tui.get_screen_dump())
        assert _cell_style_at(tui, "y/N") != _cell_style_at(
            tui, "Create missing directory?"
        )
        assert _cell_style_at(tui, "y/N", 1) == _cell_style_at(tui, "y/N")
        assert _cell_style_at(tui, "y/N", 2) == _cell_style_at(tui, "y/N")
        assert _cell_style_at(tui, "y/N", 3) == _cell_style_at(
            tui, "Create missing directory?"
        )

        captured = raw_output.getvalue()
        assert "(y/N)" not in captured, captured
        tui.child.send(Keys.ESC)
    finally:
        tui.child.logfile_read = None
        tui.quit()


def _start_file_copy(controller: YtreeNovaController, source_name: str,
                     new_name: str, destination: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.COPY)
    controller.child.expect("COPY")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(destination))


def _start_file_move(controller: YtreeNovaController, source_name: str,
                     new_name: str, destination: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.MOVE)
    controller.child.expect("MOVE")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(destination))


def test_existing_destination_conflict_shows_size_time_comparison(
    ytnova_binary, tmp_path
):
    root = tmp_path / "file_copy_existing_destination_conflict"
    root.mkdir()
    source = root / "alpha.txt"
    destination_dir = root / "dest"
    destination_dir.mkdir()
    destination = destination_dir / "alpha.txt"

    source.write_text("source payload grows", encoding="utf-8")
    destination.write_text("old", encoding="utf-8")
    os.utime(source, (1700000000, 1700000000))
    os.utime(destination, (1690000000, 1690000000))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.wait_for_text("alpha.txt", timeout=2.0), _screen_text(
            tui.get_screen_dump()
        )
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_text("alpha.txt", timeout=1.5), _screen_text(
            tui.get_screen_dump()
        )
        tui.child.send(Keys.COPY)
        tui.child.expect("COPY:", timeout=1.5)
        tui.child.send(Keys.ENTER)
        tui.child.expect("To Directory:", timeout=1.5)
        tui.child.send(f"{destination_dir}\r")
        assert tui.wait_for_text("Overwrite", timeout=1.5), _screen_text(
            tui.get_screen_dump()
        )

        screen = _screen_text(tui.get_screen_dump())
        normalized = " ".join(screen.split())
        assert "alpha.txt" in normalized, screen
        assert re.search(
            r"src [0-9]+(?:\.[0-9])?[BKMGTP] \d{4}-\d{2}-\d{2} \d{2}:\d{2}",
            normalized,
            re.IGNORECASE,
        ), screen
        assert re.search(
            r"dst [0-9]+(?:\.[0-9])?[BKMGTP] \d{4}-\d{2}-\d{2} \d{2}:\d{2}",
            normalized,
            re.IGNORECASE,
        ), screen
        assert re.search(
            r"(same size|dst smaller|dst bigger), (same time|dst older|dst newer)",
            normalized.lower(),
        ), screen
        assert "dst older" in normalized.lower(), screen
        assert "dst smaller" in normalized.lower(), screen

        tui.child.send(Keys.ESC)
    finally:
        tui.quit()


def test_file_copy_missing_destination_yes_creates_directory_and_copies(
    ytnova_binary, tmp_path
):
    root = tmp_path / "file_copy_missing_destination_yes"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha payload", encoding="utf-8")
    destination = root / "new_parent"
    copied = destination / "alpha_copy.txt"

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _start_file_copy(controller, "alpha.txt", "alpha_copy.txt", destination)

        _expect_missing_destination_prompt(controller.child, destination)

        controller.child.send(Keys.CONFIRM_YES)
        created = controller.wait_for_condition(
            lambda _lines: copied if copied.exists() else False,
            timeout=2.0,
        )
        assert created, f"Copy did not finish after creating destination.\n{_screen_text(controller.get_screen_dump())}"
    finally:
        controller.quit()

    assert destination.is_dir()
    assert copied.read_text(encoding="utf-8") == "alpha payload"


@pytest.mark.parametrize("decline_key", [Keys.CONFIRM_NO, Keys.ESC])
def test_file_move_missing_destination_decline_reopens_directory_prompt(
    ytnova_binary, tmp_path, decline_key
):
    root = tmp_path / "file_move_missing_destination_decline"
    root.mkdir()
    (root / "beta.txt").write_text("beta payload", encoding="utf-8")
    destination = root / "declined_parent"

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _start_file_move(controller, "beta.txt", "beta_moved.txt", destination)

        _expect_missing_destination_prompt(controller.child, destination)

        controller.child.send(decline_key)
        controller.child.expect("To Directory:", timeout=1.5)

        controller.child.send(Keys.ESC)
    finally:
        controller.quit()

    assert (root / "beta.txt").exists()
    assert not destination.exists()


def test_file_move_missing_destination_creation_failure_reports_error_and_aborts(
    ytnova_binary, tmp_path
):
    root = tmp_path / "file_move_missing_destination_failure"
    root.mkdir()
    (root / "gamma.txt").write_text("gamma payload", encoding="utf-8")
    locked_parent = root / "locked"
    locked_parent.mkdir()
    locked_parent.chmod(0o555)
    destination = locked_parent / "new_parent"

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _start_file_move(controller, "gamma.txt", "gamma_moved.txt", destination)

        _expect_missing_destination_prompt(controller.child, destination)

        controller.child.send(Keys.CONFIRM_YES)
        controller.child.expect("Can't create destination directory", timeout=2.0)
        controller.child.expect("Permission denied", timeout=2.0)
        controller.child.send(Keys.ENTER)
        controller.wait_for_refresh()
    finally:
        try:
            locked_parent.chmod(0o755)
        finally:
            controller.quit()

    assert (root / "gamma.txt").exists()
    assert not destination.exists()


def test_directory_copy_missing_destination_yes_creates_directory_before_copy(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_copy_missing_destination_yes"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("payload", encoding="utf-8")
    destination = root / "new_parent"
    copied = destination / "copied_src" / "nested" / "payload.txt"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.wait_for_text("src_dir", timeout=2.0), _screen_text(tui.get_screen_dump())
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=1.0)

        lines = tui.send_and_wait_for_condition(
            "c",
            lambda screen: screen if "COPY:" in _screen_text(screen) else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui.get_screen_dump())

        tui.child.send("\x15copied_src\r")
        assert tui.wait_for_text("To Directory", timeout=1.5), _screen_text(tui.get_screen_dump())
        tui.child.send("\x15./new_parent\r")

        _expect_missing_destination_prompt(tui.child, destination)

        tui.child.send("y")

        created = tui.wait_for_condition(
            lambda _screen: copied if copied.exists() else False,
            timeout=2.0,
        )
        assert created, _screen_text(tui.get_screen_dump())
        assert "Copy directory now" not in _screen_text(tui.get_screen_dump())
    finally:
        tui.quit()

    assert copied.read_text(encoding="utf-8") == "payload"
