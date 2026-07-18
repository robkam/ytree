#!/usr/bin/env python3

"""Generate or verify the compiled packaged command-preset catalog header."""

from __future__ import annotations

import argparse
import difflib
from pathlib import Path
import sys


def _escape_c_string(raw_line: str) -> str:
    if raw_line.endswith("\n"):
        body = raw_line[:-1]
        suffix = r"\n"
    else:
        body = raw_line
        suffix = ""
    escaped = body.replace("\\", r"\\").replace('"', r'\"')
    return f'        "{escaped}{suffix}"'


def render_presets_header(source_dir: Path) -> str:
    preset_files = sorted(source_dir.glob("*.conf"))
    lines = [
        "/* Auto-generated from packaged preset sources under etc/commands/. */",
        "typedef struct {",
        "  const char *preset_id;",
        "  const char *contents;",
        "} DefaultCommandPresetCatalogEntry;",
        "",
        "static const DefaultCommandPresetCatalogEntry default_command_presets_catalog[] = {",
    ]

    for preset_file in preset_files:
        lines.append(f'    {{"{preset_file.stem}",')
        for raw_line in preset_file.read_text(encoding="utf-8").splitlines(keepends=True):
            lines.append(_escape_c_string(raw_line))
        lines.append("    },")

    lines.extend(
        [
            "};",
            "",
            "static const size_t default_command_presets_catalog_count =",
            "    sizeof(default_command_presets_catalog) /",
            "    sizeof(default_command_presets_catalog[0]);",
        ]
    )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate or verify src/core/default_command_presets_catalog.h from etc/commands/*.conf."
        ),
    )
    parser.add_argument(
        "--source-dir",
        default="etc/commands",
        help="Directory containing packaged preset source files.",
    )
    parser.add_argument(
        "--header",
        default="src/core/default_command_presets_catalog.h",
        help="Generated header to write or verify.",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="Write the generated header.")
    mode.add_argument("--check", action="store_true", help="Verify the generated header.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_dir = Path(args.source_dir)
    header_path = Path(args.header)
    generated = render_presets_header(source_dir)

    if args.write:
        header_path.write_text(generated, encoding="utf-8")
        return 0

    current = header_path.read_text(encoding="utf-8")
    if current == generated:
        return 0

    diff = difflib.unified_diff(
        current.splitlines(),
        generated.splitlines(),
        fromfile=str(header_path),
        tofile=str(source_dir),
        lineterm="",
    )
    sys.stderr.write("default command preset catalog drift detected\n")
    sys.stderr.write("\n".join(diff))
    sys.stderr.write("\n")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
