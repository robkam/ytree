import time
import os
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

YTNOVA_BIN = os.path.abspath("./build/ytnova")

def debug_run():
    # Make a dummy file to copy
    os.system("touch /tmp/dummy_test_file.txt")
    
    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd="/tmp")
    assert tui.wait_for_text("dummy_test_file.txt", timeout=2.0)
    
    # Enter file view
    assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=2.0)
    
    # Send 'c' (Copy)
    print("\nSending 'c' (Copy)...")
    assert tui.send_and_wait_for_screen_change('c', timeout=2.0)
    
    # First prompt: "COPY: dummy_test_file.txt" (asking for filename)
    # We press Enter to accept default
    print("\nSending Enter...")
    assert tui.send_and_wait_for_screen_change('\r', timeout=2.0)
    
    # Second prompt: "To Directory:" (this takes F2!)
    screen = tui.get_screen_dump()
    print("SECOND PROMPT SCREEN:")
    for line in screen[-8:]:
        print(repr(line))
        
    print("\nSending F2 (\\033OQ)...")
    assert tui.send_and_wait_for_screen_change("\033OQ", timeout=2.0)
    
    screen = tui.get_screen_dump()
    print("F2 SCREEN:")
    for line in screen[-20:]:
        print(repr(line))
        
    tui.quit()

if __name__ == "__main__":
    debug_run()
