# Relay Runbook (Single-Terminal)

Use this runbook for autonomous architect -> developer -> auditor relay execution in a single terminal.

## Quickstart

```bash
cd ~/ytree
scripts/setup_relay_runtime.sh
```

Then, for each session:

```bash
cd ~/ytree
scripts/relay-workers.sh start
```

## Start a run

```bash
cd ~/ytree
scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2
```

On success, the script prints one explicit line:

- `RUN STARTED: <run_id>` (or `RUN RESUMED: <run_id>`)

If prompt artifacts are still pending, stage them immediately:

```bash
cd ~/ytree
scripts/relay-prompts.sh stage --run-id <run_id> --developer <developer_prompt_source> --auditor <auditor_prompt_source>
scripts/relay-prompts.sh verify --run-id <run_id>
```

## Monitor one run (optional)

```bash
cd ~/ytree
scripts/relay-monitor.sh --run <run_id> --view quiet
```

Detail levels:
- `quiet`: status + `ACTION NEEDED (maintainer)` only
- `normal`: key transitions (heartbeat noise filtered)
- `verbose`: full event stream including heartbeats

Convenience:
- omit `--run` to monitor the latest run in durable relay state.

## Maintainer action line contract

When the architect needs your input, updates must include exactly one explicit line:

- `ACTION NEEDED (maintainer): reply "<exact short text>"`

If no input is required:

- `ACTION NEEDED (maintainer): none`

In relay messages, **maintainer** means you (operator/user).

## Stop workers when done

```bash
cd ~/ytree
scripts/relay-workers.sh stop
```
