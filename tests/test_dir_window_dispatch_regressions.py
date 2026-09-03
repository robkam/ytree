import re

from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_text as _footer_text
from helpers_ui import line_marks_file_as_tagged as _line_marks_file_as_tagged
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _first_visible_edge_tree_row(tui):
    for line in tui.get_screen_dump():
        if not line.startswith("x"):
            continue
        match = re.search(r"dir_\d{2}_edge_scroll\b", line[:60])
        if match:
            return match.group(0)
    return None


def test_tree_viewport_edge_scroll_after_end_up_keeps_top_visible_row(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_edge_scroll"
    root.mkdir()
    for idx in range(45):
        (root / f"dir_{idx:02d}_edge_scroll").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke("\033OF", wait=0.35)
    assert tui.wait_for_content(
        "dir_44_edge_scroll", timeout=1.0
    ), _screen_text(tui)
    top_after_end = _first_visible_edge_tree_row(tui)
    assert top_after_end is not None, _screen_text(tui)

    tui.send_keystroke(Keys.UP, wait=0.35)

    screen = _screen_text(tui)
    assert "dir_43_edge_scroll" in screen, (
        "UP after END should move selection to the previous tree row.\n" f"{screen}"
    )
    assert _first_visible_edge_tree_row(tui) == top_after_end, (
        "UP after END should keep the tree viewport origin stable while the "
        "new selection remains inside the visible window.\n"
        f"top_after_end={top_after_end!r}\n{screen}"
    )

    steps = 0
    while "dir_27_edge_scroll" not in tui.get_screen_dump()[0]:
        if steps >= 45:
            raise AssertionError(
                "Could not select dir_27_edge_scroll at the viewport edge.\n"
                f"{_screen_text(tui)}"
            )
        tui.send_keystroke(Keys.UP, wait=0.05)
        steps += 1

    screen = _screen_text(tui)
    assert "dir_27_edge_scroll" in screen, (
        "Selection should be able to move to the top visible row without "
        "moving the tree viewport.\n"
        f"{screen}"
    )
    assert _first_visible_edge_tree_row(tui) == top_after_end, screen

    tui.send_keystroke(Keys.UP, wait=0.2)

    screen = _screen_text(tui)
    assert _first_visible_edge_tree_row(tui) == "dir_26_edge_scroll", (
        "Once UP moves past the top visible row, the tree should scroll by "
        "one row.\n"
        f"{screen}"
    )

    tui.quit()


def test_dir_window_navigation_selects_expected_directory(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_nav"
    root.mkdir()
    alpha = root / "alpha_dir_dispatch"
    beta = root / "beta_dir_dispatch"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only_file.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_only_file.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = _screen_text(tui)
    footer = _footer_text(tui)
    assert "beta_only_file.txt" in screen, (
        "Tree navigation + enter should open the selected directory's file list.\n"
        f"{screen}"
    )

    tui.quit()


def test_tab_wraps_to_the_next_visible_tree_sibling(ytnova_binary, tmp_path):
    home = tmp_path / "home"
    root = tmp_path / "tab_visible_sibling_wrap"
    home.mkdir()
    root.mkdir()
    (home / ".ytnova").write_text(
        "[GLOBAL]\nHIDEDOTFILES=1\nTREEDEPTH=1\nSMALLWINDOWSKIP=1\n",
        encoding="utf-8",
    )
    for name in (".hidden_first_sibling", "visible_first_sibling", "visible_last_sibling"):
        (root / name).mkdir()

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), env_extra={"HOME": str(home)}
    )
    try:
        assert tui.wait_for_content("visible_last_sibling", timeout=1.5), _screen_text(tui)
        tui.send_keystroke(Keys.DOWN, wait=0.05)
        reached_last = tui.send_and_wait_for_condition(
            Keys.DOWN,
            lambda lines: lines
            if any("Path:" in line and "visible_last_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert reached_last, _screen_text(tui)

        wrapped = tui.send_and_wait_for_condition(
            Keys.TAB,
            lambda lines: lines
            if any("Path:" in line and "visible_first_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert wrapped, _screen_text(tui)

        reverse_wrapped = tui.send_and_wait_for_condition(
            Keys.SHIFT_TAB,
            lambda lines: lines
            if any("Path:" in line and "visible_last_sibling" in line for line in lines)
            else False,
            timeout=1.0,
        )
        assert reverse_wrapped, _screen_text(tui)
    finally:
        tui.quit()


def test_dir_window_compare_prompt_round_trip(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_compare_prompt"
    root.mkdir()
    (root / "left").mkdir()
    (root / "right").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke("J", wait=0.25)
    assert tui.wait_for_content("COMPARE TARGET [", timeout=1.0), _screen_text(tui)

    tui.send_keystroke(Keys.ESC, wait=0.25)
    footer = _footer_text(tui)

    tui.quit()


def test_dir_window_split_and_tab_keeps_file_focus(ytnova_binary, tmp_path):
    root = tmp_path / "dir_dispatch_split_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_split_focus.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_split_focus.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    footer = _footer_text(tui)

    tui.quit()


def test_split_tab_refresh_rejects_stale_file_restore_snapshot(
    ytnova_binary, tmp_path
):
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in (
        "build",
        "coverage",
        "docs",
        "etc",
        "include",
        "infra",
        "src",
        "tests",
    ):
        (repo / name).mkdir()
    src_cmd = repo / "src" / "cmd"
    src_cmd.mkdir()
    tests_dir = repo / "tests"
    (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")

    for idx in range(3):
        (src_cmd / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
    for idx in range(4):
        (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(repo), env_extra={"HOME": str(home)}
    )

    try:
        def stats_current_dir_contains(marker):
            lines = tui.get_screen_dump()
            split_x = _detect_stats_split_x(lines)
            for i, line in enumerate(lines):
                segment = line[split_x:] if split_x is not None else line
                if "CURRENT DIR" not in segment:
                    continue
                for j in (1, 2):
                    idx = i + j
                    if idx >= len(lines):
                        continue
                    candidate = (
                        lines[idx][split_x:] if split_x is not None else lines[idx]
                    )
                    if marker in candidate:
                        return True
            return False

        steps = 0
        while not stats_current_dir_contains("src"):
            if steps >= 80:
                raise AssertionError(
                    f"Could not focus the src fixture directory.\n{_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.DOWN, wait=0.12)
            steps += 1

        tui.send_keystroke(Keys.RIGHT, wait=0.25)

        steps = 0
        while not stats_current_dir_contains("cmd"):
            if steps >= 80:
                raise AssertionError(
                    f"Could not focus the cmd fixture directory.\n{_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.DOWN, wait=0.12)
            steps += 1

        tui.send_keystroke(Keys.RIGHT, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.45)

        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        selected_name = "src_file_2.c"
        pre_screen = _screen_text(tui)
        pre_tag_state = {}
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = next((line for line in pre_screen.splitlines() if name in line), None)
            assert line is not None, pre_screen
            pre_tag_state[name] = _line_marks_file_as_tagged(line, name)
        assert any(pre_tag_state.values()), (
            "Precondition failed: no tagged source files before split flow.\n"
            f"{pre_screen}"
        )

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content(f"COPY: {selected_name}", timeout=1.0), (
            _screen_text(tui)
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)

        tui.send_keystroke(Keys.TAB, wait=0.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)

        after_tab = _screen_text(tui)

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content(f"COPY: {selected_name}", timeout=1.0), (
            _screen_text(tui)
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)

        for name, expected_tagged in pre_tag_state.items():
            line = next((line for line in after_tab.splitlines() if name in line), None)
            assert line is not None, (
                "Source panel lost file rows after split restore.\n"
                f"{after_tab}"
            )
            assert _line_marks_file_as_tagged(line, name) == expected_tagged, (
                "Source panel tag state changed after in-app generation bump.\n"
                f"Expected tagged={expected_tagged} Row: {line}\n"
                f"{after_tab}"
            )
    finally:
        tui.quit()


def test_dir_right_arrow_drills_into_first_child_when_already_expanded(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_dispatch_right_drill"
    root.mkdir()
    parent = root / "parent_dir_dispatch"
    child = parent / "child_dir_dispatch"
    child.mkdir(parents=True)
    (child / "child_only_marker.txt").write_text("child\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.25)  # select parent_dir_dispatch
    tui.send_keystroke(Keys.RIGHT, wait=0.35)  # expand parent
    tui.send_keystroke(Keys.RIGHT, wait=0.35)  # drill into first child

    screen = _screen_text(tui)
    assert "parent_dir_dispatch/child_dir_dispatch" in screen, (
        "RIGHT on an already-expanded node should move selection to its first "
        "child and update the active path.\n"
        f"{screen}"
    )

    tui.quit()
