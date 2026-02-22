import pytest
import time
import re
import pexpect
from ytree_keys import Keys

def get_clean_screen(yt):
    try:
        yt.child.expect(pexpect.TIMEOUT, timeout=0.2)
    except:
        pass
    raw = (yt.child.before if isinstance(yt.child.before, str) else "") + \
          (yt.child.after if isinstance(yt.child.after, str) else "")
    clean = re.sub(r'\x1B(?:\[[0-9;]*[a-zA-Z]|\(B|\[\?[0-9]*[a-zA-Z]|\**|[=>])?', '', raw)
    return clean

def sync_state(yt):
    yt.child.expect(r'20\d{2}')

def test_multi_column_rendering_metrics(controller, tmp_path):
    """
    REGRESSION: Columns overlap when metrics (max_filename_len) are not initialized correctly.
    Verifies that the file window correctly calculates columns even on first entry.
    """
    d = tmp_path / "layout_test"
    d.mkdir()
    # Create 50 files to force multi-column
    for i in range(50):
        (d / f"file_{i:02d}.txt").write_text("test")

    yt = controller(cwd=str(d))
    yt.wait_for_startup()
    
    # Enter file window via 'S' (Global Mode)
    yt.child.send(Keys.SHOWALL)
    sync_state(yt)
    
    # Cycle to MODE_3 (Name only)
    # MODE_0 -> MODE_1 -> MODE_2 -> MODE_3
    for _ in range(3):
        yt.child.send("\x06") # Ctrl-F: Toggle Mode
        time.sleep(0.5)
        sync_state(yt)

    screen = get_clean_screen(yt)
    assert "FILE" in screen
    
    # In MODE_3, 50 files with 120 width should definitely show file_49.txt
    # (around 4-5 columns)
    assert "file_49.txt" in screen
    
    yt.quit()
