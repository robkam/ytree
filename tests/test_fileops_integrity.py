import hashlib
import tarfile
import time
from pathlib import Path

from ytree_control import YtreeController
from ytree_keys import Keys


REPO_ROOT = Path(__file__).resolve().parents[1]


def _sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _snapshot_tree(root: Path) -> dict[str, str]:
    snapshot = {}
    for path in sorted(root.rglob("*")):
        if path.is_file():
            relpath = str(path.relative_to(root))
            if relpath.startswith(".ytree"):
                continue
            snapshot[relpath] = _sha256_file(path)
    return snapshot


def _snapshot_archive(path: Path) -> dict[str, str]:
    snapshot = {}
    with tarfile.open(path, "r") as archive:
        for member in sorted(archive.getmembers(), key=lambda m: m.name):
            if not member.isfile():
                continue
            member_file = archive.extractfile(member)
            assert member_file is not None, f"missing archive member data: {member.name}"
            snapshot[member.name] = hashlib.sha256(member_file.read()).hexdigest()
    return snapshot


def _copy_selected_file(controller: YtreeController, source_name: str, new_name: str, to_dir: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.COPY)
    controller.child.expect("COPY")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    time.sleep(0.7)


def _move_selected_file(controller: YtreeController, source_name: str, new_name: str, to_dir: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.MOVE)
    controller.child.expect("MOVE")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    time.sleep(0.7)


def _rename_selected_file(controller: YtreeController, source_name: str, new_name: str) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.RENAME)
    controller.child.expect("RENAME")
    controller.input_text(new_name)
    time.sleep(0.6)


def _delete_selected_file(controller: YtreeController, source_name: str) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.DELETE)
    controller.child.expect("Delete this file")
    controller.child.send("Y")
    time.sleep(0.7)


def _read(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")


def test_copy_mutation_updates_only_destination_hashes(ytree_binary, tmp_path):
    root = tmp_path / "copy_integrity"
    source = root / "source"
    dest = root / "dest"
    source.mkdir(parents=True)
    dest.mkdir()
    (source / "alpha.txt").write_text("alpha payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        _copy_selected_file(controller, "alpha.txt", "alpha_copy.txt", dest)
    finally:
        controller.quit()

    expected = dict(before)
    expected["dest/alpha_copy.txt"] = before["source/alpha.txt"]
    assert _snapshot_tree(root) == expected


def test_move_mutation_rehomes_hash_without_corrupting_payload(ytree_binary, tmp_path):
    root = tmp_path / "move_integrity"
    source = root / "source"
    dest = root / "dest"
    source.mkdir(parents=True)
    dest.mkdir()
    (source / "beta.txt").write_text("beta payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        _move_selected_file(controller, "beta.txt", "beta_moved.txt", dest)
    finally:
        controller.quit()

    expected = dict(before)
    moved_hash = expected.pop("source/beta.txt")
    expected["dest/beta_moved.txt"] = moved_hash
    assert _snapshot_tree(root) == expected


def test_rename_mutation_preserves_file_hash(ytree_binary, tmp_path):
    root = tmp_path / "rename_integrity"
    root.mkdir()
    (root / "gamma.txt").write_text("gamma payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        _rename_selected_file(controller, "gamma.txt", "gamma_renamed.txt")
    finally:
        controller.quit()

    expected = {"gamma_renamed.txt": before["gamma.txt"]}
    assert _snapshot_tree(root) == expected


def test_delete_mutation_removes_only_selected_file(ytree_binary, tmp_path):
    root = tmp_path / "delete_integrity"
    root.mkdir()
    (root / "keep.txt").write_text("keep", encoding="utf-8")
    (root / "remove.txt").write_text("remove", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        _delete_selected_file(controller, "remove.txt")
    finally:
        controller.quit()

    expected = dict(before)
    expected.pop("remove.txt")
    assert _snapshot_tree(root) == expected


def test_copy_prompt_cancel_keeps_tree_snapshot_identical(ytree_binary, tmp_path):
    root = tmp_path / "copy_cancel_integrity"
    root.mkdir()
    (root / "cancel.txt").write_text("cancel payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        controller.select_file("cancel.txt")
        controller.child.send(Keys.COPY)
        controller.child.expect("COPY")
        controller.input_text("cancel_copy.txt")
        controller.child.expect("To Directory")
        controller.child.send(Keys.ESC)
        time.sleep(0.4)
    finally:
        controller.quit()

    assert _snapshot_tree(root) == before


def test_same_path_copy_rejection_keeps_tree_snapshot_identical(ytree_binary, tmp_path):
    root = tmp_path / "same_path_copy_integrity"
    root.mkdir()
    (root / "same.txt").write_text("same payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        controller.select_file("same.txt")
        controller.child.send(Keys.COPY)
        controller.child.expect("COPY")
        controller.input_text("same.txt")
        controller.child.expect("To Directory")
        controller.input_text("")
        time.sleep(0.6)
    finally:
        controller.quit()

    assert _snapshot_tree(root) == before


def test_archive_copy_rewrite_updates_member_hashes_deterministically(ytree_binary, tmp_path):
    import io

    root = tmp_path / "archive_rewrite_integrity"
    root.mkdir()
    source_file = root / "archive_source.txt"
    source_file.write_text("archive payload", encoding="utf-8")

    archive_path = root / "dst.tar"
    with tarfile.open(archive_path, "w") as archive:
        payload = b"keep payload"
        info = tarfile.TarInfo(name="keep.txt")
        info.size = len(payload)
        info.mode = 0o644
        info.mtime = int(time.time())
        archive.addfile(info, io.BytesIO(payload))

    before_members = _snapshot_archive(archive_path)

    controller = YtreeController(ytree_binary, str(root))
    try:
        controller.wait_for_startup()
        controller.child.send(Keys.LOG)
        time.sleep(0.2)
        controller.input_text(str(archive_path))
        controller.child.expect("ARCHIVE")
        controller.child.send("\\")
        time.sleep(0.7)
        controller.child.send(Keys.ESC)
        time.sleep(0.3)

        _copy_selected_file(controller, "archive_source.txt", "copied_into_archive.txt", archive_path)
    finally:
        controller.quit()

    after_members = _snapshot_archive(archive_path)
    expected_members = dict(before_members)
    expected_members["copied_into_archive.txt"] = _sha256_file(source_file)
    assert after_members == expected_members


def test_copy_and_move_contracts_guard_cross_device_and_partial_cleanup():
    copy_src = _read("src/cmd/copy.c")
    move_src = _read("src/cmd/move.c")

    assert "if (realpath(from_path, res_from) && realpath(to_path, res_to))" in copy_src
    assert "if (!strcmp(res_from, res_to))" in copy_src
    assert "if (write(o, buffer, n) != n)" in copy_src
    assert "(void)unlink(to_path);" in copy_src

    assert "if (errno == EXDEV)" in move_src
    assert "CopyFileContent(ctx, to_path, from_path, s) == 0" in move_src
    assert "if (unlink(from_path) == 0)" in move_src
