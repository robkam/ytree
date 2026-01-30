/***************************************************************************
 *
 * src/core/sort.c
 * Core sorting logic for file entry lists within a Volume.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "ytree.h"
#include "sort.h"

static BOOL g_sort_ascending;
static BOOL g_sort_case_sensitive;

/* Forward declarations */
static int SortByName(FileEntryList *e1, FileEntryList *e2);
static int SortByChgTime(FileEntryList *e1, FileEntryList *e2);
static int SortByAccTime(FileEntryList *e1, FileEntryList *e2);
static int SortByModTime(FileEntryList *e1, FileEntryList *e2);
static int SortBySize(FileEntryList *e1, FileEntryList *e2);
static int SortByOwner(FileEntryList *e1, FileEntryList *e2);
static int SortByGroup(FileEntryList *e1, FileEntryList *e2);
static int SortByExtension(FileEntryList *e1, FileEntryList *e2);

void Panel_Sort(YtreePanel *panel, int method)
{
  int aux;
  int (*compare)(FileEntryList *, FileEntryList *);

  if (!panel || !panel->file_entry_list || panel->file_count == 0)
    return;

  g_sort_ascending = (method < SORT_DSC);
  g_sort_case_sensitive = FALSE;

  aux = method;
  if (aux > SORT_DSC)
  {
     aux -= SORT_DSC;
  }
  else
  {
     aux -= SORT_ASC;
  }

  switch( aux )
  {
    case SORT_BY_NAME :      compare = SortByName; break;
    case SORT_BY_MOD_TIME :  compare = SortByModTime; break;
    case SORT_BY_CHG_TIME :  compare = SortByChgTime; break;
    case SORT_BY_ACC_TIME :  compare = SortByAccTime; break;
    case SORT_BY_OWNER :     compare = SortByOwner; break;
    case SORT_BY_GROUP :     compare = SortByGroup; break;
    case SORT_BY_SIZE :      compare = SortBySize; break;
    case SORT_BY_EXTENSION : compare = SortByExtension; break;
    default:                 compare = SortByName;
  }

  qsort( (char *) panel->file_entry_list,
	 panel->file_count,
	 sizeof( panel->file_entry_list[0] ),
	 (int (*)(const void *, const void *)) compare
	);
}

static int SortByName(FileEntryList *e1, FileEntryList *e2)
{
  if (g_sort_case_sensitive)
     if (g_sort_ascending)
        return( strcmp( e1->file->name, e2->file->name ) );
     else
        return( - (strcmp( e1->file->name, e2->file->name ) ) );
  else
     if (g_sort_ascending)
        return( strcasecmp( e1->file->name, e2->file->name ) );
     else
        return( - (strcasecmp( e1->file->name, e2->file->name ) ) );
}


static int SortByExtension(FileEntryList *e1, FileEntryList *e2)
{
  char *ext1, *ext2;
  int cmp, casecmp;

  /* Ok, this isn't optimized */

  ext1 = GetExtension(e1->file->name);
  ext2 = GetExtension(e2->file->name);
  cmp=strcmp( ext1, ext2 );
  casecmp=strcasecmp( ext1, ext2 );

  if (g_sort_case_sensitive && !cmp)
      return SortByName( e1, e2 );
  if (!g_sort_case_sensitive && !casecmp)
      return SortByName( e1, e2 );


  if (g_sort_case_sensitive)
     if (g_sort_ascending)
        return( strcmp( ext1, ext2 ) );
     else
        return( - (strcmp( ext1, ext2 ) ) );
  else
     if (g_sort_ascending)
        return( strcasecmp( ext1, ext2 ) );
     else
        return( - (strcasecmp( ext1, ext2 ) ) );
}


static int SortByModTime(FileEntryList *e1, FileEntryList *e2)
{
  if (g_sort_ascending)
     return( e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime );
  else
     return( - (e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime ) );
}

static int SortByChgTime(FileEntryList *e1, FileEntryList *e2)
{
  if (g_sort_ascending)
     return( e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime );
  else
     return( - (e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime ) );
}

static int SortByAccTime(FileEntryList *e1, FileEntryList *e2)
{
  if (g_sort_ascending)
     return( e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime );
  else
     return( - (e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime ) );
}

static int SortBySize(FileEntryList *e1, FileEntryList *e2)
{
  int result = 0;

  if(e1->file->stat_struct.st_size < e2->file->stat_struct.st_size)
    result = 1;
  else if (e1->file->stat_struct.st_size > e2->file->stat_struct.st_size)
    result = -1;

  if(g_sort_ascending)
    result *= -1;

  return result;
}


static int SortByOwner(FileEntryList *e1, FileEntryList *e2)
{
  char *o1, *o2;
  char n1[10], n2[10];

  o1 = GetPasswdName( e1->file->stat_struct.st_uid );
  o2 = GetPasswdName( e2->file->stat_struct.st_uid );

  if( o1 == NULL )
  {
    (void) snprintf( n1, sizeof(n1), "%d", (int) e1->file->stat_struct.st_uid );
    o1 = n1;
  }

  if( o2 == NULL )
  {
    (void) snprintf( n2, sizeof(n2), "%d", (int) e2->file->stat_struct.st_uid );
    o2 = n2;
  }
  if (g_sort_case_sensitive)
     if (g_sort_ascending)
        return( strcmp( o1, o2 ) );
     else
        return( - (strcmp( o1, o2 ) ) );
  else
     if (g_sort_ascending)
        return( strcasecmp( o1, o2 ) );
     else
        return( - (strcasecmp( o1, o2 ) ) );
}



static int SortByGroup(FileEntryList *e1, FileEntryList *e2)
{
  char *g1, *g2;
  char n1[10], n2[10];

  g1 = GetGroupName( e1->file->stat_struct.st_gid );
  g2 = GetGroupName( e2->file->stat_struct.st_gid );

  if( g1 == NULL )
  {
    (void) snprintf( n1, sizeof(n1), "%d", (int) e1->file->stat_struct.st_uid );
    g1 = n1;
  }

  if( g2 == NULL )
  {
    (void) snprintf( n2, sizeof(n2), "%d", (int) e2->file->stat_struct.st_uid );
    g2 = n2;
  }
  if (g_sort_case_sensitive)
     if (g_sort_ascending)
        return( strcmp( g1, g2 ) );
     else
        return( - (strcmp( g1, g2 ) ) );
  else
     if (g_sort_ascending)
        return( strcasecmp( g1, g2 ) );
     else
        return( - (strcasecmp( g1, g2 ) ) );
}
