/* The idea of using a pipe to synchronize parent and child,
// is adopted from userns_child_exec.c from the studio.
// https://classes.engineering.wustl.edu/cse522/studios/namespace-programs/userns_child_exec.c
// Author: Ruiqi Wang
*/
#include "mid_wrapper.h"

// TODO: Include the following in the main function.
// #define STACK_SIZE (1024 * 1024)
// static char child_stack[STACK_SIZE];    /* Space for child's stack */

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

void mid_wrapper(void *arg) {
    struct wrapper_args *args = (struct wrapper_args *) arg;
    char ch;
    // TODO: Wait for the parent to put the pid of this process into the cgroup
    close(args->pipe_fd[1]);
    if (read(args->pipe_fd[0], &ch, 1) != 0) {
        errExit("Failure in mid_wrapper: read from pipe returned != 0\n");
    }

    // TODO: Make sure that the cgroup is in frozen state

    // TODO: Eecute the program
    execvp(args->argv[0], args->argv);
    errExit("execvp");
}

void main(void *arg) {
    pid_t mid_wrapper;
    struct wrapper_args args;
    args.argv = argv+0;
    if (pipe(args.pipe_fd) == -1)
        errExit("pipe");

    /* Create the child in new namespace(s) */

    mid_wrapper = clone(childFunc, child_stack + STACK_SIZE,
                      flags | SIGCHLD, &args);
    if (mid_wrapper == -1)
        errExit("clone");

    printf("%s: PID of wrapper created by clone() is %ld\n", argv[0], (long) mid_wrapper);

    // TODO: Put the pid of the child into the cgroup.procs
    char *cgroup_path = "/sys/fs/cgroup/c_1";
    char *cgroup_procs = "cgroup.procs";
    char *cgroup_memory_max = "memory.max";
    char *cgroup_memory_stat = "memory.stat";

    size_t cgroup_path_len = 128;
    char buf_path_cgroup_proc[cgroup_path_len];
    int suppose_write = snprintf(buf_path_cgroup_proc, cgroup_path_len, "%s/%s\n", cgroup_path, cgroup_procs);
    if (suppose_write > cgroup_path_len) {
        // TODO: handle if the file path is too long...
    }
    else {
        // Write the PID into the cgroup.procs
        FILE *fptr = fopen(buf_path_cgroup_proc, "w");
        fprintf(fptr, "%ld", (long) mid_wrapper);
        fclose(fptr);
    }

    // TODO: initialize the inotify

    // Close the write end of the pipe, to signal to the mid_wrapper
    close(args.pipe_fd[1]);

    // 
}

/*

    (void *arg)
    {
        struct child_args *args = (struct child_args *) arg;
        char ch;

        /* Wait until the parent has updated the UID and GID mappings. See
        the comment in main(). We wait for end of file on a pipe that will
        be closed by the parent process once it has updated the mappings. 

        close(args->pipe_fd[1]);    /* Close our descriptor for the write end
                                    of the pipe so that we see EOF when
                                    parent closes its descriptor 
        if (read(args->pipe_fd[0], &ch, 1) != 0) {
            fprintf(stderr, "Failure in child: read from pipe returned != 0\n");
            exit(EXIT_FAILURE);
        }

        /* Execute a shell command 

        execvp(args->argv[0], args->argv);
        errExit("execvp");
    }

*/

