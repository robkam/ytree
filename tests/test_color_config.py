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
    "help_topic",
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

_CATALOG_REASON = (
    "The packaged configuration/catalog is a published machine-consumed "
    "template; runtime execution cannot safely prove distribution drift."
)
_DOC_REASON = (
    "Published documentation is a user-facing contract; runtime execution cannot "
    "safely prove generated and installed help remains accurate."
)


def _assert_catalog(condition):
    assert condition, _CATALOG_REASON


def _assert_doc(condition):
    assert condition, _DOC_REASON


def _theme_section(source, section_name):
    marker = f"[{section_name}]"
    start = source.index(marker) + len(marker)
    next_section = source.find("\n[", start)
    return source[start:] if next_section == -1 else source[start:next_section]


def _theme_role_names(section):
    roles = set()
    for line in section.splitlines():
        line = line.strip().lstrip("#").strip()
        if "=" in line:
            roles.add(line.split("=", 1)[0].strip())
    return roles


def test_packaged_config_delegates_theme_details_to_theme_catalog():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    for source in (conf_source, template_source):
        _assert_catalog("THEME=quiet-blue" in source)
        _assert_catalog("[COLORS]" not in source)
        _assert_catalog("[FILE_COLORS]" not in source)


def test_packaged_theme_catalog_defines_required_semantic_roles():
    source = _read_source("etc/ytnova.themes")
    for name in ("theme quiet-blue", "theme bash-black"):
        _assert_catalog(_theme_role_names(_theme_section(source, name)) == THEME_ROLES)


def test_packaged_config_selects_classic_theme():
    conf_source = _read_source("etc/ytnova.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    for source in (conf_source, template_source):
        _assert_catalog("THEME=quiet-blue" in source)
        _assert_catalog("# THEME=bash-black" in source)


def test_packaged_theme_catalog_uses_compact_file_palettes():
    source = _read_source("etc/ytnova.themes")
    for section_name in ("file-types quiet-blue", "file-types bash-black"):
        section = _theme_section(source, section_name)
        _assert_catalog("LINK" in section)
        _assert_catalog("EXEC" in section)
        _assert_catalog("_COLOR" not in section)


def test_spec_documents_user_visible_theme_contract():
    spec_source = _read_source("docs/SPECIFICATION.md")
    _assert_doc("## 7. Theme and Color Contract" in spec_source)
    _assert_doc("Themes are plain-text user-editable files separate from the main configuration" in spec_source)
    _assert_doc("runtime binaries must not consult `etc/` directly" in spec_source)
    _assert_doc("Rules are first-match-wins" in spec_source)


def test_manpage_documents_user_visible_theme_contract():
    for path in ("etc/ytnova.1.md", "docs/USAGE.md"):
        source = _read_source(path)
        _assert_doc("compiled-in defaults" in source)
        _assert_doc("first matching extension or special selector wins" in source)
        _assert_doc("F12" not in source)


def test_architecture_documents_theme_boundaries():
    arch_source = _read_source("docs/ARCHITECTURE.md")
    _assert_doc("Theme Configuration Boundary" in arch_source)
    _assert_doc("File-Type Palette Boundary" in arch_source)
    _assert_doc("must not style directory tree rows" in arch_source)
