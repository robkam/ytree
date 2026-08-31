import pytest
from pathlib import Path
import re
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


VERTICAL_CHARS = {"x", "|"}
BORDER_MIN_Y = 2
BORDER_MAX_Y = 31


@pytest.fixture
def f7_preview_sandbox(tmp_path):
    """
    Build a dedicated tree for F7 preview assertions.

    Layout goal:
    - Root contains exactly one directory so one DOWN reliably selects it.
    - Preview directory contains one extremely long filename to test clipping.
    - File content contains a stable marker so we can assert we are in preview.
    """
    root = tmp_path / "f7_preview_root"
    root.mkdir()

    preview_dir = root / "preview_dir"
    preview_dir.mkdir()

    long_name = (
        "very_long_filename_for_f7_preview_boundary_clipping_validation_"
        "1234567890abcdefghijklmnopqrstuvwxyz.txt"
    )
    marker = "F7_PREVIEW_MARKER_LINE_001"

    (preview_dir / long_name).write_text(
        marker + "\n" + "line_2_for_preview\n",
        encoding="utf-8",
    )
    (preview_dir / "z_secondary_file.txt").write_text(
        "secondary_file_content\n",
        encoding="utf-8",
    )

    return {
        "root": root,
        "long_name": long_name,
        "long_name_token": long_name[:24],
        "marker": marker,
    }


def _screen_text(lines):
    return "\n".join(lines)


def _detect_preview_separator_column(lines):
    """
    Detect the interior vertical separator used by F7 preview mode.
    We scan interior columns and pick the one with the strongest vertical-line count.
    """
    counts = {}
    for y in range(BORDER_MIN_Y, BORDER_MAX_Y + 1):
        row = lines[y]
        for x in range(8, 61):
            if row[x] in VERTICAL_CHARS:
                counts[x] = counts.get(x, 0) + 1

    if not counts:
        return None, {}

    best_col = max(counts, key=counts.get)
    return best_col, counts


def _launch_preview(ytnova_binary, sandbox_info):
    """
    Enter F7 preview from tree mode using project-standard keystrokes.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(sandbox_info["root"]))

    assert tui.wait_for_content("preview_dir", timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.EXPAND_ALL, timeout=2.0)
    assert tui.send_and_wait_for_condition(
        Keys.DOWN,
        lambda current: current
        if sandbox_info["long_name_token"] in _screen_text(current)
        else False,
        timeout=2.0,
    ), "Preview directory did not finish loading its fixture file."
    lines = tui.send_and_wait_for_condition(
        Keys.F7,
        lambda current: current
        if sandbox_info["marker"] in _screen_text(current)
        else False,
        timeout=2.0,
    )
    if not lines:
        lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    if sandbox_info["marker"] not in screen:
        tui.quit()
        pytest.fail(
            "Failed to enter stable F7 preview state.\n"
            f"Expected marker '{sandbox_info['marker']}' not found.\n"
            f"Screen:\n{screen}"
        )

    return tui


def test_f7_vertical_separator_visibility(f7_preview_sandbox, ytnova_binary):
    """
    Required test 1:
    The interior divider between file list and preview pane must remain visible.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    lines = tui.get_screen_dump()

    separator_col, counts = _detect_preview_separator_column(lines)
    if separator_col is None or counts.get(separator_col, 0) < 10:
        tui.quit()
        pytest.fail(
            "F7 vertical separator not visible as a continuous interior line.\n"
            f"Interior line counts: {counts}\n"
            f"Screen:\n{_screen_text(lines)}"
        )

    tui.quit()


