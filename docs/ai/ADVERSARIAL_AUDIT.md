# Adversarial Audit Prompt

Use `code_auditor` when the goal is to hunt for serious defects, fragility, sloppy engineering, security issues, or memory-safety problems.

## Ready-To-Paste Prompt

```text
:at code_auditor
use skill rm-code-smells

Audit ytree as if you were actively trying to prove it is unsafe, fragile, or sloppily engineered.

Be adversarial, but stay evidence-based:
- prioritize real defects over style nits
- reason from first principles about how this code could fail, corrupt state, violate invariants, leak resources, expose unsafe behavior, mis-handle untrusted input, or mislead future maintainers
- actively search for the strongest evidence that the implementation is wrong, brittle, under-validated, over-coupled, or unsafe in normal paths, degraded states, and edge conditions
- look for root-cause problems, not isolated symptoms or one-off bad lines
- assume the code is guilty until proven innocent
- do not invent findings; mark uncertainty explicitly

Return findings only, ordered by severity.

For each finding include:
- Severity: blocker | high | medium | low
- File:line
- Evidence
- Impact
- Concrete fix
- Minimal trigger or failure path, if inferable

Also include:
- the single most dangerous bug if exploited
- the single most likely real-world failure mode
- the top 3 sloppiest or most fragile areas even if they are not immediate crashers
- final gate status: pass or fail

Do not praise the code.
Do not give a balanced summary unless no findings remain.
```
