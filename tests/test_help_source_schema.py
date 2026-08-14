import re
from pathlib import Path


HELP_SOURCES = (
    Path("etc/help/f1.en.md"),
    Path("etc/help/man.en.md"),
)
LOCALE_F1_SOURCES = (Path("etc/help/f1.de.md"),)
DISPLAY_SOURCE = Path("src/ui/display.c")
RUNTIME_HELP_CONTEXT_SOURCES = (
    Path("src/ui/application_menu.c"),
    Path("src/ui/attr_actions.c"),
    Path("src/ui/compare_request.c"),
    Path("src/ui/display.c"),
    Path("src/ui/f2_picker.c"),
    Path("src/ui/interactions.c"),
    Path("src/ui/print_controller.c"),
    Path("src/ui/volume_menu.c"),
)
REQUIRED_TOPICS = {
    "intro",
    "navigation",
    "shared-commands",
    "tagged",
    "command-line-editing",
    "copy-move-targets",
    "list-jump",
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
RUNTIME_HELP_LABEL_TOPICS = {
    "dir": "dir_help_label_specs",
    "file": "file_help_label_specs",
    "archive-dir": "archive_dir_help_label_specs",
    "archive-file": "archive_file_help_label_specs",
    "showall": "file_help_label_specs",
    "global": "file_help_label_specs",
    "f7": "preview_help_label_specs",
    "f8-dir": "dir_help_label_specs",
    "f8-file": "file_help_label_specs",
}
HELP_LABEL_ALIASES = {
    "Archive": ("Archive", "Z archive"),
    "Compare": ("Compare", "J compare"),
    "Copy": ("Copy", "C/^K copy"),
    "Dotfiles": ("Dotfiles", "` dotfiles", "\\` dotfiles"),
    "Execute": ("Execute", "eXecute"),
    "Invert Tags": ("Invert Tags", "Invert"),
    "Jump": ("Jump", "/ jump"),
    "Move": ("Move", "M/^N move"),
    "MoveDir": ("MoveDir", "moVedir"),
    "New File": ("New File", "Newfile"),
    "Pathcopy": ("Pathcopy", "pathcopY"),
    "Volume": ("Volume", "K volume"),
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


def _topic_block_map(source):
    return {match.group("topic"): match for match in _topic_blocks(source)}


def _topic_long_form(source, topic):
    return _topic_block_map(source)[topic].group("long_form")


def _topic_title_map(source):
    return {
        match.group("topic"): match.group("title") for match in _topic_blocks(source)
    }


def _contents_link_label(title):
    return title[:-5] if title.endswith(" Help") else title


def _topic_context_map(source):
    context_map = {}
    for match in _topic_blocks(source):
        contexts = match.group("contexts")
        if contexts == "none":
            continue
        for context_id in contexts.split(","):
            context_map.setdefault(context_id, []).append(match.group("topic"))
    return context_map


def _topic_explainer_links(source, topic):
    links = _topic_block_map(source)[topic].group("links") or ""
    return re.findall(r"- \[([^\]]+)\]\(topic:([a-z0-9-]+)\)", links)


def _topic_command_labels(source, topic):
    return {
        match.group(1)
        for match in re.finditer(r"^\* \*\*([^*]+)\*\*:", _topic_long_form(source, topic), re.M)
    }


def _help_label_override_map():
    source = DISPLAY_SOURCE.read_text(encoding="utf-8")
    pattern = re.compile(
        r"static const HelpLabelOverrideSpec (?P<name>[a-z_]+)\[\] = \{(?P<body>.*?)\};",
        re.S,
    )
    return {
        match.group("name"): re.findall(
            r'\{"([^"]+)",\s*"[^"]+"\}',
            match.group("body"),
        )
        for match in pattern.finditer(source)
    }


def _runtime_help_contexts():
    contexts = set()
    pattern = re.compile(r'"((?:main|overlay|prompt|dialog)\.[a-z0-9.-]+)"')
    for path in RUNTIME_HELP_CONTEXT_SOURCES:
        contexts.update(pattern.findall(path.read_text(encoding="utf-8")))
    return contexts


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


def test_f1_context_metadata_matches_runtime_help_entry_points():
    f1_context_map = _topic_context_map(_read_help_source(Path("etc/help/f1.en.md")))
    runtime_contexts = _runtime_help_contexts()

    assert set(f1_context_map) == runtime_contexts
    for context_id, topics in f1_context_map.items():
        assert len(topics) == 1, f"{context_id} is owned by multiple F1 topics: {topics}"


def test_first_pass_runtime_help_topics_keep_footer_and_reference_command_parity():
    f1_source = _read_help_source(Path("etc/help/f1.en.md"))
    man_source = _read_help_source(Path("etc/help/man.en.md"))
    help_labels = _help_label_override_map()

    for topic, array_name in RUNTIME_HELP_LABEL_TOPICS.items():
        expected_labels = help_labels[array_name]
        f1_labels = _topic_command_labels(f1_source, topic)

        man_long_form = _topic_long_form(man_source, topic)
        missing_man = []
        for label in expected_labels:
            aliases = HELP_LABEL_ALIASES.get(label, (label,))
            if not any(alias in man_long_form for alias in aliases):
                missing_man.append(label)
        assert not missing_man, (
            f"{topic} man/usage topic is missing runtime footer command coverage: "
            f"{missing_man}"
        )

        missing_f1 = []
        for label in expected_labels:
            aliases = HELP_LABEL_ALIASES.get(label, (label,))
            if not any(alias in f1_labels for alias in aliases):
                missing_f1.append(label)
        assert not missing_f1, (
            f"{topic} F1 topic is missing runtime footer command rows: {missing_f1}"
        )


def test_contents_topic_is_a_complete_alphabetical_operator_index():
    f1_source = _read_help_source(Path("etc/help/f1.en.md"))
    title_map = _topic_title_map(f1_source)
    contents_only_topics = {"execute-dir", "execute-file"}
    expected_links = sorted(
        (
            (_contents_link_label(title), topic)
            for topic, title in title_map.items()
            if topic != "intro" and topic not in contents_only_topics
        ),
        key=lambda item: item[0].casefold(),
    )
    contents_links = _topic_explainer_links(f1_source, "intro")
    assert contents_links == expected_links
