def screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def footer_lines(tui):
    return tui.get_screen_dump()[-3:]


def footer_text(tui):
    return "\n".join(footer_lines(tui)).lower()


def find_line_with_text(tui, needle):
    for line in tui.get_screen_dump():
        if needle in line:
            return line
    return None


def line_marks_file_as_tagged(line, filename):
    idx = line.find(filename)
    if idx <= 0:
        return False
    return "*" in line[:idx]


def assert_file_tag_state(tui, filename, expected_tagged):
    line = find_line_with_text(tui, filename)
    assert line is not None, f"Could not find file row for {filename!r}.\nScreen:\n{screen_text(tui)}"
    is_tagged = line_marks_file_as_tagged(line, filename)
    assert is_tagged == expected_tagged, (
        f"Unexpected tag state for {filename!r}. Expected tagged={expected_tagged}, got {is_tagged}.\n"
        f"Row: {line}\nScreen:\n{screen_text(tui)}"
    )
