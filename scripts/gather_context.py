#!/usr/bin/env python3
import os
import sys

# Configuration
IGNORE_DIRS = {'.git', '.venv', 'obj', 'build', '__pycache__', '.vscode'}
# Note: .md covers .1.md
INCLUDE_EXTS = {'.c', '.h', '.md', '.conf', '.py', '.sh'}
INCLUDE_FILES = {'Makefile'}

def is_relevant(filename):
    """Check if a file is relevant based on extension or exact name."""
    if filename in INCLUDE_FILES:
        return True
    _, ext = os.path.splitext(filename)
    return ext in INCLUDE_EXTS

def print_tree(startpath):
    """Prints a visual tree structure of the project."""
    print("# PROJECT STRUCTURE\n")
    for root, dirs, files in os.walk(startpath):
        # Filter directories in-place
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        
        level = root.replace(startpath, '').count(os.sep)
        indent = ' ' * 4 * (level)
        print(f"{indent}{os.path.basename(root)}/")
        subindent = ' ' * 4 * (level + 1)
        for f in sorted(files):
            if is_relevant(f):
                print(f"{subindent}{f}")
    print("\n" + "="*50 + "\n")

def dump_contents(startpath):
    """Dumps the content of all relevant files."""
    print("# FILE CONTENTS\n")
    
    file_count = 0
    total_chars = 0

    for root, dirs, files in os.walk(startpath):
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        
        for f in sorted(files):
            if is_relevant(f):
                path = os.path.join(root, f)
                try:
                    with open(path, 'r', encoding='utf-8', errors='replace') as content_file:
                        content = content_file.read()
                        
                    # Print Header
                    print(f"=== START FILE: {path} ===")
                    print(content)
                    print(f"=== END FILE: {path} ===\n")
                    
                    file_count += 1
                    total_chars += len(content)
                except Exception as e:
                    print(f"Error reading {path}: {e}", file=sys.stderr)

    # Print summary to stderr so it doesn't clutter the pipe if redirecting
    print(f"Processed {file_count} files.", file=sys.stderr)
    print(f"Total characters: {total_chars}", file=sys.stderr)
    print(f"Estimated Tokens: {total_chars // 4}", file=sys.stderr)

if __name__ == "__main__":
    base_dir = "."
    if len(sys.argv) > 1:
        base_dir = sys.argv[1]
        
    print_tree(base_dir)
    dump_contents(base_dir)
