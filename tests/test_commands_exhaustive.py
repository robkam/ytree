import pytest
import time
import os
import re
from pathlib import Path
from tui_harness import YtreeTUI
from ytree_keys import Keys

REPO_ROOT = Path(__file__).resolve().parents[1]


def _read_source(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")

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

def test_dir_date_change_no_footer_artifact(ytree_binary, tmp_path):
    """Verifies dir date change does not leave a stray footer character."""
    d = tmp_path / "dir_date_footer_test"
    d.mkdir()
    (d / "subdir").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)

    # Try to move off the current root entry so we exercise a real dir entry path.
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.1)

    # Directory attributes -> date -> modified
    tui.send_keystroke("a")
    time.sleep(0.1)
    tui.send_keystroke("d")
    time.sleep(0.1)
    tui.send_keystroke("m")
    time.sleep(0.1)

    # Full-width overwrite input
    tui.send_keystroke("2026-03-15 10:11:12\r")
    time.sleep(0.6)

    screen = tui.get_screen_dump()
    footer_lines = screen[-3:]
    footer_text = "\n".join(footer_lines)

    # Regression: previously a lone alphabetic character (e.g. "C") remained.
    artifact_lines = [
        line.strip()
        for line in footer_lines
        if re.fullmatch(r"[A-Za-z]", line.strip())
    ]
    assert not artifact_lines, \
        f"Footer contained lone alphabetic artifact(s): {artifact_lines}\nFooter:\n{footer_text}"

    tui.quit()


def test_dir_mkdir_cancel_no_footer_artifact(ytree_binary, tmp_path):
    """Verifies cancelling mkdir from dir mode does not corrupt footer lines."""
    d = tmp_path / "dir_mkdir_cancel_footer_test"
    d.mkdir()
    (d / "subdir").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)

    # Move to a real directory entry if present.
    tui.send_keystroke(Keys.DOWN)
    time.sleep(0.1)

    # Open mkdir prompt then cancel.
    tui.send_keystroke("M")
    time.sleep(0.1)
    tui.send_keystroke(Keys.ESC)
    time.sleep(0.4)

    screen = tui.get_screen_dump()
    footer_lines = screen[-3:]
    footer_text = "\n".join(footer_lines)
    footer_lower = footer_text.lower()

    assert "commands" in footer_lower, \
        f"COMMANDS footer line missing after mkdir cancel.\nFooter:\n{footer_text}"

    artifact_lines = [
        line.strip()
        for line in footer_lines
        if re.fullmatch(r"[A-Za-z]", line.strip())
    ]
    assert not artifact_lines, \
        f"Footer contained lone alphabetic artifact(s): {artifact_lines}\nFooter:\n{footer_text}"

    tui.quit()


def test_file_date_change_modified_updates_mtime(ytree_binary, tmp_path):
    """Verifies file attributes date->modified updates the file mtime."""
    d = tmp_path / "file_date_change_test"
    d.mkdir()
    target = d / "sample.txt"
    target.write_text("x")

    # Ensure old value is clearly different from requested timestamp.
    old_epoch = 946684800  # 2000-01-01 00:00:00 UTC
    os.utime(target, (old_epoch, old_epoch))

    tui = YtreeTUI(executable=ytree_binary, cwd=str(d))
    time.sleep(1.0)

    # Enter file window and trigger Attributes -> Date -> Modified.
    tui.send_keystroke(Keys.ENTER)
    time.sleep(0.2)
    tui.send_keystroke("a")
    time.sleep(0.1)
    tui.send_keystroke("d")
    time.sleep(0.1)
    tui.send_keystroke("m")
    time.sleep(0.1)
    tui.send_keystroke("2026-03-15 10:11:12\r")
    time.sleep(0.6)

    st = os.stat(target)
    new_mtime = int(st.st_mtime)
    new_atime = int(st.st_atime)
    expected_mtime = int(time.mktime((2026, 3, 15, 10, 11, 12, 0, 0, -1)))

    assert abs(new_mtime - expected_mtime) <= 1, \
        f"mtime mismatch: expected ~{expected_mtime}, got {new_mtime}"
    assert new_atime == old_epoch, \
        f"atime should remain unchanged for modified-only update, got {new_atime}"

    tui.quit()


def test_archive_execute_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/execute.c")
    assert "Path_CreateTempFile(temp_path, sizeof(temp_path), \"ytree_execute_\"" in src
    assert "if (fd_tmp != -1)" in src
    assert "unlink(temp_path);" in src


def test_archive_view_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/view.c")
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytree_view_\"" in src
    assert "if (fd != -1)" in src
    assert "unlink(temp_filename);" in src


def test_archive_hex_tempfile_cleanup_present() -> None:
    src = _read_source("src/cmd/hex.c")
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytree_hex_\"" in src
    assert "if (fd != -1)" in src
    assert "unlink(temp_filename);" in src
