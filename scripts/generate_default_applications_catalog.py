#!/usr/bin/env python3

"""Generate or verify the compiled default applications catalog header."""

from __future__ import annotations

import argparse
import difflib
from pathlib import Path
import sys


HEADER_COMMENT = (
    "/* Auto-generated from etc/ytnova.applications for applications.conf starter output. */"
)
HEADER_SYMBOL = "default_applications_catalog"


def render_applications_header(source_text: str) -> str:
    lines = [HEADER_COMMENT, f"static const char {HEADER_SYMBOL}[] ="]

    for raw_line in source_text.splitlines(keepends=True):
        if raw_line.endswith("\n"):
            body = raw_line[:-1]
            suffix = r"\n"
        else:
            body = raw_line
            suffix = ""
        escaped = body.replace("\\", r"\\").replace('"', r'\"')
        lines.append(f'    "{escaped}{suffix}"')

    lines.append("    ;")
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate or verify src/core/default_applications_catalog.h from etc/ytnova.applications."
        ),
    )
    parser.add_argument(
        "--source",
        default="etc/ytnova.applications",
        help="Applications starter source file to read.",
    )
    parser.add_argument(
        "--header",
        default="src/core/default_applications_catalog.h",
        help="Generated header to write or verify.",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--write",
        action="store_true",
        help="Write the generated header to --header.",
    )
    mode.add_argument(
        "--check",
        action="store_true",
        help="Verify that --header matches the generated header.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_path = Path(args.source)
    header_path = Path(args.header)
    generated = render_applications_header(source_path.read_text(encoding="utf-8"))

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
        tofile=str(source_path),
        lineterm="",
    )
    sys.stderr.write("default applications catalog drift detected\n")
    sys.stderr.write("\n".join(diff))
    sys.stderr.write("\n")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
