from pathlib import Path


def _read(path):
    return Path(path).read_text(encoding="utf-8")


def test_f2_footer_uses_required_theme_command_strip():
    source = _read("src/ui/f2_picker.c")

    assert '"(L)og  (<)/(>) Cycle"' in source
    assert '"[ (L)og (< >) Cycle ]"' not in source


def test_volume_menu_uses_required_theme_command_strip():
    source = _read("src/ui/volume_menu.c")

    assert (
        '"Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete"'
        in source
    )
    assert "Use UP/DOWN to select" not in source


def test_f10_surface_uses_required_command_strip_and_enter_default():
    source = _read("src/ui/ui_edit_config.c")

    assert '"(C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit"' in source
    assert 'case CR:' in source
    assert 'case LF:' in source
    assert 'case \'C\':' in source
    assert 'case \'T\':' in source
    assert 'case \'R\':' in source


def test_reload_failures_use_status_line_without_success_message():
    source = _read("src/ui/ui_edit_config.c")

    assert "UI_ShowStatusLineError" in source
    assert "Reload failed: can't read config" in source
    assert "Reload failed: can't load theme" in source
    assert "Reloaded" not in source
    assert "reload successful" not in source.lower()
