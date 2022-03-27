#ifndef __MID_WRAPPER__
#define __MID_WRAPPER__


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct wrapper_args {
    char **argv;        /* Command to be executed by child, with arguments */
    char *cgroup_path;
    int    pipe_fd[2];  /* Pipe used to synchronize parent and child */
};

void mid_wrapper(void *arg);



#endif