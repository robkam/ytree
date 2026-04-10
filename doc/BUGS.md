# **Verified Bugs and Defects**

This file tracks confirmed bugs, architectural violations, and naming inconsistencies that require remediation.

## **UI / Key Interaction**

### **BUG-001: Misleading Tree Expansion Action Names**
*   **Description**: The internal `YtreeAction` names for tree expansion are swapped relative to their behavior and documentation.
*   **Findings**:
    *   `+` key maps to `ACTION_TREE_EXPAND_ALL`, but only expands **one level**.
    *   `*` key maps to `ACTION_ASTERISK`, and expands **recursively**.
*   **Impact**: Developer confusion and maintenance risk.
*   **Remediation**: 
    *   Rename `ACTION_TREE_EXPAND_ALL` -> `ACTION_TREE_EXPAND` (or `ACTION_TREE_EXPAND_LEVEL`).
    *   Rename `ACTION_ASTERISK` -> `ACTION_TREE_EXPAND_RECURSIVE`.
*   **Status**: Confirmed.

### **BUG-002: VI Mode Key Ambiguity and Collisions**
*   **Description**: When `VI_KEYS=1` is enabled, lowercase navigation keys (`h/j/k/l`) collide with primary command keys without clear UI signaling.
*   **Findings**:
    *   `j` maps to both `ACTION_MOVE_DOWN` (via `VI_KEY_DOWN`) and historically to `ACTION_LOG_VOLUME` (though currently `l/L` is the log volume key, older documentation/muscle memory remains confused).
    *   `k/K` is used for `ACTION_VOL_MENU`. In VI mode, lowercase `k` is stolen for `Up`, making the volume menu reachable only via uppercase `K`.
*   **Impact**: Inconsistent UI accessibility for power users.
*   **Remediation**: Audit all `VI_KEY` remappings in `key_engine.c` and ensure the footer help lines (`display.c`) dynamically update to show the uppercase variants when `VI_KEYS=1`.
*   **Status**: Confirmed.

### **BUG-003: Incremental Search Legacy Mapping (`F12`)**
*   **Description**: `F12` is used as an alias for `/` (Incremental Search/Jump), but its presence is inconsistent in help strings and documentation.
*   **Impact**: Confuses users about "hidden" keys.
*   **Remediation**: Explicitly document `F12` as a legacy alias or deprecate it in favor of standard `/`.
*   **Status**: Confirmed.

## **Configuration & Consistency**

### **BUG-004: Configuration Template Drift (`VI_KEYS`)**
*   **Description**: Discrepancy in default visibility and documentation for `VI_KEYS`.
*   **Findings**:
    *   `default_profile_template.h` uses `VI_KEYS=0`.
    *   `etc/ytree.conf` uses `VI_KEYS=0`.
    *   User scratchpad reports `ytree.conf` had `1`. 
*   **Impact**: Users may experience "magic" behavior changes if the disk-based config diverges from the internal template.
*   **Remediation**: Ensure the `ytree --init` generation path strictly matches the `etc/ytree.conf` provided in the distribution.
*   **Status**: Verified (Potential environment-specific drift).

## **User Experience (UX)**

### **BUG-005: Recursive Scan Interrupt Responsiveness**
*   **Description**: Interrupting a recursive expansion (`*`) via `ESC` is supported but requires multiple keypresses (Prompt Y/N).
*   **Impact**: Users cannot instantly halt accidental large-branch scans.
*   **Remediation**: Evaluate if `ESC` during `ReadTree` should immediately halt the scan once instead of prompting, given that partial results are preserved.
*   **Status**: Investigation Needed.
