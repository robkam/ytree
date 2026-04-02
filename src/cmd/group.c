/***************************************************************************
 *
 * src/cmd/group.c
 * Handling of group numbers / names
 * Modernized: Direct POSIX calls, no internal caching.
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <grp.h>
#include <sys/types.h>

/*
 * ReadGroupEntries
 * Legacy compatibility: Formerly read /etc/group into cache.
 * Now a no-op as we query on demand.
 */
int ReadGroupEntries(void)
{
  return( 0 );
}

/*
 * GetGroupName
 * Returns the group name for a given GID.
 * Returns NULL if not found.
 */
char *GetGroupName(unsigned int gid)
{
  struct group *grp_ptr;

  grp_ptr = getgrgid( (gid_t)gid );

  if( grp_ptr ) return( grp_ptr->gr_name );
  else          return( NULL );
}

static char *TruncateDisplayGroupName(char *dest, const char *src,
                                      unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len )
  {
    snprintf(dest, max_len + 1, "%s", src);
  }
  else
  {
    if( max_len < 4 )
    {
      snprintf(dest, max_len + 1, "%.*s", (int)max_len, "...");
    }
    else
    {
      snprintf(dest, max_len + 1, "%.*s...", (int)(max_len - 3), src);
    }
  }
  return( dest );
}

/*
 * GetDisplayGroupName
 * Returns a formatted/truncated group name for display.
 * Uses a static buffer (not thread safe, but safe for ytree TUI).
 */
char *GetDisplayGroupName(unsigned int gid)
{
  const struct group *grp_ptr;

  grp_ptr = getgrgid( (gid_t)gid );

  if( grp_ptr )
  {
    static char buffer[DISPLAY_GROUP_NAME_MAX + 1];
    TruncateDisplayGroupName(buffer, grp_ptr->gr_name, DISPLAY_GROUP_NAME_MAX);
    return buffer;
  }
  else
  {
    return( NULL );
  }
}

/*
 * GetGroupId
 * Returns the GID for a given group name.
 * Returns -1 if not found.
 */
int GetGroupId(char *name)
{
  struct group *grp_ptr;

  grp_ptr = getgrnam( name );

  if( grp_ptr ) return( (int)grp_ptr->gr_gid );
  else          return( -1 );
}
