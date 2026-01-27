/***************************************************************************
 *
 * src/cmd/passwd.c
 * Handling of user numbers / names
 * Modernized: Direct POSIX calls, no internal caching.
 *
 ***************************************************************************/

#include "ytree.h"
#include <sys/types.h>
#include <pwd.h>
#include <string.h>

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

/*
 * GetDisplayPasswdName
 * Returns a formatted/truncated username for display.
 * Uses a static buffer (not thread safe, but safe for ytree TUI).
 */
char *GetDisplayPasswdName(unsigned int uid)
{
  static char   buffer[DISPLAY_OWNER_NAME_MAX + 1];
  struct passwd *pwd_ptr;

  pwd_ptr = getpwuid( (uid_t)uid );

  if( pwd_ptr )
  {
    CutName(buffer, pwd_ptr->pw_name, DISPLAY_OWNER_NAME_MAX);
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