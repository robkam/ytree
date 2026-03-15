import pexpect
import threading
import time

from ytree_keys import Keys


def test_refresh_handles_deleted_entries_quietly(controller, sandbox):
    """
    Regression:
    If files disappear while a refresh scan is running, ytree must not block on
    "Stat failed on ... IGNORED" dialogs.
    """
    source_dir = sandbox / "source"
    yt = controller(cwd=str(source_dir))

    try:
        yt.wait_for_startup()

        for round_idx in range(4):
            files = []
            for i in range(700):
                path = source_dir / f"race_{round_idx}_{i}.tmp"
                path.write_text("x", encoding="utf-8")
                files.append(path)

            def _delete_wave():
                time.sleep(0.02)
                for file_path in files:
                    try:
                        file_path.unlink()
                    except FileNotFoundError:
                        pass

            worker = threading.Thread(target=_delete_wave)
            worker.start()

            yt.child.send(Keys.CTRL_L)
            outcome = yt.child.expect(
                [r"Stat failed on", r"20\d{2}", pexpect.TIMEOUT], timeout=3.0
            )

            worker.join()
            assert outcome != 0, (
                "Refresh displayed blocking stat error when files were deleted "
                "during scan."
            )
    finally:
        yt.quit()
