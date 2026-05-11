# Relay Quickstart (Minimal)

## One-time (or after relay config/unit changes)

```bash
cd ~/ytree
scripts/setup_relay_runtime.sh
```

## Each working session

```bash
cd ~/ytree
scripts/relay-workers.sh start
```

## Prompt source (AI chat/IDE)

- Non-trivial work: `docs/ai/RELAY_PROMPT_TEMPLATE.md`
- Single quick unit: `docs/ai/PROMPT_TEMPLATE.md`

In IDE, open a fresh AI thread and paste the task-customized template.
Do not run prompt text in bash.

## Start a durable run

```bash
cd ~/ytree
scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2
```

Expected output:

- `RUN STARTED: <run_id>` (or `RUN RESUMED: <run_id>`)
- `PROMPT ARTIFACTS READY: <run_id>` **or** `PROMPT ARTIFACTS PENDING: <run_id>`

If prompt artifacts are pending, stage them:

```bash
cd ~/ytree
scripts/relay-prompts.sh stage --run-id <run_id> --developer <developer_prompt_source> --auditor <auditor_prompt_source>
scripts/relay-prompts.sh verify --run-id <run_id>
```

## Optional monitoring (single terminal)

```bash
cd ~/ytree
scripts/relay-monitor.sh --run <run_id> --view quiet
```

Other views:

```bash
scripts/relay-monitor.sh --run <run_id> --view normal
scripts/relay-monitor.sh --run <run_id> --view verbose
```

- `quiet` = status + `ACTION NEEDED` only
- `normal` = key transitions
- `verbose` = full stream (includes heartbeats)

Stop monitor with `Ctrl+C`.

## What to look for in AI updates

Every update should include exactly one explicit line:

- `ACTION NEEDED (maintainer): none`
- or `ACTION NEEDED (maintainer): reply "<exact text>"`

(“maintainer” means you.)

## Finish / shutdown

```bash
cd ~/ytree
scripts/relay-workers.sh stop
```

## Numbered flow

1. Run setup once: `scripts/setup_relay_runtime.sh`
2. Start workers for the session: `scripts/relay-workers.sh start`
3. Paste task prompt in IDE (not bash)
4. Run the exact `scripts/relay-run.sh ...` line
5. If pending, run `scripts/relay-prompts.sh stage ...` then `verify`
6. Optional monitor: `scripts/relay-monitor.sh --run <run_id> --view quiet`
7. When done: `scripts/relay-workers.sh stop`
