/***************************************************************************
 *
 * Ermittlung der freien Plattenkapazitaet
 *
 ***************************************************************************/


#include "ytree.h"

#ifdef WIN32
#include <dos.h>
#endif
#ifdef __GNU__
#include <hurd/hurd_types.h>
#endif


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
  /* Declare the structure type based on what STATFS macro expands to (statvfs or statfs) */
#if defined(__sun__) || defined(__hpux) || defined(_AIX) || defined(__sgi) || defined(__linux__) || defined(__GNU__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(SVR4) || defined(OSF1) || defined(_AIX41) /* statvfs systems */
  struct statvfs statfs_struct;
  #define FS_STRUCT_IS_STATVFS
#elif defined(__DJGPP__) || defined(SVR3) || defined(QNX)
  struct statfs statfs_struct;
#endif
#endif

  char *p;
  char *fname;
  int  result;
  LONGLONG bfree;
  LONGLONG this_disk_space;


#ifdef WIN32
  if( ( result = _getdiskfree( 0, &diskspace ) ) == 0 )
#else
#ifdef QNX
  /* QNX handling re-integrated for now, assuming original QNX support needed it */
   int fd = open(path, O_RDONLY );
   long total_blocks, free_blocks;
  if( ( result = disk_space( fd, &free_blocks, &total_blocks ) ) == 0 )
#else
  /* Use the STATFS macro defined in ytree.h which resolves to statvfs or statfs */
  if( ( result = STATFS( path, &statfs_struct, sizeof( statfs_struct ), 0 ) ) == 0 )
#endif /* QNX */
#endif /* WIN32 */
  {
    if( volume_name )
    {
      /* Name ermitteln */
      /*----------------*/

      if( mode == DISK_MODE || mode == USER_MODE )
      {
  
#ifdef linux
	switch( statfs_struct.f_type ) {
	  case 0xEF51:
	       fname = "EXT2-OLD"; break;
	  case 0xEF53:
	       fname = "EXT2"; break;
	  case 0x137D:
	       fname = "EXT"; break;
	  case 0x9660:
	       fname = "ISOFS"; break;
	  case 0x137F:
	       fname = "MINIX"; break;
	  case 0x138F:
	       fname = "MINIX2"; break;
	  case 0x2468:
	       fname = "MINIX-NEW"; break;
	  case 0x4d44:
	       fname = "DOS"; break;
	  case 0x6969:
	       fname = "NFS"; break;
	  case 0x9fa0:
	       fname = "PROC"; break;
	  case 0x012FD16D:
	       fname = "XIAFS"; break;
	  default:
	       fname = "LINUX";
	}
#else
#ifdef __GNU__
       switch( statfs_struct.f_type ) {
         case FSTYPE_UFS: fname = "UFS"; break;
         case FSTYPE_NFS: fname = "NFS"; break;
         case FSTYPE_GFS: fname = "GFS"; break;
         case FSTYPE_LFS: fname = "LFS"; break;
         case FSTYPE_SYSV: fname = "SYSV"; break;
         case FSTYPE_FTP: fname = "FTP"; break;
         case FSTYPE_TAR: fname = "TAR"; break;
         case FSTYPE_AR: fname = "AR"; break;
         case FSTYPE_CPIO: fname = "CPIO"; break;
         case FSTYPE_MSLOSS: fname = "DOS"; break;
         case FSTYPE_CPM: fname = "CPM"; break;
         case FSTYPE_HFS: fname = "HFS"; break;
         case FSTYPE_DTFS: fname = "DTFS"; break;
         case FSTYPE_GRFS: fname = "GRFS"; break;
         case FSTYPE_TERM: fname = "TERM"; break;
         case FSTYPE_DEV: fname = "DEV"; break;
         case FSTYPE_PROC: fname = "PROC"; break;
         case FSTYPE_IFSOCK: fname = "IFSOCK"; break;
         case FSTYPE_AFS: fname = "AFS"; break;
         case FSTYPE_DFS: fname = "DFS"; break;
         case FSTYPE_PROC9: fname = "PROC9"; break;
         case FSTYPE_SOCKET: fname = "SOCKET"; break;
         case FSTYPE_MISC: fname = "MISC"; break;
         case FSTYPE_EXT2FS: fname = "EXT2FS"; break;
         case FSTYPE_HTTP: fname = "HTTP"; break;
         case FSTYPE_MEMFS: fname = "MEM"; break;
         case FSTYPE_ISO9660: fname = "ISO9660"; break;
         default: fname = "HURD";
       }
#else
#if defined( __DJGPP__ )
        fname = "DJGPP";
#elif defined( WIN32 )
        fname = "WIN-NT";
#elif defined( QNX )
        fname = "QNX";
#elif defined( SVR4 ) || defined( OSF1 )
        /* statvfs on SVR4/OSF1 has f_fstr, use if available */
#ifdef FS_STRUCT_IS_STATVFS
        fname = statfs_struct.f_fstr; 
#else
        fname = "UNIX"; /* Fallback if not statvfs/f_fstr */
#endif
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
        /* These systems use statfs or statvfs but often don't populate a helpful f_fstr */
        fname = "UNIX";
#else
        fname = "UNIX"; 
#endif
#endif /* __GNU__ */
#endif /* linux */

        (void) strncpy( volume_name, 
	                fname,
		        MINIMUM( DISK_NAME_LENGTH, strlen( fname ) )
		      );
        volume_name[ MINIMUM( DISK_NAME_LENGTH, strlen( fname ))] = '\0';
      }
      else
      {  
        /* TAR/ZOO/ZIP-FILE_MODE */
        /*-----------------------*/
        
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
#ifdef QNX
    /* QNX specific disk space calculation re-integrated for now */
    *avail_bytes = (LONGLONG) free_blocks * 512;
    this_disk_space = (LONGLONG) total_blocks * 512;
    close(fd);
#else
    /* Consolidated POSIX logic for disk space */
    
    /* Use f_bavail for non-root, f_bfree for root (or if f_bavail not available/less reliable) */
    #if defined(FS_STRUCT_IS_STATVFS) || defined(_POSIX_SOURCE) /* Standardize on POSIX availability check if possible */
      bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_blocks - statfs_struct.f_bfree;
    #else
      bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_bfree;
    #endif

    if( bfree < 0L ) bfree = 0L;

    /* Use f_frsize (fragment size) if statvfs is used, otherwise f_bsize (block size) or BLKSIZ (SVR3) */
    #ifdef FS_STRUCT_IS_STATVFS
      #define FRAG_SIZE statfs_struct.f_frsize
    #elif defined(SVR3)
      #define FRAG_SIZE BLKSIZ
    #else
      #define FRAG_SIZE statfs_struct.f_bsize
    #endif

    *avail_bytes = bfree * FRAG_SIZE;
    this_disk_space   = (LONGLONG)statfs_struct.f_blocks * FRAG_SIZE;
    
#endif /* QNX */
#endif /* WIN32 */
    
    if( total_disk_space )
    {
      *total_disk_space = this_disk_space;
    }
  }
#if !(defined(WIN32) || defined(QNX))
  /* No need for explicit fd cleanup here as statvfs/statfs do not open files. */
#endif
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