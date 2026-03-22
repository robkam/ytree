---
name: ui-economy-navigation
description: Design ytree interactions to minimize menu depth and cognitive load using shallow, high-signal choice flows.
---

# UI Economy Navigation

Use this skill for UI workflow, prompt flow, menu structure, and interaction design decisions.

## Goal

Prefer shallow interaction flow:

- Avoid: `choice -> choice -> choice -> choice -> ... -> result`
- Prefer: `choice -> choices -> result`

## Design Rules

1. Common path target is `key -> Enter -> result` whenever safe and practical.
2. If extra detail is needed, use progressive disclosure in the second step, not extra nested menus.
3. Group related actions into one high-signal chooser instead of chained single-choice prompts.
4. Preserve expert speed: include direct key paths for common actions.
5. Keep labels concrete and action-oriented.

## Decision Budget

- Submenu depth budget on common path: <= 1.
- Hard warning threshold: > 1 submenu.
- If > 1 submenu is unavoidable, justify why and provide an equivalent fast path.

## Verification Checklist

- Can a frequent task run as `key -> Enter -> result`?
- Are low-frequency options collapsed without hiding core actions?
- Is there a keyboard fast path for experts?
- Do menu/help labels match actual behavior?
