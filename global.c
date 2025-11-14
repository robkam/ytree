/***************************************************************************
 *
 * global.c
 * Externe Groessen
 *
 ***************************************************************************/


#include "ytree.h"



WINDOW *dir_window;
WINDOW *small_file_window;
WINDOW *big_file_window;
WINDOW *file_window;
WINDOW *error_window;
WINDOW *history_window;
WINDOW *matches_window;
WINDOW *f2_window;
WINDOW *time_window;

Statistic statistic;
Statistic disk_statistic;
int       mode;
int	  user_umask;
char      message[MESSAGE_LENGTH + 1];
BOOL	  print_time;
BOOL	  resize_request;
BOOL	  bypass_small_window;
BOOL      highlight_full_line;
char      number_seperator;
char	  *initial_directory;

FileColorRule *file_color_rules_head = NULL;