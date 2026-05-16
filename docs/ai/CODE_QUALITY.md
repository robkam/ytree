# Code Quality Prompt Guide

Use this guide to invoke `code-quality` with a **prevention-first** posture.

## When this doc is used

This doc is for AI-assisted sessions (architect/developer/code-auditor) when prompts include:

- `use skill code-quality`, or
- code-quality / clean-code remediation requests that trigger cross-cutting auto-load.

It is not runtime code and not read by the compiler; it guides how AI work is requested and executed.

## Prevention-first prompt template

```text
use skill code-quality
Treat this as prevention-first work, not cleanup.
Before code edits, enforce:
- spec-first behavior check
- red/green regression proof for bugfixes
- module ownership check
- root-cause QA remediation (no bypass/suppressions)
- UX economy gate for interactive flows
- focused validation during iteration
Then report/implement only approved P0-P1 issues.
```

## Repo guardrails to rely on

Keep these repo guardrails active in every change:

1. Conventional commit-msg hook policy enforcement.
2. Mandatory focused verification in development; green PR full-QA CI (`make qa-all` equivalent) at pre-merge gate.
3. QA remediation gate: fix root cause, do not patch around failures.
4. Module ownership gate: keep business logic out of controllers when separable.
5. UX economy gate for interactive paths.

## Notes

- Skill name is `code-quality`.
- This filename is intentionally short and discoverable: `CODE_QUALITY.md`.
