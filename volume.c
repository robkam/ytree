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
    new_vol = (struct Volume *)calloc(1, sizeof(struct Volume));
    if (new_vol == NULL) {
        /* Critical error: memory allocation failed */
        fprintf(stderr, "Error: Failed to allocate memory for new Volume.\n");
        exit(1);
    }

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