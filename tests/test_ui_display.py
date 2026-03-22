"""
Tests for UI bugs discovered after initial fixes.

These tests detect specific regressions that were not caught by the original test suite:
- Directory attributes display issues
- Footer visibility and flicker issues
"""
import time
import pytest
from pathlib import Path
from tui_harness import YtreeTUI
from ytree_keys import Keys


@pytest.fixture
def ytree_binary():
    """Path to ytree executable."""
    return str(Path(__file__).parent.parent / "build" / "ytree")


@pytest.fixture
def test_dir_with_files(tmp_path):
    """Create a test directory with sample files."""
    test_dir = tmp_path / "test_stats"
    test_dir.mkdir()

    # Create test files with different sizes
    (test_dir / "file1.txt").write_text("test1")
    (test_dir / "file2.txt").write_text("test file 2 content")
    (test_dir / "file3.txt").write_text("x" * 100)  # 100 bytes

    return test_dir


class TestDirectoryAttributesDisplay:
    """Tests for ATTRIBUTES section visibility and synchronization."""

    def test_attributes_appear_on_startup(self, test_dir_with_files, ytree_binary):
        """
        BUG: ATTRIBUTES section missing on startup in directory mode.
        EXPECTED: ATTRIBUTES section appears immediately on startup.
        """
        tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
        time.sleep(1.0)

        screen = "\n".join(tui.get_screen_dump())

        assert "ATTRIBUTES" in screen, \
            f"ATTRIBUTES section missing on startup:\n{screen}"

        tui.quit()

    def test_attributes_sync_with_cursor_movement(self, test_dir_with_files, ytree_binary):
        """
        BUG: Directory attributes show previous dir when navigating.
        EXPECTED: Attributes update immediately to reflect current directory.
        """
        # Create multiple directories to navigate between
        parent = test_dir_with_files.parent
        dir1 = parent / "dir_aaa"
        dir2 = parent / "dir_bbb"
        dir3 = parent / "dir_ccc"
        dir1.mkdir(exist_ok=True)
        dir2.mkdir(exist_ok=True)
        dir3.mkdir(exist_ok=True)

        tui = YtreeTUI(executable=ytree_binary, cwd=str(parent))
        time.sleep(1.0)

        # Move down one directory
        tui.send_keystroke(Keys.DOWN)
        time.sleep(0.5)

        screen2 = "\n".join(tui.get_screen_dump())
        lines = screen2.split('\n')
        attr_section = '\n'.join([line[90:] if len(line) > 90 else "" for line in lines[18:24]])

        # Move down again
        tui.send_keystroke(Keys.DOWN)
        time.sleep(0.5)

        screen3 = "\n".join(tui.get_screen_dump())
        lines3 = screen3.split('\n')
        attr_section2 = '\n'.join([line[90:] if len(line) > 90 else "" for line in lines3[18:24]])

        # Attributes should have changed after moving
        assert attr_section != attr_section2 or "ATTRIBUTES" not in attr_section, \
            f"Attributes didn't update after cursor movement\nFirst:\n{attr_section}\nSecond:\n{attr_section2}"

        tui.quit()


