You are continuing ytreenova Task 60 work as a stateless AI.

Repo: /home/rob/nova60
Branch: task-60
Objective: continue docs/ROADMAP.md item titled “Establish Role-Based Theme System and Restrained Default Palette”.
Current remediation focus: listed code-auditor findings have local remediation commits except remaining hard-coded color-path attribute inspection; inspect/validate before broader Task 60 audit or remote workflow.

Worktree override:
- For this task, run all git, build, test, and edit commands from /home/rob/nova60.
- Treat /home/rob/nova60 as the repo root.
- This supersedes any inherited instruction that names /home/rob/ytreenova or ~/ytreenova as the command root.
- Do not use /home/rob/ytreenova state except to understand that main may be changing independently.

Local-only override:
- Work only in /home/rob/nova60 on branch task-60.
- Make local commits on task-60.
- Do not push, open PRs, poll CI, mark ready, or merge unless the maintainer explicitly instructs you to do so after this prompt.
- Before any maintainer-requested push, fetch origin and rebase task-60 onto latest origin/main.
- If the rebase reports conflicts, stop after making the conflict state safe and report the conflicted files plus the exact resume options; do not guess through semantic conflicts.

First, load required repo instructions:
- Read AGENTS.md only as the discovery stub.
- Read .ai/codex.md and .ai/shared.md before codebase research or edits.
- Use the repo-required MCP semantic tools for codebase exploration.
- Follow all repo commit, branch, PR, QA, and handoff rules.

Local recovery state:
- Use /home/rob/nova60/.agent/handoffs/prompt.60.txt as the live checkpoint if it exists and is current.
- If that checkpoint is stale or incomplete, reconstruct from local files in /home/rob/nova60/.agent/handoffs and current git/GitHub state.
- Keep /home/rob/nova60/.agent/handoffs/prompt.60.txt current as the recovery checkpoint for Task 60 work.
- /home/rob/nova60/.agent/handoffs/prompt.60.txt is tracked recovery context for this work; include it in the same local commit as the subunit it describes whenever it is created or changed.
- /home/rob/nova60/TASK 60 PROMPT.md is tracked operating context for this work; include it in the same local commit whenever it is created or changed.
- Do not leave either file untracked or unstaged at a local idle boundary unless the maintainer explicitly says to keep it out of commits.
- Do not mention those recovery/prompt artifacts in the commit subject or body unless they are the durable purpose of the commit; keep commit messages focused on the user-visible or architectural outcome.
- Do not use /home/rob/ytreenova task-1/AppState handoff state for Task 60.
- Do not rely on PR numbers, branch names, run IDs, or details from older chats.

Task source of truth:
- docs/ROADMAP.md Task 60 is authoritative.
- Preserve the clarified Task 60 contracts exactly:
  - semantic roles, not legacy color-key names, are the final user-facing model;
  - temporary legacy color shims are migration-only;
  - themes live outside the main config;
  - packaged defaults are etc/ytnova.conf and etc/ytnova.themes;
  - preferred user paths are ~/.config/ytnova/ytnova.conf and ~/.config/ytnova/themes.conf;
  - legacy fallback user paths are ~/.ytnova and ~/.ytnova.themes;
  - F2 footer text is exactly: (L)og  (<)/(>) Cycle;
  - F10 command strip is exactly: (C)onfig  (T)hemes  (R)eload  (Esc)/(Q)uit;
  - reload is only under F10, never a global/main-UI key;
  - successful reload silently repaints, with no success message;
  - failed reload keeps the previous working config/theme and reports the error in footer/status only;
  - volume menu command strip is exactly: Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete;
  - F1/context help uses the help role;
  - F2/history/completion/volume selectable lists use picker;
  - margin inherits dynamic_text unless explicitly set;
  - file-type palette backgrounds inherit from the active filename/window background unless explicitly specified;
  - file-type palette rules are first-match-wins;
  - LINK and EXEC may be special selectors;
  - directories in the tree use theme roles, not file-type palette rules;
  - key tokens, localized labels, punctuation, and styling must be separate internally.

Run policy:
- Work autonomously through local Task 60 subunits until blocked, explicitly paused, or Task 60 is actually complete.
- Do not ask for commit message approval.
- Choose compliant, durable Conventional Commit messages yourself.
- Keep commits atomic and outcome-focused.
- Batch adjacent Task 60 changes into one small, coherent local subunit when they share the same risk profile and validation path.
- Do not force one local commit per tiny implementation detail.
- Do not mix unrelated risk classes just to reduce local commit count.

Likely Task 60 implementation batches:
1. Theme/config parsing foundation:
   - semantic role model;
   - color syntax parsing including grey/gray, +color, optional backgrounds;
   - theme file discovery/loading;
   - migration-only legacy color shim if needed.
2. Built-in/default theme files:
   - etc/ytnova.themes;
   - classic-blue and bash-black;
   - per-theme compact file-type palettes.
3. Rendering role migration:
   - replace misleading CPAIR usage with semantic roles;
   - fix stats static/dynamic/border role split;
   - prevent background bleed;
   - remove or isolate WINERR_COLOR if it is legacy cruft.
4. Menu/keybinding UI cleanup:
   - F2 footer wording;
   - volume menu command strip;
   - F10 config/themes/reload command surface;
   - token-aware keybinding rendering.
