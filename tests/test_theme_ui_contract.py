from helpers_source import read_repo_source as _read_source


_PUBLISHED_DOC_REASON = (
    "Published documentation is a user-facing contract; runtime execution cannot "
    "safely prove that generated and installed help remains accurate."
)


def _assert_published_text(source, text):
    assert text in source, _PUBLISHED_DOC_REASON


def _assert_published_absence(source, text):
    assert text not in source, _PUBLISHED_DOC_REASON


def test_incremental_jump_docs_exclude_retired_f12_alias():
    man_source = _read_source("etc/ytnova.1.md")
    usage_source = _read_source("docs/USAGE.md")
    spec_source = _read_source("docs/SPECIFICATION.md")

    _assert_published_absence(man_source, "**/** (or **F12**)")
    _assert_published_absence(usage_source, "**/** (or **F12**)")
    _assert_published_absence(spec_source, "**`F12`**: incremental jump")
    _assert_published_absence(spec_source, "Legacy alias for `/`")


def test_starter_theme_docs_exclude_disabled_role():
    starter_theme_source = _read_source("etc/ytnova.themes")
    spec_source = _read_source("docs/SPECIFICATION.md")
    roadmap_source = _read_source("docs/ROADMAP.md")

    _assert_published_absence(starter_theme_source, "disabled =")
    _assert_published_absence(spec_source, "`disabled`")
    _assert_published_absence(
        roadmap_source, "`disabled`: inactive or unavailable commands/options."
    )


def test_theme_docs_describe_role_routing_contracts():
    spec_source = _read_source("docs/SPECIFICATION.md")
    arch_source = _read_source("docs/ARCHITECTURE.md")

    _assert_published_text(
        spec_source, "F1/context help surfaces use `help` for the reading body"
    )
    _assert_published_text(
        spec_source,
        "F2, history, completion, and volume selection surfaces use the `picker` role",
    )
    _assert_published_absence(spec_source, "CPAIR_")
    _assert_published_absence(spec_source, "WINERR_COLOR")
    _assert_published_text(
        arch_source, "Set a window background once per refresh path"
    )
    _assert_published_text(
        arch_source, "stats titles and fixed labels use `static_text`"
    )
    _assert_published_text(
        arch_source, "changing stats values use `dynamic_text`"
    )
    _assert_published_text(
        spec_source, "MUST NOT use raw reverse/blink styling"
    )
    _assert_published_text(spec_source, "tree guide glyphs use `tree_lines`")
    _assert_published_text(
        spec_source, "File-type palette rules do not style directory tree rows"
    )
