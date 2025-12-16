#!/usr/bin/env python3
# chat-ytree.py
import os
import sys
import glob
import time
import atexit
import datetime
import argparse
import fnmatch
from pathlib import Path
import google.generativeai as genai
from google.api_core import exceptions

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------

API_KEY = os.environ.get("GEMINI_API_KEY")
if not API_KEY:
    print("Error: GEMINI_API_KEY environment variable not set.")
    sys.exit(1)

genai.configure(api_key=API_KEY)

#MODEL_NAME = "models/gemini-3-pro-preview"
#MODEL_NAME = "models/gemini-2.5-pro"
MODEL_NAME = "models/gemini-2.5-flash"

CHAT_LOG_FILE = "chat.log"
HISTORY_FILE = os.path.expanduser("~/.ytree_chat_history")

SAFETY_SETTINGS = [
    {"category": "HARM_CATEGORY_HARASSMENT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_HATE_SPEECH", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_SEXUALLY_EXPLICIT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_DANGEROUS_CONTENT", "threshold": "BLOCK_ONLY_HIGH"},
]

SYSTEM_INSTRUCTION_HEADER = """
You are a Senior Systems Developer acting as an interactive assistant for the 'ytree' project.

**YOUR CONTEXT:**
You have access to the file structure of the project.
**IMPORTANT:** You do NOT initially have the *contents* of these files. The user must load them for you.
If you need to read a file to answer a question, ask the user to run: `/load filename`

**YOUR ROLE: THE CONSULTANT**
1.  **Analyze & Explain:** Discuss the logic, data structures, and flow of the loaded code.
2.  **Diagnose:** Help identifying bugs or architectural weaknesses.
3.  **Requirements Gathering:** Help the user define what needs to be done.

**BEHAVIORAL CONSTRAINTS:**
*   **NO UNSOLICITED CODING:** Do **NOT** generate `task.txt` files or full source code unless explicitly prompted.
*   **DISCUSSION MODE:** Keep answers conversational and focused on the *why* and *where*.
*   **NO DIFFS:** Do not provide diffs. Provide short, illustrative snippets only.
"""

# -----------------------------------------------------------------------------
# LOGGING UTILS
# -----------------------------------------------------------------------------

def log_conversation(role, text):
    """Appends conversation to a log file with timestamp."""
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    try:
        with open(CHAT_LOG_FILE, "a", encoding="utf-8") as f:
            f.write(f"[{timestamp}] [{role}]\n{text}\n\n")
    except Exception as e:
        print(f"  [!] Failed to write to log: {e}")

# -----------------------------------------------------------------------------
# INPUT HANDLING
# -----------------------------------------------------------------------------

try:
    from prompt_toolkit import PromptSession
    from prompt_toolkit.history import FileHistory
    from prompt_toolkit.key_binding import KeyBindings
    HAVE_PTK = True
except ImportError:
    HAVE_PTK = False
    try:
        import readline
    except ImportError:
        readline = None

def setup_input():
    if HAVE_PTK:
        kb = KeyBindings()

        @kb.add('enter')
        def _(event):
            event.current_buffer.validate_and_handle()

        @kb.add('escape', 'enter')
        @kb.add('c-o')
        def _(event):
            event.current_buffer.insert_text('\n')

        session = PromptSession(
            history=FileHistory(HISTORY_FILE),
            key_bindings=kb,
            multiline=False
        )
        print("  [i] Enhanced input: Enter=Submit | Alt+Enter=New Line")
        return session.prompt
    else:
        if readline:
            if os.path.exists(HISTORY_FILE):
                try:
                    readline.read_history_file(HISTORY_FILE)
                except FileNotFoundError: pass
            atexit.register(readline.write_history_file, HISTORY_FILE)
            print("  [i] Basic input (readline) enabled.")
        return input

# -----------------------------------------------------------------------------
# UTILITIES
# -----------------------------------------------------------------------------

def wait_with_countdown(seconds, reason="Quota Exceeded"):
    """Displays a visual countdown timer."""
    print(f"\n[!] {reason}. Cooling down...")
    try:
        for i in range(seconds, 0, -1):
            sys.stdout.write(f"\r    Retrying in {i}s...   ")
            sys.stdout.flush()
            time.sleep(1)
        sys.stdout.write("\r    Retrying now...        \n")
    except KeyboardInterrupt:
        print("\n[!] Wait interrupted.")
        raise KeyboardInterrupt

def generate_chat_response_with_retry(chat_session, user_input, max_retries=5):
    # Initial wait time (seconds)
    delay = 10

    for attempt in range(max_retries):
        try:
            response = chat_session.send_message(user_input, safety_settings=SAFETY_SETTINGS)
            return response
        except exceptions.ResourceExhausted:
            # Handle 429 Quota Exceeded
            try:
                wait_with_countdown(delay)
                delay *= 2  # Exponential backoff
            except KeyboardInterrupt:
                return None
        except Exception as e:
            print(f"\n[!] Unexpected error: {e}")
            return None

    print("\n[!] Max retries reached. The API is too busy.")
    return None

# -----------------------------------------------------------------------------
# FILE SYSTEM TOOLS
# -----------------------------------------------------------------------------

def get_all_project_files(root_dir="."):
    """Returns a sorted list of all relevant files in the project."""
    extensions = {'.c', '.h', '.conf', '.sh', '.py', '.md', 'Makefile'}
    ignore_dirs = {'.git', '.venv', '.vscode', 'build', '__pycache__', 'obj', 'docs'}
    ignore_files = {'copy.c.bak', 'ytree.tar', 'chat.log', 'build.log'}

    file_list = []

    for path in Path(root_dir).rglob("*"):
        if any(part in ignore_dirs for part in path.parts):
            continue
        if path.is_file():
            if path.name in ignore_files or path.name.endswith('~') or path.name.startswith('.'):
                continue
            if path.suffix in extensions or path.name in ['Makefile', 'Changes']:
                file_list.append(str(path))

    return sorted(file_list)

def read_files_content(file_paths):
    """Reads content of specific files."""
    buffer = ""
    count = 0
    for path_str in file_paths:
        try:
            with open(path_str, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
                buffer += f"=== FILE: {path_str} ===\n{content}\n\n"
                count += 1
        except Exception as e:
            print(f"  [!] Error reading {path_str}: {e}")
    return buffer, count

# -----------------------------------------------------------------------------
# CHAT SESSION
# -----------------------------------------------------------------------------

def start_chat(initial_context_filter=None):
    print("\n" + "="*50)
    print(f"YTREE AI ASSISTANT ({MODEL_NAME})")
    print("="*50)
    print(f"Logging to: {CHAT_LOG_FILE}")

    get_input = setup_input()

    # 1. Get File Structure (Low Token Cost)
    all_files = get_all_project_files()
    file_tree_str = "\n".join(all_files)

    # 2. Handle Initial Context Loading (if --context used)
    initial_content = ""
    loaded_files_msg = "No files loaded yet."

    if initial_context_filter:
        to_load = []
        patterns = initial_context_filter.replace(",", " ").split()
        for pat in patterns:
            # Simple substring or exact match logic
            for f in all_files:
                if fnmatch.fnmatch(f, pat) or pat in f:
                    if f not in to_load:
                        to_load.append(f)

        if to_load:
            content, count = read_files_content(to_load)
            initial_content = f"\n\n**INITIALLY LOADED FILES:**\n{content}"
            loaded_files_msg = f"Loaded {count} files based on argument: {initial_context_filter}"
            print(f"  [i] {loaded_files_msg}")

    # 3. Construct System Prompt
    full_system_prompt = f"""{SYSTEM_INSTRUCTION_HEADER}

**PROJECT FILE STRUCTURE:**
{file_tree_str}

{initial_content}
"""

    model = genai.GenerativeModel(
        model_name=MODEL_NAME,
        system_instruction=full_system_prompt
    )

    chat = model.start_chat(history=[])

    print("-" * 50)
    print("Commands:")
    print("  /load <pattern> : Load file content (e.g. '/load main.c' or '/load *.h')")
    print("  /files          : List all available files.")
    print("  /clear          : Clear conversation history.")
    print("  /quit           : Exit.")
    print("-" * 50)

    reason_map = {
        0: "UNKNOWN",
        1: "STOP",
        2: "MAX_TOKENS",
        3: "SAFETY",
        4: "RECITATION",
        5: "OTHER"
    }

    while True:
        try:
            user_input = get_input("\n[You]: ").strip()

            if not user_input:
                continue

            # --- COMMAND HANDLING ---

            if user_input.lower() in ['/quit', '/exit']:
                print("Goodbye.")
                break

            if user_input.lower() == '/files':
                print("\nProject Files:")
                for f in all_files:
                    print(f"  {f}")
                continue

            if user_input.lower().startswith('/load '):
                pattern = user_input[6:].strip()
                matches = []
                for f in all_files:
                    if pattern in f or fnmatch.fnmatch(os.path.basename(f), pattern):
                        matches.append(f)

                if not matches:
                    print(f"  [!] No files matched '{pattern}'")
                    continue

                content, count = read_files_content(matches)
                # Send as a "User" message to inject into history
                # We prefix it so the Model knows it's a system injection
                injection_msg = f"SYSTEM: The user has loaded the following files:\n{content}"

                print(f"  [i] Loading {count} files into context...")
                response = generate_chat_response_with_retry(chat, injection_msg)
                if response:
                    print(f"  [AI] Acknowledged. {count} files loaded.")
                continue

            if user_input.lower() == '/clear':
                print("\nClearing conversation history...")
                chat.history.clear()
                # Re-inject system context if needed, but usually system_instruction persists.
                # However, loaded files in history are gone.
                print("  [!] Note: Previously loaded files are cleared from immediate memory.")
                continue

            # --- NORMAL CHAT ---

            log_conversation("USER", user_input)
            print("[AI is thinking...]")
            response = generate_chat_response_with_retry(chat, user_input)

            if response:
                try:
                    text = response.text
                    print(f"\n[AI]:\n{text}")
                    log_conversation("AI", text)

                    if response.candidates and response.candidates[0].finish_reason != 1:
                        c = response.candidates[0]
                        reason_str = reason_map.get(c.finish_reason, "UNKNOWN")
                        print(f"\n[!] Warning: Response truncated. Reason: {reason_str}")

                except ValueError:
                    print(f"\n[!] Error: Model response blocked or empty.")

        except KeyboardInterrupt:
            print("\nInterrupted. Type /quit to exit.")
        except EOFError:
            print("\nEOF. Exiting.")
            break
        except Exception as e:
            print(f"\nError: {e}")

def main():
    parser = argparse.ArgumentParser(description="Ytree Chat Assistant")
    parser.add_argument("--context", help="Comma-separated patterns of files to preload (e.g., 'main.c,*.h')")
    args = parser.parse_args()

    start_chat(initial_context_filter=args.context)

if __name__ == "__main__":
    main()