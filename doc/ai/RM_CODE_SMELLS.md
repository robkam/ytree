# RM Code Smells Prompt Guide

This guide shows how to invoke the `rm-code-smells` skill in prompts.

## Filename and Location

- Conventional filename: `RM_CODE_SMELLS.md`
- Canonical location: `doc/ai/`

## Quick Start

```text
use skill rm-code-smells
Audit src/ui for code smells and report prioritized findings only.
```

## Prompt Templates

### 1) Audit only (no code edits)

```text
use skill rm-code-smells
Analyze src/ui/ctrl_file.c for code smells.
Return P0-P2 findings with before/after snippets, but do not implement changes yet.
```

### 2) Code-auditor posture + smell audit

```text
:at c
use skill rm-code-smells
Run a code-smell audit on src/cmd and rank issues by P0-P4.
```

### 3) Implement approved subset only

```text
:at d
use skill rm-code-smells
Implement only approved P0-P1 fixes from the previous report for src/ui/ctrl_dir.c.
Keep scope minimal and rerun relevant validation.
```

### 4) Repo-slice sweep

```text
use skill rm-code-smells
Audit src/ui and include only architecture, ownership/lifetime, and UI-flow economy smells.
```

## Recommended Prompt Pattern

1. Define target scope (`file`, `directory`, or subsystem).
2. State whether this pass is `audit-only` or `audit + implement`.
3. Constrain severity scope when needed (`P0-P2`).
4. Ask for explicit implementation confirmation before edits.

## Notes

- Cross-cutting auto-load can trigger for deslop-style or code-smell remediation requests.
- Explicit `use skill rm-code-smells` is still recommended for deterministic behavior.
