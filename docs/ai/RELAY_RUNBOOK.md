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
scripts/relay-prompts.sh stage --run-id <run_id> --auto
scripts/relay-prompts.sh verify --run-id <run_id>
```

## Monitor one run (optional)

```bash
cd ~/ytree
scripts/relay-monitor.sh --run <run_id> --view quiet
```

Sound notifications (optional):

```bash
sudo apt-get update
sudo apt-get install -y ffmpeg
scripts/relay-monitor.sh --run <run_id> --view quiet --sound
```

Detail levels:
- `quiet`: status + `ACTION NEEDED (maintainer)` only
- `normal`: key transitions (heartbeat noise filtered)
- `verbose`: full event stream including heartbeats
- on terminal completion/failure, monitor includes report handles plus an exact IDE fallback line if IDE goes silent
- `--sound` plays notifications for maintainer input needed, workflow failure, and workflow completion
- if no supported player is available, monitor warns and falls back to terminal bell

Convenience:
- omit `--run` to monitor the latest run in durable relay state.

## Maintainer action line contract

When the architect needs your input, updates must include exactly one explicit line:

- `ACTION NEEDED (maintainer): reply "<exact short text>"`

If no input is required:

- `ACTION NEEDED (maintainer): none`

In relay messages, **maintainer** means you (operator/user).
Checks/QA run autonomously; maintainer interruption is for blocker clarification or commit-message approval.

## Stop workers when done

```bash
cd ~/ytree
scripts/relay-workers.sh stop
```
