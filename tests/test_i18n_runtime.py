from pathlib import Path
import os
import subprocess
import sys
import textwrap


REPO_ROOT = Path(__file__).resolve().parents[1]
COMPILE_MO = REPO_ROOT / "scripts" / "compile_mo.py"
PO_FILE = REPO_ROOT / "po" / "de.po"
BINARY = REPO_ROOT / "build" / "ytnova"


def test_cli_usage_honors_user_locale_catalog(tmp_path):
    locale_dir = tmp_path / "data" / "locale" / "de" / "LC_MESSAGES"
    locale_dir.mkdir(parents=True)
    mo_file = locale_dir / "ytnova.mo"

    subprocess.run(
        [sys.executable, str(COMPILE_MO), str(PO_FILE), str(mo_file)],
        check=True,
        cwd=REPO_ROOT,
    )

    env = os.environ.copy()
    env["HOME"] = str(tmp_path / "home")
    env["XDG_DATA_HOME"] = str(tmp_path / "data")
    env["LANG"] = "de_DE.UTF-8"
    env["LC_ALL"] = "de_DE.UTF-8"

    result = subprocess.run(
        [str(BINARY), "--bogus"],
        cwd=REPO_ROOT,
        env=env,
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert "Verwendung:" in result.stderr


def test_cli_option_errors_support_positional_locale_placeholders(tmp_path):
    locale_dir = tmp_path / "data" / "locale" / "de" / "LC_MESSAGES"
    locale_dir.mkdir(parents=True)
    po_file = tmp_path / "de.po"
    mo_file = locale_dir / "ytnova.mo"

    po_file.write_text(
        textwrap.dedent(
            """
            msgid ""
            msgstr ""
            "Content-Type: text/plain; charset=UTF-8\\n"

            msgid "Option %s requires an argument\\n"
            msgstr "Argument für %1$s fehlt\\n"
            """
        ).lstrip(),
        encoding="utf-8",
    )
    subprocess.run(
        [sys.executable, str(COMPILE_MO), str(po_file), str(mo_file)],
        check=True,
        cwd=REPO_ROOT,
    )

    env = os.environ.copy()
    env["HOME"] = str(tmp_path / "home")
    env["XDG_DATA_HOME"] = str(tmp_path / "data")
    env["LANG"] = "de_DE.UTF-8"
    env["LC_ALL"] = "de_DE.UTF-8"

    result = subprocess.run(
        [str(BINARY), "-p"],
        cwd=REPO_ROOT,
        env=env,
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert result.stderr == "Argument für -p fehlt\n"
