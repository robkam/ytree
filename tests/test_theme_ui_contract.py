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
