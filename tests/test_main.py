import pexpect
import pytest
import os
import stat
import shutil
from pathlib import Path
import re

# Helper to avoid repetition. Waits for the main UI to draw.
def wait_for_main_screen(child: pexpect.spawn, path_str: str):
    """Waits for a stable part of the ytree UI to appear."""
    # Use a flexible regex to match the top path bar.
    # The path may be truncated, so we match the end of it.
    child.expect(rf"Path:.*{re.escape(path_str)}", timeout=5)

# Helper for a clean exit
def quit_ytree(child: pexpect.spawn, confirm: bool = False):
    """Sends the quit command and handles optional confirmation."""
    child.send('q')
    if confirm:
        child.expect(r"quit ytree \(Y/N\) \?", timeout=2)
        child.send('Y')
    child.expect(pexpect.EOF)
    child.close()
    assert child.exitstatus == 0, f"ytree exited with status {child.exitstatus}"

class TestFilesystemMode:
    """Test Suite for Filesystem Mode (Test Plan Section 2)"""

    def test_2_1_1_startup_and_initial_view(self, ytree_env: Path):
        """Test 2.1.1: Startup and Initial View"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.expect(r"d1\s+")
            child.expect(r"d2\s+")
            child.expect(r"empty_dir\s+")
            child.expect(r"rootfile\.txt")
        finally:
            quit_ytree(child)

    def test_2_1_2_directory_navigation(self, ytree_env: Path):
        """Test 2.1.2: Directory Navigation"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            # 1. Navigate into d1, then d1_1
            child.send(' ')  # Move to d1
            child.sendline('\r') # Enter file view for d1
            child.send(' ')  # Move to d1_1
            child.sendline('\r') # Enter file view for d1_1
            child.send(pexpect.ESC) # Back to dir view
            wait_for_main_screen(child, str(ytree_env / "d1/d1_1"))

            # 2. Press L (Log)
            child.send('L')
            child.expect(r"NEW LOGIN-PATH:\s+/.*ytree_test/d1/d1_1")

            # 3. Edit path and navigate up
            # Send backspaces to delete 'd1_1'
            for _ in range(5):
                child.send(chr(8)) # Backspace character
            child.sendline() # Confirm
            
            # Expected: View updates to show contents of d1
            wait_for_main_screen(child, str(ytree_env / "d1"))
            child.expect(r"d1_1\s+")
            child.expect(r"file_d1\.txt")
        finally:
            quit_ytree(child)

    def test_2_1_3_root_navigation(self, ytree_env: Path):
        """Test 2.1.3: Root Navigation (`L` command)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send('L')
            child.expect(r"NEW LOGIN-PATH:")
            # Clear input and enter /
            child.sendcontrol('u')
            child.sendline('/')
            wait_for_main_screen(child, "/")
            child.expect(r"etc")
            child.expect(r"bin")
        finally:
            quit_ytree(child)
            
    def test_2_2_1_make_directory(self, ytree_env: Path):
        """Test 2.2.1: Make Directory (m)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send('  ') # Navigate down to d2
            child.send('m')
            child.expect(r"Make Subdirectory:")
            child.sendline("new_subdir")
            child.expect(r"new_subdir") # Expect it to appear in the UI
        finally:
            quit_ytree(child)
        assert (ytree_env / "d2" / "new_subdir").is_dir()

    def test_2_2_2_delete_empty_directory(self, ytree_env: Path):
        """Test 2.2.2: Delete Empty Directory (d)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send('   ') # Navigate down to empty_dir
            child.send('d')
            child.expect(r"Delete this directory \(Y/N\) \?")
            child.sendline('Y')
            # Expect the file list to redraw, excluding empty_dir
            child.expect(r"rootfile.txt")
            with pytest.raises(pexpect.exceptions.TIMEOUT):
                child.expect(r"empty_dir", timeout=1)
        finally:
            quit_ytree(child)
        assert not (ytree_env / "empty_dir").exists()

    def test_2_2_3_prune_directory(self, ytree_env: Path):
        """Test 2.2.3: Prune Non-Empty Directory (d)"""
        target_dir = ytree_env / "d1"
        # Create a copy for the test to delete
        prune_target = ytree_env / "d1_to_prune"
        shutil.copytree(target_dir, prune_target)

        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # Navigate to d1_to_prune
            child.send('d')
            child.expect(r"Directory not empty, PRUNE \? \(Y/N\) \?")
            child.sendline('Y')
            # After prune, the UI should refresh and not contain the dir
            with pytest.raises(pexpect.exceptions.TIMEOUT):
                child.expect(r"d1_to_prune", timeout=2)
        finally:
            quit_ytree(child)
        assert not prune_target.exists()

    def test_2_3_1_copy_file(self, ytree_env: Path):
        """Test 2.3.1: Copy File (c)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # Navigate to d1
            child.sendline('\r') # Enter file mode
            child.expect("file_d1.txt")
            child.send('c')
            child.expect("AS")
            child.sendline("file_copy.txt")
            child.expect("TO")
            child.sendline("../d2")
            child.expect("file exist; overwrite") # This shouldn't happen, but good to handle
            child.sendline('Y') # In case it does
        finally:
            # We must quit before the filesystem assertion
            child.send(pexpect.ESC) # back to dir view
            quit_ytree(child)

        copied_file = ytree_env / "d2" / "file_copy.txt"
        assert copied_file.is_file()
        assert copied_file.read_text() == "content d1\n"

    def test_2_3_2_move_file(self, ytree_env: Path):
        """Test 2.3.2: Move File (m)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # d1
            child.sendline('\r') # file view of d1
            child.send(' ') # d1_1
            child.sendline('\r') # file view of d1_1
            child.send('  ') # Navigate to link_to_rootfile
            child.expect(r">\s+link_to_rootfile") # Ensure cursor is on it
            child.send('m')
            child.expect("AS")
            child.sendline("moved_link")
            child.expect("TO")
            child.sendline("../../d2")
        finally:
            child.send(pexpect.ESC)
            quit_ytree(child)
        
        assert not (ytree_env / "d1/d1_1/link_to_rootfile").exists()
        moved_link = ytree_env / "d2/moved_link"
        assert moved_link.is_symlink()
        assert os.readlink(moved_link) == "../../rootfile.txt"

    def test_2_3_3_change_permissions(self, ytree_env: Path):
        """Test 2.3.3: Change Permissions (a)"""
        target_file = ytree_env / "rootfile.txt"
        target_file.chmod(0o644) # Ensure starting state

        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.sendline('\r') # enter file mode
            child.send('    ') # Navigate to rootfile.txt
            child.expect(r">\s+rootfile.txt")
            child.send('a')
            child.expect(r"New Filemodus:\s+-rw-r--r--")
            # Change to -r--r--r-- (chmod 444)
            child.sendcontrol('e') # End of line
            for _ in range(7):
                child.send(pexpect.KEY_LEFT) # Move left
            child.send('-') # Change 'w' to '-'
            child.sendline()
            child.expect(r"-r--r--r--")
        finally:
            quit_ytree(child)
            
        new_mode = target_file.stat().st_mode
        assert stat.S_IMODE(new_mode) == 0o444

    def test_2_4_1_execute_with_directory_context(self, ytree_env: Path):
        """Test 2.4.1: Execute with Directory Context"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # Move to d1
            child.send('x')
            child.expect(r"Command:")
            child.sendline("pwd")
            child.expect(str(ytree_env / "d1"))
        finally:
            # Need to hit enter to clear the command output screen
            child.sendline()
            quit_ytree(child)

    def test_2_4_2_pipe_file_content(self, ytree_env: Path):
        """Test 2.4.2: Pipe File Content"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # d1
            child.sendline('\r') # file view
            child.send(' ') # file_d1.txt
            child.send('p')
            child.expect(r"Pipe-Command:")
            child.sendline("wc -c")
            child.expect("11") # "content d1\n" is 11 bytes
        finally:
            child.sendline()
            quit_ytree(child)

    def test_2_4_3_execute_placeholder_single_file(self, ytree_env: Path):
        """Test 2.4.3: Execute with {} Placeholder on a Single File"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            child.send(' ') # to d1
            child.sendline('\r') # file view
            child.send(' ') # to file_d1.txt
            child.send('x')
            child.expect("Command:")
            child.sendline("ls -l {}")
            # Expect to see the file name in the output of ls -l
            child.expect(r"file_d1.txt")
        finally:
            child.sendline()
            quit_ytree(child)

    def test_2_4_4_execute_placeholder_tagged_files(self, ytree_env: Path):
        """Test 2.4.4: Execute with {} Placeholder on Tagged Files (^X)"""
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(ytree_env))
            # Navigate to d1/d1_1
            child.send(' '); child.sendline('\r')
            child.send(' '); child.sendline('\r')
            
            # Tag file_d1_1.log
            child.send(' '); child.send('t')
            # Tag link_to_rootfile
            child.send(' '); child.send('t')

            child.sendcontrol('x') # ^X
            child.expect("Command:")
            child.sendline('echo "File: {}"')
            
            # Expect both full paths in the output, order might vary
            child.expect(f"File: {ytree_env}/d1/d1_1/file_d1_1.log")
            child.expect(f"File: {ytree_env}/d1/d1_1/link_to_rootfile")
        finally:
            child.sendline()
            quit_ytree(child)

