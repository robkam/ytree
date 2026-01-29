/***************************************************************************
*
* src/cmd/view.c
* External View Command Logic
*
***************************************************************************/

#include "ytree.h"
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>

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

typedef struct
{
char *extension;
int  method;
} Extension2Method;

static Extension2Method file_extensions[] = {
{ ".F",   FREEZE_COMPRESS },
{ ".Z",   COMPRESS_COMPRESS },
{ ".z",   GZIP_COMPRESS },
{ ".gz",  GZIP_COMPRESS },
{ ".tgz", GZIP_COMPRESS },
{ ".bz2", BZIP_COMPRESS },
{ ".lz",  LZIP_COMPRESS },
{ ".zst", ZSTD_COMPRESS }
};

static int ViewFile(DirEntry * dir_entry, char *file_path);
static int ViewArchiveFile(char *file_path);
static int GetFileMethod( char *filename );
static void shell_quote(char *dest, const char *src);
static int recursive_mkdir(char *path);
static int recursive_rmdir(const char *path);


int View(DirEntry * dir_entry, char *file_path)
{
switch( mode )
{
case DISK_MODE :
case USER_MODE :     return( ViewFile(dir_entry, file_path ) );
case ARCHIVE_MODE :  return( ViewArchiveFile( file_path ) );
default:             return( -1 );
}
}


static int GetFileMethod( char *filename )
{
int i, k, l;

l = strlen( filename );

for( i=0;
i < (int)(sizeof( file_extensions ) / sizeof( file_extensions[0] ));
i++
)
{
k = strlen( file_extensions[i].extension );
if( l >= k && !strcmp( &filename[l-k], file_extensions[i].extension ) )
return( file_extensions[i].method );
}

return( 0 ); /* Using 0 for NO_COMPRESS, though constant is removed */
}


static int ViewFile(DirEntry * dir_entry, char *file_path)
{
char *command_line, *aux;
int  compress_method;
int  result = -1;
char *file_p_aux;
BOOL notice_mapped = FALSE;
char path[PATH_LENGTH+1];
int start_dir_fd;

command_line = file_p_aux = NULL;

file_p_aux = (char *) xmalloc( COMMAND_LINE_LENGTH + 1 );
StrCp(file_p_aux, file_path);

if( access( file_path, R_OK ) )
{
MESSAGE( "View not possible!*\"%s\"*%s",
file_path,
strerror(errno)
);
ESCAPE;
}

command_line = xmalloc( COMMAND_LINE_LENGTH + 1 );

if (( aux = GetExtViewer(file_path))!= NULL)
{
if (strstr(aux,"%s") != NULL)
{
(void) snprintf(command_line, COMMAND_LINE_LENGTH + 1, aux, file_p_aux);
}
else {
(void) snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", aux, file_p_aux);
}
}
else
{
compress_method = GetFileMethod( file_path );
if( compress_method == FREEZE_COMPRESS )
{
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s < %s %s | %s",
MELT,
file_p_aux,
ERR_TO_STDOUT,
PAGER
);
} else if( compress_method == COMPRESS_COMPRESS ) {
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s < %s %s | %s",
UNCOMPRESS,
file_p_aux,
ERR_TO_STDOUT,
PAGER
);
} else if( compress_method == GZIP_COMPRESS ) {
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s < %s %s | %s",
GNUUNZIP,
file_p_aux,
ERR_TO_STDOUT,
PAGER
);
} else if( compress_method == BZIP_COMPRESS ) {
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s < %s %s | %s",
BUNZIP,
file_p_aux,
ERR_TO_STDOUT,
PAGER
);
} else if( compress_method == LZIP_COMPRESS ) {
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s -dc < %s %s | %s",
LUNZIP,
file_p_aux,
ERR_TO_STDOUT,
PAGER
);
} else {
(void) snprintf( command_line, COMMAND_LINE_LENGTH + 1,
"%s %s",
PAGER,
file_p_aux
);
}
}

/* --crb3 01oct02: replicating what I did to <e>dit, eliminate
the problem with jstar-chained-thru-less writing new files to
the ytree starting cwd. new code grabbed from execute.c.
*/


if (mode == DISK_MODE)
{
/* Robustly save current working directory using a file descriptor */
start_dir_fd = open(".", O_RDONLY);
if (start_dir_fd == -1) {
MESSAGE("Error saving current directory context*%s", strerror(errno));
if(file_p_aux) free(file_p_aux);
if(command_line) free(command_line);
return -1;
}

if (chdir(GetPath(dir_entry, path)))
{
MESSAGE("Can't change directory to*\"%s\"", path);
} else {
result = SystemCall(command_line, &CurrentVolume->vol_stats);

/* Restore original directory */
if (fchdir(start_dir_fd) == -1) {
MESSAGE("Error restoring directory*%s", strerror(errno));
}
}
close(start_dir_fd);
} else {
result = SystemCall(command_line, &CurrentVolume->vol_stats);
}

