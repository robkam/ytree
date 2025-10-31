/***************************************************************************
 *
 * Statistik-Modul
 *
 ***************************************************************************/


#include "ytree.h"



static void PrettyPrintNumber(int y, int x, LONGLONG number);


void DisplayDiskStatistic(void)
{
  const char *fmt= "[%-17s]";
  char buff[20];
  *buff = '\0';
  
  sprintf( buff, fmt, statistic.file_spec);
  PrintMenuOptions( stdscr, 2, COLS - 18, buff, MENU_COLOR, HIMENUS_COLOR);
  PrettyPrintNumber( 5,  COLS - 17, statistic.disk_space / (LONGLONG)1024 );
  PrintOptions( stdscr, 7,  COLS - 24, "[DISK Statistics   ]" );
  PrettyPrintNumber( 9, COLS - 17, statistic.disk_total_files );
  PrettyPrintNumber( 10, COLS - 17, statistic.disk_total_bytes );
  PrettyPrintNumber( 12, COLS - 17, statistic.disk_matching_files );
  PrettyPrintNumber( 13, COLS - 17, statistic.disk_matching_bytes );
  PrettyPrintNumber( 15, COLS - 17, statistic.disk_tagged_files );
  PrettyPrintNumber( 16, COLS - 17, statistic.disk_tagged_bytes );
  PrintOptions( stdscr, 17, COLS - 24, "[Current Directory    ]");
  DisplayDiskName();
  return;
}



void DisplayAvailBytes(void)
{
  PrettyPrintNumber( 5,  COLS - 17, statistic.disk_space / (LONGLONG)1024 );
  RefreshWindow( stdscr );
}




void DisplayFileSpec(void)
{
  mvwprintw( stdscr, 2,  COLS - 18, "%-17s", statistic.file_spec );
  RefreshWindow( stdscr );
}


void DisplayDiskName(void)
{
  const char *fmt= "[%-17s]";
  char buff[20];
  
  sprintf( buff, fmt, statistic.disk_name);
  PrintMenuOptions( stdscr, 4, COLS - 18, buff, MENU_COLOR, HIMENUS_COLOR);
  RefreshWindow( stdscr );
}




void DisplayDirStatistic(DirEntry *dir_entry)
{
  char format[20];
  char buffer[PATH_LENGTH + 1];
  char auxbuff[PATH_LENGTH + 1];

  *auxbuff = *buffer = '\0';
  (void) sprintf( format, "%%-%ds", COLS - 10 );
  (void) GetPath( dir_entry, statistic.path );
  if (dir_entry -> not_scanned)
     strcat(statistic.path,"*");
  sprintf(auxbuff, format, FormFilename( buffer, statistic.path, MAXIMUM(COLS - 10, 0)));
  wmove( stdscr, 0, 6);
  wclrtoeol( stdscr);
  Print( stdscr, 0, 6, auxbuff, HIMENUS_COLOR);
  PrintOptions( stdscr, 7,  COLS - 24, "[DIR Statistics    ]" );
  PrettyPrintNumber( 9, COLS - 17, dir_entry->total_files );
  PrettyPrintNumber( 10, COLS - 17, dir_entry->total_bytes );
  PrettyPrintNumber( 12, COLS - 17, dir_entry->matching_files );
  PrettyPrintNumber( 13, COLS - 17, dir_entry->matching_bytes );
  PrettyPrintNumber( 15, COLS - 17, dir_entry->tagged_files );
  PrettyPrintNumber( 16, COLS - 17, dir_entry->tagged_bytes );
  PrintOptions( stdscr, 17, COLS - 24, "[Current File        ]" );
  RefreshWindow( stdscr );
  return;
}





void DisplayDirTagged(DirEntry *dir_entry)
{
  PrettyPrintNumber( 15, COLS - 17, dir_entry->tagged_files );
  PrettyPrintNumber( 16, COLS - 17, dir_entry->tagged_bytes );
  
  RefreshWindow( stdscr );
}



void DisplayDiskTagged(void)
{
  PrettyPrintNumber( 15, COLS - 17, statistic.disk_tagged_files );
  PrettyPrintNumber( 16, COLS - 17, statistic.disk_tagged_bytes );
  
  RefreshWindow( stdscr );
}