def test_f7_footer_menu_persistence(f7_preview_sandbox, ytnova_binary):
    """
    Required test 2:
    Footer command help must remain visible in preview mode and should expose
    useful file actions instead of preview-navigation reminders.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    lines = tui.get_screen_dump()
    footer = "\n".join(lines[-3:])
    footer_upper = footer.upper()
    footer_lower = footer.lower()

    if "PREVIEW" not in footer_upper or "COMMANDS" not in footer_upper:
        tui.quit()
        pytest.fail(
            "F7 footer menu is missing or blank.\n"
            f"Footer:\n{footer}\n"
            f"Screen:\n{_screen_text(lines)}"
        )

    for token in ("copy", "delete", "rename", "F7 exit preview".lower()):
        if token not in footer_lower:
            tui.quit()
            pytest.fail(
                "F7 footer should expose practical preview commands.\n"
                f"Missing token: {token!r}\n"
                f"Footer:\n{footer}\n"
                f"Screen:\n{_screen_text(lines)}"
            )

    for forbidden in ("select file", "navigate preview", "scroll page"):
        if forbidden in footer_lower:
            tui.quit()
            pytest.fail(
                "F7 footer should leave preview-navigation reminders to F1.\n"
                f"Unexpected token: {forbidden!r}\n"
                f"Footer:\n{footer}\n"
                f"Screen:\n{_screen_text(lines)}"
            )

    tui.quit()


def test_f7_blocks_split_and_tab_but_allows_copy_prompt(
    f7_preview_sandbox, ytnova_binary
):
    """
    Required test 3:
    Preview must still block split-mode entry, but common file actions such as
    Copy should remain usable without leaving preview.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    preview_lines = tui.get_screen_dump()
    preview_screen = _screen_text(preview_lines)

    tui.send_keystroke(Keys.TAB, wait=0.25)

    tab_lines = tui.get_screen_dump()
    tab_screen = _screen_text(tab_lines)
    if tab_lines[1:] != preview_lines[1:]:
        tui.quit()
        pytest.fail(
            "Tab should remain a no-op while F7 preview is active.\n"
            f"Before:\n{preview_screen}\n\nAfter:\n{tab_screen}"
        )

    tui.send_keystroke(Keys.F8, wait=0.25)

    lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    upper = screen.upper()

    if f7_preview_sandbox["marker"] not in screen:
        tui.quit()
        pytest.fail(
            "Preview content marker disappeared after blocked split input.\n"
            f"Screen:\n{screen}"
        )

    if "SPLIT" in upper and "SCREEN" in upper:
        tui.quit()
        pytest.fail(
            "F8 should remain blocked while F7 preview is active.\n"
            f"Screen:\n{screen}"
        )

    tui.send_keystroke(Keys.COPY, wait=0.25)
    copy_screen = _screen_text(tui.get_screen_dump())
    if "COPY:" not in copy_screen.upper():
        tui.quit()
        pytest.fail(
            "F7 preview should allow Copy without forcing an exit first.\n"
            f"Screen:\n{copy_screen}"
        )

    tui.send_keystroke(Keys.ESC, wait=0.25)
    restored_screen = _screen_text(tui.get_screen_dump())
    if f7_preview_sandbox["marker"] not in restored_screen:
        tui.quit()
        pytest.fail(
            "Cancelling Copy from F7 preview should return to the preview surface.\n"
            f"Screen:\n{restored_screen}"
        )

    tui.quit()


def test_f7_preview_action_filter_keeps_tagged_workflow_and_blocks_panel_switch():
    source = Path("src/ui/ctrl_file.c").read_text(encoding="utf-8")
    match = re.search(
        r"static YtreeNovaAction FilterPreviewAction\(YtreeNovaAction action\) \{"
        r"(?P<body>.*?)\n\}",
        source,
        re.S,
    )
    assert match is not None
    filter_body = match.group("body")

    required_actions = (
        "ACTION_CMD_C",
        "ACTION_FILTER",
        "ACTION_TAG_ALL",
        "ACTION_CMD_TAGGED_S",
        "ACTION_CMD_TAGGED_V",
        "ACTION_COMPARE_FILE",
        "ACTION_CMD_M",
        "ACTION_CMD_R",
    )

    for action in required_actions:
        assert action in filter_body, action

    forbidden_actions = ("ACTION_SWITCH_PANEL", "ACTION_MOVE_SIBLING_NEXT")
    for action in forbidden_actions:
        assert action not in filter_body, action


