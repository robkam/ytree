import shutil
import tarfile
import time

from helpers_ui import footer_lines as _footer_lines
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _send_left_arrow(tui, wait=0.4):
    # Use CSI-left to assert ACTION_MOVE_LEFT behavior explicitly.
    tui.send_keystroke("\033[D", wait=wait)


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


def test_archive_left_at_root_does_not_exit_archive(tmp_path, ytree_binary):
    """LEFT is collapse-only and must not exit archive mode at archive root."""
    root = tmp_path / "archive_exit_root"
    root.mkdir()

    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    _create_archive(root, "Absolutely MAD.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # Enter file view and log selected archive file.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), _screen_text(tui)

    # LEFT at archive root must not exit archive mode.
    _send_left_arrow(tui, wait=0.8)

    screen = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" in screen, (
        "LEFT at archive root must not exit archive mode.\n"
        f"Screen:\n{screen}"
    )
    assert "inside_dir" in screen, (
        "LEFT at archive root should keep archive tree visible.\n"
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

    _send_left_arrow(tui, wait=0.6)

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
    assert "hex invert j compare" in footer, (
        "Backslash archive-root exit must land in file focus on archive file.\n"
        f"Footer:\n{footer}\n\nScreen:\n{screen}"
    )

    tui.quit()


def test_archive_non_root_backslash_jumps_to_archive_root(tmp_path, ytree_binary):
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
    assert "\\ root" in _footer_text(tui), (
        "Archive non-root footer should advertise backslash jump-to-root behavior."
    )

    before = _screen_text(tui)
    tui.send_keystroke("\\", wait=0.6)
    after = _screen_text(tui)
    assert "\\ exit" in _footer_text(tui), (
        "Archive root footer should advertise backslash exit behavior."
    )
    assert "ARCHIVE" in after, "Backslash at archive non-root must not exit archive mode."
    assert "inside_dir/nested" not in tui.get_screen_dump()[0], (
        "Backslash at archive non-root must jump to archive root."
    )
    assert "inside_dir" in after, "Archive root context should remain visible after jump."
    assert "\a" not in before + after, "No bell expected for archive-root jump action."

    tui.quit()


def test_archive_file_backslash_is_silent_noop(tmp_path, ytree_binary):
    root = tmp_path / "archive_file_bs_noop"
    root.mkdir()
    _create_archive(root, "archive_file_noop.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), _screen_text(tui)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    before = _screen_text(tui)
    before_footer = _footer_text(tui)
    assert "hex invert j compare" in before_footer, "Expected archive file window footer."

    tui.send_keystroke("\\", wait=0.4)
    after = _screen_text(tui)
    after_footer = _footer_text(tui)
    assert "ARCHIVE" in after, "Backslash in archive file window must stay in archive context."
    assert "hex invert j compare" in after_footer, "Backslash in archive file window must be a no-op."
    assert after_footer == before_footer, "Backslash in archive file window should not move context."
    assert "\a" not in before + after, "No bell expected for no-op backslash action."

    tui.quit()


def test_backslash_in_fs_dir_and_file_windows_is_silent_noop(tmp_path, ytree_binary):
    root = tmp_path / "fs_backslash_noop"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    dir_before = _screen_text(tui)
    dir_before_footer = _footer_text(tui)
    tui.send_keystroke("\\", wait=0.4)
    dir_after = _screen_text(tui)
    dir_after_footer = _footer_text(tui)
    assert "ARCHIVE" not in dir_after, "Normal filesystem dir window must remain non-archive."
    assert dir_after_footer == dir_before_footer, "Backslash in fs dir window should be a no-op."
    assert "\a" not in dir_before + dir_after, "No bell expected for fs dir backslash no-op."

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    file_before = _screen_text(tui)
    file_before_footer = _footer_text(tui)
    assert "hex invert j compare" in file_before_footer, "Expected normal filesystem file window."

    tui.send_keystroke("\\", wait=0.4)
    file_after = _screen_text(tui)
    file_after_footer = _footer_text(tui)
    assert "hex invert j compare" in file_after_footer, "Backslash in fs file window must be a no-op."
    assert file_after_footer == file_before_footer, "Backslash in fs file window should not move context."
    assert "\a" not in file_before + file_after, "No bell expected for fs file backslash no-op."

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
    assert "hex invert j compare" in footer, (
        "'+' should relog directory so Enter opens file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.quit()


def test_logged_empty_vs_unlogged_labels(tmp_path, ytree_binary):
    root = tmp_path / "empty_vs_unlogged_labels"
    root.mkdir()
    empty = root / "emptydir"
    empty.mkdir()
    (root / "probe.txt").write_text("probe", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    root_screen = tui.get_screen_dump()
    root_file_row = next((line for line in root_screen if "probe.txt" in line), "")
    assert root_file_row, "Expected root file row containing probe.txt."
    first_filename_col = root_file_row.find("probe.txt")
    assert first_filename_col >= 0, "Could not determine first filename column."

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("+", wait=0.4)
    logged_lines = tui.get_screen_dump()
    logged_row = next((line for line in logged_lines if "No files" in line), "")
    assert logged_row, (
        "Logged empty directory should show 'No files' text.\n"
        f"Screen:\n{_screen_text(tui)}"
    )
    logged_col = logged_row.find("No files")
    assert logged_col == first_filename_col, (
        "Logged empty label must align with first filename column.\n"
        f"expected={first_filename_col}, actual={logged_col}\n"
        f"row={logged_row!r}"
    )

    tui.send_keystroke("-", wait=0.4)
    unlogged_lines = tui.get_screen_dump()
    unlogged_row = next((line for line in unlogged_lines if "Unlogged" in line), "")
    assert unlogged_row, (
        "Unlogged directory should show 'Unlogged' text."
    )
    unlogged_col = unlogged_row.find("Unlogged")
    assert unlogged_col == first_filename_col, (
        "Unlogged label must align with first filename column.\n"
        f"expected={first_filename_col}, actual={unlogged_col}\n"
        f"row={unlogged_row!r}"
    )

    tui.quit()


def test_depth_limited_placeholder_plus_loads_leaf_files(tmp_path, ytree_binary):
    root = tmp_path / "depth_limited_placeholder"
    root.mkdir()
    doc = root / "doc"
    doc.mkdir()
    ai = doc / "ai"
    ai.mkdir()
    (ai / "AGENT_PROMPT_TEMPLATE.md").write_text("prompt", encoding="utf-8")
    (ai / "DEBUGGING.md").write_text("debug", encoding="utf-8")
    (ai / "WORKFLOW.md").write_text("workflow", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # root -> doc -> expand doc -> select doc/ai
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    header = tui.get_screen_dump()[0]
    assert "doc/ai" in header, (
        "Expected doc/ai to be selected before expansion.\n"
        f"Header: {header!r}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.send_keystroke("+", wait=0.6)
    screen = _screen_text(tui)
    assert "AGENT_PROMPT_TEMPLATE.md" in screen, (
        "Depth-limited placeholder should load visible files after '+'.\n"
        f"Screen:\n{screen}"
    )
    assert "DEBUGGING.md" in screen, (
        "Leaf directory should no longer remain an empty placeholder.\n"
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
    assert "dirmode" in header and "global" in header and header.index("dirmode") < header.index("global"), (
        "Archive dir footer should list dirmode before global."
    )

    tui.quit()
