/***************************************************************************
 *
 * ytnova_runtime_launch.h
 * Shared runtime child-process launch helpers
 *
 ***************************************************************************/
#ifndef YTNOVA_RUNTIME_LAUNCH_H
#define YTNOVA_RUNTIME_LAUNCH_H

#include "ytnova_defs.h"
#include <stdio.h>

extern int RuntimeLaunchWait(pid_t child_pid, int *status_out);
extern int RuntimeLaunchRunShell(const char *command_line,
                                 const char *working_directory, int stdin_fd,
                                 int stdout_fd, int stderr_fd,
                                 BOOL create_session, int *status_out);
extern int RuntimeLaunchExecShellChild(const char *command_line,
                                       const char *working_directory,
                                       int stdin_fd, int stdout_fd,
                                       int stderr_fd, BOOL create_session);
extern int RuntimeLaunchStartShellWriter(const char *command_line,
                                         const char *working_directory,
                                         FILE **pipe_fp_out,
                                         pid_t *child_pid_out);
extern int RuntimeLaunchStartArgvWriter(char *const argv[],
                                        const char *working_directory,
                                        const char *redirect_path,
                                        BOOL redirect_append,
                                        FILE **pipe_fp_out,
                                        pid_t *child_pid_out);
extern int RuntimeLaunchCloseWriter(FILE *pipe_fp, pid_t child_pid,
                                    int *status_out);
extern int RuntimeLaunchCaptureShellOutput(const char *command_line,
                                           const char *working_directory,
                                           char **output_ptr);

#endif
