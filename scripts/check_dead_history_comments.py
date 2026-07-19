#!/usr/bin/env python3
"""Reject dead-history source comments in first-party code."""

from __future__ import annotations

import io
import re
import tokenize
from pathlib import Path
from typing import Iterable, NamedTuple

REPO_ROOT = Path(__file__).resolve().parent.parent
FIRST_PARTY_GLOBS = (
    "src/**/*.c",
    "include/**/*.h",
    "scripts/**/*.py",
    "tests/**/*.py",
    "scripts/**/*.sh",
)
EXCLUDED_PATHS = {
    "include/uthash.h",
}
BE_VERBS = {"is", "are", "was", "were", "be", "been", "being"}
CODE_ARTIFACT_WORDS = {
    "branch",
    "buffer",
    "call",
    "check",
    "code",
    "controller",
    "declaration",
    "definition",
    "fallback",
    "flow",
    "footer",
    "function",
    "header",
    "helper",
    "include",
    "layer",
    "logic",
    "macro",
    "module",
    "panel",
    "path",
    "prompt",
    "shim",
    "state",
    "sync",
    "ui",
}
HISTORY_QUALIFIER_WORDS = {
    "earlier",
    "former",
    "formerly",
    "legacy",
    "old",
    "obsolete",
    "original",
    "previous",
}
WORD_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_./-]*")
COMMENTED_OUT_DECL_RE = re.compile(
    r"^\s*(?:static\s+)?(?:extern\s+)?(?:const\s+)?"
    r"(?:[A-Za-z_][\w]*\s+)*[A-Za-z_][\w\*\s]*\b[A-Za-z_][A-Za-z0-9_]*"
    r"\s*\([^;{}]*\)\s*;\s*$"
)
CODE_LIKE_LINE_RE = re.compile(
    r"^\s*(?:#\s*\w+|"
    r"(?:if|else|for|while|switch|case|return)\b|"
    r"(?:static|extern|const|struct|typedef|enum|int|char|void|long|short|"
    r"unsigned|signed|size_t|BOOL)\b|"
    r"[A-Za-z_][A-Za-z0-9_]*\s*\([^;{}]*\)\s*(?:\{|;)|"
    r"[A-Za-z_][A-Za-z0-9_]*\s*=)"
)
STRONG_HISTORY_PATTERNS = (
    ("dead-history marker", re.compile(r"(?i)\bformerly\b")),
    ("dead-history marker", re.compile(r"(?i)\boriginal code\b")),
    ("dead-history marker", re.compile(r"(?i)\bno longer used\b")),
    ("dead-history marker", re.compile(r"(?i)\bnow a no-op\b")),
)
INSTRUCTION_TRANSCRIPT_PATTERNS = (
    re.compile(
        r"(?i)\b(?:instruction|task instructions?)\b.*\b(?:said|provided|"
        r"implies|snippet|keep|use|leave|comment out)\b"
    ),
    re.compile(
        r"(?i)\bfor now just\b.*\b(?:keep|use|leave|comment out|remove|move|"
        r"rewrite|modify)\b"
    ),
    re.compile(
        r"(?i)\b(?:i|we)\s+(?:will|can)\b.*\b(?:keep|leave|comment out|remove|"
        r"move|rewrite|modify)\b"
    ),
    re.compile(
        r"(?i)\bwait,\s*if\b.*\b(?:keep|leave|comment out|remove|move|"
        r"rewrite|modify)\b"
    ),
)


class Comment(NamedTuple):
    relpath: str
    line: int
    text: str


def _normalize_comment_text(text: str) -> str:
    return " ".join(text.replace("\r", " ").split())


def _word_list(text: str) -> list[str]:
    return [word.lower() for word in WORD_RE.findall(text)]


def _looks_like_source_path(word: str) -> bool:
    return bool(re.search(r"\.(?:c|h|py|sh|md)$", word))


