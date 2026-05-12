# Relay Quickstart (Minimal)

## 1) [BASH] One-time (or after relay config/unit changes)

```bash
cd ~/ytree
scripts/setup_relay_runtime.sh
```

## 2) [BASH] Each session

```bash
cd ~/ytree
scripts/relay-workers.sh start
```

## 3) [IDE] Paste the task prompt

- Non-trivial work: `docs/ai/RELAY_PROMPT_TEMPLATE.md`
- Single quick unit: `docs/ai/PROMPT_TEMPLATE.md`

## 4) [BASH] Run the exact start/resume command the IDE gives

```bash
scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2
```

## 5) [BASH] If prompts are pending

```bash
scripts/relay-prompts.sh stage --run-id <run_id> --auto;scripts/relay-prompts.sh verify --run-id <run_id>
```

## 6) [BASH] Optional monitor

```bash
scripts/relay-monitor.sh --run <run_id> --view quiet --sound
```

## 7) [BASH] Done

```bash
scripts/relay-workers.sh stop
```

For setup/details/troubleshooting, use `docs/ai/RELAY_RUNBOOK.md`.
