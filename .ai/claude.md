# Claude Instructions

Claude-specific behavior for this repository.

## Scope

- Follow all rules in `.ai/shared.md`.
- Use `CLAUDE.md` at repo root only as a discovery stub.

## Build Commands

```bash
make clean && make
make clean && make DEBUG=1
./build/ytree
./build/ytree 2>/tmp/ytree_debug.log
```

## Testing Commands

```bash
source .venv/bin/activate
pytest
pytest -v -s
pytest tests/test_panels.py
pytest -k "test_panel_isolation"
pytest tests/test_panel_isolation.py::test_header_path_clearing -v
```

## Architecture Notes

- Enforce explicit context-passing through `ViewContext`, `YtreePanel`, and `Volume`.
- Avoid mutable global state except approved constants/flags already in the codebase.
- Preserve dual-panel isolation semantics (active panel drives operations).
