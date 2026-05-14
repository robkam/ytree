# Relay Runbook (Python-First)

Relay is Python-first and is the only supported runtime path.

## One-time setup

```bash
cd ~/ytree
python3 scripts/relay.py --help
```

## Run one tracked work item

```bash
cd ~/ytree
scripts/relay.py run --bug <id>
# or
scripts/relay.py run --task <id>
```

Behavior:
- Title is derived from `docs/BUGS.md` or `docs/ROADMAP.md`.
- Unknown IDs are hard errors.
- Prompt/report lifecycle is internalized.
- Runtime scratch defaults to `/tmp/ytree-relay` with stale cleanup.

## Monitor relay state

```bash
scripts/relay.py monitor --view quiet
```

Specific run:

```bash
scripts/relay.py monitor --run-id <run_id> --view quiet
```

Detail levels:
- `quiet`: latest state + single `ACTION NEEDED (maintainer)` line
- `normal`: latest + recent events
- `verbose`: full recent event list

One-shot snapshot:

```bash
scripts/relay.py monitor --once --view normal
```

## Health

```bash
scripts/relay.py health
```

## Maintainer action line contract

When maintainer input is required, updates must include exactly one explicit line:

- `ACTION NEEDED (maintainer): reply "<exact short text>"`

When no input is required:

- `ACTION NEEDED (maintainer): none`

Checks/QA run autonomously; maintainer interruption is reserved for blocker clarification or commit-message approval.
