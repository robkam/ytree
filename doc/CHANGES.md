# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.0-pre-alpha] - 2026-03-30

*Modernization Project initiated 30 Oct 2025. This release represents a comprehensive architectural refactor from the legacy 2.10 codebase to modern C99/POSIX standards. While the bulk of the core modernization is complete, it is designated as **pre-alpha** as UI/UX tightening and stability refinements continue.*

### Major Architectural & Core Modernization
- **Refactoring & Standardization**: Ported legacy C89 code to modern C99/POSIX standards.
- **SRP & SoC Enforcement**: Deep-dive refactor to decouple the Filesystem Model (Model) from the UI (View), eradicating global "God Objects" and implicit state leakage.
- **Source Normalization**: Total reorganization of the source tree into semantic layers (`core`, `ui`, `fs`, `cmd`, `util`) with modular header decomposition.
- **Memory Safety & Hygiene**: Established a zero-leak baseline using Valgrind/ASan, implemented standardized `xmalloc` wrappers, and modernized user/group database handling.
- **Signal Safety & Precision**: Replaced hazardous `SIGALRM` based clock handlers with event-loop integration and enforced strict signal-safety for application shutdown.
- **Dynamic Layout Engine**: Eliminated layout "magic numbers" in favor of a runtime geometry engine that supports responsive resizing.
- **Identifier Normalization**: Anglicised the source code by replacing legacy German identifiers with English equivalents.
- **UTF-8 Support**: Full wide-character support (`ncursesw`) for correct display of Unicode filenames.

### Multi-Volume & Archive Management
- **Multi-Volume Architecture**: Support for logging multiple drives, directories, or archives simultaneously.
- **Volume Navigation**: New Volume Menu (`K`) and cycling (`<`, `>`) to switch between loaded contexts (including integrated release/unlog via `D`).
- **Universal Archive Engine**: Fully integrated `libarchive` as the primary engine for robust browsing and extraction (ZIP, TAR, 7Z, ISO, etc.), replacing dozens of external utilities.
- **Atomic Archive Modification**: Implemented a "Stream Rewrite" engine allowing for atomic entry addition, deletion, and renaming within compressed containers.
- **Transparent Navigation**: Seamlessly traverse nested archives and exit archive contexts back to the physical filesystem using intuitive navigation logic.
- **UDF/ISO Bridge Support**: Enhanced detection logic to correctly handle multi-format bridge media (e.g., modern Windows ISOs).
- **Archive Search & Execute**: Support for searching (`^S`) and executing (`X`) files directly within compressed archive containers.

### UI/UX & Power-User Features
- **Modernized Interface**: Redesigned statistics panel with "boxed" layout, responsiveness to terminal size, and human-readable file sizes.
- **Split-Screen (F8)**: Support for independent panes with separate context, cursors, and filters for efficient cross-volume operations.
- **Integrated Window Stack**: Implemented a tiered Dialog Manager for flicker-free rendering of prompts, menus, and history pop-overs.
- **Advanced Comprehensive Filtering**: Unified filter stack supporting complex Regex patterns, Date ranges, File Attributes, and Size suffixes.
- **Iterative Incremental Search**: High-speed list navigation (`/`) with sticky-cursor logic for both file and directory windows.
- **Internal File Preview (F7)**: Fast, integrated file inspection mode that seamlessly toggles the statistics panel.
- **Vi-Keys Profile Option**: Runtime toggle for `h`/`j`/`k`/`l` navigation (remapped conflicting `K`/`L` keys).
- **Activity Spinner**: Visual feedback in the menu bar during long operations.
- **Contextual History**: Separated command history into relevant categories with support for "favorite" entries.
- **UI Consistency**: Standardized input prompt padding and command line geometry across all application modules.

### Infrastructure, QA & Bug Fixes
- **Clock & Date Localization**: Stabilized the real-time clock and date handlers, resolving the "beta" state from v2.10.
- **Build System**: Updated Makefile for dependency tracking and automated manpage generation via `pandoc`.
- **QA Suite**: Integrated GitHub Actions CI and expanded `pytest`/`pexpect` coverage for core TUI behavioral validation.

## [2.10]
- 7zip / iso support.
- Should compile with gcc 15.

## [2.09]
- Window resize support for hex-view and hex-edit.
- Display format improvements.

## [2.08]
- "Show tagged files only" in file windows (Shift-F4 or Ctrl-F4).
- Lots of UTF_8 improvements/bugfixes.
- Improve compatibility with old unix systems / some minor fixes.

## [2.07]
- Removed -O compiler switch due to problems with UTF_8 code.
- Added workaround when resizing ytree window during string input.

