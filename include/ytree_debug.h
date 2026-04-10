/***************************************************************************
 *
 * ytree_debug.h
 * Debug logging helpers
 *
 ***************************************************************************/

#ifndef YTREE_DEBUG_H
#define YTREE_DEBUG_H

#include <stdio.h>
#include <stdlib.h>

#define YTREE_DEBUG_LOG_PATH_ENV "YTREE_DEBUG_LOG_PATH"
#define YTREE_DEBUG_LOG_PATH_LIMIT 4096U

#define DEBUG_LOG(fmt, ...)                                                     \
  do {                                                                           \
    const char *ytree_debug_log_path = getenv(YTREE_DEBUG_LOG_PATH_ENV);         \
    size_t ytree_debug_log_len = 0U;                                             \
    int ytree_debug_log_path_valid = 0;                                          \
    FILE *fp = NULL;                                                             \
    if (ytree_debug_log_path != NULL && ytree_debug_log_path[0] != '\0') {      \
      ytree_debug_log_path_valid = 1;                                            \
      while (ytree_debug_log_len < YTREE_DEBUG_LOG_PATH_LIMIT &&                 \
             ytree_debug_log_path[ytree_debug_log_len] != '\0') {                \
        unsigned char ytree_debug_ch =                                           \
            (unsigned char)ytree_debug_log_path[ytree_debug_log_len];            \
        if (ytree_debug_ch < 32U || ytree_debug_ch == 127U) {                    \
          ytree_debug_log_path_valid = 0;                                        \
          break;                                                                 \
        }                                                                        \
        ++ytree_debug_log_len;                                                   \
      }                                                                          \
      if (ytree_debug_log_len == YTREE_DEBUG_LOG_PATH_LIMIT) {                   \
        ytree_debug_log_path_valid = 0;                                          \
      }                                                                          \
    }                                                                            \
    if (ytree_debug_log_path_valid) {                                            \
      fp = fopen(ytree_debug_log_path, "a");                                     \
    }                                                                            \
    if (fp != NULL) {                                                            \
      fprintf(fp, fmt "\n", ##__VA_ARGS__);                                      \
      fflush(fp);                                                                \
      fclose(fp);                                                                \
    }                                                                            \
  } while (0)

#endif /* YTREE_DEBUG_H */
