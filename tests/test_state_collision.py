import pytest
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

def test_state_collision_cursor_pos(dual_panel_sandbox, ytree_binary):
    """
    Demonstrates state collision for cursor_pos/current_dir_entry in split screen.
    If state is global, moving the cursor in one panel will affect the other.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(dual_panel_sandbox))
    # Constructor already waited for the tree to be ready.

    # Structure:
    # dual_panel_root/
    #   left_dir/
    #   right_dir/

    # 1. Move cursor DOWN twice to 'right_dir'
    #    (root is highlighted first; DOWN goes root -> left_dir -> right_dir)
    import time
    time.sleep(1.0) # Wait for UI to fully settle
    tui.send_keystroke(Keys.DOWN, wait=1.0)
    tui.send_keystroke(Keys.DOWN, wait=1.0)
    tui.send_keystroke(Keys.DOWN, wait=1.0) # just in case one is swallowed!

    # 2. Split Screen (F8) - wait for both panels to appear
    tui.send_keystroke(Keys.F8, wait=0.5)

    # 3. Switch to Right Panel (TAB)
    tui.send_keystroke(Keys.TAB)

    # 4. In right panel, cursor is cloned from left (on right_dir).
    #    Press DOWN to move to a different dir (or just ENTER right_dir from right panel)
    tui.send_keystroke(Keys.ENTER, wait=1.0)  # Enter right_dir in RIGHT panel

    # 5. Exit file window (ESC), return to tree view of right panel
    tui.send_keystroke(Keys.ESC, wait=0.5)

    # 6. Switch back to Left Panel (TAB)
    tui.send_keystroke(Keys.TAB)
    print("\n==== AFTER SWITCH BACK TO LEFT PANEL ====")
    print("\n".join(tui.get_screen_dump()))

    # 7. In left panel, press ENTER.
    # EXPECTATION: Cursor was on 'right_dir', so we should see files '0', '1', '2' etc.
    # REALITY (BUG): If state is global, the cursor was moved to 'left_dir' by Step 4.
    tui.send_keystroke(Keys.ENTER, wait=1.5)

    screen = "\n".join(tui.get_screen_dump())
    print("\n==== FINAL SCREEN ====")
    print(screen)

    # Assertion: If isolation works, ' 0 ' (from right_dir) must be present.
    if " 0 " not in screen:
        pytest.fail(f"STATE COLLISION DETECTED! Expected to see ' 0 '")

    tui.quit()
