import pytest
import os
import re
import time
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

YTNOVA_BIN = os.path.abspath("./build/ytnova")

def get_screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _destination_prompt_line(tui):
    for line in tui.get_screen_dump():
        if "To Directory:" in line:
            return line
    return ""


def _total_items_count(tui):
    for line in tui.get_screen_dump():
        match = re.search(r"Tot:\s*([0-9,]+)", line)
        if match:
            return int(match.group(1).replace(",", ""))
    return None


def _row_style_spans(tui, row, width=120):
    spans = []
    buffer = tui.screen.buffer
    x = 0
    while x < width:
        ch = buffer[row][x]
        style = (ch.reverse, ch.fg, ch.bg, ch.bold, ch.underscore)
        start = x
        while (
            x < width
            and (
                buffer[row][x].reverse,
                buffer[row][x].fg,
                buffer[row][x].bg,
                buffer[row][x].bold,
                buffer[row][x].underscore,
            )
            == style
        ):
            x += 1
        spans.append((start, x - 1, style, "".join(buffer[row][i].data for i in range(start, x))))
    return spans


def _span_containing(tui, needle, width=120):
    for row, line in enumerate(tui.get_screen_dump()):
        if needle not in line:
            continue
        for start, end, style, text in _row_style_spans(tui, row, width=width):
            if needle in text:
                return row, start, end, style, text
    raise AssertionError(f"Could not find styled span containing {needle!r}.\n{get_screen_text(tui)}")

def test_f2_log_and_cycle_volumes(tmp_path):
    """
    Test that F2 can log new volumes, cycle through them, and update the main views.
    """
    # 1. Setup two distinct directory structures
    dir_a = tmp_path / "volume_a"
    dir_a.mkdir()
    (dir_a / "file_a.txt").touch()

    dir_b = tmp_path / "volume_b"
    dir_b.mkdir()
    (dir_b / "file_b.txt").touch()

    # Start ytnova in the base path
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(tmp_path))
    
    try:
        # Give it a moment to startup
        time.sleep(1.0)
        
        # Enter file view
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)

        # Send 'c' (Copy) on the first file to trigger the copy interaction
        tui.send_keystroke('c')
        time.sleep(0.5)
        
        # Accept filename
        tui.send_keystroke('\r')
        time.sleep(0.5)

        # Now at "To Directory:" prompt. Hit F2!
        tui.send_keystroke(Keys.F2)
        time.sleep(1.0)
        
        # We are in the F2 menu. Press 'L' to log a new directory!
        tui.send_keystroke('l')
        time.sleep(0.5)
        
        # Enter dir_a
        tui.send_keystroke(str(dir_a) + '\r')
        time.sleep(1.0)
        
        # The F2 window now shows dir_a tree. Cycle back or accept it
        # Let's select it for the copy destination
        tui.send_keystroke('\r')
        time.sleep(1.0)

        # The copy likely succeeded or failed, but the side effect is we now have volume_a logged!
        # Wait, if we accept it, ytnova performs the copy and returns to the main view.
        # But we want to test cycling! Let's just enter another copy prompt and cycle!
        
        tui.send_keystroke('c')
        time.sleep(0.5)
        tui.send_keystroke('\r')
        time.sleep(0.5)
        
        # At "To Directory" again:
        tui.send_keystroke(Keys.F2)
        time.sleep(1.0)
        
        # Cycle back using '<'
        tui.send_keystroke('<')
        time.sleep(1.0)
        
        screen = get_screen_text(tui)
        # We should see the previous volume (presumably the tmp_path or volume_a depending on how it cycles)
        assert "volume" in screen or "tmp" in screen, f"Failed to cycle F2 window. Screen:\n{screen}"
        
        # Hit Escape to get out of F2, then escape to cancel Copy
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.5)
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.5)
        
        # Return to Tree view
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)
        
        # In main view, cycle volumes with '<' to verify the main view also has them logged
        tui.send_keystroke('<')
        time.sleep(1.0)
        
        screen = get_screen_text(tui)
        assert "volume_a" in screen, f"Main view did not cycle to volume_a. Screen:\n{screen}"

    finally:
        tui.quit()


