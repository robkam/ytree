/***************************************************************************
 *
 * src/fs/storage_ports.c
 * Core storage callback provider (fs-side)
 *
 ***************************************************************************/

#include "ytree_fs.h"

static int CoreStorage_GetDiskParameter(char *path, char *volume_name,
                                        long long *avail_bytes,
                                        long long *capacity, Statistic *s) {
  return GetDiskParameter(path, volume_name, avail_bytes, capacity, s);
}

static int CoreStorage_ReadTree(ViewContext *ctx, DirEntry *dir_entry, char *path,
                                int depth, Statistic *s,
                                CoreScanProgressCallback cb, void *cb_data) {
  return ReadTree(ctx, dir_entry, path, depth, s, cb, cb_data);
}

static int CoreStorage_ReadTreeFromArchive(ViewContext *ctx,
                                           DirEntry **dir_entry_ptr,
                                           const char *filename, Statistic *s,
                                           CoreScanProgressCallback cb,
                                           void *cb_data) {
  return ReadTreeFromArchive(ctx, dir_entry_ptr, filename, s, cb, cb_data);
}

static void CoreStorage_DeleteTree(DirEntry *tree) { DeleteTree(tree); }

void CoreStorageOps_Register(ViewContext *ctx) {
  if (ctx == NULL)
    return;

  ctx->core_storage_ops.get_disk_parameter = CoreStorage_GetDiskParameter;
  ctx->core_storage_ops.read_tree = CoreStorage_ReadTree;
  ctx->core_storage_ops.read_tree_from_archive = CoreStorage_ReadTreeFromArchive;
  ctx->core_storage_ops.delete_tree = CoreStorage_DeleteTree;
}
