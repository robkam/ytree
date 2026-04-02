/***************************************************************************
 *
 * src/cmd/passwd.c
 * Handling of user numbers / names
 * Modernized: Direct POSIX calls, no internal caching.
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <pwd.h>
#include <sys/types.h>

/*
 * ReadPasswdEntries
 * Legacy compatibility: Formerly read /etc/passwd into cache.
 * Now a no-op as we query on demand.
 */
int ReadPasswdEntries(void)
{
  return( 0 );
}

/*
 * GetPasswdName
 * Returns the username for a given UID.
 * Returns NULL if not found.
 */
char *GetPasswdName(unsigned int uid)
{
  struct passwd *pwd_ptr;

  pwd_ptr = getpwuid( (uid_t)uid );

  if( pwd_ptr ) return( pwd_ptr->pw_name );
  else          return( NULL );
}

static char *TruncateDisplayOwnerName(char *dest, const char *src,
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
 * GetDisplayPasswdName
 * Returns a formatted/truncated username for display.
 * Uses a static buffer (not thread safe, but safe for ytree TUI).
 */
char *GetDisplayPasswdName(unsigned int uid)
{
  const struct passwd *pwd_ptr;

  pwd_ptr = getpwuid( (uid_t)uid );

  if( pwd_ptr )
  {
    static char buffer[DISPLAY_OWNER_NAME_MAX + 1];
    TruncateDisplayOwnerName(buffer, pwd_ptr->pw_name, DISPLAY_OWNER_NAME_MAX);
    return buffer;
  }
  else
  {
    return( NULL );
  }
}

/*
 * GetPasswdUid
 * Returns the UID for a given username.
 * Returns -1 if not found.
 */
int GetPasswdUid(char *name)
{
  struct passwd *pwd_ptr;

  pwd_ptr = getpwnam( name );

  if( pwd_ptr ) return( (int)pwd_ptr->pw_uid );
  else          return( -1 );
}
