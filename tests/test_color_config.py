import re

from helpers_source import read_repo_source as _read_source

THEME_ROLES = {
    "background",
    "box_lines",
    "tree_lines",
    "margin",
    "static_text",
    "dynamic_text",
    "keybind",
    "footer",
    "selection",
    "dialog",
    "picker",
    "picker_selection",
    "help",
    "help_footer",
    "help_heading",
    "help_term",
    "help_attention",
    "help_alert",
    "help_keybind",
    "help_link",
    "help_link_selection",
    "help_box_lines",
    "info",
    "warning",
    "error",
    "search_hit",
}


def test_dialog_color_key_is_defined_in_runtime_palette():
    defs_source = _read_source("include/ytnova_defs.h")
    color_source = _read_source("src/ui/color.c")

    assert "UI_ROLE_DIALOG" in defs_source
    assert "CPAIR_DIALOG" not in defs_source
    assert '{"dialog", UI_ROLE_DIALOG, 7, 0}' in color_source
    assert "CPAIR_WINERR" not in defs_source


def test_packaged_config_delegates_theme_details_to_theme_catalog():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    changes_source = _read_source("docs/CHANGELOG.md")

    for source in (conf_source, template_source):
        assert "Built-in default profile template for ~/.config/ytnova/ytnova.conf." in source
        assert "If the XDG config path cannot be used, ytnova falls back to ~/.ytnova." in source
        assert "F10 Config and `ytnova --init` can write it for you." in source
        assert "THEME=quiet-blue" in source
        assert "# THEME=bash-black" in source
        assert "Built-in starter themes" in source
        assert "leave one uncommented" in source
        assert "semantic roles and file-type palette rules" in source
        assert "Theme customization lives in ~/.config/ytnova/themes.conf." not in source
        assert "[COLORS]" not in source
        assert "[FILE_COLORS]" not in source
        assert "DIR_COLOR=" not in source
        assert "FILE_COLOR=" not in source
        assert "DIALOG_COLOR=" not in source

    assert "[COLORS]" not in changes_source
    assert "[FILE_COLORS]" not in changes_source
    assert "semantic theme catalog" in changes_source


def test_winerr_color_is_not_runtime_alias():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    color_source = _read_source("src/ui/color.c")
    theme_source = _read_source("src/cmd/theme.c")
    init_source = _read_source("src/core/init.c")
    error_source = _read_source("src/ui/error.c")

    assert "WINERR_COLOR" not in conf_source
    assert "WINERR_COLOR" not in template_source
    assert "CPAIR_WINERR" not in color_source
    assert '"WINERR_COLOR"' not in color_source
    assert '"error"' in theme_source
    assert '"disabled"' in theme_source
    assert "legacy_color_aliases" not in color_source
    assert "ApplyMigrationRoleShim" not in theme_source
    assert "ctx->ctx_error_window, COLOR_PAIR(UI_ROLE_ERROR)" in init_source
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
        if not stripped:
            continue
        if stripped.startswith("#"):
            stripped = stripped[1:].strip()
        if "=" not in stripped:
            continue
        roles.add(stripped.split("=", 1)[0].strip())
    return roles


def _theme_visible_role_order(section):
    roles = []
    for line in section.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            stripped = stripped[1:].strip()
        if "=" not in stripped:
            continue
        roles.append(stripped.split("=", 1)[0].strip())
    return roles


