import io
import tarfile
import time

from ytree_control import YtreeController
from ytree_keys import Keys


def _create_archive(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, content in entries.items():
            payload = content.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            info.mtime = int(time.time())
            tf.addfile(info, io.BytesIO(payload))


def _archive_names(path):
    with tarfile.open(path, "r") as tf:
        return sorted(member.name.rstrip("/") for member in tf.getmembers())


def _archive_read_text(path, name):
    with tarfile.open(path, "r") as tf:
        fobj = tf.extractfile(name)
        assert fobj is not None, f"missing archive member: {name}"
        return fobj.read().decode("utf-8")


def _screen_text(controller):
    before = controller.child.before if isinstance(controller.child.before, str) else ""
    after = controller.child.after if isinstance(controller.child.after, str) else ""
    return before + after


def _log_archive(controller, archive_path):
    controller.child.send(Keys.LOG)
    time.sleep(0.2)
    controller.input_text(str(archive_path))
    controller.child.expect("ARCHIVE")


def _exit_archive_keep_volume(controller):
    controller.child.send("\\")
    time.sleep(0.7)


def _exit_archive_plain(controller):
    controller.child.send(Keys.LEFT)
    time.sleep(0.7)


def _copy_selected_file(controller, new_name, to_dir):
    controller.child.send(Keys.COPY)
    controller.child.expect("COPY")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    time.sleep(0.7)


def _move_selected_file(controller, new_name, to_dir):
    controller.child.send(Keys.MOVE)
    controller.child.expect("MOVE")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    time.sleep(0.7)


def test_archive_copy_matrix_fs_to_vfs(ytree_binary, tmp_path):
    root = tmp_path / "copy_fs_to_vfs"
    root.mkdir()
    (root / "fs_source.txt").write_text("fs payload", encoding="utf-8")
    dst_archive = root / "dst.tar"
    _create_archive(dst_archive, {})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    yt.child.send(Keys.ESC)
    time.sleep(0.3)

    yt.select_file("fs_source.txt")
    _copy_selected_file(yt, "copied_from_fs.txt", dst_archive)

    assert "copied_from_fs.txt" in _archive_names(dst_archive)
    assert _archive_read_text(dst_archive, "copied_from_fs.txt") == "fs payload"

    yt.quit()


def test_archive_copy_matrix_vfs_to_fs(ytree_binary, tmp_path):
    root = tmp_path / "copy_vfs_to_fs"
    root.mkdir()
    out_dir = root / "out"
    out_dir.mkdir()
    src_archive = root / "src.tar"
    _create_archive(src_archive, {"src.txt": "from archive"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, src_archive)
    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("src.txt")

    _copy_selected_file(yt, "copied_to_fs.txt", out_dir)

    copied = out_dir / "copied_to_fs.txt"
    assert copied.exists()
    assert copied.read_text(encoding="utf-8") == "from archive"

    yt.quit()


def test_archive_copy_matrix_vfs_to_vfs(ytree_binary, tmp_path):
    root = tmp_path / "copy_vfs_to_vfs"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    _create_archive(src_archive, {"src.txt": "vfs payload"})
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    _log_archive(yt, src_archive)
    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("src.txt")

    _copy_selected_file(yt, "copied_from_vfs.txt", dst_archive)

    src_names = _archive_names(src_archive)
    dst_names = _archive_names(dst_archive)
    assert "src.txt" in src_names
    assert "copied_from_vfs.txt" in dst_names
    assert _archive_read_text(dst_archive, "copied_from_vfs.txt") == "vfs payload"

    yt.quit()


def test_archive_move_matrix_fs_to_vfs(ytree_binary, tmp_path):
    root = tmp_path / "move_fs_to_vfs"
    root.mkdir()
    (root / "fs_source.txt").write_text("fs move payload", encoding="utf-8")
    dst_archive = root / "dst.tar"
    _create_archive(dst_archive, {})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    yt.child.send(Keys.ESC)
    time.sleep(0.3)

    yt.select_file("fs_source.txt")
    _move_selected_file(yt, "moved_from_fs.txt", dst_archive)

    assert not (root / "fs_source.txt").exists()
    assert "moved_from_fs.txt" in _archive_names(dst_archive)
    assert _archive_read_text(dst_archive, "moved_from_fs.txt") == "fs move payload"

    yt.quit()


def test_archive_move_matrix_vfs_to_fs(ytree_binary, tmp_path):
    root = tmp_path / "move_vfs_to_fs"
    root.mkdir()
    out_dir = root / "out"
    out_dir.mkdir()
    src_archive = root / "src.tar"
    _create_archive(src_archive, {"src.txt": "archive move payload"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, src_archive)
    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("src.txt")

    _move_selected_file(yt, "moved_to_fs.txt", out_dir)

    moved = out_dir / "moved_to_fs.txt"
    assert moved.exists()
    assert moved.read_text(encoding="utf-8") == "archive move payload"
    assert "src.txt" not in _archive_names(src_archive)

    yt.quit()


def test_archive_move_matrix_vfs_to_vfs(ytree_binary, tmp_path):
    root = tmp_path / "move_vfs_to_vfs"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    _create_archive(src_archive, {"src.txt": "archive move"})
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    _log_archive(yt, src_archive)
    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("src.txt")

    _move_selected_file(yt, "moved_to_other_vfs.txt", dst_archive)

    src_names = _archive_names(src_archive)
    dst_names = _archive_names(dst_archive)
    assert "src.txt" not in src_names
    assert "moved_to_other_vfs.txt" in dst_names
    assert _archive_read_text(dst_archive, "moved_to_other_vfs.txt") == "archive move"

    yt.quit()


def test_archive_create_rename_parity(ytree_binary, tmp_path):
    root = tmp_path / "create_rename"
    root.mkdir()
    archive_path = root / "ops.tar"
    _create_archive(archive_path, {"old.txt": "old payload"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, archive_path)

    yt.child.send("M")
    yt.child.expect("MAKE DIRECTORY")
    yt.input_text("newdir")

    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("old.txt")

    yt.child.send(Keys.RENAME)
    yt.child.expect("RENAME")
    yt.input_text("renamed.txt")
    time.sleep(0.5)

    names = _archive_names(archive_path)
    assert "newdir" in names
    assert "old.txt" not in names
    assert "renamed.txt" in names

    yt.quit()


def test_archive_delete_parity(ytree_binary, tmp_path):
    root = tmp_path / "archive_delete"
    root.mkdir()
    archive_path = root / "ops.tar"
    _create_archive(archive_path, {"delete_me.txt": "payload"})

    yt = YtreeController(ytree_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, archive_path)

    yt.child.send(Keys.ENTER)
    time.sleep(0.4)
    yt.child.expect("delete_me.txt")

    yt.child.send(Keys.DELETE)
    yt.child.expect("Delete this file")
    yt.child.send("Y")
    time.sleep(0.7)

    assert "delete_me.txt" not in _archive_names(archive_path)

    yt.quit()
