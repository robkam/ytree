#!/usr/bin/env python3
# build-ytree.py
import os
import sys
import json
import argparse
import time
import glob
import re
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

# Architect: Gemini 2.5 Flash is chosen for speed and low latency in high-level planning.
ARCHITECT_MODEL = "models/gemini-2.5-flash"

# Builder: Using Gemini 2.5 Flash avoids the restrictive rate limits of Pro models while
# maintaining sufficient reasoning capability for C code generation.
BUILDER_MODEL = "models/gemini-2.5-flash"

GENERATION_CONFIG = {
    "temperature": 0.1,
    "max_output_tokens": 65535, # Maximum allowed by 2.5 Pro
}

SAFETY_SETTINGS = [
    {"category": "HARM_CATEGORY_HARASSMENT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_HATE_SPEECH", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_SEXUALLY_EXPLICIT", "threshold": "BLOCK_ONLY_HIGH"},
    {"category": "HARM_CATEGORY_DANGEROUS_CONTENT", "threshold": "BLOCK_ONLY_HIGH"},
]

LOG_FILE = "build.log"

# --- ARCHITECT: The Expert Consultant ---
ARCHITECT_PROMPT_HEADER = """
You are the Lead Systems Architect overseeing the modernization of 'ytree'.

**YOUR USER:**
The user is a maintainer who describes issues in terms of **Behavior**. They are NOT a C expert.

**YOUR JOB:**
1.  **Translate:** Convert the user's high-level bug report into a precise low-level C execution plan.
2.  **Library Strategy:**
    *   **Encouraged:** Use established, portable libraries that are standard on Unix-like systems.
    *   **Prohibited:** Do NOT introduce heavy frameworks or OS-specific monitoring APIs unless explicitly requested.
3.  **Sanity Check:** If the user suggests a dangerous approach, explain *why* in the `reasoning` field and suggest a safer alternative.
4.  **Context:** Identify all files involved in the logic chain.

**NOTE ON CONTEXT:**
The file list provided below may be filtered based on the task description. If you believe a file exists in the project but is not listed (e.g., a standard header), you may still include it in your plan.

**CONSTRAINT:**
*   **NO CODE IN JSON:** The JSON output must ONLY contain file paths, action types, and context file paths.
*   **VALID JSON:** Output must be a valid JSON object.

**Output:** A JSON plan containing the files to modify and the reasoning.
"""

# --- BUILDER: The Skilled Coder ---
BUILDER_PROMPT_HEADER = """You are a Senior Systems Developer specializing in C-based command-line utilities.

**TASK:**
Implement the changes requested by the user and the Architect.

**STANDARDS:**
1.  **Logic First:** Fix the behavior described by the user.
2.  **Code Quality:** Use valid C89/C99. Handle pointers safely. Check `errno`.
3.  **Completeness:** You must output the **ENTIRE** file content. Do not strip comments unless they are obsolete.

**--- LIBRARY & PORTABILITY CONSTRAINTS ---**
*   **Standardization Mandate:** Prefer established, portable libraries available in standard Unix environments.
*   **Strict Portability:** Do NOT use OS-specific headers (like `<sys/inotify.h>`) that would break compilation on BSD or macOS. Use standard POSIX calls (`stat`, `gettimeofday`).

**--- CRITICAL OUTPUT CONSTRAINTS ---**
*   **GLOBAL TEXT CONSTRAINT:** **STRICTLY PROHIBITED:** Do not use stacked Unicode characters (combining diacritics/Zalgo).
*   **COMPLETE FILES ONLY:** Provide the **ENTIRE, FUNCTIONAL** C source or header file. **DO NOT** provide diffs or snippets.
*   **STANDALONE & CLEAN:** The output must be the final, clean, current version of the file. **DO NOT** include introductory text or meta-comments.
*   **COMMENT HANDLING:** Preserve all relevant existing comments. Update any comments that become inaccurate.
*   **NO MARKDOWN:** Do not wrap code in markdown blocks. Output raw code only.
"""

# -----------------------------------------------------------------------------
# UTILITIES
# -----------------------------------------------------------------------------

def log_msg(msg):
    """Prints to stdout and writes to the log file."""
    print(msg)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception as e:
        print(f"  [!] Failed to write to log: {e}")

# Tuple of exceptions to retry on.
RETRYABLE_ERRORS = (
    exceptions.ResourceExhausted,
    exceptions.ServiceUnavailable,
    exceptions.InternalServerError,
    exceptions.DeadlineExceeded,
)

def generate_with_retry(model, prompt, safety_settings=None, max_retries=5):
    """
    Wraps model.generate_content with automatic backoff for Quota/Server errors.
    """
    delay = 30

    for attempt in range(max_retries):
        try:
            response = model.generate_content(prompt, safety_settings=safety_settings)
            return response
        except RETRYABLE_ERRORS as e:
            log_msg(f"\n[!] API Error ({type(e).__name__}). Pausing for {delay} seconds before retry {attempt+1}/{max_retries}...")
            time.sleep(delay)
            delay *= 2
        except Exception as e:
            log_msg(f"\n[!] Unexpected error: {e}")
            raise e

    log_msg("\n[!] Max retries reached. Aborting.")
    return None

def get_project_files(root_dir="."):
    """Scans for relevant project files recursively."""
    extensions = {'.c', '.h', '.conf', '.md', '.sh', '.py'}
    relevant_files = []

    # Handle extensions
    for ext in extensions:
        for path in Path(root_dir).rglob(f"*{ext}"):
            if ".venv" not in str(path) and ".git" not in str(path):
                relevant_files.append(str(path))

    # Handle specific filenames (Makefile)
    for path in Path(root_dir).rglob("Makefile"):
        relevant_files.append(str(path))

    return sorted(list(set(relevant_files)))

def filter_context(task_instruction, all_files):
    """
    Smart Context Filter:
    1. Checks for 'Context:' or 'Scope:' lines in the task to forcefully narrow the list.
    2. Otherwise, scans task text for filenames mentioned in the project.
    3. Always includes 'Makefile' if heuristic matching is used.
    """
    task_lines = task_instruction.splitlines()
    manual_scope = []

    # 1. Check for manual explicit scope
    for line in task_lines:
        lower_line = line.strip().lower()
        if lower_line.startswith("context:") or lower_line.startswith("scope:"):
            # Extract filenames mentioned after the colon
            parts = line.split(":", 1)[1].replace(",", " ").split()
            manual_scope.extend([p.strip() for p in parts])

    if manual_scope:
        # Resolve manual entries to full paths
        resolved = set()
        for scope_item in manual_scope:
            found = False
            for real_file in all_files:
                # Loose matching: if user says "main.c", match "src/main.c"
                if scope_item in real_file:
                    resolved.add(real_file)
                    found = True
            if not found:
                log_msg(f"  [i] Warning: Manual scope '{scope_item}' not found in project.")

        if resolved:
            log_msg(f"  [i] Context: Strictly limited to {len(resolved)} files (Manual Override).")
            return sorted(list(resolved))

    # 2. Heuristic Matching
    relevant = set()
    matches_found = False

    # Always check for Makefile if we are doing heuristics
    for f in all_files:
        if os.path.basename(f) == "Makefile":
            relevant.add(f)

    # Scan task for filenames
    for f in all_files:
        fname = os.path.basename(f)
        # Check if filename is mentioned in the task text
        # (Simple substring check, usually sufficient)
        if fname in task_instruction:
            relevant.add(f)
            matches_found = True

    if matches_found:
        log_msg(f"  [i] Context: Heuristic detected {len(relevant)} relevant files.")
        return sorted(list(relevant))

    # 3. Fallback: Return everything if nothing detected
    log_msg(f"  [i] Context: Sending full project structure ({len(all_files)} files).")
    return all_files

def read_file(path):
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            return f.read()
    except Exception as e:
        log_msg(f"Error reading {path}: {e}")
        return ""

def write_file(path, content):
    try:
        # Ensure directory exists
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
        log_msg(f"  [Saved] {path}")
    except Exception as e:
        log_msg(f"  [Error Saving] {path}: {e}")

# -----------------------------------------------------------------------------
# ARCHITECT PHASE
# -----------------------------------------------------------------------------

def run_architect(task_instruction, file_list):
    log_msg("\n--- Phase 1: The Architect ---")
    log_msg(f"Analyzing project structure using {ARCHITECT_MODEL}...")

    # OPTIMIZATION: Filter the file list based on task relevance to save tokens
    filtered_files = filter_context(task_instruction, file_list)
    files_str = "\n".join(filtered_files)

    prompt = f"""{ARCHITECT_PROMPT_HEADER}

    **Relevant Project Files (Subset):**
    {files_str}

    **User Task Instruction:**
    {task_instruction}

    **Task:**
    Based on the user instruction and the file structure, create a precise execution plan.
    Identify which files need to be created, modified, or deleted.
    For every file to be touched, identify which other files provide necessary context (e.g., header files, similar modules).

    **Output Format:**
    You must respond with a valid JSON object.
    Structure:
    {{
        "reasoning": "Explanation of the strategy and any warnings about the user's request.",
        "plan": [
            {{
                "action": "modify" | "create" | "delete",
                "path": "path/to/file",
                "context_files": ["path/to/context1.h", "path/to/context2.c"]
            }}
        ]
    }}
    """

    model = genai.GenerativeModel(
        model_name=ARCHITECT_MODEL,
        generation_config={
            **GENERATION_CONFIG,
            "response_mime_type": "application/json"
        }
    )

    response = generate_with_retry(model, prompt, safety_settings=SAFETY_SETTINGS)
    if not response:
        sys.exit(1)

    try:
        return json.loads(response.text)
    except json.JSONDecodeError as e:
        log_msg(f"Error parsing Architect JSON: {e}")
        log_msg(f"Raw output:\n{response.text}")
        sys.exit(1)

# -----------------------------------------------------------------------------
# BUILDER PHASE
# -----------------------------------------------------------------------------

def send_chat_with_retry(chat, prompt, max_retries=5):
    """Wrapper for chat.send_message with retry logic"""
    delay = 30
    for attempt in range(max_retries):
        try:
            return chat.send_message(prompt, safety_settings=SAFETY_SETTINGS)
        except RETRYABLE_ERRORS as e:
             log_msg(f"\n[!] API Error ({type(e).__name__}). Pausing for {delay}s...")
             time.sleep(delay)
             delay *= 2
        except Exception as e:
             log_msg(f"\n[!] Error: {e}")
             return None
    return None

def run_builder(plan_item, task_instruction):
    action = plan_item['action']
    target_path = plan_item['path']
    context_paths = plan_item.get('context_files', [])

    log_msg(f"\n--- Builder ({BUILDER_MODEL}): {action.upper()} {target_path} ---")

    # Gather Context
    context_content = ""
    if action == "modify":
        content = read_file(target_path)
        if content:
            context_content += f"=== TARGET FILE: {target_path} ===\n{content}\n\n"

    for c_path in context_paths:
        content = read_file(c_path)
        if content:
            context_content += f"=== CONTEXT FILE: {c_path} ===\n{content}\n\n"

    prompt = f"""{BUILDER_PROMPT_HEADER}

    **User Instruction:**
    {task_instruction}

    **Task:**
    Perform the action '{action}' on the file: '{target_path}'.

    **Context Files:**
    {context_content}

    **Output Requirement:**
    Output ONLY the raw code for '{target_path}'.
    Do not use Markdown code blocks.
    Do not write explanations.
    """

    if action == "delete":
        return None

    model = genai.GenerativeModel(
        model_name=BUILDER_MODEL,
        generation_config=GENERATION_CONFIG
    )

    # Use ChatSession to enable continuation
    chat = model.start_chat(history=[])

    # Initial generation
    response = send_chat_with_retry(chat, prompt)
    if not response: return None

    full_content = ""

    # Loop to handle truncation (MAX_TOKENS)
    while True:
        try:
            if not response.candidates:
                log_msg("  [!] Error: No candidates.")
                return None

            part_text = response.text
            full_content += part_text

            finish_reason = response.candidates[0].finish_reason

            # 1 = STOP (Done), 2 = MAX_TOKENS (Truncated)
            if finish_reason == 1:
                break
            elif finish_reason == 2:
                log_msg("  [i] Output limit reached. Requesting continuation...")
                response = send_chat_with_retry(chat, "Continue exactly where you left off. Output ONLY the next part of the code. Do not repeat headers.")
                if not response: return None
            else:
                log_msg(f"  [!] Stopped for reason code: {finish_reason}")
                return None

        except ValueError:
            log_msg("  [!] Error reading response text.")
            return None

    # Clean up Markdown if present
    if full_content.startswith("```"):
        lines = full_content.splitlines()
        if lines and lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].startswith("```"):
            lines = lines[:-1]
        full_content = "\n".join(lines)

    return full_content.strip()

