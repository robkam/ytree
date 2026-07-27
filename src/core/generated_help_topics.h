/* Auto-generated from etc/help/help.en.md by scripts/generate_help_assets.py. */
#include <stddef.h>

typedef struct {
    const char *label;
    const char *target_topic_id;
} GeneratedHelpLink;

typedef struct {
    const char *title;
    const char *body;
} GeneratedHelpLongFormSection;

typedef struct {
    const char *topic_id;
    const char *title;
    const char *contexts_csv;
    const char *contextual_f1;
    size_t explainer_link_count;
    const GeneratedHelpLink *explainer_links;
    size_t long_form_section_count;
    const GeneratedHelpLongFormSection *long_form_sections;
} GeneratedHelpTopic;

static const GeneratedHelpLink generated_help_links_intro[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_intro[] = {
    {"Purpose", "This link-only topic introduces the canonical help set and explains why the\nhelp system is split into concise contextual pages plus shared explainers."},
    {"Projection notes", "Long-form outputs may use this topic as the introduction to the generated help\nbundle without forcing every runtime `F1` page to repeat the same orientation\ntext."},
};

static const GeneratedHelpLink generated_help_links_navigation[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_navigation[] = {
    {"Baseline movement", "Navigation is shared vocabulary. Context-specific help should assume this\nbaseline and document only the keys, limits, and ownership changes that are\nspecial to that surface."},
    {"Common keys", "*   **Up/Down** move the active selection.\n*   **Page Up/Page Down** scroll by pages in list-oriented surfaces.\n*   **Home/End** jump to the start or end of the current list.\n*   **Enter** accepts the current selection or toggles between paired views\n    such as tree/file or preview on/off when that context owns Enter.\n*   **Esc** backs out of temporary overlays and prompt/dialog flows without\n    committing the pending action."},
};

static const GeneratedHelpLink generated_help_links_dir[] = {
    {"Navigation", "navigation"},
    {"Filter", "filter"},
    {"F8 split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_dir[] = {
    {"Directory commands", "*   **1..9 view**: Select the active panel's base directory/file view while\n    tree-focused. `1` resets to Name, `2` shows Attributes, `3` shows Owner,\n    `4` shows Times, `5`, `7`, `8`, and `9` change the file projection, `6`\n    toggles panel-wide row size units, `0` is unused, and `9` is a silent\n    no-op outside Git worktrees.\n*   **A** (Attributes): Open attributes submenu for directory metadata changes:\n    mode (chmod), owner, group, date.\n*   **C** (Copy): Copy the selected directory branch.\n*   **D** (Delete): Delete selected directory.\n*   **F** (Filter): Set file filter. Supports regex patterns (e.g., `*.c`),\n    exclusions (`-*.o`), attributes (`:r`, `:x`), dates (`>2023-01-01`), and\n    sizes (`>1M`).\n*   **G** (Global): Show all files across all logged volumes in one global\n    list.\n*   **I** (Invert Tags): Toggle tag state for files in the selected/current\n    directory scope.\n*   **J** (Compare): Open the compare submenu (directory, logged tree, or\n    external viewer). With `VI_KEYS=1`, use uppercase `J` for this action.\n*   **K** (volume): Open the volume picker.\n*   **L** (Log): Log a new directory or archive file. Logging an already logged\n    volume/path performs a fresh reload and reanchors selection at the volume\n    root.\n*   **M** (Makedir): Create a new directory.\n*   **N** (New File): Create a new empty file.\n*   **O** (Only tagged): Toggle tagged-only file-list view for the current\n    directory scope.\n*   **P** (Pipe, or **|**): Pipe the selected directory to a command (stdin).\n*   **Q** (Quit): Quit ytnova.\n*   **R** (Rename): Rename selected directory.\n*   **S** (Showall): Show all files in all directories of the current volume.\n*   **T** (Tag): Tag all files in the selected directory.\n*   **U** (Untag): Untag all files in the selected directory.\n*   **V** (MoveDir): Move the selected directory branch.\n*   **W** (Write): Export files in the selected directory to a command or file\n    using a formatting dialog (Raw, Framed, Page Break).\n*   **X** (eXecute): Execute a shell command. Leave `{}` unquoted; ytnova\n    replaces it with the current directory path and shell-quotes the expanded\n    path. Prompt **F1** also explains the tagged-file `^X` repeat path.\n*   **Z** (archive): Create an archive from the current selection. If one or\n    more files are tagged, ytnova archives the tagged files. If nothing is\n    tagged, ytnova archives the selected file or selected directory. Directory\n    sources are archived recursively. Supported destination suffixes: `.tar`,\n    `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`, `.tar.xz`/`.txz`, `.zip`.\n*   **/** (jump): Jump to a file or directory by name within the current list.\n*   **`** (Backtick): Toggle visibility of hidden dot-files and directories."},
    {"Tree navigation", "*   **Enter**: On logged directories, switch to File Mode (focus the file\n    window). On unlogged/not-yet-scanned directories, perform one-level\n    log/reveal (same behavior as `+`) and stay in Directory Mode.\n*   **-**: State-based collapse/release. First press collapses an expanded\n    node. Second press on a collapsed logged node evicts the file list (sets\n    `+` status) and marks the directory as Unlogged. At root, use `-` to\n    release logged contents.\n*   **Tree status marker**: Unlogged directories use `+` in the left status\n    margin column. Directory names do not carry a `+` suffix; an unlogged\n    directory may still show `/` when it has subdirectories.\n*   **Left Arrow**: If the selected directory is expanded, collapse it.\n    Otherwise move selection to its parent directory. Repeated `Left` keeps\n    ascending (and collapsing where needed). At filesystem root, `Left` is a\n    no-op.\n*   **Right Arrow** (Drill Down): Progressive depth navigation. If collapsed:\n    expand one level. If already expanded: move cursor to the first child. It\n    does not jump to siblings.\n*   **+** (or **=**): One-level log/reveal only (no cursor movement). `=` is a\n    convenience alias (unshifted `+` on most keyboards).\n*   **\\*** (Asterisk): Recursively expand the current directory and all its\n    subdirectories."},
};

static const GeneratedHelpLink generated_help_links_file[] = {
    {"Navigation", "navigation"},
    {"Output", "output"},
    {"F7 preview", "f7"},
};

static const GeneratedHelpLongFormSection generated_help_sections_file[] = {
    {"File commands", "*   **A** (Attributes): Open attributes submenu for selected file metadata:\n    mode, owner, group, date.\n*   **C** (Copy): Copy the selected file.\n*   **^K**: Copy all tagged files.\n*   **D** (Delete): Delete selected file. *(With `VI_KEYS=1`, use lowercase\n    `d` for this action and uppercase `D` for Delete Tagged.)*\n*   **E** (Edit): Edit selected file with `$EDITOR` (default: vi).\n*   **F** (Filter): Set file filter.\n*   **H** (Hex): View selected file in hex mode.\n*   **I** (Invert Tags): Toggle the tag state of all visible files.\n*   **J** (Compare): Compare the selected file with a target file.\n*   **L** (Log): Log a new directory or archive file. Logging an already logged\n    volume/path performs a fresh reload and reanchors selection at the volume\n    root.\n*   **M** (Move): Move the selected file.\n*   **^N**: Move all tagged files.\n*   **N** (New File): Create a new empty file.\n*   **O** (Only tagged): Toggle tagged-only file-list view (show tagged files\n    only).\n*   **P** (Pipe, or **|**): Pipe content of file to a command (stdin).\n*   **R** (Rename): Rename the selected file.\n*   **S** (Sort): Sort filelist (Access time, Change time, Extension, Group,\n    Modification time, Name, Owner, Size).\n*   **^S** (Search): Execute grep on tagged files. The prompt expects search\n    text, not a full grep command; ytnova builds `grep -i -- PATTERN {}`\n    internally and untags files that do not match. Prompt **F1** summarizes\n    the tagged-scope behavior.\n*   **T** (Tag): Tag selected file.\n*   **^T**: Tag all displayed files.\n*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`\n    for this action.)*\n*   **^U**: Untag all displayed files. *(With `VI_KEYS=1`, `^U` is page-up\n    navigation and uppercase `U` becomes Untag All.)*\n*   **V** (View): View file with the pager defined in the main config (default:\n    less).\n*   **^V**: **View Tagged**. View all tagged files sequentially.\n*   **W** (Write): Export the selected file to a command or file using a\n    formatting dialog (Raw, Framed, Page Break).\n*   **X** (eXecute): Execute a shell command. Leave `{}` unquoted; ytnova\n    replaces it with the selected file path and shell-quotes the expanded path.\n    Prompt **F1** also explains the tagged-file `^X` repeat path.\n*   **Y**: (Pathcopy): Copy selected file, replicating its directory structure\n    relative to the current volume root.\n*   **Z** (archive): Create an archive from tagged files, or from the selected\n    file/directory when nothing is tagged. Directory sources are archived\n    recursively."},
    {"File-window navigation", "*   **1 .. 4** (Base View): Select the file or directory base view for the\n    active panel: `1` Name, `2` Attributes, `3` Owner, `4` Times. Press `2`,\n    `3`, or `4` again to return to `1`.\n*   **5**: Toggle the compact Name/full-width file rendering variant when the\n    current base view is `1` / Name.\n*   **6**: Toggle binary vs human-readable size units for directory/file rows\n    on the active panel.\n*   **7**: Toggle Mini preview detail in the file window.\n*   **8**: Toggle File detail in the file window.\n*   **9**: Toggle the Git status band for filesystem file lists when the\n    current directory is inside a Git worktree.\n*   **0**: Currently unused; silent no-op.\n*   **Enter**: Switch to Full Screen File Mode / Directory Mode.\n*   **Left Arrow**: Move to the previous visible file column; in one-column\n    layouts this performs page-up navigation.\n*   **Right Arrow**: Move to the next visible file column; in one-column\n    layouts this performs page-down navigation.\n*   **Date Changes:** Date actions change Accessed time, Modified time, or both\n    (POSIX does not allow setting creation/birth time here)."},
};

static const GeneratedHelpLink generated_help_links_archive_dir[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_dir[] = {
    {"Archive directory commands", "*   **J** (Compare): Open compare flow. With `VI_KEYS=1`, use uppercase `J`\n    for this action.\n*   **D** (Delete): Delete selected archive directory entry.\n*   **F** (Filter): Set file filter.\n*   **G** (Global): Show all files across all logged volumes in one global\n    list.\n*   **I** (Invert Tags): Toggle tag state for files in the selected/current\n    archive directory scope.\n*   **L** (Log): Log a new directory or archive. Logging an already logged\n    volume/path performs a fresh reload and reanchors selection at the volume\n    root.\n*   **M** (Makedir): Create directory in archive context where supported.\n*   **O** (Only tagged): Toggle tagged-only file-list view for the current\n    archive directory scope.\n*   **R** (Rename): Rename selected archive directory entry.\n*   **S** (Showall): Show all files in the archive.\n*   **T** (Tag): Tag all files in current virtual directory.\n*   **U** (Untag): Untag all files in current virtual directory.\n*   **1 .. 4** (Dir Mode): Select the active panel's base archive-directory/\n    file view while tree-focused: `1` Name/reset, `2` Attributes, `3` Owner,\n    `4` Times. `5`, `7`, `8`, and `9` update the panel's file projection; `6`\n    toggles panel-wide row size units; `0` is unused; `9` is a silent no-op in\n    archives."},
    {"Archive directory navigation", "*   **Enter**: Switch to Archive-File Mode.\n*   **-**: State-based collapse/release. Expanded nodes collapse; collapsed\n    logged nodes (or logged leaves) unlog/release.\n*   **Left Arrow**: Collapse the current archive directory when expanded;\n    otherwise move selection to its parent directory.\n*   **Right Arrow** (Drill Down): Progressive depth navigation. If collapsed:\n    expand one level. If already expanded: move cursor to the first child.\n*   **+** (or **=**): Expand the current archive directory by one level.\n*   **\\\\**: At archive non-root, jump to archive root. At archive root, exit\n    to parent physical directory."},
};

static const GeneratedHelpLink generated_help_links_archive_file[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_file[] = {
    {"Archive file commands", "*   **C** (Copy): Copy selected file (including extract/copy paths).\n*   **^K** (Copy Tagged): Copy all tagged files.\n*   **D** (Delete): Delete selected archive file entry.\n*   **F** (Filter): Set file filter.\n*   **H** (Hex): View file in hex mode.\n*   **I** (Invert Tags): Toggle the tag state of all visible files.\n*   **M** (Move): Move selected file using archive-aware semantics.\n*   **O** (Only tagged): Toggle tagged-only file-list view (show tagged files\n    only).\n*   **P** (Pipe, or **|**): Pipe content to command.\n*   **R** (Rename): Rename selected archive file entry.\n*   **S** (Sort): Sort file list.\n*   **^S** (Search): Search tagged files for a string. The prompt expects\n    search text, not a full grep command; ytnova builds `grep -i -- PATTERN {}`\n    internally and untags files that do not match. Prompt **F1** summarizes\n    the tagged-scope behavior.\n*   **T** (Tag): Tag selected file.\n*   **^T**: Tag all files.\n*   **U** (Untag): Untag selected file. *(With `VI_KEYS=1`, use lowercase `u`\n    for this action.)*\n*   **^U**: Untag all files. *(With `VI_KEYS=1`, `^U` is page-up navigation and\n    uppercase `U` becomes Untag All.)*\n*   **V** (View): View file.\n*   **^V**: **View Tagged**. View all tagged files sequentially.\n*   **W** (Write): Export file content to a command or file.\n*   **Y** (Pathcopy): Copy selected file with relative path preservation."},
    {"Archive file navigation", "*   **1 .. 4** (Base View): Select the archive-file base view for the active\n    panel: `1` Name, `2` Attributes, `3` Owner, `4` Times. Press `2`, `3`, or\n    `4` again to return to `1`.\n*   **5**: Toggle the compact Name/full-width file rendering variant when the\n    current base view is `1` / Name.\n*   **6**: Toggle binary vs human-readable size units for archive rows.\n*   **7**: Toggle Mini preview detail in the file window.\n*   **8**: Toggle File detail in the file window.\n*   **9**: Silent no-op in archive file lists.\n*   **0**: Currently unused; silent no-op.\n*   **Enter**: Switch to Archive-Dir Mode.\n*   **\\\\**: No-op.\n*   Archive file-window status text uses `Unlogged` when the selected directory\n    is unlogged and `No files` when the selected directory is logged and empty."},
};

static const GeneratedHelpLink generated_help_links_filter[] = {
    {"Navigation", "navigation"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLongFormSection generated_help_sections_filter[] = {
    {"Filter syntax", "Use normal glob-like patterns such as `*.c`, comma-separated unions such as\n`*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as\nattributes (`:r`, `:x`), dates (`>2023-01-01`), or sizes (`>1M`). If the\nshell would expand the pattern, quote it before launching ytnova."},
    {"Scope rules", "Filter prompts stay scoped to the active file-list family. Directory/File,\narchive, Showall, and Global contexts may share syntax while still applying the\nresult to their own current scope and tagged/untagged conventions."},
};

static const GeneratedHelpLink generated_help_links_compare[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare[] = {
    {"Compare flows", "*   **File compare (`J` in File Mode):** Compare the selected file against a\n    target file. ytnova can use an external file-diff helper if configured.\n    *   `FILEDIFF` may use `%1` (source) and `%2` (target) placeholders; when\n        omitted, ytnova appends source and target paths to the helper command.\n*   **Directory compare (`J` in Directory Mode):**\n    *   `D`: compare the current directory.\n    *   `T`: compare the current logged tree.\n    *   `X`: launch an external directory/tree compare viewer.\n    *(With `VI_KEYS=1`, use uppercase `J` for this action.)*"},
    {"Compare rules", "*   Internal compare tags matches on the active/source side only.\n*   Logged-tree compare uses logged content only; it does not auto-log unopened\n    subdirectories.\n*   There is no separate \"compare tagged files\" mode."},
};

static const GeneratedHelpLink generated_help_links_compare_target[] = {
    {"Navigation", "navigation"},
    {"Compare overview", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_target[] = {
    {"Runtime scope", "This runtime-only topic keeps the compare-target popup concise while the shared\ncompare explainer continues to own the broader compare documentation bundle."},
};

static const GeneratedHelpLink generated_help_links_compare_scope[] = {
    {"Navigation", "navigation"},
    {"Compare overview", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_scope[] = {
    {"Runtime scope", "This runtime-only topic explains the compare-scope chooser without duplicating\nthe full compare documentation into prompt-local code."},
};

static const GeneratedHelpLink generated_help_links_compare_basis[] = {
    {"Navigation", "navigation"},
    {"Compare overview", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_basis[] = {
    {"Runtime scope", "This runtime-only topic keeps the compare-basis chooser generated-content\ndriven without widening the long-form command reference."},
};

static const GeneratedHelpLink generated_help_links_compare_results[] = {
    {"Navigation", "navigation"},
    {"Compare overview", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_results[] = {
    {"Runtime scope", "This runtime-only topic keeps the compare-result chooser generated-content\ndriven while the shared compare explainer owns the durable long-form docs."},
};

static const GeneratedHelpLink generated_help_links_output[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Archive file", "archive-file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output[] = {
    {"Output destinations", "Write/output flows may send content to a file path or to an external command.\nThe canonical prompt sequence explains the distinction between ordinary file\noutput and hardcopy-oriented command entry so the same authored text can serve\nfilesystem, archive, and prompt-local help."},
    {"Output formats", "The output dialog owns the format choices used by write/export flows, including\nRaw, Framed, and Page Break variants plus any separator prompt that follows.\nIf the runtime later narrows a contextual slice, the generated long-form docs\nmust still come from this one authored topic."},
};

static const GeneratedHelpLink generated_help_links_output_format[] = {
    {"Navigation", "navigation"},
    {"Output overview", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_format[] = {
    {"Runtime scope", "This runtime-only topic keeps the format chooser generated-content driven while\nthe shared output explainer owns the durable long-form docs."},
};

static const GeneratedHelpLink generated_help_links_output_destination[] = {
    {"Navigation", "navigation"},
    {"Output overview", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_destination[] = {
    {"Runtime scope", "This runtime-only topic keeps the destination chooser generated-content driven\nwithout duplicating prompt prose in print/output controllers."},
};

static const GeneratedHelpLink generated_help_links_output_separator[] = {
    {"Navigation", "navigation"},
    {"Output overview", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_separator[] = {
    {"Runtime scope", "This runtime-only topic keeps the separator prompt generated-content driven\nwhile the shared output explainer remains the canonical long-form reference."},
};

static const GeneratedHelpLink generated_help_links_showall[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Global", "global"},
};

static const GeneratedHelpLongFormSection generated_help_sections_showall[] = {
    {"Showall behavior", "Showall toggles file-list mode for all files in the current logged volume.\nPress **Esc** to return to the previously selected directory. Press **\\\\** to\njump to the owner directory of the selected file."},
    {"Scope notes", "Shared file-view commands still behave like ordinary file mode unless the\naggregated single-volume scope changes the ownership of the current result set."},
};

static const GeneratedHelpLink generated_help_links_global[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Showall", "showall"},
};

static const GeneratedHelpLongFormSection generated_help_sections_global[] = {
    {"Global behavior", "Global toggles file-list mode for all files across all logged volumes. Press\n**Esc** to return to the previously selected directory. Press **\\\\** to jump\nto the owner directory of the selected file."},
    {"Multi-volume scope", "Global shares the aggregated-file mental model with Showall but keeps room for\nmulti-volume caveats such as owner-directory jumps that cross volume roots."},
};

static const GeneratedHelpLink generated_help_links_f7[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f7[] = {
    {"Preview behavior", "File Preview Mode is activated by **F7**. The screen layout changes to show\nthe file list on the left (or active pane) and the file contents on the right.\nPress **F7** again to leave preview mode."},
    {"Preview controls", "*   Use **Up/Down**, **Page Up/Down**, and **Home/End** to move the selection\n    in the file list. The preview pane updates immediately.\n*   Use **Shift+Up/Down** (or **^P** / **^N**) to scroll the preview contents\n    line by line.\n*   Use **Shift+Page Up/Down** to scroll by pages.\n*   Use **Shift+Home/End** to jump to the beginning or end of the file."},
};

static const GeneratedHelpLink generated_help_links_f8[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f8[] = {
    {"Split behavior", "Split Screen Mode is activated by **F8**. The screen is divided vertically into\ntwo independent file manager panels. Press **F8** again to return to\nsingle-panel mode."},
    {"Split controls", "*   Press **Tab** to switch active control between the Left and Right panels.\n*   Copy, move, and compare prompts default to the inactive (passive) panel as\n    the destination/target when split mode is active.\n*   Split mode keeps panel-local selection, view, tag, and volume state\n    isolated so the passive panel remains a real target rather than a mirror."},
};

static const GeneratedHelpTopic generated_help_topics[] = {
    {
        "intro",
        "Intro",
        NULL,
        "YtreeNova keeps `F1` short and task-local. Use the contextual page for the\nactive surface, then follow shared explainer links only when you need more\nbackground.",
        3,
        generated_help_links_intro,
        2,
        generated_help_sections_intro,
    },
    {
        "navigation",
        "Navigation",
        NULL,
        "Arrow keys, paging keys, `Home`, `End`, and `Enter` keep their usual ownership.\nContextual pages explain only the extra keys or caveats that differ from the\nnormal navigation baseline.",
        4,
        generated_help_links_navigation,
        2,
        generated_help_sections_navigation,
    },
    {
        "dir",
        "Directory Help",
        "main.dir",
        "Directory Help is the directory-specific command page. Use Navigation for the\nshared movement keys; this page keeps the focus on directory actions,\ntree/logging behavior, and directory-only caveats.",
        3,
        generated_help_links_dir,
        2,
        generated_help_sections_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "File help explains the live file footer commands, file-view operations, and\nfile-specific caveats that are not obvious from the command strip alone.",
        3,
        generated_help_links_file,
        2,
        generated_help_sections_file,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Archive directory help mirrors the live archive-directory footer, then adds the\narchive-specific caveats that differ from normal filesystem directory behavior.",
        3,
        generated_help_links_archive_dir,
        2,
        generated_help_sections_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Archive file help mirrors the live archive-file footer and documents the\ndifferences between archive file actions and normal filesystem file actions.",
        3,
        generated_help_links_archive_file,
        2,
        generated_help_sections_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Use normal glob-like patterns such as `*.c`, comma-separated unions such as\n`*.c,*.h`, and exclusions such as `-*.o`.\nExtended selectors such as `:r`, `:x`, `>2023-01-01`, and `>1M` stay valid in\nthe runtime prompt, and an empty entry falls back to `*`.\nFilter prompts stay scoped to the active file-list family, including tagged\naggregates when the current prompt came from a tagged-only view.",
        3,
        generated_help_links_filter,
        2,
        generated_help_sections_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "Compare help is split into prompt-local runtime topics plus this shared long-form\nexplainer. Runtime `F1` pages stay focused on the active compare step, then link\nback here for the broader compare model.",
        3,
        generated_help_links_compare,
        2,
        generated_help_sections_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "The current file, directory, or logged tree is the compare source.\nEnter the target path directly, or use `F2` to browse and `Up` for prompt\nhistory.\nIn split view, the inactive panel seeds the default compare target.",
        2,
        generated_help_links_compare_target,
        1,
        generated_help_sections_compare_target,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        "prompt.compare-scope",
        "Directory only compares the current directory.\nLogged tree compares the current logged tree recursively and never auto-logs\nunopened `+` subdirectories.\nExternal viewer launches `DIRDIFF` or `TREEDIFF` helpers instead of tagging\nruntime compare results.",
        2,
        generated_help_links_compare_scope,
        1,
        generated_help_sections_compare_scope,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        "prompt.compare-basis",
        "Size checks file length and Date checks the last-modified time.\n`siZe+date` marks a difference when either size or modification time differs.\nHash opens both files and compares their content exactly, so it is slower than\nmetadata-only checks.",
        2,
        generated_help_links_compare_basis,
        1,
        generated_help_sections_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        "prompt.compare-results",
        "Choose which compare result to tag in the active/source-side file list.\n`diFferent` tags basis mismatches and `Unique` tags source-only entries.\nMatch, Newer, Older, Type-mismatch, and Error each tag only that one outcome.",
        2,
        generated_help_links_compare_results,
        1,
        generated_help_sections_compare_results,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output help is split into runtime prompt topics plus this shared long-form\nexplainer. Runtime `F1` pages stay focused on the active output step, then link\nback here for the durable write/export model.",
        3,
        generated_help_links_output,
        2,
        generated_help_sections_output,
    },
    {
        "output-format",
        "Output Format Help",
        "prompt.output-format",
        "Raw writes content without frame headings.\nFramed adds per-file heading/footer framing, and Page break inserts a separator\nbetween successive files without leaving a trailing separator at the end.\nChoose the format first; later prompts gather separators and destinations.",
        2,
        generated_help_links_output_format,
        1,
        generated_help_sections_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Destination chooses whether write/output goes to a file path or to an external\ncommand.\nWhen the prompt asks for the final target, enter either the destination file or\nthe command line exactly as you want it run.\nLeave the destination blank only to cancel and return without writing.",
        2,
        generated_help_links_output_destination,
        1,
        generated_help_sections_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "Framed and Page break modes prompt for a separator string before the\ndestination step.\nLeave the separator blank to accept the default triple-backtick fence.\nThe separator text is reused between files only for the selected framed/page\nformat; Raw output skips this prompt entirely.",
        2,
        generated_help_links_output_separator,
        1,
        generated_help_sections_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall help explains the single-volume aggregated file view and the commands\nor caveats that differ from ordinary file mode.\nPress `Esc` to return to the previously selected directory.\nPress `\\\\` to jump to the owner directory of the selected file inside the\ncurrent logged volume.",
        3,
        generated_help_links_showall,
        2,
        generated_help_sections_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global help explains the multi-volume aggregated file view, including how it\nreturns to owner directories and how its scope differs from ordinary file mode.\nPress `Esc` to return to the previously selected directory.\nPress `\\\\` to jump to the owner directory of the selected file even when that\nowner lives under a different logged volume root.",
        3,
        generated_help_links_global,
        2,
        generated_help_sections_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "F7 help explains preview ownership and how the preview overlay interacts with\nthe underlying directory or file context.\nUse `Shift+Up/Down` or `^P/^N` to scroll preview contents line by line.\nUse `Shift+PgUp/PgDn` for pages and `Shift+Home/End` to jump to the top or\nbottom of the current preview.",
        2,
        generated_help_links_f7,
        2,
        generated_help_sections_f7,
    },
    {
        "f8",
        "F8 Split Help",
        "overlay.f8-dir,overlay.f8-file",
        "F8 help explains split-view ownership, inactive-panel defaults, and the keys\nor caveats that only appear while split mode is active.\nPress `Tab` to switch the active panel while leaving the passive panel's state\nintact.\nCopy, move, and compare prompts default to the inactive panel as the\ndestination/target while split mode is active.",
        3,
        generated_help_links_f8,
        2,
        generated_help_sections_f8,
    },
};

static const size_t generated_help_topic_count = 20;
