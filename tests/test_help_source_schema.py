import re
from pathlib import Path


HELP_SOURCES = (
    Path("etc/help/f1.en.md"),
    Path("etc/help/man.en.md"),
)
LOCALE_F1_SOURCES = (Path("etc/help/f1.de.md"),)
REQUIRED_TOPICS = {
    "intro",
    "navigation",
    "shared-commands",
    "tagged",
    "command-line-editing",
    "vi-keys",
    "f10",
    "theming",
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
    "history-dialog",
    "volume-menu",
    "applications-menu",
    "f2-picker",
}


def _read_help_source(path):
    return path.read_text(encoding="utf-8")


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
    for path in HELP_SOURCES + LOCALE_F1_SOURCES:
        source = _read_help_source(path)
        blocks = _topic_blocks(source)

        assert blocks, f"expected at least one canonical help topic block in {path}"
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
    topic_sets = []
    for path in HELP_SOURCES + LOCALE_F1_SOURCES:
        source = _read_help_source(path)
        topics = {match.group("topic") for match in _topic_blocks(source)}
        assert REQUIRED_TOPICS.issubset(topics)
        topic_sets.append(topics)

    assert topic_sets[0] == topic_sets[1] == topic_sets[2]
