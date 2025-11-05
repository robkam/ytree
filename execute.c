/***************************************************************************
 *
 * Ausfuehren von System-Kommandos
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);


int Execute(DirEntry *dir_entry, FileEntry *file_entry)
{
  static char command_line[COMMAND_LINE_LENGTH + 1];
  char cwd[PATH_LENGTH+1];
  char path[PATH_LENGTH+1];
  int  result;

  result = -1;

  if( file_entry )
  {
    if( file_entry->stat_struct.st_mode & 
	( S_IXUSR | S_IXGRP | S_IXOTH ) )
    {
      /* ausfuehrbare Datei */
      /*--------------------*/

      (void) StrCp( command_line, file_entry->name );
    }
  }

  MvAddStr( LINES - 2, 1, "Command:" );
  if( !GetCommandLine( command_line ) )
  {
    if( getcwd( cwd, PATH_LENGTH ) == NULL )
    {
      WARNING( "getcwd failed*\".\"assumed" );
      (void) strcpy( cwd, "." );
    }

    if( mode == DISK_MODE || mode == USER_MODE )
    {
      if( chdir( GetPath( dir_entry, path ) ) )
      {
        (void) sprintf( message, "Can't change directory to*\"%s\"", path );
        MESSAGE( message );
      }
      else
      {
        refresh();
        result = QuerySystemCall( command_line );
      }
      if( chdir( cwd ) )
      {
        (void) sprintf( message, "Can't change directory to*\"%s\"", cwd );
        MESSAGE( message );
      }
    }
    else
    {
      refresh();
      result = QuerySystemCall( command_line );
    }
  }
  
  return( result );
}




int GetCommandLine(char *command_line)
{
  int result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Command: " );
  if( InputString( command_line, LINES - 2, 10, 0, COLS - 11, "\r\033" ) == CR )
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}

    

int GetSearchCommandLine(char *command_line)
{
  int  result;
  int  pos;
  char *cptr;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Search untag command: " );
  strcpy( command_line, SEARCHCOMMAND );

  cptr = strstr( command_line, "{}" );
  if(cptr) {
    pos = (cptr - command_line) - 1;
    if(pos < 0)
      pos = 0;
  } else {
    pos = 0;
  }
  if( InputString( command_line, LINES - 2, 23, pos, COLS - 24, "\r\033" ) == CR )
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}

    

int ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  char command_line[COMMAND_LINE_LENGTH + 1];
  int i, result;
  char c;
  char *cptr;

  command_line[0] = '\0';
  cptr = command_line;

  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */

  for( i=0; (c = walking_package->function_data.execute.command[i]); i++ )
  {
    if( c == '{' && walking_package->function_data.execute.command[i+1] == '}' )
    {
      (void) GetFileNamePath( fe_ptr, cptr );
      cptr = &command_line[ strlen( command_line ) ];
      i++;
    }
    else
    {
      *cptr++ = c;
    }
  }
  *cptr = '\0';

  result = SilentSystemCallEx( command_line, FALSE );

  /* Ignore Result */
  /*---------------*/

  return( result );
}