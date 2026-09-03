import os
import subprocess


def _assert_init_config_surfaces(binary, env, config_home):
    result = subprocess.run([binary, "--init"], capture_output=True, text=True, env=env)

    assert result.returncode == 0, result.stderr
    config_dir = config_home / "ytnova"
    for filename in (
        "ytnova.conf",
        "commands.conf",
        "themes.conf",
        "applications.conf",
    ):
        assert (config_dir / filename).exists(), (
            "--init must create every user configuration surface under "
            "$XDG_CONFIG_HOME/ytnova when XDG_CONFIG_HOME is set"
        )


def test_init_uses_xdg_config_home_for_every_config_surface(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    xdg_config_home = tmp_path / "xdg-config"
    home.mkdir()

    env = os.environ.copy()
    env["HOME"] = str(home)
    env["XDG_CONFIG_HOME"] = str(xdg_config_home)
    _assert_init_config_surfaces(ytnova_binary, env, xdg_config_home)
    assert not (home / ".config" / "ytnova").exists()


def test_init_uses_absolute_xdg_config_home_without_home(ytnova_binary, tmp_path):
    xdg_config_home = tmp_path / "xdg-config"
    env = os.environ.copy()
    env.pop("HOME", None)
    env["XDG_CONFIG_HOME"] = str(xdg_config_home)

    _assert_init_config_surfaces(ytnova_binary, env, xdg_config_home)


def test_init_ignores_relative_xdg_config_home(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    home.mkdir()
    env = os.environ.copy()
    env["HOME"] = str(home)
    env["XDG_CONFIG_HOME"] = "relative-config"

    result = subprocess.run(
        [ytnova_binary, "--init"], capture_output=True, text=True, env=env, cwd=tmp_path
    )

    assert result.returncode == 0, result.stderr
    assert (home / ".config" / "ytnova" / "ytnova.conf").exists()
    assert not (tmp_path / "relative-config").exists()
