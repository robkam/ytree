/***************************************************************************
 *
 * ytnova_debug.h
 * Debug logging helpers
 *
 ***************************************************************************/

#ifndef YTNOVA_DEBUG_H
#define YTNOVA_DEBUG_H

#include <stdio.h>
#include <stdlib.h>

#define YTNOVA_DEBUG_LOG_PATH_ENV "YTNOVA_DEBUG_LOG_PATH"
#define YTNOVA_DEBUG_LOG_PATH_LIMIT 4096U

#ifndef YTNOVA_ENABLE_KEYSTROKE_DEBUG_LOG
#define YTNOVA_ENABLE_KEYSTROKE_DEBUG_LOG 0
#endif

#define DEBUG_LOG(fmt, ...)                                                     \
  do {                                                                           \
    const char *ytnova_debug_log_path = getenv(YTNOVA_DEBUG_LOG_PATH_ENV);         \
    size_t ytnova_debug_log_len = 0U;                                             \
    int ytnova_debug_log_path_valid = 0;                                          \
    FILE *fp = NULL;                                                             \
    if (ytnova_debug_log_path != NULL && ytnova_debug_log_path[0] != '\0') {      \
      ytnova_debug_log_path_valid = 1;                                            \
      while (ytnova_debug_log_len < YTNOVA_DEBUG_LOG_PATH_LIMIT &&                 \
             ytnova_debug_log_path[ytnova_debug_log_len] != '\0') {                \
        unsigned char ytnova_debug_ch =                                           \
            (unsigned char)ytnova_debug_log_path[ytnova_debug_log_len];            \
        if (ytnova_debug_ch < 32U || ytnova_debug_ch == 127U) {                    \
          ytnova_debug_log_path_valid = 0;                                        \
          break;                                                                 \
        }                                                                        \
        ++ytnova_debug_log_len;                                                   \
      }                                                                          \
      if (ytnova_debug_log_len == YTNOVA_DEBUG_LOG_PATH_LIMIT) {                   \
        ytnova_debug_log_path_valid = 0;                                          \
      }                                                                          \
    }                                                                            \
    if (ytnova_debug_log_path_valid) {                                            \
      fp = fopen(ytnova_debug_log_path, "a");                                     \
    }                                                                            \
    if (fp != NULL) {                                                            \
      fprintf(fp, fmt "\n", ##__VA_ARGS__);                                      \
      fflush(fp);                                                                \
      fclose(fp);                                                                \
    }                                                                            \
  } while (0)

#if YTNOVA_ENABLE_KEYSTROKE_DEBUG_LOG
#define DEBUG_KEYSTROKE_LOG(fmt, ...) DEBUG_LOG(fmt, ##__VA_ARGS__)
#else
#define DEBUG_KEYSTROKE_LOG(fmt, ...)                                            \
  do {                                                                           \
  } while (0)
#endif

#endif /* YTNOVA_DEBUG_H */
