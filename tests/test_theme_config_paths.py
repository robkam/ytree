from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _read(relative_path):
    return (REPO_ROOT / relative_path).read_text()


def test_config_paths_prefer_xdg_and_use_home_fallback_only_when_xdg_is_unavailable():
    defs = _read("include/ytnova_defs.h")
    config_paths_source = _read("src/core/config_paths.c")
    init_source = _read("src/core/init.c")
    edit_source = _read("src/ui/ui_edit_config.c")
    history_source = _read("src/util/history_utils.c")
    quit_source = _read("src/core/quit.c")

    assert '#define PROFILE_CONFIG_HOME_PATH ".config/ytnova/ytnova.conf"' in defs
    assert '#define PROFILE_CONFIG_HOME_PARENT ".config"' in defs
    assert '#define PROFILE_FILENAME ".ytnova"' in defs
    assert '#define HISTORY_STATE_HOME_ENV "XDG_STATE_HOME"' in defs
    assert '#define HISTORY_STATE_HOME_PATH "ytnova/ytnova.hst"' in defs
    assert '#define HISTORY_STATE_HOME_FALLBACK ".local/state/ytnova/ytnova.hst"' in defs
    assert '#define HISTORY_LEGACY_FILENAME ".ytnova-hst"' in defs
    assert "PROFILE_CONFIG_HOME_PATH" in config_paths_source
    assert "PROFILE_FILENAME" in config_paths_source
    assert "ConfigPaths_ResolveActiveEditPath" in edit_source
    assert "CONFIG_SURFACE_PROFILE" in edit_source
    assert "CreateProfileFromRuntimeState" in edit_source
    assert "HISTORY_STATE_HOME_ENV" in history_source
    assert "HISTORY_STATE_HOME_PATH" in history_source
    assert "HISTORY_STATE_HOME_FALLBACK" in history_source
    assert "ResolvePreferredHistoryPath" in init_source
    assert "ResolveLegacyHistoryPath" in init_source
    assert "ctx->history_file_path" in init_source
    assert "ctx->history_file_path" in quit_source
    assert "access(themes_path, F_OK) != 0" not in edit_source
