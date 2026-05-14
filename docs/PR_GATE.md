# PR Gate

This is the canonical pull-request governance policy for `main`.

## Who Controls What

- `SKILL.md` (`.ai/skills/pr-gate-review/SKILL.md`): AI behavior for PR scrutiny and conflict triage.
- `.github/pull_request_template.md`: plain-language author context.
- `.github/workflows/pr-gate.yml`: PR size label assignment.
- `.github/workflows/pr-conflict-assistant.yml`: required branch freshness gate (`Up To Date With Main`).

Use all four together. None of them is sufficient by itself.

## Merge Requirements

Before merging a PR into `main`, require all of the following:

1. Baseline CI is green.
2. `Up To Date With Main` is green.
3. If size is `L` or `XL`, the PR body explains why it is large, why it is not split right now, and what could break. Provide this once, updating only if scope materially changes.
4. At least one human maintainer approves.

## AI-First Review Contract

AI is expected to audit PRs directly from code + CI + PR metadata without requiring maintainers to read raw diffs:

1. Size signal from `pr-gate.yml` (`size/S`, `size/M`, `size/L`, `size/XL`).
2. Freshness result from `pr-conflict-assistant.yml` (`Up To Date With Main`).
3. CI outcomes and changed file metadata.
4. PR author context from the template.

## Branch Freshness Handling

If the freshness gate fails:

1. Contributor updates branch to latest `main`.
2. Contributor pushes updated branch.
3. Gate reruns automatically.
4. Merge only when freshness and required checks are green.
