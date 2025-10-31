/***************************************************************************
 *
 * Change Owner
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirOwner(DirEntry *de_ptr, int new_owner_id);



int ChangeFileOwner(FileEntry *fe_ptr)
{
  WalkingPackage walking_package;
  int  owner_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( owner_id = GetNewOwner( fe_ptr->stat_struct.st_uid ) ) >= 0 )
  {
    walking_package.function_data.change_owner.new_owner_id = owner_id;
    result = SetFileOwner( fe_ptr, &walking_package );
  }
  return( result );
}




int GetNewOwner(int st_uid)
{
  char owner[OWNER_NAME_MAX * 2 +1];
  char *owner_name_ptr;
  int  owner_id;
  int  id;
  
  owner_id = -1;

  id = (st_uid == -1) ? (int) getuid() : st_uid;

  owner_name_ptr = GetPasswdName( id );
  if( owner_name_ptr == NULL )
  {
    (void) sprintf( owner, "%d", id );
  }
  else
  {
    (void) strcpy( owner, owner_name_ptr );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "New Owner:" );

  if( InputString( owner, LINES - 2, 12, 0, OWNER_NAME_MAX, "\r\033" ) == CR )
  {
    if( (owner_id = GetPasswdUid( owner )) == -1 )
    {
      (void) sprintf( message, "Can't read Owner-ID:*%s", owner );
      MESSAGE( message );
    }
  }
  
  move( LINES - 2, 1 ); clrtoeol();
  
  return( owner_id );
}




int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;
  int  new_owner_id;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */
  
  new_owner_id = walking_package->function_data.change_owner.new_owner_id;

  if( !chown( GetFileNamePath( fe_ptr, buffer ), 
	      new_owner_id,
	      fe_ptr->stat_struct.st_gid 
	    ) )
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed" );
    }
    else
    {
      fe_ptr->stat_struct = stat_struct;
    }
    result = 0;
  }
  else
  {
    (void) sprintf( message, "Can't change Owner:*%s", strerror(errno) );
    MESSAGE( message );
  }
 
  return( result );
}





int ChangeDirOwner(DirEntry *de_ptr)
{
  int  owner_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( owner_id = GetNewOwner( de_ptr->stat_struct.st_uid ) ) >= 0 )
  {
    result = SetDirOwner( de_ptr, owner_id );
  }
  return( result );
}






static int SetDirOwner(DirEntry *de_ptr, int new_owner_id)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;

  result = -1;


  if( !chown( GetPath( de_ptr, buffer ), 
	      new_owner_id,
	      de_ptr->stat_struct.st_gid 
	    ) )
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed" );
    }
    else
    {
      de_ptr->stat_struct = stat_struct;
    }
    result = 0;
  }
  else
  {
    (void) sprintf( message, "Can't change Owner:*%s", strerror(errno) );
    MESSAGE( message );
  }
 
  return( result );
}




