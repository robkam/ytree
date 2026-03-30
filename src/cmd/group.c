/***************************************************************************
 *
 * src/cmd/group.c
 * Handling of group numbers / names
 * Modernized: Direct POSIX calls, no internal caching.
 *
 ***************************************************************************/

#include "ytree_ui.h"
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
    CutName(buffer, grp_ptr->gr_name, DISPLAY_GROUP_NAME_MAX);
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
