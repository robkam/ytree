from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _read(relpath: str) -> str:
    return (REPO_ROOT / relpath).read_text(encoding="utf-8")


def test_archive_preview_cache_avoids_fixed_tmp_template_name() -> None:
    src = _read("src/ui/view_preview.c")
    assert "/tmp/ytree_preview_XXXXXX" not in src
    assert "Path_BuildTempTemplate(cache_template, sizeof(cache_template)," in src
    assert '"ytree_preview_")' in src


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
