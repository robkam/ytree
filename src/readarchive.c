/***************************************************************************
 *
 * readarchive.c
 * Functions to read filetree from various archives (TAR, ZIP, ZOO, etc.)
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

    /*
     * Strictly enforce file type bits based on libarchive's explicit filetype.
     * This prevents regular files from being misidentified as directories if
     * the archive header's mode bits are ambiguous or non-standard.
     */
    switch (archive_entry_filetype(entry)) {
        case AE_IFREG:
            dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFREG;
            break;
        case AE_IFDIR:
            dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFDIR;
            break;
        case AE_IFLNK:
            dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFLNK;
            break;
        default:
            /* Keep existing mode for sockets, block devices, etc. */
            break;
    }
}

/*
 * Dispatcher function to read file tree from archive using libarchive.
 * This replaces all old ReadTreeFrom... and GetStatFrom... functions.
 *
 * Now accepts DirEntry **dir_entry_ptr to allow updating the root pointer
 * if MinimizeArchiveTree swaps it.
 */
int ReadTreeFromArchive(DirEntry **dir_entry_ptr, const char *filename, Statistic *s)
{
    struct archive *a;
    struct archive_entry *entry;
    int r;
    char path_buffer[PATH_LENGTH * 2]; /* Buffer for path + symlink target */
    struct stat stat_buf;
    int count = 0;
    const char *clean_path;
    DirEntry *dir_entry = *dir_entry_ptr;

    /* Removed: *dir_entry->name = '\0'; to preserve name set by LoginDisk */

    a = archive_read_new();
    if (a == NULL) {
        ERROR_MSG("archive_read_new() failed");
        return -1;
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, filename, 10240);
    if (r != ARCHIVE_OK) {
        (void)snprintf(message, MESSAGE_LENGTH, "Could not open archive*'%s'*%s", filename, archive_error_string(a));
        ERROR_MSG(message);
        archive_read_free(a);
        return -1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *pathname = archive_entry_pathname(entry);

        if (pathname == NULL) {
            continue;
        }

        /* Normalize path: strip leading "./" if present to avoid confusion in Fnsplit */
        if (pathname[0] == '.' && pathname[1] == FILE_SEPARATOR_CHAR) {
            clean_path = pathname + 2;
        } else {
            clean_path = pathname;
        }

        /* Skip if the path is empty or just "." (root) */
        if (*clean_path == '\0' || (strcmp(clean_path, ".") == 0)) {
            continue;
        }

        copy_stat_from_entry(&stat_buf, entry);

        if (S_ISDIR(stat_buf.st_mode)) {
            /* Safer string copy */
            snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);
            /* Ensure directory paths end with a separator for TryInsertArchiveDirEntry */
            size_t len = strlen(path_buffer);
            if (len > 0 && path_buffer[len - 1] != FILE_SEPARATOR_CHAR) {
                if (len < sizeof(path_buffer) - 2) {
                    strcat(path_buffer, FILE_SEPARATOR_STRING);
                }
            }
            (void)TryInsertArchiveDirEntry(dir_entry, path_buffer, &stat_buf, s);

        } else if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode)) {
            /* Safer string copy */
            snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);

            if (S_ISLNK(stat_buf.st_mode)) {
                const char *link_target = archive_entry_symlink(entry);
                if (link_target) {
                    /* Append symlink target after the null terminator of the path */
                    size_t len = strlen(path_buffer);
                    if (len + 1 + strlen(link_target) < sizeof(path_buffer)) {
                        strcpy(&path_buffer[len + 1], link_target);
                    }
                }
            }
            (void)InsertArchiveFileEntry(dir_entry, path_buffer, &stat_buf, s);
        }

        if (KeyPressed()) {
            Quit();
        }

        /* Update statistics / animation every 20 files */
        if( ( count++ % 20 ) == 0 ) {
            if (animation_method == 1) {
                DrawAnimationStep(dir_window); /* Changed from file_window to dir_window */
            } else {
                if ((count % 100) == 0) {
                    DisplayDiskStatistic(s);
                }
            }
            /* These UI updates should always happen, regardless of animation method */
            DrawSpinner(); /* Activity spinner */
            ClockHandler(0); /* Clock update */
            doupdate(); /* Ensure screen is refreshed after updates */
        }
    }

    /* Pass the double pointer so it can update the root if needed */
    MinimizeArchiveTree(dir_entry_ptr, s);

    archive_read_free(a);

    /* Important: Recalculate visibility based on current settings immediately after load */
    RecalculateSysStats(s);

    return 0;
}

#else
/* Stub function if ytree is compiled without libarchive support. */
int ReadTreeFromArchive(DirEntry **dir_entry_ptr, const char *filename, Statistic *s)
{
    ERROR_MSG("Archive support not compiled.*Please install libarchive-dev*and recompile ytree.");
    return -1;
}

#endif /* HAVE_LIBARCHIVE */