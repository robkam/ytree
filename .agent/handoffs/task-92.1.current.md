# Footer Command Completeness Audit

## Mission
Complete **Audit Remaining Footer Command Completeness** by giving the maintainer a footer-by-footer repair inventory before footer implementation.

## Behavioral contract
Every visible footer command must have matching dispatch and capability availability; a command without an archive-VFS implementation or runtime capability is absent.

## In-scope inventory
- Source tracker: `docs/ROADMAP.md`, Task 92.1 acceptance and Task 92.2 handoff boundary.
- Footer specification and capability projection: `src/ui/display.c` arrays, `ResolveFooterCommandList()`, and footer packing.
- Dispatch contracts: `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/ctrl_file_ops.c`, and preview action filter.
- Capability source: archive capability flags and archive mutation guards.
- Runtime surfaces: filesystem F7/preview; archive directory; archive file including tagged/Global/Showall states; archive F7/preview; writable and read-only archive distinctions.
- Tests: existing archive and F7 PTY coverage; no test change is required for this audit-only item.

## Reconciliation
- Addressed: tracker now records the required Missing/Inapplicable/Repair inventory for every audited surface, including runtime-capability exceptions.
- Addressed: shared root-cause repair is identified: all footer packing callers must consume `ResolveFooterCommandList()`'s returned count.
- Addressed: archive-directory `Invert` is identified as the parity command without an archive capability exception.
- Intentionally unchanged: runtime/footer code and tests; Task 92.1 expressly audits before footer changes. Task 92.2 owns the repair and regression matrix.
- Deferred: Task 92.2 implementation and proof, because it is a separate roadmap item with a materially different runtime-validation path.

## Validation
- Static semantic audit of footer arrays, capability resolver, preview action filter, and dispatch guards completed.
- Local docs-focused validation pending before PR.

## Delivery
- Commit: `8244413f` (will be amended to include this durable relay).
- PR: https://github.com/robkam/ytreenova/pull/549
- Focused validation: `source .venv/bin/activate && pytest -q tests/test_compatibility_shim_guard.py` — 3 passed.
