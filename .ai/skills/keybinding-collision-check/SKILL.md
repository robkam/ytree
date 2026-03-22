---
name: keybinding-collision-check
description: Validate ytree keybinding changes for ASCII/control-key collisions and keep menus/help/tests synchronized.
---

# Keybinding Collision Check

Use this skill when adding or changing keybindings, command labels, or help/footer text.

## Collision Rules

Do not bind these control keys because they collide with core terminal behavior:

- `^I` (`0x09`) collides with `Tab`
- `^M` (`0x0D`) collides with `Enter` (CR)
- `^J` (`0x0A`) collides with `Enter` (LF)
- `^[` (`0x1B`) collides with `Escape`

When `VI_KEYS` is enabled, these six keys are reserved for navigation:

- `h` -> left
- `j` -> down
- `k` -> up
- `l` -> right
- `Ctrl-U` (`0x15`) -> page up
- `Ctrl-D` (`0x04`) -> page down

Note: control-key notation is terminal-byte based and case-insensitive.
Use `Ctrl-U` / `Ctrl-D` wording to avoid ambiguity.

Any command bound to these keys must remain reachable without breaking vi navigation.
Preferred rule: keep vi navigation on lowercase and move command bindings to uppercase
or another non-conflicting key.

## Synchronization Checklist

When keybindings change, update and verify all relevant surfaces:

1. Input handling / action mapping code.
2. Footer/menu/help text shown to users.
3. Tests and centralized key abstractions.
4. Any docs that describe command keys.
5. `VI_KEYS` behavior:
   - With `VI_KEYS` enabled, lowercase `h/j/k/l` and `Ctrl-U`/`Ctrl-D` still navigate.
   - Command actions that previously used those keys remain reachable via uppercase
     or explicit alternative bindings.

## Verification

- New key does not shadow existing high-value key.
- Mode-specific behavior is deterministic.
- Help/menu text matches actual behavior.
- Key behavior is valid in both builds: default build and `VI_KEYS` build.
