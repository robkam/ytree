#include "ytree.h"
#include <stdio.h>  /* For fprintf, stderr */
#include <stdlib.h> /* For calloc, exit */

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