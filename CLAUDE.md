# ytree Development Context

## Finding Information

This project maintains comprehensive technical documentation. **Do not guess or assume** - read the appropriate document when you need context.

### CRITICAL: Check ROADMAP.md Before Architectural Changes

**BEFORE proposing any architectural changes, refactors, or new subsystems:**

1. **READ [doc/ROADMAP.md](doc/ROADMAP.md) Steps 9.x** to verify the architecture hasn't already been implemented
2. **SEARCH for keywords** related to your proposed change (e.g., "dialog", "prompt", "input", "window stack")
3. **VERIFY completion status** - Many architectural refactors are marked `[x] Status: Complete`

**Example**: Step 9.4 (Tiered Dialog Manager & Window Stack) is **already complete**. Do not propose rebuilding the dialog system.

### Global AI Development Rules

**READ FIRST:** [~/.antigravityrules](~/.antigravityrules) contains high-level directives that apply to ALL AI development work:
- Your role as Senior AI Development Partner under the Lead Architect
- Environmental context (WSL2, POSIX paths, toolchain access)
- Persona orchestration (@consultant, @builder, @auditor, @tester)
- Interaction protocol (design approval workflow, **check ROADMAP.md first**)

### Architecture & Technical Specifications

- **[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md)** - System design, core invariants, dual-panel isolation rules, and rendering standards
- **[doc/SPECIFICATION.md](doc/SPECIFICATION.md)** - Behavioral requirements, UI contracts, navigation protocols, and the "XTree&trade;" state machine
- **[doc/ROADMAP.md](doc/ROADMAP.md)** - Feature implementation history, current status, and modernization phases

### Development Workflow & Standards

