/* Auto-generated from etc/help/f1.en.md, etc/help/f1.de.md by scripts/generate_help_assets.py. */
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

typedef struct {
    const char *locale_id;
    size_t topic_count;
    const GeneratedHelpTopic *topics;
} GeneratedHelpCatalog;

static const GeneratedHelpLink generated_help_links_en_intro[] = {
    {"Applications", "applications-menu"},
    {"Archive Directory", "archive-dir"},
    {"Archive File", "archive-file"},
    {"Command-line Editing", "command-line-editing"},
    {"Compare", "compare"},
    {"Compare Basis", "compare-basis"},
    {"Compare Result", "compare-results"},
    {"Compare Scope", "compare-scope"},
    {"Compare Target", "compare-target"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Create Archive", "create-archive"},
    {"Date Change", "change-date"},
    {"Directory", "dir"},
    {"F10 Config", "f10"},
    {"F2 Picker", "f2-picker"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
    {"File", "file"},
    {"Filter", "filter"},
    {"Global", "global"},
    {"History", "history-dialog"},
    {"List Jump", "list-jump"},
    {"Navigation", "navigation"},
    {"Output", "output"},
    {"Output Destination", "output-destination"},
    {"Output Format", "output-format"},
    {"Output Separator", "output-separator"},
    {"Search Tagged", "search-tagged"},
    {"Shared Commands", "shared-commands"},
    {"Showall", "showall"},
    {"Tagged", "tagged"},
    {"Theming", "theming"},
    {"Vi Keys", "vi-keys"},
    {"Volume", "volume-menu"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_intro[] = {
    {"Purpose", "Use `F1` for the task in front of you, not as one giant manual.\nLocal pages answer the active question first. Shared topics hold the repeated rules."},
    {"Index use", "This list stays alphabetical so you can scan it quickly.\nStart with the current screen when you know it; otherwise pick the topic that matches your question.\nOpen a topic, read the short answer, then use `Left` to come back without losing your place."},
};

static const GeneratedHelpLink generated_help_links_en_navigation[] = {
    {"Directory", "dir"},
    {"File", "file"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F2 Picker", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_navigation[] = {
    {"Common list keys", "* **Up/Down**: Move up or down one row.\n* **Page Up/Page Down**: Move up or down one screen.\n* **Home/End**: Jump straight to the first or last visible row.\n* **Enter**: Open the selected item. In paired tree/file views, it may move you into the matching other view instead.\n* **Left/Right**: `Right` usually opens or expands. `Left` usually goes back or collapses. If a page changes that, its own help says so."},
    {"Shared jump", "* **/**: Press `/` to start a jump by name in the current list.\n* **Type letters**: Type enough of the name to narrow the match down.\n* **Enter**: Land on the best visible match.\n* **Tree follow-up**: In tree views, repeat the jump inside the next directory if you want to go deeper.\n* **Esc**: Cancel the jump and restore the original selection."},
    {"Help popup keys", "* **Up/Down**: Move between selectable rows or links.\n* **Page Up/Page Down**: Scroll longer help pages.\n* **Home/End**: Jump to the top or bottom of the current help page.\n* **Enter/Right**: Open the selected help item or linked topic.\n* **Left**: Go back one step.\n* **Esc/Quit**: Close the popup."},
    {"Local exceptions", "* **Archives**: `\\` jumps to archive root, or leaves the archive when you are already there.\n* **Split**: `Tab` switches the active panel.\n* **Preview**: `Shift-Up/Down`, `Shift-PgUp/PgDn`, `Shift-Home/End`, `Ctrl-P`, and `Ctrl-N` scroll the preview."},
};

static const GeneratedHelpLink generated_help_links_en_list_jump[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_list_jump[] = {
    {"Jump model", "`/` opens a live name-jump prompt for the current list only.\nTree and directory views jump among visible directory names. File-oriented views jump among the visible file rows for that surface."},
    {"Accept or cancel", "* **Type letters**: Move to the best current match as you type.\n* **Enter**: Land on the current match and stay there.\n* **Esc**: Cancel the jump and restore the original selection.\n* **Scope changes**: Filtering, Showall, Global, archives, and split mode all change which visible list `/` searches."},
};

static const GeneratedHelpLink generated_help_links_en_shared_commands[] = {
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"Applications menu", "applications-menu"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_shared_commands[] = {
    {"Shared function keys", "* **F1**: Open contextual help for the active surface.\n* **F5**: Refresh the current view.\n* **F6**: Toggle the active panel's statistics strip. Split panels keep independent visibility.\n* **F7**: Toggle preview for the active file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface.\n* **Esc**: Back out of the current overlay, prompt, or popup."},
};

static const GeneratedHelpLink generated_help_links_en_tagged[] = {
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Filter help", "filter"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_tagged[] = {
    {"Tagged basics", "Tags are a working set. They are not a second clipboard and not a saved search.\nBuild a set, act on it, narrow it, then clear or invert it."},
    {"Common tagged flows", "* **Tag** and **Untag**: Add or remove the current row from the working set.\n* **Invert Tags**: Flip tag state inside the current visible scope.\n* **Filter**: Press `F`, then `Tab` to switch the current file-list scope between all rows and tagged-only rows without changing tag state.\n* **Copy tagged** and **Move tagged**: Send the whole tagged set to one destination.\n* **View tagged**: Open the tagged files one after another.\n* **Search tagged**: Search only the tagged files, then untag non-matches.\n* **Archive**: Archive the tagged set first. When nothing is tagged, archive falls back to the current selection."},
};

static const GeneratedHelpLink generated_help_links_en_command_line_editing[] = {
    {"Vi Keys", "vi-keys"},
    {"History", "history-dialog"},
    {"F2 Picker", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_command_line_editing[] = {
    {"Editing keys", "* **Left/Right**: Move one character.\n* **Home/End**: Jump to the start or end.\n* **Ctrl-A/Ctrl-E**: Same as `Home` and `End`.\n* **Backspace/Ctrl-H**: Delete the character to the left.\n* **Delete/Ctrl-D**: Delete the character under the cursor.\n* **Ctrl-W**: Delete the word to the left.\n* **Ctrl-U**: Delete from the cursor back to the start.\n* **Ctrl-K**: Delete from the cursor to the end.\n* **Enter**: Accept the current value.\n* **Esc**: Cancel without committing the prompt."},
    {"Prompt helpers", "* **Up**: Open or cycle prompt history when that prompt keeps history.\n* **History dialog**: Use `P` to pin, `D` to delete, `Enter` to reuse, and `Esc` to cancel.\n* **F2**: Open a browser or picker when the current prompt supports browsing.\n* **F1**: Show the syntax or local rules for the current prompt."},
};

static const GeneratedHelpLink generated_help_links_en_copy_move_targets[] = {
    {"Directory", "dir"},
    {"File", "file"},
    {"Archive File", "archive-file"},
    {"F8 Split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_copy_move_targets[] = {
    {"Target forms", "* **Directory path**: Keep the original names under another directory.\n* **Replacement name**: Rename one selected item on the way out.\n* **Wildcard pattern**: Rewrite each selected basename by pattern, such as `copy-*` or `*.bak`."},
    {"Prompt flow", "* **First prompt**: Choose the replacement name or wildcard pattern.\n* **Second prompt**: Choose the destination directory.\n* **Tagged copy/move**: Uses the same target syntax as single-item copy/move.\n* **Pathcopy**: Preserves the selected file's path relative to the current volume root.\n* **Split default**: Starts with the other panel's directory, but you can replace it.\n* **Archive-backed copy/move**: Uses the same destination model even when extraction is involved."},
    {"Safety checks", "* **Prompt count**: Only real safety prompts may follow.\n* **Missing directory**: If the destination directory does not exist, ytnova asks whether to create it.\n* **Conflict details**: Overwrite prompts show source and destination size/time facts when available.\n* **Directory flow**: After you accept the destination, ytnova starts the directory copy or move. No extra copy-now or move-now confirmation follows."},
};

static const GeneratedHelpLink generated_help_links_en_vi_keys[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_vi_keys[] = {
    {"Navigation remap", "With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.\n`Ctrl-U` and `Ctrl-D` become page up and page down."},
    {"Command collisions", "Commands that would steal those lowercase keys move out of the way.\nExamples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist."},
};

static const GeneratedHelpLink generated_help_links_en_f10[] = {
    {"Theming", "theming"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f10[] = {
    {"Config surface", "Use `F10` when you want to change persistent behavior instead of changing only the current selection.\nProfile settings, command labels, themes, and reload all live here."},
    {"Related files", "`ytnova.conf` owns profile settings.\n`commands.conf` owns user command labels and bindings.\n`themes.conf` owns theme selection and theme-role overrides."},
};

static const GeneratedHelpLink generated_help_links_en_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_theming[] = {
    {"Theme model", "Themes set semantic roles such as `footer`, `help`, `help_keybind`, `help_link`, `help_link_selection`, `selection`, `picker`, and `warning`.\n`footer` owns footer-style command strips, while `help` owns the F1 reading body and `help_box_lines` owns the popup frame.\nLinked explainers use `help_link`, and the active linked target uses `help_link_selection`, so help pages stay readable without hard-coded colors."},
    {"Editing path", "Use `F10` to open the theme or config editing path.\nKeep high-frequency navigation surfaces readable first: selection, picker, footer, and help."},
};

static const GeneratedHelpLink generated_help_links_en_dir[] = {
    {"Navigation", "navigation"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_dir[] = {
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **Shared per panel**: By default, `1..4` stay linked inside one panel. Changing the tree view also changes that panel's file window. Set `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n* **Tree versus file window**: In directory focus, `5`, `7`, `8`, and `9` do not change the tree rows. They change that panel's small file window.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch file and directory rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible file row. It leaves Compact so you can see the summary.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **`0`**: Currently unused; does nothing.\n* **Attributes**: Open directory attributes.\n* **Copy**: Copy the selected directory branch.\n* **Delete**: Delete the selected directory.\n* **Filter**: Filter this file list. `Tab` switches between all files and tagged files when tags exist.\n* **Global**: Open the cross-volume file list.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare this directory, its logged tree, or another target.\n* **K volume**: Open the volume menu.\n* **Log**: Log a directory or archive, or reload a logged path from the top.\n* **Makedir**: Create a directory.\n* **Newfile**: Create an empty file here.\n* **Output**: Export the current selection.\n* **Pipe**: Type a shell command. ytnova runs it in the selected directory and sends the visible matching names to its standard input, one per line.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected directory.\n* **Showall**: Open the current-volume file list.\n* **Tag**: Tag the files under the selected directory.\n* **Untag**: Remove those tags.\n* **moVedir**: Move the selected directory branch.\n* **eXecute**: Type a shell command. Use `{}` where the selected directory path should go, and leave `{}` unquoted so ytnova can quote the path safely.\n* **Z archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match in this tree.\n* **\\` dotfiles**: Show or hide hidden names.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_file[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"C/^K copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_file[] = {
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **Shared per panel**: By default, `1..4` stay linked inside one panel. Changing the tree view also changes that panel's file window. Set `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch file and directory rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible file row. It leaves Compact so you can see the summary.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **Extra state label**: `5`, `7`, `8`, and `9` do not stack in the stats label. It shows only the one extra state you can currently see.\n* **`0`**: Currently unused; does nothing.\n* **Attributes**: Open file attributes.\n* **C/^K copy**: `C` copies the selected file. `Ctrl-K` copies the tagged set.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter this list. `Ctrl-S` searches only tagged files. `Tab` switches between all files and tagged files when tags exist.\n* **Hex**: Open the selected file in hex view.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare the selected file with another file.\n* **K volume**: Open the volume menu.\n* **Log**: Log a directory or archive without leaving file mode.\n* **M/^N move**: `M` moves the selected file. `Ctrl-N` moves the tagged set.\n* **Newfile**: Create an empty file.\n* **Output**: Export the selection. `Ctrl-O` reuses the prompts for the tagged set.\n* **Pipe**: Type a shell command and feed it the contents of the selected file on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Change the file-list sort order.\n* **Tag**: Tag the selected file. `Ctrl-T` tags every visible file.\n* **Untag**: Remove the selected tag. `Ctrl-U` clears tags in this scope.\n* **View**: View the selected file. `Ctrl-V` views the tagged files one after another.\n* **eXecute**: Type a shell command. Use `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `Ctrl-X` to repeat the command once per tagged file.\n* **pathcopY**: Copy the selected file while keeping its path relative to the current volume root.\n* **Z archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **\\` dotfiles**: Show or hide hidden files.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_archive_dir[] = {
    {"Navigation", "navigation"},
    {"Filter", "filter"},
    {"Compare", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_archive_dir[] = {
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch archive rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible archive file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible archive file row. It leaves Compact so you can see the summary.\n* **`9`**: Unused in archive lists.\n* **`0`**: Currently unused; does nothing.\n* **Delete**: Delete the selected archive directory entry.\n* **Filter**: Filter this archive-backed file list.\n* **Global**: Mix archive results into the cross-volume file list.\n* **J compare**: Compare this archive directory or its logged tree.\n* **K volume**: Open the volume menu.\n* **Log**: Log another directory or archive.\n* **Makedir**: Create a directory where the archive format supports it.\n* **Output**: Export the current archive-backed selection.\n* **Pipe**: Type a shell command. ytnova sends the visible matching names from the selected archive directory to its standard input, one per line.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive directory entry.\n* **Showall**: Open the current archive-wide file list.\n* **Tag**: Tag the files in the current virtual directory.\n* **Untag**: Remove those tags.\n* **\\ root/exit**: Jump to archive root, or leave the archive when you are already there.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **` dotfiles**: Show or hide hidden archive entries when this view exposes them.\n* **F1**: Open this help.\n* **F5**: Refresh.\n* **F6**: Toggle the active panel's statistics strip. Split panels keep independent visibility.\n* **F7**: Toggle preview.\n* **F8**: Toggle split.\n* **F9**: Open Applications.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_archive_file[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_archive_file[] = {
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch archive rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible archive file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible archive file row. It leaves Compact so you can see the summary.\n* **`9`**: Unused in archive lists.\n* **`0`**: Currently unused; does nothing.\n* **C/^K copy**: `C` copies the selected archive entry through archive-aware extract/copy paths. `Ctrl-K` copies the tagged archive entries.\n* **Delete**: Delete the selected archive entry.\n* **Filter**: Filter this archive-backed list. `Ctrl-S` searches only tagged entries and untags non-matches.\n* **Hex**: Open the selected archive entry in hex view.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare the selected archive entry with another file.\n* **K volume**: Open the volume menu.\n* **Log**: Log another directory or archive.\n* **M/^N move**: `M` moves the selected archive entry. `Ctrl-N` moves the tagged archive entries.\n* **Output**: Export the selected archive entry.\n* **Pipe**: Type a shell command and feed it the contents of the selected archive entry on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected archive entry.\n* **Sort**: Change the archive file-list sort order.\n* **Tag**: Tag the selected archive entry. `Ctrl-T` tags every visible archive row.\n* **Untag**: Remove the selected tag. `Ctrl-U` clears archive tags in this scope.\n* **View**: View the selected archive entry. `Ctrl-V` views the tagged archive entries one after another.\n* **eXecute**: Not available in archive file mode.\n* **pathcopY**: Copy the selected archive entry while keeping its relative path.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **` dotfiles**: Show or hide hidden archive entries when this view exposes them.\n* **F1**: Open this help.\n* **F5**: Refresh.\n* **F6**: Toggle the active panel's statistics strip. Split panels keep independent visibility.\n* **F7**: Toggle preview.\n* **F8**: Toggle split.\n* **F9**: Open Applications.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_filter[] = {
    {"Tagged", "tagged"},
    {"Showall", "showall"},
    {"Global", "global"},
    {"Command-line Editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_filter[] = {
    {"Syntax", "* `*` — show all files\n* `*.c` — glob match\n* `*.c,*.h` — more than one glob term\n* `-*.o` — exclude matches\n* `:r` — readable files\n* `:x` — executable files\n* `>2023-01-01` — newer than a date\n* `>1M` — larger than a size\n* Combine them, for example `*.c,-*.tmp` or `*.log,>2024-01-01,-debug*`."},
    {"Scope", "The filter always applies to the current file-list family.\nThat may be a normal file list, an archive file list, Showall, or Global.\nWhen tagged scope is active, the prompt changes to `FILTER [tagged only]:`."},
};

static const GeneratedHelpLink generated_help_links_en_compare[] = {
    {"File", "file"},
    {"Directory", "dir"},
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_compare[] = {
    {"Compare flow", "Choose the target on the compare prompt.\nUse `F3` to choose file, directory, tree, or external compare.\nUse `F4` to choose how ytnova decides whether two files match.\nUse `F5` to choose which result should be tagged after the compare.\nPress `Enter` when the prompt shows the compare plan you want."},
    {"Compare rules", "* **J compare**: The `J` key keeps the old XTree-family compare key.\n* **Logged tree**: Uses the part of the tree that is already logged. It does not auto-log unopened `+` subdirectories.\n* **FILEDIFF**: May use `%1` and `%2`. When those placeholders are missing, ytnova appends source and target paths.\n* **External compare**: Launches `DIRDIFF` or `TREEDIFF` instead of tagging results inside ytnova.\n* **Tagged compare**: There is no separate compare-tagged-files mode."},
};

static const GeneratedHelpLink generated_help_links_en_compare_target[] = {
    {"Compare", "compare"},
    {"Command-line Editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_compare_target[] = {
    {"Target rules", "Enter one path.\n`F3` cycles directory -> logged tree -> external directory -> external tree.\n`F4` cycles `size+date` -> `size` -> `date` -> `hash`.\n`F5` cycles `Different` -> `Match` -> `Newer` -> `Older` -> `Unique` -> `Type mismatch` -> `Error`.\nExternal compare still shows the saved internal choices so you can switch back without losing them."},
};

static const GeneratedHelpLink generated_help_links_en_change_date[] = {
    {"File mode", "file"},
    {"Directory mode", "dir"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_change_date[] = {
    {"Scope choices", "`modified` changes only the last-modified timestamp.\n`accessed` changes only the access timestamp.\n`both` writes the entered value to both timestamps."},
    {"Format rules", "If you omit the time portion, ytnova keeps the existing hour, minute, and second from the current value.\nUse `Up` for prompt history and `Esc` to cancel without changing either timestamp."},
};

static const GeneratedHelpLink generated_help_links_en_compare_scope[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_compare_scope[] = {
    {"Scope choices", "Use `Directory` for one level.\nUse `Logged tree` for the currently logged recursive tree.\nUse `External viewer` when you want an external diff tool instead of tagged compare results inside ytnova."},
};

static const GeneratedHelpLink generated_help_links_en_compare_basis[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_compare_basis[] = {
    {"Basis choices", "Use `Size` when you only need a quick rough pass.\nUse `size+date` for the normal “are these probably the same?” check.\nUse `Hash` when you need the strongest answer. A hash is a fingerprint made from the file contents, so matching hashes mean the content matches exactly."},
};

static const GeneratedHelpLink generated_help_links_en_compare_results[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_compare_results[] = {
    {"Result tagging", "The compare command never rewrites files.\nIt tags the chosen result on the selected side so you can inspect, copy, move, or archive that subset next."},
};

static const GeneratedHelpLink generated_help_links_en_execute_file[] = {
    {"File mode", "file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_execute_file[] = {
    {"Placeholder rules", "`{}` stands for one selected file path.\nWhen you use the tagged rerun path, the same command is repeated once per tagged file."},
};

static const GeneratedHelpLink generated_help_links_en_execute_dir[] = {
    {"Directory mode", "dir"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_execute_dir[] = {
    {"Placeholder rules", "`{}` stands for the current directory path.\nThe tagged rerun path still walks tagged files from the active list, not tagged directories from somewhere else."},
};

static const GeneratedHelpLink generated_help_links_en_search_tagged[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_search_tagged[] = {
    {"Tagged search rules", "Start by tagging a working set.\nThen search only that set. The result is another, narrower tagged set because files that do not match lose their tags."},
};

static const GeneratedHelpLink generated_help_links_en_create_archive[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_create_archive[] = {
    {"Archive creation rules", "Directory selections are archived recursively.\nArchive creation picks the tagged set first because tagging is the normal way to build a custom archive batch."},
};

static const GeneratedHelpLink generated_help_links_en_output[] = {
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_output[] = {
    {"Output model", "`Output` is an export flow, not an editor.\nIt can write plain content, framed content, or page-break-separated content.\nIt can also send that export to a printer command instead of a file path."},
    {"Prompt order", "Choose file or hardcopy first.\nOn the file destination prompt, `F3` cycles `Raw`, `Framed`, and `Page break`.\nWhen `Framed` or `Page break` is active, choose the separator before entering the final file path.\nHardcopy asks only for the printer command."},
};

static const GeneratedHelpLink generated_help_links_en_output_format[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_output_format[] = {
    {"Format choices", "Use `Raw` when another tool will parse the output.\nUse `Framed` or `Page break` when a human will read the exported batch."},
};

static const GeneratedHelpLink generated_help_links_en_output_destination[] = {
    {"Output Help", "output"},
    {"Output Format Help", "output-format"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_output_destination[] = {
    {"Destination choices", "* **File output**: Write exported text to a path.\n* **`CWD`**: Use the current working directory for bare filenames.\n* **Hardcopy**: Send raw exported text to a shell printer command such as `lpr`, `lp`, or `cat > /dev/lp1`."},
    {"Format cycle", "`F3` is available only on the file destination prompt.\nWhen it selects `Framed` or `Page break`, ytnova asks for the separator before returning to the file path prompt."},
};

static const GeneratedHelpLink generated_help_links_en_output_separator[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_output_separator[] = {
    {"Separator rules", "The separator is reused between files for the current framed or page-break export.\nIt is not appended after the last file."},
};

static const GeneratedHelpLink generated_help_links_en_showall[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_showall[] = {
    {"View and scope", "* **Scope**: Showall lists every file inside the current logged volume only. It does not cross into other loaded volumes.\n* **Return**: Return to the previously selected directory.\n* **Open owner**: Jump to the owner directory of the selected file inside the current logged volume.\n* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset rule**: `1` returns to plain Name. Pressing the already-active `2`, `3`, or `4` again also drops back to Name.\n* **Shared-per-panel rule**: By default, `1..4` are shared inside one panel. Set `SEPARATE_DIR_FILE_VIEWS=1` to make Showall/file-window and tree-directory base views independent again.\n* **`5`**: Toggle Compact from the current `1` / Name view only.\n* **`6`**: Switch file rows between human-readable and raw size units. Stats stay human-readable.\n* **`7`**: Show Mini preview text on each visible file row, and leave Compact so you can see it.\n* **`8`**: Show File detail text on each visible file row, and leave Compact so you can see it.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **`0`**: Currently unused; it does nothing.\n* **Sort**: Repeating `S` changes sort without leaving Showall.\n* **Jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **Dotfiles**: Show or hide hidden files in the current Showall result set."},
    {"Working set", "* **Filter**: Filter the current Showall result set. `Ctrl-S` searches only tagged files there. Inside the prompt, `Tab` narrows the same result set to tagged-only.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current Showall result set.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` clears tags in the current Showall result set.\n* **Invert Tags**: Flip tag state inside the visible Showall result set.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged."},
    {"File actions", "* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file, and `Ctrl-K` copies the tagged set through the same prompt.\n* **Move**: `M` moves the selected file, and `Ctrl-N` moves the tagged set through the same prompt.\n* **View**: View the selected file, and `Ctrl-V` views the tagged files one after another.\n* **Edit**: Open the selected file in the configured editor.\n* **Hex**: View the selected file in hex mode.\n* **Compare**: Compare the selected file against another file.\n* **Output**: Export the selection. `Ctrl-O` reuses the prompts for the tagged set, and `Ctrl-W` stays as a legacy alias.\n* **Execute**: Type a shell command. Use `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `Ctrl-X` to repeat the command once per tagged file.\n* **Pathcopy**: Copy the selected file while preserving its path relative to the current volume root.\n* **Pipe**: Type a shell command and feed it the contents of the selected file on standard input.\n* **New File**: Create a new empty file.\n* **Rename**: Rename the selected file.\n* **Delete**: Delete the selected file.\n* **Log**: Log a new directory or archive file without leaving Showall.\n* **Volume**: Open the volume picker.\n* **Quit**: Quit ytnova."},
    {"Showall function keys", "* **F1**: Open contextual help for the current Showall surface.\n* **F5**: Refresh the active panel.\n* **F6**: Toggle the active panel's statistics strip. Split panels keep independent visibility.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_en_global[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_global[] = {
    {"View and scope", "* **Scope**: Global lists files from every logged volume.\n* **Return**: Return to the previously selected directory.\n* **Open owner**: Jump to the owner directory of the selected file even when it lives under another logged volume root.\n* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset rule**: `1` returns to plain Name. Pressing the already-active `2`, `3`, or `4` again also drops back to Name.\n* **Shared-per-panel rule**: By default, `1..4` are shared inside one panel. Set `SEPARATE_DIR_FILE_VIEWS=1` to make Global/file-window and tree-directory base views independent again.\n* **`5`**: Toggle Compact from the current `1` / Name view only.\n* **`6`**: Switch file rows between human-readable and raw size units. Stats stay human-readable.\n* **`7`**: Show Mini preview text on each visible file row, and leave Compact so you can see it.\n* **`8`**: Show File detail text on each visible file row, and leave Compact so you can see it.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **`0`**: Currently unused; it does nothing.\n* **Sort**: `S` changes sort without leaving Global.\n* **Jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **Dotfiles**: Show or hide hidden files in the current Global result set."},
    {"Working set", "* **Filter**: Filter the current Global result set. `Ctrl-S` searches only tagged files there. Inside the prompt, `Tab` narrows the same result set to tagged-only.\n* **Tag**: Tag the selected file, and `Ctrl-T` tags every visible file in the current Global result set.\n* **Untag**: Remove the tag from the selected file, and `Ctrl-U` clears tags in the current Global result set.\n* **Invert Tags**: Flip tag state inside the visible Global result set.\n* **Archive**: Archive the tagged set first, or the current selection when nothing is tagged."},
    {"File actions", "* **Attributes**: Open the attributes submenu for the selected file.\n* **Copy**: `C` copies the selected file, and `Ctrl-K` copies the tagged set through the same prompt.\n* **Move**: `M` moves the selected file, and `Ctrl-N` moves the tagged set through the same prompt.\n* **View**: View the selected file, and `Ctrl-V` views the tagged files one after another.\n* **Edit**: Open the selected file in the configured editor.\n* **Hex**: View the selected file in hex mode.\n* **Compare**: Compare the selected file against another file.\n* **Output**: Export the selection. `Ctrl-O` reuses the prompts for the tagged set, and `Ctrl-W` stays as a legacy alias.\n* **Execute**: Type a shell command. Use `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `Ctrl-X` to repeat the command once per tagged file.\n* **Pathcopy**: Copy the selected file while preserving its path relative to the owning volume root.\n* **Pipe**: Type a shell command and feed it the contents of the selected file on standard input.\n* **New File**: Create a new empty file.\n* **Rename**: Rename the selected file.\n* **Delete**: Delete the selected file.\n* **Log**: Log a new directory or archive file without leaving Global.\n* **Volume**: Open the volume picker.\n* **Quit**: Quit ytnova."},
    {"Global function keys", "* **F1**: Open contextual help for the current Global surface.\n* **F5**: Refresh the active panel.\n* **F6**: Toggle the active panel's statistics strip. Split panels keep independent visibility.\n* **F7**: Toggle preview for the selected file context.\n* **F8**: Toggle split-screen mode.\n* **F9**: Open the Applications menu.\n* **F10**: Open the configuration command surface."},
};

static const GeneratedHelpLink generated_help_links_en_f7[] = {
    {"Navigation", "navigation"},
    {"Tagged", "tagged"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Applications", "applications-menu"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f7[] = {
    {"Preview navigation", "* **Up/Down, PgUp/PgDn, Home/End**: Keep moving the selected file.\n* **Shift-Up/Down** or **Ctrl-P/Ctrl-N**: Scroll preview lines.\n* **Shift-PgUp/PgDn**: Scroll preview by pages.\n* **Shift-Home/End**: Jump to the start or end of the preview.\n* **F7**: Return to the underlying directory or file view.\n* **F8**: Split does nothing while preview is active.\n* **F9**: Open Applications without leaving preview.\n* **Tab**: Do not switch panels while preview is active.\n* **Esc**: Leave preview immediately."},
    {"Live commands", "* **Attributes**: Open file attributes.\n* **C/^K copy**: `C` copies the selected file. `Ctrl-K` copies the tagged set.\n* **Delete**: Delete the selected file without leaving preview.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter this preview list. `Ctrl-S` searches only tagged files.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare the selected file with another file.\n* **M/^N move**: `M` moves the selected file. `Ctrl-N` moves the tagged set.\n* **Newfile**: Create an empty file without leaving preview.\n* **Rename**: Rename the selected file without leaving preview.\n* **Tag**: Tag the selected file.\n* **Untag**: Remove the selected tag.\n* **View**: View the selected file. `Ctrl-V` views the tagged files one after another.\n* **Output**: Export the selection. `Ctrl-O` reuses the prompts for the tagged set.\n* **eXecute**: Type a shell command. Use `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `Ctrl-X` to repeat the command once per tagged file.\n* **pathcopY**: Copy the selected file while keeping its path relative to the current volume root.\n* **Z archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **\\` dotfiles**: Show or hide hidden files in this preview-backed list."},
};

static const GeneratedHelpLink generated_help_links_en_f8[] = {
    {"Navigation", "navigation"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f8[] = {
    {"Split rules", "* **F8**: Return to single-panel mode.\n* **Tab**: Switch the active panel and keep the other panel's state intact.\n* **Target defaults**: Copy, move, and compare prompts start with the other panel as the default destination or target.\n* **Panel independence**: Each panel keeps its own selection, view, tags, volume, and restore state."},
};

static const GeneratedHelpLink generated_help_links_en_f8_dir[] = {
    {"Navigation", "navigation"},
    {"F8 Split", "f8"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"moVedir", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f8_dir[] = {
    {"Live commands", "* **F8**: Return to single-panel mode.\n* **Tab**: Change the active panel.\n* **Target defaults**: Copy, move, and compare default to the inactive panel."},
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **Shared per panel**: By default, `1..4` stay linked inside one panel. Changing the tree view also changes that panel's file window. Set `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n* **Tree versus file window**: In split directory focus, `5`, `7`, `8`, and `9` do not change the tree rows. They change the active panel's file window.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch file and directory rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible file row. It leaves Compact so you can see the summary.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **`0`**: Currently unused; does nothing.\n* **Attributes**: Open directory attributes.\n* **Copy**: Copy the selected directory branch. In split mode the other panel is the default destination.\n* **Delete**: Delete the selected directory.\n* **Filter**: Filter this file list. `Tab` switches between all files and tagged files when tags exist.\n* **Global**: Open the cross-volume file list for the active panel.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare this directory, its logged tree, or another target.\n* **K volume**: Open the volume menu.\n* **Log**: Log a directory or archive, or reload a logged path from the top.\n* **Makedir**: Create a directory.\n* **Newfile**: Create an empty file here.\n* **Output**: Export the current selection.\n* **Pipe**: Type a shell command. ytnova runs it in the selected directory and sends the visible matching names to its standard input, one per line.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected directory.\n* **Showall**: Open the current-volume file list for the active panel.\n* **Tag**: Tag the files under the selected directory.\n* **Untag**: Remove those tags.\n* **moVedir**: Move the selected directory branch. In split mode the other panel is the default destination.\n* **eXecute**: Type a shell command. Use `{}` where the selected directory path should go, and leave `{}` unquoted so ytnova can quote the path safely.\n* **Z archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match in this tree.\n* **\\` dotfiles**: Show or hide hidden names.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_f8_file[] = {
    {"Navigation", "navigation"},
    {"F8 Split", "f8"},
    {"Tagged", "tagged"},
    {"C/^K copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"M/^N move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f8_file[] = {
    {"Live commands", "* **F8**: Return to single-panel mode.\n* **Tab**: Change the active panel.\n* **Target defaults**: Copy, move, and compare default to the inactive panel."},
    {"View and scope", "* **`1`**: Name only. This is the plain default view.\n* **`2`**: Attributes. In file lists this also shows `name -> target` for symlinks.\n* **`3`**: Owner.\n* **`4`**: Times.\n* **Reset**: `1` always returns to the plain Name view. If `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n* **Shared per panel**: By default, `1..4` stay linked inside one panel. Changing the tree view also changes that panel's file window. Set `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n* **`5`**: Turn Compact on or off from the current `1` / Name view only.\n* **`6`**: Switch file and directory rows between readable and raw size units. Stats stay readable.\n* **`7`**: Show a small text preview on each visible file row. It leaves Compact so you can see the text.\n* **`8`**: Show file detail text on each visible file row. It leaves Compact so you can see the summary.\n* **`9`**: Show the Git band when the current directory is inside a Git worktree.\n* **Extra state label**: `5`, `7`, `8`, and `9` do not stack in the stats label. It shows only the one extra state you can currently see.\n* **`0`**: Currently unused; does nothing.\n* **Attributes**: Open file attributes.\n* **C/^K copy**: `C` copies the selected file. `Ctrl-K` copies the tagged set. In split mode the other panel is the default destination.\n* **Delete**: Delete the selected file.\n* **Edit**: Open the selected file in the configured editor.\n* **Filter**: Filter this list. `Ctrl-S` searches only tagged files. `Tab` switches between all files and tagged files when tags exist.\n* **Hex**: Open the selected file in hex view.\n* **Invert**: Flip tags in the visible scope.\n* **J compare**: Compare the selected file with another file.\n* **K volume**: Open the volume menu.\n* **Log**: Log a directory or archive without leaving split file mode.\n* **M/^N move**: `M` moves the selected file. `Ctrl-N` moves the tagged set. In split mode the other panel is the default destination.\n* **Newfile**: Create an empty file.\n* **Output**: Export the selection. `Ctrl-O` reuses the prompts for the tagged set.\n* **Pipe**: Type a shell command and feed it the contents of the selected file on standard input.\n* **Quit**: Quit ytnova.\n* **Rename**: Rename the selected file.\n* **Sort**: Change the file-list sort order.\n* **Tag**: Tag the selected file. `Ctrl-T` tags every visible file.\n* **Untag**: Remove the selected tag. `Ctrl-U` clears tags in this scope.\n* **View**: View the selected file. `Ctrl-V` views the tagged files one after another.\n* **eXecute**: Type a shell command. Use `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `Ctrl-X` to repeat the command once per tagged file.\n* **pathcopY**: Copy the selected file while keeping its path relative to the current volume root.\n* **Z archive**: Archive the tagged set first, or the current selection when nothing is tagged.\n* **/ jump**: Press `/`, type letters, and press `Enter` to land on the best visible match.\n* **\\` dotfiles**: Show or hide hidden files.\n* **F10**: Open configuration."},
};

static const GeneratedHelpLink generated_help_links_en_history_dialog[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_history_dialog[] = {
    {"History actions", "* **Select entry**: `Up` and `Down` move through the current history list.\n* **Scroll long entry**: `Left` and `Right` shift a long history line horizontally.\n* **Pin**: `P` keeps an important entry at the top of the current history list.\n* **Delete**: `D` removes the selected entry from the current history list.\n* **Accept**: `Enter` reuses the selected entry.\n* **Cancel**: `Esc` closes the dialog without reusing an entry."},
};

static const GeneratedHelpLink generated_help_links_en_volume_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_volume_menu[] = {
    {"Volume actions", "* **Select volume**: `Up` and `Down` move through the loaded-volume list.\n* **Switch volume**: `Enter` activates the selected volume.\n* **Keep state**: Selecting the already active volume keeps its current in-memory state.\n* **Release volume**: `D` unloads the selected volume unless it is the last remaining one.\n* **Cancel**: `Esc` closes the menu."},
};

static const GeneratedHelpLink generated_help_links_en_applications_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_applications_menu[] = {
    {"Applications actions", "* **Select preset**: `Up` and `Down` move through the preset list.\n* **Launch behavior**: `F9` starts the selected preset and returns straight to the TUI. Use it for repeat-heavy external workflows, not for one-off shell typing.\n* **Use `eXecute` for one-offs**: The `X` command prompt stays the ad hoc shell surface with history and terminal-style output. Use it when you need a one-off command.\n* **Edit presets**: `E` opens the dedicated applications catalog so presets can be changed without leaving the chooser family.\n* **Selection and working directory**: `{}` inserts the current file or folder. Presets also start in that directory, so scripts without `{}` still run from the place you selected.\n* **Prompt text**: `{input}` inserts the extra text you typed for the preset prompt.\n* **Starter presets**: The bundled catalog starts with `xdg-open` launchers and includes commented examples for tools such as `mpv` or local helper scripts.\n* **Cancel menu**: `Esc` closes the chooser without selecting a preset."},
};

static const GeneratedHelpLink generated_help_links_en_f2_picker[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_en_f2_picker[] = {
    {"Picker actions", "* **Move**: `Up` and `Down` move through the visible directory rows.\n* **Expand**: `Right` expands the current directory one level, then moves into the first child when that level is already open.\n* **Collapse**: `Left` collapses the current directory, or moves to its parent when the current row is already closed.\n* **Select**: `Enter` uses the highlighted directory for the calling prompt.\n* **Cancel**: `Esc` closes the picker without changing the prompt."},
};

static const GeneratedHelpTopic generated_help_topics_en[] = {
    {
        "intro",
        "Contents",
        NULL,
        "Browse this index when you know the question but not the page.\nPress `Enter` or `Right` on a topic to open it.\nUse `Left` to come back here or `Esc` to leave help.",
        36,
        generated_help_links_en_intro,
        2,
        generated_help_sections_en_intro,
    },
    {
        "navigation",
        "Navigation",
        NULL,
        "Use this page when you just want to move around a ytnova list.\n`Up/Down` move one row at a time. `Right` usually opens or expands the selected row, and `Left` usually goes back or collapses it.\n`Enter` opens the selected row. `PgUp/PgDn` move a screen at a time. `Home/End` jump to the top or bottom.\n`/` starts a jump by name: press `/`, type letters, then press `Enter` to land on the best visible match.\nIf one screen changes any of those keys, that screen explains the exception on its own help page.",
        5,
        generated_help_links_en_navigation,
        4,
        generated_help_sections_en_navigation,
    },
    {
        "list-jump",
        "List Jump",
        NULL,
        "Press `/`, type letters, and press `Enter` to land on the best visible match in the current list.\nIn tree views, repeat the jump in the next directory if you want to go deeper.",
        4,
        generated_help_links_en_list_jump,
        2,
        generated_help_sections_en_list_jump,
    },
    {
        "shared-commands",
        "Shared Commands",
        NULL,
        "These keys keep the same high-level meaning across more than one mode.\nUse the mode page for local commands, and use this page for the shared function-key family.",
        4,
        generated_help_links_en_shared_commands,
        1,
        generated_help_sections_en_shared_commands,
    },
    {
        "tagged",
        "Tagged",
        NULL,
        "Tags let you build a working set, then run one command against that set.\nWhen tags exist, copy, move, view, archive, and search often act on the tagged set instead of only the current row.\n`Filter` owns the tagged-only scope toggle; press `F`, then `Tab` to turn the current tagged set into its own temporary list without changing the tags.",
        3,
        generated_help_links_en_tagged,
        2,
        generated_help_sections_en_tagged,
    },
    {
        "command-line-editing",
        "Command-line Editing",
        NULL,
        "Most prompts share the same editing keys.\nUse this page for cursor movement, delete keys, history, and picker shortcuts.",
        3,
        generated_help_links_en_command_line_editing,
        2,
        generated_help_sections_en_command_line_editing,
    },
    {
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "Copy, move, and pathcopy use two explicit prompts.\nFirst choose the replacement name or wildcard rename pattern.\nThen choose the destination directory.\nMerging them would hide meaning instead of removing friction.\nAfter that, only real safety prompts may follow.\nOverwrite conflicts compare size/time so you can judge newer/older and bigger/smaller.\nFor directories, no copy-now or move-now confirmation follows.",
        4,
        generated_help_links_en_copy_move_targets,
        3,
        generated_help_sections_en_copy_move_targets,
    },
    {
        "vi-keys",
        "Vi Keys",
        NULL,
        "With `VI_KEYS=1`, lowercase vi movement keys stay active.\nCommands that would collide move to uppercase or another safe key.",
        2,
        generated_help_links_en_vi_keys,
        2,
        generated_help_sections_en_vi_keys,
    },
    {
        "f10",
        "F10 Config Help",
        NULL,
        "Use `F10` for configuration work, not one-off file actions.\nThat is where profile, commands, themes, reload, and similar setup actions belong.",
        2,
        generated_help_links_en_f10,
        2,
        generated_help_sections_en_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes style semantic roles, not one-off screen positions.\nThat keeps help, picker, selection, footer, and warning surfaces readable as one system.",
        1,
        generated_help_links_en_theming,
        2,
        generated_help_sections_en_theming,
    },
    {
        "dir",
        "Directory Help",
        "main.dir",
        "This page explains the live directory footer commands.\nUse `Navigation` for shared movement keys, and use the linked topics only when a short command row needs more detail.",
        5,
        generated_help_links_en_dir,
        1,
        generated_help_sections_en_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "This page explains the live file footer commands.\nUse `Navigation` for shared movement keys, and use the linked topics only when a short command row needs more detail.",
        6,
        generated_help_links_en_file,
        1,
        generated_help_sections_en_file,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Navigation follows the usual list keys except for the archive-only rules on this page.\nThis page explains the live archive-directory footer commands.",
        3,
        generated_help_links_en_archive_dir,
        1,
        generated_help_sections_en_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Navigation follows the usual list keys except for the archive-only rules on this page.\nThis page explains the live archive-file footer commands.",
        6,
        generated_help_links_en_archive_file,
        1,
        generated_help_sections_en_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Type one or more filter terms for the current file list.\n`*` means show everything, `*.c` matches by name, `-*.o` excludes, `:r` and `:x` test readable or executable, and `>2023-01-01` or `>1M` test date or size.\nSeparate terms with commas so they all apply together.\nPress `Tab` to switch between all files and tagged files when that extra scope is available.",
        4,
        generated_help_links_en_filter,
        2,
        generated_help_sections_en_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "File compare checks the selected file against one target file.\nDirectory compare checks the selected directory against another directory, the logged tree under it, or an external diff tool.\nThe built-in compare does not change files. It tags the results you asked for so you can act on them next.",
        3,
        generated_help_links_en_compare,
        2,
        generated_help_sections_en_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "The current file, directory, or logged tree is the compare source.\nEnter one target path directly, use `F2` to browse, or use `Up` for history.\nPress `F3` for Compare Scope: file, directory, tree, or external compare.\nPress `F4` for Compare Basis: `size`, `date`, `size+date`, or `hash`.\nPress `F5` to choose which result gets tagged after the compare.\nIn split view, the inactive panel seeds the default compare target.",
        2,
        generated_help_links_en_compare_target,
        1,
        generated_help_sections_en_compare_target,
    },
    {
        "change-date",
        "Date Change Help",
        "prompt.change-date",
        "Enter the new date as `YYYY-MM-DD` or add a time as `YYYY-MM-DD HH:MM[:SS]`.\nPress `F3` to cycle whether the entered value updates the modified time, accessed time, or both.\nTagged date edits use the same prompt and scope cycle.",
        3,
        generated_help_links_en_change_date,
        2,
        generated_help_sections_en_change_date,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        NULL,
        "Directory compares only the current directory.\nLogged tree compares everything already logged under the current directory and never auto-logs unopened branches.\nExternal viewer hands the paths to your configured diff tool instead of tagging results inside ytnova.",
        1,
        generated_help_links_en_compare_scope,
        1,
        generated_help_sections_en_compare_scope,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        NULL,
        "`Size` is the quickest rough check.\n`size+date` is usually better because it also compares last-modified time.\n`Hash` is the strongest check: ytnova reads both files and compares their actual content, so it is slower.",
        1,
        generated_help_links_en_compare_basis,
        1,
        generated_help_sections_en_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        NULL,
        "Choose which compare result should be tagged after the compare.\n`diFferent` tags mismatches, `Unique` tags files that exist only on the selected side, and the other choices tag only that one result.",
        1,
        generated_help_links_en_compare_results,
        1,
        generated_help_sections_en_compare_results,
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "Type the shell command you want to run. Put `{}` where the selected file path should go.\nLeave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\nUse `Ctrl-X` to repeat the same command once per tagged file.",
        2,
        generated_help_links_en_execute_file,
        1,
        generated_help_sections_en_execute_file,
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "Type the shell command you want to run. Put `{}` where the selected directory path should go.\nLeave `{}` unquoted so ytnova can expand it and quote the resulting path safely.\nUse `Ctrl-X` to repeat the same command once per tagged file in the active list.",
        2,
        generated_help_links_en_execute_dir,
        1,
        generated_help_sections_en_execute_dir,
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Enter plain search text only.\nytnova builds `grep -i -- PATTERN {}` for you.\nOnly tagged files are searched, and non-matches are untagged.",
        2,
        generated_help_links_en_search_tagged,
        1,
        generated_help_sections_en_search_tagged,
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Use `.tar`, `.tar.gz` or `.tgz`, `.tar.bz2` or `.tbz2`, `.tar.xz` or `.txz`, or `.zip`.\nWhen tags exist, the tagged set wins.\nWhen nothing is tagged, ytnova archives the current file or directory selection.",
        2,
        generated_help_links_en_create_archive,
        1,
        generated_help_sections_en_create_archive,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output exports file content to a file path or a printer command.\nChoose file or hardcopy first, then give the final destination.\nOn file output, `F3` cycles `Raw`, `Framed`, and `Page break`.\n`Framed` and `Page break` ask for a separator before the final file path.",
        3,
        generated_help_links_en_output,
        2,
        generated_help_sections_en_output,
    },
    {
        "output-format",
        "Output Format Help",
        NULL,
        "`Raw` writes content with no extra framing.\n`Framed` adds per-file headings or footers.\n`Page break` inserts a separator between files and skips a trailing separator at the end.",
        1,
        generated_help_links_en_output_format,
        1,
        generated_help_sections_en_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Choose file or hardcopy first, then enter that destination exactly as ytnova should use it.\nFile output writes exported text to a path.\nBare filenames go to `CWD`, the current working directory.\nHardcopy sends raw exported text to a printer command.\nUse helpers such as `lpr`, `lp`, or `cat > /dev/lp1`.\nPress `F3` on the file destination prompt to cycle `Raw`, `Framed`, and `Page break`.\n`Framed` and `Page break` later ask for a separator.",
        3,
        generated_help_links_en_output_destination,
        2,
        generated_help_sections_en_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "This prompt appears only when `F3` selects `Framed` or `Page break`.\nLeave it blank to accept the default triple-backtick fence.\nRaw output skips this prompt.",
        1,
        generated_help_links_en_output_separator,
        1,
        generated_help_sections_en_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall gathers every file in the current logged volume.\nUse this page for the aggregate-view rules and its footer commands.",
        7,
        generated_help_links_en_showall,
        4,
        generated_help_sections_en_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global gathers files from every logged volume.\nUse this page for the cross-volume rules and its footer commands.",
        7,
        generated_help_links_en_global,
        4,
        generated_help_sections_en_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "Navigation follows the usual list keys except for the preview-only keys on this page.\nThis page explains the commands that still work without leaving preview.",
        7,
        generated_help_links_en_f7,
        2,
        generated_help_sections_en_f7,
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "Split mode keeps both panels live.\nNavigation follows the usual keys except for the split-only rules on this page.",
        3,
        generated_help_links_en_f8,
        1,
        generated_help_sections_en_f8,
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "This page explains the live split-directory footer commands.\nUse `Tab` to change the active panel, and remember that copy, move, and compare default to the inactive panel.\nEach panel keeps its own selection, view, tags, volume, and restore state.",
        7,
        generated_help_links_en_f8_dir,
        2,
        generated_help_sections_en_f8_dir,
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "This page explains the live split-file footer commands.\nUse `Tab` to change the active panel, and remember that copy, move, and compare default to the inactive panel.\nEach panel keeps its own selection, view, tags, volume, and restore state.",
        8,
        generated_help_links_en_f8_file,
        2,
        generated_help_sections_en_f8_file,
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Use `Up` and `Down` to choose an entry.\nUse `Left` and `Right` to scroll a long entry.\nUse `P` to pin or unpin, `D` to delete, `Enter` to accept, and `Esc` to cancel.",
        1,
        generated_help_links_en_history_dialog,
        1,
        generated_help_sections_en_history_dialog,
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Use `Up` and `Down` to choose a loaded volume.\nUse `Enter` to switch to it.\nUse `D` to release it, unless it is the last one.\nUse `Esc` to leave the menu.",
        1,
        generated_help_links_en_volume_menu,
        1,
        generated_help_sections_en_volume_menu,
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Use Enter to select the highlighted preset.\nAfter launch, ytnova keeps running and the application continues on its own.\nUse E to edit the applications catalog that backs application presets.\nUse Esc to cancel the menu.\nUse {} for the file or folder currently selected in ytnova.\nUse {input} for the text you type when the preset asks for extra input.",
        1,
        generated_help_links_en_applications_menu,
        1,
        generated_help_sections_en_applications_menu,
    },
    {
        "f2-picker",
        "F2 Picker Help",
        "dialog.f2-picker",
        "Use `Up` and `Down` to move.\nUse `Right` to expand or enter the first child, and `Left` to collapse or go to the parent.\nUse `<` and `>` to cycle loaded volumes.\nUse `L` to log a new path.\nUse `` ` `` to toggle dotfiles.\nUse `Enter` to select the highlighted directory, and `Esc` to cancel.",
        2,
        generated_help_links_en_f2_picker,
        1,
        generated_help_sections_en_f2_picker,
    },
};

static const size_t generated_help_topic_count_en = 39;

static const GeneratedHelpLink generated_help_links_de_intro[] = {
    {"Applications", "applications-menu"},
    {"Archive Directory", "archive-dir"},
    {"Archive File", "archive-file"},
    {"Command-line Editing", "command-line-editing"},
    {"Compare", "compare"},
    {"Compare Basis", "compare-basis"},
    {"Compare Result", "compare-results"},
    {"Compare Scope", "compare-scope"},
    {"Compare Target", "compare-target"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Create Archive", "create-archive"},
    {"Date Change", "change-date"},
    {"Directory", "dir"},
    {"F10 Config", "f10"},
    {"F2 Picker", "f2-picker"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
    {"File", "file"},
    {"Filter", "filter"},
    {"Global", "global"},
    {"History", "history-dialog"},
    {"List Jump", "list-jump"},
    {"Navigation", "navigation"},
    {"Output", "output"},
    {"Output Destination", "output-destination"},
    {"Output Format", "output-format"},
    {"Output Separator", "output-separator"},
    {"Search Tagged", "search-tagged"},
    {"Shared Commands", "shared-commands"},
    {"Showall", "showall"},
    {"Tagged", "tagged"},
    {"Theming", "theming"},
    {"Vi Keys", "vi-keys"},
    {"Volume", "volume-menu"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_intro[] = {
    {"Zweck", "Benutze `F1` für die Aufgabe vor dir und nicht als ein riesiges Handbuch.\nLokale Seiten beantworten zuerst die aktuelle Frage. Gemeinsame Themen halten die Regeln, die sich wiederholen."},
    {"Benutzung des Index", "Diese Liste bleibt alphabetisch, damit du sie schnell überfliegen kannst.\nWenn du den aktuellen Bildschirm kennst, starte dort. Sonst nimm das Thema, das zu deiner Frage passt.\nÖffne ein Thema, lies die kurze Antwort und gehe dann mit `Left` zurück, ohne deinen Platz zu verlieren."},
};

static const GeneratedHelpLink generated_help_links_de_navigation[] = {
    {"Directory", "dir"},
    {"File", "file"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F2 Picker", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_navigation[] = {
    {"Gemeinsame Listentasten", "* **Up/Down**: Eine Zeile nach oben oder unten bewegen.\n* **Page Up/Page Down**: Eine Bildschirmseite nach oben oder unten bewegen.\n* **Home/End**: Direkt zur ersten oder letzten sichtbaren Zeile springen.\n* **Enter**: Das gewählte Element öffnen. In gekoppelten Baum-/Dateiansichten kann es stattdessen in die passende andere Ansicht wechseln.\n* **Left/Right**: `Right` öffnet oder erweitert meistens. `Left` geht meistens zurück oder klappt zu. Wenn sich das ändert, sagt es die lokale Seite."},
    {"Gemeinsamer Sprung", "* **/**: Drücke `/`, um in der aktuellen Liste einen Sprung nach Namen zu starten.\n* **Buchstaben tippen**: Tippe genug vom Namen ein, um den Treffer einzugrenzen.\n* **Enter**: Auf dem besten sichtbaren Treffer landen.\n* **Baum-Folge**: In Baumansichten wiederholst du den Sprung im nächsten Verzeichnis, wenn du tiefer gehen willst.\n* **Esc**: Den Sprung abbrechen und die ursprüngliche Auswahl wiederherstellen."},
    {"Tasten im Hilfe-Popup", "* **Up/Down**: Zwischen wählbaren Zeilen oder Links bewegen.\n* **Page Up/Page Down**: Längere Hilfeseiten scrollen.\n* **Home/End**: Zum Anfang oder Ende der aktuellen Hilfeseite springen.\n* **Enter/Right**: Das gewählte Hilfeelement oder verlinkte Thema öffnen.\n* **Left**: Einen Schritt zurückgehen.\n* **Esc/Quit**: Das Popup schließen."},
    {"Lokale Ausnahmen", "* **Archive**: `\\` springt zur Archivwurzel oder verlässt das Archiv, wenn du dort schon bist.\n* **Split**: `Tab` wechselt das aktive Panel.\n* **Preview**: `Shift-Up/Down`, `Shift-PgUp/PgDn`, `Shift-Home/End`, `Ctrl-P` und `Ctrl-N` scrollen die Vorschau."},
};

static const GeneratedHelpLink generated_help_links_de_list_jump[] = {
    {"Directory", "dir"},
    {"File", "file"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_list_jump[] = {
    {"Sprungmodell", "`/` öffnet einen Live-Sprung-Prompt nur für die aktuelle Liste.\nBaum- und Verzeichnisansichten springen zwischen sichtbaren Verzeichnisnamen. Dateiorientierte Ansichten springen zwischen den sichtbaren Dateizeilen dieser Oberfläche."},
    {"Übernehmen oder abbrechen", "* **Buchstaben tippen**: Zum besten aktuellen Treffer springen, während du tippst.\n* **Enter**: Auf dem aktuellen Treffer landen und dort bleiben.\n* **Esc**: Den Sprung abbrechen und die ursprüngliche Auswahl wiederherstellen.\n* **Bereichswechsel**: Filter, Showall, Global, Archive und Split-Modus ändern, welche sichtbare Liste `/` durchsucht."},
};

static const GeneratedHelpLink generated_help_links_de_shared_commands[] = {
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"Applications menu", "applications-menu"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_shared_commands[] = {
    {"Gemeinsame Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Oberfläche öffnen.\n* **F5**: Die aktuelle Ansicht aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung der aktiven Ansicht ändern.\n* **F7**: Die Vorschau für den aktiven Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen.\n* **Esc**: Das aktuelle Overlay, den Prompt oder das Popup verlassen."},
};

static const GeneratedHelpLink generated_help_links_de_tagged[] = {
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Filter help", "filter"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_tagged[] = {
    {"Grundlagen", "Markierungen sind eine Arbeitsmenge. Sie sind weder eine zweite Zwischenablage noch eine gespeicherte Suche.\nBaue eine Menge auf, arbeite damit, verenge sie weiter und leere oder invertiere sie danach."},
    {"Häufige Abläufe", "* **Tag** und **Untag**: Die aktuelle Zeile zur Arbeitsmenge hinzufügen oder daraus entfernen.\n* **Invert Tags**: Den Markierungszustand im aktuellen sichtbaren Bereich umkehren.\n* **Filter**: Mit `F` und danach `Tab` zwischen allen Zeilen und nur markierten Zeilen umschalten, ohne die Markierungen selbst zu ändern.\n* **Copy tagged** und **Move tagged**: Die ganze markierte Menge an ein Ziel schicken.\n* **View tagged**: Die markierten Dateien nacheinander öffnen.\n* **Search tagged**: Nur die markierten Dateien durchsuchen und Nicht-Treffer wieder entmarkieren.\n* **Archive**: Zuerst die markierte Menge archivieren. Wenn nichts markiert ist, fällt Archivieren auf die aktuelle Auswahl zurück."},
};

static const GeneratedHelpLink generated_help_links_de_command_line_editing[] = {
    {"VI-Tasten", "vi-keys"},
    {"Historie", "history-dialog"},
    {"F2-Auswahl", "f2-picker"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_command_line_editing[] = {
    {"Bearbeitungstasten", "* **Left/Right**: Innerhalb des aktuellen Prompt-Textes bewegen.\n* **Home/End**: Zum Anfang oder Ende des Prompt-Textes springen.\n* **Backspace/Delete**: Das Zeichen links oder rechts vom Cursor löschen.\n* **Enter**: Den aktuellen Wert übernehmen.\n* **Esc**: Abbrechen, ohne den Prompt zu übernehmen."},
    {"Gemeinsame Hilfen", "* **Up**: Die Prompt-Historie öffnen oder darin weitergehen, wenn dieser Prompt eine Historie hat.\n* **F2**: Einen Browser oder eine Auswahl öffnen, wenn der aktuelle Prompt Browsing unterstützt.\n* **F1**: Syntax- oder Bereichsregeln zeigen, die nur für diesen Prompt gelten."},
};

static const GeneratedHelpLink generated_help_links_de_copy_move_targets[] = {
    {"Directory mode", "dir"},
    {"File mode", "file"},
    {"Archive File Help", "archive-file"},
    {"F8 split", "f8"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_copy_move_targets[] = {
    {"Zielformen", "* **Verzeichnispfad**: Die ursprünglichen Namen unter einem anderen Verzeichnis behalten.\n* **Ersatzname**: Ein ausgewähltes Objekt beim Kopieren oder Verschieben umbenennen.\n* **Wildcard-Muster**: Jeden ausgewählten Basisnamen per Muster umschreiben, zum Beispiel `copy-*` oder `*.bak`."},
    {"Prompt-Ablauf", "* **Erster Prompt**: Ersatzname oder Wildcard-Muster wählen.\n* **Zweiter Prompt**: Zielverzeichnis wählen.\n* **Markiertes Kopieren/Verschieben**: Benutzt dieselbe Zielsprache wie bei Einzelobjekten.\n* **Pathcopy**: Erhält den Pfad der gewählten Datei relativ zur aktuellen Volume-Wurzel.\n* **Split-Standardziel**: Belegt das Verzeichnis des inaktiven Panels vor, aber du kannst diesen Standard noch ersetzen.\n* **Archivgestützte Pfade**: Behalten dasselbe Zielmodell auch dann, wenn Extraktion beteiligt ist."},
    {"Sicherheitsprüfungen", "* **Prompt-Anzahl**: Danach dürfen nur echte Sicherheitsabfragen folgen.\n* **Konfliktdetails**: Überschreibabfragen zeigen wenn möglich Größen- und Zeitdaten von Quelle und Ziel.\n* **Verzeichnisablauf**: Verzeichniskopien und -verschiebungen starten sofort nach dem akzeptierten Ziel; es gibt keine zusätzliche Jetzt-kopieren- oder Jetzt-verschieben-Abfrage."},
};

static const GeneratedHelpLink generated_help_links_de_vi_keys[] = {
    {"Navigation", "navigation"},
    {"Bearbeitung in Eingabezeilen", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_vi_keys[] = {
    {"Navigations-Umlegung", "Mit `VI_KEYS=1` werden `h`, `j`, `k` und `l` zu `Left`, `Down`, `Up` und `Right`.\n`Ctrl-U` und `Ctrl-D` werden zu Seite hoch und Seite runter."},
    {"Befehlskollisionen", "Befehle, die diese Kleinbuchstaben stehlen würden, gehen aus dem Weg.\nBeispiele sind `J compare`, `K volume`, `D delete tagged` und `U untag all`, wo diese Aktionen existieren."},
};

static const GeneratedHelpLink generated_help_links_de_f10[] = {
    {"Theming", "theming"},
    {"Gemeinsame Befehle", "shared-commands"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f10[] = {
    {"Konfigurationsoberfläche", "Benutze `F10`, wenn du dauerhaftes Verhalten ändern willst, statt nur die aktuelle Auswahl zu beeinflussen.\nProfileinstellungen, Befehlsbeschriftungen, Themes und Reload leben hier."},
    {"Zugehörige Dateien", "`ytnova.conf` enthält Profileinstellungen.\n`commands.conf` enthält Benutzerlabels und Bindings für Befehle.\n`themes.conf` enthält Theme-Auswahl und Theme-Overrides."},
};

static const GeneratedHelpLink generated_help_links_de_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_theming[] = {
    {"Theme-Modell", "Themes setzen semantische Rollen wie `footer`, `help`, `help_link`, `selection`, `picker` und `warning`.\nSo bleibt eine Theme-Änderung im ganzen UI konsistent."},
    {"Bearbeitungspfad", "Benutze `F10`, um den Bearbeitungspfad für Theme oder Konfiguration zu öffnen.\nAchte zuerst auf lesbare Flächen mit hoher Nutzung: Auswahl, Picker, Footer und Hilfe."},
};

static const GeneratedHelpLink generated_help_links_de_dir[] = {
    {"Navigation", "navigation"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_dir[] = {
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt das Panel immer auf die schlichte Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Wenn du also die Baumansicht änderst, ändert sich auch das Dateifenster dieses Panels. Mit `SEPARATE_DIR_FILE_VIEWS=1` trennst du beide wieder.\n* **Baum gegen Dateifenster**: Im Verzeichnisfokus ändern `5`, `7`, `8` und `9` nicht die Baumzeilen. Sie ändern das kleine Dateifenster dieses Panels.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Showall**: Die Sammelliste für das aktuelle Volume öffnen.\n* **Global**: Die Sammelliste über mehrere Volumes öffnen.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer in diesem Baum zu landen.\n* **Dotfiles**: Versteckte Namen im aktuellen Baum ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Den aktuellen Dateibereich filtern. Im Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n* **Tag**: Dateien im gewählten Verzeichnisbereich markieren.\n* **Untag**: Dateien im gewählten Verzeichnisbereich entmarkieren.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Verzeichnisaktionen", "* **Attributes**: Das Attribut-Untermenü für das gewählte Verzeichnis öffnen.\n* **Copy**: Den gewählten Verzeichniszweig kopieren.\n* **MoveDir**: Den gewählten Verzeichniszweig verschieben.\n* **Compare**: Das gewählte Verzeichnis, den aktuellen geloggten Baum oder ein anderes Ziel vergleichen.\n* **Output**: Die aktuelle Auswahl über den Output-Ablauf exportieren.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Verzeichnispfad stehen soll, und lasse `{}` unzitiert, damit ytnova den Pfad sicher quoten kann.\n* **Pipe**: Einen Shell-Befehl eingeben. ytnova führt ihn im gewählten Verzeichnis aus und schickt die sichtbaren passenden Namen zeilenweise an seine Standard-Eingabe.\n* **Makedir**: Ein neues Verzeichnis anlegen.\n* **New File**: Eine neue leere Datei im aktuellen Verzeichnis anlegen.\n* **Rename**: Das gewählte Verzeichnis umbenennen.\n* **Delete**: Das gewählte Verzeichnis löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen oder einen bereits geloggten Pfad von oben neu laden.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Verzeichnis-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Verzeichnisoberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_file[] = {
    {"Navigation", "navigation"},
    {"Markiert", "tagged"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_file[] = {
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt immer zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Eine Baum-Änderung ändert also auch das Dateifenster dieses Panels. Mit `SEPARATE_DIR_FILE_VIEWS=1` machst du beide wieder unabhängig.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **Zusatzstatus**: `5`, `7`, `8` und `9` stapeln sich nicht im Statistiklabel; dort steht immer nur der eine sichtbare Zusatzstatus.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Sort**: Die Sortierung der aktuellen Dateiliste ändern.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Dateien im aktuellen Bereich ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle Liste filtern. `Ctrl-S` durchsucht nur markierte Dateien. Im Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n* **Tag**: Die gewählte Datei markieren, und `Ctrl-T` markiert alle sichtbaren Dateien im aktuellen Bereich.\n* **Untag**: Die Markierung der gewählten Datei entfernen, und `Ctrl-U` löscht Markierungen im aktuellen Bereich.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Dateiaktionen", "* **Attributes**: Das Attribut-Untermenü für die gewählte Datei öffnen.\n* **Copy**: `C` kopiert die gewählte Datei, und `Ctrl-K` kopiert die markierte Menge über denselben Prompt.\n* **Move**: `M` verschiebt die gewählte Datei, und `Ctrl-N` verschiebt die markierte Menge über denselben Prompt.\n* **View**: Die gewählte Datei anzeigen, und `Ctrl-V` zeigt die markierten Dateien nacheinander an.\n* **Edit**: Die gewählte Datei im konfigurierten Editor öffnen.\n* **Hex**: Die gewählte Datei in der Hex-Ansicht öffnen.\n* **Compare**: Die gewählte Datei mit einer anderen Datei vergleichen.\n* **Output**: Die Auswahl exportieren. `Ctrl-O` benutzt dieselben Prompts für die markierte Menge, und `Ctrl-W` bleibt ein Legacy-Alias.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Dateipfad stehen soll, lasse `{}` unzitiert, damit ytnova ihn sicher quoten kann, und benutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n* **Pathcopy**: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n* **New File**: Eine neue leere Datei anlegen.\n* **Rename**: Die gewählte Datei umbenennen.\n* **Delete**: Die gewählte Datei löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne den Dateimodus zu verlassen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Datei-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Dateioberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_archive_dir[] = {
    {"Navigation", "navigation"},
    {"Filter", "filter"},
    {"Compare", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_archive_dir[] = {
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Archivbaum gegen Dateifenster**: Im Archiv-Verzeichnisfokus ändern `5`, `7` und `8` nicht die Baumzeilen. Sie ändern das Archiv-Dateifenster dieses Panels.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Archivzeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: In Archiven unbenutzt.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Showall**: Die Dateiliste für das aktuelle Archiv öffnen.\n* **Global**: Archiv-Ergebnisse in die Sammelliste über mehrere Volumes mischen.\n* **Root/Exit**: `\\` springt zur Archivwurzel oder verlässt das Archiv, wenn du dort schon bist.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Archiveinträge ein- oder ausblenden, wenn diese Ansicht sie zeigt."},
    {"Arbeitsmenge", "* **Filter**: Den aktuellen archivgestützten Dateibereich filtern.\n* **Tag**: Dateien im aktuellen virtuellen Verzeichnisbereich markieren.\n* **Untag**: Markierungen im aktuellen virtuellen Verzeichnisbereich entfernen."},
    {"Archiv-Verzeichnisaktionen", "* **Compare**: Das gewählte Archivverzeichnis oder den aktuellen Archivbaum vergleichen.\n* **Output**: Die aktuelle archivgestützte Auswahl über den Output-Ablauf exportieren.\n* **Pipe**: Einen Shell-Befehl eingeben. ytnova schickt die sichtbaren passenden Namen aus dem gewählten Archivverzeichnis zeilenweise an seine Standard-Eingabe.\n* **Log**: Ein weiteres Verzeichnis oder eine Archivdatei loggen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Makedir**: Ein Verzeichnis anlegen, sofern das Archivformat es unterstützt.\n* **Rename**: Den gewählten Archiv-Verzeichniseintrag umbenennen.\n* **Delete**: Den gewählten Archiv-Verzeichniseintrag löschen.\n* **Quit**: ytnova beenden."},
    {"Archiv-Verzeichnis-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Archiv-Verzeichnisoberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_archive_file[] = {
    {"Navigation", "navigation"},
    {"Markiert", "tagged"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_archive_file[] = {
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Archivzeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: In Archiven unbenutzt.\n* **Zusatzstatus**: `5`, `7` und `8` ersetzen einander im Statistiklabel, statt sich zu stapeln.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Sort**: Die Sortierung der aktuellen Archiv-Dateiliste ändern.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Archiveinträge ein- oder ausblenden, wenn diese Ansicht sie zeigt."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle archivgestützte Liste filtern. `Ctrl-S` durchsucht nur markierte Archiveinträge und entmarkiert Nicht-Treffer.\n* **Tag**: Den gewählten Archiveintrag markieren, und `Ctrl-T` markiert alle sichtbaren Zeilen im aktuellen Bereich.\n* **Untag**: Die Markierung des gewählten Archiveintrags entfernen, und `Ctrl-U` löscht Archiv-Markierungen im aktuellen Bereich.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren."},
    {"Archiv-Dateiaktionen", "* **Copy**: `C` kopiert den gewählten Archiveintrag, und `Ctrl-K` kopiert die markierten Archiveinträge über denselben Prompt.\n* **Move**: `M` verschiebt den gewählten Archiveintrag, und `Ctrl-N` verschiebt die markierten Archiveinträge über denselben Prompt.\n* **View**: Den gewählten Archiveintrag anzeigen, und `Ctrl-V` zeigt die markierten Archiveinträge nacheinander an.\n* **Hex**: Den gewählten Archiveintrag in der Hex-Ansicht öffnen.\n* **Compare**: Den gewählten Archiveintrag mit einer anderen Datei vergleichen.\n* **Output**: Den gewählten Archiveintrag über den Output-Ablauf exportieren.\n* **Pathcopy**: Den gewählten Archiveintrag kopieren und seinen relativen Pfad erhalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt des gewählten Archiveintrags über Standard-Eingabe zuführen.\n* **Rename**: Den gewählten Archiveintrag umbenennen.\n* **Delete**: Den gewählten Archiveintrag löschen.\n* **Log**: Ein weiteres Verzeichnis oder eine Archivdatei loggen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Archiv-Datei-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Archiv-Dateioberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_filter[] = {
    {"Tagged", "tagged"},
    {"Showall", "showall"},
    {"Global", "global"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_filter[] = {
    {"Häufige Begriffe", "* `*` — alle Dateien anzeigen\n* `*.c` — Glob-Muster\n* `*.c,*.h` — mehrere Glob-Muster\n* `-*.o` — Treffer ausschließen\n* `:r` — Attributtest\n* `:x` — Attributtest\n* `>2023-01-01` — Datumstest\n* `>1M` — Größentest"},
    {"Kombinationsregeln", "Stapele Begriffe mit Kommas in einem Filter, zum Beispiel `*.c,-*.tmp` oder `*.log,>2024-01-01,-debug*`.\nZitiere den Filter am Shell-Prompt nur dann, wenn deine Shell ihn vorher expandieren würde."},
    {"Bereich", "Der Filter wirkt immer auf die aktuelle Dateilisten-Familie.\nDas kann eine normale Dateiliste, eine Archiv-Dateiliste, Showall oder Global sein.\nMit `Tab` schaltest du zwischen allen Dateien und nur markierten Dateien um, wenn der Markierungsbereich verfügbar ist.\nWenn der Markierungsbereich aktiv ist, zeigt der Prompt `FILTER [tagged only]:`."},
};

static const GeneratedHelpLink generated_help_links_de_compare[] = {
    {"File mode", "file"},
    {"Directory mode", "dir"},
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_compare[] = {
    {"Vergleichsablauf", "Wähle zuerst das Ziel.\nWähle dann den Vergleichsbereich, wenn die Quelle ein Verzeichnis ist.\nWähle dann die Vergleichsbasis, wenn die Laufzeit mehr als eine Basis anbietet.\nZum Schluss wähle die Ergebnisklasse, die auf der Quellseite markiert werden soll."},
    {"Vergleichsregeln", "* Geloggter-Baum-Vergleich benutzt nur bereits geloggte Inhalte. Ungeöffnete `+`-Äste werden nicht automatisch geloggt.\n* `FILEDIFF` darf `%1` und `%2` verwenden. Fehlen diese Platzhalter, hängt ytnova Quell- und Zielpfad an den Hilfsbefehl an.\n* Ein externer Verzeichnis- oder Baumvergleich startet `DIRDIFF` oder `TREEDIFF`, statt Laufzeitergebnisse zu markieren.\n* Es gibt keinen separaten Modus „markierte Dateien vergleichen“."},
};

static const GeneratedHelpLink generated_help_links_de_compare_target[] = {
    {"Compare Help", "compare"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_compare_target[] = {
    {"Zielregeln", "Gib genau einen Pfad ein.\nDer Vergleichsbereich entscheidet danach, ob dieser eine Pfad als Dateiziel, Verzeichnisziel oder geloggtes Baumziel behandelt wird."},
};

static const GeneratedHelpLink generated_help_links_de_compare_scope[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_compare_scope[] = {
    {"Bereichswahl", "Benutze `Directory` für eine Ebene.\nBenutze `Logged tree` für den aktuell geloggten rekursiven Baum.\nBenutze `External viewer`, wenn du ein externes Diff-Werkzeug statt markierter Vergleichsergebnisse in ytnova willst."},
};

static const GeneratedHelpLink generated_help_links_de_change_date[] = {
    {"File mode", "file"},
    {"Directory mode", "dir"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_change_date[] = {
    {"Bereichswahl", "`modified` ändert nur den Zeitstempel der letzten Änderung.\n`accessed` ändert nur den Zugriffszeitstempel.\n`both` schreibt den eingegebenen Wert in beide Zeitstempel."},
    {"Formatregeln", "Wenn du den Zeitanteil weglässt, behält ytnova Stunde, Minute und Sekunde aus dem aktuellen Wert bei.\nBenutze `Up` für die Prompt-Historie und `Esc`, um ohne Änderung abzubrechen."},
};

static const GeneratedHelpLink generated_help_links_de_compare_basis[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_compare_basis[] = {
    {"Basiswahl", "Wähle die billigste Basis, die deine eigentliche Frage beantwortet.\nBenutze `Hash` nur dann, wenn Metadaten nicht zuverlässig genug sind."},
};

static const GeneratedHelpLink generated_help_links_de_compare_results[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_compare_results[] = {
    {"Ergebnismarkierung", "Der Vergleich überschreibt keine Dateien.\nEr markiert die gewählte Ergebnisklasse auf der aktiven Quellseite, damit du diese Teilmenge danach prüfen, kopieren, verschieben oder archivieren kannst."},
};

static const GeneratedHelpLink generated_help_links_de_execute_file[] = {
    {"File mode", "file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_execute_file[] = {
    {"Platzhalterregeln", "`{}` steht für genau einen ausgewählten Dateipfad.\nWenn du den markierten Wiederholungspfad benutzt, wird derselbe Befehl einmal pro markierter Datei wiederholt."},
};

static const GeneratedHelpLink generated_help_links_de_execute_dir[] = {
    {"Directory mode", "dir"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_execute_dir[] = {
    {"Platzhalterregeln", "`{}` steht für den aktuellen Verzeichnispfad.\nDer markierte Wiederholungspfad läuft trotzdem über markierte Dateien aus der aktiven Liste, nicht über markierte Verzeichnisse aus irgendeiner anderen Stelle."},
};

static const GeneratedHelpLink generated_help_links_de_search_tagged[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_search_tagged[] = {
    {"Regeln für markierte Suche", "Baue zuerst eine markierte Arbeitsmenge auf.\nDurchsuche dann nur diese Menge. Das Ergebnis ist wieder eine kleinere markierte Menge, weil Dateien ohne Treffer ihre Markierung verlieren."},
};

static const GeneratedHelpLink generated_help_links_de_create_archive[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_create_archive[] = {
    {"Archivierungsregeln", "Verzeichnisauswahlen werden rekursiv archiviert.\nArchivieren nimmt zuerst die markierte Menge, weil Markieren der normale Weg ist, um einen benutzerdefinierten Archivsatz aufzubauen."},
};

static const GeneratedHelpLink generated_help_links_de_output[] = {
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_output[] = {
    {"Output-Modell", "`Output` ist ein Exportablauf, kein Editor.\nEr kann reinen Inhalt, gerahmten Inhalt oder durch Seitentrenner getrennten Inhalt ausgeben.\nEr kann diese Ausgabe auch an einen Druckbefehl schicken statt an einen Dateipfad."},
    {"Prompt-Reihenfolge", "Wähle zuerst Datei oder Hardcopy.\nIm Datei-Ziel-Prompt schaltet `F3` zwischen `Raw`, `Framed` und `Page break` um.\nWenn `Framed` oder `Page break` aktiv ist, wähle zuerst den Trenner und gib danach den endgültigen Dateipfad ein.\nHardcopy fragt nur nach dem Druckbefehl."},
};

static const GeneratedHelpLink generated_help_links_de_output_format[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_output_format[] = {
    {"Formatwahl", "Benutze `Raw`, wenn ein anderes Werkzeug die Ausgabe parsen soll.\nBenutze `Framed` oder `Page break`, wenn ein Mensch den exportierten Satz lesen soll."},
};

static const GeneratedHelpLink generated_help_links_de_output_destination[] = {
    {"Output Help", "output"},
    {"Output Format Help", "output-format"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_output_destination[] = {
    {"Zielarten", "* **Dateiausgabe**: Exportierten Text in einen Pfad schreiben.\n* **`CWD`**: Das aktuelle Arbeitsverzeichnis für einfache Dateinamen benutzen.\n* **Hardcopy**: Rohe Ausgabe an einen Shell-Druckbefehl wie `lpr`, `lp` oder `cat > /dev/lp1` schicken."},
    {"Formatumschaltung", "`F3` ist nur im Datei-Ziel-Prompt verfügbar.\nWenn `Framed` oder `Page break` gewählt ist, fragt ytnova zuerst den Trenner und kehrt dann zum Datei-Prompt zurück."},
};

static const GeneratedHelpLink generated_help_links_de_output_separator[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_output_separator[] = {
    {"Trennerregeln", "Der Trenner wird zwischen Dateien für den aktuellen Framed- oder Page-break-Export wiederverwendet.\nNach der letzten Datei wird er nicht angehängt."},
};

static const GeneratedHelpLink generated_help_links_de_showall[] = {
    {"Navigation", "navigation"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_showall[] = {
    {"Ansicht und Bereich", "* **Scope**: Showall listet jede Datei nur innerhalb des aktuellen geloggten Volumes auf.\n* **Return**: Zum zuvor gewählten Verzeichnis zurückkehren.\n* **Open owner**: Zum Besitzerverzeichnis der gewählten Datei im aktuellen Volume springen.\n* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Mit `SEPARATE_DIR_FILE_VIEWS=1` trennst du Showall-/Dateifenster und Baum-/Verzeichnis-Grundansichten wieder.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Sort**: `S` ändert die Sortierung, ohne Showall zu verlassen.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Dateien in der aktuellen Showall-Ergebnismenge ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle Showall-Ergebnismenge filtern. `Ctrl-S` durchsucht dort nur markierte Dateien. Im Prompt verengt `Tab` dieselbe Ergebnismenge auf nur markierte Zeilen.\n* **Tag**: Die gewählte Datei markieren, und `Ctrl-T` markiert alle sichtbaren Dateien in der aktuellen Showall-Ergebnismenge.\n* **Untag**: Die Markierung der gewählten Datei entfernen, und `Ctrl-U` löscht Markierungen in der aktuellen Showall-Ergebnismenge.\n* **Invert Tags**: Den Markierungszustand in der sichtbaren Showall-Ergebnismenge umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Dateiaktionen", "* **Attributes**: Das Attribut-Untermenü für die gewählte Datei öffnen.\n* **Copy**: `C` kopiert die gewählte Datei, und `Ctrl-K` kopiert die markierte Menge über denselben Prompt.\n* **Move**: `M` verschiebt die gewählte Datei, und `Ctrl-N` verschiebt die markierte Menge über denselben Prompt.\n* **View**: Die gewählte Datei anzeigen, und `Ctrl-V` zeigt die markierten Dateien nacheinander an.\n* **Edit**: Die gewählte Datei im konfigurierten Editor öffnen.\n* **Hex**: Die gewählte Datei in der Hex-Ansicht öffnen.\n* **Compare**: Die gewählte Datei mit einer anderen Datei vergleichen.\n* **Output**: Die Auswahl exportieren. `Ctrl-O` benutzt dieselben Prompts für die markierte Menge, und `Ctrl-W` bleibt ein Legacy-Alias.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Dateipfad stehen soll, lasse `{}` unzitiert, damit ytnova ihn sicher quoten kann, und benutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n* **Pathcopy**: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n* **New File**: Eine neue leere Datei anlegen.\n* **Rename**: Die gewählte Datei umbenennen.\n* **Delete**: Die gewählte Datei löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne Showall zu verlassen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Showall-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktuelle Showall-Oberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_global[] = {
    {"Navigation", "navigation"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_global[] = {
    {"Ansicht und Bereich", "* **Scope**: Global listet Dateien aus allen geloggten Volumes auf.\n* **Return**: Zum zuvor gewählten Verzeichnis zurückkehren.\n* **Open owner**: Zum Besitzerverzeichnis der gewählten Datei springen, auch wenn es unter einer anderen Volume-Wurzel liegt.\n* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Mit `SEPARATE_DIR_FILE_VIEWS=1` trennst du Global-/Dateifenster und Baum-/Verzeichnis-Grundansichten wieder.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Sort**: `S` ändert die Sortierung, ohne Global zu verlassen.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Dateien in der aktuellen Global-Ergebnismenge ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle Global-Ergebnismenge filtern. `Ctrl-S` durchsucht dort nur markierte Dateien. Im Prompt verengt `Tab` dieselbe Ergebnismenge auf nur markierte Zeilen.\n* **Tag**: Die gewählte Datei markieren, und `Ctrl-T` markiert alle sichtbaren Dateien in der aktuellen Global-Ergebnismenge.\n* **Untag**: Die Markierung der gewählten Datei entfernen, und `Ctrl-U` löscht Markierungen in der aktuellen Global-Ergebnismenge.\n* **Invert Tags**: Den Markierungszustand in der sichtbaren Global-Ergebnismenge umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Dateiaktionen", "* **Attributes**: Das Attribut-Untermenü für die gewählte Datei öffnen.\n* **Copy**: `C` kopiert die gewählte Datei, und `Ctrl-K` kopiert die markierte Menge über denselben Prompt.\n* **Move**: `M` verschiebt die gewählte Datei, und `Ctrl-N` verschiebt die markierte Menge über denselben Prompt.\n* **View**: Die gewählte Datei anzeigen, und `Ctrl-V` zeigt die markierten Dateien nacheinander an.\n* **Edit**: Die gewählte Datei im konfigurierten Editor öffnen.\n* **Hex**: Die gewählte Datei in der Hex-Ansicht öffnen.\n* **Compare**: Die gewählte Datei mit einer anderen Datei vergleichen.\n* **Output**: Die Auswahl exportieren. `Ctrl-O` benutzt dieselben Prompts für die markierte Menge, und `Ctrl-W` bleibt ein Legacy-Alias.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Dateipfad stehen soll, lasse `{}` unzitiert, damit ytnova ihn sicher quoten kann, und benutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n* **Pathcopy**: Die gewählte Datei kopieren und ihren Pfad relativ zur Volume-Wurzel behalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n* **New File**: Eine neue leere Datei anlegen.\n* **Rename**: Die gewählte Datei umbenennen.\n* **Delete**: Die gewählte Datei löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne Global zu verlassen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Global-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktuelle Global-Oberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Den Split-Screen-Modus umschalten.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_f7[] = {
    {"Navigation", "navigation"},
    {"Markiert", "tagged"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Anwendungsmenue", "applications-menu"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f7[] = {
    {"Vorschau-Regeln", "* **F7**: Zur darunterliegenden Verzeichnis- oder Dateiansicht zurückkehren.\n* **F8**: Split hat keine Wirkung, solange die Vorschau aktiv ist.\n* **F9**: Das Anwendungsmenü öffnen, ohne die Vorschau zu verlassen.\n* **Tab**: Panels nicht wechseln, solange die Vorschau aktiv ist.\n* **Esc**: Die Vorschau sofort verlassen."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle Vorschau-Liste filtern. `Ctrl-S` durchsucht dort nur markierte Dateien.\n* **Tag**: Die gewählte Datei markieren, und `Ctrl-T` markiert alle sichtbaren Dateien im aktuellen Bereich.\n* **Untag**: Die Markierung der gewählten Datei entfernen, und `Ctrl-U` löscht Markierungen im aktuellen Bereich.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Dateien im aktuellen Vorschau-Bereich ein- oder ausblenden."},
    {"Dateiaktionen", "* **Attributes**: Das Attribut-Untermenü für die gewählte Datei öffnen.\n* **Copy**: `C` kopiert die gewählte Datei, und `Ctrl-K` kopiert die markierte Menge über denselben Prompt.\n* **Move**: `M` verschiebt die gewählte Datei, und `Ctrl-N` verschiebt die markierte Menge über denselben Prompt.\n* **View**: Die gewählte Datei anzeigen, und `Ctrl-V` zeigt die markierten Dateien nacheinander an.\n* **Edit**: Die gewählte Datei im konfigurierten Editor öffnen.\n* **Compare**: Die gewählte Datei mit einer anderen Datei vergleichen.\n* **Output**: Die Auswahl exportieren. `Ctrl-O` benutzt dieselben Prompts für die markierte Menge, und `Ctrl-W` bleibt ein Legacy-Alias.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Dateipfad stehen soll, lasse `{}` unzitiert, damit ytnova ihn sicher quoten kann, und benutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n* **Pathcopy**: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n* **New File**: Eine neue leere Datei anlegen, ohne die Vorschau zu verlassen.\n* **Rename**: Die gewählte Datei umbenennen, ohne die Vorschau zu verlassen.\n* **Delete**: Die gewählte Datei löschen, ohne die Vorschau zu verlassen."},
};

static const GeneratedHelpLink generated_help_links_de_f8[] = {
    {"Navigation", "navigation"},
    {"Directory split page", "f8-dir"},
    {"File split page", "f8-file"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f8[] = {
    {"Split-Regeln", "* **F8**: Zum Ein-Panel-Modus zurückkehren.\n* **Tab**: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n* **Target defaults**: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n* **Panel independence**: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand."},
};

static const GeneratedHelpLink generated_help_links_de_f8_dir[] = {
    {"Navigation", "navigation"},
    {"F8-Split", "f8"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f8_dir[] = {
    {"Split-Regeln", "* **F8**: Zum Ein-Panel-Modus zurückkehren.\n* **Tab**: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n* **Target defaults**: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n* **Panel independence**: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand."},
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Wenn du also die Split-Baumansicht änderst, ändert sich auch das Dateifenster dieses Panels. Mit `SEPARATE_DIR_FILE_VIEWS=1` trennst du beide wieder.\n* **Baum gegen Dateifenster**: Im Split-Verzeichnisfokus ändern `5`, `7`, `8` und `9` nicht die Baumzeilen. Sie ändern das Dateifenster des aktiven Panels.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Showall**: Die Sammelliste des aktuellen Volumes für das aktive Panel öffnen.\n* **Global**: Die Sammelliste über mehrere Volumes für das aktive Panel öffnen.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer im aktiven Baum zu landen.\n* **Dotfiles**: Versteckte Namen im aktiven Baum ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Den aktuellen Dateibereich filtern. Im Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n* **Tag**: Dateien im gewählten Verzeichnisbereich markieren.\n* **Untag**: Dateien im gewählten Verzeichnisbereich entmarkieren.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Verzeichnisaktionen", "* **Attributes**: Das Attribut-Untermenü für das gewählte Verzeichnis öffnen.\n* **Copy**: Den gewählten Verzeichniszweig kopieren. Im Split-Modus ist das inaktive Panel das Standardziel.\n* **MoveDir**: Den gewählten Verzeichniszweig verschieben. Im Split-Modus ist das inaktive Panel das Standardziel.\n* **Compare**: Das gewählte Verzeichnis, den aktuellen geloggten Baum oder ein anderes Ziel vergleichen.\n* **Output**: Die aktuelle Auswahl über den Output-Ablauf exportieren.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Verzeichnispfad stehen soll, und lasse `{}` unzitiert, damit ytnova den Pfad sicher quoten kann.\n* **Pipe**: Einen Shell-Befehl eingeben. ytnova führt ihn im gewählten Verzeichnis aus und schickt die sichtbaren passenden Namen zeilenweise an seine Standard-Eingabe.\n* **Makedir**: Ein neues Verzeichnis anlegen.\n* **New File**: Eine neue leere Datei im aktuellen Verzeichnis anlegen.\n* **Rename**: Das gewählte Verzeichnis umbenennen.\n* **Delete**: Das gewählte Verzeichnis löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen oder einen bereits geloggten Pfad von oben neu laden.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Split-Verzeichnis-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Split-Verzeichnisoberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Zum Ein-Panel-Modus zurückkehren.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_f8_file[] = {
    {"Navigation", "navigation"},
    {"F8-Split", "f8"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f8_file[] = {
    {"Split-Regeln", "* **F8**: Zum Ein-Panel-Modus zurückkehren.\n* **Tab**: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n* **Target defaults**: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n* **Panel independence**: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand."},
    {"Ansicht und Bereich", "* **`1`**: Nur Name. Das ist die schlichte Standardansicht.\n* **`2`**: Attribute. In Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n* **`3`**: Eigentümer.\n* **`4`**: Zeiten.\n* **Zurücksetzen**: `1` bringt immer zur schlichten Namensansicht zurück. Wenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n* **Gemeinsam pro Panel**: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt. Eine Split-Baum-Änderung ändert also auch das Dateifenster dieses Panels. Mit `SEPARATE_DIR_FILE_VIEWS=1` machst du beide wieder unabhängig.\n* **`5`**: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n* **`6`**: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten. Die Statistik bleibt lesbar.\n* **`7`**: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`8`**: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n* **`9`**: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n* **Zusatzstatus**: `5`, `7`, `8` und `9` stapeln sich nicht im Statistiklabel; dort steht immer nur der eine sichtbare Zusatzstatus.\n* **`0`**: Derzeit unbenutzt; macht nichts.\n* **Sort**: Die Sortierung der aktiven Dateiliste ändern.\n* **Jump**: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n* **Dotfiles**: Versteckte Dateien im aktuellen Bereich ein- oder ausblenden."},
    {"Arbeitsmenge", "* **Filter**: Die aktuelle Liste filtern. `Ctrl-S` durchsucht nur markierte Dateien. Im Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n* **Tag**: Die gewählte Datei markieren, und `Ctrl-T` markiert alle sichtbaren Dateien im aktuellen Bereich.\n* **Untag**: Die Markierung der gewählten Datei entfernen, und `Ctrl-U` löscht Markierungen im aktuellen Bereich.\n* **Invert Tags**: Den Markierungszustand im sichtbaren Bereich umkehren.\n* **Archive**: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl."},
    {"Dateiaktionen", "* **Attributes**: Das Attribut-Untermenü für die gewählte Datei öffnen.\n* **Copy**: `C` kopiert die gewählte Datei, und `Ctrl-K` kopiert die markierte Menge über denselben Prompt. Im Split-Modus ist das inaktive Panel das Standardziel.\n* **Move**: `M` verschiebt die gewählte Datei, und `Ctrl-N` verschiebt die markierte Menge über denselben Prompt. Im Split-Modus ist das inaktive Panel das Standardziel.\n* **View**: Die gewählte Datei anzeigen, und `Ctrl-V` zeigt die markierten Dateien nacheinander an.\n* **Edit**: Die gewählte Datei im konfigurierten Editor öffnen.\n* **Hex**: Die gewählte Datei in der Hex-Ansicht öffnen.\n* **Compare**: Die gewählte Datei mit einer anderen Datei vergleichen.\n* **Output**: Die Auswahl exportieren. `Ctrl-O` benutzt dieselben Prompts für die markierte Menge, und `Ctrl-W` bleibt ein Legacy-Alias.\n* **Execute**: Einen Shell-Befehl eingeben. Setze `{}` dort ein, wo der gewählte Dateipfad stehen soll, lasse `{}` unzitiert, damit ytnova ihn sicher quoten kann, und benutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n* **Pathcopy**: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n* **Pipe**: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n* **New File**: Eine neue leere Datei anlegen.\n* **Rename**: Die gewählte Datei umbenennen.\n* **Delete**: Die gewählte Datei löschen.\n* **Log**: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne den Split-Dateimodus zu verlassen.\n* **Volume**: Die Volume-Auswahl öffnen.\n* **Quit**: ytnova beenden."},
    {"Split-Datei-Funktionstasten", "* **F1**: Kontextuelle Hilfe für die aktive Split-Dateioberfläche öffnen.\n* **F5**: Das aktive Panel aktualisieren.\n* **F6**: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n* **F7**: Die Vorschau für den aktuellen Dateikontext umschalten.\n* **F8**: Zum Ein-Panel-Modus zurückkehren.\n* **F9**: Das Anwendungsmenü öffnen.\n* **F10**: Die Konfigurationsoberfläche öffnen."},
};

static const GeneratedHelpLink generated_help_links_de_history_dialog[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_history_dialog[] = {
    {"Historienaktionen", "* **Select entry**: `Up` und `Down` bewegen sich durch die aktuelle Historienliste.\n* **Scroll long entry**: `Left` und `Right` verschieben eine lange Historienzeile horizontal.\n* **Pin**: `P` hält einen wichtigen Eintrag oben in der aktuellen Historienliste.\n* **Delete**: `D` entfernt den gewählten Eintrag aus der aktuellen Historienliste.\n* **Accept**: `Enter` übernimmt den gewählten Eintrag erneut.\n* **Cancel**: `Esc` schließt den Dialog ohne Übernahme."},
};

static const GeneratedHelpLink generated_help_links_de_volume_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_volume_menu[] = {
    {"Volume-Aktionen", "* **Select volume**: `Up` und `Down` bewegen sich durch die Liste geladener Volumes.\n* **Switch volume**: `Enter` aktiviert das gewählte Volume.\n* **Keep state**: Wenn du das bereits aktive Volume auswählst, bleibt sein Zustand im Speicher erhalten.\n* **Release volume**: `D` entlädt das gewählte Volume, außer es ist das letzte verbleibende.\n* **Cancel**: `Esc` schließt das Menü."},
};

static const GeneratedHelpLink generated_help_links_de_applications_menu[] = {
    {"Navigation", "navigation"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_applications_menu[] = {
    {"Anwendungsaktionen", "* **Select preset**: `Up` und `Down` bewegen sich durch die Voreinstellungs-Liste.\n* **Launch behavior**: `F9` startet die gewählte Voreinstellung und kehrt direkt zur TUI zurück. Benutze es für wiederkehrende externe Abläufe, nicht für spontane Shell-Befehle.\n* **Use `eXecute` for one-offs**: Der `X`-Prompt bleibt die Ad-hoc-Shell mit Historie und Terminalausgabe. Benutze ihn, wenn du einen einmaligen Befehl brauchst.\n* **Edit presets**: `E` öffnet den Anwendungskatalog, damit Voreinstellungen ohne Verlassen der Auswähler-Familie geändert werden können.\n* **Selection and working directory**: `{}` setzt die aktuelle Datei oder den aktuellen Ordner ein. Voreinstellungen starten außerdem in diesem Verzeichnis, sodass Skripte auch ohne `{}` vom gewählten Ort aus laufen.\n* **Prompt text**: `{input}` setzt den zusätzlichen Text ein, den du im Voreinstellungs-Prompt eingegeben hast.\n* **Starter presets**: Der mitgelieferte Katalog startet mit `xdg-open`-Startern und enthält auskommentierte Beispiele für Werkzeuge wie `mpv` oder lokale Hilfsskripte.\n* **Cancel menu**: `Esc` schließt den Auswähler, ohne eine Voreinstellung zu starten."},
};

static const GeneratedHelpLink generated_help_links_de_f2_picker[] = {
    {"Navigation", "navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLongFormSection generated_help_sections_de_f2_picker[] = {
    {"Auswahlaktionen", "* **Move**: `Up` und `Down` bewegen sich durch die sichtbaren Verzeichniszeilen.\n* **Expand**: `Right` klappt das aktuelle Verzeichnis um eine Ebene auf und geht dann in das erste Kind, wenn diese Ebene schon offen ist.\n* **Collapse**: `Left` klappt das aktuelle Verzeichnis ein oder geht zum Elternknoten, wenn die aktuelle Zeile schon geschlossen ist.\n* **Select**: `Enter` benutzt das markierte Verzeichnis für den aufrufenden Prompt.\n* **Cancel**: `Esc` schließt die Auswahl, ohne den Prompt zu ändern."},
};

static const GeneratedHelpTopic generated_help_topics_de[] = {
    {
        "intro",
        "Inhalt",
        NULL,
        "Benutze diesen Index, wenn du die Frage kennst, aber nicht weißt, welche Seite sie beantwortet.\nDrücke auf einem Thema `Enter` oder `Right`, um es zu öffnen.\nGehe mit `Left` zurück hierher oder mit `Esc` aus der Hilfe.",
        36,
        generated_help_links_de_intro,
        2,
        generated_help_sections_de_intro,
    },
    {
        "navigation",
        "Navigation",
        NULL,
        "Benutze diese Seite, wenn du dich einfach in einer ytnova-Liste bewegen willst.\n`Up/Down` bewegt eine Zeile weiter. `Right` öffnet oder erweitert die gewählte Zeile meistens, und `Left` geht meistens zurück oder klappt sie zu.\n`Enter` öffnet die gewählte Zeile. `PgUp/PgDn` bewegt eine Bildschirmseite weiter. `Home/End` springt ganz nach oben oder unten.\n`/` startet einen Sprung nach Namen: Drücke `/`, tippe Buchstaben und drücke dann `Enter`, um auf dem besten sichtbaren Treffer zu landen.\nWenn eine Oberfläche eine dieser Tasten anders benutzt, erklärt ihre eigene Hilfeseite diese Ausnahme.",
        5,
        generated_help_links_de_navigation,
        4,
        generated_help_sections_de_navigation,
    },
    {
        "list-jump",
        "List Jump",
        NULL,
        "Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer in der aktuellen Liste zu landen.\nIn Baumansichten wiederholst du den Sprung im nächsten Verzeichnis, wenn du tiefer gehen willst.",
        4,
        generated_help_links_de_list_jump,
        2,
        generated_help_sections_de_list_jump,
    },
    {
        "shared-commands",
        "Gemeinsame Befehle",
        NULL,
        "Diese Tasten behalten in mehr als einem Modus dieselbe Grundbedeutung.\nBenutze die Modusseite für lokale Befehle und diese Seite für die gemeinsame Funktionstastenfamilie.",
        4,
        generated_help_links_de_shared_commands,
        1,
        generated_help_sections_de_shared_commands,
    },
    {
        "tagged",
        "Markierungen",
        NULL,
        "Markierungen bauen eine Arbeitsmenge auf, gegen die du dann einen Befehl ausführen kannst.\nWenn Markierungen existieren, arbeiten Kopieren, Verschieben, Anzeigen, Archivieren und Suchen oft auf der markierten Menge statt nur auf der aktuellen Zeile.\n`Filter` besitzt den Umschalter für den Nur-Markierte-Bereich; drücke `F` und dann `Tab`, um aus der aktuellen Menge eine temporäre Liste zu machen, ohne die Markierungen zu ändern.",
        3,
        generated_help_links_de_tagged,
        2,
        generated_help_sections_de_tagged,
    },
    {
        "command-line-editing",
        "Bearbeitung in Eingabezeilen",
        NULL,
        "Die meisten Prompts teilen sich dieselben Bearbeitungstasten.\nLerne sie einmal hier und benutze die Prompt-Seite nur noch für Syntax, Vorgaben und Geltungsbereich.",
        3,
        generated_help_links_de_command_line_editing,
        2,
        generated_help_sections_de_command_line_editing,
    },
    {
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "Kopieren, Verschieben und Pathcopy benutzen absichtlich zwei Prompts.\nZuerst wählst du neuen Namen oder Wildcard-Muster.\nDanach wählst du das Zielverzeichnis.\nDann startet ytnova die Aktion, solange keine echte Sicherheitsabfrage eingreifen muss.\nÜberschreibkonflikte zeigen Größen- und Zeitdaten, damit du den Konflikt beurteilen kannst.",
        4,
        generated_help_links_de_copy_move_targets,
        3,
        generated_help_sections_de_copy_move_targets,
    },
    {
        "vi-keys",
        "VI-Tasten",
        NULL,
        "Mit `VI_KEYS=1` bleiben die vi-Bewegungstasten in Kleinbuchstaben aktiv.\nBefehle, die kollidieren würden, wandern auf Großbuchstaben oder eine andere sichere Taste.",
        2,
        generated_help_links_de_vi_keys,
        2,
        generated_help_sections_de_vi_keys,
    },
    {
        "f10",
        "F10-Konfiguration",
        NULL,
        "Benutze `F10` für Konfigurationsarbeit, nicht für einmalige Dateiaktionen.\nDort gehören Profil, Befehle, Themes, Reload und ähnliche dauerhafte Einstellungen hin.",
        2,
        generated_help_links_de_f10,
        2,
        generated_help_sections_de_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes stylen semantische Rollen, nicht einzelne Bildschirmpositionen.\nDadurch bleiben Hilfe, Auswahlfenster, Auswahlmarkierung, Footer und Warnflächen als ein System lesbar.",
        1,
        generated_help_links_de_theming,
        2,
        generated_help_sections_de_theming,
    },
    {
        "dir",
        "Directory Help",
        "main.dir",
        "Benutze diese Seite für den Baum-Footer und die Regeln des aktuellen Bereichs.\nGemeinsame Zeilen öffnen ihr erklärendes Thema. Einzeilige Zeilen sagen schon genug zum Handeln.",
        5,
        generated_help_links_de_dir,
        4,
        generated_help_sections_de_dir,
    },
    {
        "file",
        "File Help",
        "main.file",
        "Benutze diese Seite für den Datei-Footer und die Regeln des aktuellen Bereichs.\nGemeinsame Zeilen öffnen ihr erklärendes Thema. Einzeilige Zeilen sagen schon genug zum Handeln.",
        6,
        generated_help_links_de_file,
        4,
        generated_help_sections_de_file,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Benutze diese Seite für Befehle im Archivbaum und für Regeln, die nur in Archiven gelten.\nGemeinsame Zeilen öffnen ihr erklärendes Thema. Einzeilige Zeilen sagen schon genug zum Handeln.",
        3,
        generated_help_links_de_archive_dir,
        4,
        generated_help_sections_de_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Benutze diese Seite für Befehle in Archiv-Dateilisten und für dateispezifische Archivregeln.\nGemeinsame Zeilen öffnen ihr erklärendes Thema. Einzeilige Zeilen sagen schon genug zum Handeln.",
        6,
        generated_help_links_de_archive_file,
        4,
        generated_help_sections_de_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Gib einen oder mehrere Filterbegriffe ein und trenne sie mit Kommas.\nDer Prompt startet mit `*`; das bedeutet „alles anzeigen“.\nAlle Begriffe wirken zusammen auf den aktuellen Dateibereich.",
        4,
        generated_help_links_de_filter,
        3,
        generated_help_sections_de_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "Compare startet von der aktuellen Datei, dem aktuellen Verzeichnis oder dem aktuellen geloggten Baum im aktiven Panel.\nDateivergleich prüft eine Datei gegen ein Ziel.\nVerzeichnisvergleich kann das aktuelle Verzeichnis, den geloggten Baum oder ein externes Viewer-Ziel vergleichen.\nInterner Vergleich markiert Ergebnisse nur auf der Quellseite.",
        3,
        generated_help_links_de_compare,
        2,
        generated_help_sections_de_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "Die aktuelle Datei, das aktuelle Verzeichnis oder der aktuelle geloggte Baum ist die Vergleichsquelle.\nGib genau einen Zielpfad direkt ein, benutze `F2` zum Browsen oder `Up` für die Historie.\nDrücke `F3` für den Vergleichsbereich: Datei, Verzeichnis, Baum oder externer Vergleich.\nDrücke `F4` für die Vergleichsbasis: `size`, `date`, `size+date` oder `hash`.\nDrücke `F5`, um das Ergebnis zu wählen, das nach dem Vergleich markiert wird.\nIm Split-Modus liefert das inaktive Panel das Standardziel für den Vergleich.",
        2,
        generated_help_links_de_compare_target,
        1,
        generated_help_sections_de_compare_target,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        NULL,
        "`Directory` vergleicht nur das aktuelle Verzeichnis.\n`Logged tree` vergleicht den aktuellen geloggten Baum und loggt ungeöffnete Äste nicht automatisch nach.\n`External viewer` startet den konfigurierten externen Vergleich statt Laufzeitergebnisse zu markieren.",
        1,
        generated_help_links_de_compare_scope,
        1,
        generated_help_sections_de_compare_scope,
    },
    {
        "change-date",
        "Date Change Help",
        "prompt.change-date",
        "Gib das neue Datum als `YYYY-MM-DD` ein oder ergänze eine Zeit als `YYYY-MM-DD HH:MM[:SS]`.\n`F3` wechselt, ob der eingegebene Wert die Änderungszeit, die Zugriffszeit oder beide aktualisiert.\nMarkierte Datumsänderungen benutzen denselben Prompt und dieselbe Bereichsumschaltung.",
        3,
        generated_help_links_de_change_date,
        2,
        generated_help_sections_de_change_date,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        NULL,
        "`Size` vergleicht die Dateilänge.\n`Date` vergleicht die Änderungszeit.\n`siZe+date` behandelt jede Abweichung als Unterschied.\n`Hash` öffnet beide Dateien und vergleicht den Inhalt exakt, ist also langsamer.",
        1,
        generated_help_links_de_compare_basis,
        1,
        generated_help_sections_de_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        NULL,
        "Wähle die Ergebnisklasse, die auf der Quellseite markiert werden soll.\n`diFferent` markiert Unterschiede, `Unique` markiert nur Quell-eigene Einträge.\n`Match`, `Newer`, `Older`, `Type mismatch` und `Error` markieren jeweils nur diese eine Klasse.",
        1,
        generated_help_links_de_compare_results,
        1,
        generated_help_sections_de_compare_results,
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "Gib den Shell-Befehl ein, den du ausführen willst. Setze `{}` dort ein, wo der Pfad der gewählten Datei stehen soll.\nLasse `{}` unzitiert, damit ytnova den Pfad einsetzen und sicher quoten kann.\nBenutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei zu wiederholen.",
        2,
        generated_help_links_de_execute_file,
        1,
        generated_help_sections_de_execute_file,
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "Gib den Shell-Befehl ein, den du ausführen willst. Setze `{}` dort ein, wo der gewählte Verzeichnispfad stehen soll.\nLasse `{}` unzitiert, damit ytnova den Pfad einsetzen und sicher quoten kann.\nBenutze `Ctrl-X`, um denselben Befehl einmal pro markierter Datei in der aktiven Liste zu wiederholen.",
        2,
        generated_help_links_de_execute_dir,
        1,
        generated_help_sections_de_execute_dir,
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Gib nur den Suchtext ein.\nytnova baut `grep -i -- PATTERN {}` für dich.\nNur markierte Dateien werden durchsucht, und Nicht-Treffer werden entmarkiert.",
        2,
        generated_help_links_de_search_tagged,
        1,
        generated_help_sections_de_search_tagged,
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Benutze `.tar`, `.tar.gz` oder `.tgz`, `.tar.bz2` oder `.tbz2`, `.tar.xz` oder `.txz` oder `.zip`.\nWenn Markierungen existieren, gewinnt die markierte Menge.\nWenn nichts markiert ist, archiviert ytnova die aktuelle Datei- oder Verzeichnisauswahl.",
        2,
        generated_help_links_de_create_archive,
        1,
        generated_help_sections_de_create_archive,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output exportiert Dateiinhalte in einen Pfad oder an einen Druckbefehl.\nWähle zuerst Datei oder Hardcopy und gib dann das endgültige Ziel an.\nBei Dateiausgabe schaltet `F3` zwischen `Raw`, `Framed` und `Page break` um.\n`Framed` und `Page break` fragen den Trenner ab, bevor der endgültige Dateipfad folgt.",
        3,
        generated_help_links_de_output,
        2,
        generated_help_sections_de_output,
    },
    {
        "output-format",
        "Output Format Help",
        NULL,
        "`Raw` schreibt Inhalt ohne zusätzlichen Rahmen.\n`Framed` fügt Kopf- oder Fußzeilen pro Datei hinzu.\n`Page break` fügt zwischen Dateien einen Trenner ein und lässt am Ende keinen zusätzlichen Trenner stehen.",
        1,
        generated_help_links_de_output_format,
        1,
        generated_help_sections_de_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Wähle zuerst Datei oder Hardcopy und gib dieses Ziel dann genau so ein, wie ytnova es benutzen soll.\nDateiausgabe schreibt in einen Pfad, und einfache Dateinamen landen in `CWD`.\nHardcopy schickt rohe Ausgabe an einen Druckbefehl wie `lpr`, `lp` oder `cat > /dev/lp1`.\nDrücke `F3` im Datei-Ziel-Prompt, um zwischen `Raw`, `Framed` und `Page break` umzuschalten.",
        3,
        generated_help_links_de_output_destination,
        2,
        generated_help_sections_de_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "Dieser Prompt erscheint nur, wenn `F3` `Framed` oder `Page break` auswählt.\nLasse ihn leer, um den Standard mit dreifachen Backticks zu nehmen.\nRaw-Ausgabe überspringt diesen Prompt.",
        1,
        generated_help_links_de_output_separator,
        1,
        generated_help_sections_de_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall sammelt alle Dateien des aktuellen geloggten Volumes.\nBenutze diese Seite für die Regeln der Sammelansicht und ihre Footer-Befehle.",
        7,
        generated_help_links_de_showall,
        4,
        generated_help_sections_de_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global sammelt Dateien aus allen geloggten Volumes.\nBenutze diese Seite für die volumenübergreifenden Regeln und ihre Footer-Befehle.",
        7,
        generated_help_links_de_global,
        4,
        generated_help_sections_de_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "Die Vorschau hält die aktuelle Datei offen, während ein kleinerer Befehlssatz aktiv bleibt.\nBenutze diese Seite für die Vorschau-Regeln und die Befehle, die ohne Verlassen der Vorschau weiterlaufen.",
        7,
        generated_help_links_de_f7,
        3,
        generated_help_sections_de_f7,
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "Der Split-Modus hält beide Panels aktiv.\nLaufzeit-`F1` öffnet die Split-Seite des aktiven Panels; diese Seite erklärt die gemeinsamen Split-Regeln.",
        3,
        generated_help_links_de_f8,
        1,
        generated_help_sections_de_f8,
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "Benutze diese Seite für den Footer des aktiven Split-Verzeichnisses.\nSie verbindet die gemeinsamen Split-Regeln mit den Befehlen des aktiven Panels.",
        7,
        generated_help_links_de_f8_dir,
        5,
        generated_help_sections_de_f8_dir,
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "Benutze diese Seite für den Footer der aktiven Split-Dateiliste.\nSie verbindet die gemeinsamen Split-Regeln mit den Befehlen des aktiven Panels.",
        8,
        generated_help_links_de_f8_file,
        5,
        generated_help_sections_de_f8_file,
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Benutze `Up` und `Down`, um einen Eintrag zu wählen.\nBenutze `Left` und `Right`, um einen langen Eintrag horizontal zu verschieben.\nBenutze `P` zum Anheften oder Lösen, `D` zum Löschen, `Enter` zum Übernehmen und `Esc` zum Abbrechen.",
        1,
        generated_help_links_de_history_dialog,
        1,
        generated_help_sections_de_history_dialog,
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Benutze `Up` und `Down`, um ein geladenes Volume zu wählen.\nBenutze `Enter`, um dorthin zu wechseln.\nBenutze `D`, um es freizugeben, außer es ist das letzte.\nBenutze `Esc`, um das Menü zu verlassen.",
        1,
        generated_help_links_de_volume_menu,
        1,
        generated_help_sections_de_volume_menu,
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Benutze `Up` und `Down`, um eine Voreinstellung zu wählen, `Enter`, um sie zu starten, und `Esc`, um abzubrechen.\nBenutze `E`, um den Anwendungskatalog zu bearbeiten.\n`{}` setzt die aktuelle Datei oder den aktuellen Ordner ein. `{input}` setzt den zusätzlichen Prompt-Text der Voreinstellung ein.",
        1,
        generated_help_links_de_applications_menu,
        1,
        generated_help_sections_de_applications_menu,
    },
    {
        "f2-picker",
        "F2-Auswahl",
        "dialog.f2-picker",
        "Benutze `Up` und `Down` zum Bewegen.\nBenutze `Right`, um aufzuklappen oder in das erste Kind zu gehen, und `Left`, um einzuklappen oder zum Elternknoten zu gehen.\nBenutze `<` und `>`, um durch geladene Volumes zu wechseln.\nBenutze `L`, um einen neuen Pfad zu protokollieren.\nBenutze `` ` ``, um Dotfiles umzuschalten.\nBenutze `Enter`, um das markierte Verzeichnis zu wählen, und `Esc` zum Abbrechen.",
        2,
        generated_help_links_de_f2_picker,
        1,
        generated_help_sections_de_f2_picker,
    },
};

static const size_t generated_help_topic_count_de = 39;

static const GeneratedHelpCatalog generated_help_catalogs[] = {
    {"en", 39, generated_help_topics_en},
    {"de", 39, generated_help_topics_de},
};

static const size_t generated_help_catalog_count = 2;
