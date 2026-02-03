import os
import time
import shutil
import pexpect
import tempfile
import pytest

# Force absolute path for the binary
YTREE_BIN = os.path.abspath("./build/ytree")

def setup_module(module):
    if not os.path.exists(YTREE_BIN):
        pytest.skip(f"Binary {YTREE_BIN} not found. Please run 'make' first.")

def test_exit_on_delete():
    # Use /tmp for test directory
    test_base_dir = tempfile.mkdtemp(prefix="ytree_test_del_", dir="/tmp")
    test_dir = os.path.join(test_base_dir, "test_exit_del")
    os.makedirs(test_dir)
    test_file = os.path.join(test_dir, "file1.txt")
    
    with open(test_file, "w") as f:
        f.write("test")

    try:
        os.environ['TERM'] = 'xterm'
        child = pexpect.spawn(f"{YTREE_BIN} {test_dir}", timeout=10)
        child.setwinsize(24, 80)
        
        # Wait for ytree to start
        child.expect(r"Path:.*test_exit_del")
        
        # Enter file window
        child.send("\r")
        child.expect("file1.txt")
        
        # Delete the file
        child.send("d")
        child.expect("Delete this file")
        child.send("y")
        
        # Check if we are back in the directory window (DIR in help bar)
        index = child.expect(["FILE", "DIR"], timeout=5)
        assert index == 1, "Should be in directory window (DIR) after deletion"
        
        child.send("q")
        child.send("y")
    finally:
        shutil.rmtree(test_base_dir)

def test_exit_on_move():
    # Use /tmp for test directory
    test_base_dir = tempfile.mkdtemp(prefix="ytree_test_move_", dir="/tmp")
    src_dir = os.path.join(test_base_dir, "src")
    dst_dir = os.path.join(test_base_dir, "dst")
    os.makedirs(src_dir)
    os.makedirs(dst_dir)
    
    with open(os.path.join(src_dir, "file1.txt"), "w") as f:
        f.write("test")

    try:
        os.environ['TERM'] = 'xterm'
        child = pexpect.spawn(f"{YTREE_BIN} {src_dir}", timeout=10)
        child.setwinsize(24, 80)
        
        # Wait for ytree to start
        child.expect(r"Path:.*src")
        
        # Enter file window
        child.send("\r")
        child.expect("file1.txt")
        
        # Move the file
        child.send("m")
        child.expect(r"MOVE:.*")
        child.send("\r") # Accept default name
        child.expect("To Directory:")
        child.send(dst_dir + "\r")
        
        # Check if we are back in the directory window (DIR in help bar)
        index = child.expect(["FILE", "DIR"], timeout=10)
        assert index == 1, "Should be in directory window (DIR) after move"
        
        child.send("q")
        child.send("y")
    finally:
        shutil.rmtree(test_base_dir)