- **[doc/ai/WORKFLOW.md](doc/ai/WORKFLOW.md)** - The Spec-First development protocol and "Golden Loop" methodology
- **[doc/ai/TESTING.md](doc/ai/TESTING.md)** - Test organization standards, naming conventions, and TDD protocol
- **[doc/ai/DEBUGGING.md](doc/ai/DEBUGGING.md)** - External Expert Architecture Analysis methodology for complex bugs
- **[.agent/rules/*.md](.agent/rules/)** - Role-specific persona instructions for different task types

### Available Development Personas

The `.agent/rules/` directory contains specialized persona definitions. Read the appropriate file when taking on a specific role:

- **[consultant.md](.agent/rules/consultant.md)** - Planning and analysis (behavior-to-C translation, no coding)
- **[builder.md](.agent/rules/builder.md)** - Implementation (C89/C99 standards, complete files only, no snippets)
- **[auditor.md](.agent/rules/auditor.md)** - Architecture review (identify global state coupling and fragility)
- **[tester.md](.agent/rules/tester.md)** - Test generation (Python/pexpect TUI automation)
- **[mentor.md](.agent/rules/mentor.md)** - General technical guidance

---

## Leveraging MCP Servers: jCodemunch & Serena

This project is configured with two powerful Model Context Protocol (MCP) servers that provide **semantic code understanding** capabilities far superior to basic file operations.

### When to Use Semantic Tools vs. Basic File Tools

**PREFER Serena/jCodemunch for:**
- Understanding code structure and relationships
- Finding function/class definitions across the codebase
- Tracing references and call chains
- Identifying where symbols are used
- Architecture exploration and refactoring analysis
- Large-scale code navigation

**AVOID basic file tools (Read, Grep, Glob) when:**
- You need to understand "where is X defined?"
- You want to find "what calls function Y?"
- You're exploring unfamiliar code areas
- You need architectural understanding

### Strategic Use of jCodemunch

jCodemunch provides **indexed symbol navigation** across the entire codebase:

```bash
# First-time setup: Index the ytree codebase
mcp__jcodemunch__index_folder(path: "/home/rob/ytree")

# Search for symbols by name
mcp__jcodemunch__search_symbols(repo: "ytree", query: "RefreshView")

# Get file structure overview
mcp__jcodemunch__get_file_tree(repo: "ytree", path_prefix: "src/")

# Get outline of specific file (all functions/classes)
mcp__jcodemunch__get_file_outline(repo: "ytree", file_path: "src/ui/ctrl_dir.c")

# Retrieve symbol implementation
mcp__jcodemunch__get_symbol(repo: "ytree", symbol_id: "<id-from-search>")
```

**Key Benefits:**
- **Fast symbol lookup** - No need to grep through hundreds of files
- **Cross-file navigation** - Instantly find definitions regardless of file location
- **Signature awareness** - See function signatures without reading full implementations
- **AI-generated summaries** - Get natural language explanations of symbols

### Strategic Use of Serena

Serena provides **semantic code analysis** with LSP-powered symbol intelligence:

```bash
# Get high-level overview of a file's structure
mcp__serena__get_symbols_overview(relative_path: "src/ui/ctrl_dir.c", depth: 1)

# Find symbol by name pattern (supports wildcards)
mcp__serena__find_symbol(name_path_pattern: "RefreshView", include_body: true)

# Find all references to a symbol
mcp__serena__find_referencing_symbols(
    name_path: "RefreshView",
    relative_path: "src/ui/screen.c"
)

# Search for patterns in code (regex-aware)
mcp__serena__search_for_pattern(
    substring_pattern: "ctx->.*panel",
    restrict_search_to_code_files: true
)

# Symbol-level editing (preferred over text replacement)
mcp__serena__replace_symbol_body(
    name_path: "UpdateFooter",
    relative_path: "src/ui/footer.c",
    body: "<new-implementation>"
)
```

**Key Benefits:**
- **LSP integration** - Uses language server for accurate symbol resolution
- **Architectural editing** - Replace entire functions atomically
- **Reference tracking** - Find all usages before refactoring
- **Symbol-aware search** - Filter by symbol kind (function, class, etc.)

### Workflow Integration

**Code Exploration (Initial Understanding):**
1. Use `mcp__jcodemunch__get_repo_outline()` to understand project structure
2. Use `mcp__serena__get_symbols_overview()` for specific file overviews
3. Use `mcp__jcodemunch__search_symbols()` to locate definitions
4. Use `mcp__serena__find_symbol()` with `include_body=true` only when you need implementation details

**Refactoring (Architectural Changes):**
1. Use `mcp__serena__find_symbol()` to locate target functions
2. Use `mcp__serena__find_referencing_symbols()` to find all call sites
3. Use `mcp__serena__replace_symbol_body()` for atomic function replacement
4. Use `mcp__serena__rename_symbol()` for codebase-wide identifier changes

**Debugging (Root Cause Analysis):**
1. Use `mcp__jcodemunch__search_text()` for string literals/error messages
2. Use `mcp__serena__find_referencing_symbols()` to trace data flow
3. Use `mcp__serena__search_for_pattern()` for state access patterns (e.g., `"ctx->.*focused_panel"`)

**Implementation (New Code):**
1. Use `mcp__serena__get_symbols_overview()` to understand existing structure
2. Use `mcp__serena__insert_after_symbol()` / `insert_before_symbol()` for new functions
3. Use `mcp__serena__replace_content()` for small in-function edits
4. Use traditional `Edit` tool only when semantic tools aren't appropriate

### Anti-Patterns to Avoid

**DON'T:**
- Use `Grep` when `mcp__jcodemunch__search_symbols()` would find it instantly
- Use `Read` entire files when `mcp__serena__get_symbols_overview()` gives you the structure
- Use `Edit` for function replacement when `mcp__serena__replace_symbol_body()` is safer
- Guess where symbols are defined - **search semantically first**

**DO:**
- Start exploration with semantic tools
- Use semantic search before falling back to text search
- Leverage symbol-level editing for architectural changes
- Combine tools: jCodemunch for discovery, Serena for manipulation

### Memory System (Long-Term Context)

Serena includes a **persistent memory system** for project-specific knowledge:

```bash
# Store architectural decisions
mcp__serena__write_memory(
    memory_name: "refactoring/context_passing_rules",
    content: "All functions MUST receive ViewContext *ctx as first parameter..."
)

# Recall stored knowledge
mcp__serena__read_memory(memory_name: "refactoring/context_passing_rules")

# Organize by topic
mcp__serena__list_memories(topic: "refactoring")
```

Use memories to preserve architectural insights across conversation boundaries.

---

## Critical Context: Current Architecture State

### [COMPLETE] Context-Oriented Refactor (Phase 4.9)

The codebase has been **completely refactored** to eliminate global mutable state:

- **All state consolidated** into `ViewContext` structure ([include/ytree_defs.h:630-729](include/ytree_defs.h))
- **228 function signatures** updated to receive `ViewContext *ctx` parameter
- **Panel state isolated** into `YtreePanel` structures for dual-panel independence
- **Zero runtime regressions** - 100% test suite passing after refactor

### [COMPLETE] Context-Passing Migration

The migration to a fully context-passing architecture is **complete**. The `GlobalView` compatibility bridge has been removed. All functions now receive explicit context pointers (`ViewContext *ctx`, `YtreePanel *`, or `Volume *`) as their first argument. Only three permitted global exceptions remain (immutable config tables and the POSIX signal flag). See [ARCHITECTURE.md §3](doc/ARCHITECTURE.md) for details.

**Rules for new code:**
- [REQUIRED] Always accept `ViewContext *ctx` (or a more specific context pointer) as first parameter
- [REQUIRED] Pass context explicitly down call chains
- [FORBIDDEN] Introducing any new global mutable state

---

## The Anti-Patching Directive

**CRITICAL ARCHITECTURAL RULE:**

> Do not apply superficial fixes for deep architectural problems.

If a bug is caused by:
- Fragmented state across multiple files
- Global variables being modified inconsistently
- Panel state collision (left/right panels interfering)
- Logic duplicated in multiple locations

**STOP.** Do not patch the symptom. Refactor the architecture to unify the logic before fixing the specific bug.

**Rationale:** It is better to break one thing to fix the system than to patch the system and break everything.

---

## Quick Reference Commands

```bash
# Build
make clean && make

# Run tests
source .venv/bin/activate;pytest

# Run with debug logging
./ytree 2>/tmp/ytree_debug.log

# Check for memory leaks
valgrind --leak-check=full ./ytree
```

---

## When You Need More Information

1. **Understanding a bug?** Read [doc/SPECIFICATION.md](doc/SPECIFICATION.md) to verify expected behavior
2. **Planning a feature?** Read [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) for design constraints
3. **Implementing code?** Read [.agent/rules/builder.md](.agent/rules/builder.md) for coding standards
4. **Writing tests?** Read [doc/ai/TESTING.md](doc/ai/TESTING.md) for test patterns
5. **Debugging architectural issues?** Read [doc/ai/DEBUGGING.md](doc/ai/DEBUGGING.md) for the External Expert Analysis method
6. **Reviewing architecture?** Read [.agent/rules/auditor.md](.agent/rules/auditor.md) for audit guidelines

**Progressive Disclosure Principle:** Only read the documents you need for the current task. Don't try to load all context upfront.
