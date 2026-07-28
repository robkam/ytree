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
    f1_topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "f1.en.md").read_text(encoding="utf-8")
    )
    man_topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "man.en.md").read_text(encoding="utf-8")
    )

    helpgen.validate_topic_inventory(f1_topics, man_topics)
    manpage = helpgen.render_manpage_markdown(
        man_topics, usage_mode=False, source_path="etc/help/man.en.md"
    )
    usage = helpgen.render_manpage_markdown(
        man_topics, usage_mode=True, source_path="etc/help/man.en.md"
    )
    header = helpgen.render_runtime_header(
        f1_topics, source_path="etc/help/f1.en.md"
    )

    assert helpgen.generated_banner("etc/help/man.en.md") in manpage
    assert "### Directory Mode" in manpage
    assert "### Help System" in manpage
    assert "**Attributes**: Open the attributes submenu." in manpage
    assert "### Filter Help" in manpage
    assert "### Command-line Editing" in manpage
    assert "### F10 Config" in manpage
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
            "--f1-source",
            "etc/help/f1.en.md",
            "--man-source",
            "etc/help/man.en.md",
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
            "--f1-source",
            "etc/help/f1.en.md",
            "--man-source",
            "etc/help/man.en.md",
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


def test_help_generator_roff_preserves_option_keywords_and_literal_globs():
    topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "man.en.md").read_text(encoding="utf-8")
    )

    roff = helpgen.render_roff_document(
        helpgen.render_manpage_markdown(
            topics, usage_mode=False, source_path="etc/help/man.en.md"
        ),
        version="1.0.0-alpha",
        versiondate="June 2026",
    )

    assert r"\fBmin\fR/\fBroot\fR (0), \fBmax\fR/\fBall\fR (100)." in roff
    assert r"\fB*.c\fR" in roff
    assert r"\fB*.c,*.h\fR" in roff
    assert r"\\fB-h\\fR" not in roff
