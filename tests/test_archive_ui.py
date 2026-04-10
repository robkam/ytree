import io
import tarfile
import zipfile

from tui_harness import YtreeTUI
from ytree_keys import Keys


def _create_zip(path, entries):
    with zipfile.ZipFile(path, "w") as zf:
        for name, data in entries.items():
            zf.writestr(name, data)


def _zip_names(path):
    with zipfile.ZipFile(path, "r") as zf:
        return sorted(zf.namelist())


def _zip_read_text(path, name):
    with zipfile.ZipFile(path, "r") as zf:
        return zf.read(name).decode("utf-8")


def _create_tar(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, data in entries.items():
            payload = data.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))


def _enter_archive_from_selected_file(tui):
    tui.send_keystroke(Keys.ENTER, wait=0.5)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)

    for _ in range(6):
        if tui.wait_for_content("Skipped unsafe archive member path", timeout=0.3):
            tui.send_keystroke(Keys.ENTER, wait=0.3)
        else:
            break


def test_archive_internal_path_trust_rejects_unsafe_members(
    ytree_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_rejects"
    root.mkdir()
    archive_path = root / "trust.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe payload",
            "../unsafe_dotdot_member.txt": "bad",
            "/unsafe_absolute_member.txt": "bad",
            "nested//unsafe_empty_segment_member.txt": "bad",
            "nested/./unsafe_dot_segment_member.txt": "bad",
            r"nested\\unsafe_separator_ambiguity_member.txt": "bad",
        },
    )

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)

        assert not tui.wait_for_content("unsafe_dotdot_member.txt", timeout=1.0)
        assert not tui.wait_for_content("unsafe_absolute_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_empty_segment_member.txt", timeout=1.0
        )
        assert not tui.wait_for_content("unsafe_dot_segment_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_separator_ambiguity_member.txt", timeout=1.0
        )
    finally:
        tui.quit()


def test_archive_internal_path_trust_safe_member_still_viewable(
    ytree_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_safe"
    root.mkdir()
    archive_path = root / "trust_safe.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe payload",
            "../unsafe_dotdot_member.txt": "bad",
        },
    )

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        assert not tui.wait_for_content("No files extracted.", timeout=0.8)
    finally:
        tui.quit()


def test_archive_internal_path_trust_trailing_slash_empty_dir_visible(
    ytree_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_trailing_dir"
    root.mkdir()
    archive_path = root / "trust_trailing_dir.tar"

    with tarfile.open(archive_path, "w") as tf:
        dir_info = tarfile.TarInfo(name="safe_dir/")
        dir_info.type = tarfile.DIRTYPE
        dir_info.mode = 0o755
        tf.addfile(dir_info)

        payload = "anchor payload".encode("utf-8")
        file_info = tarfile.TarInfo(name="safe_anchor.txt")
        file_info.size = len(payload)
        file_info.mode = 0o644
        tf.addfile(file_info, io.BytesIO(payload))

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_anchor.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)
        assert tui.wait_for_content("safe_dir", timeout=3.0)
    finally:
        tui.quit()


def test_archive_root_dot_member_is_ignored_without_warning(ytree_binary, tmp_path):
    root = tmp_path / "archive_root_dot_member"
    root.mkdir()
    archive_path = root / "dot_root.tar"

    with tarfile.open(archive_path, "w") as tf:
        dot_info = tarfile.TarInfo(name=".")
        dot_info.type = tarfile.DIRTYPE
        dot_info.mode = 0o755
        tf.addfile(dot_info)

        payload = "safe payload".encode("utf-8")
        file_info = tarfile.TarInfo(name="safe_member.txt")
        file_info.size = len(payload)
        file_info.mode = 0o644
        tf.addfile(file_info, io.BytesIO(payload))

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.LOG, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.8)

        assert not tui.wait_for_content("Skipped unsafe archive member path", timeout=1.0)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
    finally:
        tui.quit()


def test_archive_f7_preview_renders_member_content(ytree_binary, tmp_path):
    root = tmp_path / "archive_f7_preview"
    root.mkdir()
    archive_path = root / "preview.tar"
    _create_tar(archive_path, {"safe_member.txt": "safe payload"})

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)

        tui.send_keystroke(Keys.F7, wait=0.8)
        assert not tui.wait_for_content("Error opening file:", timeout=0.8)
        assert tui.wait_for_content("safe payload", timeout=3.0)
    finally:
        tui.quit()


