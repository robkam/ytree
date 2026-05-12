# Prompt Template (Single Quick Dev Unit)

Edit only the work item to match the tracked work item exactly, then copy/paste the full prompt as-is.

You are acting as my implementation agent in ~/ytree.

Work item:
- Source: docs/ROADMAP.md or docs/BUGS.md
- Kind: task or bug
- ID: number
- Name: title

Scope gate (mandatory):
- This prompt is for exactly one quick developer unit (single focused fix/change).
- If the request is broader than a single quick developer unit, STOP and reply exactly:
  "No — this is more than a single quick development task. Use RELAY_PROMPT_TEMPLATE.md for this, and use RELAY_RUNBOOK.md for runtime execution/monitoring."

## Subagent-safe prompt profile (required)

- Keep worker prompts to minimal technical payload only (scope, acceptance, commands, blockers).
- Avoid long policy/governance prose in worker prompts.
- If worker creation is policy-blocked, auto-retry once with a reduced prompt profile and continue without maintainer interruption for that recoverable path.
- Relay proceeds autonomously; maintainer interruption is only for `true_blocker_decision` and `commit_message_approval`.
- Canonical relay autonomy policy tokens (required):
  - `policy_block_retry_once`
  - `watchdog_stall_retry_terminal`
  - `maintainer_pause_gate=true_blocker_decision|commit_message_approval`

Process requirements:
1) Sync local main with GitHub main:
   - git checkout main
   - git fetch origin
   - git pull --ff-only

2) Create and use branch:
   - git checkout -b <type>/<short-task-name>
   - use a suitable type (e.g. fix, feat, chore, docs)
   - never push directly to main; all pushes must go to the branch and PR flow

3) Before implementing:
   - discuss the task with me first
   - confirm exact scope, intended behavior, and acceptance criteria
   - for bug work: first provide a concise, copy/paste manual reproduction script for me to run, then ask me to confirm one of:
     - reproduced (still a bug), or
     - not reproduced (possibly fixed since report)
   - only implement after I explicitly approve

4) Implementation:
   - implement the approved task only
   - keep scope tight and avoid unrelated edits

5) Validation:
   - run required checks/QA autonomously; do not stop for checks approval
   - before first push, run a quick local gate (build + targeted smoke/tests)
   - run targeted tests as needed
   - run `make qa-all` at most once per accepted code state; rerun only after code changes
   - after task completion, run full gate:
     source .venv/bin/activate
     make qa-all
   - DO NOT run full gate before task completion

6) Commits:
   - use Conventional Commits
   - for follow-up corrections to the same intent, prefer amend:
     git commit --amend --no-edit
   - keep branch history workable; final merge will be rebase/ff-friendly

7) Push + draft PR + PR-ready summary:
   - open a draft PR early after first push (it may be red while iterating)
   - when using `gh pr create`, provide PR text via `--body-file` (or heredoc/file), never escaped `\n` in `--body`
   - do not request reviewers until full gate is green
   - convert to Ready only after posting full QA evidence in the PR
   - what changed (by file)
   - test evidence (commands + outcomes)
   - risks/regressions to watch

8) Stay through completion:
   - if checks fail, fix root cause and re-run gates
   - continue until branch is green and PR-ready

Important guardrails:
- Do not rewrite or drop prior merged work from main.
- Do not broaden scope beyond the approved task.
- Status updates must be facts-first: report what was completed (with evidence handle) before stating next action.
- If uncertain, ask before choosing behavior-changing options.
- If anything is ambiguous, ask me to clarify instead of guessing.
- Maintainer interruption is reserved for:
  - `true_blocker_decision`
  - `commit_message_approval`

Operator UX contract (mandatory):
- First line of every update MUST be exactly one of:
  - `ACTION NEEDED (maintainer): none`
  - `ACTION NEEDED (maintainer): reply "<exact text>"`
- If `ACTION NEEDED` is `none`, keep the rest of the update to at most six concise lines.
- If `ACTION NEEDED` is not `none`, print that line before any other content.
- Required section order (no extra sections):
  1) `ACTION NEEDED`
  2) `COMPLETED`
  3) `RUN` (only when relay runtime is involved)
  4) `NEXT`
  5) `REPRO` (only when maintainer-run repro is needed)
  6) `LATEST EVENT` (only when relay runtime is involved)
- Repro instructions must be numbered with one step per line (no single-line paragraph lists).
- Do not emit `Model:` / `Reasoning level:` banners in routine updates.
- Do not ask the maintainer to extract machine/runtime internals from logs; provide exact values directly.
- If relay prompt artifacts are needed, do not ask maintainer for source-path discovery; provide one exact command:
  `scripts/relay-prompts.sh stage --run-id <run_id> --auto;scripts/relay-prompts.sh verify --run-id <run_id>`
- Do not ask maintainer to run `relay-prompts.sh` with `--developer` / `--auditor` source-path arguments.
- If relay run start/resume is reported in the current update, include one exact launch command with concrete values:
  `scripts/relay-run.sh --run-id <actual_run_id> --idempotency-key <actual_idempotency_key> --activity-timeout 900 --retry-limit 2`
- If that run-start/resume command line is missing, immediately send a correction update with only the missing command line.
- Do not stop/pause relay workers for routine process gating; only stop on explicit maintainer stop/cancel or terminal runtime failure recovery.
- On completion, proactively emit final delivery package (summary, verification evidence, commit-ready status, and exact approval text when needed) without waiting for maintainer prodding.