if(result)
{
MESSAGE( "can't execute*%s", command_line );
}

if( notice_mapped )
{
UnmapNoticeWindow();
}

FNC_XIT:

if(file_p_aux)
free(file_p_aux);
if(command_line)
free(command_line);

return( result );
}



static int ViewArchiveFile(char *file_path)
{
char temp_filename[] = "/tmp/ytree_view_XXXXXX";
int fd;
char *archive;
char *command_line = NULL;
char *file_p_aux = NULL;
char *aux;
int result = -1;

/* 1. Create a temporary file to hold the extracted content */
fd = mkstemp(temp_filename);
if (fd == -1) {
ERROR_MSG("Could not create temporary file for view");
return -1;
}

/* 2. Extract the archive entry to the temporary file */
archive = CurrentVolume->vol_stats.login_path;
#ifdef HAVE_LIBARCHIVE
if (ExtractArchiveEntry(archive, file_path, fd, ArchiveUICallback, NULL) != 0) {
/* Error message handled by callback or here if generic failure */
/* If callback showed error, we just cleanup. */
close(fd);
unlink(temp_filename);
return -1;
}
#endif
close(fd); /* Close the file descriptor, the file now has content */

/* 3. Build the view command, same logic as ViewFile, but using temp_filename */
command_line = xmalloc(COMMAND_LINE_LENGTH + 1);
file_p_aux = xmalloc(COMMAND_LINE_LENGTH + 1);
StrCp(file_p_aux, temp_filename);

/* Use the original file_path to check for custom viewers by extension */
if ((aux = GetExtViewer(file_path)) != NULL) {
if (strstr(aux, "%s") != NULL) {
(void)snprintf(command_line, COMMAND_LINE_LENGTH + 1, aux, file_p_aux);
} else {
(void)snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", aux, file_p_aux);
}
} else {
/* No custom viewer, fall back to pager.
* The content is already decompressed by libarchive, so we just use PAGER. */
(void)snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", PAGER, file_p_aux);
}

/* 4. Execute the command */
result = SystemCall(command_line, &CurrentVolume->vol_stats);

if (result != 0) {
MESSAGE("can't execute*%s", command_line);
}

/* 5. Clean up */
free(command_line);
free(file_p_aux);
unlink(temp_filename);

return result;
}


/* Helper to quote paths for shell */
static void shell_quote(char *dest, const char *src) {
*dest++ = '\'';
while (*src) {
if (*src == '\'') {
*dest++ = '\'';
*dest++ = '\\';
*dest++ = '\'';
*dest++ = '\'';
} else {
*dest++ = *src;
}
src++;
}
*dest++ = '\'';
*dest = '\0';
}

static int recursive_mkdir(char *path) {
char *p = path;
if (*p == '/') p++;
while (*p) {
if (*p == '/') {
*p = '\0';
if (mkdir(path, 0700) != 0 && errno != EEXIST) return -1;
*p = '/';
}
p++;
}
if (mkdir(path, 0700) != 0 && errno != EEXIST) return -1;
return 0;
}

static int recursive_rmdir(const char *path) {
DIR *d = opendir(path);
struct dirent *entry;
char fullpath[PATH_LENGTH];

if (!d) return -1;

while ((entry = readdir(d))) {
if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
struct stat st;
if (stat(fullpath, &st) == 0) {
if (S_ISDIR(st.st_mode)) {
recursive_rmdir(fullpath);
} else {
unlink(fullpath);
}
}
}
closedir(d);
return rmdir(path);
}

