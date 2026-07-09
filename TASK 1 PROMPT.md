You are continuing the ytreenova AppState roadmap work as a stateless AI.

Repo: /home/rob/ytreenova
Objective: complete the docs/ROADMAP.md item titled "Unified AppState Transition Machine + Projection Contract".

Startup requirements:
- Read AGENTS.md only as the discovery stub.
- Read .ai/codex.md and .ai/shared.md before any codebase research or edits.
- Use the repo-required MCP semantic tools for codebase exploration.
- Follow all repo commit, branch, PR, QA, and handoff rules.

Recovery and source of truth:
- Use /home/rob/ytreenova/.agent/handoffs/prompt.1.txt as the live checkpoint if current.
- If it is stale or incomplete, reconstruct only from the minimum necessary files in /home/rob/ytreenova/.agent/handoffs.
- Keep `/home/rob/ytreenova/.agent/handoffs/prompt.1.txt` current and include it in the same branch push as the migration slice it describes.
- Use docs/appstate*.json to identify remaining AppState registry/runtime boundary work.
- Use git and GitHub to discover the current branch, active PR, merged PRs, and stale branches.
- Do not rely on branch names, PR numbers, run IDs, or details from older chats.

Primary goal:
- Finish this roadmap item correctly and completely.
- Optimize for full boundary-family migrations with explicit proof of coverage.
- Prevent false completion, silent omission, and one-helper-at-a-time churn.

Batching rules:
- Before starting a branch, identify the next boundary family by shared owner field set, generation domain, transition family, or runtime module cluster.
- Default to one PR per boundary family, not one PR per helper.
- Batch together all adjacent remaining helpers, guards, registry surfaces, dispatch surfaces, transition/projection surfaces, and focused tests that share the same authority boundary and focused validation path.
- Do not stop at the first passing helper cluster if adjacent work in the same family can safely land in the same PR.
- If a proposed batch touches fewer than about 3 adjacent helpers/surfaces, assume it is too small and expand it unless there is a concrete reason not to.
- Split a family only if one of these is true:
  - blocked dependency,
  - materially different validation path,
  - materially different risk class,
  - materially different subsystem owner.
- Do not mix unrelated risk classes just to reduce PR count.

Completeness rules:
- Before editing a boundary family, build an explicit family inventory.
- The inventory must list all relevant items in scope, including where applicable:
  - docs/appstate*.json entries,
  - runtime helpers,
  - guards,
  - registry surfaces,
  - dispatch surfaces,
  - transition machine surfaces,
  - projection/contract surfaces,
  - related runtime modules,
  - focused regression/contract tests,
  - legacy compatibility seams or call paths touching that boundary.
- Record the inventory in the live handoff before or alongside implementation.
- Do not start coding until the inventory is explicit enough to prove scope.

Closure rules:
- Before opening a PR, reconcile the inventory item by item.
- Every inventoried item must be marked as exactly one of:
  - migrated,
  - intentionally unchanged with reason,
  - blocked/deferred with reason.
- Silent omission is forbidden.
- "Done for now" is not an allowed status.
- If an item is left for later, the handoff must name the concrete split reason. If no valid split reason exists, include it in the current PR.

Anti-premature-completion rules:
- Do not treat one passing helper cluster as proof that the family is complete.
- Do not stop after the first green focused validation if adjacent inventoried surfaces in the same family remain.
- Do not treat "I changed the obvious call sites" as proof of completeness.
- The required proof of completeness for a family is the reconciled inventory.

Global audit rules:
- After every merge, re-scan:
  - docs/ROADMAP.md for this roadmap item,
  - docs/appstate*.json,
  - the live handoff,
  - relevant runtime/registry/dispatch/transition/projection code surfaces.
- Use that scan to identify the next highest-value remaining boundary family and catch stale assumptions or missed adjacent work.
- If previously missed work belongs to the same family, prioritize folding it into the next coherent batch.

Task completion rules:
- Do not declare this roadmap item complete until all of the following are true:
  - all relevant docs/appstate*.json entries for this roadmap item are resolved or explicitly deferred with reason,
  - the live handoff lists no unaccounted AppState boundary work for this roadmap item,
  - every identified boundary family has an inventory and closure status,
  - a final audit finds no remaining unmigrated in-scope surfaces,
  - any intentionally deferred items are explicitly named and justified.
