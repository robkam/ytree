/***************************************************************************
 *
 * readarchive.c
 * functions to read filetree from various archives (TAR, ZIP, ZOO, etc.)
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef HAVE_LIBARCHIVE

/*
 * Copy stat data from libarchive's const struct to our mutable one.
 */
static void copy_stat_from_entry(struct stat *dest, struct archive_entry *entry)
{
    const struct stat *st;

    memset(dest, 0, sizeof(struct stat));
    st = archive_entry_stat(entry);

    if (st) {
        dest->st_mode = st->st_mode;
        dest->st_uid = st->st_uid;
        dest->st_gid = st->st_gid;
        dest->st_size = st->st_size;
        dest->st_mtime = st->st_mtime;
        dest->st_nlink = st->st_nlink;
    } else {
        /* Fallback if stat is not available */
        dest->st_mode = archive_entry_mode(entry);
        dest->st_uid = archive_entry_uid(entry);
        dest->st_gid = archive_entry_gid(entry);
        dest->st_size = archive_entry_size(entry);
        dest->st_mtime = archive_entry_mtime(entry);
        dest->st_nlink = archive_entry_nlink(entry);
    }
}

/*
 * Dispatcher function to read file tree from archive using libarchive.
 * This replaces all old ReadTreeFrom... and GetStatFrom... functions.
 */
int ReadTreeFromArchive(DirEntry *dir_entry, const char *filename)
{
    struct archive *a;
    struct archive_entry *entry;
    int r;
    char path_buffer[PATH_LENGTH * 2]; /* Buffer for path + symlink target */
    struct stat stat_buf;

    *dir_entry->name = '\0';

    a = archive_read_new();
    if (a == NULL) {
        ERROR_MSG("archive_read_new() failed");
        return -1;
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, filename, 10240);
    if (r != ARCHIVE_OK) {
        (void)sprintf(message, "Could not open archive*'%s'*%s", filename, archive_error_string(a));
        ERROR_MSG(message);
        archive_read_free(a);
        return -1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);

        if (pathname == NULL) {
            continue;
        }

        copy_stat_from_entry(&stat_buf, entry);

        if (S_ISDIR(stat_buf.st_mode)) {
            (void)strcpy(path_buffer, pathname);
            /* Ensure directory paths end with a separator for TryInsertArchiveDirEntry */
            if (path_buffer[strlen(path_buffer) - 1] != FILE_SEPARATOR_CHAR) {
                strcat(path_buffer, FILE_SEPARATOR_STRING);
            }
            if (strcmp(path_buffer, "./") != 0) {
                 (void)TryInsertArchiveDirEntry(dir_entry, path_buffer, &stat_buf);
            }
        } else if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode)) {
            (void)strcpy(path_buffer, pathname);

            if (S_ISLNK(stat_buf.st_mode)) {
                const char *link_target = archive_entry_symlink(entry);
                if (link_target) {
                    /* Append symlink target after the null terminator of the path */
                    strcpy(&path_buffer[strlen(path_buffer) + 1], link_target);
                }
            }
            (void)InsertArchiveFileEntry(dir_entry, path_buffer, &stat_buf);
        }

        if (KeyPressed()) {
            Quit();
        }
        DisplayDiskStatistic();
        doupdate();
    }

    MinimizeArchiveTree(dir_entry);
    archive_read_free(a);

    return 0;
}

#else
/* Stub function if ytree is compiled without libarchive support. */
int ReadTreeFromArchive(DirEntry *dir_entry, const char *filename)
{
    ERROR_MSG("Archive support not compiled.*Please install libarchive-dev*and recompile ytree.");
    return -1;
}

#endif /* HAVE_LIBARCHIVE */