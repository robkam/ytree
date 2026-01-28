/***************************************************************************
 *
 * src/core/volume.c
 * Volume Management Functions
 *
 ***************************************************************************/


#include "ytree.h"
#include <stdio.h>  /* For fprintf, stderr */
#include <stdlib.h> /* For calloc, exit, free */

/* Static counter to generate unique volume IDs */
static int VolumeSerial = 0;

/*
 * Volume_Create
 *
 * Allocates and initializes a new Volume structure, assigns a unique ID,
 * and adds it to the global VolumeList hash table.
 *
 * Returns:
 *   A pointer to the newly created Volume on success.
 *   Exits with an error message on memory allocation failure.
 */
struct Volume *Volume_Create(void) {
    struct Volume *new_vol = NULL;

    /* Allocate memory for a new Volume and initialize it to zero */
    new_vol = (struct Volume *)xcalloc(1, sizeof(struct Volume));

    /* Assign a unique ID and increment the serial counter */
    new_vol->id = VolumeSerial++;

    /* Add the new volume to the global hash table (VolumeList)
     * The 'id' field of the Volume struct is used as the key. */
    HASH_ADD_INT(VolumeList, id, new_vol);

    return new_vol;
}

/*
 * Volume_Delete
 *
 * Safely destroys a Volume: removes it from the global hash table,
 * frees its associated directory tree, and then frees the Volume structure itself.
 *
 * Parameters:
 *   vol - A pointer to the Volume structure to be deleted.
 */
void Volume_Delete(struct Volume *vol) {
    if (vol == NULL) {
        return;
    }

    if (vol->dir_entry_list) {
        free(vol->dir_entry_list);
        vol->dir_entry_list = NULL;
    }
    if (vol->file_entry_list) {
        free(vol->file_entry_list);
        vol->file_entry_list = NULL;
    }

    /* Remove the volume from the global hash table */
    HASH_DEL(VolumeList, vol);

    /* Free the associated directory tree if it exists */
    if (vol->vol_stats.tree != NULL) {
        DeleteTree(vol->vol_stats.tree);
        vol->vol_stats.tree = NULL; /* Good practice to nullify after freeing */
    }

    /* Free the Volume structure itself */
    free(vol);
}

/*
 * Volume_FreeAll
 *
 * Iterates through all loaded volumes in VolumeList, deletes each one,
 * and then clears the global VolumeList and CurrentVolume pointers.
 * This is intended for a safe shutdown to prevent memory leaks.
 */
void Volume_FreeAll(void) {
    struct Volume *s, *tmp;

    /* Safely iterate and delete everything */
    HASH_ITER(hh, VolumeList, s, tmp) {
        Volume_Delete(s);
    }
    CurrentVolume = NULL;
    VolumeList = NULL;
}

/*
 * Volume_GetByPath
 *
 * Finds the volume that contains the given path.
 * Returns the volume with the longest matching login_path prefix.
 * This handles cases where volumes are nested or distinct.
 */
struct Volume *Volume_GetByPath(const char *path) {
    struct Volume *s, *tmp;
    struct Volume *best_match = NULL;
    size_t best_len = 0;
    size_t len;

    if (!path) return NULL;

    HASH_ITER(hh, VolumeList, s, tmp) {
        /* Check if this volume is valid (has a path) */
        if (s->vol_stats.login_path[0] == '\0') continue;

        /* Check if 'path' starts with this volume's login path */
        if (strncmp(path, s->vol_stats.login_path, strlen(s->vol_stats.login_path)) == 0) {
            len = strlen(s->vol_stats.login_path);

            /* Ensure it's a true path prefix match (e.g. "/usr" matches "/usr/bin" but not "/usrlocal") */
            /* Logic: prefix must be full path (equal) OR followed by separator in 'path' */
            /* Exception: Root "/" matches everything */

            if (path[len] == '\0' || path[len] == FILE_SEPARATOR_CHAR || len == 1) { /* len=1 handles root "/" case */
                if (len > best_len) {
                    best_len = len;
                    best_match = s;
                }
            }
        }
    }
    return best_match;
}

/*
 * Volume_Load
 *
 * Core logic to load a volume from a path.
 * Separates I/O and validation from UI interaction.
 *
 * Parameters:
 *   path      - The file system path to load.
 *   reuse_vol - Pointer to an existing volume to reuse (e.g., the empty virgin volume).
 *               If NULL, a new volume is created.
 *   cb        - Callback function for scanning progress.
 *
 * Returns:
 *   Pointer to the successfully loaded Volume, or NULL on failure.
 *   In case of failure, an error message is written to the global 'message' buffer.
 */
