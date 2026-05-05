You are acting as my implementation agent in ~/ytree.

Task:

Process requirements:
1) Preflight input-quality + routing gate (before any git commands):
   - evaluate my task input for convention and best-practice quality
   - if scope, naming, acceptance criteria, or workflow details are weak/non-standard, warn me and recommend a better version before proceeding
   - classify the request as either:
     a) simple/direct implementation task (continue with this template), or
     b) non-trivial/multi-stage task that should use relay orchestration
   - if (b), stop and instruct me to use `docs/ai/RELAY_PROMPT_TEMPLATE.md` instead of continuing with this template

2) Sync local main with GitHub main:
   - git checkout main
   - git fetch origin
   - git pull --ff-only

3) Create and use branch:
   - git checkout -b codex/mixed-features-fixes <type>/<short-task-name>
   - use a suitable type (e.g. fix, feat, chore, docs)

4) Before implementing:
   - discuss the task with me first
   - confirm exact scope, intended behavior, and acceptance criteria
   - only implement after I explicitly approve

5) Implementation:
   - implement the approved task only
   - keep scope tight and avoid unrelated edits

6) Validation:
   - run targeted tests as needed
   - after task completion, run full gate:
     source .venv/bin/activate
     make qa-all
   - DO NOT run full gate before task completion

7) Commits:
   - use Conventional Commits
   - for follow-up corrections to the same intent, prefer amend:
     git commit --amend --no-edit
   - keep branch history workable; final merge will be rebase/ff-friendly

8) Push + PR-ready summary:
   - what changed (by file)
   - test evidence (commands + outcomes)
   - risks/regressions to watch

9) Stay through completion:
   - if checks fail, fix root cause and re-run gates
   - continue until branch is green and PR-ready

Important guardrails:
- Do not rewrite or drop prior merged work from main.
- Do not broaden scope beyond the approved task.
- If uncertain, ask before choosing behavior-changing options.
- If anything is ambiguous, ask me to clarify instead of guessing.
