---
name: ncurses-render-safety
description: Apply safe ncurses rendering patterns in ytree to prevent flicker, color bleed, and redraw regressions.
---

# Ncurses Render Safety

Use this skill for rendering, color, redraw, or window-painting changes.

## Core Rules

1. Set window background once per refresh path, not per line.
2. After setting a new window background, clear with `werase()` before redraw.
3. Use `wattron()`/`wattroff()` for line-level styling only.
4. Keep redraw ownership clear: you MUST NOT introduce hidden side effects across modules.

## WbkgdSet Rule

- `WbkgdSet()` applies to the full window background state.
- Do not call it inside row/entry rendering loops.
- Typical safe shape:
  - Set background once in top-level display function.
  - `werase()` the window.
  - Render lines with temporary attributes.

## Verification Checklist

- No repeated `WbkgdSet()` in per-item loops.
- No background color bleed between entries.
- No flicker introduced under frequent refresh.
- Active/inactive panel visuals remain consistent.
