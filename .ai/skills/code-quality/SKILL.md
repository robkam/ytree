---
name: code-quality
description: Prevent and remove high-impact code-quality issues with principle-driven, risk-ranked recommendations and optional implementation.
---

# Code Quality

Use this skill when the request is about code quality, clean-code enforcement, deslop-style review, or cleanup/refactor recommendations.

## Objective

- Detect concrete code-quality issues in the requested scope.
- Prioritize fixes by impact, risk, and effort.
- Offer implementation only after presenting findings and obtaining user approval.
- Keep implementations lean and readable without introducing clever/opaque patterns.

## Canonical Blueprint

Read `docs/ai/CODE_QUALITY.md` before producing findings or edits.

That document is the source of truth for:

- smell classes and severity,
- the lean simplicity / anti-obfuscation contract,
- recurring burn-down cadence triggers,
- before/after hotspot evidence,
- and the per-pass acceptance checklist.

This skill adds the execution workflow on top of that policy.

## Workflow

1. Define the target (`file`, `directory`, or repository slice) and keep scope explicit.
2. Read only the minimum required context with semantic tools.
3. Build an inventory before editing and map findings to the smell classes in `docs/ai/CODE_QUALITY.md`.
4. Rank findings by the documented severity rules plus effort/risk modifiers.
5. Output findings with concrete before/after snippets and rationale.
6. Ask whether to implement all, some, or none of the recommended changes unless implementation approval is already explicit.
7. If implementation is approved, execute the smallest coherent batch that closes the selected family and run relevant validation.
8. For burn-down missions, capture before/after hotspot evidence with `scripts/report_code_quality_hotspots.py`.

## ytnova Guardrails

- Do not introduce speculative abstractions before a clear rule-of-three signal.
- Prefer root-cause fixes over cosmetic rewrites.
- Preserve architectural invariants (explicit context passing, panel isolation, deterministic single-thread behavior).
- For tests, favor clarity and deterministic behavior over over-DRY abstraction.
- Keep comments limited to invariants and non-obvious rationale; remove stale comments.
- Do not broaden scope into unrelated refactors unless the user explicitly approves.
