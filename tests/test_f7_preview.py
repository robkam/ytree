import pytest
from tui_harness import YtreeTUI
from ytree_keys import Keys


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


def _launch_preview(ytree_binary, sandbox_info):
    """
    Enter F7 preview from tree mode using project-standard keystrokes.
    """
    tui = YtreeTUI(executable=ytree_binary, cwd=str(sandbox_info["root"]))

    tui.send_keystroke(Keys.EXPAND_ALL, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.F7, wait=0.35)

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


def test_f7_vertical_separator_visibility(f7_preview_sandbox, ytree_binary):
    """
    Required test 1:
    The interior divider between file list and preview pane must remain visible.
    """
    tui = _launch_preview(ytree_binary, f7_preview_sandbox)
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


def test_f7_footer_menu_persistence(f7_preview_sandbox, ytree_binary):
    """
    Required test 2:
    Footer command help must remain visible in preview mode.
    """
    tui = _launch_preview(ytree_binary, f7_preview_sandbox)
    lines = tui.get_screen_dump()
    footer = "\n".join(lines[-3:]).upper()

    if "PREVIEW" not in footer or "COMMANDS" not in footer:
        tui.quit()
        pytest.fail(
            "F7 footer menu is missing or blank.\n"
            f"Footer:\n{footer}\n"
            f"Screen:\n{_screen_text(lines)}"
        )

    tui.quit()


def test_f7_modal_input_blocking(f7_preview_sandbox, ytree_binary):
    """
    Required test 3:
    Preview acts as a modal state. F8, Attribute, and Copy should not trigger
    their normal actions while preview is active.
    """
    tui = _launch_preview(ytree_binary, f7_preview_sandbox)

    tui.send_keystroke(Keys.F8, wait=0.25)
    tui.send_keystroke(Keys.ATTRIBUTE, wait=0.25)
    tui.send_keystroke(Keys.COPY, wait=0.25)

    lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    upper = screen.upper()

    forbidden_prompts = [
        "ATTRIBUTES:",
        "COPY:",
        "TO DIRECTORY:",
    ]
    for prompt in forbidden_prompts:
        if prompt in upper:
            tui.quit()
            pytest.fail(
                "Preview did not block modal-incompatible input.\n"
                f"Found forbidden prompt '{prompt}'.\n"
                f"Screen:\n{screen}"
            )

    if f7_preview_sandbox["marker"] not in screen:
        tui.quit()
        pytest.fail(
            "Preview content marker disappeared after blocked input keys.\n"
            f"Screen:\n{screen}"
        )

    tui.quit()


def test_f7_file_name_clipping_at_boundaries(f7_preview_sandbox, ytree_binary):
    """
    Required test 4:
    Very long filenames must be clipped to the file-list pane boundary.
    """
    tui = _launch_preview(ytree_binary, f7_preview_sandbox)
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


def test_f7_window_border_integrity(f7_preview_sandbox, ytree_binary):
    """
    Required test 5:
    Outer window frame must remain intact with no gaps while in preview.
    """
    tui = _launch_preview(ytree_binary, f7_preview_sandbox)
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
