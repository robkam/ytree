#ifndef TESTS_FUZZ_FUZZ_COMMON_H
#define TESTS_FUZZ_FUZZ_COMMON_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *data;
  size_t size;
  size_t offset;
} FuzzCursor;

void FuzzCursor_Init(FuzzCursor *cursor, const uint8_t *data, size_t size);
size_t FuzzCursor_Remaining(const FuzzCursor *cursor);
uint8_t FuzzCursor_NextByte(FuzzCursor *cursor);
int FuzzCursor_NextBool(FuzzCursor *cursor);
size_t FuzzCursor_ReadAsciiString(FuzzCursor *cursor, char *dst, size_t dst_size,
                                  size_t max_len);
long long FuzzCursor_ReadSigned64(FuzzCursor *cursor);

#endif /* TESTS_FUZZ_FUZZ_COMMON_H */
