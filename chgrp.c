/***************************************************************************
 *
 * Change Group
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirGroup(DirEntry *de_ptr, int new_group_id);



int ChangeFileGroup(FileEntry *fe_ptr)
{
  WalkingPackage walking_package;
  int  group_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup( fe_ptr->stat_struct.st_gid ) ) >= 0 )
  {
    walking_package.function_data.change_group.new_group_id = group_id;
    result = SetFileGroup( fe_ptr, &walking_package );
  }
  return( result );
}




int GetNewGroup(int st_gid)
{
  char group[GROUP_NAME_MAX * 2 +1];
  char *group_name_ptr;
  int  id;
  int  group_id;

  group_id = -1;
  
  id = ( st_gid == -1 ) ? (int) getgid() : st_gid;

  group_name_ptr = GetGroupName( id );
  if( group_name_ptr == NULL )
  {
    (void) sprintf( group, "%d", id );
  }
  else
  {
    (void) strcpy( group, group_name_ptr );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "New Group:" );

  if( InputString( group, LINES - 2, 12, 0, GROUP_NAME_MAX, "\r\033" ) == CR )
  {
    if( (group_id = GetGroupId( group )) == -1 )
    {
      (void) sprintf( message, "Can't read Group-ID:*\"%s\"", group );
      MESSAGE( message );
    }
  }
  
  move( LINES - 2, 1 ); clrtoeol();
  
  return( group_id );
}




int SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;
  int  new_group_id;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */
  
  new_group_id = walking_package->function_data.change_group.new_group_id;

  if( !chown( GetFileNamePath( fe_ptr, buffer ), 
	      fe_ptr->stat_struct.st_uid ,
	      new_group_id
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
    (void) sprintf( message, "Can't change owner:*%s", strerror(errno) );
    MESSAGE( message );
  }
 
  return( result );
}




int ChangeDirGroup(DirEntry *de_ptr)
{
  int  group_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup( de_ptr->stat_struct.st_gid ) ) >= 0 )
  {
    result = SetDirGroup( de_ptr, group_id );
  }
  return( result );
}






static int SetDirGroup(DirEntry *de_ptr, int new_group_id)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;

  result = -1;


  if( !chown( GetPath( de_ptr, buffer ), 
	      de_ptr->stat_struct.st_uid ,
	      new_group_id
	    ) )
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      Warning( "stat failed" );
    }
    else
    {
      de_ptr->stat_struct = stat_struct;
    }
    result = 0;
  }
  else
  {
    (void) sprintf( message, "Can't change owner:*%s", strerror(errno) );
    MESSAGE( message );
  }
 
  return( result );
}



