#!/usr/bin/env python3
"""
Automation-only repro for the real HOME workflow reported by user.

This drives /usr/local/bin/ytree against the caller's real HOME tree:
  ytree ~
  -> ytree/src/cmd file mode
  -> tag three files (t,down x3)
  -> F8 TAB ENTER HOME mkdir a temporary unique directory
  -> assert source pane still shows file list and unchanged tag state after TAB back

Exit codes:
  0: no bug detected
  1: bug detected
  2: navigation/setup failure
"""

import os
import re
import shutil
import time
from pathlib import Path

from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_text as _footer_text
from helpers_ui import line_marks_file_as_tagged as _line_marks_file_as_tagged
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


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


def _find_line(screen, needle):
    for line in screen.splitlines():
        if needle in line:
            return line
    return None


def _discover_track_names(screen):
    names = []
    for line in screen.splitlines():
        if line.count(".c") == 0:
            continue
        for m in re.finditer(r"([A-Za-z0-9_][A-Za-z0-9_.-]*\.c)\b", line):
            name = m.group(1)
            if name not in names:
                names.append(name)
            if len(names) >= 3:
                return names
    return names


def _goto_marker(tui, marker, steps=300):
    for _ in range(steps):
        if _stats_current_dir_contains(tui.get_screen_dump(), marker):
            return True
        tui.send_keystroke(Keys.DOWN, wait=0.08)
    return False


def main():
    home = Path(os.environ.get("HOME", ""))
    if not home.exists():
        print("SETUP_FAIL: HOME missing")
        return 2

    exe = os.environ.get("YTREE_REPRO_BIN", "/usr/local/bin/ytree")
    if not Path(exe).exists():
        print(f"SETUP_FAIL: binary missing: {exe}")
        return 2

    mkdir_name = f"ytree-repro-mkdir-{os.getpid()}"
    created_paths = [
        home / mkdir_name,
        home / "ytree" / "src" / "cmd" / mkdir_name,
    ]
    tui = YtreeTUI(executable=exe, cwd=str(home), env_extra={"HOME": str(home)})
    time.sleep(0.9)
    try:
        if not _goto_marker(tui, "ytree"):
            print("SETUP_FAIL: ytree dir not found in HOME listing")
            print(_screen_text(tui))
            return 2
        tui.send_keystroke(Keys.RIGHT, wait=0.25)
        if not _goto_marker(tui, "src"):
            print("SETUP_FAIL: src not found under ytree")
            print(_screen_text(tui))
            return 2
        tui.send_keystroke(Keys.RIGHT, wait=0.25)
        if not _goto_marker(tui, "cmd"):
            print("SETUP_FAIL: cmd not found under src")
            print(_screen_text(tui))
            return 2

        tui.send_keystroke(Keys.RIGHT, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.45)
        if "hex invert j compare" not in _footer_text(tui):
            print("SETUP_FAIL: not in file mode for src/cmd")
            print(_screen_text(tui))
            return 2

        for _ in range(3):
            tui.send_keystroke("t", wait=0.2)

        before = _screen_text(tui)
        tracked = {}
        track_names = _discover_track_names(before)
        for name in track_names:
            line = _find_line(before, name)
            if line is not None:
                tracked[name] = _line_marks_file_as_tagged(line, name)
        if not tracked:
            print("SETUP_FAIL: could not identify tracked src files")
            print(before)
            return 2
        if not any(tracked.values()):
            print("SETUP_FAIL: no tagged src files before split flow")
            print(before)
            return 2

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        if not tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0):
            print("SETUP_FAIL: MAKE DIRECTORY prompt missing")
            print(_screen_text(tui))
            return 2
        tui.send_keystroke(mkdir_name + Keys.ENTER, wait=0.8)
        tui.send_keystroke(Keys.TAB, wait=0.5)

        after = _screen_text(tui)
        if "No files" in after:
            print("BUG_REPRODUCED=1 reason=source_no_files_after_tab")
            print(after)
            return 1
        if "hex invert j compare" not in _footer_text(tui):
            print("BUG_REPRODUCED=1 reason=not_in_file_mode_after_tab")
            print(after)
            return 1

        for name, tagged_before in tracked.items():
            line = _find_line(after, name)
            if line is None:
                print(f"BUG_REPRODUCED=1 reason=missing_row_{name}")
                print(after)
                return 1
            if _line_marks_file_as_tagged(line, name) != tagged_before:
                print(f"BUG_REPRODUCED=1 reason=tag_state_changed_{name}")
                print(after)
                return 1

        print("BUG_REPRODUCED=0")
        return 0
    finally:
        tui.quit()
        for path in created_paths:
            if path.is_dir():
                shutil.rmtree(path)


if __name__ == "__main__":
    raise SystemExit(main())
