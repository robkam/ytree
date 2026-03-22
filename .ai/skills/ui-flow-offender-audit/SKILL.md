---
name: ui-flow-offender-audit
description: Detect prompt-chain offenders in ytree UI flows and produce ranked compression plans that enforce the UX economy gate.
---

# UI Flow Offender Audit

Use this skill when auditing interaction flow depth, prompt chaining, or menu-dive friction.

## Audit Objective

Find flows where the common path violates UX economy:

- Common-path target: `key -> Enter -> result`
- Maximum common-path submenu depth: 1

## Workflow

1. Locate interaction chains in code/tests (menus, prompts, modal choices).
2. For each flow, capture:
   - current chain
   - common-path submenu depth
   - common-path decisions before result
3. Flag offenders where depth > 1 or where defaults cannot reach fast path.
4. Propose compressed flow preserving behavior and safety.
5. Rank offenders by user-frequency impact and chain depth.

## Required Output

- Ranked offender table with:
  - command/flow name
  - current chain
  - proposed chain
  - current vs proposed submenu depth
  - expected speed gain
  - likely files/tests to update
- Per-offender implementation prompt suitable for a clean-slate AI.

## Guardrails

- Do not remove safety confirmations for destructive operations without replacement safeguards.
- Prefer combining decisions into one prompt over adding nested submenus.
- Keep advanced options available as in-prompt toggles/defaults.
