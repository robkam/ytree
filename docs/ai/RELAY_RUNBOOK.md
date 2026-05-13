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

The run wrapper auto-attempts prompt staging+verify once. If prompt artifacts are still pending, run:

```bash
cd ~/ytree
scripts/relay-prompts.sh stage --run-id <run_id> --auto
scripts/relay-prompts.sh verify --run-id <run_id>
```

If run output says `INFO: auto prompt sources not emitted yet`, wait for the architect update that emits prompt sources, then run the staging command.
Do not run `relay-prompts.sh stage --auto` before the architect has acknowledged your repro result for that run.

## Monitor one run (optional)

```bash
cd ~/ytree
scripts/relay-monitor.sh --run <run_id> --view quiet
```

Latest run shortcut (no placeholder required):

```bash
cd ~/ytree
scripts/relay-monitor.sh --view quiet
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

## Attention notifications (IDE + bash)

IDE/relay attention is covered by monitor sound events (`ACTION NEEDED`, failure, completion):

```bash
scripts/relay-monitor.sh --run <run_id> --view quiet --sound
```

For long bash commands outside relay monitor, add a one-time helper to `~/.bashrc`:

```bash
cat >> ~/.bashrc <<'EOF'
attn(){ "$@"; rc=$?; if [ $rc -eq 0 ]; then ffplay -nodisp -autoexit -loglevel error ~/ytree/scripts/assets/sounds/all.mp3 >/dev/null 2>&1; else ffplay -nodisp -autoexit -loglevel error ~/ytree/scripts/assets/sounds/gen.mp3 >/dev/null 2>&1; fi; return $rc; }
EOF
source ~/.bashrc
```

Usage:

```bash
attn make qa-all
```

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
