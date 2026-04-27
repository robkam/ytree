import tempfile
import os
import time
from ytree_control import YtreeController
from ytree_keys import Keys

def _select_write_destination(ytree: YtreeController, destination_key: str) -> None:
    ytree.child.expect(r"Destination", timeout=5)
    ytree.child.send(destination_key)
    ytree.child.expect(r"Write", timeout=5)

def test_print_and_pipe_feature():
    with tempfile.TemporaryDirectory() as td:
        # Create files with unique content upfront
        file_path = os.path.join(td, "test_file.txt")
        with open(file_path, "w") as f:
            f.write("FILE1_START\nHello World!\nLine 2\nFILE1_END\n")
        
        file2_path = os.path.join(td, "test_file2.txt")
        with open(file2_path, "w") as f:
            f.write("FILE2_START\nFile 2 Content\nFILE2_END\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build', 'ytree')
        ytree = YtreeController(binary, td)
        ytree.wait_for_startup()

        # Hit Enter to go into file mode
        ytree.child.send(Keys.ENTER)
        ytree.wait_for_refresh()
        ytree.wait_for_refresh()

        # 1. Test Raw Print (R)
        out_file_raw = os.path.join(td, "out_raw.txt")
        ytree.child.send("w") 
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("R")
        _select_write_destination(ytree, "F")
        ytree.input_text(out_file_raw)
        time.sleep(0.5)

        assert os.path.exists(out_file_raw)
        with open(out_file_raw, "r") as f:
            content = f.read()
            assert "FILE1_START" in content
            assert "###" not in content # Raw should NOT have headings

        # 2. Test Framed Print (F) - Each file is framed
        out_file_framed = os.path.join(td, "out_framed.txt")
        ytree.child.send("w")
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("F")
        ytree.child.expect(r"Frame separator", timeout=5)
        ytree.input_text("===")
        _select_write_destination(ytree, "F")
        ytree.input_text(out_file_framed)
        time.sleep(0.5)

        assert os.path.exists(out_file_framed)
        with open(out_file_framed, "r") as f:
            content = f.read()
            assert "=== test_file.txt ===" in content
            assert "======" in content # Footer frame
            assert "FILE1_START" in content

        # 3. Test Pipe
        out_file_pipe = os.path.join(td, "out_pipe.txt")
        ytree.child.send("p") 
        ytree.child.expect(r"Pipe", timeout=5)
        ytree.input_text(f"cat > {out_file_pipe}")
        ytree.child.expect(r"return to continue", timeout=10)
        ytree.child.send(Keys.ENTER)
        time.sleep(0.5)

        assert os.path.exists(out_file_pipe)
        with open(out_file_pipe, "r") as f:
            content = f.read()
            assert "FILE1_START" in content

        # 4. Test Multi-file Page Break (P) - Divider Logic
        # Tag both files
        ytree.child.send("T") 
        time.sleep(0.2)
        ytree.child.send(Keys.DOWN)
        time.sleep(0.2)
        ytree.child.send("T")
        time.sleep(1.0)
        
        out_file_multi = os.path.join(td, "out_multi.txt")
        ytree.child.send("W") # Tagged Write
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("P") # Page break mode
        ytree.child.expect(r"Frame separator", timeout=5)
        ytree.input_text("---SEP---")
        _select_write_destination(ytree, "F")
        ytree.input_text(out_file_multi)
        time.sleep(0.5)
        
        assert os.path.exists(out_file_multi)
        with open(out_file_multi, "r") as f:
            content = f.read()
            if "### test_file.txt" in content and "### test_file2.txt" in content:
                assert "---SEP---" in content, "Separator should be between files"
                assert not content.strip().endswith("---SEP---"), "Separator should NOT be at the very end"
    
        ytree.quit()

def test_write_plain_path_defaults_to_file_destination():
    with tempfile.TemporaryDirectory() as td:
        file_path = os.path.join(td, "source.txt")
        with open(file_path, "w") as f:
            f.write("plain-path-default\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "ytree")
        ytree = YtreeController(binary, td)
        ytree.wait_for_startup()
        ytree.child.send(Keys.ENTER)
        ytree.wait_for_refresh()
        ytree.wait_for_refresh()

        out_path = os.path.join(td, "plain_destination.txt")
        ytree.child.send("w")
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("R")
        _select_write_destination(ytree, "F")
        ytree.input_text(out_path)
        time.sleep(0.5)

        assert os.path.exists(out_path)
        with open(out_path, "r") as f:
            assert "plain-path-default" in f.read()

        alias_out_path = os.path.join(td, "legacy_alias_destination.txt")
        ytree.child.send("w")
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("R")
        _select_write_destination(ytree, "C")
        ytree.input_text(f">{alias_out_path}")
        time.sleep(0.5)

        assert os.path.exists(alias_out_path)
        with open(alias_out_path, "r") as f:
            assert "plain-path-default" in f.read()

        ytree.quit()

def test_write_command_failure_shows_error_without_crash():
    with tempfile.TemporaryDirectory() as td:
        file_path = os.path.join(td, "source.txt")
        with open(file_path, "w") as f:
            f.write("command-failure\n")

        binary = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "ytree")
        ytree = YtreeController(binary, td)
        ytree.wait_for_startup()
        ytree.child.send(Keys.ENTER)
        ytree.wait_for_refresh()
        ytree.wait_for_refresh()

        ytree.child.send("w")
        ytree.child.expect(r"cancel", timeout=10)
        ytree.child.send("R")
        _select_write_destination(ytree, "C")
        ytree.input_text("false")
        ytree.child.expect(r"execution of command", timeout=10)
        assert ytree.child.isalive(), "ytree crashed after write command failure"

        ytree.quit()
