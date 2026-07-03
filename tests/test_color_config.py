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
    assert "CPAIR_MENU,\n  CPAIR_DIALOG,\n  CPAIR_HELP," in defs_source
    assert '{"DIALOG_COLOR", CPAIR_DIALOG, 7, 0},' in color_source
    assert "CPAIR_WINERR" not in defs_source


def test_packaged_config_delegates_theme_details_to_theme_catalog():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    changes_source = _read_source("docs/CHANGES.md")

    for source in (conf_source, template_source):
        assert "THEME=classic-blue" in source
        assert "theme catalog" in source
        assert "dynamic_text" in source
        assert "[COLORS]" not in source
        assert "[FILE_COLORS]" not in source
        assert "DIR_COLOR=" not in source
        assert "FILE_COLOR=" not in source
        assert "DIALOG_COLOR=" not in source

    assert "[COLORS]" not in changes_source
    assert "[FILE_COLORS]" not in changes_source
    assert "semantic theme catalog" in changes_source


def test_winerr_color_is_migration_only_alias():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    color_source = _read_source("src/ui/color.c")
    theme_source = _read_source("src/cmd/theme.c")
    init_source = _read_source("src/core/init.c")
    error_source = _read_source("src/ui/error.c")

    assert "WINERR_COLOR" not in conf_source
    assert "WINERR_COLOR" not in template_source
    assert "CPAIR_WINERR" not in color_source
    assert '"WINERR_COLOR"' in color_source
    assert '{"error", {"ERR_COLOR", NULL}},' in theme_source
    assert '{"disabled", {"DISABLED_COLOR", NULL}},' in theme_source
    assert "migration_role_shims" in theme_source
    assert "ApplyMigrationRoleShim" in theme_source
    assert "ctx->ctx_error_window, COLOR_PAIR(CPAIR_ERR)" in init_source
    assert "CPAIR_WINERR" not in error_source


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


def test_spec_documents_user_visible_theme_contract():
    spec_source = _read_source("docs/SPECIFICATION.md")

    assert "## 7. Theme and Color Contract" in spec_source
    assert (
        "Themes are plain-text user-editable files separate from the main configuration"
        in spec_source
    )
    assert (
        "Preferred user paths are `~/.config/ytnova/ytnova.conf` and `~/.config/ytnova/themes.conf`"
        in spec_source
    )
    assert (
        "Legacy fallback user paths are `~/.ytnova` and `~/.ytnova.themes`"
        in spec_source
    )
    assert (
        "Required roles are `background`, `box_lines`, `tree_lines`, `margin`"
        in spec_source
    )
    assert "`grey`/`gray`" in spec_source
    assert "bright black" not in spec_source.lower()
    assert "Rules are first-match-wins" in spec_source
    assert "Reload is available only inside this surface" in spec_source


def test_architecture_documents_theme_boundaries():
    arch_source = _read_source("docs/ARCHITECTURE.md")

    assert "Theme Configuration Boundary" in arch_source
    assert (
        "role definitions and file-type palettes live in separate theme files"
        in arch_source
    )
    assert "Legacy color-key parsing is migration-only compatibility input" in (
        arch_source
    )
    assert "File-Type Palette Boundary" in arch_source
    assert "must not style directory tree rows" in arch_source
