import importlib.util
from pathlib import Path
import subprocess
import sys

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / "scripts" / "generate_help_assets.py"


spec = importlib.util.spec_from_file_location("generate_help_assets", SCRIPT_PATH)
helpgen = importlib.util.module_from_spec(spec)
assert spec.loader is not None
sys.modules[spec.name] = helpgen
spec.loader.exec_module(helpgen)


def test_help_generator_renders_runtime_topics_and_usage_projection():
    topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "help.en.md").read_text(encoding="utf-8")
    )

    manpage = helpgen.render_manpage_markdown(topics, usage_mode=False)
    usage = helpgen.render_manpage_markdown(topics, usage_mode=True)
    header = helpgen.render_runtime_header(topics)

    assert helpgen.BANNER in manpage
    assert "### Directory Mode" in manpage
    assert "**A** (Attributes): Open attributes submenu for directory metadata changes:" in manpage
    assert "### Filter Help" in manpage
    assert "Use normal glob-like patterns such as `*.c`" in manpage
    assert "Authors and contributors are listed in the AUTHORS.md file." in manpage
    assert "Authors and contributors are listed in the [AUTHORS.md](AUTHORS.md) file." in usage
    assert "generated_help_topic_count" in header
    assert '"main.dir"' in header
    assert '"prompt.compare-target"' in header
    assert '"prompt.output-format"' in header
    assert 'generated_help_links_filter' in header


def test_help_generator_rejects_missing_long_form_section():
    broken_source = """## topic:test
```ytnova-help-meta
title: Test
contexts: main.test
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match="missing ### Long form"):
        helpgen.parse_help_source(broken_source)


def test_help_generator_drift_checker_rejects_stale_output(tmp_path):
    man_md = tmp_path / "ytnova.1.md"
    usage_md = tmp_path / "USAGE.md"
    runtime_header = tmp_path / "generated_help_topics.h"

    write_result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_PATH),
            "--source",
            "etc/help/help.en.md",
            "--man-md",
            str(man_md),
            "--usage-md",
            str(usage_md),
            "--runtime-header",
            str(runtime_header),
            "--write",
        ],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    assert write_result.returncode == 0, write_result.stderr

    runtime_header.write_text(
        runtime_header.read_text(encoding="utf-8").replace(
            "generated_help_topic_count", "generated_help_topic_total", 1
        ),
        encoding="utf-8",
    )

    check_result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_PATH),
            "--source",
            "etc/help/help.en.md",
            "--man-md",
            str(man_md),
            "--usage-md",
            str(usage_md),
            "--runtime-header",
            str(runtime_header),
            "--check",
        ],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )

    assert check_result.returncode != 0
    assert "drift" in (check_result.stdout + check_result.stderr).lower()
