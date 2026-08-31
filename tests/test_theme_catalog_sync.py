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

