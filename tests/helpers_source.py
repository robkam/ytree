from pathlib import Path


def read_repo_source(path):
    repo_root = Path(__file__).resolve().parents[1]
    return (repo_root / path).read_text(encoding="utf-8")


def extract_function_block(source, signature):
    start = source.find(signature)
    assert start >= 0, f"Could not find function signature: {signature}"

    open_brace = source.find("{", start)
    assert open_brace >= 0, f"Could not find opening brace for: {signature}"

    depth = 0
    for idx in range(open_brace, len(source)):
        ch = source[idx]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source[start : idx + 1]

    raise AssertionError(f"Could not find closing brace for: {signature}")
