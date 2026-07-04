You are continuing ytreenova Task 60 work as a stateless AI.

Repo: /home/rob/nova60
Branch: task-60
Local-only rule: work only in /home/rob/nova60 on task-60; make local commits only; do not push, open/update PRs, poll CI, mark ready, or merge unless explicitly instructed by the maintainer.

Required startup:
- Read AGENTS.md only as the discovery stub.
- Read .ai/codex.md and .ai/shared.md before codebase research or edits.
- Use the repo-required MCP semantic tools (`serena` and `jcodemunch`) for codebase exploration.
- Treat docs/ROADMAP.md Task 60 as authoritative.

Live checkpoint:
- Use /home/rob/nova60/.agent/handoffs/prompt.60.txt as the current recovery checkpoint.
- Keep /home/rob/nova60/.agent/handoffs/prompt.60.txt current whenever additional local Task 60 work is done.
- Include both /home/rob/nova60/.agent/handoffs/prompt.60.txt and /home/rob/nova60/TASK 60 PROMPT.md in the same local commit whenever either file is changed.

Current state:
- The latest code-auditor findings named in the recovery checkpoint are closed locally:
  - unreadable preferred theme catalogs fail closed;
  - omitted `margin` inherits `dynamic_text`;
  - Task 60 render/setup surfaces use internal semantic `UI_ROLE_*` aliases instead of legacy `CPAIR_*` names;
  - the recovery checkpoint is refreshed.
- Task 60 remains open until every acceptance criterion in docs/ROADMAP.md has been implemented, validated, documented, and committed.

If asked to continue:
1. Inspect `git status --short --branch` and confirm branch `task-60`.
2. Read `.agent/handoffs/prompt.60.txt` and current git state.
3. Select the next small Task 60 subunit only after a fresh focused audit of docs/ROADMAP.md Task 60 acceptance criteria.
4. Run focused validation only unless the maintainer explicitly asks for broader QA.
5. Commit atomically with durable Conventional Commit messages and update the recovery checkpoint.

Before any maintainer-requested push:
- `git fetch origin`
- `git rebase origin/main`
- If conflicts occur, stop in a safe explicit conflict state and report conflicted files plus exact resume options.