def test_archive_create_overwrite_prompt_respects_no_then_yes(ytree_binary, tmp_path):
    root = tmp_path / "overwrite_prompt"
    root.mkdir()
    source_file = root / "0_source.txt"
    source_file.write_text("new payload", encoding="utf-8")
    archive_path = root / "z_existing.zip"
    _create_zip(archive_path, {"existing.txt": "old payload"})

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("0_source.txt", timeout=3.0)

        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{archive_path}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite z_existing.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("n", wait=0.3)

        assert _zip_names(archive_path) == ["existing.txt"]
        assert _zip_read_text(archive_path, "existing.txt") == "old payload"

        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{archive_path}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite z_existing.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.6)

        assert "0_source.txt" in _zip_names(archive_path)
        assert _zip_read_text(archive_path, "0_source.txt") == "new payload"
    finally:
        tui.quit()


def test_archive_create_ctrl_o_triggers_archive_prompt(ytree_binary, tmp_path):
    root = tmp_path / "ctrl_o_archive"
    root.mkdir()
    source_file = root / "source.txt"
    source_file.write_text("payload", encoding="utf-8")
    archive_path = root / "out.zip"

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("source.txt", timeout=3.0)

        tui.send_keystroke("\x0f", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{archive_path}\r", wait=0.6)

        assert archive_path.exists()
        assert _zip_names(archive_path) == ["source.txt"]
        assert _zip_read_text(archive_path, "source.txt") == "payload"
    finally:
        tui.quit()


def test_archive_create_inside_source_directory_is_allowed(ytree_binary, tmp_path):
    root = tmp_path / "inside_source_allowed"
    root.mkdir()
    child = root / "child"
    child.mkdir()
    (child / "nested.txt").write_text("payload", encoding="utf-8")
    destination = child / "out.zip"

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.8)

        assert destination.exists()
        assert _zip_names(destination) == ["child/nested.txt"]
        assert _zip_read_text(destination, "child/nested.txt") == "payload"
    finally:
        tui.quit()


def test_archive_create_overwrite_excludes_destination_from_payload(
    ytree_binary, tmp_path
):
    root = tmp_path / "overwrite_excludes_dest"
    root.mkdir()
    child = root / "child"
    child.mkdir()
    (child / "a.txt").write_text("alpha", encoding="utf-8")
    (child / "b.txt").write_text("beta", encoding="utf-8")
    destination = child / "out.zip"
    _create_zip(destination, {"stale.txt": "stale"})

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite out.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.8)

        names = _zip_names(destination)
        assert names == ["child/a.txt", "child/b.txt"]
        assert "out.zip" not in names
        assert _zip_read_text(destination, "child/a.txt") == "alpha"
        assert _zip_read_text(destination, "child/b.txt") == "beta"
    finally:
        tui.quit()


def test_archive_create_exclusion_empty_payload_shows_status_and_aborts(
    ytree_binary, tmp_path
):
    root = tmp_path / "empty_after_exclusion"
    root.mkdir()
    destination = root / "only.zip"
    _create_zip(destination, {"keep.txt": "keep"})

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("only.zip", timeout=3.0)

        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite only.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.4)
        assert tui.wait_for_content("Nothing to archive", timeout=3.0)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        screen = "\n".join(tui.get_screen_dump())
        assert "Nothing to archive" not in screen
        assert _zip_names(destination) == ["keep.txt"]
        assert _zip_read_text(destination, "keep.txt") == "keep"
    finally:
        tui.quit()


def test_archive_create_inside_source_round_trip_integrity(ytree_binary, tmp_path):
    root = tmp_path / "round_trip_integrity"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    (source / "alpha.txt").write_text("alpha payload", encoding="utf-8")
    (source / "beta.txt").write_text("beta payload", encoding="utf-8")
    (source / "nested").mkdir()
    (source / "nested" / "gamma.txt").write_text("gamma payload", encoding="utf-8")
    destination = source / "bundle.zip"
    extract_root = root / "extracted"
    extract_root.mkdir()

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.8)
    finally:
        tui.quit()

    with zipfile.ZipFile(destination, "r") as zf:
        zf.extractall(extract_root)

    expected = {
        "source/alpha.txt": "alpha payload",
        "source/beta.txt": "beta payload",
        "source/nested/gamma.txt": "gamma payload",
    }

    extracted_files = sorted(
        str(path.relative_to(extract_root))
        for path in extract_root.rglob("*")
        if path.is_file()
    )
    assert extracted_files == sorted(expected.keys())
    for rel_path, expected_text in expected.items():
        assert (extract_root / rel_path).read_text(encoding="utf-8") == expected_text


def test_archive_create_unsupported_format_shows_and_clears_status_error(
    ytree_binary, tmp_path
):
    root = tmp_path / "unsupported"
    root.mkdir()
    (root / "source.txt").write_text("payload", encoding="utf-8")
    destination = root / "out.7z"

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("source.txt", timeout=3.0)

        tui.send_keystroke("O", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.4)
        assert tui.wait_for_content("Unsupported archive format: .7z", timeout=3.0)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        screen = "\n".join(tui.get_screen_dump())
        assert "Unsupported archive format: .7z" not in screen
        assert not destination.exists()
    finally:
        tui.quit()