def test_packaged_theme_catalog_defines_required_semantic_roles():
    source = _read_source("etc/ytnova.themes")
    norton_blue = _theme_section(source, "theme norton-blue")
    classic = _theme_section(source, "theme quiet-blue")
    bash_black = _theme_section(source, "theme bash-black")

    assert "help = black on white\nhelp_footer = help\nhelp_heading = blue\nhelp_term = help_heading\nhelp_attention = help_term\nhelp_alert = help_attention\nhelp_keybind = yellow\nhelp_link = black on cyan\nhelp_link_selection = yellow on cyan\n# help_box_lines = black on white\n# uses fallback from help fg/bg\n" in norton_blue

    for section in (classic, bash_black):
        assert _theme_role_names(section) == THEME_ROLES

    assert _theme_visible_role_order(classic) == _theme_visible_role_order(bash_black)

    assert "background = blue" in classic
    assert "# margin = dynamic_text" in classic
    assert "# uses fallback from dynamic_text" in classic
    assert "error = +white on red" in classic
    assert "warning = black on yellow" in classic
    assert "search_hit = black on yellow" in classic
    assert "box_lines = cyan\n" in classic
    assert "tree_lines = +white\n" in classic
    assert "dynamic_text = +white\n" in classic
    assert "keybind = +white\n" in classic
    assert "selection = black on white\n" in classic
    assert "dialog = white\n" in classic
    assert "picker = black on cyan\n" in classic
    assert "# picker_selection = selection" in classic
    assert "# uses fallback from selection" in classic
    assert "footer = white\n" in classic
    assert "help = white\n" in classic
    assert "help_footer = help\n" in classic
    assert "help_heading = help\n" in classic
    assert "help_term = help_heading\n" in classic
    assert "help_attention = help_term\n" in classic
    assert "help_alert = help_attention\n" in classic
    assert "# help_keybind = keybind" in classic
    assert "# uses fallback from keybind on help_footer background" in classic
    assert "help_link = cyan\n" in classic
    assert "help_link_selection = yellow\n" in classic
    assert "help_box_lines = cyan on blue\n" in classic
    assert "disabled =" not in classic
    assert "\nbox_lines = cyan on blue\n" not in classic
    assert "dynamic_text = +white on blue" not in classic
    assert "background = black" in bash_black
    assert "# margin = dynamic_text" in bash_black
    assert "keybind = +white\n" in bash_black
    assert "picker = black on grey\n" in bash_black
    assert "# picker_selection = selection" in bash_black
    assert "footer = white\n" in bash_black
    assert "help = white\n" in bash_black
    assert "help_footer = help\n" in bash_black
    assert "help_heading = help\n" in bash_black
    assert "help_term = help_heading\n" in bash_black
    assert "help_attention = help_term\n" in bash_black
    assert "help_alert = help_attention\n" in bash_black
    assert "# help_keybind = keybind" in bash_black
    assert "help_link = cyan\n" in bash_black
    assert "help_link_selection = yellow\n" in bash_black
    assert "help_box_lines = grey on black\n" in bash_black
    assert "info = white on blue\n" in bash_black
    assert "error = white on red\n" in bash_black


def test_packaged_config_selects_classic_theme():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")
    profile_source = _read_source("src/cmd/profile.c")

    assert "THEME=quiet-blue" in conf_source
    assert "THEME=quiet-blue" in template_source
    assert "# THEME=bash-black" in conf_source
    assert "# THEME=bash-black" in template_source
    assert '{"THEME", "quiet-blue", NULL, NULL},' in profile_source


def test_packaged_theme_catalog_uses_compact_file_palettes():
    source = _read_source("etc/ytnova.themes")

    assert "[file-types quiet-blue]" in source
    assert "[file-types bash-black]" in source
    assert "_COLOR" not in source
    assert "bright black" not in source.lower()

    for section_name in ("file-types quiet-blue", "file-types bash-black"):
        section = _theme_section(source, section_name)
        assert "\nDIR" not in section
        assert "*." not in section

    classic = _theme_section(source, "file-types quiet-blue")
    assert "archives = red: tar,tgz,zip,gz,rar,7z,iso" in classic
    assert "scripts = +cyan: sh,bash,py,pl,rb" in classic
    assert "code = yellow: c,h,cpp,rs,go,java,js,ts" in classic
    assert "media = magenta: jpg,png,gif,mp4,mp3,wav" in classic
    assert "documents = white: pdf,txt,md,doc,docx" in classic
    assert "links = cyan: LINK" in classic
    assert "executables = green: EXEC" in classic

    bash_black = _theme_section(source, "file-types bash-black")
    assert "LINK" in bash_black
    assert "EXEC" in bash_black
    assert ":" in bash_black
    assert "links = cyan: LINK" in bash_black
    assert "executables = green: EXEC" in bash_black


