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


def _assert_margin_plus_marker(screen_rows, dir_name, expect_slash=False):
    candidates = [
        line
        for line in screen_rows
        if dir_name in line and "Path:" not in line and "CURRENT DIR" not in line
        and "ATTRIBUTES" not in line
    ]
    row = next((line for line in candidates if "tq" in line or "mq" in line), "")
    if not row and candidates:
        row = candidates[0]
    assert row, f"Expected a tree row for {dir_name!r}.\n" + "\n".join(screen_rows)
    name_col = row.find(dir_name)
    plus_col = row.find("+")
    assert plus_col >= 0 and plus_col < name_col, (
        "Unlogged marker must render in tree status margin, not in name text.\n"
        f"row={row!r}"
    )
    assert f"{dir_name}+" not in row, (
        "Tree row must not append '+' suffixes to directory names.\n"
        f"row={row!r}"
    )
    if expect_slash:
        assert f"{dir_name}/" in row, (
            "Unlogged directory with subdirs should show '/' suffix.\n"
            f"row={row!r}"
        )
    else:
        assert f"{dir_name}/" not in row, (
            "Unlogged directory without subdirs should not show '/' suffix.\n"
            f"row={row!r}"
        )


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
    assert "leaf_file.txt" in screen, (
        "Enter on unlogged leaf should relog/reveal one level while staying in tree mode.\n"
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
    rows_after_unlog = tui.get_screen_dump()
    _assert_margin_plus_marker(rows_after_unlog, "node")

    tui.send_keystroke("+", wait=0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    footer = _footer_text(tui)
    assert "hex invert j compare" in footer, (
        "'+' should relog directory so Enter opens file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.quit()


def test_enter_on_unlogged_dir_relogs_and_reveals_first_level_only(
    tmp_path, ytree_binary
):
    root = tmp_path / "unlogged_enter_relogs_first_level"
    root.mkdir()
    node = root / "node"
    node.mkdir()
    (node / "child_a").mkdir()
    (node / "child_b").mkdir()
    (node / "child_a" / "nested.txt").write_text("payload", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("-", wait=0.4)
        before = _screen_text(tui)
        assert "child_a" not in before, (
            "Precondition failed: collapsed unlogged directory should hide "
            "first-level children before Enter.\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "child_a" in after and "child_b" in after, (
            "Enter on unlogged dir should relog and reveal immediate "
            "subdirectories.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "child_a")
        _assert_margin_plus_marker(rows, "child_b")
        assert "hex invert j compare" not in footer, (
            "Enter on unlogged dir should stay in directory window, not switch "
            "to file window.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_unlogged_directory_with_subdirs_shows_slash_suffix(
    tmp_path, ytree_binary
):
    root = tmp_path / "unlogged_slash_suffix"
    root.mkdir()
    top = root / "top"
    top.mkdir()
    (top / "child").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("+", wait=0.5)
        tui.send_keystroke("-", wait=0.4)
        tui.send_keystroke("-", wait=0.4)
        _assert_margin_plus_marker(tui.get_screen_dump(), "top", expect_slash=True)
    finally:
        tui.quit()


def test_unlogged_placeholder_with_subdirs_shows_slash_suffix(
    tmp_path, ytree_binary
):
    root = tmp_path / "unlogged_placeholder_slash_suffix"
    root.mkdir()
    (root / ".ytree").write_text("TREEDEPTH=0\n", encoding="utf-8")
    top = root / "top"
    top.mkdir()
    (top / "child").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.4)
        _assert_margin_plus_marker(tui.get_screen_dump(), "top", expect_slash=True)
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_logs_and_reveals_first_level_only(
    tmp_path, ytree_binary
):
    root = tmp_path / "placeholder_enter_reveals_first_level"
    root.mkdir()
    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()
    (src / "cmd" / "main.c").write_text("int main(void){return 0;}\n",
                                         encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter.\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter on placeholder dir should reveal immediate subdirectories.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert "hex invert j compare" not in footer, (
            "Enter on placeholder dir should stay in directory window, not "
            "switch to file window.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_root_minus_resets_tree_and_right_relogs_to_profile_depth(
    tmp_path, ytree_binary
):
    root = tmp_path / "root_minus_right_profile_depth"
    root.mkdir()
    (root / ".ytree").write_text("TREEDEPTH=1\n", encoding="utf-8")
    src = root / "src"
    src.mkdir()
    cmd = src / "cmd"
    cmd.mkdir()
    deep = cmd / "deeper"
    deep.mkdir()
    (deep / "leaf.txt").write_text("payload", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("*", wait=0.7)
        expanded = _screen_text(tui)
        assert "deeper" in expanded, (
            "Precondition failed: expected deep expansion before root reset.\n"
            f"{expanded}"
        )

        tui.send_keystroke(Keys.HOME, wait=0.3)
        for _ in range(8):
            header = tui.get_screen_dump()[0]
            if str(root) in header:
                break
            tui.send_keystroke(Keys.LEFT, wait=0.3)

        before_rows = tui.get_screen_dump()
        at_root_before = "\n".join(before_rows)
        assert str(root) in tui.get_screen_dump()[0], (
            "Precondition failed: expected selection at root before root-left "
            "no-op assertion.\n"
            f"{at_root_before}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.5)
        after_rows = tui.get_screen_dump()
        at_root_after = "\n".join(after_rows)
        assert "\n".join(after_rows[1:]) == "\n".join(before_rows[1:]), (
            "Left at root should be a no-op; use '-' to release root contents.\n"
            f"Before:\n{at_root_before}\n\nAfter:\n{at_root_after}"
        )

        tui.send_keystroke("-", wait=0.7)
        tui.send_keystroke("-", wait=0.7)
        after_minus = _screen_text(tui)
        assert "deeper" not in after_minus, (
            "Root '-' release should clear expanded descendant state.\n"
            f"{after_minus}"
        )

        tui.send_keystroke(Keys.RIGHT, wait=0.9)
        after_right = _screen_text(tui)
        assert "src/" in after_right, (
            "Right on reset root should relog to configured depth and show "
            "first-level directory placeholders.\n"
            f"{after_right}"
        )
        assert "deeper" not in after_right, (
            "Right after root reset must not restore previous deep expansion state.\n"
            f"{after_right}"
        )
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_is_consistent_with_smallwindowskip_one(
    tmp_path, ytree_binary
):
    root = tmp_path / "placeholder_enter_smallwindowskip_one"
    root.mkdir()
    (root / ".ytree").write_text("SMALLWINDOWSKIP=1\n", encoding="utf-8")

    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter (SMALLWINDOWSKIP=1).\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter behavior should match standard mode for placeholder dirs "
            "when SMALLWINDOWSKIP=1.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert "hex invert j compare" not in footer, (
            "Enter on placeholder dir should remain in directory view when "
            "SMALLWINDOWSKIP=1.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_is_consistent_with_smallwindowskip_zero(
    tmp_path, ytree_binary
):
    root = tmp_path / "placeholder_enter_smallwindowskip_zero"
    root.mkdir()
    (root / ".ytree").write_text("SMALLWINDOWSKIP=0\n", encoding="utf-8")

    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter (SMALLWINDOWSKIP=0).\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter behavior should match standard mode for placeholder dirs "
            "when SMALLWINDOWSKIP=0.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert "hex invert j compare" not in footer, (
            "Enter on placeholder dir should remain in directory view when "
            "SMALLWINDOWSKIP=0.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
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


def test_small_window_tagged_symlink_and_empty_labels_share_name_column(
    tmp_path, ytree_binary
):
    root = tmp_path / "small_window_symlink_alignment"
    root.mkdir()
    target = root / "check_xml_integrit"
    target.write_text("payload", encoding="utf-8")
    (root / "current").symlink_to(target.name)

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        initial_lines = tui.get_screen_dump()
        file_row = next(
            (line for line in initial_lines if "check_xml_integrit" in line), ""
        )
        symlink_row = next((line for line in initial_lines if "@current" in line), "")
        assert file_row, "Expected untagged file row."
        assert symlink_row, "Expected untagged symlink row with '@' prefix."

        name_col = file_row.find("check_xml_integrit")
        symlink_col = symlink_row.find("@current")
        assert name_col >= 0 and symlink_col >= 0
        assert symlink_col == name_col, (
            "Untagged symlink label must start at the same file-name column.\n"
            f"name_col={name_col}, symlink_col={symlink_col}\n"
            f"file_row={file_row!r}\nsymlink_row={symlink_row!r}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("\x14", wait=0.5)  # Ctrl+T (tag all)

        tagged_lines = tui.get_screen_dump()
        tagged_file_row = next(
            (line for line in tagged_lines if "* check_xml_integrit" in line), ""
        )
        tagged_symlink_row = next(
            (line for line in tagged_lines if "* @current" in line), ""
        )
        assert tagged_file_row, "Expected tagged file row with '* ' prefix."
        assert tagged_symlink_row, "Expected tagged symlink row with '* @' prefix."

        tagged_name_col = tagged_file_row.find("check_xml_integrit")
        tagged_symlink_col = tagged_symlink_row.find("@current")
        assert tagged_name_col == name_col, (
            "Tagged file label must preserve the filename start column.\n"
            f"expected={name_col}, actual={tagged_name_col}\n"
            f"row={tagged_file_row!r}"
        )
        assert tagged_symlink_col == name_col, (
            "Tagged symlink label must preserve the filename start column.\n"
            f"expected={name_col}, actual={tagged_symlink_col}\n"
            f"row={tagged_symlink_row!r}"
        )
    finally:
        tui.quit()


def test_placeholder_dir_shows_unlogged_not_no_files(tmp_path, ytree_binary):
    root = tmp_path / "placeholder_shows_unlogged"
    root.mkdir()
    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "cmd" / "main.c").write_text("int main(void){return 0;}\n",
                                         encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.4)
        screen = _screen_text(tui)
        assert "Unlogged" in screen, (
            "Unscanned placeholder directory should display Unlogged in file view.\n"
            f"{screen}"
        )
        assert "No files" not in screen, (
            "Placeholder/unlogged directory must not display No files.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_volume_menu_enter_on_current_volume_preserves_existing_state(
    tmp_path, ytree_binary
):
    root = tmp_path / "volume_menu_preserve_current_state"
    root.mkdir()
    active_dir = root / "active_dir"
    stale_dir = root / "vol_old_name_dir"
    active_dir.mkdir()
    stale_dir.mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_rename = _screen_text(tui)
        assert "vol_old_name_dir" in before_rename, (
            "Precondition failed: expected original tree entry before rename.\n"
            f"{before_rename}"
        )

        stale_dir.rename(root / "vol_new_name_dir")
        time.sleep(0.4)

        before_relog = _screen_text(tui)
        assert "vol_old_name_dir" in before_relog, (
            "Tree should remain stale before explicit relog when focused on sibling.\n"
            f"{before_relog}"
        )

        tui.send_keystroke("K", wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        after_select = _screen_text(tui)
        assert "vol_old_name_dir" in after_select, (
            "Selecting the current volume in Volume Menu should preserve its "
            "existing in-memory state (no relog).\n"
            f"{after_select}"
        )
        assert "vol_new_name_dir" not in after_select, (
            "Volume Menu selection must not implicitly refresh/reload current "
            "volume state.\n"
            f"{after_select}"
        )
    finally:
        tui.quit()


def test_log_command_on_current_volume_reloads_tree_state(
    tmp_path, ytree_binary
):
    root = tmp_path / "log_current_volume_refresh"
    root.mkdir()
    active_dir = root / "active_dir"
    stale_dir = root / "vol_old_name_dir"
    active_dir.mkdir()
    stale_dir.mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_rename = _screen_text(tui)
        assert "vol_old_name_dir" in before_rename, (
            "Precondition failed: expected original tree entry before rename.\n"
            f"{before_rename}"
        )

        stale_dir.rename(root / "vol_new_name_dir")
        time.sleep(0.4)

        before_log = _screen_text(tui)
        assert "vol_old_name_dir" in before_log, (
            "Tree should remain stale before explicit relog command.\n"
            f"{before_log}"
        )

        tui.send_keystroke(Keys.LOG, wait=0.3)
        tui.send_keystroke(Keys.CTRL_U, wait=0.1)
        tui.send_keystroke(str(root), wait=0.1)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        after_log = _screen_text(tui)
        assert "vol_new_name_dir" in after_log, (
            "Logging the current volume should refresh the tree from disk.\n"
            f"{after_log}"
        )
        assert "vol_old_name_dir" not in after_log, (
            "Relog should replace stale directory entries with current disk state.\n"
            f"{after_log}"
        )
        header = tui.get_screen_dump()[0]
        assert str(root) in header and str(root / "active_dir") not in header, (
            "Explicit relog should reanchor selection at volume root.\n"
            f"Header: {header!r}\n\nScreen:\n{after_log}"
        )
    finally:
        tui.quit()


def test_depth_limited_placeholder_plus_loads_leaf_files(tmp_path, ytree_binary):
    root = tmp_path / "depth_limited_placeholder"
    root.mkdir()
    docs = root / "docs"
    docs.mkdir()
    ai = docs / "ai"
    ai.mkdir()
    (ai / "AGENT_PROMPT_TEMPLATE.md").write_text("prompt", encoding="utf-8")
    (ai / "DEBUGGING.md").write_text("debug", encoding="utf-8")
    (ai / "WORKFLOW.md").write_text("workflow", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    # root -> docs -> expand docs -> select docs/ai
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    header = tui.get_screen_dump()[0]
    assert "docs/ai" in header, (
        "Expected docs/ai to be selected before expansion.\n"
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
