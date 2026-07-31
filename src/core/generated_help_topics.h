/* Auto-generated from etc/help/f1.en.md by scripts/generate_help_assets.py. */
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
    {"Tagged", "tagged"},
    {"Shared commands", "shared-commands"},
    {"F2 picker", "f2-picker"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_intro[] = {
    {"Purpose", "Use `F1` for the thing you are doing now, not as one giant manual.\nShort local pages keep the first answer on screen. Shared topics carry the rules that repeat across more than one place."},
    {"Contents", "* **Navigation**: Learn the help-popup keys, list movement, and back/close rules.\n* **Tagged**: Learn how ytnova treats a tagged set as one working group.\n* **Shared commands**: Learn the function-key family that appears in more than one mode.\n* **Directory Help**: Learn tree-mode commands and directory-only rules.\n* **File Help**: Learn file-list commands, tagged actions, and file-only rules.\n* **Archive Directory Help**: Learn what changes inside an archive tree.\n* **Archive File Help**: Learn what changes inside an archive file list.\n* **Showall** and **Global**: Learn the aggregated file-list modes.\n* **F7 Preview** and **F8 Split**: Learn overlay-only controls and caveats.\n* **Filter**, **Compare**, and **Output**: Learn the option-heavy command families.\n* **Command-line Editing**, **VI Keys**, **F2 picker**, **F10 config**, and **Theming**: Learn the shared operator rules."},
};

static const GeneratedHelpLink generated_help_links_navigation[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"F2 picker", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_navigation[] = {
    {"Help popup keys", "* **Up/Down**: Move between selectable rows or links.\n* **Page Up/Page Down**: Scroll longer help pages.\n* **Home/End**: Jump to the top or bottom of the current help page.\n* **Enter/Right**: Open the selected help item or linked topic.\n* **Left**: Go back one step.\n* **Esc/Quit**: Close the popup."},
    {"Shared list movement", "* **Up/Down**: Move the active selection.\n* **Page Up/Page Down**: Move by pages.\n* **Home/End**: Jump to the start or end of the current list.\n* **Enter**: Accept the current row or switch between paired views when that surface owns `Enter`.\n* **Esc**: Cancel the current prompt, dialog, or overlay."},
};

static const GeneratedHelpLink generated_help_links_shared_commands[] = {
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"Applications menu", "applications-menu"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_shared_commands[] = {
    {"Shared function keys", "* **F1**: Open contextual help for the active surface.\n* **F5**: Refresh the current view.\n* **F6**: Change the stats/details presentation for the active view.\n* **F7**: Toggle preview for the active file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface.\n* **Esc**: Back out of the current overlay, prompt, or popup."},
};

static const GeneratedHelpLink generated_help_links_tagged[] = {
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Search tagged", "search-tagged"},
    {"Filter help", "filter"},
};

static const GeneratedHelpLongFormSection generated_help_sections_tagged[] = {
    {"Tagged basics", "Tags are a working set. They are not a second clipboard and not a saved search.\nYou can build a set, act on it, narrow it, then clear or invert it."},
    {"Common tagged flows", "* **Tag** and **Untag**: Add or remove the current row from the working set.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Only tagged**: Show only the tagged rows inside the current scope.\n* **Copy tagged** and **Move tagged**: Send the whole tagged set to one destination.\n* **View tagged**: Open the tagged files one after another.\n* **Search tagged**: Search only the tagged files, then untag non-matches.\n* **Archive**: Archive the tagged set first. When nothing is tagged, archive falls back to the current selection."},
};

static const GeneratedHelpLink generated_help_links_command_line_editing[] = {
    {"VI keys", "vi-keys"},
    {"F2 picker", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_command_line_editing[] = {
    {"Editing keys", "* **Left/Right**: Move inside the current prompt text.\n* **Home/End**: Jump to the start or end of the prompt text.\n* **Backspace/Delete**: Delete the character to the left or right of the cursor.\n* **Enter**: Accept the current value.\n* **Esc**: Cancel without committing the prompt."},
    {"Shared helpers", "* **Up**: Open or cycle prompt history when that prompt keeps history.\n* **F2**: Open a browser or picker when the current prompt supports browsing.\n* **F1**: Show syntax or scope rules that matter only to the current prompt."},
};

static const GeneratedHelpLink generated_help_links_vi_keys[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_vi_keys[] = {
    {"Navigation remap", "With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.\n`Ctrl-U` and `Ctrl-D` become page up and page down."},
    {"Command collisions", "Commands that would steal those lowercase keys move out of the way.\nExamples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist."},
};

static const GeneratedHelpLink generated_help_links_f10[] = {
    {"Theming", "theming"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f10[] = {
    {"Config surface", "Use `F10` when you want to change persistent behavior instead of doing one one-off file or directory action.\nProfile settings, command labels, themes, and reload all live here."},
    {"Related files", "`ytnova.conf` owns profile settings.\n`commands.conf` owns user command labels and bindings.\n`themes.conf` owns theme selection and theme-role overrides."},
};

static const GeneratedHelpLink generated_help_links_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_theming[] = {
    {"Theme model", "Themes set semantic roles such as `footer`, `help`, `help_link`, `selection`, `picker`, and `warning`.\nThis keeps one theme change consistent across the whole UI."},
    {"Editing path", "Use `F10` to open the theme or config editing path.\nKeep high-frequency navigation surfaces readable first: selection, picker, footer, and help."},
};

static const GeneratedHelpLink generated_help_links_dir[] = {
    {"Navigation", "navigation"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_dir[] = {
    {"Directory navigation", "* **Enter**: Open the file window. If the selected directory is still unlogged, `Enter` logs or reveals one level first.\n* **Collapse**: Collapse the current branch. Press `-` again on a collapsed logged node to release it and mark it unlogged.\n* **Left Arrow**: Collapse the current node or move to its parent.\n* **Right Arrow**: Expand one level first, then move to the first child.\n* **Plus**: Log or reveal one level without moving the cursor.\n* **Asterisk**: Recursively expand the selected directory and its subdirectories."},
    {"Directory commands", "* **1..9 view**: Change the active panel's directory and file presentation. `1` resets to the default Name view, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5`/`7`/`8`/`9` affect the file projection, and `6` toggles size units.\n* **Attributes**: Open the attributes submenu. Change mode, owner, group, or date.\n* **Copy**: Copy the selected directory branch.\n* **Delete**: Delete the selected directory.\n* **Filter**: Filter the current file-list scope. Use globs such as `*.c`, comma lists such as `*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as `:r`, `>2023-01-01`, or `>1M`.\n* **Global**: Show files from every logged volume in one list.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the current directory, the current logged tree, or an external viewer target.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file. Logging an already logged path reloads it from the top.\n* **Makedir**: Create a new directory.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only tagged files for the current scope without changing the tag state.\n* **Pipe**: Send the selected directory to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected directory.\n* **Showall**: Show every file inside the current logged volume.\n* **Tag**: Tag the files in the selected directory scope.\n* **Untag**: Untag the files in the selected directory scope.\n* **MoveDir**: Move the selected directory branch.\n* **Write**: Export the current selection to a file or command through the output prompts.\n* **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Archive**: Create an archive from the tagged set first, or from the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files and dot-directories."},
};

static const GeneratedHelpLink generated_help_links_file[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_file[] = {
    {"File navigation", "* **1..9 view**: Change the active panel's file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.\n* **Enter**: Switch between the embedded file window and full-screen file mode.\n* **Left Arrow**: Move to the previous visible file column. In one-column layouts it behaves like page up.\n* **Right Arrow**: Move to the next visible file column. In one-column layouts it behaves like page down."},
    {"File commands", "* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: Copy the selected file.\n* **Copy tagged**: Copy the tagged set to one destination.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current file-list scope with globs, exclusions, and extended selectors.\n* **Hex**: View the selected file in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected file against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file without leaving file mode.\n* **Move**: Move the selected file.\n* **Move tagged**: Move the tagged set to one destination.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only tagged files in the current scope without changing the tag state.\n* **Pipe**: Send the selected file to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Change the current file-list sort order.\n* **Tag**: Tag the selected file.\n* **Tag all**: Tag every visible file in the current scope.\n* **Untag**: Remove the tag from the selected file.\n* **Untag all**: Remove every tag in the current scope.\n* **View**: View the selected file with the configured pager.\n* **View tagged**: View the tagged files one after another.\n* **Write**: Export the selected file or tagged set through the output prompts.\n* **Execute**: Run a shell command. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely. `Ctrl-X` reruns the command for each tagged file.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.\n* **Search tagged**: Search only the tagged files, then untag files that do not match.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current scope."},
};

static const GeneratedHelpLink generated_help_links_archive_dir[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_dir[] = {
    {"Archive directory navigation", "* **Enter**: Switch to Archive File Mode for the selected archive directory.\n* **Left Arrow**: Collapse the current archive node or move to its parent.\n* **Right Arrow**: Expand one level first, then move to the first child.\n* **Root**: `\\` jumps to archive root when you are below it.\n* **Exit archive**: `\\` leaves the archive when you are already at archive root."},
    {"Archive directory commands", "* **1..9 view**: `1..4` choose the base archive directory/file view. `5`, `7`, and `8` still affect the paired file projection, `6` toggles row-size units, and `9` stays a no-op inside archives.\n* **Delete**: Delete the selected archive directory entry.\n* **Filter**: Filter the current archive-backed file-list scope.\n* **Global**: Show archive-backed results together with other logged volumes.\n* **Compare**: Compare the current archive directory or logged tree view.\n* **Volume**: Open the volume picker.\n* **Log**: Log another directory or archive file.\n* **Makedir**: Create a directory where the archive format supports it.\n* **Pipe**: Send the selected archive path to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive directory entry.\n* **Showall**: Show every file in the current archive.\n* **Tag**: Tag the files in the current virtual directory scope.\n* **Untag**: Untag the files in the current virtual directory scope.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden entries when the archive view exposes them."},
};

static const GeneratedHelpLink generated_help_links_archive_file[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Tagged", "tagged"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_file[] = {
    {"Archive file navigation", "* **1..9 view**: `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, and `8` toggles File detail. `9` stays a no-op inside archives.\n* **Enter**: Switch back to Archive Directory Mode.\n* **Jump**: Jump to a matching name in the current list."},
    {"Archive file commands", "* **Copy**: Copy the selected archive entry through archive-aware extract/copy paths.\n* **Copy tagged**: Copy the tagged archive entries to one destination.\n* **Delete**: Delete the selected archive entry.\n* **Filter**: Filter the current archive-backed file-list scope.\n* **Hex**: View the selected archive entry in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected archive entry against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log another directory or archive file.\n* **Move**: Move the selected archive entry through archive-aware paths.\n* **Move tagged**: Move the tagged archive entries to one destination.\n* **Pipe**: Send the selected archive entry to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive entry.\n* **Sort**: Change the current file-list sort order.\n* **Tag**: Tag the selected archive entry.\n* **Untag**: Remove the tag from the selected archive entry.\n* **View**: View the selected archive entry.\n* **View tagged**: View the tagged archive entries one after another.\n* **Pathcopy**: Copy the selected archive entry while keeping its relative path.\n* **Search tagged**: Search only the tagged archive entries, then untag non-matches.\n* **Execute**: Not available in archive file mode.\n* **Write**: Not available in archive file mode.\n* **Dotfiles**: Toggle hidden entries when the archive view exposes them."},
};

static const GeneratedHelpLink generated_help_links_filter[] = {
    {"Tagged", "tagged"},
    {"Showall", "showall"},
    {"Global", "global"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_filter[] = {
    {"Syntax", "Use normal glob-like patterns such as `*.c`, comma-separated unions such as `*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as `:r`, `:x`, `>2023-01-01`, or `>1M`.\nIf your shell would expand the pattern before ytnova sees it, quote it at the shell prompt."},
    {"Scope", "The filter always applies to the current file-list family.\nThat may be an ordinary file list, an archive file list, Showall, Global, or a tagged-only view built from one of those scopes."},
};

static const GeneratedHelpLink generated_help_links_compare[] = {
    {"File mode", "file"},
    {"Directory mode", "dir"},
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare[] = {
    {"Compare flow", "Choose the target first.\nThen choose the compare scope when the source is a directory.\nThen choose the compare basis when the runtime offers more than one basis.\nFinally choose which result class to tag on the source side."},
    {"Compare rules", "* Logged-tree compare uses logged content only. It does not auto-log unopened `+` subdirectories.\n* `FILEDIFF` may use `%1` and `%2`. When those placeholders are missing, ytnova appends source and target paths to the helper command.\n* External directory/tree compare launches `DIRDIFF` or `TREEDIFF` instead of tagging runtime results.\n* There is no separate compare-tagged-files mode."},
};

static const GeneratedHelpLink generated_help_links_compare_target[] = {
    {"Compare Help", "compare"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_target[] = {
    {"Target rules", "Enter one path.\nThe compare scope decides whether that one path is treated as a file target, a directory target, or a logged-tree target."},
};

static const GeneratedHelpLink generated_help_links_compare_scope[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_scope[] = {
    {"Scope choices", "Use `Directory` for one level.\nUse `Logged tree` for the currently logged recursive tree.\nUse `External viewer` when you want an external diff tool instead of tagged compare results inside ytnova."},
};

static const GeneratedHelpLink generated_help_links_compare_basis[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_basis[] = {
    {"Basis choices", "Choose the cheapest basis that answers the question you actually have.\nUse `Hash` only when metadata is not trustworthy enough."},
};

static const GeneratedHelpLink generated_help_links_compare_results[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_compare_results[] = {
    {"Result tagging", "The compare command never rewrites files.\nIt marks the chosen result class on the active/source side so you can inspect, copy, move, or archive that subset next."},
};

static const GeneratedHelpLink generated_help_links_execute_file[] = {
    {"File mode", "file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_execute_file[] = {
    {"Placeholder rules", "`{}` stands for one selected file path.\nWhen you use the tagged rerun path, the same command is repeated once per tagged file."},
};

static const GeneratedHelpLink generated_help_links_execute_dir[] = {
    {"Directory mode", "dir"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_execute_dir[] = {
    {"Placeholder rules", "`{}` stands for the current directory path.\nThe tagged rerun path still walks tagged files from the active list, not tagged directories from somewhere else."},
};

static const GeneratedHelpLink generated_help_links_search_tagged[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_search_tagged[] = {
    {"Tagged search rules", "Start by tagging a working set.\nThen search only that set. The result is another, narrower tagged set because files that do not match lose their tags."},
};

static const GeneratedHelpLink generated_help_links_create_archive[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_create_archive[] = {
    {"Archive creation rules", "Directory selections are archived recursively.\nArchive creation picks the tagged set first because tagging is the normal way to build a custom archive batch."},
};

static const GeneratedHelpLink generated_help_links_output[] = {
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output[] = {
    {"Output model", "`Write` is an export flow.\nIt can write plain content, framed content, or page-break-separated content.\nIt can also send that output to a command instead of a file path."},
    {"Output order", "Choose the format first.\nChoose the separator only when the format asks for one.\nThen choose the final destination."},
};

static const GeneratedHelpLink generated_help_links_output_format[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_format[] = {
    {"Format choices", "Use `Raw` when another tool will parse the output.\nUse `Framed` or `Page break` when a human will read the exported batch."},
};

static const GeneratedHelpLink generated_help_links_output_destination[] = {
    {"Output Help", "output"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_destination[] = {
    {"Destination choices", "A file destination writes exported text to a path.\nA command destination sends the exported text to the chosen command."},
};

static const GeneratedHelpLink generated_help_links_output_separator[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_output_separator[] = {
    {"Separator rules", "The separator is reused between files for the current framed or page-break export.\nIt is not appended after the last file."},
};

static const GeneratedHelpLink generated_help_links_showall[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Tagged", "tagged"},
};

static const GeneratedHelpLongFormSection generated_help_sections_showall[] = {
    {"Showall behavior", "* **Scope**: Showall lists every file inside the current logged volume only.\n* **Esc**: Return to the previously selected directory.\n* **Open owner**: `\\` jumps to the owner directory of the selected file inside the current logged volume.\n* **Sort**: Repeating `S` changes sort; it does not leave Showall.\n* **Filter**: Filter still applies only to the current Showall result set.\n* **Only tagged**: Show only the tagged rows from the current Showall result set."},
};

static const GeneratedHelpLink generated_help_links_global[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
    {"Tagged", "tagged"},
};

static const GeneratedHelpLongFormSection generated_help_sections_global[] = {
    {"Global behavior", "* **Scope**: Global lists files from every logged volume.\n* **Esc**: Return to the previously selected directory.\n* **Open owner**: `\\` jumps to the owner directory of the selected file even when it lives under another logged volume root.\n* **Global**: Repeating `G` is a no-op while you are already in Global.\n* **Filter**: Filter still applies only to the current Global result set.\n* **Only tagged**: Show only the tagged rows from the current Global result set."},
};

static const GeneratedHelpLink generated_help_links_f7[] = {
    {"Navigation", "navigation"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f7[] = {
    {"Preview controls", "* **Leave preview**: Press `F7` again to return to the underlying directory or file context.\n* **Move selection**: Use `Up`, `Down`, `Page Up`, `Page Down`, `Home`, and `End` to move the selected file while the preview updates.\n* **Scroll preview**: Use `Shift-Up` and `Shift-Down`, or `Ctrl-P` and `Ctrl-N`, to scroll preview text line by line.\n* **Page preview**: Use `Shift-PgUp` and `Shift-PgDn` to scroll preview text by pages.\n* **Jump in preview**: Use `Shift-Home` and `Shift-End` to jump to the start or end of the preview."},
};

static const GeneratedHelpLink generated_help_links_f8[] = {
    {"Navigation", "navigation"},
    {"Directory mode", "dir"},
    {"File mode", "file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f8[] = {
    {"Split controls", "* **Leave split**: Press `F8` again to return to single-panel mode.\n* **Tab**: Switch the active panel and keep the passive panel's state intact.\n* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.\n* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state.\n* **Overlay ownership**: Split help shows only split-specific rules. Ordinary directory and file commands stay on their own pages."},
};

static const GeneratedHelpLink generated_help_links_history_dialog[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_history_dialog[] = {
    {"History actions", "* **Select entry**: `Up` and `Down` move through the current history list.\n* **Scroll long entry**: `Left` and `Right` shift a long history line horizontally.\n* **Pin**: `P` keeps an important entry at the top of the current history list.\n* **Delete**: `D` removes the selected entry from the current history list.\n* **Accept**: `Enter` reuses the selected entry.\n* **Cancel**: `Esc` closes the dialog without reusing an entry."},
};

static const GeneratedHelpLink generated_help_links_volume_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_volume_menu[] = {
    {"Volume actions", "* **Select volume**: `Up` and `Down` move through the loaded-volume list.\n* **Switch volume**: `Enter` activates the selected volume.\n* **Keep state**: Selecting the already active volume keeps its current in-memory state.\n* **Release volume**: `D` unloads the selected volume unless it is the last remaining one.\n* **Cancel**: `Esc` closes the menu."},
};

static const GeneratedHelpLink generated_help_links_applications_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_applications_menu[] = {
    {"Applications actions", "* **Select preset**: `Up` and `Down` move through the preset list.\n* **Accept preset**: `Enter` accepts the selected row.\n* **Cancel**: `Esc` closes the menu.\n* **Current state**: The shipped Applications menu is still a lightweight placeholder surface, so treat it as a shell for future app-launch behavior."},
};

static const GeneratedHelpLink generated_help_links_f2_picker[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f2_picker[] = {
    {"Picker actions", "* **Move**: `Up` and `Down` move through the visible directory rows.\n* **Expand**: `Right` expands the current directory one level, then moves into the first child when that level is already open.\n* **Collapse**: `Left` collapses the current directory, or moves to its parent when the current row is already closed.\n* **Accept**: `Enter` uses the highlighted directory for the calling prompt.\n* **Cancel**: `Esc` closes the picker without changing the prompt."},
};

static const GeneratedHelpTopic generated_help_topics[] = {
    {
        "intro",
        "Contents",
        NULL,
        "Start with the page for the screen or prompt you are using now.\nUse shared topics only when you need the wider rule behind a command.\nUse `Navigation` if you forget the popup keys.",
        5,
        generated_help_links_intro,
        2,
        generated_help_sections_intro,
    },
    {
        "navigation",
        "Navigation",
        NULL,
        "Use `Up` and `Down` to move.\nUse `Enter` or `Right` to follow the selected help link.\nUse `Left` to go back, and `Esc` or `Q` to close help.",
        5,
        generated_help_links_navigation,
        2,
        generated_help_sections_navigation,
    },
    {
        "shared-commands",
        "Shared Commands",
        NULL,
        "These keys keep the same high-level meaning across more than one mode.\nUse the mode page for local commands, and use this page for the shared function-key family.",
        4,
        generated_help_links_shared_commands,
        1,
        generated_help_sections_shared_commands,
    },
    {
        "tagged",
        "Tagged",
        NULL,
        "Tags let you mark more than one file, then run one command against that set.\nWhen tags exist, copy, move, view, archive, and search often work on the tagged set instead of only the current row.\n`Only tagged` turns that set into its own temporary list without changing the tags themselves.",
        4,
        generated_help_links_tagged,
        2,
        generated_help_sections_tagged,
    },
    {
        "command-line-editing",
        "Command-line Editing",
        NULL,
        "Most prompts share the same editing keys.\nLearn them once here, then use the prompt page only for syntax, defaults, and scope.",
        2,
        generated_help_links_command_line_editing,
        2,
        generated_help_sections_command_line_editing,
    },
    {
        "vi-keys",
        "VI Keys",
        NULL,
        "`VI_KEYS=1` keeps lowercase vi navigation available.\nCommands that would collide move to uppercase or another safe key.",
        2,
        generated_help_links_vi_keys,
        2,
        generated_help_sections_vi_keys,
    },
    {
        "f10",
        "F10 Config Help",
        NULL,
        "Use `F10` for configuration work.\nThat is where profile, commands, themes, reload, and similar setup actions belong.",
        2,
        generated_help_links_f10,
        2,
        generated_help_sections_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes style semantic roles, not one-off screen positions.\nThat keeps help, picker, selection, footer, and warning surfaces readable as one system.",
        1,
        generated_help_links_theming,
        2,
        generated_help_sections_theming,
    },
    {
        "dir",
        "Directory Help",
        "main.dir",
        "Directory Help is the tree-mode command list.\nPick a command for the short meaning, then press `Enter` for the fuller rule.",
        2,
        generated_help_links_dir,
        2,
        generated_help_sections_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "File Help is the main file-list command list.\nPick a command for the short meaning, then press `Enter` for the fuller rule.",
        3,
        generated_help_links_file,
        2,
        generated_help_sections_file,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Archive Directory Help is the tree-style list for paths inside an archive.\nPick a command for the short meaning, then press `Enter` for the fuller rule.",
        2,
        generated_help_links_archive_dir,
        2,
        generated_help_sections_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Archive File Help is the file-list command list for archive-backed content.\nPick a command for the short meaning, then press `Enter` for the fuller rule.",
        3,
        generated_help_links_archive_file,
        2,
        generated_help_sections_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Type a glob such as `*.c`, a comma list such as `*.c,*.h`, or an exclusion such as `-*.o`.\nUse `:r` and `:x` for attribute tests, `>2023-01-01` for date tests, and `>1M` for size tests.\nLeave the prompt empty to fall back to `*`.",
        4,
        generated_help_links_filter,
        2,
        generated_help_sections_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "Compare starts from the current file, directory, or logged tree on the active panel.\nFile compare checks one file against one target.\nDirectory compare can compare the current directory, the current logged tree, or an external viewer target.\nInternal compare tags results on the source side only.",
        3,
        generated_help_links_compare,
        2,
        generated_help_sections_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "The current file, directory, or logged tree is the compare source.\nEnter the target path directly, use `F2` to browse, or use `Up` for history.\nIn split view, the inactive panel seeds the default compare target.",
        2,
        generated_help_links_compare_target,
        1,
        generated_help_sections_compare_target,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        "prompt.compare-scope",
        "Directory only compares the current directory.\nLogged tree compares the current logged tree recursively and never auto-logs unopened branches.\nExternal viewer launches the configured external compare helper instead of tagging runtime results.",
        1,
        generated_help_links_compare_scope,
        1,
        generated_help_sections_compare_scope,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        "prompt.compare-basis",
        "Size checks file length.\nDate checks the last-modified time.\nsiZe+date treats either difference as a mismatch.\nHash opens both files and compares their content exactly, so it is slower.",
        1,
        generated_help_links_compare_basis,
        1,
        generated_help_sections_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        "prompt.compare-results",
        "Choose which compare result to tag in the source-side file list.\ndiFferent tags basis mismatches, and Unique tags source-only entries.\nMatch, Newer, Older, Type mismatch, and Error each tag only that one class.",
        1,
        generated_help_links_compare_results,
        1,
        generated_help_sections_compare_results,
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "Use `{}` where the selected file path should be inserted.\nLeave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\nUse `Ctrl-X` to rerun the command for each tagged file.",
        2,
        generated_help_links_execute_file,
        1,
        generated_help_sections_execute_file,
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "Use `{}` where the current directory path should be inserted.\nLeave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\nUse `Ctrl-X` to rerun the command for each tagged file in the active list.",
        2,
        generated_help_links_execute_dir,
        1,
        generated_help_sections_execute_dir,
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Enter plain search text only.\nytnova builds `grep -i -- PATTERN {}` for you.\nOnly tagged files are searched, and non-matches are untagged.",
        2,
        generated_help_links_search_tagged,
        1,
        generated_help_sections_search_tagged,
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Use `.tar`, `.tar.gz` or `.tgz`, `.tar.bz2` or `.tbz2`, `.tar.xz` or `.txz`, or `.zip`.\nWhen tags exist, the tagged set wins.\nWhen nothing is tagged, ytnova archives the current file or directory selection.",
        2,
        generated_help_links_create_archive,
        1,
        generated_help_sections_create_archive,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output sends file content either to a destination file or to an external command.\nThe prompts first choose a format, then a separator when that format needs one, then the final destination.\nUse `Write` when you want exported content, not in-place editing.",
        3,
        generated_help_links_output,
        2,
        generated_help_sections_output,
    },
    {
        "output-format",
        "Output Format Help",
        "prompt.output-format",
        "`Raw` writes content with no extra framing.\n`Framed` adds per-file headings or footers.\n`Page break` inserts a separator between files and skips a trailing separator at the end.",
        1,
        generated_help_links_output_format,
        1,
        generated_help_sections_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Enter the final destination exactly as you want it used.\nThat may be a file path or a command line, depending on the earlier choice.\nLeave it blank only to cancel and return without writing.",
        2,
        generated_help_links_output_destination,
        1,
        generated_help_sections_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "This prompt appears only for formats that need a separator.\nLeave it blank to accept the default triple-backtick fence.\nRaw output skips this prompt.",
        1,
        generated_help_links_output_separator,
        1,
        generated_help_sections_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall Help is the single-volume aggregated file-list page.\nPick a line for the short meaning, then press `Enter` for the fuller rule.",
        3,
        generated_help_links_showall,
        1,
        generated_help_sections_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global Help is the multi-volume aggregated file-list page.\nPick a line for the short meaning, then press `Enter` for the fuller rule.",
        3,
        generated_help_links_global,
        1,
        generated_help_sections_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "F7 Preview Help is the preview command list.\nPick a line for the short meaning, then press `Enter` for the fuller rule.",
        2,
        generated_help_links_f7,
        1,
        generated_help_sections_f7,
    },
    {
        "f8",
        "F8 Split Help",
        "overlay.f8-dir,overlay.f8-file",
        "F8 Split Help is the split-mode command list.\nPick a line for the short meaning, then press `Enter` for the fuller rule.",
        3,
        generated_help_links_f8,
        1,
        generated_help_sections_f8,
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Use `Up` and `Down` to choose an entry.\nUse `Left` and `Right` to scroll a long entry.\nUse `P` to pin or unpin, `D` to delete, `Enter` to accept, and `Esc` to cancel.",
        1,
        generated_help_links_history_dialog,
        1,
        generated_help_sections_history_dialog,
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Use `Up` and `Down` to choose a loaded volume.\nUse `Enter` to switch to it.\nUse `D` to release it, unless it is the last one.\nUse `Esc` to leave the menu.",
        1,
        generated_help_links_volume_menu,
        1,
        generated_help_sections_volume_menu,
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Use `Up` and `Down` to choose an application preset.\nUse `Enter` to accept it.\nUse `Esc` to leave the menu.\nThe shipped menu is still a placeholder surface.",
        1,
        generated_help_links_applications_menu,
        1,
        generated_help_sections_applications_menu,
    },
    {
        "f2-picker",
        "F2 Picker Help",
        "dialog.f2-picker",
        "Use `Up` and `Down` to move.\nUse `Right` to expand or enter the first child, and `Left` to collapse or go to the parent.\nUse `Enter` to accept the highlighted directory, and `Esc` to cancel.",
        2,
        generated_help_links_f2_picker,
        1,
        generated_help_sections_f2_picker,
    },
};

static const size_t generated_help_topic_count = 34;
