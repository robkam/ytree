import pexpect

from tui_harness import YtreeTUI
from ytree_keys import Keys


def test_f12_starts_key_trace_recording(tmp_path, ytree_binary):
    root = tmp_path / "root"
    root.mkdir()
    (root / "src").mkdir()

    record_path = tmp_path / "keys.txt"
    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))

    tui.send_keystroke(Keys.F12)
    assert tui.wait_for_content("Record key trace to:"), "Trace prompt did not appear."
    tui.send_keystroke(str(record_path))
    tui.send_keystroke(Keys.ENTER)

    tui.send_keystroke(Keys.DOWN)
    tui.send_keystroke(Keys.UP)
    tui.send_keystroke(Keys.QUIT)
    tui.child.expect(pexpect.EOF)

    recorded_lines = [
        line.strip()
        for line in record_path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    ]

    assert any(line.endswith("KEY_DOWN") for line in recorded_lines), (
        "The recorded trace should capture the normalized down-arrow key.\n"
        f"Recorded lines: {recorded_lines}"
    )
    assert any(line.endswith("KEY_UP") for line in recorded_lines), (
        "The recorded trace should capture the normalized up-arrow key.\n"
        f"Recorded lines: {recorded_lines}"
    )
    assert any(line.endswith("q") for line in recorded_lines), (
        "The recorded trace should capture the quit key.\n"
        f"Recorded lines: {recorded_lines}"
    )
