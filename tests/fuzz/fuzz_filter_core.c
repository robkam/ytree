#include "fuzz_common.h"

#include "ytree_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FileEntry *AllocFileEntry(const char *name) {
  size_t name_len = strlen(name);
  FileEntry *file_entry =
      (FileEntry *)calloc(1, sizeof(FileEntry) + name_len + 1);
  if (!file_entry)
    return NULL;
  memcpy(file_entry->name, name, name_len + 1);
  return file_entry;
}

static DirEntry *AllocDirEntry(void) {
  DirEntry *dir_entry = (DirEntry *)calloc(1, sizeof(DirEntry) + 2);
  if (!dir_entry)
    return NULL;
  dir_entry->name[0] = '.';
  dir_entry->name[1] = '\0';
  return dir_entry;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzCursor cursor;
  char file_name[PATH_LENGTH + 1];
  char file_spec[FILE_SPEC_LENGTH + 1];
  long long signed_size;
  unsigned long long size_magnitude;
  FileEntry *file_entry;
  DirEntry *dir_entry;
  Statistic statistic;

  FuzzCursor_Init(&cursor, data, size);
  FuzzCursor_ReadAsciiString(&cursor, file_name, sizeof(file_name), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, file_spec, sizeof(file_spec), FILE_SPEC_LENGTH);
  if (file_name[0] == '\0')
    (void)snprintf(file_name, sizeof(file_name), "fuzz.tmp");

  file_entry = AllocFileEntry(file_name);
  if (!file_entry)
    return 0;

  memset(&statistic, 0, sizeof(statistic));
  memcpy(statistic.file_spec, file_spec, sizeof(statistic.file_spec) - 1);
  statistic.file_spec[sizeof(statistic.file_spec) - 1] = '\0';

  signed_size = FuzzCursor_ReadSigned64(&cursor);
  if (signed_size < 0)
    size_magnitude = (unsigned long long)(-(signed_size + 1LL)) + 1ULL;
  else
    size_magnitude = (unsigned long long)signed_size;
  file_entry->stat_struct.st_size = (off_t)(size_magnitude % 65536ULL);

  (void)FsMatchFilter(file_entry, &statistic);

  dir_entry = AllocDirEntry();
  if (dir_entry) {
    dir_entry->file = file_entry;
    FsApplyFilter(dir_entry, &statistic);
    free(dir_entry);
  }

  free(file_entry);
  return 0;
}
