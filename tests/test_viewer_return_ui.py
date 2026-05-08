import time
import tarfile
import shutil

from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeTUI
from ytree_keys import Keys


def _create_archive(root, archive_name="sample.tar"):
    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")

    archive_path = root / archive_name
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)
    return archive_path


def test_external_viewer_return_restores_full_ui_frame(tmp_path, ytree_binary):
    root = tmp_path / "external_viewer_return"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")

    pager_script = root / "pager_clobber.sh"
    pager_script.write_text(
        "#!/bin/sh\n"
        "printf '\\033[2J\\033[HEXTERNAL VIEWER\\n'\n"
        "cat \"$1\" >/dev/null\n",
        encoding="utf-8",
    )
    pager_script.chmod(0o755)
    (root / ".ytree").write_text(
        f"[GLOBAL]\nPAGER={pager_script}\n", encoding="utf-8"
    )

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert "file" in _footer_text(tui).lower(), _screen_text(tui)

        tui.send_keystroke("v", wait=0.8)
        screen = _screen_text(tui)
        footer = _footer_text(tui).lower()
        assert "Path:" in screen, f"Header row not restored after external viewer.\n{screen}"
        assert "hex" in footer and "compare" in footer, (
            "Footer row not restored after external viewer.\n"
            f"Footer:\n{footer}\n\nScreen:\n{screen}"
        )
    finally:
        tui.quit()


def test_internal_viewer_return_restores_full_ui_frame(tmp_path, ytree_binary):
    root = tmp_path / "internal_viewer_return"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert "file" in _footer_text(tui).lower(), _screen_text(tui)

        tui.send_keystroke("h", wait=0.7)
        tui.send_keystroke("q", wait=0.8)

        screen = _screen_text(tui)
        footer = _footer_text(tui).lower()
        assert "Path:" in screen, f"Header row not restored after internal viewer.\n{screen}"
        assert "hex" in footer and "compare" in footer, (
            "Footer row not restored after internal viewer.\n"
            f"Footer:\n{footer}\n\nScreen:\n{screen}"
        )
    finally:
        tui.quit()


def test_internal_viewer_return_in_archive_mode_restores_full_ui_frame(
    tmp_path, ytree_binary
):
    root = tmp_path / "internal_viewer_archive_return"
    root.mkdir()
    _create_archive(root, "archive.tar")

    tui = YtreeTUI(executable=ytree_binary, cwd=str(root))
    time.sleep(0.8)

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.LOG, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.9)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0), _screen_text(tui)

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert "file" in _footer_text(tui).lower(), _screen_text(tui)

        tui.send_keystroke("h", wait=0.7)
        tui.send_keystroke("q", wait=0.8)

        screen = _screen_text(tui)
        footer = _footer_text(tui).lower()
        assert "ARCHIVE" in screen, f"Archive context not restored after internal viewer.\n{screen}"
        assert "compare" in footer and "hex" in footer, (
            "Archive file footer not restored after internal viewer.\n"
            f"Footer:\n{footer}\n\nScreen:\n{screen}"
        )
    finally:
        tui.quit()
