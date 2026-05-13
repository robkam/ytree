# Relay Quickstart (Minimal)

## 1) [BASH] One-time (or after relay config/unit changes)

```bash
cd ~/ytree
scripts/setup_relay_runtime.sh
```

## 2) [BASH] Each session

```bash
cd ~/ytree
scripts/relay-workers.sh stop;scripts/relay-workers.sh start
```

## 3) [IDE] Paste the task prompt

- Non-trivial work: `docs/ai/RELAY_PROMPT_TEMPLATE.md`
- Single quick unit: `docs/ai/PROMPT_TEMPLATE.md`
- Always copy fresh from repo files for each run (do not reuse older pasted variants).

## 4) [BASH] Run the exact start/resume command the IDE gives

```bash
scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2
```

## 5) [BASH] If run output says `PROMPT ARTIFACTS PENDING`

```bash
scripts/relay-prompts.sh stage --run-id <run_id> --auto;scripts/relay-prompts.sh verify --run-id <run_id>
```

If run output says `INFO: auto prompt sources not emitted yet`, wait for the architect update that emits prompt sources, then run the command.
Do not run `relay-prompts.sh stage --auto` before the architect has acknowledged your repro result for that run.

## 6) [BASH] Optional monitor

```bash
scripts/relay-monitor.sh --run <run_id> --view quiet --sound
```

Or follow the latest run without placeholders:

```bash
scripts/relay-monitor.sh --view quiet --sound
```

For long non-relay bash commands with sound alerts, use `attn <command>` after one-time setup in `docs/ai/RELAY_RUNBOOK.md`.

## 7) [BASH] Done

```bash
scripts/relay-workers.sh stop
```

For setup/details/troubleshooting, use `docs/ai/RELAY_RUNBOOK.md`.
