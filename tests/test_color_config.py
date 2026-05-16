from helpers_source import read_repo_source as _read_source


def test_dialog_color_key_is_defined_in_runtime_palette():
    defs_source = _read_source("include/ytree_defs.h")
    color_source = _read_source("src/ui/color.c")

    assert "CPAIR_DIALOG" in defs_source
    assert "CPAIR_MENU,\n  CPAIR_DIALOG,\n  CPAIR_WINERR," in defs_source
    assert '{"DIALOG_COLOR", CPAIR_DIALOG, 7, 0},' in color_source


def test_dialog_color_key_is_documented_in_config_templates():
    conf_source = _read_source("etc/ytree.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    assert conf_source.count("DIALOG_COLOR=") >= 2
    assert template_source.count("DIALOG_COLOR=") >= 2


def test_modal_severity_comment_lines_include_non_bright_equivalents():
    conf_source = _read_source("etc/ytree.conf")
    template_source = _read_source("src/core/default_profile_template.h")

    info_comment = "# INFO_COLOR=15,4     # Bright White on Blue (non-bright: 7,4)"
    warn_comment = "# WARN_COLOR=11,0     # Bright Yellow on Black (non-bright: 3,0)"
    err_comment = "# ERR_COLOR=15,1      # Bright White on Red (non-bright: 7,1)"

    for source in (conf_source, template_source):
        assert info_comment in source
        assert warn_comment in source
        assert err_comment in source
