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
    {"Shared commands", "shared-commands"},
    {"F10 config", "f10"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_intro[] = {
    {"Purpose", "This link-only topic introduces the canonical help set and explains why the\nhelp system is split into concise contextual pages plus shared explainers."},
    {"Contents", "*   Start with **Navigation** for the shared movement baseline.\n*   Use **Shared commands** for the cross-context function-key family,\n    including **F10 config**.\n*   Use **Directory mode**, **File mode**, **Archive directory**, **Archive\n    file**, **Showall**, **Global**, **F7 preview**, and **F8 split** for\n    context-specific command pages.\n*   Use **Filter**, **Compare overview**, and **Output overview** when one\n    command family needs more detail than a one-line definition can hold.\n*   Use **Command-line editing**, **VI keys**, **Theming**, and **F10\n    config** for shared operator/reference topics that apply across multiple\n    prompts or configuration surfaces."},
};

static const GeneratedHelpLink generated_help_links_navigation[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"Shared commands", "shared-commands"},
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_navigation[] = {
    {"Help popup navigation", "*   **Up/Down**: Move between help popup links when the current page offers a\n    selectable list.\n*   **Page Up/Page Down**: Scroll longer help pages.\n*   **Home/End**: Jump to the start or end of the current help page.\n*   **Enter/Right**: Follow the selected help popup link.\n*   **Left Arrow**: Go back one help page when you followed a link.\n*   **Esc/Quit**: Close the help popup from anywhere."},
    {"Shared YtreeNova navigation", "*   **Up/Down**: Move the active selection.\n*   **Page Up/Page Down**: Scroll by pages in list-oriented surfaces.\n*   **Home/End**: Jump to the start or end of the current list.\n*   **Enter**: Accept the current selection or toggle between paired views\n    when that surface owns Enter.\n*   **Esc**: Back out of temporary overlays and prompts without committing the\n    pending action."},
};

static const GeneratedHelpLink generated_help_links_shared_commands[] = {
    {"Navigation", "navigation"},
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_shared_commands[] = {
    {"Shared commands", "*   **F1** (help): Open contextual help for the active surface.\n*   **F5** (refresh): Refresh the current view.\n*   **F6** (stats): Switch the stats/details presentation for the active view.\n*   **F7** (autoview): Toggle preview/autoview for the active file context.\n*   **F8** (split): Toggle split-screen mode.\n*   **F9** (apps): Open the applications menu shell.\n*   **F10** (config): Open the configuration command surface.\n*   **Esc** (cancel): Close the current help popup or cancel the active\n    overlay/prompt."},
};

static const GeneratedHelpLink generated_help_links_command_line_editing[] = {
    {"Navigation", "navigation"},
    {"VI keys", "vi-keys"},
};

static const GeneratedHelpLongFormSection generated_help_sections_command_line_editing[] = {
    {"Editing keys", "*   **Left/Right** move within the current prompt buffer.\n*   **Home/End** jump to the start or end of the current prompt buffer.\n*   **Backspace/Delete** erase the character to the left/right of the cursor.\n*   **Enter** accepts the current prompt value.\n*   **Esc** cancels the prompt without committing it."},
    {"Shared helpers", "*   **Up** opens or cycles prompt history when that prompt keeps history.\n*   **F2** opens a browser/picker when the active prompt supports browsing a\n    path or reusable choice list.\n*   Prompt-local `F1` explains only syntax and scope that are specific to that\n    prompt; it should not re-teach the shared editing baseline."},
};

static const GeneratedHelpLink generated_help_links_vi_keys[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_vi_keys[] = {
    {"Navigation remap", "With `VI_KEYS=1`, lowercase **h/j/k/l** become Left/Down/Up/Right and **^U**\n/**^D** become page-up/page-down."},
    {"Command collisions", "Commands that would collide with lowercase vi navigation move to uppercase or a\nnon-conflicting fallback. Examples include **J** for Compare, **K** for Volume\nMenu, **D** for Delete Tagged, and **U** for Untag All where applicable."},
};

static const GeneratedHelpLink generated_help_links_f10[] = {
    {"Shared commands", "shared-commands"},
    {"Theming", "theming"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f10[] = {
    {"Config surface", "Use **F10** to reach configuration-oriented commands instead of treating them\nas per-directory actions. Persistent changes belong here, not in the active\nfile or directory command pages."},
    {"Related areas", "Theme selection, semantic colors, and presentation tweaks are covered by\n**Theming**. Prompt-edit/history behavior that appears inside config flows is\nstill owned by **Command-line Editing**."},
};

static const GeneratedHelpLink generated_help_links_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_theming[] = {
    {"Theme model", "Themes are role-based: users configure semantic roles rather than styling each\nsurface with ad-hoc colors. Help popups, pickers, and the footer command strip\neach have their own dedicated roles."},
    {"Editing path", "Use **F10** and the theme/config files to change theme selection or role\ndefinitions. Keep contrasts readable for help, picker, and selection surfaces;\nthose are high-frequency navigation aids."},
};

static const GeneratedHelpLink generated_help_links_dir[] = {
    {"Navigation", "navigation"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_dir[] = {
    {"Directory navigation", "*   **Enter**: Open the file window, or log/reveal one level when the selected\n    directory is still unlogged. Logged directories switch to File Mode.\n*   **Collapse**: Collapse or release the selected directory. `-` first\n    collapses an expanded node; pressing it again on a collapsed logged node\n    evicts the file list and marks that node unlogged.\n*   **Tree marker**: Show logged state in the left margin. Unlogged\n    directories use `+`; directory names themselves do not gain a `+` suffix.\n*   **Left Arrow**: Collapse the current node or move selection to its parent.\n    At filesystem root, Left is a no-op.\n*   **Right Arrow**: Expand one level or move to the first child. It does not\n    jump across siblings.\n*   **Plus**: Log or reveal one level without moving the cursor. `=` is the\n    unshifted alias on many keyboards.\n*   **Asterisk**: Recursively expand the selected directory and its\n    subdirectories."},
    {"Directory commands", "*   **1..9 view**: Select the active panel's base directory and file view. `1`\n    resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times,\n    `5`, `7`, `8`, and `9` change the file projection, `6` toggles panel-wide\n    row size units, and `9` is a silent no-op outside Git worktrees.\n*   **Attributes**: Open the attributes submenu. Change mode (chmod), owner,\n    group, or date.\n*   **Copy**: Copy the selected directory branch.\n*   **Delete**: Delete the selected directory.\n*   **Filter**: Set file filter. Supports patterns such as `*.c`, exclusions\n    such as `-*.o`, attributes such as `:r` and `:x`, dates such as\n    `>2023-01-01`, and sizes such as `>1M`.\n*   **Global**: Show all files across all logged volumes in one list.\n*   **Invert Tags**: Toggle tag state for the current directory scope.\n*   **Compare**: Open the compare submenu. Choose directory, logged-tree, or\n    external-viewer compare modes. With `VI_KEYS=1`, use uppercase `J`.\n*   **Volume**: Open the volume picker.\n*   **Log**: Log a new directory or archive file. Logging an already logged\n    path performs a fresh reload and reanchors selection at the volume root.\n*   **Makedir**: Create a new directory.\n*   **New File**: Create a new empty file.\n*   **Only tagged**: Toggle tagged-only view for the current directory scope.\n*   **Pipe**: Pipe the selected directory to a command on stdin. `|` is the\n    alternate key.\n*   **Quit**: Quit ytnova.\n*   **Rename**: Rename the selected directory.\n*   **Showall**: Show all files in all directories of the current volume.\n*   **Tag**: Tag all files in the selected directory.\n*   **Untag**: Untag all files in the selected directory.\n*   **MoveDir**: Move the selected directory branch.\n*   **Write**: Export files in the selected directory to a command or file.\n    The formatter dialog offers Raw, Framed, and Page Break output.\n*   **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand\n    it to the current directory path and shell-quote the result. Prompt `F1`\n    also explains the tagged-file `^X` repeat path.\n*   **Archive**: Create an archive from the current selection. Tagged files win;\n    otherwise ytnova archives the selected file or directory recursively.\n    Supported suffixes are `.tar`, `.tar.gz`/`.tgz`, `.tar.bz2`/`.tbz2`,\n    `.tar.xz`/`.txz`, and `.zip`.\n*   **Jump**: Jump to a file or directory name in the current list.\n*   **Dotfiles**: Toggle hidden dot-files and dot-directories."},
};

static const GeneratedHelpLink generated_help_links_file[] = {
    {"Navigation", "navigation"},
    {"Output", "output"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_file[] = {
    {"File-window navigation", "*   **1..9 view**: Select the active panel's file view. `1` resets to Name,\n    `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles\n    Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles\n    File detail, and `9` toggles the Git band inside Git worktrees.\n*   **Enter**: Switch between the file window and full-screen file mode.\n*   **Left Arrow**: Move to the previous visible file column. In one-column\n    layouts, Left performs page-up navigation.\n*   **Right Arrow**: Move to the next visible file column. In one-column\n    layouts, Right performs page-down navigation.\n*   **Date changes**: Date actions change Accessed time, Modified time, or\n    both. POSIX does not offer creation/birth time updates here."},
    {"File commands", "*   **Attributes**: Open the file attributes submenu. Change mode, owner,\n    group, or date.\n*   **Copy**: Copy the selected file.\n*   **Pathcopy**: Copy the selected file while preserving its path relative to\n    the current volume root.\n*   **Copy tagged**: Copy all tagged files.\n*   **Delete**: Delete the selected file. With `VI_KEYS=1`, lowercase `d`\n    keeps this action and uppercase `D` becomes Delete Tagged.\n*   **Edit**: Edit the selected file with `$EDITOR`. The default editor is\n    `vi`.\n*   **Filter**: Set file filter.\n*   **Hex**: View the selected file in hex mode.\n*   **Invert Tags**: Toggle the tag state of all visible files.\n*   **Compare**: Compare the selected file with a target file.\n*   **Volume**: Open the volume picker.\n*   **Log**: Log a new directory or archive file. Logging an already logged\n    path performs a fresh reload and reanchors selection at the volume root.\n*   **Move**: Move the selected file.\n*   **Move tagged**: Move all tagged files.\n*   **New File**: Create a new empty file.\n*   **Only tagged**: Toggle tagged-only file view.\n*   **Pipe**: Pipe the selected file to a command on stdin. `|` is the\n    alternate key.\n*   **Rename**: Rename the selected file.\n*   **Sort**: Sort the file list. Choose Access time, Change time, Extension,\n    Group, Modification time, Name, Owner, or Size.\n*   **Search tagged**: Search tagged files with grep. The prompt expects plain\n    search text, builds `grep -i -- PATTERN {}` internally, and untags files\n    that do not match.\n*   **Tag**: Tag the selected file.\n*   **Tag all**: Tag all displayed files.\n*   **Untag**: Untag the selected file. With `VI_KEYS=1`, lowercase `u` keeps\n    this action.\n*   **Untag all**: Untag all displayed files. With `VI_KEYS=1`, `^U` stays\n    page-up navigation and uppercase `U` becomes Untag All.\n*   **View**: View the selected file with the configured pager. The default is\n    View file with the pager defined in the main config. The default is\n    `less`.\n*   **View tagged**: View all tagged files sequentially.\n*   **Write**: Export the selected file to a command or file. The formatter\n    dialog offers Raw, Framed, and Page Break output.\n*   **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand\n    it to the selected file path and shell-quote the result. Prompt `F1` also\n    explains the tagged-file `^X` repeat path.\n*   **Archive**: Create an archive from tagged files, or from the selected\n    file/directory when nothing is tagged. Directory sources are archived\n    recursively."},
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
    {"Command-line editing", "command-line-editing"},
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
        "Contents",
        NULL,
        "YtreeNova keeps `F1` short and task-local. Use the contextual page for the\nactive surface, then follow shared explainer links only when you need more\nbackground.",
        4,
        generated_help_links_intro,
        2,
        generated_help_sections_intro,
    },
    {
        "navigation",
        "Navigation",
        NULL,
        "This page keeps help-popup navigation distinct from ordinary YtreeNova\nnavigation. Learn the shared popup keys once here, then return to the active\ncontext page for tree-only or file-only movement.\nArrow keys, paging keys, `Home`, `End`, and `Enter` keep their usual\nownership.",
        5,
        generated_help_links_navigation,
        2,
        generated_help_sections_navigation,
    },
    {
        "shared-commands",
        "Shared Commands",
        NULL,
        "Shared Commands explains the cross-context help keys and overlays that can\nappear from multiple main views. Use it for the shared function-key family\n(`F1`, `F5`, `F6`, `F7`, `F8`, `F9`, `F10`) instead of repeating those hints\non every context page.",
        4,
        generated_help_links_shared_commands,
        1,
        generated_help_sections_shared_commands,
    },
    {
        "command-line-editing",
        "Command-line Editing",
        NULL,
        "Prompt editing is shared across filter, compare, output, shell-command, and\npath-entry prompts. Learn it once here instead of re-reading it in every\nprompt-local page.",
        2,
        generated_help_links_command_line_editing,
        2,
        generated_help_sections_command_line_editing,
    },
    {
        "vi-keys",
        "VI Keys",
        NULL,
        "`VI_KEYS=1` changes command ownership to preserve lowercase vi-style\nnavigation. This explainer keeps the mode shift separate from ordinary\nnavigation help so users do not mix the two models.",
        2,
        generated_help_links_vi_keys,
        2,
        generated_help_sections_vi_keys,
    },
    {
        "f10",
        "F10 Config Help",
        NULL,
        "F10 opens the configuration command surface for profile, commands, themes, and\nother persistent setup changes. Use this page for the high-level map, then\nfollow Theming when the change is color/layout specific.",
        2,
        generated_help_links_f10,
        2,
        generated_help_sections_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes control semantic UI roles such as footer, picker, help, selection, and\nseverity colors. This topic keeps color-system explanation separate from the\nday-to-day command pages.",
        1,
        generated_help_links_theming,
        2,
        generated_help_sections_theming,
    },
    {
        "dir",
        "Directory Help",
        "main.dir",
        "Directory Help keeps the focus on directory commands plus tree-only navigation.\nUse the shared Navigation page for popup controls and generic movement you only\nneed to learn once.",
        2,
        generated_help_links_dir,
        2,
        generated_help_sections_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "File Help keeps the focus on file commands plus file-window navigation. Use the\nshared Navigation page for popup controls and generic movement you only need to\nlearn once.",
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
        4,
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

static const size_t generated_help_topic_count = 25;
