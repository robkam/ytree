#!/usr/bin/env python3
"""
Manual repro checker for the same-volume inactive-pane drift bug.

Exit codes:
  0: bug not observed
  1: bug observed (inactive pane jumped to tests file view)
  2: setup/navigation failed
"""

import os
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from helpers_ui import drive_action_until
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _stats_current_dir_contains(lines, marker):
    split_x = _detect_stats_split_x(lines)
    for i, line in enumerate(lines):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT DIR" not in segment:
            continue
        for j in (1, 2):
            idx = i + j
            if idx >= len(lines):
                continue
            candidate = lines[idx][split_x:] if split_x is not None else lines[idx]
            if marker in candidate:
                return True
    return False


def _navigate_to_src_cmd_file_mode(tui):
    if not drive_action_until(
        tui, Keys.DOWN,
        lambda lines: lines if _stats_current_dir_contains(lines, "src") else False,
        max_actions=128,
    ):
        return False, f"SETUP_FAIL: could not focus src\n{_screen_text(tui)}"

    tui.send_keystroke(Keys.RIGHT, wait=0.25)

    if not drive_action_until(
        tui, Keys.DOWN,
        lambda lines: lines if _stats_current_dir_contains(lines, "cmd") else False,
        max_actions=128,
    ):
        return False, f"SETUP_FAIL: could not focus src/cmd\n{_screen_text(tui)}"

    tui.send_keystroke(Keys.RIGHT, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.45)
    if "hex invert j compare" not in _footer_text(tui):
        return False, "SETUP_FAIL: not in file mode for src/cmd"
    return True, ""


def _run_sequence_a(tui):
    ok, err = _navigate_to_src_cmd_file_mode(tui)
    if not ok:
        return 2, err, _screen_text(tui)

    tagged = drive_action_until(
        tui, "t" + Keys.DOWN,
        lambda lines: lines if sum("*" in line.split(".c")[0] for line in lines if ".c" in line) >= 3 else False,
        max_actions=3,
    )
    if not tagged:
        return 2, "SETUP_FAIL: could not tag three src files", _screen_text(tui)

    # Original manual sequence: f8 tab enter home m 00
    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke(Keys.HOME, wait=0.35)
    tui.send_keystroke("M", wait=0.2)
    if not tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0):
        return 2, "SETUP_FAIL: MAKE DIRECTORY prompt missing", _screen_text(tui)
    tui.send_keystroke("00" + Keys.ENTER, wait=0.8)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = _screen_text(tui)
    bug = ("test_file_0.py" in screen) or ("/tests" in screen)
    if bug:
        return 1, "BUG_REPRODUCED=1 sequence=A", screen
    return 0, "BUG_REPRODUCED=0 sequence=A", screen


def _run_sequence_b(tui, home):
    # User-observed variant:
    # log <home>, down down right, / s enter, right, down enter, t t t,
    # f8 tab enter home, m 00 enter
    tui.send_keystroke("l", wait=0.2)
    if tui.wait_for_content("LOG", timeout=0.8) or tui.wait_for_content("PATH", timeout=0.8):
        tui.send_keystroke(str(home) + Keys.ENTER, wait=0.7)
    else:
        tui.send_keystroke(Keys.ESC, wait=0.2)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.RIGHT, wait=0.25)
    tui.send_keystroke("/", wait=0.2)
    tui.send_keystroke("s", wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke(Keys.RIGHT, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    if "hex invert j compare" not in _footer_text(tui):
        return 2, "SETUP_FAIL: variant B did not reach file mode", _screen_text(tui)
    if "src_file_0.c" not in _screen_text(tui):
        tui.send_keystroke(Keys.ESC, wait=0.25)
        selected = drive_action_until(
            tui, Keys.UP,
            lambda lines: lines if _stats_current_dir_contains(lines, "cmd") else False,
            max_actions=40,
        ) or drive_action_until(
            tui, Keys.DOWN,
            lambda lines: lines if _stats_current_dir_contains(lines, "cmd") else False,
            max_actions=120,
        )
        if not selected:
            return (
                2,
                "SETUP_FAIL: variant B could not re-focus src/cmd",
                _screen_text(tui),
            )
        tui.send_keystroke(Keys.ENTER, wait=0.45)
        if "src_file_0.c" not in _screen_text(tui):
            return 2, "SETUP_FAIL: variant B did not enter src/cmd file mode", _screen_text(tui)

    tui.send_keystroke("t", wait=0.2)
    tui.send_keystroke("t", wait=0.2)
    tui.send_keystroke("t", wait=0.2)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.35)
    tui.send_keystroke(Keys.HOME, wait=0.35)
    tui.send_keystroke("M", wait=0.2)
    if not tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0):
        return 2, "SETUP_FAIL: variant B missing MAKE DIRECTORY prompt", _screen_text(tui)
    tui.send_keystroke("00" + Keys.ENTER, wait=0.8)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = _screen_text(tui)
    path_line = screen.splitlines()[0] if screen else ""
    repo_root_jump = ("Path: " in path_line and path_line.rstrip().endswith("/ytreenova"))
    root_file_window = "bak.sh" in screen
    jumped_to_tests = ("/tests" in screen) or ("test_file_0.py" in screen)
    bug = jumped_to_tests or (repo_root_jump and root_file_window)
    if bug:
        return 1, "BUG_REPRODUCED=1 sequence=B", screen
    return 0, "BUG_REPRODUCED=0 sequence=B", screen


def main():
    exe = os.environ.get("YTNOVA_REPRO_BIN", "/usr/local/bin/ytnova")

    with TemporaryDirectory(prefix="ytnova-repro-") as td:
        tmp = Path(td)
        home = tmp / "home" / "user"
        repo = home / "ytnova"
        repo.mkdir(parents=True)

        (home / ".ytnova").write_text(
            "[GLOBAL]\n"
            "AUTO_REFRESH=3\n"
            "TREEDEPTH=2\n"
            "FILEMODE=2\n"
            "SMALLWINDOWSKIP=1\n"
            "HIDEDOTFILES=1\n",
            encoding="utf-8",
        )

        for name in ("build", "coverage", "docs", "etc", "include", "infra", "src", "tests"):
            (repo / name).mkdir()
        cmd_dir = repo / "src" / "cmd"
        cmd_dir.mkdir()
        tests_dir = repo / "tests"
        (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")
        for idx in range(3):
            (cmd_dir / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
        for idx in range(4):
            (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

        for run_name, runner in (
            ("A", lambda t: _run_sequence_a(t)),
            ("B", lambda t: _run_sequence_b(t, home)),
        ):
            tui = YtreeNovaTUI(executable=exe, cwd=str(repo), env_extra={"HOME": str(home)})
            if not tui.wait_for_text("src", timeout=2.0):
                print("SETUP_FAIL: fixture tree did not render")
                return 2
            try:
                code, msg, screen = runner(tui)
                print(msg)
                if code == 1:
                    print(screen)
                    return 1
                if code == 2:
                    print(screen)
                else:
                    if screen:
                        print(screen.splitlines()[0])
                        print(_footer_text(tui))
            finally:
                tui.quit()
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