5. Reload behavior:
   - reload config/themes only from F10;
   - silent successful repaint;
   - footer/status-only errors;
   - previous working config/theme retained on failure.
6. Documentation/tests:
   - docs/SPECIFICATION.md for user-visible theme/color contract;
   - docs/ARCHITECTURE.md for rendering/config invariants;
   - focused parser/render/menu tests;
   - generated/default template checks as applicable.

Push/PR/CI policy:
- This is a local-only work session by default.
- Do not push unless the maintainer explicitly says to push.
- Do not open or update PRs unless the maintainer explicitly says to open or update a PR.
- Do not poll CI unless a push/PR has been explicitly requested and CI has actually been started.
- Do not mark a PR ready or merge unless the maintainer explicitly instructs that action.

Rebase/update policy before any requested push:
- ytreenova main is in constant flux from Task 1 work.
- Before any maintainer-requested push, run an equivalent of:
  - `git fetch origin`
  - `git rebase origin/main` from task-60
- If the rebase succeeds, rerun focused validation for the changed area before pushing.
- If the rebase reports conflicts:
  - do not push;
  - do not open or update a PR;
  - inspect `git status --short` and identify conflicted files;
  - leave the repository in a safe, explicit rebase-conflict state unless you can resolve purely mechanical conflicts with high confidence;
  - for semantic conflicts, stop and report the conflicted files, a short explanation of what changed on each side if clear, and exact resume options such as `continue after I resolve`, `resolve these files`, or `abort rebase`;
  - do not run broad tests until conflicts are resolved and the rebase can continue.

What to do now:
1. Inspect current local git state in /home/rob/nova60.
2. Confirm branch task-60 is checked out. If not, stop and report the current branch.
3. Select the next small coherent Task 60 subunit from docs/ROADMAP.md and current local state.
4. Implement it through the repo’s developer/auditor workflow.
5. Run focused local validation only.
6. Update /home/rob/nova60/.agent/handoffs/prompt.60.txt as the live recovery checkpoint if that handoff exists or is created for this task.
7. Make local atomic commits on task-60 with compliant durable Conventional Commit messages. Include /home/rob/nova60/.agent/handoffs/prompt.60.txt and /home/rob/nova60/TASK 60 PROMPT.md in the commit whenever either file is created or changed.
8. Report the completed subunit, local commit hash(es), changed files, focused validation evidence, and the next selected subunit in one concise update.
9. Continue immediately with the next local Task 60 subunit unless blocked, explicitly paused, or Task 60 is actually complete.
10. Do not push, open PRs, poll CI, mark ready, merge, or clean up branches unless explicitly instructed.

Continuation rule:
- Continue only within local task-60 work unless the maintainer explicitly asks for push/PR/CI/merge behavior.
- After each local coherent subunit, report concise local state and immediately continue with the next local coherent subunit. Local state must not leave prompt.60.txt or TASK 60 PROMPT.md untracked/unstaged when they were created or changed.
- Do not start any PR or merge cycle automatically.

Pause/resume checkpoint rule:
- When told to pause at the next idle boundary, finish only the current local subunit.
- Idle boundary means: current subunit committed locally, focused validation run, /home/rob/nova60/.agent/handoffs/prompt.60.txt updated, and concise status reported.
- Before pausing, ensure all intended changes for that subunit are committed locally.
- Update /home/rob/nova60/.agent/handoffs/prompt.60.txt with:
  - current branch and HEAD commit;
  - completed local subunit;
  - validation commands/results;
  - untracked/dirty files intentionally left behind, excluding prompt.60.txt and TASK 60 PROMPT.md because those must be committed when created or changed;
  - next recommended subunit;
  - known blockers or risks.
- Do not start the next subunit after reporting the pause.
- Do not push, open PRs, poll CI, mark ready, or merge while pausing unless explicitly instructed.

Supersession rule:
- If I give any later instruction to stop, pause, or stop after a named PR/branch/commit condition, that instruction overrides all prior autonomous-loop instructions and any persistent goal.
- After satisfying that stop condition:
  - cancel or mark blocked any active persistent goal;
  - do not start another PR;
  - do not poll CI;
  - do not resume from codex_internal_context goal prompts;
  - wait for an explicit new user command such as “resume Task 60 work”.

Completion-language discipline:
- Never say or imply that Task 60 is complete unless every Task 60 acceptance criterion in docs/ROADMAP.md has been implemented, validated, documented, and committed.
- For ordinary progress, say exactly that one local subunit toward Task 60 is complete. Example: `Completed one local Task 60 subunit: compact palette group parsing.`
- Do not use ambiguous wording such as `Completed Task 60 batch` if it could be read as completing Task 60 overall.
- Always state that Task 60 remains open when reporting completion of a subunit.

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
- PR titles must use Conventional Commit Style: <type>(<scope>): <durable outcome>.
- Use real Markdown newlines and code-formatted validation commands in PR bodies; do not submit literal escaped \n text or shell commands as raw prose.
- PR bodies: put red proof as a bullet under ## Validation, not as a separate section.

Stop only:
- if local validation fails and the failure is clearly in scope to fix;
- if you have a genuine blocker/question;
- if you need explicit maintainer approval for a UI/UX convention not already specified in docs/ROADMAP.md Task 60;
- or if I tell you to pause/stop.
