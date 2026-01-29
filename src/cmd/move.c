/***************************************************************************
*
* src/cmd/move.c
* Move files
*
***************************************************************************/


#include "ytree.h"


/* Buffers for Prompt Redraw Callback - Deprecated by UI_ReadString, keeping for safety if needed */
static char move_prompt_header[PATH_LENGTH + 50];
static char move_prompt_as[PATH_LENGTH + 1];

/* Callback to redraw prompt after F2 refresh during Step 1 */
static void RedrawMovePrompt1(void) {
/* No-op with UI_ReadString */
}

/* Callback to redraw prompt after F2 refresh during Step 2 */
static void RedrawMovePrompt2(void) {
/* No-op with UI_ReadString */
}


static int Move(char *to_path, char *from_path);


int MoveFile(FileEntry *fe_ptr,
unsigned char confirm,
char *to_file,
DirEntry *dest_dir_entry,
char *to_dir_path,
FileEntry **new_fe_ptr,
int *dir_create_mode,
int *overwrite_mode
)
{
DirEntry    *de_ptr;
LONGLONG    file_size;
char        from_path[PATH_LENGTH+1];
char        to_path[PATH_LENGTH+1];
FileEntry   *dest_file_entry;
FileEntry   *fen_ptr;
struct stat stat_struct;
int         term;
int         result;
/* New variables for EnsureDirectoryExists */
char        abs_path[PATH_LENGTH+1];
char        from_dir[PATH_LENGTH+1];
BOOL        refresh_dirwindow = FALSE;
int         force = 1; /* Force delete for overwrite */

/* Context-Aware Variables */
struct Volume *target_vol = NULL;
DirEntry *target_tree = NULL;
Statistic *target_stats_ptr = NULL;
Statistic *s = &CurrentVolume->vol_stats;

result = -1;
*new_fe_ptr = NULL;
de_ptr = fe_ptr->dir_entry;

(void) GetPath( de_ptr, from_dir ); /* Get clean source directory path */
(void) strcpy( from_path, from_dir );
(void) strcat( from_path, FILE_SEPARATOR_STRING );
(void) strcat( from_path, fe_ptr->name );

/* Construct base destination path */
(void) strcpy( to_path, to_dir_path );
(void) strcat( to_path, FILE_SEPARATOR_STRING );

/* Handle relative path: make absolute based on source directory */
if (*to_path != FILE_SEPARATOR_CHAR) {
strcpy(abs_path, from_dir);
strcat(abs_path, FILE_SEPARATOR_STRING);
strcat(abs_path, to_path);
strcpy(to_path, abs_path);
}

/* Identify Target Volume */
target_vol = Volume_GetByPath(to_path);
if (target_vol) {
target_tree = target_vol->vol_stats.tree;
target_stats_ptr = &target_vol->vol_stats;
} else {
/* Fallback to current if path matches current tree prefix, else NULL (external) */
if (strncmp(s->tree->name, to_path, strlen(s->tree->name)) == 0) {
target_tree = s->tree;
target_stats_ptr = s;
} else {
target_tree = NULL;
target_stats_ptr = NULL;
}
}


/* Ensure the destination directory exists */
{
BOOL created = FALSE;
/* FIX: Pass &dest_dir_entry to update the pointer */
if (EnsureDirectoryExists(to_path, target_tree, &created, &dest_dir_entry, dir_create_mode) == -1) {
return -1;
}
if (created) refresh_dirwindow = TRUE;
}

(void) strcat( to_path, to_file );


if( !strcmp( to_path, from_path ) )
{
MESSAGE( "Can't move file into itself" );
ESCAPE;
}


if( access( from_path, W_OK ) )
{
MESSAGE( "Unmoveable file*\"%s\"*%s",
from_path,
strerror(errno)
);
ESCAPE;
}


if( dest_dir_entry )
{
/* destination is in sub-tree */
/*----------------------------*/

(void) GetFileEntry( dest_dir_entry, to_file, &dest_file_entry );

if( dest_file_entry )
{
/* file exists */
/*-------------*/

if( confirm )
{
if (overwrite_mode && *overwrite_mode == 1) {
term = 'Y';
} else {
term = InputChoice( "file exist; overwrite (Y/N/A) ? ", "YNA\033" );
}

if (term == 'A') {
if (overwrite_mode) *overwrite_mode = 1;
term = 'Y';
}

if( term != 'Y' ) {
result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
ESCAPE;
}
}

(void) DeleteFile( dest_file_entry, (overwrite_mode && *overwrite_mode == 1) ? &force : NULL, target_stats_ptr ? target_stats_ptr : s );
}
}
else
{
/* use access */
/*------------*/

if( !access( to_path, F_OK ) )
{
/* file exists */
/*-------------*/

if( confirm )
{
if (overwrite_mode && *overwrite_mode == 1) {
term = 'Y';
} else {
term = InputChoice( "file exist; overwrite (Y/N/A) ? ", "YNA\033" );
}

if (term == 'A') {
if (overwrite_mode) *overwrite_mode = 1;
term = 'Y';
}

if( term != 'Y' ) {
result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
ESCAPE;
}
}

if( unlink( to_path ) )
{
MESSAGE( "Can't unlink*\"%s\"*%s",
to_path,
strerror(errno)
);
ESCAPE;
}
}
}


if( !Move( to_path, from_path ) )
{
/* File moved */
/*-----------*/

/* Remove original from tree */
/*---------------------------*/

(void) RemoveFile( fe_ptr, s );


if( dest_dir_entry )
{
if( STAT_( to_path, &stat_struct ) )
{
ERROR_MSG( "Stat Failed*ABORT" );
exit( 1 );
}

file_size = stat_struct.st_size;

/* Update Total Stats for TARGET volume */
dest_dir_entry->total_bytes += file_size;
dest_dir_entry->total_files++;

if (target_stats_ptr) {
target_stats_ptr->disk_total_bytes += file_size;
target_stats_ptr->disk_total_files++;
}

/* Create File Entry manually */
/* FIX: Added +1 to allocation for null terminator */
fen_ptr = (FileEntry *) xmalloc( sizeof( FileEntry ) +
strlen( to_file ) + 1
);

(void) strcpy( fen_ptr->name, to_file );

(void) memcpy( &fen_ptr->stat_struct,
&stat_struct,
sizeof( stat_struct )
);

fen_ptr->dir_entry   = dest_dir_entry;
fen_ptr->tagged      = FALSE;
fen_ptr->matching    = Match( fen_ptr );

/* Update Matching Stats for TARGET volume */
if (fen_ptr->matching) {
dest_dir_entry->matching_bytes += file_size;
dest_dir_entry->matching_files++;
if (target_stats_ptr) {
target_stats_ptr->disk_matching_bytes += file_size;
target_stats_ptr->disk_matching_files++;
}
}

/* Link into list (Head) */
fen_ptr->next        = dest_dir_entry->file;
fen_ptr->prev        = NULL;
if( dest_dir_entry->file ) dest_dir_entry->file->prev = fen_ptr;
dest_dir_entry->file = fen_ptr;
*new_fe_ptr          = fen_ptr;

/* Force refresh if we modified the tree structure or contents */
refresh_dirwindow = TRUE;
}

(void) GetAvailBytes( &s->disk_space, s );

result = 0;
}

if (refresh_dirwindow) {
/* REMOVED: RefreshDirWindow() to prevent UI glitch */
/* Instead, rely on normal loop refresh or explicit call if needed */
}

FNC_XIT:

move( LINES - 3, 1 ); clrtoeol();
move( LINES - 2, 1 ); clrtoeol();
move( LINES - 1, 1 ); clrtoeol();

return( result );
}





