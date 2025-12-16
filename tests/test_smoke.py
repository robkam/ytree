import pexpect
import os
import sys
import time

def test_startup_and_quit():
    """
    Basic Smoke Test:
    1. Start ytree.
    2. Wait for the UI to appear (look for 'Attributes' or 'Size' in the header).
    3. Send 'q' to quit.
    4. Confirm user is asked to confirm (if configured) or exits.
    """
    
    # Path to binary (assuming running from project root)
    binary_path = "./ytree"
    
    if not os.path.exists(binary_path):
        print(f"Error: Binary not found at {binary_path}")
        sys.exit(1)

    # Spawn the process with a specific terminal size to ensure consistent rendering
    child = pexpect.spawn(binary_path, dimensions=(24, 80))
    
    # 1. Check for main UI elements
    # We look for standard column headers found in the Directory or File window
    try:
        child.expect('Command', timeout=5) # Assuming "Command" is in the menu or help line
    except pexpect.TIMEOUT:
        print("Timeout waiting for ytree UI. Screen dump:")
        print(child.before)
        raise

    # 2. Send Quit command
    child.send("q")

    # 3. Handle Confirm Quit (If enabled in defaults)
    # The default config usually has CONFIRMQUIT=0, so it should exit.
    # If it asks "quit ytree (Y/N)", we send 'y'.
    try:
        index = child.expect(['quit ytree', pexpect.EOF], timeout=2)
        if index == 0:
            child.send("y")
            child.expect(pexpect.EOF)
    except pexpect.TIMEOUT:
        print("Timeout waiting for exit.")
        raise

    print("Smoke test passed: ytree started and exited cleanly.")
