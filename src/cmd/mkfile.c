/***************************************************************************
 *
 * src/cmd/mkfile.c
 * Creating empty files (Touch command)
 *
 ***************************************************************************/

#include "ytree.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int MakeFile(DirEntry *dir_entry)
{
    char file_name[PATH_LENGTH * 2 + 1];
    char buffer[PATH_LENGTH + 1];
    int result = -1;
    int fd;
    struct stat stat_struct;

    if (mode != DISK_MODE)
    {
        /* Currently only supporting Disk Mode for simplicity.
           Archive mode would require the Rewrite Engine hooks.
           User Mode relies on external commands so it's different.
        */
        MESSAGE("Touch command only available in Disk Mode");
        return -1;
    }

    ClearHelp();

    *file_name = '\0';

    if (UI_ReadString("MAKE FILE:", file_name, PATH_LENGTH, HST_FILE) == CR)
    {
        /* Check if file already exists in the current view to warn user? 
           open with O_EXCL will handle the actual FS existence.
        */

        (void) GetPath(dir_entry, buffer);
        (void) strcat(buffer, FILE_SEPARATOR_STRING);
        (void) strcat(buffer, file_name);

        /* Check existence via stat first to give a nicer error or prompt? 
           For now, let's just try to create. 
        */

        if (stat(buffer, &stat_struct) == 0) {
            MESSAGE("File already exists!");
        } else {
            fd = open(buffer, O_CREAT | O_EXCL | O_WRONLY, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) & ~user_umask);
            
            if (fd == -1) {
                 MESSAGE("Can't create File*\"%s\"*%s", buffer, strerror(errno));
            } else {
                close(fd);
                result = 0;

                /* Refresh logic to show the new file */
                /* For now, trigger a safe refresh of the current view */
                if (ActivePanel) {
                    RefreshTreeSafe(ActivePanel, dir_entry);
                } else {
                    /* Fallback if no active panel context (shouldn't happen) */
                     /* This might be too heavy? RescanDir is what RefreshTreeSafe does. */
                }
            }
        }
    }

    move(LINES - 2, 1); clrtoeol();

    return result;
}
