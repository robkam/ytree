import time
import os
from tui_harness import YtreeTUI
from ytree_keys import Keys

YTREE_BIN = os.path.abspath("./build/ytree")

def debug_run():
    # Make a dummy file to copy
    os.system("touch /tmp/dummy_test_file.txt")
    
    tui = YtreeTUI(executable=YTREE_BIN, cwd="/tmp")
    time.sleep(1.0)
    
    # Enter file view
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.5)
    
    # Send 'c' (Copy)
    print("\nSending 'c' (Copy)...")
    tui.send_keystroke('c')
    time.sleep(0.5)
    
    # First prompt: "COPY: dummy_test_file.txt" (asking for filename)
    # We press Enter to accept default
    print("\nSending Enter...")
    tui.send_keystroke('\r')
    time.sleep(0.5)
    
    # Second prompt: "To Directory:" (this takes F2!)
    screen = tui.get_screen_dump()
    print("SECOND PROMPT SCREEN:")
    for line in screen[-8:]:
        print(repr(line))
        
    print("\nSending F2 (\\033OQ)...")
    tui.send_keystroke("\033OQ")
    time.sleep(1.0)
    
    screen = tui.get_screen_dump()
    print("F2 SCREEN:")
    for line in screen[-20:]:
        print(repr(line))
        
    tui.quit()

if __name__ == "__main__":
    debug_run()
