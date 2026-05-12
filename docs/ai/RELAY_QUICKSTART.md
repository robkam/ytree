# Relay Quickstart (Minimal)

## [BASH] One-time (or after relay config/unit changes)

```bash
cd ~/ytree
scripts/setup_relay_runtime.sh
```

## [BASH] Each working session

```bash
cd ~/ytree
scripts/relay-workers.sh start
```

## [IDE] Prompt source (AI chat/IDE)

- Non-trivial work: `docs/ai/RELAY_PROMPT_TEMPLATE.md`
- Single quick unit: `docs/ai/PROMPT_TEMPLATE.md`

In IDE, open a fresh AI thread and paste the task-customized template.
Do not run prompt text in bash.

## [BASH] Start a durable run

```bash
cd ~/ytree
scripts/relay-run.sh --run-id <run_id> --idempotency-key <idempotency_key> --activity-timeout 900 --retry-limit 2
```

Expected output:

- `RUN STARTED: <run_id>` (or `RUN RESUMED: <run_id>`)
- `PROMPT ARTIFACTS READY: <run_id>` **or** `PROMPT ARTIFACTS PENDING: <run_id>`

If prompt artifacts are pending, stage them in bash:

```bash
cd ~/ytree
scripts/relay-prompts.sh stage --run-id <run_id> --auto
scripts/relay-prompts.sh verify --run-id <run_id>
```

## [BASH] Optional monitoring (single terminal)

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

Other views:

```bash
scripts/relay-monitor.sh --run <run_id> --view normal
scripts/relay-monitor.sh --run <run_id> --view verbose
```

- `quiet` = status + `ACTION NEEDED` only
- `normal` = key transitions
- `verbose` = full stream (includes heartbeats)
- on terminal completion/failure, monitor prints report handles and an exact IDE fallback line if IDE goes silent
- `--sound` plays notifications for maintainer input needed, workflow failure, and workflow completion
- if no supported player is available, monitor warns and falls back to terminal bell

Stop monitor with `Ctrl+C`.

## [IDE] What to look for in AI updates

Every update should include exactly one explicit line:

- `ACTION NEEDED (maintainer): none`
- or `ACTION NEEDED (maintainer): reply "<exact text>"`
- checks/QA should run autonomously; maintainer replies should normally be only blocker clarification or commit-message approval.

(“maintainer” means you.)

## [BASH] Finish / shutdown

```bash
cd ~/ytree
scripts/relay-workers.sh stop
```

## Numbered flow

1. [BASH] Run setup once: `scripts/setup_relay_runtime.sh`
2. [BASH] Start workers for the session: `scripts/relay-workers.sh start`
3. [IDE] Paste task prompt in IDE (not bash)
4. [BASH] Run the exact `scripts/relay-run.sh ...` line
5. [BASH] If prompt artifacts are pending, stage prompts: `scripts/relay-prompts.sh stage --run-id <run_id> --auto`
6. [BASH] Verify prompts: `scripts/relay-prompts.sh verify --run-id <run_id>`
7. [BASH] Optional monitor: `scripts/relay-monitor.sh --run <run_id> --view quiet`
8. [BASH] When done: `scripts/relay-workers.sh stop`
