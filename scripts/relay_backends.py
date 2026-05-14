#!/usr/bin/env python3
"""Provider-neutral relay backend contracts."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Mapping, Protocol

if False:  # pragma: no cover
    from relay_core import RunContext

EventEmitter = Callable[[str, str, Mapping[str, Any] | None], dict[str, Any]]


@dataclass(frozen=True)
class BackendResult:
    status: str
    summary: str
    action_needed: str = "none"


class RelayBackend(Protocol):
    name: str

    def run(self, context: "RunContext", emit: EventEmitter) -> BackendResult:
        """Execute one run and emit lifecycle events."""
