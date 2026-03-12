# ytree - A File Manager for UNIX

---

> [!IMPORTANT]
> **STATUS: PRE-ALPHA and UNDER HEAVY DEVELOPMENT**
>
> This is a massive modernization, rewrite, and enhancement of Ytree v2.10. The original program is being drastically modified, but the TUI still follows the classic XTree&trade; model.
>
> **Notice to other developers:**
> I am a solo developer working on this as much as my spare time and tokens allow. I am the driver of this project, utilizing an agentic IDE, but I am entirely reliant on LLMs, AI agents, and MCP utilities; I simply would not have attempted this rewrite without them.
>
> Because of this workflow, and because I am figuring it out as I go along, I am not yet looking for traditional code contributions or bug reports. I am currently working through a significant backlog of internal debugging and known issues; I do not have the AI tokens or the bandwidth to investigate external reports, scrutinize pull requests, or manage feature requests. Please see [ROADMAP.md](doc/ROADMAP.md) for current plans and progress.
>
> While I am not actively encouraging general ideas or code critiques, architectural correctness is my highest priority. If you happen to spot a place where my methodology is fundamentally outdated or flawed, you are welcome to open an Issue. I cannot promise a reply, but when I am able to, I will feed valid architectural critiques to the agents to improve the codebase.
>
> I have high aspirations for this rewrite, and because reaching that standard is still some way down the road and because perfect is the enemy of good, I have decided to share this work in progress now.
>
> Ytree is currently usable, but it is still imperfect. Parts might not work properly and are absolutely subject to change.

---

**Ytree** is a file manager for UNIX-like systems (Linux, BSD, etc.), optimized for speed and keyboard efficiency.

## Background

Born from the lineage of [XTree&trade;](https://www.xtreefanpage.org/lowres/x10dirja.htm) (DOS),  [Ytree](https://www.han.de/~werner/ytree.html)  was intended to be the definitive tree-based logger for Unix. While it has been maintained for compatibility over the decades, its feature set remained largely frozen in the late 1990s, leaving Linux users without a true equivalent to the powerful "log and tag" workflow.

Many file managers today function as "browsers"—they look at one directory at a time and rely on the OS to fetch files on demand. `Ytree` is different: it is a **Logger**. It scans ("logs") entire drive hierarchies into memory. This treats the filesystem as a database, allowing you to **Show All** files in a flat view, filter across thousands of subdirectories instantly, and perform bulk operations on tagged files regardless of their location.

This v3.0 project modernizes `Ytree` to fulfill its potential, introducing features required by modern power users (like split-screen and integrated autoview) while moving to a clean, modern C99 architecture.

## Development Methodology

This refactor serves as a case study in using Large Language Models (LLMs) to modernize legacy code. The codebase was not simply "ported"; it was systematically disassembled and re-architected. An LLM was utilized to analyze the original K&R C source, understand the undocumented logic, and reimplement it using modern C99 standards, the MVC pattern, and strict encapsulation. This demonstrates that with persistence and strict architectural guidance, AI tools can be effectively used to maintain and improve serious systems software.

## Features (v3.0 Alpha)

*   **Classic XTree&trade; Interface:** Directory Tree + File List layout.
*   **Split Screen Mode (F8):** Manage two independent file panels side-by-side.
*   **File Preview (F7):** Instant view of file contents without launching external tools.
*   **Multi-Volume Support:** Log multiple drives or archives simultaneously and switch instantly.
*   **Archives as Directories:** Browse ZIP, TAR, GZ, and ISO files transparently using `libarchive`.
*   **Advanced Filtering:** Filter by RegEx, Attribute, Date, and Size.
*   **Modern Architecture:** Clean C99, strict context-passing design — no global mutable state. See [ARCHITECTURE.md](doc/ARCHITECTURE.md).
*   **Auto-Refresh:** Inotify integration for live directory updates.
*   **External Viewers:** Associate specific file extensions with external programs (images, PDFs, etc.).
*   **User Commands:** Bind keys to custom shell commands/scripts for infinite extensibility.

## Screenshots

<details>
<summary>Click to view Gallery</summary>

**1. The Classic Interface**
Visualize and navigate your directory hierarchy instantly.
![Main View](doc/screenshots/01_main.png)

**2. Split Screen & Archives**
Manage two independent panels. Here, browsing an ISO on the left and copying files directly to the Home Directory on the right.
![Split Screen Archive](doc/screenshots/02_split_archive.png)

**3. Integrated Preview**
Inspect file contents without leaving the file manager. (Shown: Previewing a file *inside* an ISO archive).
![Preview Mode](doc/screenshots/03_preview.png)

</details>

## Installation

### Prerequisites

*   **C Compiler** (GCC or Clang)
*   **ncurses** (libncurses-dev / ncurses-devel)
*   **readline** (libreadline-dev / readline-devel)
*   **libarchive** (libarchive-dev / libarchive-devel)

### Build from Source

```bash
# Clone the repository
git clone https://github.com/robkam/ytree.git
cd ytree

# Compile (Optimized Release Build)
make

# Install
sudo make install

# Uninstall
sudo make uninstall
```

*Note: Developers can compile with AddressSanitizer enabled by running `make DEBUG=1`.*

### Optional: VI Keys Navigation

By default, Ytree uses **case-insensitive key bindings** (e.g., both 'k' and 'K' open the volume menu).

If you prefer **vi-style cursor navigation** (h/j/k/l keys), you can enable it:

```bash
# Edit Makefile and uncomment the VI_KEYS line:
# ADD_CFLAGS  = -DVI_KEYS

# Then rebuild:
make clean && make
```

**VI Keys mappings when enabled:**
- `h` = LEFT, `j` = DOWN, `k` = UP, `l` = RIGHT
- `Ctrl+D` = PAGE DOWN, `Ctrl+U` = PAGE UP

**Note:** When VI keys are enabled, lowercase 'k' will move the cursor UP instead of opening the volume menu. Use uppercase 'K' for volume menu in VI mode.

## Usage

For detailed usage, configuration, and keybindings, see [USAGE.md](doc/USAGE.md) or run `man ytree`.

## Changelog

See [CHANGES.md](doc/CHANGES.md) for a detailed history of changes and version updates.

## Reporting problems

As noted in the **Pre-Alpha** warning at the top of this document, I am not currently accepting general bug reports. I have a substantial backlog of known issues and internal debugging to complete before the project is ready for external feedback.

If you have a critical **architectural critique** regarding the core design or memory safety of the rewrite, please open a GitHub Issue.

<!--
## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](doc/CONTRIBUTING.md) for guidelines. See [SPECIFICATION.md](doc/SPECIFICATION.md) for behavioral requirements, and [ARCHITECTURE.md](doc/ARCHITECTURE.md) to understand the system design before submitting code.

## Reporting problems

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytree/issues).

It will help us to address the issue if you include the following:
*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **Ytree version:**
*   **Steps to Reproduce:**
*   **Expected Behavior:**
*   **Actual Behavior:**
-->

## License

Ytree is free software distributed under the GPL. See the [LICENSE.md](LICENSE.md) file for details.

## Contributors

For detailed authorship, see [AUTHORS.md](doc/AUTHORS.md).

