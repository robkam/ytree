#!/usr/bin/env python3
import sys
import os
import re

def extract_files(input_stream):
    """
    Reads from an input stream line by line and extracts files based on
    two delimiter formats: Markdown backticks and '=== START/END FILE ==='.
    """
    in_block = False
    current_file_handle = None
    current_file_path = ""
    prev_line = ""

    for line in input_stream:
        # --- Format 1: Markdown ``` blocks ---
        if line.strip().startswith('```'):
            if in_block:
                # Closing a block
                print(f"Finished: {current_file_path}")
                if current_file_handle:
                    current_file_handle.close()
                in_block = False
                current_file_handle = None
                current_file_path = ""
            else:
                # Opening a block, filename is the previous line
                filename = prev_line.strip()
                if filename:
                    current_file_path = filename
                    print(f"Extracting: {current_file_path}")
                    # Ensure directory exists
                    dir_name = os.path.dirname(current_file_path)
                    if dir_name:
                        os.makedirs(dir_name, exist_ok=True)
                    current_file_handle = open(current_file_path, 'w')
                    in_block = True
                else:
                    print("Warning: Found code block start but could not determine filename.", file=sys.stderr)

        # --- Format 2: '=== START/END FILE ===' blocks ---
        elif (match := re.match(r'^===\s*START\s*FILE:\s*(.*?)\s*===$', line.strip())):
            filename = match.group(1).strip()
            if filename:
                current_file_path = filename
                print(f"Extracting: {current_file_path}")
                # Ensure directory exists
                dir_name = os.path.dirname(current_file_path)
                if dir_name:
                    os.makedirs(dir_name, exist_ok=True)
                current_file_handle = open(current_file_path, 'w')
                in_block = True
            else:
                print("Warning: Found 'START FILE' block but could not extract filename.", file=sys.stderr)

        elif line.strip().startswith('=== END FILE:'):
            if in_block:
                print(f"Finished: {current_file_path}")
                if current_file_handle:
                    current_file_handle.close()
                in_block = False
                current_file_handle = None
                current_file_path = ""

        # --- Content writing ---
        elif in_block and current_file_handle:
            current_file_handle.write(line)

        prev_line = line

    print("Extraction complete.")

def main():
    """
    Main function to handle input from a file argument or stdin.
    """
    if not sys.stdin.isatty() or len(sys.argv) > 1:
        if len(sys.argv) > 1:
            try:
                with open(sys.argv[1], 'r') as file_input:
                    extract_files(file_input)
            except FileNotFoundError:
                print(f"Error: File not found - {sys.argv[1]}", file=sys.stderr)
                sys.exit(1)
        else:
            extract_files(sys.stdin)
    else:
        script_name = sys.argv[0]
        print(f"Usage: {script_name} <file_with_code_blocks>")
        print(f"   or: cat output.txt | {script_name}")
        sys.exit(1)

if __name__ == "__main__":
    main()