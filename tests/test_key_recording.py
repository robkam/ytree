import pexpect

from tui_harness import YtreeTUI
from ytree_keys import Keys


def test_f12_toggles_key_trace_recording_and_default_name(
    tmp_path, ytree_binary
):
    root = tmp_path / "root"
    root.mkdir()
    (root / "src").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))

    assert tui.wait_for_content("F12 record"), "Record footer did not appear."

    tui.send_keystroke(Keys.F12)
    assert tui.wait_for_content("Record key trace to:"), "Trace prompt did not appear."
    assert tui.wait_for_content("ytree-keys-001.txt"), (
        "The default recording filename should be incremented and prefilled."
    )
    screen = tui.get_screen_dump()
    assert not any("F2 browse" in line for line in screen), (
        "The recording prompt should not advertise unrelated input helpers."
    )
    tui.send_keystroke(Keys.ENTER)
    assert tui.wait_for_content("F12 stop"), "Stop footer did not appear."

    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.UP)
    tui.send_keystroke("/")
    assert tui.wait_for_content("Jump to:"), "Jump prompt did not appear."
    tui.send_keystroke("src")
    tui.send_keystroke(Keys.ENTER)

    tui.send_keystroke(Keys.F12)
    assert tui.wait_for_content("F12 record"), "Record footer did not return."

    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.UP)

    tui.send_keystroke(Keys.F12)
    assert tui.wait_for_content("Record key trace to:"), "Second trace prompt did not appear."
    assert tui.wait_for_content("ytree-keys-002.txt"), (
        "The next recording should default to the next incremented filename."
    )
    tui.send_keystroke(Keys.ESC)

    tui.send_keystroke(Keys.QUIT)
    tui.child.expect(pexpect.EOF)

    recorded_lines = [
        line.strip()
        for line in (root / "ytree-keys-001.txt").read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    ]

    assert recorded_lines == [
        "key down",
        "key up",
        "key /",
        "key enter",
        "key f12",
    ], f"Recorded lines did not use the human-readable trace format: {recorded_lines}"