def _iter_c_comments(text: str, relpath: str) -> Iterable[Comment]:
    state = "code"
    i = 0
    line = 1
    start_line = 1
    buffer: list[str] = []
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if ch == '"':
                state = "string"
                i += 1
                continue
            if ch == "'":
                state = "char"
                i += 1
                continue
            if ch == "/" and nxt == "/":
                state = "line_comment"
                start_line = line
                buffer = []
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = "block_comment"
                start_line = line
                buffer = []
                i += 2
                continue
        elif state == "string":
            if ch == "\\":
                i += 2
                continue
            if ch == '"':
                state = "code"
                i += 1
                continue
        elif state == "char":
            if ch == "\\":
                i += 2
                continue
            if ch == "'":
                state = "code"
                i += 1
                continue
        elif state == "line_comment":
            if ch == "\n":
                yield Comment(relpath, start_line, "".join(buffer))
                state = "code"
            else:
                buffer.append(ch)
            i += 1
            if ch == "\n":
                line += 1
            continue
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                yield Comment(relpath, start_line, "".join(buffer))
                state = "code"
                i += 2
                continue
            buffer.append(ch)
            i += 1
            if ch == "\n":
                line += 1
            continue

        if ch == "\n":
            line += 1
        i += 1

    if state == "line_comment" and buffer:
        yield Comment(relpath, start_line, "".join(buffer))


def _iter_python_comments(text: str, relpath: str) -> Iterable[Comment]:
    try:
        tokens = tokenize.generate_tokens(io.StringIO(text).readline)
    except tokenize.TokenError:
        return
    for token in tokens:
        if token.type != tokenize.COMMENT:
            continue
        if token.string.startswith("#!"):
            continue
        yield Comment(relpath, token.start[0], token.string[1:])


def _iter_shell_comments(text: str, relpath: str) -> Iterable[Comment]:
    for line_no, raw_line in enumerate(text.splitlines(), start=1):
        stripped = raw_line.lstrip()
        if not stripped.startswith("#") or stripped.startswith("#!"):
            continue
        yield Comment(relpath, line_no, stripped[1:])


def _iter_comments(path: Path, relpath: str) -> Iterable[Comment]:
    text = path.read_text(encoding="utf-8", errors="replace")
    if path.suffix in {".c", ".h"}:
        yield from _iter_c_comments(text, relpath)
        return
    if path.suffix == ".py":
        yield from _iter_python_comments(text, relpath)
        return
    if path.suffix == ".sh":
        yield from _iter_shell_comments(text, relpath)


def _comment_lines(raw_text: str) -> list[str]:
    lines: list[str] = []
    for raw_line in raw_text.splitlines():
        line = raw_line.strip()
        if line.startswith("*"):
            line = line[1:].strip()
        if line:
            lines.append(line)
    return lines


def _is_commented_out_code_block(raw_text: str) -> bool:
    lines = _comment_lines(raw_text)
    if len(lines) < 2:
        return False

    code_like_lines = 0
    for line in lines:
        normalized = _normalize_comment_text(line)
        if COMMENTED_OUT_DECL_RE.match(normalized):
            code_like_lines += 1
            continue
        if CODE_LIKE_LINE_RE.match(normalized):
            code_like_lines += 1
            continue
        if any(token in normalized for token in (";", "{", "}", "#")) and WORD_RE.search(
            normalized
        ):
            code_like_lines += 1
    return code_like_lines >= 2


def _has_used_to_history(lowered: str) -> bool:
    marker = "used to"
    start = lowered.find(marker)
    while start != -1:
        previous_words = _word_list(lowered[:start])
        previous_word = previous_words[-1] if previous_words else ""
        if previous_word not in BE_VERBS:
            return True
        start = lowered.find(marker, start + len(marker))
    return False


def _has_moved_to_history(lowered: str) -> bool:
    marker = "moved to"
    start = lowered.find(marker)
    while start != -1:
        tail_words = _word_list(lowered[start + len(marker) :])[:8]
        if any(word in CODE_ARTIFACT_WORDS or _looks_like_source_path(word) for word in tail_words):
            return True
        start = lowered.find(marker, start + len(marker))
    return False


