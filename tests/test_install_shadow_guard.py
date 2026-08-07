from pathlib import Path
import subprocess


def _run_shadow_check(repo_root: Path, home_dir: Path, *extra_make_args: str):
    return subprocess.run(
        ["make", "install-shadow-check", f"HOME={home_dir}", *extra_make_args],
        cwd=repo_root,
        text=True,
        capture_output=True,
        check=False,
    )


def test_install_shadow_guard_passes_without_user_local_ytnova(tmp_path):
    result = _run_shadow_check(Path.cwd(), tmp_path)

    assert result.returncode == 0, result.stdout + result.stderr


def test_install_shadow_guard_blocks_shadow_binary_and_manpage(tmp_path):
    shadow_bin = tmp_path / ".local" / "bin" / "ytnova"
    shadow_man = tmp_path / ".local" / "share" / "man" / "man1" / "ytnova.1.gz"
    shadow_bin.parent.mkdir(parents=True)
    shadow_man.parent.mkdir(parents=True)
    shadow_bin.write_text("old-binary", encoding="utf-8")
    shadow_man.write_text("old-man", encoding="utf-8")

    result = _run_shadow_check(Path.cwd(), tmp_path)

    assert result.returncode != 0
    combined = result.stdout + result.stderr
    assert "stale binary" in combined
    assert "stale man" in combined


def test_install_shadow_guard_can_be_overridden_explicitly(tmp_path):
    shadow_bin = tmp_path / ".local" / "bin" / "ytnova"
    shadow_bin.parent.mkdir(parents=True)
    shadow_bin.write_text("old-binary", encoding="utf-8")

    result = _run_shadow_check(Path.cwd(), tmp_path, "ALLOW_SHADOW_INSTALL=1")

    assert result.returncode == 0, result.stdout + result.stderr
