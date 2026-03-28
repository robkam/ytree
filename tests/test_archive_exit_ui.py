import shutil
import tarfile
import time

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _footer_text(tui):
    return "\n".join(tui.get_screen_dump()[-3:]).lower()


def _footer_lines(tui):
    return tui.get_screen_dump()[-3:]


def _screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _create_archive(root, archive_name="sample.tar"):
    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")
    (archive_source / "nested").mkdir()
    (archive_source / "nested" / "nested.txt").write_text("nested", encoding="utf-8")

    archive_path = root / archive_name
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)
    return archive_path


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


def test_minus_on_leaf_unlogs_directory_state(tmp_path, ytree_binary):
    root = tmp_path / "minus_leaf_unlog"
    root.mkdir()
    leaf = root / "leaf_dir"
    leaf.mkdir()
    (leaf / "leaf_file.txt").write_text("leaf", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("-", wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    footer = _footer_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert "hex j compare" not in footer, (
        "Leaf directory should be unlogged after '-' and not enter file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{screen}"
    )
    assert "leaf_file.txt" not in screen, (
        "Leaf file remained visible after '-' unlog contract.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_archive_left_non_root_does_not_exit_immediately(tmp_path, ytree_binary):
    root = tmp_path / "a_left"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    archive_path = _create_archive(root, "l.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke("*", wait=0.6)
    for _ in range(12):
        if "inside_dir/nested" in tui.get_screen_dump()[0]:
            break
        tui.send_keystroke(Keys.DOWN, wait=0.25)
    assert "inside_dir/nested" in tui.get_screen_dump()[0], "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.LEFT, wait=0.6)

    screen = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" in screen, (
        "LEFT from archive non-root must not exit archive mode immediately.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_archive_root_backslash_exits_to_parent_file_focus(tmp_path, ytree_binary):
    root = tmp_path / "a_bs"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    archive_path = _create_archive(root, "b.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke("\\", wait=0.8)

    screen = "\n".join(tui.get_screen_dump())
    footer = _footer_text(tui)
    assert "ARCHIVE" not in screen, (
        "Backslash at archive root must exit archive context.\n"
        f"Screen:\n{screen}"
    )
    assert "hex j compare" in footer, (
        "Backslash archive-root exit must land in file focus on archive file.\n"
        f"Footer:\n{footer}\n\nScreen:\n{screen}"
    )

    tui.quit()


def test_archive_non_root_backslash_is_silent_noop(tmp_path, ytree_binary):
    root = tmp_path / "archive_non_root_bs_noop"
    root.mkdir()
    _create_archive(root, "noop.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), _screen_text(tui)

    tui.send_keystroke("*", wait=0.6)
    for _ in range(10):
        header = tui.get_screen_dump()[0]
        if "inside_dir/nested" in header:
            break
        tui.send_keystroke(Keys.DOWN, wait=0.2)
    assert "inside_dir/nested" in tui.get_screen_dump()[0], _screen_text(tui)

    before = _screen_text(tui)
    tui.send_keystroke("\\", wait=0.6)
    after = _screen_text(tui)
    assert "ARCHIVE" in after, "Backslash at archive non-root must not exit archive mode."
    assert "inside_dir/nested" in tui.get_screen_dump()[0], (
        "Backslash at archive non-root must be a no-op and keep current node."
    )
    assert "\a" not in before + after, "No bell expected for no-op backslash action."

    tui.quit()


def test_unlogged_tree_shows_plus_marker_and_plus_relogs(tmp_path, ytree_binary):
    root = tmp_path / "unlogged_plus_marker"
    root.mkdir()
    node = root / "node"
    node.mkdir()
    (node / "child.txt").write_text("payload", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("-", wait=0.4)
    screen_after_unlog = _screen_text(tui)
    node_lines = [line for line in tui.get_screen_dump() if "node" in line]
    assert node_lines, f"Expected node row after unlog.\n{screen_after_unlog}"
    assert any("+" in line for line in node_lines), (
        "Unlogged directory should display '+' marker in tree row.\n"
        f"{screen_after_unlog}"
    )

    tui.send_keystroke("+", wait=0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    footer = _footer_text(tui)
    assert "hex j compare" in footer, (
        "'+' should relog directory so Enter opens file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.quit()


def test_logged_empty_vs_unlogged_labels(tmp_path, ytree_binary):
    root = tmp_path / "empty_vs_unlogged_labels"
    root.mkdir()
    empty = root / "emptydir"
    empty.mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    logged_empty_screen = _screen_text(tui).lower()
    assert "no files" in logged_empty_screen, (
        "Logged empty directory should show 'No files' text."
    )

    tui.send_keystroke(Keys.LEFT, wait=0.4)
    tui.send_keystroke("-", wait=0.4)
    unlogged_screen = _screen_text(tui).lower()
    assert "unlogged" in unlogged_screen, (
        "Unlogged directory should show 'Unlogged' text."
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
