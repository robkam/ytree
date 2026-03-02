import time
import pytest
from tui_harness import YtreeTUI
from ytree_keys import Keys

def test_screen_wipe_after_error(ytree_binary, tmp_path):
    """
    BUG: After an error message (e.g., bad filter), the UI borders/stats vanish.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(tmp_path))
    time.sleep(1.0)

    # 1. Open Filter (f)
    tui.send_keystroke("f")
    # 2. Clear default '*' (Ctrl-U or backspaces) and enter bad filter
    tui.send_keystroke("\x15") # Ctrl-U
    tui.send_keystroke("-*.nonexistent\r")
    time.sleep(0.5)

    # 3. Expect Error Message (usually blocks or shows briefly)
    # 4. Clear error (Enter/Esc)
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # ASSERTION: The bottom menu footer must survive the redraw
    assert "DIR Attribute" in screen or "COMMANDS" in screen, "Footer menu wiped out after error dialog!"
    # ASSERTION: The borders (VT100 'q' or ASCII '-') must exist
    assert "qqq" in screen or "---" in screen, "Borders wiped out after error dialog!"

def test_l_key_binding(ytree_binary, tmp_path):
    """
    BUG: 'L' key moves cursor down (Vi-keys) instead of opening Log/Volume menu.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(tmp_path))
    time.sleep(1.0)

    # 1. Press 'L' (Shift+l)
    tui.send_keystroke("L")
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # ASSERTION: We should see "Volume" or "Log" or a dialog, NOT just a cursor move.
    # If the Volume Menu opens, it usually lists volumes like "A:" or "/"
    assert "Volume" in screen or "Log" in screen or "Select" in screen, "'L' key did not trigger Volume/Log menu!"

def test_split_screen_garbage(dual_panel_sandbox, ytree_binary):
    """
    BUG: Inactive panel shows '*?^X' garbage when Active panel is used.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    time.sleep(1.0)

    # 1. Split Screen
    tui.send_keystroke(Keys.F8)
    time.sleep(0.5)

    # 2. Tab to Right
    tui.send_keystroke(Keys.TAB)

    # 3. Do some work (Mkdir)
    tui.send_keystroke("M")
    tui.send_keystroke("newdir\r")
    time.sleep(0.5)

    screen = "\n".join(tui.get_screen_dump())

    # ASSERTION: No control characters in output
    assert "*?" not in screen, "Found garbage memory artifacts in UI!"
    assert "^X" not in screen, "Found garbage memory artifacts in UI!"
