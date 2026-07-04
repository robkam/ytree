You are continuing the ytreenova AppState roadmap work as a stateless AI.

Repo: /home/rob/ytreenova
Objective: continue the docs/ROADMAP.md item titled “Unified AppState Transition Machine + Projection Contract”.

First, load required repo instructions:
- Read AGENTS.md only as the discovery stub.
- Read .ai/codex.md and .ai/shared.md before codebase research or edits.
- Use the repo-required MCP semantic tools for codebase exploration.
- Follow all repo commit, branch, PR, QA, and handoff rules.

Local recovery state:
- Use /home/rob/ytreenova/.agent/handoffs/prompt.1.txt as the live checkpoint if it is current.
- If that checkpoint is stale or incomplete, reconstruct from local files in /home/rob/ytreenova/.agent/handoffs.
- The handoff files are tracked recovery context; keep `/home/rob/ytreenova/.agent/handoffs/prompt.1.txt` current and include it in the same branch push as the subunit it describes.
- Use etc/appstate/appstate*.json to identify remaining AppState registry/runtime boundary work.
- Use git and GitHub to discover the current branch, current PR, merged PRs, and stale branches.
- Do not rely on PR numbers, branch names, run IDs, or details from older chats.

Run policy:
- Work autonomously.
- Do not ask for commit message approval.
- Choose compliant, durable Conventional Commit messages yourself.
- Keep commits atomic and outcome-focused.
- Batch adjacent AppState units into one small, coherent PR when they share the same risk profile and validation path.
- Do not force one PR per tiny subunit.
- Do not mix unrelated risk classes just to reduce PR count.

CI wait rule:
- After CI starts or restarts, wait 15 minutes before the first status check.
- If CI is still running or pending, check every 10 minutes until 50 minutes have elapsed.
- After 50 minutes, check every 5 minutes until CI is green or red.
- If you explicitly report that CI is red or green, stop waiting and handle that state immediately.
- When checking CI, use only a compact pass/fail/running summary via `--jq`.
- Fetch detailed check or log output only if a check is red.
- Keep maintainer-facing updates to one short line.
- If a pushed fix changes the PR, restart the wait cycle from the beginning.

Continuation rule after merge:
- After every merge, resume the next task 1 subunit without waiting for my go-ahead, unless a later supersession/stop/pause instruction says otherwise.

Supersession rule:
- If you give any later instruction to stop, pause, or stop after a named PR/branch/commit condition, that instruction overrides all prior autonomous-loop instructions and any persistent goal.
- After satisfying that stop condition:
  - cancel or mark blocked any active persistent goal;
  - do not start another PR;
  - do not poll CI;
  - do not resume from codex_internal_context goal prompts;
  - wait for an explicit new user command such as “resume AppState work”.

What to do now:
1. Inspect current git/GitHub state.
2. If there is an active PR:
   - Follow the CI wait rule.
   - If checks are green, mark ready if needed, merge when allowed, clean up stale remote/local branch state, update .agent/handoffs/prompt.1.txt, then continue according to the autonomous continuation policy unless a superseding stop condition applies.
   - If checks failed, inspect only the failure-relevant log tail/summary. Fix only clearly repo-side, in-scope failures. If the failure is external, inconclusive, cancelled, or not clearly repo-side, stop and report the PR URL, check summary, and resume instruction.
3. If there is no active PR:
   - Select the next small coherent AppState batch from etc/appstate/appstate*.json and the current handoff context.
   - Implement it through the repo’s developer/auditor/PR workflow.
   - Run focused local validation only.
   - Push a draft PR.
   - Update .agent/handoffs/prompt.1.txt as the live recovery checkpoint.
   - Follow the CI wait rule.
   - When CI is green, mark ready if needed, merge when allowed, clean up stale remote/local branch state, update .agent/handoffs/prompt.1.txt, then continue with the next small coherent AppState batch unless a superseding stop condition applies.

Context discipline:
- Do not stream full CI logs.
- Do not paste long test output.
- Do not read all historical handoffs unless reconstruction is impossible without them.
- Do not provide broad recaps unless asked.
- Report only completed state, current next action, and new/changed handles.

Wording rules:
- Do not put roadmap item numbers, subunit numbers, PR numbers, branch numbers, or volatile workflow IDs in commit messages, PR titles, or durable change descriptions.
- Commit subjects and PR titles must describe durable outcomes only.
- Do not present routine required test passes as roadmap progress. Use validation evidence only where required for PR/handoff/merge decisions, and keep it concise.
- PR titles must use the same Conventional Commit Style as commit subjects: `<type>(<scope>): <durable outcome>`.
- Keep PR title style consistent across the AppState series; do not mix sentence-case descriptive titles with Conventional Commit Style titles.
- Use real Markdown newlines and code-formatted validation commands in PR bodies; do not submit literal escaped `\n` text or shell commands as raw prose.
- PR bodies: put red proof as a bullet under `## Validation`, not as a separate section.

What you must do from now until completion of task 1, unless I tell you to pause:

For any active/open PR after CI starts or restarts, follow the CI wait rule from the main prompt.

At each allowed CI status check:
- If CI is green, mark the PR ready if needed, merge when branch protection and review requirements allow, clean up branch state, return to main, then proceed according to the final resume rule.
- If CI is red, inspect only the failure-relevant summary/tail and fix clear repo-side failures. Each pushed fix restarts the CI wait rule.
- If CI is still running or pending, continue following the CI wait rule.

If I explicitly report CI is red or green, stop waiting and handle that state immediately.

Stop only:
- to fix red until green;
- if you have a question/blocker; or
- if I tell you the PR is red/green or tell you to pause/stop.

After every merge, resume the next task 1 subunit without waiting for my go-ahead, unless a later supersession/stop/pause instruction says otherwise.
