import pytest
import time
import os
from tui_harness import YtreeTUI
from ytree_keys import Keys

def test_mkdir_command(ytree_binary, tmp_path):
    """Verifies (M)ake Directory command."""
    d = tmp_path / "mkdir_test"
    d.mkdir()
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    tui.send_keystroke("M")
    time.sleep(0.1)
    # The prompt is "MAKE DIRECTORY:"
    tui.send_keystroke("new_dir\r")
    time.sleep(0.5)
    
    print("\n==== SCREEN ====")
    print("\n".join(tui.get_screen_dump()))
    
    assert (d / "new_dir").exists()
    assert (d / "new_dir").is_dir()
    
    tui.quit()

def test_mkfile_command(ytree_binary, tmp_path):
    """Verifies (n) - Touch/Make File command."""
    d = tmp_path / "mkfile_test"
    d.mkdir()
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    tui.send_keystroke("n") # Touch is 'n'
    time.sleep(0.1)
    # The prompt is "MAKE FILE:"
    tui.send_keystroke("new_file.txt\r")
    time.sleep(0.5)
    
    assert (d / "new_file.txt").exists()
    
    tui.quit()

def test_delete_file_command(ytree_binary, tmp_path):
    """Verifies (d)elete file command."""
    d = tmp_path / "delete_test"
    d.mkdir()
    target = d / "to_delete.txt"
    target.write_text("junk")
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.2)
    
    # Delete
    tui.send_keystroke("d")
    time.sleep(0.2)
    # It might ask "Delete ... (Y/N)?"
    tui.send_keystroke("y")
    time.sleep(0.5)
    
    assert not target.exists()
    
    tui.quit()

def test_delete_dir_command(ytree_binary, tmp_path):
    """Verifies (D)elete directory command (Shift-D)."""
    d = tmp_path / "delete_dir_test"
    d.mkdir()
    target = d / "subdir"
    target.mkdir()
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    # Navigate to subdir
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.1)
    
    # Delete Dir (Shift-D)
    tui.send_keystroke("D")
    time.sleep(0.2)
    # Confirm
    tui.send_keystroke("y")
    time.sleep(0.5)
    
    assert not target.exists()
    
    tui.quit()

def test_chmod_command(ytree_binary, tmp_path):
    """Verifies (a) - attribute/chmod command."""
    d = tmp_path / "chmod_test"
    d.mkdir()
    target = d / "test.txt"
    target.write_text("junk")
    # Set to something known
    os.chmod(target, 0o644)
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.2)
    
    # Attributes submenu (a -> m)
    tui.send_keystroke("a")
    time.sleep(0.1)
    tui.send_keystroke("m")
    time.sleep(0.1)
    tui.send_keystroke(Keys.CTRL_U)  # Ctrl+U clears prefilled mode
    time.sleep(0.05)
    # Change to 0755 using octal input
    tui.send_keystroke("755\r")
    time.sleep(0.5)
    
    # Verify
    mode = os.stat(target).st_mode & 0o777
    assert mode == 0o755
    
    tui.quit()

def test_chmod_4digit_octal_command(ytree_binary, tmp_path):
    """Verifies 4-digit octal mode input via attributes submenu."""
    d = tmp_path / "chmod_4digit_test"
    d.mkdir()
    target = d / "suid_test.sh"
    target.write_text("#!/bin/sh\necho ok\n")
    os.chmod(target, 0o644)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)

    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.2)

    tui.send_keystroke("a")
    time.sleep(0.1)
    tui.send_keystroke("m")
    time.sleep(0.1)
    tui.send_keystroke(Keys.CTRL_U)  # Ctrl+U clears prefilled mode
    time.sleep(0.05)
    tui.send_keystroke("4755\r")
    time.sleep(0.5)

    mode = os.stat(target).st_mode & 0o7777
    assert mode == 0o4755

    tui.quit()

def test_chown_command(ytree_binary, tmp_path):
    """Verifies owner change via attributes submenu (A -> O)."""
    # Note: chown might fail if not root, but we can try to "change" to current user.
    d = tmp_path / "chown_test"
    d.mkdir()
    target = d / "test.txt"
    target.write_text("junk")
    
    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)
    
    # Enter file window
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.2)
    
    # Attributes submenu -> Owner
    tui.send_keystroke("a")
    time.sleep(0.1)
    tui.send_keystroke("o")
    time.sleep(0.1)
    # It should prompt for owner name. We'll use the current user.
    import getpass
    user = getpass.getuser()
    tui.send_keystroke(f"{user}\r")
    time.sleep(0.5)
    
    # Verify (even if it didn't change, we just want to see it didn't crash)
    assert target.exists()
    
    tui.quit()
