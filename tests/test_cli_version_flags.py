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
    legacy_profile = home / ".ytnova"

    first = _run_cmd(ytnova_binary, ["--init"], home=home)
    assert first.returncode == 0
    assert profile.exists()
    assert not legacy_profile.exists()
    assert "Created profile:" in first.stdout
    created = profile.read_text(encoding="utf-8")
    assert "# YtreeNova Defaults" in created
    assert "[GLOBAL]" in created
    assert "THEME=classic-blue" in created

    profile.write_text("SENTINEL\n", encoding="utf-8")
    second = _run_cmd(ytnova_binary, ["--init"], home=home)
    assert second.returncode == 0
    assert profile.read_text(encoding="utf-8") == "SENTINEL\n"
    assert "already exists; not overwritten" in second.stdout

def test_init_with_explicit_profile_path_preserves_target(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    home.mkdir()
    profile = tmp_path / "explicit.conf"

    result = _run_cmd(ytnova_binary, ["--init", "-p", str(profile)], home=home)
    assert result.returncode == 0
    assert profile.exists()
    assert not (home / ".config" / "ytnova" / "ytnova.conf").exists()
    assert "Created profile:" in result.stdout
    assert "THEME=classic-blue" in profile.read_text(encoding="utf-8")
