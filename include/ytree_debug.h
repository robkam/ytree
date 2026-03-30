/***************************************************************************
 *
 * ytree_debug.h
 * Debug logging helpers
 *
 ***************************************************************************/

#ifndef YTREE_DEBUG_H
#define YTREE_DEBUG_H

#include <stdio.h>

#define DEBUG_LOG(fmt, ...)                                                    \
  {                                                                            \
    FILE *fp = fopen("/tmp/ytree_debug.log", "a");                             \
    if (fp) {                                                                  \
      fprintf(fp, fmt "\n", ##__VA_ARGS__);                                    \
      fflush(fp);                                                              \
      fclose(fp);                                                              \
    }                                                                          \
  }

#endif /* YTREE_DEBUG_H */