- "No obvious next helper" is not completion.
- "Current PR merged" is not completion.
- Completion requires an explicit final sweep and explicit proof that no in-scope boundary families remain unaccounted.

Execution policy:
- Work autonomously.
- Do not ask for commit message approval.
- Choose compliant, durable Conventional Commit messages yourself.
- Keep commits and PRs coherent and outcome-focused.
- Prefer medium-sized or larger coherent boundary-family PRs over micro-PRs.
- Optimize for fewer, fuller PRs while preserving reviewable coherence.

PR and wording rules:
- Do not put roadmap item numbers, migration-slice numbers, PR numbers, branch numbers, or volatile workflow IDs in commit messages, PR titles, or durable change descriptions.
- Commit subjects and PR titles must describe durable outcomes only.
- PR titles must use Conventional Commit style: `<type>(<scope>): <durable outcome>`.
- Keep title style consistent across the AppState series.
- Use real Markdown newlines and code-formatted validation commands in PR bodies.
- Put red proof as a bullet under `## Validation`.
- Omit repository template boilerplate and courtesy preambles from PR bodies.

Validation rules:
- Run focused local validation only.
- Do not present routine test passes as roadmap progress.
- Use validation evidence only where required for PR, handoff, or merge decisions, and keep it concise.
- Do not stream full CI logs or paste long test output.

CI wait rule:
- After CI starts or restarts, wait 15 minutes before the first status check.
- If CI is still running or pending, check every 10 minutes until 50 minutes have elapsed.
- After 50 minutes, check every 5 minutes until CI is green or red.
- If I explicitly report CI is red or green, stop waiting and handle that state immediately.
- When checking CI, use only a compact pass/fail/running summary via `--jq`.
- Fetch detailed check or log output only if a check is red.
- Keep maintainer-facing updates to one short line.
- If a pushed fix changes the PR, restart the wait cycle from the beginning.

Continuation and stop rules:
- After every merge, resume the next largest coherent remaining boundary family without waiting for my go-ahead, unless I later tell you to pause or stop.
- Resume from the latest global audit and handoff, not from the smallest next helper.
- If I later give a stop, pause, or stop-after condition, that overrides all prior autonomous-loop instructions.
- After satisfying a stop condition:
  - cancel or mark blocked any active persistent goal,
  - do not start another PR,
  - do not poll CI,
  - do not resume automatically,
  - wait for an explicit new user command such as "resume AppState work".

What to do now:
1. Inspect current git/GitHub state.
2. If there is an active PR:
   - Follow the CI wait rule.
   - If checks are green, mark ready if needed, merge when allowed, clean up stale remote/local branch state, update .agent/handoffs/prompt.1.txt, then continue according to the continuation rule unless a superseding stop condition applies.
   - If checks failed, inspect only the failure-relevant summary or log tail. Fix only clearly repo-side, in-scope failures.
   - If the failure is external, inconclusive, cancelled, or not clearly repo-side, stop and report the PR URL, check summary, and resume instruction.
3. If there is no active PR:
   - Select the next largest coherent adjacent AppState boundary family from docs/appstate*.json and the current handoff context.
   - Build the explicit family inventory first.
   - Implement the full coherent batch through the repo's developer/auditor/PR workflow.
   - Run focused local validation only.
   - Reconcile the inventory before opening the PR.
   - Push a draft PR.
   - Update .agent/handoffs/prompt.1.txt as the live recovery checkpoint, including the family inventory and closure status.
   - Follow the CI wait rule.
   - When CI is green, mark ready if needed, merge when allowed, clean up stale remote/local branch state, update .agent/handoffs/prompt.1.txt, then continue with the next largest coherent boundary family unless a superseding stop condition applies.

Reporting discipline:
- Do not provide broad recaps unless asked.
- Report only:
  - completed state,
  - current next action,
  - new or changed handles.

Stop only:
- to fix red until green,
- if you have a blocker or needed question,
- if you hit a genuine boundary-family split decision that materially affects batching or risk,
- if the completeness inventory cannot be reconciled without human clarification,
- or if I tell you to pause or stop.