@pytest.mark.parametrize("archive_name", ["test.tar.gz", "test.zip"])
class TestArchiveMode:
    """Test Suite for Archive Mode (Test Plan Section 3)"""

    def test_3_1_1_startup_and_archive_view(self, ytree_env: Path, archive_name: str):
        """Test 3.1.1: Startup and Archive View UI"""
        archive_path = ytree_env.parent / archive_name
        child = pexpect.spawn(f"./ytree {archive_path}", encoding='utf-8', dimensions=(24, 120))
        try:
            # 1. Wait for main screen with real path to archive
            wait_for_main_screen(child, str(archive_path))
            # 2. Check for ARCHIVE label
            child.expect(rf"ARCHIVE:\s+\[{re.escape(archive_name)}")
            # 3. Check for virtual content
            child.expect(r"d1\s+")
            child.expect(r"rootfile\.txt")
            # 4. Check for current directory (virtual)
            child.expect(rf"\[{re.escape(archive_name)}")
        finally:
            quit_ytree(child)

    def test_3_1_4_context_menus(self, ytree_env: Path, archive_name: str):
        """Test 3.1.4: Context-Aware Menu Verification"""
        archive_path = ytree_env.parent / archive_name
        child = pexpect.spawn(f"./ytree {archive_path}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(archive_path))
            # 1. In directory menu, 'eXecute' should be absent
            with pytest.raises(pexpect.exceptions.TIMEOUT):
                child.expect(r"e\(X\)ecute", timeout=1)
            
            # 2. Enter file view
            child.sendline('\r')
            child.expect(r"file_d1\.txt")
            
            # In file menu, forbidden commands should be absent
            forbidden = ["e\(X\)ecute", "\(E\)dit", "\(A\)ttribute", "\(O\)wner", "\(G\)roup", "\(M\)ove", "\(R\)ename"]
            for cmd in forbidden:
                with pytest.raises(pexpect.exceptions.TIMEOUT):
                    child.expect(cmd, timeout=0.5)

            # Permitted commands should be present
            child.expect(r"\(C\)opy")
            child.expect(r"\(P\)ipe")
        finally:
            quit_ytree(child)

    def test_3_2_1_copy_from_archive(self, ytree_env: Path, archive_name: str):
        """Test 3.2.1: Copy from Archive (c)"""
        archive_path = ytree_env.parent / archive_name
        child = pexpect.spawn(f"./ytree {archive_path}", encoding='utf-8', dimensions=(24, 120))
        try:
            wait_for_main_screen(child, str(archive_path))
            child.send(' ') # d1
            child.sendline('\r') # file view
            child.send(' ') # file_d1.txt
            child.send('c')
            child.expect("AS")
            child.sendline("extracted.txt")
            child.expect("TO")
            child.sendline(str(ytree_env / "d2"))
        finally:
            child.send(pexpect.ESC)
            quit_ytree(child)

        extracted_file = ytree_env / "d2" / "extracted.txt"
        assert extracted_file.is_file()
        assert extracted_file.read_text() == "content d1\n"

class TestQuitFunctionality:
    """Test Suite for Quit Functionality (Test Plan Section 4)"""
    
    def test_4_1_1_quit_with_confirmation(self, ytree_env: Path):
        """Test 4.1.1: Quit with Confirmation"""
        conf_file = ytree_env / "test.conf"
        conf_file.write_text("[GLOBAL]\nCONFIRMQUIT=1\n")
        
        child = pexpect.spawn(f"./ytree -p {conf_file} {ytree_env}", encoding='utf-8')
        quit_ytree(child, confirm=True)

    def test_4_1_3_quit_without_confirmation(self, ytree_env: Path):
        """Test 4.1.3: Quit without Confirmation"""
        conf_file = ytree_env / "test.conf"
        conf_file.write_text("[GLOBAL]\nCONFIRMQUIT=0\n")
        
        child = pexpect.spawn(f"./ytree -p {conf_file} {ytree_env}", encoding='utf-8')
        quit_ytree(child, confirm=False)

    def test_4_2_1_quit_to_filesystem(self, ytree_env: Path):
        """Test 4.2.1: Quit To a Filesystem Directory"""
        # The test needs the *parent* PID of the ytree process.
        # We can't easily get the shell's PID that pytest is running under.
        # So we mock it by setting an environment variable that ytree will read.
        # This requires a small temporary change to ytree's source (quit.c).
        # For now, we'll simulate what the test *would* do.
        
        # NOTE: This test will fail without a modification to quit.c to allow
        # reading a mocked PPID from an environment variable for testing.
        pytest.skip("Requires source code modification in quit.c to mock getppid()")

        shell_pid = os.getpid() # Mock PID
        chdir_file = Path.home() / f".ytree-{shell_pid}.chdir"
        
        # Create the initial file that the `yt` function would create
        chdir_file.write_text(f'cd "{os.getcwd()}"\n')

        # Mock the parent PID for ytree
        child = pexpect.spawn(f"./ytree {ytree_env}", encoding='utf-8', env={'Y TREE_TEST_PPID': str(shell_pid)})
        try:
            wait_for_main_screen(child, str(ytree_env))
            # Navigate into d1/d1_1
            child.send(' '); child.sendline('\r')
            child.send(' '); child.sendline('\r')
            child.send(pexpect.ESC)
            
            # Quit To
            child.sendcontrol('q') # ^Q
            child.expect("quit ytree")
            child.sendline('Y')
            child.expect(pexpect.EOF)
        finally:
            child.close()
            # Verify the contents of the chdir file
            expected_path = ytree_env / "d1" / "d1_1"
            assert chdir_file.exists()
            content = chdir_file.read_text()
            assert f'cd "{expected_path}"' in content
            # Clean up
            if chdir_file.exists():
                chdir_file.unlink()