## [2.06]
- Fixed compile error in input.c (UTF_8 only).

## [2.05]
- Uses **regcomp** instead of deprecated re_comp/re_exec (**Unix/POSIX**).
- Debug file sort issue for very large file sizes.
- Reduced compiler warnings.

## [2.04]
- Avoid security warning for mvprintw in input.c.

## [2.03]
- cleanup + added LDFLAG "-ltinfo" (needed for some newer ncurses versions).

## [2.02]
- Crash during startup (group.c).

## [2.01]
- Updated man page (Thanks to Micah Muer).
- Debug crash when using "tagged rename".

## [2.00]
- No new functionality, but changed the copyright to GPLv2 or (at your option) any later version.

## [1.99pl2]
- Filename extensions debug (Thanks to Frank).

## [1.99pl1]
- Debug Path display for non color mode (Thanks to Ali).

## [1.99]
- Debug crash when switching directory view.
- Support large disks.

## [1.98]
- Bugfix for crash during showall with empty subfolders.
- New function "ListJump" (F12).
- Removed some compiler warnings.

## [1.97]
- Bugfixing in pathcopy / allow cancel during tagged copy.

## [1.96]
- Bugfix for long usernames in /etc/passwd.

## [1.95]
- Improved UTF-8 support.

## [1.94]
- Adapted Makefile to respect environment CC and LDFLAGS.
- New login to parent node ('p') command.

## [1.93]
- Debug Crash during switch to file window (Fedora 11).

## [1.92]
- Compile on DJGPP.
- Debug archive read if clock is enabled.

## [1.91]
- Compiles on Mac OS 10.5.4.
- Debug crash for tagged delete.

## [1.90]
- Changes for NetBSD.

## [1.89]
- Changes for NetBSD.

## [1.88]
- Changes for OpenBSD.

## [1.87]
- Removed warning in util.c.

## [1.86]
- Fixed crash when deleting last file in a directory.

## [1.85]
- Hopefully compiles again on sun architecture.

## [1.84]
- Enhanced UTF-8 support + russian man-page.

## [1.83]
- Bugfixes in Hex-Editor.
- UTF-8 support.

## [1.82]
- Removed character mangling bug in "Path:" line on certain platforms (at least Solaris).
- Ported to Mac OS X.

## [1.81]
- Hex-Editor.

## [1.80]
- ^Search command to untag files using an external program.

## [1.79]
- Libreadline support optional. Changed license from GPL 1 to GPL 2 due to use of libreadline.

## [1.78-cbf]
- Securityfix: ~/.ytree-hst now honors umask.
- Bugfix: some window sizes and layout adjustments.
- New builtin hexdump with some code to do the editing of hex files.
- Readline integration into InputString including tilde expansion.
- Cleanup dirwin.c and others to make it more readeable.

## [1.78]
- Bugfix: Copy to rel. nonexisting path does not work (Thanks to ZP Gu).

## [1.77]
- Builtin hexdump.

## [1.76]
- Added QuitTo feature.

## [1.75]
- Port to OpenBSD.

## [1.74]
- Sort by extension now with secondary key.
- Port to GNU Hurd.

## [1.73]
- Show all tagged files added.
- Makefile update.
- Layout adjustments.

## [1.72]
- Added fully user-configurable commands and menu text.
- New command-line arguments: -p (profile), -h (history).
- Added configuration items: INITIALDIR, [MENU], [DIRCMD], [DIRMAP], [FILECMD], [FILEMAP].

## [1.71]
- ia64 ready.
- port to QNX 4.25 using ncurses 4.2.
- native X11 port using PDCurses.
- added configuration items: NOSMALLWINDOW, NUMBERSEP, FILEMODE.

## [1.70]
- Large file support.
- New spanish man-page.
- Dynamic resize window support.
- Bugfixes...

## [1.67]
- rar support.
- No 4GB freespace limit.

## [1.66]
- User defined view.
- spm Support.

## [1.65]
- FreeBSD Port.
- DJGPP Port.
- "jar" Support.
- minor changes.

## [1.64]
- rpm Support.
- Bugfixes...

## [1.63]
- Bzip2 Support.
- 8bit file/directory-names.
- Bugfixes...

## [1.62]
- Better escaping of meta-characters.
- Some minor bugfixes.

## [1.61]
- Clock Support.
- Enhanced Sort.
- Display Directory Attributes.
- Viewer Configuration.
- Color support.
- Command line history.
- Visual directory selection.
- Helper-Applications now configurable in ~/.ytree.
- Force removing of non-empty directories.
- Dynamic directory scanning.
