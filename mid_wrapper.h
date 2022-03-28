#ifndef __MID_WRAPPER__
#define __MID_WRAPPER__

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct wrapper_args {
    char **argv;        /* Command to be executed by child, with arguments */
    char *cgroup_path;
    int    pipe_fd[2];  /* Pipe used to synchronize parent and child */
};

int mid_wrapper(void *arg);



#endif