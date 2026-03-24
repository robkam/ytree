#!/usr/bin/env python3
"""Guardrail check for banned unsafe C string APIs in src/**/*.c."""

from __future__ import annotations

from pathlib import Path

BANNED_CALLS = ("strcpy", "strcat", "sprintf")


def find_banned_calls(text: str) -> list[tuple[int, str]]:
    """Return (line, call) findings from C source while skipping comments/literals."""
    findings: list[tuple[int, str]] = []
    n = len(text)
    i = 0
    line = 1
    state = "code"

    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if state == "code":
            if ch == "/" and nxt == "/":
                state = "line_comment"
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = "block_comment"
                i += 2
                continue
            if ch == '"':
                state = "string"
                i += 1
                continue
            if ch == "'":
                state = "char"
                i += 1
                continue
            if ch == "\n":
                line += 1
                i += 1
                continue

            if ch.isalpha() or ch == "_":
                start_line = line
                start = i
                i += 1
                while i < n and (text[i].isalnum() or text[i] == "_"):
                    i += 1
                ident = text[start:i]

                j = i
                while j < n and text[j].isspace():
                    j += 1
                if ident in BANNED_CALLS and j < n and text[j] == "(":
                    findings.append((start_line, f"{ident}("))
                continue

            i += 1
            continue

        if state == "line_comment":
            if ch == "\n":
                state = "code"
                line += 1
            i += 1
            continue

        if state == "block_comment":
            if ch == "\n":
                line += 1
            if ch == "*" and nxt == "/":
                state = "code"
                i += 2
            else:
                i += 1
            continue

        if state == "string":
            if ch == "\\" and i + 1 < n:
                if text[i + 1] == "\n":
                    line += 1
                i += 2
                continue
            if ch == '"':
                state = "code"
                i += 1
                continue
            if ch == "\n":
                line += 1
            i += 1
            continue

        if state == "char":
            if ch == "\\" and i + 1 < n:
                if text[i + 1] == "\n":
                    line += 1
                i += 2
                continue
            if ch == "'":
                state = "code"
                i += 1
                continue
            if ch == "\n":
                line += 1
            i += 1
            continue

    return findings


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    src_root = root / "src"
    violations: list[tuple[str, int, str]] = []

    for path in sorted(src_root.rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        rel_path = path.relative_to(root).as_posix()
        for line, call in find_banned_calls(text):
            violations.append((rel_path, line, call))

    if violations:
        for path, line, call in violations:
            print(f"{path}:{line}: {call}")
        print(f"FAIL: banned unsafe API calls found ({len(violations)})")
        return 1

    print("PASS: no banned unsafe C API calls found in src/**/*.c")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
