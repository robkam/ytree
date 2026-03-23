#!/usr/bin/env python3
"""Check root AI-governance discovery stubs for drift-prone invariants."""

from __future__ import annotations

import sys
from pathlib import Path


def read_lines(path: Path) -> list[str]:
    try:
        return path.read_text(encoding="utf-8").splitlines()
    except FileNotFoundError:
        return []


def contains_line_with_all(lines: list[str], tokens: tuple[str, ...]) -> bool:
    return any(all(token in line for token in tokens) for line in lines)


def check_file(root: Path, file_name: str, provider_path: str) -> list[str]:
    path = root / file_name
    lines = read_lines(path)
    if not lines:
        return [f"{file_name}: file missing or unreadable"]

    failures: list[str] = []

    checks = [
        (
            "missing canonical provider pointer",
            contains_line_with_all(lines, (provider_path,)),
        ),
        (
            "missing shared pointer line",
            contains_line_with_all(lines, ("./.ai/shared.md",)),
        ),
        (
            "missing docs note line with doc/USAGE.md and etc/ytree.1.md",
            contains_line_with_all(lines, ("doc/USAGE.md", "etc/ytree.1.md")),
        ),
        (
            "missing section marker 'Persona quick switch:'",
            contains_line_with_all(lines, ("Persona quick switch:",)),
        ),
        (
            "missing section marker 'Persona skill auto-load:'",
            contains_line_with_all(lines, ("Persona skill auto-load:",)),
        ),
        (
            "missing section marker 'UX economy gate:'",
            contains_line_with_all(lines, ("UX economy gate:",)),
        ),
        (
            "missing section marker 'QA remediation gate:'",
            contains_line_with_all(lines, ("QA remediation gate:",)),
        ),
    ]

    for description, passed in checks:
        if not passed:
            failures.append(f"{file_name}: {description}")

    return failures


def main() -> int:
    root = Path(__file__).resolve().parent.parent

    targets = {
        "AGENTS.md": "./.ai/codex.md",
        "CLAUDE.md": "./.ai/claude.md",
        "GEMINI.md": "./.ai/gemini.md",
    }

    failures: list[str] = []
    for file_name, provider_path in targets.items():
        failures.extend(check_file(root, file_name, provider_path))

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: governance drift check failed ({len(failures)} issue(s))")
        return 1

    print("PASS: governance drift check passed for AGENTS.md, CLAUDE.md, GEMINI.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
