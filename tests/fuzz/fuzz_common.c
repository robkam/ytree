#include "fuzz_common.h"

#include <limits.h>

void FuzzCursor_Init(FuzzCursor *cursor, const uint8_t *data, size_t size) {
  if (!cursor)
    return;
  cursor->data = data;
  cursor->size = size;
  cursor->offset = 0;
}

size_t FuzzCursor_Remaining(const FuzzCursor *cursor) {
  if (!cursor || cursor->offset >= cursor->size)
    return 0;
  return cursor->size - cursor->offset;
}

uint8_t FuzzCursor_NextByte(FuzzCursor *cursor) {
  if (!cursor || cursor->offset >= cursor->size)
    return 0;
  return cursor->data[cursor->offset++];
}

int FuzzCursor_NextBool(FuzzCursor *cursor) {
  return (FuzzCursor_NextByte(cursor) & 1U) ? 1 : 0;
}

size_t FuzzCursor_ReadAsciiString(FuzzCursor *cursor, char *dst, size_t dst_size,
                                  size_t max_len) {
  size_t remaining;
  size_t max_by_buffer;
  size_t target_len;
  size_t i;
  static const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
      "._-/%*? ,'\"$@![]{}():+=\\";
  const size_t charset_len = sizeof(charset) - 1;

  if (!dst || dst_size == 0)
    return 0;

  remaining = FuzzCursor_Remaining(cursor);
  max_by_buffer = dst_size - 1;
  if (max_len < max_by_buffer)
    max_by_buffer = max_len;
  if (remaining < max_by_buffer)
    max_by_buffer = remaining;

  if (max_by_buffer == 0) {
    dst[0] = '\0';
    return 0;
  }

  target_len = (size_t)(FuzzCursor_NextByte(cursor) % (max_by_buffer + 1));
  for (i = 0; i < target_len; ++i) {
    uint8_t b = FuzzCursor_NextByte(cursor);
    dst[i] = charset[(size_t)(b % charset_len)];
  }
  dst[target_len] = '\0';
  return target_len;
}

long long FuzzCursor_ReadSigned64(FuzzCursor *cursor) {
  unsigned long long value = 0;
  int i;
  for (i = 0; i < 8; ++i) {
    value <<= 8;
    value |= (unsigned long long)FuzzCursor_NextByte(cursor);
  }
  if (value <= (unsigned long long)LLONG_MAX)
    return (long long)value;
  return -1LL - (long long)(~value);
}
