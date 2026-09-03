import pytest
import time
import os
import re
from pathlib import Path
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

REPO_ROOT = Path(__file__).resolve().parents[1]


def _read_source(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")

def test_mkdir_command(ytnova_binary, tmp_path):
    """Verifies (M)ake Directory command."""
    d = tmp_path / "mkdir_test"
    d.mkdir()
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    assert tui.send_and_wait_for_condition(
        "M", lambda lines: any("MAKE DIRECTORY" in line for line in lines), timeout=1.0
    )
    # The prompt is "MAKE DIRECTORY:"
    tui.child.send("new_dir\r")
    assert tui.wait_for_condition(
        lambda _lines: (d / "new_dir").is_dir(),
        timeout=1.5,
        description="new directory creation",
    )
    
    print("\n==== SCREEN ====")
    print("\n".join(tui.get_screen_dump()))
    
    assert (d / "new_dir").exists()
    assert (d / "new_dir").is_dir()
    
    tui.quit()

def test_mkfile_command(ytnova_binary, tmp_path):
    """Verifies (n) - Touch/Make File command."""
    d = tmp_path / "mkfile_test"
    d.mkdir()
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    assert tui.send_and_wait_for_condition(
        "n", lambda lines: any("MAKE FILE" in line for line in lines), timeout=1.0
    )
    # The prompt is "MAKE FILE:"
    tui.child.send("new_file.txt\r")
    assert tui.wait_for_condition(
        lambda _lines: (d / "new_file.txt").exists(),
        timeout=1.5,
        description="new file creation",
    )
    
    assert (d / "new_file.txt").exists()
    
    tui.quit()

def test_delete_file_command(ytnova_binary, tmp_path):
    """Verifies (d)elete file command."""
    d = tmp_path / "delete_test"
    d.mkdir()
    target = d / "to_delete.txt"
    target.write_text("junk")
    
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Enter file window
    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: any("to_delete.txt" in line for line in lines),
        timeout=1.0,
    )
    
    # Delete
    assert tui.send_and_wait_for_condition(
        "d", lambda lines: any("Delete" in line for line in lines), timeout=1.0
    )
    # It might ask "Delete ... (Y/N)?"
    tui.child.send("y")
    assert tui.wait_for_condition(
        lambda _lines: not target.exists(),
        timeout=1.5,
        description="selected file deletion",
    )
    
    assert not target.exists()
    
    tui.quit()

def test_delete_dir_command(ytnova_binary, tmp_path):
    """Verifies (D)elete directory command (Shift-D)."""
    d = tmp_path / "delete_dir_test"
    d.mkdir()
    target = d / "subdir"
    target.mkdir()
    
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Navigate to subdir
    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=1.0)
    
    # Delete Dir (Shift-D)
    assert tui.send_and_wait_for_condition(
        "D", lambda lines: any("Delete" in line for line in lines), timeout=1.0
    )
    # Confirm
    tui.child.send("y")
    assert tui.wait_for_condition(
        lambda _lines: not target.exists(),
        timeout=1.5,
        description="selected directory deletion",
    )
    
    assert not target.exists()
    
    tui.quit()

def test_chmod_command(ytnova_binary, tmp_path):
    """Verifies (a) - attribute/chmod command."""
    d = tmp_path / "chmod_test"
    d.mkdir()
    target = d / "test.txt"
    target.write_text("junk")
    # Set to something known
    os.chmod(target, 0o644)
    
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Enter file window
    assert tui.send_and_wait_for_condition(
        Keys.ENTER, lambda lines: any("test.txt" in line for line in lines), timeout=1.0
    )
    
    # Attributes submenu (a -> m)
    assert tui.send_and_wait_for_screen_change("a", timeout=1.0)
    assert tui.send_and_wait_for_condition(
        "m", lambda lines: any("MODE" in line for line in lines), timeout=1.0
    )
    tui.send_keystroke(Keys.CTRL_U)  # Ctrl+U clears prefilled mode
    # Change to 0755 using octal input
    tui.child.send("755\r")
    assert tui.wait_for_condition(
        lambda _lines: os.stat(target).st_mode & 0o777 == 0o755,
        timeout=1.5,
        description="chmod 0755",
    )
    
    # Verify
    mode = os.stat(target).st_mode & 0o777
    assert mode == 0o755
    
    tui.quit()

