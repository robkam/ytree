# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## ytree Development Context

ytree is a file manager for UNIX-like systems, optimized for speed and keyboard efficiency. This is a v3.0 modernization of the original ytree, using C99 standards and a strict context-passing architecture.

## Build Commands

```bash
# Clean build (required after header changes)
make clean && make

# Debug build with AddressSanitizer
make clean && make DEBUG=1

# Run the application
./build/ytree

# Run with debug logging
./build/ytree 2>/tmp/ytree_debug.log
```

## Testing Commands

```bash
# Activate Python virtual environment (required for tests)
source .venv/bin/activate

# Run all tests
pytest

# Run tests with verbose output
pytest -v -s

# Run a specific test file
pytest tests/test_panels.py

# Run a specific test by name pattern
pytest -k "test_panel_isolation"

# Run a specific test function
pytest tests/test_panel_isolation.py::test_header_path_clearing -v

# Check for memory leaks
valgrind --leak-check=full ./build/ytree
```

## High-Level Architecture

### Context-Passing Design

The codebase follows a **strict context-passing architecture**. All functions receive explicit context pointers as their first parameter:

```c
// Every function signature begins with a context pointer
void UpdateDisplay(ViewContext *ctx, ...);
void RefreshPanel(YtreePanel *panel, ...);
void LoadVolume(Volume *volume, ...);
```

**Key Rules:**
- NO global mutable state (only 3 permitted exceptions: `ui_colors[]`, `NUM_UI_COLORS`, `ytree_shutdown_flag`)
- All state flows through `ViewContext` → `YtreePanel` → `Volume` hierarchy
- Functions cannot reach "sideways" through globals to find siblings

### State Hierarchy

```
ViewContext (The Session Root)
├── left   → YtreePanel (left panel view state)
├── right  → YtreePanel (right panel view state)
├── active → pointer to current panel (left or right)
├── volumes_head → Volume linked list (filesystem data)
└── viewer, layout, mode flags, etc.
```

### Dual-Panel Isolation (F8 Split-Screen)

The split-screen mode maintains **complete panel independence**:
- Active panel owns keyboard focus and initiates operations
- Inactive panel is DORMANT - no updates or state changes
- Tab key switches focus, preserving exact state
- Copy/Move operations are directional: Active → Inactive

### Module Organization

```
src/
├── core/       # Main loop, signal handling, initialization
├── ui/         # Display rendering, cursor movement, screen layout
├── fs/         # Filesystem operations, directory tree management
├── cmd/        # Command execution (copy, move, delete, etc.)
├── archive/    # Archive handling via libarchive
└── util/       # String utilities, memory management
```

### Key Architectural Invariants

1. **Single-threaded execution** - No threads, deterministic state transitions
2. **MVC separation** - UI (View), filesystem (Model), commands (Controller)
3. **Panel independence** - Left/right panels cannot interfere with each other
4. **Context isolation** - All state access goes through explicit pointers

## Finding Information & Global Rules

This project maintains comprehensive documentation and follows established global directives. **Do not guess or assume** - read the appropriate document.

### Global AI Development Rules

**READ FIRST (Global):** [~/.gemini/GEMINI.md](~/.gemini/GEMINI.md) contains high-level directives that apply to ALL AI development work:
- Your role as Senior AI Development Partner under the Lead Architect.
- Environmental context (WSL2, POSIX paths, toolchain access).
- Persona orchestration (@consultant, @builder, @auditor, @tester).
- Interaction protocol (design approval workflow).

**READ NEXT (Repository):** [GEMINI.md](~/ytree/GEMINI.md) contains project-level directives and strict pacing rules for this repository.

### Required Reading & Modernization Context

1. **Check [doc/ROADMAP.md](doc/ROADMAP.md) modernization steps** before proposing architectural changes - many refactors are moving targets.
2. **Read [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md)** for system design and invariants.
3. **Read [doc/SPECIFICATION.md](doc/SPECIFICATION.md)** for behavioral requirements.
4. **Read [doc/ai/WORKFLOW.md](doc/ai/WORKFLOW.md)** for the development protocol.
5. **Progressive Disclosure Principle**: Only read the documents you need for the current task. Don't try to load all context upfront.

### The Anti-Patching Directive

**DO NOT apply superficial fixes for deep architectural problems.** If a bug is caused by fragmented state, inconsistent globals, or panel collision, **STOP** and refactor to unify logic first.