class TestFooterVisibility:
    """Tests for footer display stability across all UI modes."""

    def test_footer_visible_in_directory_mode(self, test_dir_with_files, ytree_binary):
        """Footer should always be visible and non-blank in directory mode."""
        tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
        time.sleep(1.0)

        screen = "\n".join(tui.get_screen_dump())
        footer = '\n'.join(screen.split('\n')[-3:])

        # Footer should contain command text
        footer_text = footer.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')
        assert len(footer_text) > 10, f"Footer appears blank in directory mode:\n{footer}"

        tui.quit()

    def test_footer_no_flicker_entering_small_window(self, test_dir_with_files, ytree_binary):
        """
        BUG: Footer momentarily disappears when entering file window.
        EXPECTED: Footer remains visible during transition to small file window.
        """
        tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
        time.sleep(1.0)

        # Enter file window (small window mode)
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.2)

        screen = "\n".join(tui.get_screen_dump())
        footer = '\n'.join(screen.split('\n')[-3:])
        footer_text = footer.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')

        assert len(footer_text) > 10, f"Footer blank in small file window:\n{footer}"

        tui.quit()

    def test_footer_no_flicker_entering_big_window(self, test_dir_with_files, ytree_binary):
        """
        BUG: Footer momentarily disappears when entering big window mode (ENTER ENTER).
        EXPECTED: Footer remains visible during transition to big file window.

        User report: "enter enter, dir to big window, footer still momentarily gone,
                      unexpected and disconcerting"
        """
        tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
        time.sleep(1.0)

        # Enter big window mode (ENTER twice) and sample screen multiple times
        # to catch any momentary blank footer
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.05)

        # Sample screen immediately after first ENTER
        screen1 = "\n".join(tui.get_screen_dump())
        footer1 = '\n'.join(screen1.split('\n')[-3:])
        footer1_text = footer1.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')

        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.05)  # Sample immediately after second ENTER

        screen2 = "\n".join(tui.get_screen_dump())
        footer2 = '\n'.join(screen2.split('\n')[-3:])
        footer2_text = footer2.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')

        time.sleep(0.2)  # Wait for stabilization

        screen3 = "\n".join(tui.get_screen_dump())
        footer3 = '\n'.join(screen3.split('\n')[-3:])
        footer3_text = footer3.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')

        # Check all three samples - footer should NEVER be blank
        errors = []
        if len(footer1_text) <= 10:
            errors.append(f"Footer blank after first ENTER:\n{footer1}")
        if len(footer2_text) <= 10:
            errors.append(f"Footer blank immediately after second ENTER:\n{footer2}")
        if len(footer3_text) <= 10:
            errors.append(f"Footer blank after stabilization:\n{footer3}")

        if errors:
            pytest.fail("Footer flicker detected:\n" + "\n---\n".join(errors) + f"\n\nFinal screen:\n{screen3}")

        tui.quit()

    def test_footer_visible_after_escape_from_file_window(self, test_dir_with_files, ytree_binary):
        """Footer should remain visible when exiting file window back to directory."""
        tui = YtreeTUI(executable=ytree_binary, cwd=str(test_dir_with_files))
        time.sleep(1.0)

        # Enter and exit file window
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.2)
        tui.send_keystroke(Keys.ESC)
        time.sleep(0.2)

        screen = "\n".join(tui.get_screen_dump())
        footer = '\n'.join(screen.split('\n')[-3:])
        footer_text = footer.replace('x', '').replace('m', '').replace('q', '').replace(' ', '').replace('\n', '')

        assert len(footer_text) > 10, f"Footer blank after ESC from file window:\n{footer}"

        tui.quit()

    def test_footer_blank_on_big_window_entry(self, test_dir_with_files, ytree_binary):
        """
        BUG A: Footer is blank upon initially entering the big file window via the small window.
        EXPECTED: The footer menu with command text should be visible.
        """
        # Start with SMALLWINDOWSKIP=0
        ytree_cfg = test_dir_with_files.parent / ".ytree"
        ytree_cfg.write_text("SMALLWINDOWSKIP=0\n")

        tui = YtreeTUI(
            executable=ytree_binary, 
            cwd=str(test_dir_with_files.parent)
        )
        time.sleep(1.0)
        # Move down to highlight test_dir_with_files
        tui.send_keystroke(Keys.DOWN)
        time.sleep(0.5)
        # Enter small file window
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)
        # Enter big file window
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)
        
        screen = "\n".join(tui.get_screen_dump())
        lines = screen.split('\n')
        
        # Grab the bottom three lines where the footer resides
        footer = '\n'.join(lines[-3:]).lower()
        
        # Assert that footer command text is present. If blank, this will fail.
        # We check specifically for the command row text. 
        # In ytree, this is usually at the very bottom or second from bottom.
        missing_commands = []
        for cmd in ["Attribute", "Delete", "Filter"]:
            if cmd.lower() not in footer:
                missing_commands.append(cmd)
        
        # We'll check if the combined text of the last 3 lines is just whitespace/borders
        footer_clean = footer.replace('x', '').replace('q', '').replace(' ', '').replace('\n', '').replace('m', '')
        if len(footer_clean) < 5:
            pytest.fail(f"BUG: Footer is blank on big window entry.\nFooter snapshot:\n{footer}")
            
        if missing_commands:
            pytest.fail(f"BUG: Footer is missing expected commands: {missing_commands}\nFooter snapshot:\n{footer}")
        tui.quit()

    def test_rename_prompt_no_footer_bleed(self, test_dir_with_files, ytree_binary):
        """
        BUG B: Footer text bleeds through under the rename prompt in big window mode.
        EXPECTED: The prompt row should cleanly show the rename prompt without footer text mixing in.
        """
        tui = YtreeTUI(
            executable=ytree_binary, 
            cwd=str(test_dir_with_files),
            env_extra={"SMALLWINDOWSKIP": "1"}
        )
        time.sleep(1.0)
        # Enter big file window
        tui.send_keystroke(Keys.ENTER)
        time.sleep(0.5)
        # Press 'r' to trigger the rename prompt
        tui.send_keystroke('r')
        time.sleep(0.5)
        screen = tui.get_screen_dump()
        
        # Find the row containing the rename prompt
        prompt_row_idx = -1
        for i, line in enumerate(screen):
            if "RENAME TO:" in line or "Rename to:" in line:
                prompt_row_idx = i
                break
                
        if prompt_row_idx == -1:
            pytest.fail(f"BUG: Could not find the Rename prompt. It may be non-responsive in big window.\nScreen:\n{screen}")
            
        prompt_line = screen[prompt_row_idx]
        line_above_prompt = screen[prompt_row_idx - 1] if prompt_row_idx > 0 else ""
        
        # Assert that footer text is NOT present on the line above the prompt
        # In a bug-free UI, the rename prompt clears the entire footer area
        footer_keywords = ["Attribute", "Brief", "Delete", "Execute", "Filter"]
        bleeding_words = [word for word in footer_keywords if word in line_above_prompt]
        if bleeding_words:
            pytest.fail(f"BUG: Footer text bled through above rename prompt. Found: {bleeding_words}\nLine above prompt: {line_above_prompt}")
        tui.quit()
