/***************************************************************************
 *
 * Ermittlung der freien Plattenkapazitaet
 *
 ***************************************************************************/


#include "ytree.h"

/* Removed #include <dos.h> and #include <hurd/hurd_types.h> */

/* Volume-Name und freien Plattenplatz ermitteln */
/*-----------------------------------------------*/

int GetDiskParameter( char *path, 
		      char *volume_name, 
		      LONGLONG *avail_bytes,
		      LONGLONG *total_disk_space
		    )
{

#ifdef WIN32
  struct _diskfree_t diskspace;
#else
  /* Use struct statvfs as the modern POSIX standard */
  struct statvfs statfs_struct;
  #define FS_STRUCT_IS_STATVFS
#endif

  char *p;
  char *fname;
  int  result;
  LONGLONG bfree;
  LONGLONG this_disk_space;


#ifdef WIN32
  if( ( result = _getdiskfree( 0, &diskspace ) ) == 0 )
#else
  /* Use the STATFS macro defined in ytree.h which resolves to statvfs */
  if( ( result = STATFS( path, &statfs_struct, 0, 0 ) ) == 0 )
#endif /* WIN32 */
  {
    if( volume_name )
    {
      /* Name ermitteln */
      /*----------------*/

      if( mode == DISK_MODE || mode == USER_MODE )
      {
  
#if defined(__linux__)
	/* Minimal Linux/GNU FS Type Detection */
	switch( statfs_struct.f_type ) {
	  case 0xEF53: fname = "EXT2/3/4"; break;
	  case 0x9660: fname = "ISOFS"; break;
	  case 0x4d44: fname = "DOS/FAT"; break;
	  case 0x6969: fname = "NFS"; break;
	  case 0x9fa0: fname = "PROC"; break;
      case 0x5346544e: /* 'NTFS' magic from ntfs-3g */
	  case 0x53465435: fname = "NTFS"; break;
      case 0x72656d6f: fname = "WSLFS/9P"; break; /* For WSL2 mounts */
	  default: fname = "OTHER-FS";
	}
#elif defined(__APPLE__)
	/* Rely on statvfs f_fstypename */
	fname = statfs_struct.f_fstypename;
#elif defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__)
	/* BSDs often provide f_fstypename via statvfs */
	fname = statfs_struct.f_fstypename;
#else
        /* Generic POSIX fallback (includes WSL/other Unix-likes) */
        fname = "UNIX-FS";
#endif

        (void) strncpy( volume_name, 
	                fname,
		        MINIMUM( DISK_NAME_LENGTH, strlen( fname ) )
		      );
        volume_name[ MINIMUM( DISK_NAME_LENGTH, strlen( fname ))] = '\0';
      }
      else
      {  
        /* ARCHIVE_MODE */
        /*--------------*/
        
        if( ( p = strrchr( statistic.login_path, FILE_SEPARATOR_CHAR ) ) == NULL ) 
          p = statistic.login_path;
        else p++;
  
        (void) strncpy( volume_name, p, sizeof( statistic.disk_name ) );
        volume_name[sizeof( statistic.disk_name )] = '\0';
      }
    } /* volume_name */

#ifdef WIN32
    *avail_bytes = (LONGLONG) diskspace.bytes_per_sector *
                   (LONGLONG) diskspace.sectors_per_cluster *
                   (LONGLONG) diskspace.avail_clusters;
    this_disk_space = 900000000L; /* for now.. */
#else
    /* Consolidated POSIX logic for disk space */
    
    /* Use f_bavail for non-root, f_bfree for root/fallback */
    bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_bfree;

    if( bfree < 0L ) bfree = 0L;

    /* statvfs uses f_frsize (fragment size) */
    #define FRAG_SIZE statfs_struct.f_frsize

    *avail_bytes = bfree * FRAG_SIZE;
    this_disk_space   = (LONGLONG)statfs_struct.f_blocks * FRAG_SIZE;
    
#endif /* WIN32 */
    
    if( total_disk_space )
    {
      *total_disk_space = this_disk_space;
    }
  }
  return( result );
}




int GetAvailBytes(LONGLONG *avail_bytes)
{
  return( GetDiskParameter( statistic.tree->name, 
			    NULL, 
			    avail_bytes, 
			    NULL 
			  ) 
        );
}