from helpers_source import read_repo_source as _read_source

THEME_ROLES = {
    "background",
    "box_lines",
    "tree_lines",
    "margin",
    "static_text",
    "dynamic_text",
    "keybind",
    "selection",
    "dialog",
    "picker",
    "help",
    "info",
    "warning",
    "error",
    "search_hit",
    "disabled",
}


def test_dialog_color_key_is_defined_in_runtime_palette():
    defs_source = _read_source("include/ytnova_defs.h")
    color_source = _read_source("src/ui/color.c")

    assert "CPAIR_DIALOG" in defs_source
    assert "CPAIR_MENU,\n  CPAIR_DIALOG,\n  CPAIR_WINERR," in defs_source
    assert '{"DIALOG_COLOR", CPAIR_DIALOG, 7, 0},' in color_source


def test_dialog_color_key_is_documented_in_config_templates():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    assert conf_source.count("DIALOG_COLOR=") >= 2
    assert template_source.count("DIALOG_COLOR=") >= 2


def test_modal_severity_comment_lines_include_non_bright_equivalents():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    info_comment = "# INFO_COLOR=15,4     # Bright White on Blue (non-bright: 7,4)"
    warn_comment = "# WARN_COLOR=11,0     # Bright Yellow on Black (non-bright: 3,0)"
    err_comment = "# ERR_COLOR=15,1      # Bright White on Red (non-bright: 7,1)"

    for source in (conf_source, template_source):
        assert info_comment in source
        assert warn_comment in source
        assert err_comment in source


def _theme_section(source, section_name):
    marker = f"[{section_name}]"
    start = source.index(marker) + len(marker)
    next_section = source.find("\n[", start)
    if next_section == -1:
        return source[start:]
    return source[start:next_section]


def _theme_role_names(section):
    roles = set()
    for line in section.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        roles.add(stripped.split("=", 1)[0].strip())
    return roles


def test_packaged_theme_catalog_defines_required_semantic_roles():
    source = _read_source("etc/ytnova.themes")

    for theme_name in ("theme classic-blue", "theme bash-black"):
        section = _theme_section(source, theme_name)
        assert _theme_role_names(section) == THEME_ROLES

    classic = _theme_section(source, "theme classic-blue")
    assert "background = blue" in classic
    assert "margin = dynamic_text" in classic
    assert "error = +white on red" in classic
    assert "warning = black on yellow" in classic
    assert "search_hit = black on yellow" in classic


def test_packaged_config_selects_classic_theme():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    profile_source = _read_source("src/cmd/profile.c")

    assert "THEME=classic-blue" in conf_source
    assert "THEME=classic-blue" in template_source
    assert '{"THEME", "classic-blue", NULL, NULL},' in profile_source


def test_packaged_theme_catalog_uses_compact_file_palettes():
    source = _read_source("etc/ytnova.themes")

    assert "[file-types classic-blue]" in source
    assert "[file-types bash-black]" in source
    assert "_COLOR" not in source
    assert "bright black" not in source.lower()

    for section_name in ("file-types classic-blue", "file-types bash-black"):
        section = _theme_section(source, section_name)
        assert "LINK" in section
        assert "EXEC" in section
        assert "\nDIR" not in section
        assert "*." not in section
        assert ":" in section
