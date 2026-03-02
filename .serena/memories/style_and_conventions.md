# Style and Conventions

## Core Architectural Rules
- **No Global Mutable State:** The codebase has been completely refactored to eliminate global mutable state.
- **`ViewContext *ctx` Requirement:** All 228 function signatures have been updated to receive a `ViewContext *ctx` parameter as the first argument. This context encapsulates all local and panel state.
- **Compatibility Bridge (`GlobalView`):** A single global pointer (`ViewContext *GlobalView = NULL;`) exists only for legacy code compatibility. **New code MUST avoid using `GlobalView`** and instead pass `ctx` explicitly down the call chains.
- **Dual-Panel Independence:** Panel state is isolated into `YtreePanel` structures to prevent left/right panels from interfering.
- **File Hierarchy:** Main logic is in `src/`, headers in `include/`, documentation in `doc/`. Look in `include/ytree_defs.h` for the `ViewContext` structure definitions.

## The Anti-Patching Directive
**Do not apply superficial fixes for deep architectural problems.**
If a bug is caused by fragmented state, inconsistent global variable modification, panel state collision, or duplicated logic: **STOP**. Do not patch the symptom. Refactor the architecture to unify the logic before fixing the bug. Breaking one thing to fix the system is better than patching the system to break everything.

## Persona Rules and Process Guidance
The AI agent must operate strictly via `.agent/rules/`:
- Use **@consultant** for planning and "Architect Mode" refactoring logic. **Never implement code blindly**; always propose a high-level design and wait for explicit approval.
- Use **@builder** for source code implementation (must output Complete Files Only).
- Use **@auditor** for discovering technical debt, coupling, and fragility.
- Use **@tester** for behavioral TUI verification using `pexpect` (run within `.venv`).