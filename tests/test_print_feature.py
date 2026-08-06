import os
import tempfile

from ytnova_control import YtreeNovaController
from ytnova_keys import Keys


def _binary_path() -> str:
    return os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "build",
        "ytnova",
    )


def _spawn_controller(cwd: str) -> YtreeNovaController:
    ytnova = YtreeNovaController(_binary_path(), cwd)
    ytnova.wait_for_startup()
    return ytnova


def _enter_file_mode(ytnova: YtreeNovaController) -> None:
    ytnova.child.send(Keys.ENTER)
    ytnova.wait_for_refresh()
    ytnova.wait_for_refresh()


def _wait_for_screen_text(ytnova: YtreeNovaController, text: str, timeout: float = 2.0):
    lines = ytnova.wait_for_text(text, timeout=timeout)
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _open_output_flow(ytnova: YtreeNovaController, key: str = "o"):
    lines = ytnova.send_and_wait_for_condition(
        key,
        lambda screen: screen
        if any("Format:" in line and "Page break" in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _choose_raw_format(ytnova: YtreeNovaController):
    lines = ytnova.send_and_wait_for_condition(
        "R",
        lambda screen: screen
        if any("Output to:" in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _choose_page_break_format(ytnova: YtreeNovaController):
    lines = ytnova.send_and_wait_for_condition(
        "P",
        lambda screen: screen
        if any("Page break separator" in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _choose_output_route(ytnova: YtreeNovaController, route_key: str, prompt_text: str):
    lines = ytnova.send_and_wait_for_condition(
        route_key,
        lambda screen: screen
        if any(prompt_text in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def test_output_flow_uses_o_key_and_explicit_file_and_hardcopy_prompts():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("explicit-output-prompts\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            format_screen = "\n".join(_open_output_flow(ytnova, "o"))
            assert "Format:" in format_screen, format_screen
            assert "Page break" in format_screen, format_screen

            route_screen = "\n".join(_choose_raw_format(ytnova))
            assert "Output to:" in route_screen, route_screen
            assert "File" in route_screen, route_screen
            assert "Hardcopy" in route_screen, route_screen
            assert "Command" not in route_screen, route_screen

            file_prompt = "\n".join(_choose_output_route(ytnova, "F", "Output file"))
            assert "Output file" in file_prompt, file_prompt

            output_path = os.path.join(td, "prompt_check.txt")
            ytnova.input_text(output_path)
            assert os.path.exists(output_path)

            _open_output_flow(ytnova, "o")
            _choose_raw_format(ytnova)
            hardcopy_prompt = "\n".join(
                _choose_output_route(ytnova, "H", "Printer command:")
            )
            assert "Printer command:" in hardcopy_prompt, hardcopy_prompt
        finally:
            ytnova.quit()


def test_output_plain_path_defaults_to_file_destination():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("plain-path-default\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            out_path = os.path.join(td, "plain_destination.txt")
            _open_output_flow(ytnova, "o")
            _choose_raw_format(ytnova)
            _choose_output_route(ytnova, "F", "Output file")
            ytnova.input_text(out_path)

            assert os.path.exists(out_path)
            with open(out_path, "r", encoding="utf-8") as handle:
                assert "plain-path-default" in handle.read()

            alias_out_path = os.path.join(td, "legacy_alias_destination.txt")
            _open_output_flow(ytnova, "o")
            _choose_raw_format(ytnova)
            _choose_output_route(ytnova, "H", "Printer command:")
            ytnova.input_text(f">{alias_out_path}")

            assert os.path.exists(alias_out_path)
            with open(alias_out_path, "r", encoding="utf-8") as handle:
                assert "plain-path-default" in handle.read()
        finally:
            ytnova.quit()


def test_tagged_output_shortcut_reuses_output_flow():
    with tempfile.TemporaryDirectory() as td:
        with open(os.path.join(td, "alpha.txt"), "w", encoding="utf-8") as handle:
            handle.write("alpha-tagged-output\n")
        with open(os.path.join(td, "beta.txt"), "w", encoding="utf-8") as handle:
            handle.write("beta-tagged-output\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)
            ytnova.child.send("t")
            ytnova.wait_for_refresh()
            ytnova.child.send(Keys.DOWN)
            ytnova.wait_for_refresh()
            ytnova.child.send("t")
            ytnova.wait_for_refresh()

            output_path = os.path.join(td, "tagged_output.txt")
            _open_output_flow(ytnova, Keys.CTRL_W)
            _choose_raw_format(ytnova)
            _choose_output_route(ytnova, "F", "Output file")
            ytnova.input_text(output_path)

            assert os.path.exists(output_path)
            with open(output_path, "r", encoding="utf-8") as handle:
                output_text = handle.read()
            assert "alpha-tagged-output" in output_text
            assert "beta-tagged-output" in output_text
        finally:
            ytnova.quit()


def test_output_hardcopy_failure_shows_error_without_crash():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("command-failure\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            _open_output_flow(ytnova, "o")
            _choose_raw_format(ytnova)
            _choose_output_route(ytnova, "H", "Printer command:")
            ytnova.input_text("false")

            _wait_for_screen_text(ytnova, "execution of command", timeout=5.0)
            assert ytnova.child.isalive(), "ytnova crashed after hardcopy failure"
        finally:
            ytnova.quit()


def test_page_break_prompt_still_uses_distinct_separator_prompt():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("page-break-prompt\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            _open_output_flow(ytnova, "o")
            page_break_prompt = "\n".join(_choose_page_break_format(ytnova))
            assert "Page break separator" in page_break_prompt, page_break_prompt

            ytnova.input_text("---SEP---")
            route_screen = "\n".join(_wait_for_screen_text(ytnova, "Output to:"))
            assert "Hardcopy" in route_screen, route_screen
        finally:
            ytnova.quit()
