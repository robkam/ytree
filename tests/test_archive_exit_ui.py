import shutil
import tarfile
import time

from tui_harness import YtreeTUI
from ytree_keys import Keys


def test_archive_left_exit_restores_small_window_and_files(tmp_path, ytree_binary):
    """
    Regression:
    After logging into an archive and pressing LEFT at archive root,
    ytree must restore the normal small file window with visible files
    and intact horizontal separators.
    """
    root = tmp_path / "archive_exit_root"
    root.mkdir()

    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")

    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")

    archive_path = root / "Absolutely MAD.tar"
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Log directly into archive view.
    tui.send_keystroke(Keys.LOG)
    time.sleep(0.2)
    tui.send_keystroke(str(archive_path) + Keys.ENTER)
    time.sleep(0.8)

    # Exit archive root with LEFT arrow.
    tui.send_keystroke(Keys.LEFT)
    time.sleep(0.8)

    screen = "\n".join(tui.get_screen_dump())

    # File pane must not be blank after archive exit.
    # Bug report indicates "blank" rather than explicit "No Files!" text.
    file_window_lines = tui.get_screen_dump()[21:31]
    pane_has_content = any(line.strip() for line in file_window_lines)
    assert pane_has_content, (
        "File pane area is blank after LEFT exit from archive root.\n"
        f"Screen:\n{screen}"
    )
    assert ("alpha.txt" in screen or "beta.txt" in screen), (
        "Expected files are missing after archive exit.\n"
        f"Screen:\n{screen}"
    )

    # Separator line should remain visible.
    assert ("qqq" in screen or "---" in screen), (
        "Horizontal separator disappeared after archive exit.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()