struct Volume *Volume_Load(const char *path, struct Volume *reuse_vol, ScanProgressCallback cb) {
    char resolved_path[PATH_LENGTH + 1];
    struct stat stat_struct;
    struct Volume *vol = NULL;
    Statistic *s;
    int depth;
    int res;

    /* 1. Path Resolution */
    if (realpath(path, resolved_path) == NULL) {
        strncpy(resolved_path, path, PATH_LENGTH);
        resolved_path[PATH_LENGTH] = '\0';
    }

    /* 2. Validation */
    if (STAT_(resolved_path, &stat_struct)) {
        MESSAGE("Can't access*\"%s\"*%s", resolved_path, strerror(errno));
        return NULL;
    }

    /* 3. Archive Check */
    if (!S_ISDIR(stat_struct.st_mode)) {
#ifdef HAVE_LIBARCHIVE
        struct archive *a_test;
        int r_test;

        a_test = archive_read_new();
        if (a_test == NULL) {
             MESSAGE("archive_read_new() failed");
             return NULL;
        }
        archive_read_support_filter_all(a_test);
        archive_read_support_format_all(a_test);
        r_test = archive_read_open_filename(a_test, resolved_path, 10240);
        archive_read_free(a_test);

        if (r_test != ARCHIVE_OK) {
             MESSAGE("Not a recognized archive file*or format not supported*\"%s\"", resolved_path);
             return NULL;
        }
#else
        MESSAGE("Cannot open file as archive*ytree not compiled with*libarchive support");
        return NULL;
#endif
    }

    /* 4. Allocation */
    if (reuse_vol) {
        vol = reuse_vol;
        if (vol->vol_stats.tree) {
            DeleteTree(vol->vol_stats.tree);
            vol->vol_stats.tree = NULL;
        }
        memset(&vol->vol_stats, 0, sizeof(Statistic));
        FreeVolumeCache(vol);
    } else {
        vol = Volume_Create();
        if (!vol) return NULL; /* Volume_Create exits on failure, but safety check */
    }

    s = &vol->vol_stats;

    /* 5. Setup */
    s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
    strcpy(s->file_spec, DEFAULT_FILE_SPEC);

    if (!s->tree) {
        s->tree = (DirEntry *)xcalloc(1, sizeof(DirEntry) + PATH_LENGTH);
    }

    strncpy(s->login_path, resolved_path, PATH_LENGTH);
    s->login_path[PATH_LENGTH] = '\0';
    strncpy(s->path, resolved_path, PATH_LENGTH);
    s->path[PATH_LENGTH] = '\0';

    memcpy(&s->tree->stat_struct, &stat_struct, sizeof(stat_struct));

    /* 6. Mode Detection */
    if (!S_ISDIR(stat_struct.st_mode)) {
        /* "root" node is always a directory structure for Ytree, even for archives */
        memset(&s->tree->stat_struct, 0, sizeof(struct stat));
        s->tree->stat_struct.st_mode = S_IFDIR;
        s->disk_total_directories = 1;
        s->login_mode = ARCHIVE_MODE;
    } else if (IsUserActionDefined()) {
        s->login_mode = USER_MODE;
    } else {
        s->login_mode = DISK_MODE;
    }

    GetDiskParameter(resolved_path, s->disk_name, &s->disk_space, &s->disk_capacity, s);
    strcpy(s->tree->name, resolved_path);

    /* 7. Scanning */
    if (s->login_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
         if (ReadTreeFromArchive(&s->tree, s->login_path, s, cb, s)) {
             /* Error message is set by ReadTreeFromArchive or we can set a generic one */
             /* Note: ReadTreeFromArchive typically prints to 'message' on error */
             if (!reuse_vol) Volume_Delete(vol);
             else {
                 DeleteTree(s->tree);
                 s->tree = NULL;
                 memset(s, 0, sizeof(Statistic));
                 s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
                 strcpy(s->file_spec, DEFAULT_FILE_SPEC);
             }
             return NULL;
         }
#endif
    } else {
        s->tree->next = s->tree->prev = NULL;
        depth = strtol(TREEDEPTH, NULL, 0);
        res = ReadTree(s->tree, resolved_path, depth, s, cb, s);
        if (res != 0) {
             if (res != -1) MESSAGE("ReadTree Failed");
             if (!reuse_vol) Volume_Delete(vol);
             else {
                 DeleteTree(s->tree);
                 s->tree = NULL;
                 memset(s, 0, sizeof(Statistic));
                 s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
                 strcpy(s->file_spec, DEFAULT_FILE_SPEC);
             }
             return NULL;
        }
        /* Copy stats to disk_stats for later reference */
        memcpy(&vol->vol_disk_stats, s, sizeof(Statistic));
    }

    return vol;
}