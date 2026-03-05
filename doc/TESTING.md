# Testing Guide

## 1. Naming & Documentation

Test names and structures must describe **what behavior is being tested**, completely independent of issue tracker numbers.

*   **Files:** `test_<feature_area>.py` (e.g., `test_stats_display.py`)
*   **Classes:** `Test<ComponentName><Aspect>` (e.g., `TestFooterVisibility`)
*   **Methods:** `test_<component>_<behavior>_<context>` (e.g., `test_footer_remains_visible_during_transition`)
*   **Fixtures:** `lowercase_with_underscores` (e.g., `ytree_binary`)

### The "Good vs. Bad" Standard

**BAD:**
```python
# Unclear generic names, tests multiple unrelated things, references bug numbers
def test_issue_123():
def test_ui():
def test_for_3_new_bugs():
```

**GOOD:**
```python
# Self-documenting, grouped by component, isolated behaviors
class TestDirectoryAttributesDisplay:
    """Tests for ATTRIBUTES section visibility and synchronization."""

    def test_attributes_appear_on_startup(self, ...):
    def test_attributes_sync_with_cursor_movement(self, ...):

class TestFooterVisibility:
    """Tests for footer display stability across all UI modes."""

    def test_footer_visible_in_directory_mode(self, ...):
    def test_footer_no_flicker_entering_big_window(self, ...):
```

### Required Docstrings
Every test must include a docstring explaining its intent:
1. **What bug it detects** (if fixing a regression).
2. **Expected behavior**.
3. **User report context** (if applicable).

---

## 2. Structure & Organization

1. **One Test Per Behavior:** Do not duplicate coverage. If you are testing the footer, stats, and attributes, those belong in separate test functions, organized under their respective test classes.
2. **File Isolation:** One major functional area per file (e.g., keep `test_navigation.py` strictly separate from `test_file_operations.py`).
3. **Shared State:** If tests share setup code, use fixtures or class setup methods instead of copy-pasting initialization logic.

---

## 3. Test-Driven Debugging Workflow

Always prove the bug exists in the test suite before fixing it in the source code:

1. **Write the test FIRST** to target the specific bug.
2. **Verify the test FAILS** (confirming your test accurately detects the issue).
3. **Implement the fix** in the codebase.
4. **Verify the test PASSES**.
5. **Run the full test suite** to ensure your fix didn't introduce regressions.

---

## 4. CLI Cheat Sheet

```bash
# Activate the virtual environment before running tests
source .venv/bin/activate

# Run all tests
pytest tests/ -v

# Run a specific test file
pytest tests/test_footer_display.py -v

# Run a specific test class
pytest tests/test_footer_display.py::TestFooterVisibility -v

# Run a specific test method
pytest tests/test_footer_display.py::TestFooterVisibility::test_footer_visible_in_directory_mode -v

# Run tests matching a keyword/pattern
pytest tests/ -k "footer or big_window" -v
```