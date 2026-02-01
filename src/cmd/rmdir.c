/***************************************************************************
 *
 * src/cmd/rmdir.c
 * Deleting directories
 *
 ***************************************************************************/


#include "ytree.h"

/* Helper to resolve Panel from Volume */
static YtreePanel *GetPanelForVol(struct Volume *vol) {
    if (!vol) return NULL;
    if (ActivePanel && ActivePanel->vol == vol) return ActivePanel;
    if (IsSplitScreen) {
        if (LeftPanel && LeftPanel->vol == vol) return LeftPanel;
        if (RightPanel && RightPanel->vol == vol) return RightPanel;
    }
    return NULL;
}


static int DeleteSubTree(DirEntry *dir_entry);
static int DeleteSingleDirectory(DirEntry *dir_entry);


static int RmdirProgressCallback(int status, const char *msg, void *user_data)
{
    (void)msg;
    (void)user_data;

    if (status == ARCHIVE_STATUS_PROGRESS) {
        DrawSpinner();
        if (EscapeKeyPressed()) {
            return ARCHIVE_CB_ABORT;
        }
    }
    /* We could handle ARCHIVE_STATUS_ERROR here if we wanted custom message handling,
     * but the caller usually handles the final result. */
    return ARCHIVE_CB_CONTINUE;
}


int DeleteDirectory(DirEntry *dir_entry)
{
  char buffer[PATH_LENGTH+1];
  int result = -1;

  /* Added check for ARCHIVE_MODE */
  if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
  {
    return( result );
  }

  ClearHelp();

  if( dir_entry == CurrentVolume->vol_stats.tree )
  {
    MESSAGE( "Can't delete ROOT" );
  }
#ifdef HAVE_LIBARCHIVE
  else if (CurrentVolume->vol_stats.login_mode == ARCHIVE_MODE)
  {
      if (dir_entry->file || dir_entry->sub_tree) {
          MESSAGE("Directory not empty");
      } else if( InputChoice( "Delete this directory (Y/N) ? ", "YN\033" ) == 'Y' ) {
          GetPath(dir_entry, buffer);

          if (Archive_DeleteEntry(CurrentVolume->vol_stats.login_path, buffer, RmdirProgressCallback, NULL) == 0) {
              /* Success - Update UI */
              YtreePanel *p = GetPanelForVol(CurrentVolume);
              if (p) RefreshTreeSafe(p, CurrentVolume->vol_stats.tree);
              result = 0;
          }
      }
  }
#endif
  else if( dir_entry->file || dir_entry->sub_tree )
  {
    if( InputChoice( "Directory not empty, PRUNE ? (Y/N) ? ", "YN\033" ) == 'Y' ) {
      if( dir_entry->sub_tree ) {
        if( ScanSubTree( dir_entry, &CurrentVolume->vol_stats ) ) {
	  ESCAPE;
	}
        if( DeleteSubTree( dir_entry->sub_tree ) ) {
          ESCAPE;
        }
      }
      if( DeleteSingleDirectory( dir_entry ) ) {
	ESCAPE;
      }
      result = 0;
      ESCAPE;
    }
  }
  else if( InputChoice( "Delete this directory (Y/N) ? ", "YN\033" ) == 'Y' )
  {
    (void) GetPath( dir_entry, buffer );

    if( access( buffer, W_OK ) )
    {
      MESSAGE( "Can't delete directory*\"%s\"*%s",
		      buffer, strerror(errno)
		    );
    }
    else if( rmdir( buffer ) )
    {
      MESSAGE( "Can't delete directory*\"%s\"*%s",
		      buffer, strerror(errno)
		    );
    }
    else
    {
      /* Directory geloescht
       * ==> aus Baum loeschen
       */

      CurrentVolume->vol_stats.disk_total_directories--;

      if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
      else dir_entry->up_tree->sub_tree = dir_entry->next;

      if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

      free( dir_entry );

      (void) GetAvailBytes( &CurrentVolume->vol_stats.disk_space, &CurrentVolume->vol_stats );

      result = 0;
    }
  }

FNC_XIT:

  return( result );
}





static int DeleteSubTree( DirEntry *dir_entry )
{
  int result = -1;
  DirEntry *de_ptr, *next_de_ptr;


  for( de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr ) {
    next_de_ptr = de_ptr->next;

    if( de_ptr->sub_tree ) {
      if( DeleteSubTree( de_ptr->sub_tree ) ) {
        ESCAPE;
      }
    }
    if( DeleteSingleDirectory( de_ptr ) ) {
      ESCAPE;
    }
  }

  result = 0;

FNC_XIT:

    return( result );
}



static int DeleteSingleDirectory( DirEntry *dir_entry )
{
  int result = -1;
  char buffer[PATH_LENGTH+1];
  FileEntry *fe_ptr, *next_fe_ptr;
  int force = 1;


  (void) GetPath( dir_entry, buffer );


  if( access( buffer, W_OK ) ) {
    MESSAGE( "Can't delete directory*\"%s\"*%s",
		    buffer, strerror(errno)
		    );
    ESCAPE;
  }

  for( fe_ptr = dir_entry->file; fe_ptr; fe_ptr=next_fe_ptr ) {
    next_fe_ptr = fe_ptr->next;
    /* Spinner to indicate progress during bulk deletion */
    DrawSpinner();
    doupdate();
    if( DeleteFile( fe_ptr, &force, &CurrentVolume->vol_stats ) ) {
      ESCAPE;
    }
  }

  if( rmdir( buffer ) ) {
    MESSAGE( "Can't delete directory*\"%s\"*%s",
		    buffer, strerror(errno)
		  );
    ESCAPE;
  }

  if( !dir_entry->up_tree->not_scanned )
    CurrentVolume->vol_stats.disk_total_directories--;

  if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
  else dir_entry->up_tree->sub_tree = dir_entry->next;
  if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

  free( dir_entry );

  result = 0;

FNC_XIT:

  return( result );
}