from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _read(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")


def test_archive_preview_cache_uses_shared_tempfile_creator() -> None:
    src = _read("src/ui/view_preview.c")
    assert "Path_CreateTempFile(preview_cache_file, sizeof(preview_cache_file)," in src
    assert '"ytree_preview_"' in src
    assert "/tmp/ytree_preview_XXXXXX" not in src


def test_archive_execute_view_hex_use_shared_tempfile_creator() -> None:
    execute_src = _read("src/cmd/execute.c")
    view_src = _read("src/cmd/view.c")
    hex_src = _read("src/cmd/hex.c")

    assert "Path_CreateTempFile(temp_path, sizeof(temp_path), \"ytree_execute_\"" in execute_src
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytree_view_\"" in view_src
    assert "Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytree_hex_\"" in hex_src

    assert "/tmp/ytree_XXXXXX" not in execute_src
    assert "/tmp/ytree_view_XXXXXX" not in view_src
    assert "/tmp/ytree_hex_XXXXXX" not in hex_src


def test_archive_tempfile_cleanup_paths_remain_present() -> None:
    execute_src = _read("src/cmd/execute.c")
    view_src = _read("src/cmd/view.c")
    hex_src = _read("src/cmd/hex.c")
    preview_src = _read("src/ui/view_preview.c")

    assert "if (fd_tmp != -1)" in execute_src
    assert "temp_path[0] != '\\0'" in execute_src
    assert "unlink(temp_path);" in execute_src

    assert "if (fd != -1)" in view_src
    assert "if (temp_filename[0] != '\\0')" in view_src
    assert "unlink(temp_filename);" in view_src

    assert "if (fd != -1)" in hex_src
    assert "if (temp_filename[0] != '\\0')" in hex_src
    assert "unlink(temp_filename);" in hex_src

    assert "if (fd != -1)" in preview_src
    assert "InvalidatePreviewCache();" in preview_src


def test_tagged_archive_view_temp_root_avoids_fixed_tmp_template_name() -> None:
    src = _read("src/ui/interactions.c")
    assert "/tmp/ytree_view_XXXXXX" not in src
    assert "Path_BuildTempTemplate(temp_dir_template, sizeof(temp_dir_template)," in src
    assert '"ytree_view_")' in src
    assert "temp_dir = mkdtemp(temp_dir_template);" in src
    assert "recursive_rmdir(temp_dir);" in src


def test_debug_logging_requires_explicit_env_path_without_tmp_fallback() -> None:
    src = _read("include/ytree_debug.h")
    assert "getenv(YTREE_DEBUG_LOG_PATH_ENV)" in src
    assert "if (ytree_debug_log_path_valid)" in src
    assert "fopen(ytree_debug_log_path, \"a\")" in src
    assert "/tmp/ytree_" not in src


def test_ui_tmp_debug_keystroke_logs_not_present() -> None:
    key_engine = _read("src/ui/key_engine.c")
    ctrl_file = _read("src/ui/ctrl_file.c")
    dir_ops = _read("src/ui/dir_ops.c")
    debug_header = _read("include/ytree_debug.h")

    assert "/tmp/ytree_wgetch.log" not in key_engine
    assert "/tmp/ytree_debug_exit.log" not in ctrl_file
    assert "/tmp/ytree_debug_switch.log" not in dir_ops
    assert "YTREE_ENABLE_KEYSTROKE_DEBUG_LOG" in debug_header
    assert "#define YTREE_ENABLE_KEYSTROKE_DEBUG_LOG 0" in debug_header
