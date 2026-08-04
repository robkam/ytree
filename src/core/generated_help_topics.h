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
    {"Contents", "* **Navigation**: Learn the help-popup keys, list movement, and back/close rules.\n* **Tagged**: Learn how ytnova treats a tagged set as one working group.\n* **Shared commands**: Learn the function-key family that appears in more than one mode.\n* **Directory Help**: Learn tree-mode commands and directory-only rules.\n* **File Help**: Learn file-list commands, tagged actions, and file-only rules.\n* **Archive Directory Help**: Learn what changes inside an archive tree.\n* **Archive File Help**: Learn what changes inside an archive file list.\n* **Showall** and **Global**: Learn the aggregated file-list modes.\n* **F7 Preview** and **F8 Split**: Learn overlay-only controls and caveats.\n* **List Jump** and **Copy/Move Targets**: Learn the shared `/` jump model plus destination and wildcard rename rules.\n* **Filter**, **Compare**, and **Output**: Learn the option-heavy command families.\n* **Command-line Editing**, **VI Keys**, **F2 picker**, **F10 config**, and **Theming**: Learn the shared operator rules."},
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
    {"Scope boundary", "This topic owns help-popup movement only.\nUse `List Jump` for runtime `/` name-jump behavior, and use the local mode page for ordinary tree/file selection commands."},
};

