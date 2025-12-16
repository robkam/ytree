# Why Modernize ytree?

The `ytree` and `UnixTree` projects both aimed to provide a file manager for Unix-like systems inspired by the `XTree` application for DOS. Given the existence of `UnixTree`, this document clarifies the goals and architectural philosophy behind the ongoing modernization of the `ytree` codebase.

The key difference between the projects lies in their architectural approach, which reflects the eras in which they were developed.

### Architectural Approaches

#### 1. The Self-Contained Framework (`UnixTree`)

The original `UnixTree` was developed in a time of diverse and often incompatible commercial Unix systems (e.g., AIX, HP-UX, Solaris). To ensure portability across these fragmented environments, its architecture relied on a self-contained framework. This involved bundling and maintaining custom or heavily modified libraries for core functions, including its own version of the `curses` library (`libecurses`) and a complex, platform-specific build system.

This approach was a practical solution to the problem of inconsistent system APIs at the time. By bundling its own dependencies, the application could guarantee a consistent environment at the cost of increased size and maintenance complexity.

#### 2. Leveraging Standard Libraries (`ytree`)

The modern `ytree` project takes a different approach, leveraging the stability and standardization of modern POSIX-compliant operating systems. Instead of bundling dependencies, `ytree` relies on standard, well-maintained system libraries that are now widely available:

*   **Terminal Interface:** The industry-standard **`ncurses`** library.
*   **Archive Handling:** The **`libarchive`** library for reading a wide variety of formats.
*   **Command Input:** The GNU **`readline`** library.
*   **Filesystem Information:** Standard POSIX APIs such as **`statvfs`**.

### Advantages of the Modern Approach

Relying on standard system libraries provides several advantages for the `ytree` project:

*   **Maintainability:** The codebase is smaller and easier for new contributors to understand. Development can focus on the core file management features rather than on maintaining low-level libraries.
*   **Stability:** `ytree` benefits from the ongoing maintenance and bug fixes applied to these shared system libraries by the broader open-source community.
*   **Capability:** When a library is updated with new features (e.g., `libarchive` adds support for a new compression format), `ytree` can benefit from these improvements.
*   **Simplified Build Process:** The project can be compiled on any compliant system with a standard `Makefile`, without requiring complex configuration scripts.

### Conclusion

The `UnixTree` project was a capable tool designed for the constraints of its time. The goal of modernizing `ytree` is to build a file manager that is architecturally suited to the current landscape of open-source operating systems. By using standard, shared libraries, `ytree` aims to be a lean, maintainable, and powerful tool that is easy for users to compile and for developers to extend.