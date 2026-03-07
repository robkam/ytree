# Frequently Asked Questions

## Project Philosophy

### Why modernize Ytree when UnixTree already exists?

While both projects aim to provide a file manager inspired by [XTree](https://www.xtreefanpage.org/x10dirja.htm), they represent architectural solutions for different eras.

`UnixTree` was designed for a time of fragmented, incompatible commercial Unix systems. To guarantee portability, it had to be a self-contained monolith, bundling and maintaining its own internal libraries.

The goal of modernizing Ytree is to create a tool native to the *current* landscape of open-source operating systems. By discarding the custom frameworks required in the past and leveraging standard system libraries, Ytree becomes a lean, POSIX-compliant utility that fits naturally into modern distributions—easier to package, audit, and maintain.

### How do the architectures compare?

The difference lies in how they handle system dependencies:

**1. UnixTree: The Self-Contained Framework**
*   **Context:** Built for inconsistent environments (AIX, HP-UX, Solaris).
*   **Approach:** Bundles heavily modified internal libraries (like `libecurses`) to ensure it runs the same everywhere.
*   **Trade-off:** High consistency on legacy systems, but increased code size, complex build requirements, and high maintenance overhead.

**2. Ytree: The Modern Approach**
*   **Context:** Built for standardized POSIX systems (Linux, *BSD, macOS).
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
1.  **XTree Veterans:** Users who developed "muscle memory" for the XTree layout and keybindings in the DOS era and find the Norton Commander style (used by `mc`) unintuitive.
2.  **Terminal Power Users:** Developers and Admins who want a fast, lightweight file manager that integrates seamlessly with their shell history and standard CLI tools.
3.  **Open Source Archivists:** Those interested in keeping classic Unix tools alive, compilable, and secure on modern hardware.

---

## Development Decisions

### Why refactor the existing code instead of writing a new one from scratch?

The decision was made to modernize the existing codebase because Ytree is a known application with an established look and feel.

Creating a new file manager from scratch would have resulted in a loss of identity and name recognition. By refactoring, we preserve the specific behavior and interface style of the original application while significantly enhancing the user experience and replacing the underlying architecture. This approach revitalizes the project without discarding its history.

### Why is this not written in Rust?

The primary objective of this phase was the architectural cleanup and restoration of the existing C codebase.

Switching languages immediately would constitute a total rewrite rather than a modernization. However, now that the architecture has been simplified, legacy dependencies removed, and the project stabilized, a port to a modern memory-safe language like Rust is a possibility for a future version.