import shutil
import tarfile
import time

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    return "\n".join(tui.get_screen_dump()[-3:]).lower()


def _footer_lines(tui):
    return tui.get_screen_dump()[-3:]


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


def test_archive_file_footer_uses_full_labels_and_shows_compare(tmp_path, ytree_binary):
    root = tmp_path / "archive_footer_labels"
    root.mkdir()

    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")

    archive_path = root / "aa_footer_test.tar"
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Enter file view first, then log the selected archive file.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.6)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), (
        "Expected archive mode after logging into tar file."
    )

    # Enter archive file view.
    footer = _footer_text(tui)
    for _ in range(4):
        if "j compare" in footer and ("arch-file" in footer or "file" in footer):
            break
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer = _footer_text(tui)

    assert "arch" in footer and "file" in footer
    assert "filemode" in footer, f"Archive file footer should show Filemode.\n{footer}"
    assert "rename" in footer, f"Archive file footer should show Rename wording.\n{footer}"
    assert "j compare" in footer, f"Archive file footer should expose J compare action.\n{footer}"
    assert "fmode" not in footer, f"Archive file footer still shows mangled Fmode label.\n{footer}"
    assert "rnm" not in footer, f"Archive file footer still shows mangled Rnm label.\n{footer}"

    tui.quit()


def test_archive_dir_footer_uses_compare_and_dirmode_before_global(tmp_path, ytree_binary):
    root = tmp_path / "archive_dir_footer_compare"
    root.mkdir()

    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")

    archive_path = root / "aa_dir_footer_test.tar"
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Enter file view first, then log the selected archive file.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.6)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), (
        "Expected archive mode after logging into tar file."
    )

    header = _footer_lines(tui)[0].lower()
    assert "compare" in header, f"Archive dir footer should show Compare.\n{header}"
    assert " copy " not in f" {header} ", f"Archive dir footer should not show Copy.\n{header}"
    assert "dirmode" in header and "global" in header and header.index("dirmode") < header.index("global"), (
        "Archive dir footer should list dirmode before global."
    )

    tui.quit()
