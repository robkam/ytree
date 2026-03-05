# Testing Guide

This document is the canonical reference for test naming, structure, workflow, and infrastructure standards for the `ytree` project.

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
3. **User report context** (if applicable). Paraphrase the report clearly and professionally; do not quote informal or unclear language verbatim.

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

This workflow implements the broader **Spec → Test → Implement → Refactor** loop defined in [AI_WORKFLOW.md §1.1 "The Golden Loop"](AI_WORKFLOW.md). The critical rule: if the implementation behaves differently than the Spec, **the implementation is wrong** — never change the test to match broken code.

---

## 4. Running Tests

### The Preferred Method: `make test`

This is the standard way to run tests and should be used for general regression checks and CI:

```bash
# Activate the virtual environment
source .venv/bin/activate

# Standard run (quiet, shows progress dots)
make test

# Verbose run (shows individual test names and stdout)
make test-v
```

`make test` automatically recompiles the C binary if source files have changed, ensuring you always test against the latest code.

### Direct CLI: `pytest`

Use `pytest` directly when debugging, developing new features, or iterating on specific test cases:

```bash
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

# Stop on the first failure (useful for debugging loops)
pytest -x
```

**Tip:** For memory-error debugging, compile with AddressSanitizer enabled (`make clean && make DEBUG=1`) before running the test suite. See [CONTRIBUTING.md §"Debug Build"](CONTRIBUTING.md) for details.

---

## 5. Test Infrastructure

The test suite uses `pytest` and `pexpect` to simulate user interaction with the ncurses TUI in a headless PTY environment.

| File | Purpose |
|:---|:---|
| `tests/conftest.py` | Pytest fixtures for setup and teardown (sandbox directories, binary path). |
| `tests/ytree_control.py` | The PTY controller that drives `ytree`, handling input clearing, screen reading, and timeouts. |
| `tests/ytree_keys.py` | Centralized `Keys` class defining all key commands (e.g., `Keys.COPY`, `Keys.DELETE`). |
| `tests/test_core.py` | Main regression tests for core features (Copy, Move, Rename). |

---

## 6. Test Harness Rules

These constraints ensure tests are reliable, portable, and maintainable. For the full persona specification, see [`.agent/rules/tester.md`](../.agent/rules/tester.md).

### Abstraction over Hardcoding
Never hardcode keystrokes (e.g., sending `'c'`) inside a test function. Always use the `Keys` class (e.g., `Keys.COPY`). This ensures that if keybindings change in the C source, only one Python definition file needs updating.

### Sandbox Fixtures (No Absolute Paths)
Never use absolute paths like `/home/user` or `/mnt/p`. Always use the `sandbox` fixture to generate temporary directories (`/tmp/pytest-of-user/...`). Tests must run on Linux, WSL, and macOS without modification.

### Fail-Fast Timeouts
`ytree` is a highly optimized, single-threaded C application that responds to keystrokes instantly. If `pexpect` times out, it means the test's state machine is out of sync — not that the application is "processing."

*   **Always** use short, aggressive timeouts (1–2 seconds maximum).
*   **Never** use long timeouts or looping `try/except` blocks hoping the UI will catch up.
*   If a timeout occurs, let the test fail immediately with a descriptive error including the last screen dump.

### Tripartite Verification
A test is not complete until you verify all three layers:
1. **UI Layer:** Did the expected prompt or screen content appear?
2. **System Layer:** Did the file actually move/copy/delete on disk?
3. **Data Layer:** Is the content of the resulting file correct?