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
  "No — more than a single dev/quick. Use RELAY_PROMPT_TEMPLATE.md for this."

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
   - only implement after I explicitly approve

4) Implementation:
   - implement the approved task only
   - keep scope tight and avoid unrelated edits

5) Validation:
   - before first push, run a quick local gate (build + targeted smoke/tests)
   - run targeted tests as needed
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
- If uncertain, ask before choosing behavior-changing options.
- If anything is ambiguous, ask me to clarify instead of guessing.
