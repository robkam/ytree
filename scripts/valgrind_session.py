#!/usr/bin/env python3
"""Automated interactive Valgrind Memcheck session for ytree QA."""

from __future__ import annotations

import os
import re
import shutil
import signal
import sys
import tempfile
import time
from pathlib import Path

import pexpect


# Key constants (from tests/ytree_keys.py)
DOWN = "\033OB"
ENTER = "\r"
COPY = "c"
QUIT = "q"
CONFIRM_YES = "y"
CTRL_U = "\x15"


def create_sandbox() -> Path:
    """Create temporary sandbox directory with test files."""
    sandbox = Path(tempfile.mkdtemp(prefix="ytree_valgrind_"))

    source = sandbox / "source"
    source.mkdir()
    (source / "root_file.txt").write_text("root content", encoding="utf-8")

    deep = source / "deep" / "nested"
    deep.mkdir(parents=True)
    (deep / "deep_file.txt").write_text("deep content", encoding="utf-8")

    dest = sandbox / "dest"
    dest.mkdir()

    return sandbox


def parse_valgrind_log(log_path: Path) -> dict[str, int | list[str]]:
    """Parse Valgrind log and extract key metrics."""
    if not log_path.exists():
        return {
            "error_summary": -1,
            "definitely_lost": -1,
            "indirectly_lost": -1,
            "still_reachable": -1,
            "possibly_lost": -1,
            "leaked_fds": [],
            "parse_error": True,
        }

    text = log_path.read_text(encoding="utf-8", errors="replace")

    result = {
        "error_summary": None,
        "definitely_lost": None,
        "indirectly_lost": None,
        "still_reachable": None,
        "possibly_lost": None,
        "leaked_fds": [],
        "parse_error": False,
    }

    # ERROR SUMMARY: 0 errors from 0 contexts
    m = re.search(r"ERROR SUMMARY:\s+(\d+)\s+errors?", text)
    if m:
        result["error_summary"] = int(m.group(1))

    # LEAK SUMMARY:
    #    definitely lost: 0 bytes in 0 blocks
    #    indirectly lost: 0 bytes in 0 blocks
    #    possibly lost: 0 bytes in 0 blocks
    #    still reachable: 655,525 bytes in 508 blocks
    m = re.search(r"definitely lost:\s+([\d,]+)\s+bytes", text)
    if m:
        result["definitely_lost"] = int(m.group(1).replace(",", ""))

    m = re.search(r"indirectly lost:\s+([\d,]+)\s+bytes", text)
    if m:
        result["indirectly_lost"] = int(m.group(1).replace(",", ""))

    m = re.search(r"possibly lost:\s+([\d,]+)\s+bytes", text)
    if m:
        result["possibly_lost"] = int(m.group(1).replace(",", ""))

    m = re.search(r"still reachable:\s+([\d,]+)\s+bytes", text)
    if m:
        result["still_reachable"] = int(m.group(1).replace(",", ""))
    else:
        result["still_reachable"] = 0

    # FILE DESCRIPTORS: 4 open (3 std), 1 open at exit.
    # We're looking for open FDs at exit beyond 0/1/2
    fd_matches = re.findall(r"Open file descriptor \d+:.*", text)
    for fd_line in fd_matches:
        # Filter out stdin/stdout/stderr (0/1/2)
        if not re.search(r"<std(in|out|err)>", fd_line):
            result["leaked_fds"].append(fd_line)

    # Check if we got the critical metrics
    if result["error_summary"] is None or result["definitely_lost"] is None:
        result["parse_error"] = True

    return result


