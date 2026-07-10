import subprocess
from pathlib import Path


def _read(path):
    return Path(path).read_text(encoding="utf-8")


def test_theme_catalog_drift_checker_rejects_stale_header(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    stale_header = tmp_path / "default_theme_catalog.h"
    stale_header.write_text(
        _read("src/core/default_theme_catalog.h").replace(
            "box_lines = cyan", "box_lines = white on magenta", 1
        ),
        encoding="utf-8",
    )

    result = subprocess.run(
        [
            "python3",
            "scripts/generate_theme_catalog.py",
            "--source",
            "etc/ytnova.themes",
            "--header",
            str(stale_header),
            "--check",
        ],
        cwd=repo_root,
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert "drift" in (result.stdout + result.stderr).lower()


def test_main_border_window_uses_content_background_role():
    theme_source = _read("etc/ytnova.themes")
    contrast_theme = theme_source.replace(
        "box_lines = cyan", "box_lines = white on magenta", 1
    ).replace("box_lines = grey", "box_lines = black on cyan", 1)
    init_source = _read("src/core/init.c")
    display_source = _read("src/ui/display.c")

    assert "box_lines = white on magenta" in contrast_theme
    assert "box_lines = black on cyan" in contrast_theme
    assert (
        "CoreInitWbkgdSet(ctx, ctx->ctx_border_window,\n"
        "                     COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));"
    ) in init_source
    assert "wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);" in display_source
