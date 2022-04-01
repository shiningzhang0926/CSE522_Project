/*
 * mid_wrapper.h
 * ----------------------------
 * a middle level wrapper to call the user program after the initialization
 * from the resmanager is completed.
 * 
 * Authors: Ruiqi Wang, Shining Zhang, William Hsaio
 */
#ifndef __MID_WRAPPER__
#define __MID_WRAPPER__

//////////////////// Comment out for debug mode //////////////////////////////

// #define __DEBUG__

/////////////////////////////////////////////////////////////////////////////
#ifdef __DEBUG__
#define __DEBUG_PRINT__ 1
#else
#define __DEBUG_PRINT__ 0
#endif

#define debug_printf(fmt, ...) \
            if (__DEBUG_PRINT__) fprintf(stderr, ">>>> Debug print: "); \
            do { if (__DEBUG_PRINT__) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

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

#endif // __MID_WRAPPER__