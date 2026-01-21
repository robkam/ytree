/***************************************************************************
 *
 * src/util/string_utils.c
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
  const char *s = in_filename;
  const char *p = pattern;
  char *d = out_filename;

  while (*p) {
    if (*p == '*') {
        char next_char = *(p + 1);
        if (next_char == '\0') {
            /* Optimization: pattern ends with *, copy rest of source */
            strcpy(d, s);
            return 0;
        }

        const char *stop;
        if (next_char == '.') {
            /* Special Case: Extension. Find LAST dot. */
            stop = strrchr(s, '.');
        } else {
            /* General Case: Find FIRST occurrence of next delimiter */
            stop = strchr(s, next_char);
        }

        if (stop) {
            size_t len = stop - s;
            strncpy(d, s, len);
            d += len;
            s = stop; /* Advance source to the delimiter */
        } else {
            /* Delimiter not found in source, consume remainder */
            strcpy(d, s);
            d += strlen(s);
            s += strlen(s);
        }
        p++; /* Consume '*' */
    } else if (*p == '?') {
        if (*s) {
            *d++ = *s++;
        } else {
            *d++ = '?'; /* Source exhausted, keep literal placeholder? */
        }
        p++;
    } else if (*p == '.') {
        /* Literal dot: synchronize with the extension separator in source */
        char *last_dot = strrchr(in_filename, '.');
        if (last_dot) {
            s = last_dot;
        }
        *d++ = *p;
        /* If source is now at the dot, consume it to stay in sync */
        if (*s == *p) {
            s++;
        }
        p++;
    } else {
        /* Literal character */
        *d++ = *p;
        /* If source matches the pattern literal, consume source to stay in sync */
        if (*s == *p) {
            s++;
        }
        p++;
    }
  }
  *d = '\0';

  return( 0 );
}


char *SubString(char *dest, char *src, int pos, int len)
{
  strncpy(dest, &src[pos], len);
  dest[len] = '\0';
  return dest;
}
