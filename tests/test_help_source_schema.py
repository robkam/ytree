import re
from pathlib import Path


HELP_SOURCES = (
    Path("etc/help/f1.en.md"),
    Path("etc/help/man.en.md"),
)
LOCALE_F1_SOURCES = (Path("etc/help/f1.de.md"),)
LOCALE_MAN_SOURCES = (Path("etc/help/man.de.md"),)
ALL_HELP_SOURCES = HELP_SOURCES + LOCALE_F1_SOURCES + LOCALE_MAN_SOURCES
DISPLAY_SOURCE = Path("src/ui/display.c")
RUNTIME_HELP_CONTEXT_SOURCES = (
    Path("src/ui/application_menu.c"),
    Path("src/ui/attr_actions.c"),
    Path("src/ui/compare_request.c"),
    Path("src/ui/display.c"),
    Path("src/ui/f2_picker.c"),
    Path("src/ui/interactions.c"),
    Path("src/ui/print_controller.c"),
    Path("src/ui/tagged_view.c"),
    Path("src/ui/volume_menu.c"),
)
REQUIRED_TOPICS = {
    "intro",
    "f1-navigation",
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
    "Copy": ("Copy", "C/^Copy"),
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
        r"^## topic:(?P<topic>[a-z0-9-]+)\n+"
        r"```ytnova-help-meta\n"
        r"title: (?P<title>[^\n]+)\n"
        r"contexts: (?P<contexts>[^\n]+)\n"
        r"```\n+"
        r"### Contextual F1\n(?P<contextual>.*?)(?:\n+### Explainer links\n(?P<links>.*?))?"
        r"(?:\n+### Long form\n(?P<long_form>.*?))?(?=^## topic:|\Z)",
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
    if title == "YtreeNova Navigation":
        return "Navigation"
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
    pattern = re.compile(r'"((?:main|overlay|prompt|dialog|viewer)\.[a-z0-9.-]+)"')
    for path in RUNTIME_HELP_CONTEXT_SOURCES:
        contexts.update(pattern.findall(path.read_text(encoding="utf-8")))
    return contexts


def test_help_source_uses_deterministic_topic_block_schema():
    for path in ALL_HELP_SOURCES:
        source = _read_help_source(path)
        blocks = _topic_blocks(source)

        assert blocks, f"expected at least one canonical help topic block in {path}"
        assert len(blocks) == source.count("\n## topic:") + source.startswith("## topic:")

        for block in blocks:
            contexts = block.group("contexts")
            contextual = block.group("contextual").strip()
            long_form = (block.group("long_form") or "").strip()
            links = (block.group("links") or "").strip()

            assert block.group("title").strip()
            assert contexts == "none" or re.fullmatch(
                r"[a-z0-9.-]+(?:,[a-z0-9.-]+)*", contexts
            ), f"invalid contexts list for topic {block.group('topic')}: {contexts!r}"
            assert contextual
            if path in (Path("etc/help/man.en.md"),) + LOCALE_MAN_SOURCES:
                assert re.search(r"^#### ", long_form, re.M), (
                    f"topic {block.group('topic')} needs at least one long-form subsection"
                )
            if links:
                assert re.fullmatch(
                    r"(?:- \[[^\]]+\]\(topic:[a-z0-9-]+\)\n?)+", links
                ), f"invalid explainer links block for topic {block.group('topic')}"


def test_help_source_keeps_blank_lines_around_markdown_headings():
    for path in ALL_HELP_SOURCES:
        lines = _read_help_source(path).splitlines()
        in_fence = False

        for index, line in enumerate(lines):
            if line.startswith("```"):
                in_fence = not in_fence
                continue
            if in_fence or not re.match(r"^#{1,6} ", line):
                continue

            assert index == 0 or not lines[index - 1].strip(), (
                f"heading in {path} needs a blank line before it: {line!r}"
            )
            assert index == len(lines) - 1 or not lines[index + 1].strip(), (
                f"heading in {path} needs a blank line after it: {line!r}"
            )


def test_contextual_f1_keeps_actions_on_separate_source_lines():
    for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES:
        source = _read_help_source(path)
        assert "separate keys or actions need explanation, give each one its own short source line" in source or (
            "verschiedene Tasten oder Aktionen erklärt werden müssen, erhält jede eine eigene kurze Quellzeile"
            in source
        )


def test_ytnova_navigation_keeps_its_facts_in_visible_contextual_help():
    for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES:
        topic = _topic_block_map(_read_help_source(path))["ytnova-navigation"]

        assert not (topic.group("long_form") or "").strip()
        assert "C-m" in topic.group("contextual")
        assert "C-i" in topic.group("contextual")
        assert "C-[" in topic.group("contextual")
        assert "Alt" in topic.group("contextual")
        assert "Tab" in topic.group("contextual")
        assert "topic:list-jump" in topic.group("contextual")
        assert "topic:f7" in topic.group("contextual")
        assert "topic:f8" in topic.group("contextual")
        assert not (topic.group("links") or "").strip()


def test_contextual_help_uses_portable_control_key_notation():
    for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES:
        source = _read_help_source(path)
        assert "Ctrl" "-" not in source
        assert "C-" in source
    assert not re.search(r"C-[A-Z]", source)


def test_tagged_help_explains_control_key_operations_and_footer_marker():
    expected = {
        Path("etc/help/f1.en.md"): (
            "Hold the Control key with the letter after `C-`",
            "`^` in a footer label means the same tagged operation",
        ),
        Path("etc/help/f1.de.md"): (
            "Halte die Steuerungstaste mit dem Buchstaben nach `C-` gedrückt",
            "`^` in einem Footer-Label bedeutet denselben Markierungsvorgang",
        ),
    }

    for path, phrases in expected.items():
        contextual = _topic_block_map(_read_help_source(path))["tagged"].group(
            "contextual"
        )
        for phrase in phrases:
            assert phrase in contextual


def test_manpage_defines_portable_control_key_notation():
    source = Path("etc/help/man.en.md").read_text(encoding="utf-8")

    assert "C-<chr>" in source
    assert "hold the Control key" in source


def test_manpage_documents_terminal_text_size_controls_for_small_footers():
    source = Path("etc/help/man.en.md").read_text(encoding="utf-8")

    assert "`C--`" in source
    assert "`C-+`" in source
    assert "small terminal windows" in source
    for path in (Path("etc/ytnova.1.md"), Path("docs/USAGE.md")):
        generated = path.read_text(encoding="utf-8")
        assert "`C--`" in generated
        assert "`C-+`" in generated


def test_man_sources_do_not_emit_per_topic_see_also_noise():
    for path in (Path("etc/help/man.en.md"), Path("etc/help/man.de.md")):
        assert not re.search(
            r"^### Explainer links\n- \[", path.read_text(encoding="utf-8"), re.MULTILINE
        )


def test_help_source_defines_required_first_pass_topics():
    for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES:
        source = _read_help_source(path)
        topics = {match.group("topic") for match in _topic_blocks(source)}
        assert REQUIRED_TOPICS.issubset(topics)

    f1_topics = [
        {match.group("topic") for match in _topic_blocks(_read_help_source(path))}
        for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES
    ]
    assert all(topics == f1_topics[0] for topics in f1_topics[1:])


def test_f1_context_metadata_matches_runtime_help_entry_points():
    f1_context_map = _topic_context_map(_read_help_source(Path("etc/help/f1.en.md")))
    runtime_contexts = _runtime_help_contexts()

    assert set(f1_context_map) == runtime_contexts
    for context_id, topics in f1_context_map.items():
        assert len(topics) == 1, f"{context_id} is owned by multiple F1 topics: {topics}"


def test_runtime_footer_commands_remain_covered_by_the_man_reference():
    man_source = _read_help_source(Path("etc/help/man.en.md"))
    help_labels = _help_label_override_map()

    for topic, array_name in RUNTIME_HELP_LABEL_TOPICS.items():
        expected_labels = help_labels[array_name]
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


def test_f1_sources_limit_long_form_sections_to_tagged_viewer_help():
    for path in (Path("etc/help/f1.en.md"),) + LOCALE_F1_SOURCES:
        blocks = _topic_block_map(_read_help_source(path))
        long_form_topics = {
            topic
            for topic, block in blocks.items()
            if (block.group("long_form") or "").strip()
        }
        assert long_form_topics == {"tagged-viewer"}


def test_contents_topic_is_a_complete_alphabetical_operator_index():
    f1_source = _read_help_source(Path("etc/help/f1.en.md"))
    title_map = _topic_title_map(f1_source)
    contents_only_topics = {"execute-dir", "execute-file", "f1-navigation"}
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
