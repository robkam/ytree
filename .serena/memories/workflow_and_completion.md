# Workflow and Completion Protocol

When tasked with implementing a feature or fixing a bug in ytree:

1. **Design First:** Act as the **@architect** first. Do not start writing code immediately. Propose a high-level design that resolves the root issue rather than masking symptoms (Anti-Patching Directive). Wait for the user approval.
2. **Implementation:** Act as the **@developer**. Write clean C99 code. Always encapsulate state within `ViewContext` (no global states). Outputs must be complete files.
3. **Verification:**
   - **Compile:** Execute `make DEBUG=1` to compile with AddressSanitizer (ASan). Resolve any warnings or errors. Ensure no conflicting types or implicit declarations.
   - **Test:** Act as the **@tester**. Write or execute behavioral UI tests in Python (using `pexpect` inside `.venv`). Run the test suite via `pytest tests/`.
   - **Audit:** Execute `./ytree` or `valgrind` to verify that there are no memory leaks or segmentation faults resulting from the changes. Act as the **@code_auditor** to continually check for fragility.
4. **Completion:** Confirm that 100% of the relevant test suite passes without regressions. Provide the user with the status of the implementation, any identified technical debt, and a summary of the test results.
