# Frequently Asked Questions

## Development Decisions

### How can I trust a file manager written with AI assistance on my filesystem?

Ytree performs standard file operations (rename, move, copy, delete) through normal OS APIs, just like other file managers.

This is the same trust model you should use for any file manager: do not assume zero risk, verify behavior, keep backups, and, at first, try it out in a non-critical directory.

For detailed safeguards, limits, and verification pointers, see [`TRUST.md`](TRUST.md).

### How is AI used in this project?

AI is used as an implementation assistant, not as an autonomous authority.

The human maintainer owns design decisions, architecture constraints, and merge quality. AI helps accelerate coding and refactoring work, but changes are accepted only after manual review plus repository QA gates. This workflow is iterative and often slow: a lot of the effort is in steering, verification, and correction.

For the concrete checks and evidence model, see:
- [`docs/AUDIT.md`](AUDIT.md)
- [`docs/TRUST.md`](TRUST.md)
- [`docs/PR_GATE.md`](PR_GATE.md)

### Why release an alpha before beta?

The alpha is published early so users and contributors can inspect the code, test real workflows, and provide feedback while major design decisions are still adjustable.

In short: the project is usable now, but not stable yet. Expect rough edges, occasional regressions, and evolving UX details until beta and then stable release.

### Why refactor the existing code instead of writing a new one from scratch?

The decision was made to modernize the existing codebase because Ytree is a known application with an established look and feel.

Creating a new file manager from scratch would have resulted in a loss of identity and name recognition. By refactoring, we preserve the specific behavior and interface style of the original application while significantly enhancing the user experience and replacing the underlying architecture. This approach revitalizes the project without discarding its history.

### Why is this not written in Rust?

The primary objective of this phase was the architectural cleanup and restoration of the existing C codebase.

Switching languages immediately would constitute a total rewrite rather than a modernization. However, now that the architecture has been simplified, legacy dependencies removed, and the project stabilized, a port to a modern memory-safe language like Rust is a possibility for a future version.

### Why ncurses, why not termbox2 or notcurses?

Ytree only needs fast, reliable text/line-box terminal UI for file and VFS browsing, and ncurses already provides that cleanly, while switching to termbox2 or notcurses would add backend complexity for features outside ytree’s core scope (like richer in-app media rendering) that are better handled by external helper programs.

---

## Project Philosophy

### Why modernize Ytree when UnixTree already exists?

While both projects aim to provide a file manager inspired by [XTree&trade;](https://www.xtreefanpage.org/x10dirja.htm), they represent architectural solutions for different eras.

`UnixTree` was designed for a time of fragmented, incompatible commercial Unix systems. To guarantee portability, it had to be a self-contained monolith, bundling and maintaining its own internal libraries.

The goal of modernizing Ytree is to create a tool native to the *current* landscape of open-source operating systems. By discarding the custom frameworks required in the past and leveraging standard system libraries, Ytree becomes a lean, POSIX-compliant utility that fits naturally into modern distributions, easier to package, audit, and maintain.

### How do the architectures compare?

The difference lies in how they handle system dependencies:

**1. UnixTree: The Self-Contained Framework**
*   **Context:** Built for inconsistent environments (AIX, HP-UX, Solaris).
*   **Approach:** Bundles heavily modified internal libraries (like `libecurses`) to ensure it runs the same everywhere.
*   **Trade-off:** High consistency on legacy systems, but increased code size, complex build requirements, and high maintenance overhead.

**2. Ytree: The Modern Approach**
*   **Context:** Built for standardized **POSIX-compliant Unix** systems (Linux, *BSD, macOS).
*   **Approach:** Offloads functionality to shared, well-maintained system libraries:
    *   **Terminal:** `ncurses` (Industry standard).
    *   **Archives:** `libarchive` (Supports a wide variety of formats).
    *   **Input:** GNU `readline`.
*   **Trade-off:** Requires modern dependencies, but yields a significantly smaller, more secure, and maintainable codebase.

---

## Project Relevance

### Is there still a need for a text-mode file manager?

Yes. While graphical file managers are standard for desktop users, TUI (Text User Interface) tools remain vital for specific workflows:
*   **Server Management:** System administrators working over SSH need efficient tools that do not require a graphical environment.
*   **Efficiency:** For power users, keyboard-driven navigation is often significantly faster than dragging and dropping with a mouse.
*   **Minimalism:** Users of tiling window managers and lightweight distributions often prefer low-resource, terminal-based applications.

### Who is the target audience?

Ytree specifically targets:
1.  **XTree&trade; Veterans:** Users who developed "muscle memory" for the XTree&trade; layout and keybindings in the DOS era and find the Midnight Commander style unintuitive.
2.  **Terminal Power Users:** Developers and Admins who want a fast, lightweight file manager that integrates seamlessly with their shell history and standard CLI tools.
3.  **Open Source Archivists:** Those interested in keeping classic Unix tools alive, compilable, and secure on modern hardware.
