import tempfile
import os
import time
from ytnova_control import YtreeNovaController
from ytnova_keys import Keys

def _select_write_destination(ytnova: YtreeNovaController, destination_key: str) -> None:
    ytnova.child.expect(r"Destination", timeout=5)
    ytnova.child.send(destination_key)
    ytnova.child.expect(r"Write", timeout=5)

def test_print_and_pipe_feature():
    with tempfile.TemporaryDirectory() as td:
        # Create files with unique content upfront
        file_path = os.path.join(td, "test_file.txt")
        with open(file_path, "w") as f:
            f.write("FILE1_START\nHello World!\nLine 2\nFILE1_END\n")

        file2_path = os.path.join(td, "test_file2.txt")
        with open(file2_path, "w") as f:
            f.write("FILE2_START\nFile 2 Content\nFILE2_END\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build', 'ytnova')
        ytnova = YtreeNovaController(binary, td)
        ytnova.wait_for_startup()

        # Hit Enter to go into file mode
        ytnova.child.send(Keys.ENTER)
        ytnova.wait_for_refresh()
        ytnova.wait_for_refresh()

        # 1. Test Raw Print (R)
        out_file_raw = os.path.join(td, "out_raw.txt")
        ytnova.child.send("w")
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("R")
        _select_write_destination(ytnova, "F")
        ytnova.input_text(out_file_raw)
        time.sleep(0.5)

        assert os.path.exists(out_file_raw)
        with open(out_file_raw, "r") as f:
            content = f.read()
            assert "FILE1_START" in content
            assert "###" not in content # Raw should NOT have headings

        # 2. Test Framed Print (F) - Each file is framed
        out_file_framed = os.path.join(td, "out_framed.txt")
        ytnova.child.send("w")
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("F")
        ytnova.child.expect(r"Frame separator", timeout=5)
        ytnova.input_text("===")
        _select_write_destination(ytnova, "F")
        ytnova.input_text(out_file_framed)
        time.sleep(0.5)

        assert os.path.exists(out_file_framed)
        with open(out_file_framed, "r") as f:
            content = f.read()
            assert "=== test_file.txt ===" in content
            assert "======" in content # Footer frame
            assert "FILE1_START" in content

        # 3. Test Pipe
        out_file_pipe = os.path.join(td, "out_pipe.txt")
        ytnova.child.send("p")
        ytnova.child.expect(r"Pipe", timeout=5)
        ytnova.input_text(f"cat > {out_file_pipe}")
        ytnova.child.expect(r"return to continue", timeout=10)
        ytnova.child.send(Keys.ENTER)
        time.sleep(0.5)

        assert os.path.exists(out_file_pipe)
        with open(out_file_pipe, "r") as f:
            content = f.read()
            assert "FILE1_START" in content

        # 4. Test Multi-file Page Break (P) - Divider Logic
        # Tag both files
        ytnova.child.send("T")
        time.sleep(0.2)
        ytnova.child.send(Keys.DOWN)
        time.sleep(0.2)
        ytnova.child.send("T")
        time.sleep(1.0)

        out_file_multi = os.path.join(td, "out_multi.txt")
        ytnova.child.send("W") # Tagged Write
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("P") # Page break mode
        ytnova.child.expect(r"Page break separator", timeout=5)
        ytnova.input_text("---SEP---")
        _select_write_destination(ytnova, "F")
        ytnova.input_text(out_file_multi)
        time.sleep(0.5)

        assert os.path.exists(out_file_multi)
        with open(out_file_multi, "r") as f:
            content = f.read()
            if "### test_file.txt" in content and "### test_file2.txt" in content:
                assert "---SEP---" in content, "Separator should be between files"
                assert not content.strip().endswith("---SEP---"), "Separator should NOT be at the very end"

        ytnova.quit()

def test_write_plain_path_defaults_to_file_destination():
    with tempfile.TemporaryDirectory() as td:
        file_path = os.path.join(td, "source.txt")
        with open(file_path, "w") as f:
            f.write("plain-path-default\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "ytnova")
        ytnova = YtreeNovaController(binary, td)
        ytnova.wait_for_startup()
        ytnova.child.send(Keys.ENTER)
        ytnova.wait_for_refresh()
        ytnova.wait_for_refresh()

        out_path = os.path.join(td, "plain_destination.txt")
        ytnova.child.send("w")
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("R")
        _select_write_destination(ytnova, "F")
        ytnova.input_text(out_path)
        time.sleep(0.5)

        assert os.path.exists(out_path)
        with open(out_path, "r") as f:
            assert "plain-path-default" in f.read()

        alias_out_path = os.path.join(td, "legacy_alias_destination.txt")
        ytnova.child.send("w")
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("R")
        _select_write_destination(ytnova, "C")
        ytnova.input_text(f">{alias_out_path}")
        time.sleep(0.5)

        assert os.path.exists(alias_out_path)
        with open(alias_out_path, "r") as f:
            assert "plain-path-default" in f.read()

        ytnova.quit()

def test_write_command_failure_shows_error_without_crash():
    with tempfile.TemporaryDirectory() as td:
        file_path = os.path.join(td, "source.txt")
        with open(file_path, "w") as f:
            f.write("command-failure\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "ytnova")
        ytnova = YtreeNovaController(binary, td)
        ytnova.wait_for_startup()
        ytnova.child.send(Keys.ENTER)
        ytnova.wait_for_refresh()
        ytnova.wait_for_refresh()

        ytnova.child.send("w")
        ytnova.child.expect(r"cancel", timeout=10)
        ytnova.child.send("R")
        _select_write_destination(ytnova, "C")
        ytnova.input_text("false")
        ytnova.child.expect(r"execution of command", timeout=10)
        assert ytnova.child.isalive(), "ytnova crashed after write command failure"

        ytnova.quit()


def test_page_break_prompt_is_not_reused_from_framed_mode():
    with tempfile.TemporaryDirectory() as td:
        file_path = os.path.join(td, "source.txt")
        with open(file_path, "w") as f:
            f.write("page-break-prompt\n")

        binary = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "build",
            "ytnova",
        )
        ytnova = YtreeNovaController(binary, td)
        ytnova.wait_for_startup()
        ytnova.child.send(Keys.ENTER)
        ytnova.wait_for_refresh()
        ytnova.wait_for_refresh()

        format_prompt = ytnova.send_and_wait_for_condition(
            "w",
            lambda screen: screen
            if any("Format:" in line and "Page break" in line for line in screen)
            else False,
            timeout=2.0,
        )
        assert format_prompt, "Write format prompt did not appear"

        ytnova.child.send("P")
        assert ytnova.wait_for_text("Page break separator", timeout=2.0), (
            "\n".join(ytnova.get_screen_dump())
        )

        ytnova.quit()
