"""Regression coverage for panel anchor capture invariants."""

from pathlib import Path


def test_capture_panel_anchor_falls_back_when_file_context_is_unavailable():
    source = Path("src/ui/panel_anchor.c").read_text()
    start = source.index("BOOL CapturePanelAnchorPath")
    end = source.index("void CapturePanelViewportSnapshot", start)
    capture = source[start:end]

    assert "panel->saved_focus == FOCUS_FILE" in capture
    assert "if (panel->file_selection_dir_path[0])" in capture
    assert "panel->saved_focus != FOCUS_FILE" not in capture
