#!/usr/bin/env python3
"""Credential-free backend used for local/testing relay runs."""

from __future__ import annotations

import time
from dataclasses import dataclass

from relay_backends import BackendResult, EventEmitter, RelayBackend
from relay_core import RunContext


@dataclass
class FakeBackend(RelayBackend):
    name: str = "fake"
    require_action: bool = False
    action_message: str = 'reply "commit_message_approval"'

    def run(self, context: RunContext, emit: EventEmitter) -> BackendResult:
        emit("backend_started", f"backend={self.name}", {"backend": self.name})
        if self.require_action:
            emit("action_needed", self.action_message, {"reason": "simulated"})
            emit("action_resolved", "simulated action resolved", {"reason": "simulated"})

        report = "\n".join(
            [
                f"# Relay Report: {context.work_item.label}",
                "",
                f"Title: {context.work_item.title}",
                "",
                "Status: Completed (fake backend)",
                f"Generated at: {time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime())} UTC",
            ]
        )
        context.report_path.write_text(report + "\n", encoding="utf-8")
        emit("backend_completed", f"report={context.report_path}", {"report_path": str(context.report_path)})
        return BackendResult(
            status="completed",
            summary=f"Fake backend completed; report={context.report_path}",
            action_needed="none",
        )