def test_file_color_pair_exhaustion_cannot_reuse_semantic_roles():
    defs_source = _read_source("include/ytnova_defs.h")
    color_source = _read_source("src/ui/color.c")

    assert "FILE_COLOR_PAIR_UNASSIGNED = 0" in defs_source
    assert "rule->pair_id = UI_ROLE_DYNAMIC_TEXT" not in color_source
    assert "rule->pair_id = UI_ROLE_" not in color_source
    assert "if (rule->pair_id == FILE_COLOR_PAIR_UNASSIGNED)" in color_source


def test_file_type_palette_special_selectors_are_link_and_exec_only():
    theme_source = _read_source("src/cmd/theme.c")
    profile_source = _read_source("src/cmd/profile.c")
    color_source = _read_source("src/ui/color.c")

    assert 'strcasecmp(selector, "LINK")' in theme_source
    assert 'strcasecmp(selector, "EXEC")' in theme_source
    assert 'strcasecmp(selector, "LINK")' not in profile_source
    assert 'strcasecmp(selector, "EXEC")' not in profile_source
    assert 'strcasecmp(selector, "DIR")' not in theme_source
    assert 'strcasecmp(selector, "DIR")' not in profile_source
    assert 'strcmp(rule->pattern, "LINK")' in color_source
    assert 'strcmp(rule->pattern, "EXEC")' in color_source
    assert 'strcmp(rule->pattern, "DIR")' not in color_source


def test_spec_documents_user_visible_theme_contract():
    spec_source = _read_source("docs/SPECIFICATION.md")

    assert "## 7. Theme and Color Contract" in spec_source
    assert (
        "Themes are plain-text user-editable files separate from the main configuration"
        in spec_source
    )
    assert (
        "Preferred config-family paths are `$XDG_CONFIG_HOME/ytnova/ytnova.conf`, `$XDG_CONFIG_HOME/ytnova/themes.conf`, `$XDG_CONFIG_HOME/ytnova/commands.conf`, and `$XDG_CONFIG_HOME/ytnova/applications.conf`"
        in spec_source
    )
    assert (
        "Home-directory fallback user paths are `~/.ytnova`, `~/.ytnova.themes`, `~/.ytnova.commands`, and `~/.ytnova.applications`"
        in spec_source
    )
    assert "runtime binaries must not consult `etc/` directly" in spec_source
    assert (
        "runtime loads packaged or compiled-in default theme data without creating `~/.config/ytnova/themes.conf`"
        in spec_source
    )
    assert "The starter-theme role surface is `background`, `box_lines`, `tree_lines`, `margin`" in spec_source
    assert "Each packaged starter-theme block MUST include every semantic role either as an active assignment or as a full-line commented fallback documentation entry." in spec_source
    assert "`footer`" in spec_source
    assert "`help_footer`" in spec_source
    assert "`help_heading`" in spec_source
    assert "`help_term`" in spec_source
    assert "`help_attention`" in spec_source
    assert "`help_alert`" in spec_source
    assert "`help_link`" in spec_source
    assert "`help_link_selection`" in spec_source
    assert "`help_box_lines`" in spec_source
    assert "`disabled`" not in spec_source
    assert "`grey`/`gray`" in spec_source
    assert "bright black" not in spec_source.lower()
    assert "Rules are first-match-wins" in spec_source
    assert (
        "Command-strip words stay readable: the live UI renders the full word and highlights the bound letter in place"
        in spec_source
    )
    assert "must not capitalize the leading letter just for title-case styling" in spec_source
    assert (
        "`THEME=` selects one named theme block, role aliases stay within that theme, and omitted backgrounds inherit that theme's background unless explicitly pinned."
        in spec_source
    )
    assert "Reload is available only inside this surface" in spec_source


