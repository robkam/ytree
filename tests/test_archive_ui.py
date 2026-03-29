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
