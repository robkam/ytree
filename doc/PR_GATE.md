# PR Gate

This is the canonical pull-request governance policy for `main`.

## Who Controls What

- `SKILL.md` (`.ai/skills/pr-gate-review/SKILL.md`): AI behavior for PR scrutiny and conflict triage.
- `.github/pull_request_template.md`: optional contributor context for maintainers.
- `.github/workflows/pr-gate.yml`: automated machine triage comment.
- `.github/workflows/pr-conflict-assistant.yml`: automated merge-conflict detection and actionable conflict report.

Use all four together. None of them is sufficient by itself.

## Merge Requirements

Before merging a PR into `main`, require all of the following:

1. Baseline CI is green.
2. PR risk summary comment has been posted and reviewed.
3. Conflict assistant reports no unresolved conflicts.
4. At least one human maintainer approves.

## AI-First Review Contract

AI is expected to audit PRs directly from code + CI + PR metadata without requiring maintainers to read raw diffs:

1. Risk summary from `pr-gate.yml`.
2. Conflict report from `pr-conflict-assistant.yml`.
3. CI outcomes and changed file metadata.
4. Optional PR author context when provided.

## Conflict Handling

If conflicts are detected:

1. AI conflict assistant identifies conflicting files.
2. Contributor or maintainer resolves conflicts on the branch.
3. CI and PR gate rerun.
4. Merge only when conflict report is clean and required checks are green.
