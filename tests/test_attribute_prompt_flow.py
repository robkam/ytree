from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _open_file_window(tui, filename):
    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda current_lines: current_lines
        if any(filename in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)


def _open_attribute_prompt(tui, activation_key):
    lines = tui.send_and_wait_for_condition(
        activation_key,
        lambda current_lines: current_lines
        if any("ATTRIBUTES:" in line for line in current_lines[-4:])
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)
    return lines


def _open_inline_date_prompt(tui, activation_key="d"):
    lines = tui.send_and_wait_for_condition(
        activation_key,
        lambda current_lines: current_lines
        if any("DATE [" in line for line in current_lines[-4:])
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)
    prompt_text = "\n".join(lines[-4:])
    assert "DATE FIELD:" not in "\n".join(lines), prompt_text
    hint_text = prompt_text.lower()
    assert "f1 help  f1 help" not in hint_text, prompt_text
    assert "f1 help" in hint_text, prompt_text
    assert "f3 scope" in hint_text, prompt_text
    return lines


def test_attribute_date_prompt_skips_field_chooser_and_advertises_help(
    ytnova_binary, tmp_path
):
    work_dir = tmp_path / "attribute_date_prompt"
    work_dir.mkdir()
    (work_dir / "sample.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        _open_file_window(tui, "sample.txt")
        _open_attribute_prompt(tui, "a")
        _open_inline_date_prompt(tui)

        help_lines = tui.send_and_wait_for_condition(
            Keys.F1,
            lambda current_lines: current_lines
            if "yyyy-mm-dd" in "\n".join(current_lines).lower()
            else False,
            timeout=1.5,
        )
        assert help_lines, _screen_text(tui)
        help_text = "\n".join(help_lines).lower()
        assert "f3" in help_text, help_text
        assert "modified" in help_text, help_text
        assert "accessed" in help_text, help_text
        assert "scope cycle" in help_text or "entered value updates" in help_text, help_text

        restored_lines = tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda current_lines: current_lines
            if any("DATE [" in line for line in current_lines[-4:])
            else False,
            timeout=1.5,
        )
        assert restored_lines, _screen_text(tui)
    finally:
        tui.quit()


def test_tagged_attribute_date_prompt_cycles_scope_inline(ytnova_binary, tmp_path):
    work_dir = tmp_path / "tagged_attribute_date_prompt"
    work_dir.mkdir()
    (work_dir / "alpha.txt").write_text("alpha", encoding="utf-8")
    (work_dir / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(work_dir))

    try:
        _open_file_window(tui, "alpha.txt")
        tui.send_keystroke("t", wait=0.2)

        attr_lines = _open_attribute_prompt(tui, Keys.CTRL_A)
        assert "tagged date" in "\n".join(attr_lines).lower(), _screen_text(tui)

        first_prompt = _open_inline_date_prompt(tui)
        assert "modified" in "\n".join(first_prompt).lower(), _screen_text(tui)

        accessed_lines = tui.send_and_wait_for_condition(
            Keys.F3,
            lambda current_lines: current_lines
            if any(
                "DATE [" in line and "accessed" in line.lower()
                for line in current_lines[-4:]
            )
            else False,
            timeout=1.5,
        )
        assert accessed_lines, _screen_text(tui)

        both_lines = tui.send_and_wait_for_condition(
            Keys.F3,
            lambda current_lines: current_lines
            if any(
                "DATE [" in line and "both" in line.lower()
                for line in current_lines[-4:]
            )
            else False,
            timeout=1.5,
        )
        assert both_lines, _screen_text(tui)
    finally:
        tui.quit()
