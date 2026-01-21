/***************************************************************************
 *
 * src/fs/watcher.c
 * File System Watcher interface wrapping inotify (Linux)
 * for automatic directory refresh functionality.
 *
 ***************************************************************************/

#include "ytree.h"

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>

static int inotify_fd = -1;
static int current_wd = -1;
static char current_watch_path[PATH_LENGTH + 1] = "";

void Watcher_Init(void)
{
    /* Initialize inotify with Non-Blocking and Close-on-Exec flags */
    inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    /*
     * If initialization fails, inotify_fd will be -1.
     * We don't abort here; subsequent calls check for valid fd.
     */
}

void Watcher_SetDir(const char *path)
{
    if (inotify_fd < 0) return;
    if (!path) return;

    /* Prevent redundant watch updates for the same directory */
    if (strncmp(path, current_watch_path, PATH_LENGTH) == 0) return;

    /* Remove existing watch if valid */
    if (current_wd >= 0) {
        inotify_rm_watch(inotify_fd, current_wd);
        current_wd = -1;
    }

    /* Add new watch */
    /*
     * Monitor for:
     * - File Creation/Deletion (IN_CREATE, IN_DELETE)
     * - Moves/Renames (IN_MOVE, IN_MOVED_FROM, IN_MOVED_TO)
     * - Content changes (IN_CLOSE_WRITE)
     */
    current_wd = inotify_add_watch(inotify_fd, path,
                                   IN_CREATE | IN_DELETE | IN_MOVE |
                                   IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO);

    if (current_wd >= 0) {
        strncpy(current_watch_path, path, PATH_LENGTH);
        current_watch_path[PATH_LENGTH] = '\0';
    } else {
        /* Failed to watch (e.g. permission denied), clear path tracking */
        current_wd = -1;
        current_watch_path[0] = '\0';
    }
}

int Watcher_GetFD(void)
{
    return inotify_fd;
}

BOOL Watcher_ProcessEvents(void)
{
    /* Buffer for inotify events. alignof ensures safety for struct casting. */
    char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;
    BOOL valid_event = FALSE;
    const struct inotify_event *event;
    char *ptr;

    if (inotify_fd < 0) return FALSE;

    /* Drain the inotify socket loop */
    while ((len = read(inotify_fd, buf, sizeof(buf))) > 0) {
        /* Iterate over events in the buffer */
        for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *) ptr;

            /* Handle ignored events (watch removed/deleted) */
            if (event->mask & IN_IGNORED) {
                /* The watch descriptor was removed (either explicitly or directory deleted) */
                /* Reset internal state to allow re-watching if recreated or visited again */
                if (event->wd == current_wd) {
                    current_wd = -1;
                    current_watch_path[0] = '\0';
                }
                continue;
            }

            /* Check for relevant events that require a UI refresh */
            if (event->mask & (IN_CREATE | IN_DELETE | IN_MOVE |
                               IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO)) {
                valid_event = TRUE;
            }
        }
    }

    /* Handle read errors or empty cases */
    if (len == -1) {
        /* EAGAIN means no more data, which is expected in non-blocking mode */
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            /* Real error occurred */
            /* We could log it, but usually safe to ignore in this loop */
        }
    }

    return valid_event;
}

void Watcher_Close(void)
{
    if (inotify_fd >= 0) {
        if (current_wd >= 0) {
            inotify_rm_watch(inotify_fd, current_wd);
            current_wd = -1;
        }
        close(inotify_fd);
        inotify_fd = -1;
    }
    current_watch_path[0] = '\0';
}

#else /* !__linux__ */

/* Non-Linux Stubs */

void Watcher_Init(void)
{
}

void Watcher_SetDir(const char *path)
{
    (void)path;
}

int Watcher_GetFD(void)
{
    return -1;
}

BOOL Watcher_ProcessEvents(void)
{
    return FALSE;
}

void Watcher_Close(void)
{
}

#endif /* __linux__ */
