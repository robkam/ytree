/***************************************************************************
 *
 * usermode.c
 * Functions for executing User Defined Commands.
 *
 ***************************************************************************/


#include "ytree.h"


#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

int DirUserMode(DirEntry *dir_entry, int ch)
{
    int chremap;
    char *command_str;
    int command_was_run = 0;
    int current_key = ch;

    while (1) {
        command_str = GetUserDirAction(current_key, &chremap);

        if (command_str != NULL) {
            command_was_run = 1;

            char command_line[COMMAND_LINE_LENGTH + 1];
            char path[PATH_LENGTH + 1];
            char filepath[PATH_LENGTH + 1];
            char escaped_filepath[PATH_LENGTH * 2 + 1];
            int start_dir_fd;

            GetPath(dir_entry, filepath);
            StrCp(escaped_filepath, filepath);

            /* Safe string formatting with escaped path */
            if (strstr(command_str, "%s") != NULL) {
                /* If user provided %s placeholder, use it */
                snprintf(command_line, sizeof(command_line), command_str, escaped_filepath);
            } else {
                /* Otherwise append filepath to command */
                /* Use snprintf directly to ensure safety and silence warnings */
                int written = snprintf(command_line, sizeof(command_line), "%s %s", command_str, escaped_filepath);
                if (written >= (int)sizeof(command_line)) {
                    WARNING("Command line truncated!*Path too long.");
                    return -1;
                }
            }

            /* Robustly save current working directory using a file descriptor */
            start_dir_fd = open(".", O_RDONLY);
            if (start_dir_fd == -1) {
                snprintf(message, MESSAGE_LENGTH, "Error saving current directory context*%s", strerror(errno));
                MESSAGE(message);
                /* Continue loop processing? Usually we consume key here. */
                return -1;
            }

            if (chdir(GetPath(dir_entry, path))) {
                snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
                MESSAGE(message);
            } else {
                QuerySystemCall(command_line, &CurrentVolume->vol_stats);

                /* Restore original directory */
                if (fchdir(start_dir_fd) == -1) {
                    snprintf(message, MESSAGE_LENGTH, "Error restoring directory*%s", strerror(errno));
                    MESSAGE(message);
                }
            }
            close(start_dir_fd);
        }

        /* Check if the chain ends */
        if (chremap == current_key || chremap <= 0) {
            break;
        } else {
            /* Follow the remap chain */
            current_key = chremap;
        }
    }

    if (command_was_run) {
        return -1; /* Command was executed, consume the keypress. */
    }

    /* No command was run, return the final result of remapping. */
    return chremap;
}

int FileUserMode(FileEntryList *file_entry_list, int ch)
{
    int chremap;
    char *command_str;
    int command_was_run = 0;
    int current_key = ch;

    while (1) {
        command_str = GetUserFileAction(current_key, &chremap);

        if (command_str != NULL) {
            command_was_run = 1;

            char command_line[COMMAND_LINE_LENGTH + 1];
            char path[PATH_LENGTH + 1];
            char filepath[PATH_LENGTH + 1];
            char escaped_filepath[PATH_LENGTH * 2 + 1];
            int start_dir_fd;
            DirEntry *dir_entry = file_entry_list->file->dir_entry;

            GetRealFileNamePath(file_entry_list->file, filepath);
            StrCp(escaped_filepath, filepath);

            /* Safe string formatting with escaped path */
            if (strstr(command_str, "%s") != NULL) {
                snprintf(command_line, sizeof(command_line), command_str, escaped_filepath);
            } else {
                /* Use snprintf directly to ensure safety and silence warnings */
                int written = snprintf(command_line, sizeof(command_line), "%s %s", command_str, escaped_filepath);
                if (written >= (int)sizeof(command_line)) {
                    WARNING("Command line truncated!*Path too long.");
                    return -1;
                }
            }

            /* Robustly save current working directory using a file descriptor */
            start_dir_fd = open(".", O_RDONLY);
            if (start_dir_fd == -1) {
                snprintf(message, MESSAGE_LENGTH, "Error saving current directory context*%s", strerror(errno));
                MESSAGE(message);
                return -1;
            }

            if (chdir(GetPath(dir_entry, path))) {
                snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
                MESSAGE(message);
            } else {
                QuerySystemCall(command_line, &CurrentVolume->vol_stats);

                /* Restore original directory */
                if (fchdir(start_dir_fd) == -1) {
                    snprintf(message, MESSAGE_LENGTH, "Error restoring directory*%s", strerror(errno));
                    MESSAGE(message);
                }
            }
            close(start_dir_fd);
        }

        /* Check if the chain ends */
        if (chremap == current_key || chremap <= 0) {
            break;
        } else {
            /* Follow the remap chain */
            current_key = chremap;
        }
    }

    if (command_was_run) {
        return -1; /* Command was executed, consume the keypress. */
    }

    /* No command was run, return the final result of remapping. */
    return chremap;
}