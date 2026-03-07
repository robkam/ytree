import os
import time
from tui_harness import YtreeTUI
from ytree_keys import Keys

def is_dir_window(tui):
    """
    Checks if the TUI is in the directory window.
    The footer's top line shows "DIR " (case-insensitive) in directory window.
    """
    screen = tui.get_screen_dump()
    # Footer is the last 3 lines. pyte screen is 36 lines (0-35).
    # Line 33 is the top line of the footer.
    footer_top = screen[33].lower()
    return "dir" in footer_top[:5]

def is_file_window(tui):
    """
    Checks if the TUI is in the file window.
    The footer's top line shows "FILE " (case-insensitive) in file window.
    """
    screen = tui.get_screen_dump()
    footer_top = screen[33].lower()
    return "file" in footer_top[:5]

def test_exit_on_delete(ytree_binary, tmp_path):
    """
    TEST 1 — test_exit_on_delete:
    1. Create tmp dir with one file
    2. Launch YtreeTUI pointed at that dir
    3. Enter file window (Keys.ENTER), wait for file to appear
    4. Delete file: send 'd', wait for "Delete" prompt, send 'y'
    5. After deletion, the dir has no files — ytree should auto-return to directory window
    6. Assert footer shows "dir" (not "file")
    7. Quit
    """
    test_dir = tmp_path / "test_exit_del"
    test_dir.mkdir()
    test_file = test_dir / "file1.txt"
    test_file.write_text("test content")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir))
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    assert tui.wait_for_content("file1.txt"), "File should appear in file window"
    assert is_file_window(tui), "Should be in file window"

    # Delete the file
    tui.send_keystroke(Keys.DELETE)
    assert tui.wait_for_content("Delete"), "Delete prompt should appear"
    tui.send_keystroke(Keys.CONFIRM_YES)
    
    # Wait for ytree to return to directory window
    # We poll is_dir_window since it's an automatic transition
    success = False
    for _ in range(20):
        if is_dir_window(tui):
            success = True
            break
        time.sleep(0.1)
    
    assert success, "Should auto-return to directory window after last file is deleted"
    assert not is_file_window(tui), "Should NOT be in file window"
    
    tui.quit()

def test_exit_on_move(ytree_binary, tmp_path):
    """
    TEST 2 — test_exit_on_move:
    1. Create src/ and dst/ dirs, put one file in src/
    2. Launch YtreeTUI pointed at src/
    3. Enter file window (Keys.ENTER)
    4. Move file: send 'm', wait for MOVE prompt, send Enter (accept default name), 
       wait for "To Directory:" prompt, send dst path + Enter
    5. After move, src/ is empty — ytree should auto-return to directory window
    6. Assert footer shows "dir" (not "file")
    7. Quit
    """
    src_dir = tmp_path / "src"
    dst_dir = tmp_path / "dst"
    src_dir.mkdir()
    dst_dir.mkdir()
    
    test_file = src_dir / "file1.txt"
    test_file.write_text("test content")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(src_dir))
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    assert tui.wait_for_content("file1.txt"), "File should appear in file window"
    assert is_file_window(tui), "Should be in file window"

    # Move the file
    tui.send_keystroke(Keys.MOVE)
    assert tui.wait_for_content("MOVE"), "Move prompt should appear"
    tui.send_keystroke(Keys.ENTER) # Accept default name
    
    assert tui.wait_for_content("To Directory:"), "Destination prompt should appear"
    tui.send_keystroke(str(dst_dir) + Keys.ENTER)
    
    # Wait for ytree to return to directory window
    success = False
    for _ in range(20):
        if is_dir_window(tui):
            success = True
            break
        time.sleep(0.1)
    
    assert success, "Should auto-return to directory window after last file is moved"
    
    # Verify file actually moved
    assert not (src_dir / "file1.txt").exists()
    assert (dst_dir / "file1.txt").exists()
    
    tui.quit()