int GetMoveParameter(char *from_file, char *to_file, char *to_dir)
{
if( from_file == NULL )
{
from_file = "TAGGED FILES";
(void) strcpy( to_file, "*" );
}
else
{
(void) strcpy( to_file, from_file );
}

(void) snprintf( move_prompt_header, sizeof(move_prompt_header), "MOVE: %s", from_file );

ClearHelp();

/* MvAddStr removed - handled by UI_ReadString */

if( UI_ReadString(move_prompt_header, to_file, PATH_LENGTH, HST_FILE) == CR ) {

strncpy(move_prompt_as, to_file, PATH_LENGTH);
move_prompt_as[PATH_LENGTH] = '\0';

if (IsSplitScreen && ActivePanel) {
YtreePanel *target = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
if (target && target->vol && target->vol->total_dirs > 0) {
int idx = target->disp_begin_pos + target->cursor_pos;
/* Safety bounds check */
if (idx < 0) idx = 0;
if (idx >= target->vol->total_dirs) idx = target->vol->total_dirs - 1;

GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
}
}

if( UI_ReadString("To Directory:", to_dir, PATH_LENGTH, HST_PATH) == CR ) {
if (to_dir[0] == '\0') {
strcpy(to_dir, ".");
}
return( 0 );
}
}
ClearHelp();
return( -1 );
}





static int Move(char *to_path, char *from_path)
{
/* Statistic *s = &CurrentVolume->vol_stats; */ /* Not needed here unless CopyFileContent needs it */
/* CopyFileContent does take a Statistic *s now. We need access to it. */
/* Since Move is static and called from MoveFile, we should pass it or use global CurrentVolume */
Statistic *s = &CurrentVolume->vol_stats;

if( !strcmp( to_path, from_path ) )
{
MESSAGE( "Can't move file into itself" );
return( -1 );
}

/* Try atomic rename first */
if( rename( from_path, to_path ) == 0 )
{
return 0;
}

/* Handle Cross-Device link error by copying and deleting */
if (errno == EXDEV) {
if (CopyFileContent(to_path, from_path, s) == 0) {
if (unlink(from_path) == 0) {
return 0;
} else {
MESSAGE( "Move failed during unlink*\"%s\"*%s", from_path, strerror(errno) );
return -1;
}
} else {
/* Copy failed, error message already shown by CopyFileContent */
return -1;
}
}

/* Fallback for other errors (e.g. permission denied) */
MESSAGE( "Can't move (rename) \"%s\"*to \"%s\"*%s",
from_path,
to_path,
strerror(errno)
);
return( -1 );
}






int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
int  result = -1;
char new_name[PATH_LENGTH+1];


if( BuildFilename( fe_ptr->name,
walking_package->function_data.mv.to_file,
new_name
) == 0 )

{
if( *new_name == '\0' )
{
MESSAGE( "Can't move file to*empty name" );
}
else
{
result = MoveFile( fe_ptr,
walking_package->function_data.mv.confirm,
new_name,
walking_package->function_data.mv.dest_dir_entry,
walking_package->function_data.mv.to_path,
&walking_package->new_fe_ptr,
&walking_package->function_data.mv.dir_create_mode,
&walking_package->function_data.mv.overwrite_mode
);
}
}

return( result );
}