def _has_obsolete_history(lowered: str) -> bool:
    marker = "obsolete"
    start = lowered.find(marker)
    while start != -1:
        tail_words = _word_list(lowered[start + len(marker) :])[:6]
        if any(word in CODE_ARTIFACT_WORDS or _looks_like_source_path(word) for word in tail_words):
            return True
        start = lowered.find(marker, start + len(marker))
    return False


def _has_removal_history(lowered: str) -> bool:
    stripped = lowered.lstrip("* ")
    if stripped.startswith("removed:"):
        return True
    if "removed #include" in stripped:
        return True
    if stripped.startswith("removed "):
        tail_words = _word_list(stripped[len("removed ") :])[:8]
        if any(
            word in CODE_ARTIFACT_WORDS
            or word in HISTORY_QUALIFIER_WORDS
            or _looks_like_source_path(word)
            for word in tail_words
        ):
            return True

    words = _word_list(lowered)
    for index, word in enumerate(words[1:], start=1):
        if word != "removed":
            continue
        previous_word = words[index - 1]
        if previous_word in CODE_ARTIFACT_WORDS:
            return True
    return False


def _classify_instruction_transcript(normalized: str) -> str | None:
    for pattern in INSTRUCTION_TRANSCRIPT_PATTERNS:
        if pattern.search(normalized):
            return "instruction transcript"
    return None


def _classify_dead_history(normalized: str) -> str | None:
    lowered = normalized.lower()

    for label, pattern in STRONG_HISTORY_PATTERNS:
        if pattern.search(normalized):
            return label
    if _has_used_to_history(lowered):
        return "dead-history marker"
    if _has_moved_to_history(lowered):
        return "moved-to history"
    if _has_obsolete_history(lowered):
        return "obsolete marker"
    if _has_removal_history(lowered):
        return "removal history"
    return None


def classify_comment(comment: Comment) -> str | None:
    normalized = _normalize_comment_text(comment.text)
    if not normalized:
        return None

    if COMMENTED_OUT_DECL_RE.match(normalized):
        return "commented-out declaration"
    if _is_commented_out_code_block(comment.text):
        return "commented-out code block"

    instruction_reason = _classify_instruction_transcript(normalized)
    if instruction_reason is not None:
        return instruction_reason

    return _classify_dead_history(normalized)


def should_scan(path: Path, root: Path) -> bool:
    relpath = path.relative_to(root).as_posix()
    if relpath in EXCLUDED_PATHS:
        return False
    if relpath.startswith("src/") and path.suffix == ".c":
        return True
    if relpath.startswith("include/") and path.suffix == ".h":
        return True
    if relpath.startswith(("scripts/", "tests/")) and path.suffix == ".py":
        return True
    if relpath.startswith("scripts/") and path.suffix == ".sh":
        return True
    return False


def iter_first_party_paths(root: Path) -> Iterable[Path]:
    seen: set[Path] = set()
    for glob in FIRST_PARTY_GLOBS:
        for path in sorted(root.glob(glob)):
            if not path.is_file() or path in seen:
                continue
            seen.add(path)
            if should_scan(path, root):
                yield path


def check_path(path: Path, root: Path) -> list[str]:
    if not should_scan(path, root):
        return []
    relpath = path.relative_to(root).as_posix()
    failures: list[str] = []
    for comment in _iter_comments(path, relpath):
        reason = classify_comment(comment)
        if reason is None:
            continue
        snippet = _normalize_comment_text(comment.text)
        failures.append(f"{relpath}:{comment.line}: {reason}: {snippet}")
    return failures


def check_repository(root: Path) -> list[str]:
    failures: list[str] = []
    for path in iter_first_party_paths(root):
        failures.extend(check_path(path, root))
    return failures


def main() -> int:
    failures = check_repository(REPO_ROOT)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: dead-history comment guard failed ({len(failures)} issue(s))")
        return 1
    print("PASS: dead-history comment guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
