/***************************************************************************
*
* src/cmd/delete.c
* Deleting files / directories
*
***************************************************************************/


#include "ytree.h"


extern int unlink(const char *);
extern int rmdir(const char *);


/* Helper to resolve Panel from Stats context */
static YtreePanel *GetPanelForStats(Statistic *s) {
    if (!s) return NULL;
    if (ActivePanel && &ActivePanel->vol->vol_stats == s) return ActivePanel;
    if (IsSplitScreen) {
        if (LeftPanel && &LeftPanel->vol->vol_stats == s) return LeftPanel;
        if (RightPanel && &RightPanel->vol->vol_stats == s) return RightPanel;
    }
    return NULL;
}

/* Helper for Archive Callback */
static int ArchiveUICallback(int status, const char *msg, void *user_data) {
(void)user_data;
if (status == ARCHIVE_STATUS_PROGRESS) {
DrawSpinner();
if (EscapeKeyPressed()) {
return ARCHIVE_CB_ABORT;
}
} else if (status == ARCHIVE_STATUS_ERROR) {
if (msg) ERROR_MSG("%s", msg);
} else if (status == ARCHIVE_STATUS_WARNING) {
if (msg) WARNING("%s", msg);
}
return ARCHIVE_CB_CONTINUE;
}

int DeleteFile(FileEntry *fe_ptr, int *auto_override, Statistic *s)
{
char     filepath[PATH_LENGTH+1];
char	   buffer[PATH_LENGTH+1];
int      result;
int      term;

result = -1;

(void) GetFileNamePath( fe_ptr, filepath );

/* Handle Archive Mode Deletion Hook */
#ifdef HAVE_LIBARCHIVE
if (s->login_mode == ARCHIVE_MODE) {
/* In archive mode, permissions are virtual. We skip access checks and unlink.
* We rely on the Rewrite Engine to handle the deletion.
*/
if (Archive_DeleteEntry(s->login_path, filepath, ArchiveUICallback, NULL) == 0) {
/* Success. The archive container has been rewritten.
* Explicitly refresh the tree to update UI.
*/
YtreePanel *p = GetPanelForStats(s);
if (p) RefreshTreeSafe(p, s->tree);
else RescanDir(s->tree, strtol(TREEDEPTH, NULL, 0), s, NULL, NULL);

return 0;
} else {
return -1;
}
}
#endif

if( !S_ISLNK( fe_ptr->stat_struct.st_mode ) )
{
if( access( filepath, W_OK ) )
{
if( access( filepath, F_OK ) )
{
/* File does not exist ==> done */

goto UNLINK_DONE;
}

if (auto_override && *auto_override == 1) {
term = 'Y';
} else {
(void) snprintf( buffer, sizeof(buffer), "overriding mode %04o for \"%s\" (Y/N/A) ? ",
fe_ptr->stat_struct.st_mode & 0777, fe_ptr->name);

term = InputChoice( buffer, "YNA\033" );
}

if (term == 'A') {
if (auto_override) *auto_override = 1;
term = 'Y';
}

if( term != 'Y' )
{
MESSAGE( "Can't delete file*\"%s\"*%s",
filepath,
strerror(errno)
);
ESCAPE;
}
flushinp();
}
}

if( unlink( filepath ) )
{
MESSAGE( "Can't delete file*\"%s\"*%s",
filepath,
strerror(errno)
);
ESCAPE;
}

UNLINK_DONE:

/* Remove file record */
/*----------------*/

result = RemoveFile( fe_ptr, s );
(void) GetAvailBytes( &s->disk_space, s );

FNC_XIT:

return( result );
}





int RemoveFile(FileEntry *fe_ptr, Statistic *s)
{
DirEntry *de_ptr;
LONGLONG file_size;

de_ptr = fe_ptr->dir_entry;

file_size = fe_ptr->stat_struct.st_size;

de_ptr->total_bytes -= file_size;
de_ptr->total_files--;
s->disk_total_bytes -= file_size;
s->disk_total_files--;
if( fe_ptr->matching ) {
de_ptr->matching_bytes -= file_size;
de_ptr->matching_files--;
s->disk_matching_bytes -= file_size;
s->disk_matching_files--;
}
if( fe_ptr->tagged )
{
de_ptr->tagged_bytes -= file_size;
de_ptr->tagged_files--;
s->disk_tagged_bytes -= file_size;
s->disk_tagged_files--;
}

/* Remove file record */
/*----------------*/

if( fe_ptr->next ) fe_ptr->next->prev = fe_ptr->prev;
if( fe_ptr->prev ) fe_ptr->prev->next = fe_ptr->next;
else
de_ptr->file = fe_ptr->next;

free( (char *) fe_ptr );

return( 0 );
}