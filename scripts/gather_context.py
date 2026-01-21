#!/usr/bin/env python3
"""
gather_context.py - Interactive Context Gatherer for ytree AI

Features:
- Interactive TUI to select files.
- Auto-detects project root.
- Saves output to 'context.txt' in project root.
- Copies output to clipboard automatically (if supported).
- Generates a file tree + file contents.
"""

import os
import sys
import argparse
import subprocess
import fnmatch
from prompt_toolkit import Application
from prompt_toolkit.key_binding import KeyBindings
from prompt_toolkit.layout import Layout, HSplit, Window
from prompt_toolkit.layout.controls import FormattedTextControl
from prompt_toolkit.formatted_text import HTML
from prompt_toolkit.styles import Style

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------

# Directories to ignore completely
IGNORE_DIRS = {'.git', '.venv', 'obj', 'build', '__pycache__', '.vscode', 'scripts', '.pytest_cache', 'screenshots'}

# Allowed extensions
INCLUDE_EXTS = {'.c', '.h', '.md', '.conf', '.py', '.sh'}

# Specific files to always include unless ignored
INCLUDE_FILES = {'Makefile'}

# Default patterns to uncheck in the UI (User can toggle them back)
DEFAULT_EXCLUDES = {'LICENSE.md', 'why_ytree.md', 'USAGE.md', 'AUTHORS.md', 'CHANGES.md', 'chat.log', 'build.log', 'context.txt'}

# -----------------------------------------------------------------------------
# LOGIC
# -----------------------------------------------------------------------------

def is_text_file(filename):
    """Check if file is relevant based on extension or exact name."""
    if filename in INCLUDE_FILES:
        return True
    _, ext = os.path.splitext(filename)
    return ext in INCLUDE_EXTS

def get_project_files(root_dir):
    """Scans for all relevant files."""
    file_list = []
    for root, dirs, files in os.walk(root_dir):
        # Filter directories in-place
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]

        for f in sorted(files):
            if is_text_file(f):
                full_path = os.path.join(root, f)
                rel_path = os.path.relpath(full_path, root_dir)
                file_list.append(rel_path)
    return sorted(file_list)

def copy_to_clipboard(text):
    """Copies text to clipboard using system tools."""
    try:
        if sys.platform == 'darwin':
            process = subprocess.Popen(['pbcopy'], stdin=subprocess.PIPE)
            process.communicate(text.encode('utf-8'))
            return True
        elif sys.platform.startswith('linux'):
            # Try xclip (X11)
            try:
                process = subprocess.Popen(['xclip', '-selection', 'clipboard'], stdin=subprocess.PIPE)
                process.communicate(text.encode('utf-8'))
                return True
            except FileNotFoundError:
                pass

            # Try xsel (X11)
            try:
                process = subprocess.Popen(['xsel', '-b', '-i'], stdin=subprocess.PIPE)
                process.communicate(text.encode('utf-8'))
                return True
            except FileNotFoundError:
                pass

            # Try wl-copy (Wayland)
            try:
                process = subprocess.Popen(['wl-copy'], stdin=subprocess.PIPE)
                process.communicate(text.encode('utf-8'))
                return True
            except FileNotFoundError:
                pass

            # Try clip.exe (WSL)
            try:
                process = subprocess.Popen(['clip.exe'], stdin=subprocess.PIPE)
                process.communicate(text.encode('utf-8'))
                return True
            except FileNotFoundError:
                pass

    except Exception:
        pass
    return False

# -----------------------------------------------------------------------------
# OUTPUT GENERATION
# -----------------------------------------------------------------------------

def generate_output(root_dir, selected_files):
    """Generates the final formatted string."""
    output = []

    # 1. Project Structure Tree
    output.append("# PROJECT STRUCTURE\n")

    for root, dirs, files in os.walk(root_dir):
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]

        rel_path = os.path.relpath(root, root_dir)
        if rel_path == ".":
            level = 0
        else:
            level = rel_path.count(os.sep) + 1

        indent = ' ' * 4 * level
        output.append(f"{indent}{os.path.basename(root)}/")

        subindent = ' ' * 4 * (level + 1)
        for f in sorted(files):
            if is_text_file(f):
                f_rel = os.path.join(rel_path, f)
                if rel_path == ".": f_rel = f

                # Mark if it's included in the dump
                marker = ""
                if f_rel not in selected_files:
                    marker = " (Excluded)"
                output.append(f"{subindent}{f}{marker}")

    output.append("\n" + "="*50 + "\n")
    output.append("# FILE CONTENTS\n")

    file_count = 0
    total_chars = 0

    for rel_path in selected_files:
        full_path = os.path.join(root_dir, rel_path)
        try:
            with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
                output.append(f"=== START FILE: {rel_path} ===")
                output.append(content)
                output.append(f"=== END FILE: {rel_path} ===\n")
                file_count += 1
                total_chars += len(content)
        except Exception as e:
            output.append(f"Error reading {rel_path}: {e}\n")

    final_text = "\n".join(output)
    return final_text, file_count, total_chars

# -----------------------------------------------------------------------------
# TUI INTERFACE
# -----------------------------------------------------------------------------

