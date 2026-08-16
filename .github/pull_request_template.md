## Summary

## Validation
- `make`
- focused `pytest ...`
- `make qa-unsafe-apis` (required when this PR triggers `.github/workflows/full-qa.yml` or changes safety-sensitive behavior)
- `make qa-fileops-integrity` (required when file/archive mutation flows change)

<!--
Large PRs are difficult to review and carry more regression risk.
For size/L and size/XL PRs, please add once (update only if scope materially changes):
- Why it is not split right now (include links to related PRs, if any)
- What could break
-->
