/***************************************************************************
 *
 * Ermittlung der freien Plattenkapazitaet
 *
 ***************************************************************************/


#include "ytree.h"

#if ( defined( linux ) || defined( sun ) || defined( __NeXT__ )  || defined(hpux) || defined( __DJGPP__ ) || defined ( __GNU__ ) ) && !defined( SVR4 )

#include <sys/vfs.h>
#else
#ifdef WIN32
#include <dos.h>
#else
#if defined( SVR4 ) || defined( OSF1 ) || defined( sun56 ) || defined( __NetBSD__ )
#include <sys/statvfs.h>
#else
#if ( defined( ultrix ) || defined( __OpenBSD__ ) || defined ( __FreeBSD__ ) || defined (__APPLE__) )
#include <sys/param.h>
#include <sys/mount.h>
#else
#if defined( QNX )
#include <sys/types.h>
#include <sys/disk.h>
#else
/* z.B. SVR3 */
#include <sys/statfs.h>
#endif /* QNX */
#endif /* ultrix */
#endif /* SVR4 */
#endif /* WIN32 */
#endif /* sun / linux / __NeXT__  hpux */

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
#if defined( SVR4 ) || defined( OSF1 ) || defined( __NetBSD__ )
  struct statvfs statfs_struct;
#else
#ifdef ultrix
  struct fs_data statfs_struct;
#else
#ifdef QNX
  long total_blocks, free_blocks;
  int fd;
#else
  struct statfs statfs_struct;
#endif /* QNX */
#endif /* ultrix */
#endif /* SVR4 */
#endif /* WIN32 */

  char *p;
  char *fname;
  int  result;
  LONGLONG bfree;
  LONGLONG this_disk_space;


#ifdef WIN32
  if( ( result = _getdiskfree( 0, &diskspace ) ) == 0 )
#else
#ifdef __DJGPP__
  if( ( result = statfs( path, &statfs_struct ) ) == 0 )
#else
#ifdef QNX
   fd = open(path, O_RDONLY );
  if( ( result = disk_space( fd, &free_blocks, &total_blocks ) ) == 0 )
#else

  if( ( result = STATFS( path, &statfs_struct, sizeof( statfs_struct ), 0 ) ) == 0 )
#endif /* QNX */
#endif /* __DJGPP__ */
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
         case FSTYPE_UFS:
              fname = "UFS"; break;
         case FSTYPE_NFS:
              fname = "NFS"; break;
         case FSTYPE_GFS:
              fname = "GFS"; break;
         case FSTYPE_LFS:
              fname = "LFS"; break;
         case FSTYPE_SYSV:
              fname = "SYSV"; break;
         case FSTYPE_FTP:
              fname = "FTP"; break;
         case FSTYPE_TAR:
              fname = "TAR"; break;
         case FSTYPE_AR:
              fname = "AR"; break;
         case FSTYPE_CPIO:
              fname = "CPIO"; break;
         case FSTYPE_MSLOSS:
              fname = "DOS"; break;
         case FSTYPE_CPM:
              fname = "CPM"; break;
         case FSTYPE_HFS:
              fname = "HFS"; break;
         case FSTYPE_DTFS:
              fname = "DTFS"; break;
         case FSTYPE_GRFS:
              fname = "GRFS"; break;
         case FSTYPE_TERM:
              fname = "TERM"; break;
         case FSTYPE_DEV:
              fname = "DEV"; break;
         case FSTYPE_PROC:
              fname = "PROC"; break;
         case FSTYPE_IFSOCK:
              fname = "IFSOCK"; break;
         case FSTYPE_AFS:
              fname = "AFS"; break;
         case FSTYPE_DFS:
              fname = "DFS"; break;
         case FSTYPE_PROC9:
              fname = "PROC9"; break;
         case FSTYPE_SOCKET:
              fname = "SOCKET"; break;
         case FSTYPE_MISC:
              fname = "MISC"; break;
         case FSTYPE_EXT2FS:
              fname = "EXT2FS"; break;
         case FSTYPE_HTTP:
              fname = "HTTP"; break;
         case FSTYPE_MEMFS:
              fname = "MEM"; break;
         case FSTYPE_ISO9660:
              fname = "ISO9660"; break;
         default:
              fname = "HURD";
       }
#else
#if defined( sun56 ) || defined( sun ) || defined( hpux ) || defined( __NeXT__ ) || defined( ultrix ) || defined ( __FreeBSD__ ) || defined (__APPLE__)
        fname = "UNIX";
#else
#ifdef WIN32
        fname = "WIN-NT";
#else
#ifdef __DJGPP__
        fname = "DJGPP";
#else
#ifdef QNX
        fname = "QNX";
#else
#if defined( SVR4 ) || defined( OSF1 )
        fname = statfs_struct.f_fstr;
#else
#if defined(__OpenBSD__) || defined(__NetBSD__)
        fname = statfs_struct.f_fstypename;
#else
        fname = statfs_struct.f_fname;
#endif /* __OpenBSD__ || __NetBSD__ */
#endif /* SVR4 */
#endif /* __DJGPP__ */
#endif /* QNX */
#endif /* WIN32 */
#endif /* sun / hpux / __NeXT__ ultrix */
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
    *avail_bytes = (long) diskspace.bytes_per_sector *
                   (long) diskspace.sectors_per_cluster *
                   (long) diskspace.avail_clusters;
    this_disk_space = 900000000L; /* for now.. */
#else

#if (defined( SVR4 ) || defined( OSF1 )) && !defined( __DGUX__ )
    bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_bfree;
    if( bfree < 0L ) bfree = 0L;
    *avail_bytes = bfree * statfs_struct.f_frsize;
    this_disk_space   = statfs_struct.f_blocks * statfs_struct.f_frsize;
#else
#if defined( _IBMR2 ) || defined( linux ) || defined( sun ) || defined( __NeXT__ ) || defined( __GNU__ )
    bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_bfree;
    if( bfree < 0L ) bfree = 0L;
    *avail_bytes = bfree * statfs_struct.f_bsize;
    this_disk_space   = statfs_struct.f_blocks * statfs_struct.f_blocks;
#else
#ifdef SVR3
    bfree = statfs_struct.f_bfree;
    if( bfree < 0L ) bfree = 0L;
    *avail_bytes = bfree * BLKSIZ;  /* SYSV */
    this_disk_space   = statfs_struct.f_blocks * BLKSIZ;
#else 
#if defined( ultrix )
    bfree = statfs_struct.fd_req.bfree;
    if( bfree < 0L ) bfree = 0L;
#else 
#if defined( QNX )
    *avail_bytes = free_blocks * 512;
    this_disk_space = total_blocks * 512;
#else
    bfree = statfs_struct.f_bfree;
    if( bfree < 0L ) bfree = 0L;
    *avail_bytes = bfree * statfs_struct.f_bsize;
    this_disk_space   = statfs_struct.f_blocks * statfs_struct.f_bsize;
#endif /* QNX */
#endif /* ultrix */
#endif /* SVR3 */
#endif /* SVR4/!__DGUX__ */
#endif /* _IBMR2/linux/sun/__NeXT__/__GNU__ */
#endif /* WIN32 */
    
    if( total_disk_space )
    {
      *total_disk_space = this_disk_space;
    }
  }
#ifdef QNX
  close(fd);
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



