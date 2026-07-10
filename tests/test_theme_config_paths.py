from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _read(relative_path):
    return (REPO_ROOT / relative_path).read_text()


def test_config_paths_prefer_xdg_and_use_home_fallback_only_when_xdg_is_unavailable():
    defs = _read("include/ytnova_defs.h")
    init_source = _read("src/core/init.c")
    edit_source = _read("src/ui/ui_edit_config.c")

    assert '#define PROFILE_CONFIG_HOME_PATH ".config/ytnova/ytnova.conf"' in defs
    assert '#define PROFILE_CONFIG_HOME_PARENT ".config"' in defs
    assert '#define PROFILE_FILENAME ".ytnova"' in defs
    assert "PROFILE_CONFIG_HOME_PATH" in init_source
    assert "PROFILE_FILENAME" in init_source
    assert "PROFILE_CONFIG_HOME_PATH" in edit_source
    assert "PROFILE_CONFIG_HOME_DIR" in edit_source
    assert "access(profile_path, F_OK) != 0" not in edit_source
    assert "access(themes_path, F_OK) != 0" not in edit_source