class FileSelector:
    def __init__(self, root_dir, files):
        self.root_dir = root_dir
        self.files = files
        self.selected = set()
        self.cursor_index = 0
        self.scroll_offset = 0

        # Initialize selection (default excludes)
        for f in files:
            is_excluded = False
            for pat in DEFAULT_EXCLUDES:
                if fnmatch.fnmatch(os.path.basename(f), pat):
                    is_excluded = True
                    break
            if not is_excluded:
                self.selected.add(f)

    def get_display_text(self):
        """Renders the visible portion of the file list."""
        lines = []
        height = shutil_get_terminal_size().lines - 6 # Reserve space for header/footer
        if height < 5: height = 5

        # Adjust scrolling
        if self.cursor_index < self.scroll_offset:
            self.scroll_offset = self.cursor_index
        elif self.cursor_index >= self.scroll_offset + height:
            self.scroll_offset = self.cursor_index - height + 1

        visible_files = self.files[self.scroll_offset : self.scroll_offset + height]

        for i, filename in enumerate(visible_files):
            real_index = self.scroll_offset + i

            # Checkbox
            check = "[x]" if filename in self.selected else "[ ]"

            # Styling
            style = ""
            if real_index == self.cursor_index:
                style = "class:current-line"
                prefix = "> "
            else:
                prefix = "  "

            # Formatting line
            line_content = f"{prefix}{check} {filename}"
            lines.append((style, line_content + "\n"))

        return lines

    def run(self):
        kb = KeyBindings()

        @kb.add('up')
        @kb.add('k')
        def _(event):
            self.cursor_index = max(0, self.cursor_index - 1)

        @kb.add('down')
        @kb.add('j')
        def _(event):
            self.cursor_index = min(len(self.files) - 1, self.cursor_index + 1)

        @kb.add('space')
        def _(event):
            f = self.files[self.cursor_index]
            if f in self.selected:
                self.selected.remove(f)
            else:
                self.selected.add(f)

        @kb.add('a')
        def _(event):
            self.selected = set(self.files)

        @kb.add('n')
        def _(event):
            self.selected = set()

        @kb.add('enter')
        def _(event):
            event.app.exit(result=True)

        @kb.add('c-c')
        @kb.add('q')
        def _(event):
            event.app.exit(result=False)

        # Status Bar Text
        def get_status_bar():
            count = len(self.selected)
            total = len(self.files)
            return HTML(f" <b>{count}/{total}</b> files selected. | <b>Space</b>: Toggle | <b>a</b>: All | <b>n</b>: None | <b>Enter</b>: Generate")

        # Layout
        root_container = HSplit([
            Window(height=1, content=FormattedTextControl(HTML("<b>YTREE CONTEXT GATHERER</b> | Use Up/Down/j/k to navigate"))),
            Window(height=1, char='-'),
            Window(content=FormattedTextControl(self.get_display_text)),
            Window(height=1, char='-'),
            Window(height=1, content=FormattedTextControl(get_status_bar), style="class:status"),
        ])

        layout = Layout(root_container)

        style = Style.from_dict({
            'current-line': 'reverse',
            'status': 'bg:blue white',
        })

        app = Application(layout=layout, key_bindings=kb, full_screen=True, style=style)
        return app.run()

# Helper for terminal size (fallback)
def shutil_get_terminal_size():
    import shutil
    return shutil.get_terminal_size((80, 24))

# -----------------------------------------------------------------------------
# MAIN
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", action="store_true", help="Run in non-interactive mode (dump all)")
    parser.add_argument("--exclude", nargs='*', help="Patterns to exclude in CLI mode")
    parser.add_argument("--output", help="Output file (default: context.txt)", default="context.txt")
    args = parser.parse_args()

    # Detect Root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    # Resolve output file path
    # If it's just a filename, put it in project root. If path, verify logic?
    # Let's keep it simple: always relative to PROJECT_ROOT if not absolute.
    if os.path.isabs(args.output):
        output_file = args.output
    else:
        output_file = os.path.join(project_root, args.output)

    files = get_project_files(project_root)
    selected = set(files) # Default to all

    if args.cli:
        # CLI Mode
        if args.exclude:
            for f in list(selected):
                for pat in args.exclude:
                    if fnmatch.fnmatch(os.path.basename(f), pat):
                        if f in selected: selected.remove(f)
    else:
        # TUI Mode
        selector = FileSelector(project_root, files)
        success = selector.run()
        if not success:
            print("Aborted.")
            sys.exit(0)
        selected = selector.selected

    # Generate
    print(f"Generating context for {len(selected)} files...", file=sys.stderr)
    content, count, chars = generate_output(project_root, selected)

    # Write to File
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"[SUCCESS] Context saved to: {output_file}", file=sys.stderr)
    except Exception as e:
        print(f"[ERROR] Failed to write context file: {e}", file=sys.stderr)
        # Fallback to stdout? No, let's keep it clean.

    # Try Clipboard (Convenience)
    if copy_to_clipboard(content):
        print(f"[SUCCESS] Also copied to clipboard.", file=sys.stderr)
    else:
        print(f"[NOTE] Clipboard copy skipped (tool not found or error).", file=sys.stderr)