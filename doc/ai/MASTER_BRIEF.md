Canonical in-repo conversion of `/home/rob/ytree/master_brief_minimal.txt`.

# 1. Mission

Convert `ytree` into a repo that is simultaneously:

1. Best-practice and conventional for human developers.
2. Best-practice and conventional for AI-assisted development.
3. Provider-neutral across Codex, Claude, and Gemini.
4. A credible proof that agentic coding can produce high-quality C/ncurses software.

Do this without throwing away existing personas/skills/tests/docs unless a replacement is clearly better and justified.

# 2. Non-Negotiable Agreements (Must Be Preserved)

1. Keep existing accumulated personas, skills, tests, and docs whenever possible.
2. `README.md` in repo root is the human "start here".
3. Linux shell/TUI conventions are the default. Do not optimize around Windows/macOS conventions.
4. XTree/ZTree interaction lineage is the behavioral model for ytree.
5. Always evaluate user requests against best practices.
6. If user asks for a weak/nonstandard approach, explicitly explain the stronger approach before implementing.
7. Prefer mature, popular, well-supported libraries.
8. Do not write custom code for features already solved well by existing libraries.
9. Source code must be self-explanatory where possible; comments are for invariants, rationale, ownership, and non-obvious behavior.

# 3. Required Two-AI Operating Model

There are two temporary AI threads only.

1. AI #1 Planner:
- Has big picture.
- Creates and maintains prompts/packets.
- Runs drift-prevention preflight before every packet handoff.
- Troubleshoots ambiguity and failures.
- Reviews Executor output.
- Reports to user when a step is done and gated.
- Suggests commit message.
- Asks user whether to proceed to next step.
- Can rewrite future packets after ambiguity/mis-solve.
- Gets deleted when overall mission is complete.

2. AI #2 Executor:
- Knows nothing and has no prior history.
- Receives one atomic task packet only.
- Executes task to completion.
- Writes report.
- Gets deleted immediately after that task.
- Must never be told about next steps, commit flow, or mission plan.
- Must never commit/push/branch/merge.

# 4. Packet/Report File Protocol (Exact)

Use root-level handoff files in `/home/rob/ytree`:

1. `packet1.txt`, `packet2.txt`, ...
2. `report1.txt`, `report2.txt`, ...

Rules:

1. Planner writes `packetN.txt`.
2. Executor reads only `packetN.txt` plus repo files needed for that task.
3. Executor writes `reportN.txt` and stops.
4. Planner reviews `reportN.txt`, informs user of outcome, suggests commit message, asks whether to continue.
5. After step is accepted and processed, `packetN.txt` and `reportN.txt` are deleted.
6. Commit history is the long-term archive of accepted work.

## 4.1 Drift-Prevention Protocol (Mandatory, Lightweight)

Goal: Executor must never receive a stale packet.

1. Drift prevention is Planner-owned.
2. Before writing each `packetN.txt`, Planner must refresh context with semantic tools:
- Serena for symbol/file navigation and reference checks.
- jCodeMunch for repo outline, symbol search, references, and dependency checks.
3. Planner must re-validate packet assumptions immediately before handoff (target files, symbols, constraints still match current repo state).
4. If drift is found, Planner rewrites `packetN.txt` before Executor sees it.
5. Executor must not compensate for drift. If packet assumptions do not match observed repo state, Executor stops and reports `STATUS: DRIFT_DETECTED_NEEDS_PLANNER_REPACKET`.

# 5. Git and Control Flow

1. Mission starts with creating branch and remote branch.
2. Executor is forbidden from any commit/push/branch/merge action.
3. Planner suggests commit message only after step completion and gates.
4. User approves commit/push.
5. User controls whether next packet is created.
6. Mission ends with user-approved merge or squash merge.
7. If a step fails or is wrong, Planner can direct restore/rework and issue a rewritten packet.

# 6. Commit Message Rules

1. Use Conventional Commits.
2. Commit messages must describe durable, future-relevant codebase value.
3. Avoid transient/noise wording that is meaningless later.

# 7. Skill Architecture Requirements (C + ncurses)

Do not split source-commenting into separate policy.
Embed source clarity/documentation conventions inside these three skill families.

1. Language safety family:
- C proficiency
- memory management
- pointers and pointer arithmetic
- data structures
- algorithms
- debugging
- standard library usage
- system calls
- UB avoidance
- ownership/lifetime/aliasing contracts in code comments where needed

2. UI/runtime family:
- ncurses redraw invariants
- keybinding collision safety
- PTY behavior and flake handling
- TUI interaction consistency with XTree/ZTree expectations
- non-obvious rendering/state assumptions documented in source comments

3. Delivery gate family:
- strict red-green bugfix workflow
- QA root-cause remediation
- release/PR audit loop
- reportable gate evidence

# 8. Provider-Neutral AI Architecture

1. One shared canonical policy for all providers.
2. Thin provider overlays for Codex, Claude, Gemini.
3. Same personas, skill mapping, QA gates, and acceptance criteria regardless of provider.
4. Provider switching must not alter engineering standards or completion criteria.
5. Repo must remain comfortable for:
- human-only contributors
- human+AI contributors
- Codex-only, Claude-only, Gemini-only periods

# 9. Convention Goal for Public Repo

Make the repo feel conventional and understandable without requiring AI tooling.

1. Human contributor can clone/build/test/contribute without learning personas first.
2. AI workflow is clearly documented but optional.
3. Documentation order is obvious and non-fragmented.
4. Architecture and quality gates are explicit and enforceable.

# 10. What You Must Deliver Now (Architect Output)

Produce all of the following:

1. Current-state assessment of ytree against this brief.
2. Gap list with priority and risk.
3. Target-state repo/instruction structure.
4. Incremental migration plan (no big rewrite).
5. Governance rules for Planner/Executor lifecycle.
6. Skill coverage matrix showing existing vs missing C/ncurses skills.
7. Draft `packet1.txt` for Executor with exactly one atomic task.
8. `packet1.txt` must include:
- objective
- allowed files
- forbidden actions
- required gates/tests
- output expectations for `report1.txt`
- explicit stop condition

# 11. Hard Constraints for Your Response

1. Be concrete and file-level.
2. Do not hand-wave.
3. Do not propose deleting existing system wholesale.
4. Preserve existing strengths.
5. Include everything in this brief; omit nothing.