def test_manpage_documents_user_visible_theme_contract():
    man_source = _read_source("etc/ytnova.1.md")
    usage_source = _read_source("docs/USAGE.md")

    for source in (man_source, usage_source):
        assert "(C)onfig  co(M)mands  (T)hemes  (R)eload  (Esc)/(Q)uit" in source
        assert (
            "By default this creates `~/.config/ytnova/ytnova.conf`, `~/.config/ytnova/commands.conf`, `~/.config/ytnova/themes.conf`, and `~/.config/ytnova/applications.conf`"
            in source
        )
        assert (
            "instead of the default `~/.config/ytnova/ytnova.conf`" in source
        )
        assert "View the selected file with the configured pager." in source
        assert "~/.config/ytnova/ytnova.conf" in source
        assert "~/.config/ytnova/commands.conf" in source
        assert "~/.config/ytnova/themes.conf" in source
        assert "~/.config/ytnova/applications.conf" in source
        assert "~/.ytnova.commands" in source
        assert "~/.ytnova.themes" in source
        assert "~/.ytnova.applications" in source
        assert "/usr/share/ytnova/commands/<preset>.conf" in source or (
            "/usr/share/ytnova/commands/*.conf" in source
        )
        assert "compiled-in defaults" in source
        assert "quiet-blue" not in source
        assert "bash-black" not in source
        assert "**/**: **Incremental Jump**" in source
        assert "F12" not in source
        assert "`grey`/`gray`" in source
        assert "inherits the active filename/window background" in source
        assert "archives = red: tar,tgz,zip" in source
        assert "first matching extension or special selector wins" in source
        assert "bright black" not in source.lower()


def test_compare_helper_messages_do_not_prefer_legacy_profile_path():
    for path in ("src/ui/file_compare.c", "src/ui/dir_compare.c"):
        source = _read_source(path)

        assert "in ~/.ytnova" not in source
        assert "in the main config" in source


def test_startup_theme_load_falls_back_before_abort():
    init_source = _read_source("src/core/init.c")
    theme_source = _read_source("src/cmd/theme.c")
    defs_source = _read_source("include/ytnova_defs.h")
    spec_source = _read_source("docs/SPECIFICATION.md")

    assert "LoadTheme failed*ABORT" in init_source
    assert "load_startup_theme" in defs_source
    assert "ops->load_startup_theme = CoreInit_LoadStartupTheme;" in theme_source
    assert "ctx->core_init_ops.load_startup_theme(ctx) != 0" in init_source
    assert "int LoadStartupTheme(ViewContext *ctx)" in theme_source
    assert "If startup cannot load the selected theme from a user theme catalog" in spec_source
    assert "interactive reload still keeps the previous working theme" in spec_source
    assert re.search(
        r"if \(\(\(ctx->core_init_ops\.load_startup_theme != NULL &&\s*"
        r"ctx->core_init_ops\.load_startup_theme\(ctx\) != 0\) \|\|\s*"
        r"\(ctx->core_init_ops\.load_startup_theme == NULL &&\s*"
        r"ctx->core_init_ops\.load_theme != NULL &&\s*"
        r"ctx->core_init_ops\.load_theme\(ctx\) != 0\)\)\) \{\s*"
        r"CoreInitUINotice\(ctx, \"LoadTheme failed\*ABORT\"\);\s*"
        r"exit\(1\);\s*"
        r"\}",
        init_source,
    )
    assert "return LoadConfiguredTheme(ctx);" in theme_source


def test_architecture_documents_theme_boundaries():
    arch_source = _read_source("docs/ARCHITECTURE.md")

    assert "Theme Configuration Boundary" in arch_source
    assert (
        "role definitions and file-type palettes live in separate theme files"
        in arch_source
    )
    assert "Legacy color-key parsing" not in arch_source
    assert "File-Type Palette Boundary" in arch_source
    assert "must not style directory tree rows" in arch_source
