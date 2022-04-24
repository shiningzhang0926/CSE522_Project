/*
 * mid_wrapper.c
 * ----------------------------
 * a middle level wrapper to call the user program after the initialization
 * from the resmanager is completed.
 * 
 * Authors: Ruiqi Wang, Shining Zhang, William Hsaio
 */
/**
 * Additional Comments: The idea of using a pipe to synchronize parent and child,
 * is adopted from userns_child_exec.c from the studio.
 * https://classes.engineering.wustl.edu/cse522/studios/namespace-programs/userns_child_exec.c
*/
#include "mid_wrapper.h"


/*
 * Function: mid_wrapper
 * ----------------------------
 * a middle level wrapper to call the user program after the initialization
 * from the resmanager is completed.
 * 
 * Params:
 * arg: the args that will the user wants to run. 
 * 
 * returns: None
 */
int mid_wrapper(void *arg) {
    #ifdef __DEBUG__ 
    debug_printf("[DEBUG: mid_wrapper] wrapper pid: %ld\n", (long) getpid());
    #endif
    struct wrapper_args *args = (struct wrapper_args *) arg;
    char ch;
    close(args->pipe_fd[1]);
    close(args->stdio_pipe_fd[1]);
    dup2(args->stdio_pipe_fd[0], 0);

    // Wait for the parent to put the pid of this process into the cgroup
    if (read(args->pipe_fd[0], &ch, 1) != 0) {
        errExit("Failure in mid_wrapper: read from pipe returned != 0\n");
    }

    #ifdef __DEBUG__ 
    debug_printf("%s", "[DEBUG: mid_wrapper] execvp.\n");
    #endif
    execvp(args->argv[0], args->argv);
    errExit("execvp");
}
