import re
from pathlib import Path


HELP_SOURCE = Path("etc/help/help.en.md")
REQUIRED_TOPICS = {
    "intro",
    "navigation",
    "dir",
    "file",
    "archive-dir",
    "archive-file",
    "filter",
    "compare",
    "output",
    "showall",
    "global",
    "f7",
    "f8",
}


def _read_help_source():
    return HELP_SOURCE.read_text(encoding="utf-8")


def _topic_blocks(source):
    pattern = re.compile(
        r"^## topic:(?P<topic>[a-z0-9-]+)\n"
        r"```ytnova-help-meta\n"
        r"title: (?P<title>[^\n]+)\n"
        r"contexts: (?P<contexts>[^\n]+)\n"
        r"```\n"
        r"### Contextual F1\n(?P<contextual>.*?)(?:\n### Explainer links\n(?P<links>.*?))?"
        r"\n### Long form\n(?P<long_form>.*?)(?=^## topic:|\Z)",
        re.M | re.S,
    )
    return list(pattern.finditer(source))


def test_help_source_uses_deterministic_topic_block_schema():
    source = _read_help_source()
    blocks = _topic_blocks(source)

    assert blocks, "expected at least one canonical help topic block"
    assert len(blocks) == source.count("\n## topic:") + source.startswith("## topic:")

    for block in blocks:
        contexts = block.group("contexts")
        contextual = block.group("contextual").strip()
        long_form = block.group("long_form").strip()
        links = (block.group("links") or "").strip()

        assert block.group("title").strip()
        assert contexts == "none" or re.fullmatch(
            r"[a-z0-9.-]+(?:,[a-z0-9.-]+)*", contexts
        ), f"invalid contexts list for topic {block.group('topic')}: {contexts!r}"
        assert contextual
        assert re.search(r"^#### ", long_form, re.M), (
            f"topic {block.group('topic')} needs at least one long-form subsection"
        )
        if links:
            assert re.fullmatch(
                r"(?:- \[[^\]]+\]\(topic:[a-z0-9-]+\)\n?)+", links
            ), f"invalid explainer links block for topic {block.group('topic')}"


def test_help_source_defines_required_first_pass_topics():
    source = _read_help_source()
    topics = {match.group("topic") for match in _topic_blocks(source)}

    assert REQUIRED_TOPICS.issubset(topics)