static const GeneratedHelpLink generated_help_links_list_jump[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLongFormSection generated_help_sections_list_jump[] = {
    {"Jump model", "`/` starts an in-place jump prompt for the current list only.\nTree/directory views jump among visible directory names, while file-oriented views jump among the visible file rows for that surface."},
    {"Acceptance and cancel", "* **Type text**: Move immediately to the best current match as you type.\n* **Enter**: Keep the current match and stay there.\n* **Esc**: Cancel the jump and restore the original selection.\n* **Scope changes**: Filtering, Showall/Global projection, archives, and split mode all change which visible list `/` searches, but they do not change the jump keys themselves."},
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

static const GeneratedHelpLink generated_help_links_copy_move_targets[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"F8 split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_copy_move_targets[] = {
    {"Target forms", "Use a directory path when you want the original names preserved under another directory.\nUse one full replacement name when you want one selected item to land under a new explicit name.\nUse a wildcard pattern such as `*.bak` or `copy-*` when you want ytnova to rewrite each selected basename by pattern."},
    {"Shared rules", "Tagged copy/move uses the same target syntax as single-item copy/move.\nSplit mode may seed the inactive-panel directory as the default target, but you can still replace that default before the operation starts.\nArchive-backed copy/move keeps the same destination model even when extraction or archive-aware paths are involved."},
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
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"MoveDir", "copy-move-targets"},
    {"Output", "output"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_dir[] = {
    {"Directory commands", "* **1..9 view**: Change the active panel's directory and file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5`/`7`/`8`/`9` change the paired file view, and `6` toggles size units.\n* **Attributes**: Open the attributes submenu for the selected directory.\n* **Copy**: Copy the selected directory branch to another directory.\n* **Delete**: Delete the selected directory.\n* **Filter**: Filter the current file-list scope.\n* **Global**: Show files from every logged volume in one list.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected directory, the current logged tree, or another target.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file, or reload an already logged path from the top.\n* **Makedir**: Create a new directory.\n* **New File**: Create a new empty file in the current directory.\n* **Only tagged**: Show only tagged files in the current scope without changing the tag state.\n* **Pipe**: Send the selected directory to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected directory.\n* **Showall**: Show every file inside the current logged volume.\n* **Tag**: Tag the files in the selected directory scope.\n* **Untag**: Remove tags from the selected directory scope.\n* **MoveDir**: Move the selected directory branch.\n* **Write**: Export the current selection through the output prompts.\n* **Execute**: Run a shell command on the current selection while leaving `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files and dot-directories."},
    {"Directory function keys", "* **F1**: Open contextual help for the active directory surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the current file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_file[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
    {"Write", "output"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_file[] = {
    {"File commands", "* **1..9 view**: Change the active panel's file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.\n* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file and `Ctrl-K` copies the tagged set through the same prompt.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current file-list scope. `Ctrl-S` searches only the tagged files and untags non-matches.\n* **Hex**: View the selected file in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected file against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file without leaving file mode.\n* **Move**: `M` moves the selected file and `Ctrl-N` moves the tagged set through that same prompt.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only tagged files in the current scope without changing the tag state.\n* **Pipe**: Send the selected file to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Change the current file-list sort order.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current scope.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` removes every tag in the current scope.\n* **View**: View the selected file with the configured pager, and `Ctrl-V` views the tagged files one after another.\n* **Write**: Export the selected file through the output prompts, and `Ctrl-W` reuses the same prompts for the tagged set.\n* **Execute**: Run a shell command on the selected file, and `Ctrl-X` reruns the same command once for each tagged file. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current scope."},
    {"File function keys", "* **F1**: Open contextual help for the active file surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_archive_dir[] = {
    {"Navigation", "navigation"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_dir[] = {
    {"Archive directory commands", "* **1..9 view**: Change the archive directory/file presentation with keys `1` through `9`. `1..4` choose the base archive directory/file view, `5`, `7`, and `8` still affect the paired file projection, `6` toggles row-size units, and `9` stays a no-op inside archives.\n* **Delete**: Delete the selected archive directory entry.\n* **Filter**: Filter the current archive-backed file-list scope.\n* **Global**: Show archive-backed results together with other logged volumes.\n* **Compare**: Compare the selected archive directory or the current archive tree.\n* **Volume**: Open the volume picker.\n* **Log**: Log another directory or archive file.\n* **Makedir**: Create a directory where the archive format supports it.\n* **Jump**: Jump to a matching name in the current list.\n* **Pipe**: Send the selected archive path to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive directory entry.\n* **Showall**: Show every file in the current archive.\n* **Tag**: Tag the files in the current virtual directory scope.\n* **Untag**: Remove tags from the current virtual directory scope.\n* **Root/Exit**: `\\` jumps to archive root when you are below it, or leaves the archive when you are already there.\n* **Dotfiles**: Toggle hidden entries when the archive view exposes them."},
    {"Archive directory function keys", "* **F1**: Open contextual help for the active archive-directory surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the current file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_archive_file[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_archive_file[] = {
    {"Archive file commands", "* **1..9 view**: Change the archive-file presentation with keys `1` through `9`. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, and `8` toggles File detail, while `9` stays a no-op inside archives.\n* **Copy**: `C` copies the selected archive entry through archive-aware extract/copy paths, and `Ctrl-K` copies the tagged archive entries through the same prompt.\n* **Delete**: Delete the selected archive entry.\n* **Filter**: Filter the current archive-backed file-list scope. `Ctrl-S` searches only the tagged archive entries and untags non-matches.\n* **Hex**: View the selected archive entry in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected archive entry against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log another directory or archive file.\n* **Move**: `M` moves the selected archive entry through archive-aware paths, and `Ctrl-N` moves the tagged archive entries through that same prompt.\n* **Pipe**: Send the selected archive entry to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive entry.\n* **Sort**: Change the current file-list sort order.\n* **Tag**: Tag the selected archive entry, and `Ctrl-T` tags every visible archive row in the current scope.\n* **Untag**: Remove the tag from the selected archive entry, and `Ctrl-U` removes every archive tag in the current scope.\n* **View**: View the selected archive entry, and `Ctrl-V` views the tagged archive entries one after another.\n* **Pathcopy**: Copy the selected archive entry while keeping its relative path.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden entries when the archive view exposes them."},
    {"Archive file function keys", "* **F1**: Open contextual help for the active archive-file surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
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
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
    {"Write", "output"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_showall[] = {
    {"Showall rules", "* **Scope**: Showall lists every file inside the current logged volume only. It does not merge in other logged volumes.\n* **Return**: Return to the previously selected directory.\n* **Open owner**: Jump to the owner directory of the selected file inside the current logged volume."},
    {"Showall commands", "* **1..9 view**: Change the active panel's file presentation for the Showall list. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.\n* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file and `Ctrl-K` copies the tagged set through the same prompt.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current Showall result set. `Ctrl-S` searches only the tagged files in the current Showall result set and untags non-matches.\n* **Hex**: View the selected file in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible Showall result set.\n* **Compare**: Compare the selected file against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file without leaving Showall.\n* **Move**: `M` moves the selected file and `Ctrl-N` moves the tagged set through that same prompt.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only the tagged rows from the current Showall result set without changing the tag state.\n* **Pipe**: Send the selected file to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Repeating `S` changes sort without leaving Showall.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current Showall result set.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` removes every tag in the current Showall result set.\n* **View**: View the selected file with the configured pager, and `Ctrl-V` views the tagged files one after another.\n* **Write**: Export the selected file through the output prompts, and `Ctrl-W` reuses the same prompts for the tagged set.\n* **Execute**: Run a shell command on the selected file, and `Ctrl-X` reruns the same command once for each tagged file. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current Showall result set."},
    {"Showall function keys", "* **F1**: Open contextual help for the current Showall surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_global[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
    {"Write", "output"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_global[] = {
    {"Global rules", "* **Scope**: Global lists files from every logged volume. It is the cross-volume aggregate view.\n* **Return**: Return to the previously selected directory.\n* **Open owner**: Jump to the owner directory of the selected file even when it lives under another logged volume root."},
    {"Global commands", "* **1..9 view**: Change the active panel's file presentation for the Global list. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.\n* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file and `Ctrl-K` copies the tagged set through the same prompt.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current Global result set. `Ctrl-S` searches only the tagged files in the current Global result set and untags non-matches.\n* **Hex**: View the selected file in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible Global result set.\n* **Compare**: Compare the selected file against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file without leaving Global.\n* **Move**: `M` moves the selected file and `Ctrl-N` moves the tagged set through that same prompt.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only the tagged rows from the current Global result set without changing the tag state.\n* **Pipe**: Send the selected file to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Repeating `S` changes sort without leaving Global.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current Global result set.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` removes every tag in the current Global result set.\n* **View**: View the selected file with the configured pager, and `Ctrl-V` views the tagged files one after another.\n* **Write**: Export the selected file through the output prompts, and `Ctrl-W` reuses the same prompts for the tagged set.\n* **Execute**: Run a shell command on the selected file, and `Ctrl-X` reruns the same command once for each tagged file. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the owning volume root.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current Global result set."},
    {"Global function keys", "* **F1**: Open contextual help for the current Global surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_f7[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Applications menu", "applications-menu"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
    {"Write", "output"},
    {"Jump", "list-jump"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f7[] = {
    {"Preview rules", "* **F1**: Open contextual help for preview.\n* **F7**: Return to the underlying directory or file view.\n* **F8**: Split does nothing while preview is active.\n* **F9**: Open the applications menu without leaving preview.\n* **Tab panels**: `Tab` does not switch panels while preview is active.\n* **Esc**: Leave preview immediately."},
    {"Preview commands", "* **Attributes**: Open the attributes submenu for the selected file without leaving preview.\n* **Copy**: `C` copies the selected file and `Ctrl-K` copies the tagged set through the same prompt.\n* **Delete**: Delete the selected file without leaving preview.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current preview-backed file list. `Ctrl-S` searches only the tagged files and untags non-matches.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected file against another file.\n* **Move**: `M` moves the selected file and `Ctrl-N` moves the tagged set through that same prompt.\n* **New File**: Create a new empty file without leaving preview.\n* **Rename**: Rename the selected file without leaving preview.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current scope.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` removes every tag in the current scope.\n* **View**: View the selected file with the configured pager, and `Ctrl-V` views the tagged files one after another.\n* **Write**: Export the selected file through the output prompts, and `Ctrl-W` reuses the same prompts for the tagged set.\n* **Execute**: Run a shell command on the selected file, and `Ctrl-X` reruns the same command once for each tagged file. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current scope."},
};

static const GeneratedHelpLink generated_help_links_f8[] = {
    {"Navigation", "navigation"},
    {"Directory split page", "f8-dir"},
    {"File split page", "f8-file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f8[] = {
    {"Split rules", "* **F8**: Return to single-panel mode.\n* **Tab**: Switch the active panel and keep the passive panel's state intact.\n* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.\n* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state."},
};

static const GeneratedHelpLink generated_help_links_f8_dir[] = {
    {"F8", "f8"},
    {"Navigation", "navigation"},
    {"Split overview", "f8"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"MoveDir", "copy-move-targets"},
    {"Output", "output"},
    {"Jump", "list-jump"},
    {"Tab", "f8"},
    {"Target defaults", "f8"},
    {"Panel independence", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f8_dir[] = {
    {"Split rules", "* **F8**: Return to single-panel mode.\n* **Tab**: Switch the active panel and keep the passive panel's state intact.\n* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.\n* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state."},
    {"Split directory commands", "* **1..9 view**: Change the active panel's directory and file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5`/`7`/`8`/`9` change the paired file view, and `6` toggles size units.\n* **Attributes**: Open the attributes submenu for the selected directory.\n* **Copy**: Copy the selected directory branch. In split mode the destination prompt defaults to the selected directory on the inactive panel.\n* **Delete**: Delete the selected directory.\n* **Filter**: Filter the current file-list scope.\n* **Global**: Show files from every logged volume in one list.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected directory, the current logged tree, or another target.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file, or reload an already logged path from the top.\n* **Makedir**: Create a new directory.\n* **New File**: Create a new empty file in the current directory.\n* **Only tagged**: Show only tagged files in the current scope without changing the tag state.\n* **Pipe**: Send the selected directory to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected directory.\n* **Showall**: Show every file inside the current logged volume.\n* **Tag**: Tag the files in the selected directory scope.\n* **Untag**: Remove tags from the selected directory scope.\n* **MoveDir**: Move the selected directory branch.\n* **Write**: Export the current selection through the output prompts.\n* **Execute**: Run a shell command on the current selection while leaving `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files and dot-directories."},
    {"Split directory function keys", "* **F1**: Open contextual help for the active split-directory surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the current file context.\n* **F8**: Return to single-panel mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_f8_file[] = {
    {"F8", "f8"},
    {"Navigation", "navigation"},
    {"Split overview", "f8"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
    {"Write", "output"},
    {"Jump", "list-jump"},
    {"Tab", "f8"},
    {"Target defaults", "f8"},
    {"Panel independence", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_f8_file[] = {
    {"Split rules", "* **F8**: Return to single-panel mode.\n* **Tab**: Switch the active panel and keep the passive panel's state intact.\n* **Target defaults**: Copy, move, and compare prompts default to the inactive panel as destination or target.\n* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state."},
    {"Split file commands", "* **1..9 view**: Change the active panel's file presentation. `1` resets to Name, `2` shows Attributes, `3` shows Owner, `4` shows Times, `5` toggles Compact, `6` toggles size units, `7` toggles Mini preview, `8` toggles File detail, and `9` toggles the Git band inside Git worktrees.\n* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file and `Ctrl-K` copies the tagged set through the same prompt, which in split mode defaults to the selected directory on the inactive panel.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter the current file-list scope with globs, exclusions, and extended selectors. `Ctrl-S` searches only the tagged files and untags non-matches.\n* **Hex**: View the selected file in hex mode.\n* **Invert Tags**: Flip the tag state inside the current visible scope.\n* **Compare**: Compare the selected file against another file.\n* **Volume**: Open the volume picker.\n* **Log**: Log a new directory or archive file without leaving file mode.\n* **Move**: `M` moves the selected file and `Ctrl-N` moves the tagged set through that same prompt, which in split mode defaults to the selected directory on the inactive panel. The prompt still accepts rename or wildcard pattern targets.\n* **New File**: Create a new empty file.\n* **Only tagged**: Show only tagged files in the current scope without changing the tag state.\n* **Pipe**: Send the selected file to a command on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Change the current file-list sort order.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current scope.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` removes every tag in the current scope.\n* **View**: View the selected file with the configured pager, and `Ctrl-V` views the tagged files one after another.\n* **Write**: Export the selected file through the output prompts, and `Ctrl-W` reuses the same prompts for the tagged set.\n* **Execute**: Run a shell command on the selected file, and `Ctrl-X` reruns the same command once for each tagged file. Leave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\n* **Pathcopy**: Copy the selected file while keeping its path relative to the current volume root.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **Jump**: Jump to a matching name in the current list.\n* **Dotfiles**: Toggle hidden dot-files in the current scope."},
    {"Split file function keys", "* **F1**: Open contextual help for the active split-file surface.\n* **F5**: Refresh the active panel.\n* **F6**: Change the active panel's stats/details presentation.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Return to single-panel mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
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
        "list-jump",
        "List Jump",
        NULL,
        "Use `/` to type a live name fragment and jump inside the current list.\nThe active mode still decides which list you are searching.",
        4,
        generated_help_links_list_jump,
        2,
        generated_help_sections_list_jump,
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
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "Copy and move prompts accept a destination directory, a full replacement name, or a wildcard rename pattern.\nThe local mode page still owns which key copies or moves, which tagged repeat exists, and any split/archive caveat.",
        4,
        generated_help_links_copy_move_targets,
        2,
        generated_help_sections_copy_move_targets,
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
        "Directory Help lists the active tree-panel commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        7,
        generated_help_links_dir,
        2,
        generated_help_sections_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "File Help lists the active file-panel commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        9,
        generated_help_links_file,
        2,
        generated_help_sections_file,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Archive Directory Help lists the active archive-tree commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        4,
        generated_help_links_archive_dir,
        2,
        generated_help_sections_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Archive File Help lists the active archive-file commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        7,
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
        "Showall Help lists the commands for the current single-volume aggregate view.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        9,
        generated_help_links_showall,
        3,
        generated_help_sections_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global Help lists the commands for the current multi-volume aggregate view.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        9,
        generated_help_links_global,
        3,
        generated_help_sections_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "F7 Preview Help lists the commands that still work while preview stays open.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        10,
        generated_help_links_f7,
        2,
        generated_help_sections_f7,
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "Split mode keeps two panels active at once, and runtime F1 opens the directory or file split page for the active panel.\nUse the split page for the live footer command list and this page for the shared split model.",
        3,
        generated_help_links_f8,
        1,
        generated_help_sections_f8,
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "F8 Split Directory Help lists the split-specific rules plus the active split directory-footer commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        12,
        generated_help_links_f8_dir,
        3,
        generated_help_sections_f8_dir,
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "F8 Split File Help lists the split-specific rules plus the active split file-footer commands.\nRows with shared rules open their owning explainer; one-line rows already say the whole action.",
        14,
        generated_help_links_f8_file,
        3,
        generated_help_sections_f8_file,
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

static const size_t generated_help_topic_count = 38;
