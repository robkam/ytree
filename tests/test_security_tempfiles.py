import re
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


TEMPFILE_SECURITY_INVARIANT = (
    "Temporary-file security invariant: production paths never reintroduce predictable "
    "temporary names or omit cleanup."
)


def _assert_invariant(condition: bool) -> None:
    assert condition, (
        f"{TEMPFILE_SECURITY_INVARIANT} Runtime execution cannot safely prove the "
        "global absence of unsafe temporary-path or cleanup regressions."
    )


def _read(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")


def test_archive_preview_cache_uses_shared_tempfile_creator() -> None:
    src = _read("src/ui/view_preview.c")
    _assert_invariant("Path_CreateTempFile(preview_cache_file, sizeof(preview_cache_file)," in src)
    _assert_invariant('"ytnova_preview_"' in src)
    _assert_invariant("/tmp/ytnova_preview_XXXXXX" not in src)


def test_archive_execute_view_hex_use_shared_tempfile_creator() -> None:
    execute_src = _read("src/cmd/execute.c")
    view_src = _read("src/cmd/view.c")
    hex_src = _read("src/cmd/hex.c")

    _assert_invariant("Path_CreateTempFile(temp_path, sizeof(temp_path), \"ytnova_execute_\"" in execute_src)
    _assert_invariant("Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytnova_view_\"" in view_src)
    _assert_invariant("Path_CreateTempFile(temp_filename, sizeof(temp_filename), \"ytnova_hex_\"" in hex_src)

    _assert_invariant("/tmp/ytnova_XXXXXX" not in execute_src)
    _assert_invariant("/tmp/ytnova_view_XXXXXX" not in view_src)
    _assert_invariant("/tmp/ytnova_hex_XXXXXX" not in hex_src)


def test_archive_tempfile_cleanup_paths_remain_present() -> None:
    execute_src = _read("src/cmd/execute.c")
    view_src = _read("src/cmd/view.c")
    hex_src = _read("src/cmd/hex.c")
    preview_src = _read("src/ui/view_preview.c")

    _assert_invariant("if (fd_tmp != -1)" in execute_src)
    _assert_invariant("temp_path[0] != '\\0'" in execute_src)
    _assert_invariant("unlink(temp_path);" in execute_src)

    _assert_invariant("if (fd != -1)" in view_src)
    _assert_invariant("if (temp_filename[0] != '\\0')" in view_src)
    _assert_invariant("unlink(temp_filename);" in view_src)

    _assert_invariant("if (fd != -1)" in hex_src)
    _assert_invariant("if (temp_filename[0] != '\\0')" in hex_src)
    _assert_invariant("unlink(temp_filename);" in hex_src)

    _assert_invariant("if (fd != -1)" in preview_src)
    _assert_invariant("InvalidatePreviewCache();" in preview_src)


def test_tagged_archive_view_temp_root_avoids_fixed_tmp_template_name() -> None:
    src = _read("src/ui/tagged_view.c")
    _assert_invariant("/tmp/ytnova_view_XXXXXX" not in src)
    _assert_invariant("char (*temp_dir_template)[PATH_LENGTH]" in src)
    _assert_invariant(
        re.search(
            r"Path_BuildTempTemplate\(\*temp_dir_template,\s*"
            r"sizeof\(\*temp_dir_template\),",
            src,
        )
        is not None
    )
    _assert_invariant(
        re.search(
            r"PrepareTaggedView\(ctx,\s*s,\s*&temp_dir_template,\s*&temp_dir",
            src,
        )
        is not None
    )
    _assert_invariant('"ytnova_view_")' in src)
    _assert_invariant("*temp_dir_out = mkdtemp(*temp_dir_template);" in src)
    _assert_invariant("recursive_rmdir(temp_dir);" in src)


def test_debug_logging_requires_explicit_env_path_without_tmp_fallback() -> None:
    src = _read("include/ytnova_debug.h")
    _assert_invariant("getenv(YTNOVA_DEBUG_LOG_PATH_ENV)" in src)
    _assert_invariant("if (ytnova_debug_log_path_valid)" in src)
    _assert_invariant("fopen(ytnova_debug_log_path, \"a\")" in src)
    _assert_invariant("/tmp/ytnova_" not in src)


def test_ui_tmp_debug_keystroke_logs_not_present() -> None:
    key_engine = _read("src/ui/key_engine.c")
    ctrl_file = _read("src/ui/ctrl_file.c")
    dir_ops = _read("src/ui/dir_ops.c")
    debug_header = _read("include/ytnova_debug.h")

    _assert_invariant("/tmp/ytnova_wgetch.log" not in key_engine)
    _assert_invariant("/tmp/ytnova_debug_exit.log" not in ctrl_file)
    _assert_invariant("/tmp/ytnova_debug_switch.log" not in dir_ops)
    _assert_invariant("YTNOVA_ENABLE_KEYSTROKE_DEBUG_LOG" in debug_header)
    _assert_invariant("#define YTNOVA_ENABLE_KEYSTROKE_DEBUG_LOG 0" in debug_header)
