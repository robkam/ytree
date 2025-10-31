/***************************************************************************
 *
 * Funktionen zur Handhabung des FILE-Windows
 *
 ***************************************************************************/


#include "ytree.h"


#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

int DirUserMode(DirEntry *dir_entry, int ch)
{
  int chremap;
  char *command_line, *aux;
  char filepath[PATH_LENGTH +1];

  GetPath( dir_entry, filepath );
  command_line = NULL;
  
  while (( aux = GetUserDirAction(ch, &chremap)) != NULL) {
     if( command_line == NULL &&
          ( command_line = malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
     {
        ERROR_MSG( "Malloc failed*ABORT" );
        exit( 1 );
     }
     if (strstr(aux,"%s") != NULL) {
  	(void) sprintf( command_line, aux, filepath );
     } else {
  	(void) sprintf( command_line, "%s%c%s", aux, ' ', filepath );
     }
     if(SilentSystemCall( command_line ))
     {
       (void) sprintf( message, "can't execute*%s", command_line );
       MESSAGE( message );
     }
     if (chremap == ch || chremap == 0)
       break;
     else
       ch = chremap;
  }

  if (command_line != NULL)
     free( command_line );

  return(ch);
}

int FileUserMode(FileEntryList *file_entry_list, int ch)
{
  int chremap;
  char filepath[PATH_LENGTH +1];
  char *command_line, *aux;

  GetRealFileNamePath( file_entry_list->file, filepath );

  command_line = NULL;
  while (( aux = GetUserFileAction(ch, &chremap)) != NULL) {
     if( command_line == NULL &&
          ( command_line = malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
     {
        ERROR_MSG( "Malloc failed*ABORT" );
        exit( 1 );
     }
  
     if (strstr(aux,"%s") != NULL) {
  	(void) sprintf( command_line, aux, filepath );
     } else {
  	(void) sprintf( command_line, "%s%c%s", aux, ' ', filepath );
     }
     if(SilentSystemCall( command_line ))
     {
       (void) sprintf( message, "can't execute*%s", command_line );
       MESSAGE( message );
     }
     if (chremap == ch || chremap == 0)
       break;
     else
       ch = chremap;
  }

  if (command_line != NULL)
     free( command_line );

  return(chremap);
}