void DisplayDirParameter(DirEntry *dir_entry)
{
  char *p, *f;
  char format[20];
  char buffer[PATH_LENGTH + 1];
  char auxbuff[PATH_LENGTH + 1];

  p = strrchr( dir_entry->name, FILE_SEPARATOR_CHAR ); 
 
  if( p == NULL ) f = dir_entry->name;
  else            f = p + 1;
 
  (void) sprintf( format, "%%-%ds", COLS - 10 );
  (void) GetPath( dir_entry, statistic.path );
  if (dir_entry -> not_scanned)
     strcat(statistic.path,"*");
  sprintf(auxbuff, format, FormFilename(buffer,statistic.path, MAXIMUM(COLS-10,0)));
  wmove( stdscr, 0, 6);
  wclrtoeol( stdscr);
  Print( stdscr, 0, 6, auxbuff, HIMENUS_COLOR);
  *auxbuff = '\0';
  sprintf(auxbuff, "[%-20s]", CutFilename(buffer, f, 20));
  PrintMenuOptions( stdscr, 18, COLS - 22, auxbuff, MENU_COLOR, HIMENUS_COLOR);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG) dir_entry->total_bytes );
  RefreshWindow( stdscr );
}






void DisplayGlobalFileParameter(FileEntry *file_entry)
{
  char buffer1[PATH_LENGTH+1];
  char buffer2[PATH_LENGTH+3];
  char format[20];

  (void) sprintf( format, "[%%-%ds]", COLS - 10 );
  (void) GetPath( file_entry->dir_entry, buffer1 );
  FormFilename( buffer2, buffer1, MAXIMUM(COLS - 10, 0) );
  sprintf(buffer1, format, buffer2);
  wmove( stdscr, 0, 6);
  wclrtoeol( stdscr);
  PrintMenuOptions( stdscr, 0, 6, buffer1, GLOBAL_COLOR, HIGLOBAL_COLOR);
  CutFilename( buffer1, file_entry->name, 20 );
  sprintf( buffer2, "[%-20s]", buffer1 );
  PrintMenuOptions( stdscr, 18, COLS - 22, buffer2, GLOBAL_COLOR, HIGLOBAL_COLOR);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG) file_entry->stat_struct.st_size );
  RefreshWindow( stdscr );
}




void DisplayFileParameter(FileEntry *file_entry)
{
  char buffer[21*6];
  char auxbuff[23*6];
  sprintf( auxbuff, "[%-20s]", CutFilename( buffer, file_entry->name, 20 ) );
  PrintMenuOptions( stdscr, 18, COLS - 22, auxbuff, MENU_COLOR, HIMENUS_COLOR);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG)file_entry->stat_struct.st_size );
  RefreshWindow( stdscr );
}



void PrettyPrintNumber(int y, int x, LONGLONG number)
{
  char buffer[40];
  long terra, giga, mega, kilo, one;
  
  *buffer = 0;
  terra    = (long)   ( number / (LONGLONG) 1000000000000 );
  giga     = (long) ( ( number % (LONGLONG) 1000000000000 ) / (LONGLONG) 1000000000 );
  mega     = (long) ( ( number % (LONGLONG) 1000000000 ) / (LONGLONG) 1000000 );
  kilo     = (long) ( ( number % (LONGLONG) 1000000 ) / (LONGLONG) 1000 );
  one      = (long)   ( number % (LONGLONG) 1000 );

  if( terra ){
     /* "123123123123123" */
     sprintf( buffer, "[%3ld%3ld%03ld%03ld%03ld]", terra, giga, mega, kilo, one );
     PrintMenuOptions( stdscr, y, x, buffer, MENU_COLOR, HIMENUS_COLOR);
     }
  if( giga ){
     /* "123,123,123,123" */
     sprintf( buffer, "[%3ld%c%03ld%c%03ld%c%03ld]", giga, number_seperator, mega, number_seperator, kilo, number_seperator, one );
     PrintMenuOptions( stdscr, y, x, buffer, MENU_COLOR, HIMENUS_COLOR);
     }
  else if( mega ) {
     /* "    123,123,123" */
     sprintf( buffer, "[    %3ld%c%03ld%c%03ld]", mega, number_seperator, kilo,  number_seperator, one );
     PrintMenuOptions( stdscr, y, x, buffer, MENU_COLOR, HIMENUS_COLOR);
     }
  else if( kilo ) {
     /* "        123,123" */
     sprintf( buffer, "[        %3ld%c%03ld]", kilo, number_seperator, one);
     PrintMenuOptions( stdscr, y, x, buffer, MENU_COLOR, HIMENUS_COLOR);
     }
  else {
     /* "            123" */
     sprintf( buffer, "[            %3ld]", one);
     PrintMenuOptions( stdscr, y, x, buffer, MENU_COLOR, HIMENUS_COLOR);
     }
 return;
}



