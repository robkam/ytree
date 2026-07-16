from pathlib import Path


def _render_profile_header(source_text: str) -> str:
    lines = [
        "/* Auto-generated from etc/ytnova.conf for --init profile output. */",
        "static const char default_profile_template[] =",
    ]

    for raw_line in source_text.splitlines(keepends=True):
        if raw_line.endswith("\n"):
            body = raw_line[:-1]
            suffix = r"\n"
        else:
            body = raw_line
            suffix = ""
        escaped = body.replace("\\", r"\\").replace('"', r"\"")
        lines.append(f'    "{escaped}{suffix}"')

    lines.append("    ;")
    return "\n".join(lines) + "\n"


def _render_commands_header(source_text: str) -> str:
    lines = [
        "/* Auto-generated from etc/ytnova.commands for commands.conf starter output. */",
        "static const char default_commands_catalog[] =",
    ]

    for raw_line in source_text.splitlines(keepends=True):
        if raw_line.endswith("\n"):
            body = raw_line[:-1]
            suffix = r"\n"
        else:
            body = raw_line
            suffix = ""
        escaped = body.replace("\\", r"\\").replace('"', r"\"")
        lines.append(f'    "{escaped}{suffix}"')

    lines.append("    ;")
    return "\n".join(lines) + "\n"


def test_default_profile_template_header_matches_packaged_config():
    conf_source = Path("etc/ytnova.conf").read_text(encoding="utf-8")
    header_source = Path("src/core/default_profile_template.h").read_text(
        encoding="utf-8"
    )

    assert header_source == _render_profile_header(conf_source)


def test_default_commands_catalog_header_matches_packaged_commands():
    commands_source = Path("etc/ytnova.commands").read_text(encoding="utf-8")
    header_source = Path("src/core/default_commands_catalog.h").read_text(
        encoding="utf-8"
    )

    assert header_source == _render_commands_header(commands_source)
