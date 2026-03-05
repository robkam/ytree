# ytree Development Context

## Finding Information

This project maintains comprehensive technical documentation. **Do not guess or assume** - read the appropriate document when you need context.

### Global AI Development Rules

**READ FIRST:** [~/.antigravityrules](~/.antigravityrules) contains high-level directives that apply to ALL AI development work:
- Your role as Senior AI Development Partner under the Lead Architect
- Environmental context (WSL2, POSIX paths, toolchain access)
- Persona orchestration (@consultant, @builder, @auditor, @tester)
- Interaction protocol (design approval workflow)

### Architecture & Technical Specifications

- **[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md)** - System design, core invariants, dual-panel isolation rules, and rendering standards
- **[doc/SPECIFICATION.md](doc/SPECIFICATION.md)** - Behavioral requirements, UI contracts, navigation protocols, and the "XTree" state machine
- **[doc/ROADMAP.md](doc/ROADMAP.md)** - Feature implementation history, current status, and modernization phases

### Development Workflow & Standards

- **[doc/AI_WORKFLOW.md](doc/AI_WORKFLOW.md)** - The Spec-First development protocol and "Golden Loop" methodology
- **[doc/TESTING.md](doc/TESTING.md)** - Test organization standards, naming conventions, and TDD protocol
- **[.agent/rules/*.md](.agent/rules/)** - Role-specific persona instructions for different task types

### Available Development Personas

The `.agent/rules/` directory contains specialized persona definitions. Read the appropriate file when taking on a specific role:

- **[consultant.md](.agent/rules/consultant.md)** - Planning and analysis (behavior-to-C translation, no coding)
- **[builder.md](.agent/rules/builder.md)** - Implementation (C89/C99 standards, complete files only, no snippets)
- **[auditor.md](.agent/rules/auditor.md)** - Architecture review (identify global state coupling and fragility)
- **[tester.md](.agent/rules/tester.md)** - Test generation (Python/pexpect TUI automation)
- **[mentor.md](.agent/rules/mentor.md)** - General technical guidance

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
pytest tests/

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
4. **Writing tests?** Read [.agent/rules/tester.md](.agent/rules/tester.md) for test patterns
5. **Reviewing architecture?** Read [.agent/rules/auditor.md](.agent/rules/auditor.md) for audit guidelines

**Progressive Disclosure Principle:** Only read the documents you need for the current task. Don't try to load all context upfront.