/*
* ViewTaggedFiles
* View marked files using a batch command for external PAGER.
*/
int ViewTaggedFiles(DirEntry *dir_entry)
{
FileEntry *fe;
char *command_line;
char path[PATH_LENGTH + 1];
char quoted_path[PATH_LENGTH * 2 + 1];
int start_dir_fd = -1;
char *temp_files[100]; /* Simple limit for now */
int temp_count = 0;
int i;
int max_cmd_len = COMMAND_LINE_LENGTH;
int current_len = 0;
char temp_dir_template[PATH_LENGTH];
char *temp_dir = NULL;
Statistic *s = &CurrentVolume->vol_stats;

/* DEBUG: Entry */
fprintf(stderr, "DEBUG: ViewTaggedFiles START. Mode=%d (ARCHIVE=%d)\n", s->login_mode, ARCHIVE_MODE);

/* Use configured PAGER or default */
const char *viewer = PAGER;
if (!viewer || !*viewer) viewer = "less";

command_line = (char *)xmalloc(max_cmd_len + 1);

strcpy(command_line, viewer);

/* If search term is active, append it (assuming less syntax -p) */
if (GlobalSearchTerm[0] != '\0') {
char search_arg[300];
/* Fix quoting for search term */
snprintf(search_arg, sizeof(search_arg), " -p \"%s\"", GlobalSearchTerm);
strcat(command_line, search_arg);
}

current_len = strlen(command_line);

if (s->login_mode == ARCHIVE_MODE) {
strcpy(temp_dir_template, "/tmp/ytree_view_XXXXXX");
temp_dir = mkdtemp(temp_dir_template);
if (!temp_dir) {
ERROR_MSG("Could not create temp dir for viewing");
free(command_line);
return -1;
}
} else {
/* Initialize array elements to NULL for non-archive mode to stay safe */
for(i=0; i<100; i++) temp_files[i] = NULL;
}

for (i = 0; i < (int)CurrentVolume->file_count; i++) {
fe = CurrentVolume->file_entry_list[i].file;

if (fe->tagged && fe->matching) {
if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
GetFileNamePath(fe, path);
shell_quote(quoted_path, path);

/* Check length */
if (current_len + strlen(quoted_path) + 1 >= max_cmd_len) {
WARNING("Too many tagged files. Truncated list.");
break;
}
strcat(command_line, " ");
strcat(command_line, quoted_path);
current_len += strlen(quoted_path) + 1;

} else if (s->login_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
if (temp_count >= 100) {
WARNING("Archive view limit (100) reached.");
break;
}

/* Create unique filename in temp dir to avoid collisions */
char t_filename[PATH_LENGTH];
char t_dirname[PATH_LENGTH];

/* Extract relative path structure */
char root_path[PATH_LENGTH+1];
char relative_path[PATH_LENGTH+1];
char internal_path[PATH_LENGTH+1];
char full_path[PATH_LENGTH+1];

GetPath(CurrentVolume->vol_stats.tree, root_path);
GetFileNamePath(fe, full_path);

/* DEBUG: Path Calculation */
fprintf(stderr, "DEBUG: Archive Extraction:\n  Root: %s\n  Full: %s\n", root_path, full_path);

/* Logic to strip archive root path from full_path */
size_t root_len = strlen(root_path);
if (strncmp(full_path, root_path, root_len) == 0) {
char *ptr = full_path + root_len;
/* Check for separator or boundary */
if (*ptr == FILE_SEPARATOR_CHAR) {
ptr++;
strcpy(relative_path, ptr);
} else if (*ptr == '\0') {
/* Matched root exactly? Archive logic handles files in root. */
relative_path[0] = '\0'; /* Should not happen for file entry? */
} else {
/* Substring match, not path match (e.g. /root.zip vs /root.zip.bak) */
strcpy(relative_path, full_path);
}
} else {
/* Fallback */
strcpy(relative_path, full_path);
}

/* If relative_path is empty, use filename (root of archive) */
if (strlen(relative_path) == 0) {
strcpy(relative_path, fe->name);
}

if (strlen(relative_path) > 0) {
snprintf(t_dirname, sizeof(t_dirname), "%s/%s", temp_dir, relative_path);
/* Strip filename from t_dirname */
char *dir_only = xstrdup(t_dirname);
dirname(dir_only);
recursive_mkdir(dir_only);
free(dir_only);
snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, relative_path);
} else {
snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, fe->name);
}

strcpy(internal_path, relative_path);

fprintf(stderr, "  Rel : %s\n  Int : %s\n  Dest: %s\n", relative_path, internal_path, t_filename);

int ext_res = ExtractArchiveNode(s->login_path, internal_path, t_filename, ArchiveUICallback, NULL);
fprintf(stderr, "  Result: %d\n", ext_res);

if (ext_res == 0) {
shell_quote(quoted_path, t_filename);

if (current_len + strlen(quoted_path) + 1 >= max_cmd_len) {
WARNING("Command line too long.");
break;
}

strcat(command_line, " ");
strcat(command_line, quoted_path);
current_len += strlen(quoted_path) + 1;
temp_count++;
} else {
/* Explicit warning on failure */
WARNING("Failed to extract*\"%s\"", internal_path);
}
#else
(void)temp_count; /* Suppress unused warning */
#endif
}
}
}

fprintf(stderr, "DEBUG: Final Command: %s\n", command_line);

/* Execute Batch Command */
if (s->login_mode == DISK_MODE) {
/* Save CWD */
char cwd[PATH_LENGTH+1];
start_dir_fd = open(".", O_RDONLY);
if (chdir(GetPath(dir_entry, cwd)) == 0) {
SystemCall(command_line, &CurrentVolume->vol_stats);
if (start_dir_fd != -1) fchdir(start_dir_fd);
}
if (start_dir_fd != -1) close(start_dir_fd);
} else {
/* Archive mode executes in current dir but points to absolute temp paths */
if (temp_count > 0) {
SystemCall(command_line, &CurrentVolume->vol_stats);
} else if (s->login_mode == ARCHIVE_MODE) {
MESSAGE("No files extracted.");
}
}

/* Cleanup Temps */
if (s->login_mode == ARCHIVE_MODE && temp_dir) {
recursive_rmdir(temp_dir);
}

free(command_line);
return 0;
}