# -----------------------------------------------------------------------------
# MAIN LOOP
# -----------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Auto-Ytree Refactoring Tool")
    parser.add_argument("--task", required=True, help="Path to text file containing the task instructions")
    parser.add_argument("--apply", action="store_true", help="Execute the plan and write changes to disk")
    args = parser.parse_args()

    # Initialize Log
    try:
        with open(LOG_FILE, "w", encoding="utf-8") as f:
            f.write(f"--- Build Log Started: {time.ctime()} ---\n")
    except Exception as e:
        print(f"Warning: Could not create log file: {e}")

    if not os.path.exists(args.task):
        log_msg(f"Error: Task file '{args.task}' not found.")
        sys.exit(1)

    task_instruction = read_file(args.task)
    project_files = get_project_files()
    plan_data = run_architect(task_instruction, project_files)

    log_msg("\n" + "="*40)
    log_msg("ARCHITECT'S PLAN")
    log_msg("="*40)
    log_msg(f"Reasoning: {plan_data.get('reasoning', 'None provided')}\n")

    for i, item in enumerate(plan_data.get("plan", [])):
        log_msg(f"{i+1}. {item['action'].upper()} : {item['path']}")

    if not args.apply:
        log_msg("\nDry run complete. Use --apply to execute this plan.")
        return

    log_msg("\n" + "="*40)
    log_msg("EXECUTING PLAN")
    log_msg("="*40)

    for item in plan_data.get("plan", []):
        path = item['path']
        if item['action'] == "delete":
            if os.path.exists(path):
                os.remove(path)
                log_msg(f"  [Deleted] {path}")
            continue

        new_content = run_builder(item, task_instruction)

        if new_content:
            write_file(path, new_content)
        else:
            log_msg(f"  [!] Failed to generate content for {path}")

if __name__ == "__main__":
    main()