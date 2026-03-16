# Style and Conventions

## Core Architectural Rules
- **No Global Mutable State:** The codebase has been completely refactored to eliminate global mutable state.
- **`ViewContext *ctx` Requirement:** All 228 function signatures have been updated to receive a `ViewContext *ctx` parameter as the first argument. This context encapsulates all local and panel state.
- **Compatibility Bridge (Removed):** The former global pointer `ViewContext *GlobalView` has been fully removed. The name "GlobalView" only survives in two function names (`RefreshGlobalView`, `InitGlobalView`) which both properly accept `ViewContext *ctx`. These have been renamed to `RefreshView`/`InitView`.
- **Remaining Globals (3 total, all justified):** `ui_colors[]` and `NUM_UI_COLORS` (app config, defined in `color.c`), and `volatile sig_atomic_t ytree_shutdown_flag` (signal handler, defined in `main.c` — POSIX requirement).
- **Dual-Panel Independence:** Panel state is isolated into `YtreePanel` structures to prevent left/right panels from interfering.
- **File Hierarchy:** Main logic is in `src/`, headers in `include/`, documentation in `doc/`. Look in `include/ytree_defs.h` for the `ViewContext` structure definitions.

## The Anti-Patching Directive
**Do not apply superficial fixes for deep architectural problems.**
If a bug is caused by fragmented state, inconsistent global variable modification, panel state collision, or duplicated logic: **STOP**. Do not patch the symptom. Refactor the architecture to unify the logic before fixing the bug. Breaking one thing to fix the system is better than patching the system to break everything.

## Persona Rules and Process Guidance
The AI agent must operate strictly via `.agent/rules/`:
- Use **@architect** for planning and architecture logic. **Never implement code blindly**; always propose a high-level design and wait for explicit approval.
- Use **@developer** for source code implementation (must output Complete Files Only).
- Use **@code_auditor** for discovering technical debt, coupling, and fragility.
- Use **@tester** for behavioral TUI verification using `pexpect` (run within `.venv`).
