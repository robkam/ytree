import os
import subprocess


def _run_cmd(binary, args, home=None):
    env = os.environ.copy()
    if home is not None:
        env["HOME"] = str(home)
    return subprocess.run([binary] + args, capture_output=True, text=True, env=env)


def test_lowercase_v_prints_version(ytnova_binary):
    result = _run_cmd(ytnova_binary, ["-v"])
    assert result.returncode == 0
    assert result.stdout.startswith("ytnova ")


def test_uppercase_v_prints_version(ytnova_binary):
    result = _run_cmd(ytnova_binary, ["-V"])
    assert result.returncode == 0
    assert result.stdout.startswith("ytnova ")


def test_init_creates_profile_only_if_missing(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    home.mkdir()
    profile = home / ".config" / "ytnova" / "ytnova.conf"
    commands = home / ".config" / "ytnova" / "commands.conf"
    themes = home / ".config" / "ytnova" / "themes.conf"
    legacy_profile = home / ".ytnova"
    legacy_commands = home / ".ytnova.commands"
    legacy_themes = home / ".ytnova.themes"

    first = _run_cmd(ytnova_binary, ["--init"], home=home)
    assert first.returncode == 0
    assert profile.exists()
    assert commands.exists()
    assert themes.exists()
    assert not legacy_profile.exists()
    assert not legacy_commands.exists()
    assert not legacy_themes.exists()
    assert "Created profile:" in first.stdout
    assert "Created commands:" in first.stdout
    assert "Created themes:" in first.stdout
    created = profile.read_text(encoding="utf-8")
    assert "# YtreeNova Defaults" in created
    assert "[GLOBAL]" in created
    assert "THEME=quiet-blue" in created
    assert "# THEME=bash-black" in created
    assert "commands.conf" in created
    created_commands = commands.read_text(encoding="utf-8")
    assert "context | binding | shown | label | action | command" in created_commands
    assert "dir | A | A | Attributes | ACTION_CMD_A |" in created_commands
    created_themes = themes.read_text(encoding="utf-8")
    assert "[theme quiet-blue]" in created_themes
    assert "picker = black on cyan" in created_themes
    assert "picker = black on grey" in created_themes
    assert "info = white on blue" in created_themes
    assert "error = white on red" in created_themes
    assert "disabled =" not in created_themes

    profile.write_text("SENTINEL\n", encoding="utf-8")
    commands.write_text("COMMANDS SENTINEL\n", encoding="utf-8")
    themes.write_text("THEME SENTINEL\n", encoding="utf-8")
    second = _run_cmd(ytnova_binary, ["--init"], home=home)
    assert second.returncode == 0
    assert profile.read_text(encoding="utf-8") == "SENTINEL\n"
    assert commands.read_text(encoding="utf-8") == "COMMANDS SENTINEL\n"
    assert themes.read_text(encoding="utf-8") == "THEME SENTINEL\n"
    assert "already exists; not overwritten" in second.stdout

def test_init_with_explicit_profile_path_preserves_target(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    home.mkdir()
    profile = tmp_path / "explicit.conf"
    commands = home / ".config" / "ytnova" / "commands.conf"
    themes = home / ".config" / "ytnova" / "themes.conf"

    result = _run_cmd(ytnova_binary, ["--init", "-p", str(profile)], home=home)
    assert result.returncode == 0
    assert profile.exists()
    assert commands.exists()
    assert themes.exists()
    assert not (home / ".config" / "ytnova" / "ytnova.conf").exists()
    assert "Created profile:" in result.stdout
    assert "Created commands:" in result.stdout
    assert "Created themes:" in result.stdout
    profile_text = profile.read_text(encoding="utf-8")
    assert "THEME=quiet-blue" in profile_text
    assert "# THEME=bash-black" in profile_text
    assert "F12" not in profile_text
    assert "context | binding | shown | label | action | command" in commands.read_text(
        encoding="utf-8"
    )


def test_init_falls_back_to_home_targets_when_xdg_dir_is_unavailable(
    ytnova_binary, tmp_path
):
    home = tmp_path / "home"
    home.mkdir()
    (home / ".config").write_text("blocked\n", encoding="utf-8")

    profile = home / ".ytnova"
    commands = home / ".ytnova.commands"
    themes = home / ".ytnova.themes"

    result = _run_cmd(ytnova_binary, ["--init"], home=home)
    assert result.returncode == 0
    assert profile.exists()
    assert commands.exists()
    assert themes.exists()
    assert "Created profile:" in result.stdout
    assert "Created commands:" in result.stdout
    assert "Created themes:" in result.stdout
