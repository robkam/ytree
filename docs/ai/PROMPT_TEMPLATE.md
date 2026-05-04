You are acting as my implementation agent in ~/ytree.

Task:

Process requirements:
1) Sync local main with GitHub main:
   - git checkout main
   - git fetch origin
   - git pull --ff-only

2) Create and use branch:
   - git checkout -b codex/mixed-features-fixes <type>/<short-task-name>
   - use a suitable type (e.g. fix, feat, chore, docs)

3) Before implementing:
   - discuss the task with me first
   - confirm exact scope, intended behavior, and acceptance criteria
   - only implement after I explicitly approve

4) Implementation:
   - implement the approved task only
   - keep scope tight and avoid unrelated edits

5) Validation:
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

7) Push + PR-ready summary:
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