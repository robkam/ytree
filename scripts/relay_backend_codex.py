#!/usr/bin/env python3
"""Codex backend adapter isolated from relay core."""

from __future__ import annotations

import subprocess
from dataclasses import dataclass

from relay_backends import BackendResult, EventEmitter, RelayBackend
from relay_core import RunContext


@dataclass
class CodexBackend(RelayBackend):
    command: tuple[str, ...]
    timeout_seconds: int
    name: str = "codex"

    def run(self, context: RunContext, emit: EventEmitter) -> BackendResult:
        emit("backend_started", f"backend={self.name}", {"backend": self.name})
        if not self.command:
            message = "Codex backend not configured; set RELAY_CODEX_COMMAND or relay.toml codex_command"
            emit("action_needed", message, {"reason": "true_blocker_decision"})
            emit("backend_failed", message, {"reason": "missing_command"})
            return BackendResult(status="failed", summary=message, action_needed=message)

        rendered = tuple(
            token.format(
                run_id=context.run_id,
                prompt_path=str(context.prompt_path),
                report_path=str(context.report_path),
                run_dir=str(context.run_dir),
                work_kind=context.work_item.kind,
                work_id=context.work_item.numeric_id,
                work_title=context.work_item.title,
            )
            for token in self.command
        )
        emit("worker_command_started", "running codex command", {"command": list(rendered)})

        try:
            completed = subprocess.run(
                rendered,
                cwd=context.repo_root,
                text=True,
                capture_output=True,
                timeout=self.timeout_seconds,
                check=False,
            )
        except FileNotFoundError:
            message = f"Codex command not found: {rendered[0]}"
            emit("action_needed", message, {"reason": "true_blocker_decision"})
            emit("backend_failed", message, {"reason": "command_not_found"})
            return BackendResult(status="failed", summary=message, action_needed=message)
        except subprocess.TimeoutExpired:
            message = f"Codex command timed out after {self.timeout_seconds}s"
            emit("backend_failed", message, {"reason": "timeout"})
            return BackendResult(status="failed", summary=message, action_needed="none")

        stdout_path = context.run_dir / "codex.stdout.log"
        stderr_path = context.run_dir / "codex.stderr.log"
        stdout_path.write_text(completed.stdout or "", encoding="utf-8")
        stderr_path.write_text(completed.stderr or "", encoding="utf-8")

        if completed.returncode != 0:
            message = f"Codex command failed rc={completed.returncode}; see {stderr_path}"
            emit("backend_failed", message, {"returncode": completed.returncode})
            return BackendResult(status="failed", summary=message, action_needed="none")

        if not context.report_path.exists():
            context.report_path.write_text(
                "\n".join(
                    [
                        f"# Relay Report: {context.work_item.label}",
                        "",
                        "Codex backend completed without explicit report output.",
                        f"stdout: {stdout_path}",
                        f"stderr: {stderr_path}",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

        emit("worker_command_completed", "codex command completed", {"returncode": completed.returncode})
        emit("backend_completed", f"report={context.report_path}", {"report_path": str(context.report_path)})
        return BackendResult(
            status="completed",
            summary=f"Codex backend completed; report={context.report_path}",
            action_needed="none",
        )
