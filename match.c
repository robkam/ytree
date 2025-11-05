/***************************************************************************
 *
 * Behandlung reg. Ausdruecke fuer Dateinamen
 *
 ***************************************************************************/


#include "ytree.h"
#include <regex.h>


static regex_t	re;
static BOOL	re_flag = FALSE;


int SetMatchSpec(char *new_spec)
{
  char *buffer;
  char *b_ptr;
  BOOL meta_flag = FALSE;
  int result = 0; /* Assume success */


  if( ( buffer = (char *)malloc( strlen( new_spec ) * 2 + 4 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  b_ptr = buffer;
  *b_ptr++ = '^';

  for(; *new_spec; new_spec++)
  {
    if( meta_flag ) 
    {
      *b_ptr++ = *new_spec;
      meta_flag = FALSE;
    }
    else if( *new_spec == '\\' ) meta_flag = TRUE;
    else if( *new_spec == '?' ) *b_ptr++ = '.';
    else if( *new_spec == '.' )
    {
      *b_ptr++ = '\\';
      *b_ptr++ = '.';
    }
    else if( *new_spec == '*' )
    {
      *b_ptr++ = '.';
      *b_ptr++ = '*';
    }
    else
      *b_ptr++ = *new_spec;
  }

  *b_ptr++ = '$';
  *b_ptr = '\0';


  if(re_flag) 
  {
    regfree(&re);
    re_flag = FALSE;
  }
  
  if( regcomp(&re, buffer, REG_NOSUB | REG_EXTENDED) ) /* Use REG_EXTENDED for consistency */
  {
    result = 1;
  }
  else 
  {
    re_flag = TRUE;
  }
  
  free( buffer );
  return( result );
}



BOOL Match(char *file_name)
{
  if(re_flag == FALSE)
    return( TRUE ); /* No regex compiled, match all */

  /* REG_NOSUB was used in regcomp, so don't pass capture info */
  if( ( regexec(&re, file_name, (size_t) 0, NULL, 0 ) ) == 0 )
  {
    return( TRUE );
  }
  else 
  {
    return( FALSE );
  }
}