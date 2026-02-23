import pytest
import time
import os
from tui_harness import YtreeTUI
from ytree_keys import Keys

def test_sort_help_missing_options(ytree_binary, sandbox):
    """
    Verifies that the sort help menu contains all required options.
    Specifically, (N)ame, (S)ize, and o(W)ner should be present.
    """
    # Set ASAN options to log to a file
    asan_log = sandbox / "asan.log"
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = f"log_path={asan_log}"
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(sandbox), env_extra=env)
    # Note: YtreeTUI handles its own env, but we can't easily pass env to it 
    # without modifying it. Let's modify YtreeTUI in our mind or just 
    # run it differently.
    
    # Actually YtreeTUI in tui_harness.py has:
    # env = { "COLUMNS": "120", "LINES": "36", "TERM": "xterm-256color" }
    # It doesn't use os.environ.
    
    time.sleep(1.0)
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    if not tui.wait_for_content("Attribute"):
        print("DEBUG: Timed out waiting for file window")
    
    # Press 'S' for sort
    tui.send_keystroke("S")
    if not tui.wait_for_content("SORT by"):
        print("DEBUG: Timed out waiting for sort prompt")
    
    # Get screen dump
    screen = tui.get_screen_dump()
    
    print("DEBUG: Full Screen Dump:")
    for i, line in enumerate(screen):
        if line.strip():
            print(f"{i:2d}: {line}")
    
    prompt_line_34 = screen[34].strip()
    prompt_line_35 = screen[35].strip()
    
    print(f"DEBUG: Prompt line 34: '{prompt_line_34}'")
    print(f"DEBUG: Prompt line 35: '{prompt_line_35}'")
    
    # Check if there are ASAN logs
    log_files = list(sandbox.glob("asan.log*"))
    if log_files:
        print(f"DEBUG: ASAN Log found: {log_files[0]}")
        with open(log_files[0], "r") as f:
            print(f"DEBUG: ASAN Output:\n{f.read()}")

    full_prompt = prompt_line_34 + " " + prompt_line_35
    
    missing = []
    if "(N)ame" not in full_prompt:
        missing.append("(N)ame")
    if "(S)ize" not in full_prompt:
        missing.append("(S)ize")
    if "o(W)ner" not in full_prompt and "(W)owner" not in full_prompt:
        missing.append("o(W)ner")
        
    assert not missing, f"Missing options in sort help: {', '.join(missing)}"
    
    tui.quit()