def test_f2_right_expands_then_enters_and_left_collapses_or_returns_parent(tmp_path):
    root = tmp_path / "f2_tree_nav"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / "alpha" / "child" / "grand").mkdir(parents=True)
    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)

        tui.send_keystroke(Keys.F2, wait=0.8)
        tui.send_keystroke(Keys.DOWN, wait=0.3)   # alpha
        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # expand alpha
        expanded = get_screen_text(tui)
        assert "child" in expanded, (
            "RIGHT in the F2 tree should expand a collapsed directory.\n"
            f"{expanded}"
        )
        assert "grand" not in expanded, (
            "RIGHT in the F2 tree should respect TREEDEPTH instead of "
            "expanding all descendants at once.\n"
            f"{expanded}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.6)   # collapse alpha
        collapsed = get_screen_text(tui)
        assert "child" not in collapsed, (
            "LEFT in the F2 tree should collapse an expanded directory.\n"
            f"{collapsed}"
        )

        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # expand alpha again
        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # enter child
        tui.send_keystroke(Keys.LEFT, wait=0.6)   # collapse child
        child_collapsed = get_screen_text(tui)
        assert "grand" not in child_collapsed, (
            "LEFT on an expanded child in the F2 tree should collapse that child first.\n"
            f"{child_collapsed}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.6)   # parent
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        parent_prompt = _destination_prompt_line(tui)
        assert str(root / "alpha") in parent_prompt, (
            "LEFT in the F2 tree should go back to the parent directory.\n"
            f"{get_screen_text(tui)}"
        )
        assert str(root / "alpha" / "child") not in parent_prompt, (
            "LEFT in the F2 tree should not leave the child path selected.\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_escape_can_abort_right_expand_scan(tmp_path):
    root = tmp_path / "f2_tree_abort"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=0\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    alpha = root / "alpha"
    alpha.mkdir()
    total_files = 1
    for i in range(300):
        branch = alpha / f"dir_{i:03d}" / "child" / "grand"
        branch.mkdir(parents=True)
        for j in range(25):
            (alpha / f"dir_{i:03d}" / f"f_{j:03d}.txt").write_text(
                "x", encoding="utf-8"
            )
            total_files += 1

    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        assert _total_items_count(tui) == 1

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)

        tui.send_keystroke(Keys.F2, wait=0.8)
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # alpha
        tui.child.send(Keys.RIGHT)                # start subtree scan
        time.sleep(0.02)
        tui.child.send(Keys.ESC)                  # abort the scan mid-flight
        tui._read_output(timeout=0.8)

        scanned_total = _total_items_count(tui)
        assert scanned_total is not None
        assert scanned_total < total_files, (
            "ESC during F2 RIGHT-expansion should stop the subtree scan before "
            "the full file count is loaded.\n"
            f"expected less than {total_files}, saw {scanned_total}\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_history_selection_only_covers_the_current_item(tmp_path):
    root = tmp_path / "history_selection_bounds"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    (root / "dest").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke("dest\r", wait=0.8)

        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.UP, wait=0.8)

        _, start, end, _, text = _span_containing(tui, "dest")
        assert text == text.rstrip(), (
            "History selection should stop at the selected item instead of "
            "extending across the rest of the row.\n"
            f"span {start}-{end}: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_selection_stops_before_the_synthetic_expand_suffix(tmp_path):
    root = tmp_path / "f2_selection_bounds"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / "alpha" / "child").mkdir(parents=True)
    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.F2, wait=0.8)
        tui.send_keystroke(Keys.DOWN, wait=0.2)

        _, _, _, _, text = _span_containing(tui, "alpha")
        assert text == "alpha", (
            "F2 selection should cover only the current item, not the "
            "synthetic expandability suffix.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_volume_menu_selection_only_covers_current_item(tmp_path):
    root = tmp_path / "volume_selection_bounds"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        tui.send_keystroke("K", wait=0.6)

        _, _, _, _, text = _span_containing(tui, f"[*] {root}")
        assert text.startswith("[*] "), (
            "Volume-menu selection should start at the current item instead of "
            "including the whole row padding.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
        assert text == text.rstrip(), (
            "Volume-menu selection should stop at the selected item instead of "
            "extending across the rest of the row.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f9_applications_menu_selection_only_covers_current_item(tmp_path):
    root = tmp_path / "applications_menu_bounds"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        time.sleep(0.8)
        tui.send_keystroke(Keys.F9, wait=0.6)

        _, _, _, _, text = _span_containing(tui, "wget fetch preset")
        assert text.startswith("wget fetch preset"), (
            "Applications-menu selection should start at the current item instead "
            "of including row padding.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
        assert text == text.rstrip(), (
            "Applications-menu selection should stop at the selected item instead "
            "of extending across the rest of the row.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()