def run_valgrind_session() -> int:
    """Drive ytree under Valgrind and return exit code (0=pass, 1=fail)."""
    repo_root = Path(__file__).resolve().parent.parent
    ytree_binary = repo_root / "build" / "ytree"

    if not ytree_binary.exists():
        print("ERROR: ytree binary not found. Run 'make clean && make' first.", file=sys.stderr)
        return 1

    sandbox = None
    valgrind_log = None

    try:
        sandbox = create_sandbox()
        valgrind_log = repo_root / "valgrind.log"

        # Build Valgrind command
        valgrind_cmd = [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--errors-for-leak-kinds=definite,indirect",
            "--track-origins=yes",
            "--track-fds=yes",
            "--error-exitcode=42",
            f"--log-file={valgrind_log}",
            str(ytree_binary),
            str(sandbox),
        ]

        print(f"Spawning ytree under Valgrind in {sandbox}")
        print(f"Valgrind log: {valgrind_log}")
        print("This will take 30-60 seconds due to Valgrind overhead...")

        # Spawn with pexpect (matching tests/ytree_control.py patterns)
        debug_log = repo_root / "valgrind_pexpect_debug.log"
        child = pexpect.spawn(
            valgrind_cmd[0],
            valgrind_cmd[1:],
            cwd=str(sandbox),
            dimensions=(24, 160),
            encoding="utf-8",
            env={"TERM": "xterm", "LC_ALL": "C.UTF-8", "HOME": str(sandbox)},
            timeout=60,
        )
        child.logfile = open(debug_log, "w", encoding="utf-8")

        # Wait for startup (expect Path: or COMMANDS or ANSI escape)
        print("Waiting for ytree startup...")
        idx = child.expect(
            [r"Path:", r"COMMANDS", r"\x1b\[[0-9;?]*[A-Za-z]", pexpect.TIMEOUT],
            timeout=60,
        )
        if idx == 3:
            print("ERROR: Startup timeout. ytree did not start.", file=sys.stderr)
            child.kill(signal.SIGKILL)
            return 1

        print("ytree started. Driving session...")

        # Navigate down a few times (with longer sleeps under Valgrind)
        time.sleep(2.0)
        child.send(DOWN)
        time.sleep(2.0)
        child.send(DOWN)
        time.sleep(2.0)

        # Select first file (send ENTER to expand if directory, or just select)
        child.send(ENTER)
        time.sleep(2.0)

        # Copy flow has two prompts:
        # 1) "COPY" (filename/new name), 2) "To Directory:" (destination).
        child.send(COPY)
        child.expect(r"COPY", timeout=60)
        time.sleep(1.0)
        child.send(ENTER)  # Accept default filename.
        child.expect(r"To Directory:", timeout=60)

        # Input destination path: sandbox/dest
        dest_path = str(sandbox / "dest")
        child.send(CTRL_U)
        time.sleep(1.0)
        child.send(dest_path + ENTER)

        # Ensure copy prompt sequence completed before sending quit keys.
        for _ in range(2):
            idx = child.expect(
                [r"To Directory:", r"COMMANDS", r"\bFILE\b", r"\bDIR\b", pexpect.TIMEOUT],
                timeout=60,
            )
            if idx == 0:
                child.send(CTRL_U)
                time.sleep(1.0)
                child.send(dest_path + ENTER)
                continue
            if idx in (1, 2, 3):
                break
            print("ERROR: copy workflow did not return to main UI.", file=sys.stderr)
            return 1
        else:
            print("ERROR: copy destination prompt did not resolve.", file=sys.stderr)
            return 1

        time.sleep(1.0)

        # Quit cleanly
        print("Quitting ytree...")
        child.send(QUIT)
        time.sleep(3.0)
        child.send(CONFIRM_YES)
        time.sleep(5.0)

        # Wait for process to exit (Valgrind teardown/report flush can be slow)
        forced_termination = False
        try:
            child.expect(pexpect.EOF, timeout=120)
        except pexpect.TIMEOUT:
            print("WARNING: ytree did not exit within 120s, sending SIGTERM...", file=sys.stderr)
            forced_termination = True
            child.kill(signal.SIGTERM)
            time.sleep(5.0)
            if child.isalive():
                print("WARNING: ytree still alive after SIGTERM, sending SIGKILL...", file=sys.stderr)
                child.kill(signal.SIGKILL)
                time.sleep(2.0)

        child.close()
        if hasattr(child, "logfile") and child.logfile:
            child.logfile.close()

        # Give Valgrind extra time to flush log file
        time.sleep(2.0)
        exit_status = child.exitstatus
        signal_status = child.signalstatus

        print(f"ytree exited with status: {exit_status}")
        if signal_status is not None:
            print(f"ytree terminated by signal: {signal_status}")

        # Parse Valgrind log
        print(f"\nParsing Valgrind log: {valgrind_log}")
        metrics = parse_valgrind_log(valgrind_log)

        if metrics["parse_error"]:
            print("ERROR: Failed to parse Valgrind log.", file=sys.stderr)
            return 1

        # Read full log text for error detection
        text = valgrind_log.read_text(encoding="utf-8", errors="replace")

        # Report results
        print("\n=== Valgrind Results ===")
        print(f"ERROR SUMMARY: {metrics['error_summary']} errors")
        print(f"definitely lost: {metrics['definitely_lost']} bytes")
        print(f"indirectly lost: {metrics['indirectly_lost']} bytes")
        print(f"possibly lost: {metrics['possibly_lost']} bytes")
        print(f"still reachable: {metrics['still_reachable']} bytes")

        if metrics["leaked_fds"]:
            print(f"leaked FDs: {len(metrics['leaked_fds'])}")
            for fd in metrics["leaked_fds"]:
                print(f"  {fd}")
        else:
            print("leaked FDs: 0")

        # Determine pass/fail.
        failures = []

        if forced_termination:
            failures.append("interactive session required forced termination (timeout waiting for clean exit)")

        if metrics["error_summary"] != 0:
            failures.append(f"error summary: {metrics['error_summary']} (expected 0)")

        if metrics["definitely_lost"] != 0:
            failures.append(f"definitely lost: {metrics['definitely_lost']} bytes (expected 0)")

        if metrics["indirectly_lost"] != 0:
            failures.append(f"indirectly lost: {metrics['indirectly_lost']} bytes (expected 0)")

        # Warnings (non-fatal)
        warnings = []
        if metrics["still_reachable"] > 0:
            warnings.append(f"still reachable: {metrics['still_reachable']} bytes")

        if metrics["possibly_lost"] > 0:
            warnings.append(f"possibly lost: {metrics['possibly_lost']} bytes")

        if metrics["leaked_fds"]:
            warnings.append(f"leaked FDs: {len(metrics['leaked_fds'])}")

        print("\n=== Summary ===")

        if warnings:
            print("WARNINGS (non-fatal):")
            for w in warnings:
                print(f"  - {w}")

        if failures:
            print("\nFAILURES:")
            for f in failures:
                print(f"  - {f}")
            print("\nFAIL: Valgrind interactive session found critical issues.")
            print(f"Full log: {valgrind_log}")
            return 1

        print("\nPASS: Valgrind interactive session met gate criteria.")
        print(f"Full log: {valgrind_log}")
        return 0

    finally:
        # Cleanup sandbox
        if sandbox and sandbox.exists():
            shutil.rmtree(sandbox, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(run_valgrind_session())