def test_f7_preview_search_highlight_contract_uses_tagged_matches():
    preview_source = Path("src/ui/view_preview.c").read_text(encoding="utf-8")
    tagged_view_source = Path("src/ui/tagged_view.c").read_text(encoding="utf-8")

    assert "ctx->global_search_term[0] != '\\0'" in preview_source
    assert "strcasestr(ptr, ctx->global_search_term)" in preview_source
    assert "if (fe->tagged && fe->matching)" in tagged_view_source
    assert "if (!(fe->tagged && fe->matching))" in tagged_view_source


def test_f7_file_name_clipping_at_boundaries(f7_preview_sandbox, ytnova_binary):
    """
    Required test 4:
    Very long filenames must be clipped to the file-list pane boundary.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    lines = tui.get_screen_dump()
    screen = _screen_text(lines)

    long_name = f7_preview_sandbox["long_name"]

    if long_name in screen:
        tui.quit()
        pytest.fail(
            "Long filename appears unbounded in F7 preview (no clipping).\n"
            f"Long name: {long_name}\n"
            f"Screen:\n{screen}"
        )

    separator_col, _ = _detect_preview_separator_column(lines)
    if separator_col is None:
        tui.quit()
        pytest.fail(
            "Could not detect preview separator while validating clipping.\n"
            f"Screen:\n{screen}"
        )

    # Find a clipped filename candidate in the left pane. Do not hardcode a
    # specific visible prefix length; that depends on pane width.
    clipped_row = None
    clipped_label = None
    for y in range(BORDER_MIN_Y, BORDER_MAX_Y + 1):
        row = lines[y]
        if len(row) <= separator_col:
            continue

        left_cell = row[2:separator_col].strip()
        if not left_cell.endswith("..."):
            continue

        visible_part = left_cell[:-3].rstrip()
        if visible_part and long_name.startswith(visible_part):
            clipped_row = y
            clipped_label = left_cell
            break

    if clipped_row is None:
        tui.quit()
        pytest.fail(
            "Could not locate a valid clipped filename in the left pane.\n"
            f"Screen:\n{screen}"
        )

    if lines[clipped_row][separator_col] not in VERTICAL_CHARS:
        tui.quit()
        pytest.fail(
            "Filename row crosses the pane boundary; separator not intact on the row.\n"
            f"Row: {clipped_row}\n"
            f"Detected clipped label: {clipped_label}\n"
            f"Separator column: {separator_col}\n"
            f"Screen:\n{screen}"
        )

    tui.quit()


def test_f7_window_border_integrity(f7_preview_sandbox, ytnova_binary):
    """
    Required test 5:
    Outer window frame must remain intact with no gaps while in preview.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    lines = tui.get_screen_dump()
    width = len(lines[0])

    # Check top and bottom corners of the main frame.
    if lines[1][0] == " " or lines[1][width - 1] == " ":
        tui.quit()
        pytest.fail(
            "Top frame corners are broken in F7 preview.\n"
            f"Screen:\n{_screen_text(lines)}"
        )

    if lines[32][0] == " " or lines[32][width - 1] == " ":
        tui.quit()
        pytest.fail(
            "Bottom frame corners are broken in F7 preview.\n"
            f"Screen:\n{_screen_text(lines)}"
        )

    # Verify continuous left/right vertical borders.
    for y in range(BORDER_MIN_Y, BORDER_MAX_Y + 1):
        if lines[y][0] == " ":
            tui.quit()
            pytest.fail(
                "Left border has a gap in F7 preview.\n"
                f"Gap row: {y}\n"
                f"Screen:\n{_screen_text(lines)}"
            )
        if lines[y][width - 1] == " ":
            tui.quit()
            pytest.fail(
                "Right border has a gap in F7 preview.\n"
                f"Gap row: {y}\n"
                f"Screen:\n{_screen_text(lines)}"
            )

    tui.quit()
