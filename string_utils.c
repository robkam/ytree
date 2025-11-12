/***************************************************************************
 *
 * Generic string manipulation functions
 *
 ***************************************************************************/


#include "ytree.h"


int Strrcmp(char *s1, char* s2)/*compares in reverse order 2 strings*/
{
   int aux;
   int l1 = strlen(s1);
   int l2 = strlen(s2);

   for (aux = 0; aux <= l2; aux++)
   {
      if ((l1 - aux) < 0) 
	return(-1);
      if (s1[l1 - aux] > s2[l2 - aux])
         return(1);
      else if (s1[l1 - aux] < s2[l2 - aux])
         return(-1);
   }
   return(0);
}


void StrCp(char *dest, const char *src)
{
   static char esc_chars[] ="#*|&;()<> \t\n\r\"!$?'`~";

   while(*src)
   {
     if(strchr(esc_chars, *src))
       *dest++ = '\\';
     *dest++ = *src++;
   }
   *dest = '\0';
}


int BuildFilename( char *in_filename,
		   char *pattern,
		   char *out_filename
		 )
{
  char *cptr;
  int  result = 0;


  for( ; *pattern; pattern++ )
  {
    if( *pattern == '*' )
    {
      cptr = in_filename;
      for( ; (*out_filename = *cptr); out_filename++, cptr++ );
    }
    else
    {
      *out_filename++ = *pattern;
    }
  }

  *out_filename = '\0';

  return( result );
}


char *SubString(char *dest, char *src, int pos, int len)
{
  strncpy(dest, &src[pos], len);
  dest[len] = '\0';
  return dest;
}