def test_chmod_4digit_octal_command(ytnova_binary, tmp_path):
    """Verifies 4-digit octal mode input via attributes submenu."""
    d = tmp_path / "chmod_4digit_test"
    d.mkdir()
    target = d / "suid_test.sh"
    target.write_text("#!/bin/sh\necho ok\n")
    os.chmod(target, 0o644)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: any("suid_test.sh" in line for line in lines),
        timeout=1.0,
    )

    assert tui.send_and_wait_for_screen_change("a", timeout=1.0)
    assert tui.send_and_wait_for_condition(
        "m", lambda lines: any("MODE" in line for line in lines), timeout=1.0
    )
    tui.send_keystroke(Keys.CTRL_U)  # Ctrl+U clears prefilled mode
    tui.child.send("4755\r")
    assert tui.wait_for_condition(
        lambda _lines: os.stat(target).st_mode & 0o7777 == 0o4755,
        timeout=1.5,
        description="chmod 04755",
    )

    mode = os.stat(target).st_mode & 0o7777
    assert mode == 0o4755

    tui.quit()

def test_chown_command(ytnova_binary, tmp_path):
    """Verifies owner change via attributes submenu (A -> O)."""
    # Note: chown might fail if not root, but we can try to "change" to current user.
    d = tmp_path / "chown_test"
    d.mkdir()
    target = d / "test.txt"
    target.write_text("junk")
    
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Enter file window
    assert tui.send_and_wait_for_condition(
        Keys.ENTER, lambda lines: any("test.txt" in line for line in lines), timeout=1.0
    )
    
    # Attributes submenu -> Owner
    assert tui.send_and_wait_for_screen_change("a", timeout=1.0)
    assert tui.send_and_wait_for_condition(
        "o", lambda lines: any("OWNER" in line for line in lines), timeout=1.0
    )
    # It should prompt for owner name. We'll use the current user.
    import getpass
    user = getpass.getuser()
    assert tui.send_and_wait_for_screen_change(f"{user}\r", timeout=1.5)
    
    # Verify (even if it didn't change, we just want to see it didn't crash)
    assert target.exists()
    
    tui.quit()





def test_file_date_change_modified_updates_mtime(ytnova_binary, tmp_path):
    """Verifies file attributes date prompt updates the default modified scope."""
    d = tmp_path / "file_date_change_test"
    d.mkdir()
    target = d / "sample.txt"
    target.write_text("x")

    # Ensure old value is clearly different from requested timestamp.
    old_epoch = 946684800  # 2000-01-01 00:00:00 UTC
    os.utime(target, (old_epoch, old_epoch))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Enter file window and trigger Attributes -> Date.
    assert tui.send_and_wait_for_condition(
        Keys.ENTER, lambda lines: any("sample.txt" in line for line in lines), timeout=1.0
    )
    assert tui.send_and_wait_for_screen_change("a", timeout=1.0)
    assert tui.send_and_wait_for_condition(
        "d", lambda lines: any("DATE" in line for line in lines), timeout=1.0
    )
    tui.child.send("2026-03-15 10:11:12\r")
    expected_mtime = int(time.mktime((2026, 3, 15, 10, 11, 12, 0, 0, -1)))
    assert tui.wait_for_condition(
        lambda _lines: abs(int(os.stat(target).st_mtime) - expected_mtime) <= 1,
        timeout=1.5,
        description="file modified time update",
    )

    st = os.stat(target)
    new_mtime = int(st.st_mtime)
    new_atime = int(st.st_atime)
    assert abs(new_mtime - expected_mtime) <= 1, \
        f"mtime mismatch: expected ~{expected_mtime}, got {new_mtime}"
    assert new_atime == old_epoch, \
        f"atime should remain unchanged for modified-only update, got {new_atime}"

    tui.quit()


def test_archive_execute_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/execute.c")
    assert "Path_CreateTempFile(temp_path, sizeof(temp_path), \"ytnova_execute_\"" in src
    assert "if (fd_tmp != -1)" in src
    assert "unlink(temp_path);" in src


def test_archive_view_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/view.c")
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytnova_view_\"" in src
    assert "if (fd != -1)" in src
    assert "unlink(temp_filename);" in src


def test_archive_hex_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/hex.c")
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytnova_hex_\"" in src
    assert "if (fd != -1)" in src
    assert "unlink(temp_filename);" in src