### Repository Rule Files

- **[GEMINI.md](~/ytree/GEMINI.md)** - Project-level Master Directives and Strict Pacing.
- **[consultant.md](~/ytree/.agent/rules/consultant.md)** - Planning and analysis (behavior-to-C translation, no coding).
- **[builder.md](~/ytree/.agent/rules/builder.md)** - Implementation (C89/C99 standards, complete files only).
- **[auditor.md](~/ytree/.agent/rules/auditor.md)** - Architecture review (identifying global state coupling).
- **[tester.md](~/ytree/.agent/rules/tester.md)** - Test generation (Python/pexpect TUI automation).

## Leveraging MCP Servers: jCodemunch, Serena, & GitHub

This project utilizes Model Context Protocol (MCP) servers for semantic code understanding and repository management.

### Strategic Use of jCodemunch (Symbol Navigation)
jCodemunch provides indexed symbol navigation across the entire codebase. Use it for finding definitions, signatures, and file outlines.

```bash
# Search for functions/symbols by name
mcp__jcodemunch__search_symbols(repo: "ytree", query: "RefreshView")

# Get file outline (all functions/classes in a file)
mcp__jcodemunch__get_file_outline(repo: "ytree", file_path: "src/ui/ctrl_dir.c")
```

### Strategic Use of Serena (Symbol Analysis & Editing)
Serena provides LSP-powered symbol intelligence. Use it for tracing references, call chains, and architectural editing (e.g., atomic function replacement).

```bash
# Get file structure overview
mcp__serena__get_symbols_overview(relative_path: "src/ui/ctrl_dir.c")

# Find symbol with implementation
mcp__serena__find_symbol(name_path_pattern: "UpdateDisplay", include_body: true)

# Find all references to a symbol
mcp__serena__find_referencing_symbols(name_path: "ViewContext", relative_path: "src/ui/screen.c")

# Replace entire function atomically
mcp__serena__replace_symbol_body(name_path: "UpdateFooter", relative_path: "src/ui/footer.c", body: "...")
```

### GitHub MCP Server Integration
Used for seamless repository management, issue tracking, and PR creation.

```bash
# Create and manage issues
mcp__github-mcp-server__create_issue(owner: "robkam", repo: "ytree", title: "Bug: ...", body: "...")

# Create pull requests
mcp__github-mcp-server__create_pull_request(owner: "robkam", repo: "ytree", title: "fix: ...", base: "main", head: "branch")
```

### Workflow Integration Patterns

- **Exploration**: Start with `get_symbols_overview` for file structure, then `search_symbols` to locate logic.
- **Refactoring**: Use `find_referencing_symbols` to find all call sites before using `replace_symbol_body` for atomic changes.
- **Root Cause Analysis**: Use `search_for_pattern` for state access patterns and trace data flow via references.
- **Memory System**: Use Serena's memory system (`write_memory` / `read_memory`) to preserve architectural insights across sessions.

## Common Development Tasks

### Adding a New Command

1. Create handler in `src/cmd/` following existing patterns
2. Register in command dispatch table
3. Update help text in `src/ui/help.c`
4. Add tests using pexpect framework

### Debugging UI Issues

1. Enable debug logging: `./build/ytree 2>/tmp/ytree_debug.log`
2. Add debug prints: `fprintf(stderr, "DEBUG: panel=%p cursor=%d\n", panel, panel->cursor);`
3. Use `tests/debug_screen.py` for screen capture analysis
4. Check panel isolation with `test_panel_isolation.py`

### Memory Debugging

```bash
# Build with AddressSanitizer
make clean && make DEBUG=1

# Run with valgrind for leak detection
valgrind --leak-check=full --show-leak-kinds=all ./build/ytree

# Check specific allocation patterns
valgrind --track-origins=yes ./build/ytree
```

## Test Organization

Tests use Python's `pexpect` library to automate the TUI:

- `tests/conftest.py` - Pytest fixtures and test helpers
- `tests/ytree_control.py` - Main controller class for TUI automation
- `test_panel_isolation.py` - Dual-panel independence tests
- `test_stats_panel.py` - Statistics window behavior
- `test_core.py` - Basic functionality tests

When in doubt about architecture or behavior, consult the documentation in `doc/` rather than making assumptions.
