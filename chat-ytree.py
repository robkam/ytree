#!/usr/bin/env python3
# chat-ytree.py
import os
import sys
import glob
import time
import atexit
import datetime
import google.generativeai as genai
from google.api_core import exceptions
from pathlib import Path

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------

API_KEY = os.environ.get("GEMINI_API_KEY")
if not API_KEY:
    print("Error: GEMINI_API_KEY environment variable not set.")
    sys.exit(1)

genai.configure(api_key=API_KEY)

# MODEL NOTE: Gemini 2.5 Flash is adequate for general Q&A, context retrieval,
# and basic debugging.
MODEL_NAME = "models/gemini-2.5-flash"

CHAT_LOG_FILE = "chat.log"
HISTORY_FILE = os.path.expanduser("~/.ytree_chat_history")

SAFETY_SETTINGS = [
    {"category": "HARM_CATEGORY_HARASSMENT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_HATE_SPEECH", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_SEXUALLY_EXPLICIT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_DANGEROUS_CONTENT", "threshold": "BLOCK_ONLY_HIGH"},
]

SYSTEM_INSTRUCTION = """
You are a Senior Systems Developer acting as an interactive assistant for the 'ytree' project.

**CONTEXT:**
You have the full, current source code of the project in your context.

**YOUR ROLE: THE CONSULTANT**
1.  **Analyze & Explain:** Discuss the logic, data structures, and flow of the existing code.
2.  **Diagnose:** Help identifying bugs or architectural weaknesses based on the provided context.
3.  **Requirements Gathering:** Help the user define what needs to be done before they attempt to implement it.

**BEHAVIORAL CONSTRAINTS:**
*   **NO UNSOLICITED CODING:** Do **NOT** generate `task.txt` files, JSON plans, or full source code implementations unless the user explicitly prompts you with a phrase like "Write the instructions to..." or "Draft the plan for...".
*   **DISCUSSION MODE:** By default, keep your answers conversational and focused on the *why* and *where* of the code, not the *implementation*.
*   **NO DIFFS:** Do not provide diffs. If asked for code, provide short, illustrative snippets only.

**EXCEPTION - INSTRUCTION MODE:**
If the user asks you to "Write the instructions to [do something]" or "Create a task.txt":
1.  Then, and ONLY then, produce a numbered list of requirements suitable for a `task.txt`.
2.  Focus on *behavior* and *logic requirements* (e.g., "Modify log.c to check for NULL pointers"), not exact lines of C code.
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
                delay *= 2  # Exponential backoff: 10, 20, 40, 80...
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

def get_codebase_context(root_dir="."):
    extensions = {'.c', '.h', '.conf', '.sh', '.py', 'Makefile'}
    ignore_dirs = {'.git', '.venv', '.vscode', 'build', '__pycache__', 'obj', 'docs'}
    ignore_files = {'copy.c.bak', 'ytree.tar', 'chat.log'}

    context_buffer = []
    file_count = 0
    total_chars = 0

    print("Scanning project files...")

    for path in Path(root_dir).rglob("*"):
        if any(part in ignore_dirs for part in path.parts):
            continue

        if path.is_file():
            if path.name in ignore_files or path.name.endswith('~') or path.name.startswith('.'):
                continue

            if path.suffix in extensions or path.name in ['Makefile', 'Changes']:
                try:
                    with open(path, 'r', encoding='utf-8', errors='replace') as f:
                        content = f.read()
                        total_chars += len(content)

                    entry = f"=== {path} ===\n{content}\n"
                    context_buffer.append(entry)
                    file_count += 1
                except Exception as e:
                    print(f"Skipping {path}: {e}")

    est_tokens = total_chars / 4
    print(f"Loaded {file_count} files (~{int(est_tokens)} tokens).")
    return "\n".join(context_buffer)

# -----------------------------------------------------------------------------
# CHAT SESSION
# -----------------------------------------------------------------------------

def start_chat():
    print("\n" + "="*50)
    print(f"YTREE AI ASSISTANT ({MODEL_NAME})")
    print("="*50)
    print(f"Logging to: {CHAT_LOG_FILE}")

    get_input = setup_input()
    codebase_context = get_codebase_context()

    model = genai.GenerativeModel(
        model_name=MODEL_NAME,
        system_instruction=f"{SYSTEM_INSTRUCTION}\n\nCURRENT CODEBASE:\n{codebase_context}"
    )

    chat = model.start_chat(history=[])

    print("-" * 50)
    print("Commands:")
    print("  /reload : Re-scan files and restart.")
    print("  /clear  : Clear conversation history only.")
    print("  /quit   : Exit.")
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

            if user_input.lower() in ['/quit', '/exit']:
                print("Goodbye.")
                break

            if user_input.lower() == '/reload':
                print("\nReloading codebase context...")
                return start_chat()

            if user_input.lower() == '/clear':
                print("\nClearing conversation history (Context preserved)...")
                chat.history.clear()
                with open(CHAT_LOG_FILE, "a", encoding="utf-8") as f:
                    f.write(f"\n--- HISTORY CLEARED ---\n")
                continue

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

if __name__ == "__main__":
    start_chat()