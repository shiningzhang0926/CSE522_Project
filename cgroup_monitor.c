#define _GNU_SOURCE

#include <sched.h>
// #include <linux/sched.h>    /* Definition of struct clone_args */

#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <unistd.h>
#define BUF_LEN 4096
#include "mid_wrapper.h"

#define STACK_SIZE (1024 * 1024)
static char wrapper_stack[STACK_SIZE];

static void print_needed_info(int index, int fd) {
    char buf[1024] = {'\0'};
    char info_buf[1024] = {'\0'};
    char *begin, *end;
    if (read(fd, buf, 1024) < 0) {
        perror("Failed to read from file.");
        exit (EXIT_FAILURE);
    }
    

    if (index == 0) {
        begin = buf;
        end = strchr(buf, '\n');
        size_t len_info = end - begin + 1;
        strncpy(info_buf, begin, len_info);
        printf("%s", info_buf);
    }
    else if (index == 1) {
        begin = strchr(buf, '\n') + 1;
        end = strchr(begin, '\n');
        size_t len_info = end - begin + 1;
        strncpy(info_buf, begin, len_info);
        printf("%s", info_buf);
    }
    lseek(fd, 0, SEEK_SET);
}

static void inotify_event_handler(int fd, char **filenames, int *fds, int *wds) {
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event)))); 
    ssize_t len, i = 0;
    

    /* read BUF_LEN bytes' worth of events */ 
    len = read (fd, buf, BUF_LEN);

    /* loop over every read event until none remain */ 
    while (i < len) {
        struct inotify_event *event = (struct inotify_event *) &buf[i];
        printf ("wd=%d mask=0x%x cookie=%d len=%d ", event->wd, event->mask, event->cookie, event->len);

        /* if there is a name, print it */ 
        if (event->len) printf ("name=%s\n", event->name);
        else printf("\n");
        
        for (int k = 0; k < 2; k ++) {
            if (event->wd == wds[k]) {
                printf("%s\t changed\n", filenames[k]);
                print_needed_info(k, fds[k]);
                break;
            }
        }
            
        i += sizeof (struct inotify_event) + event->len;

    }
}

static int add_to_watch(int fd_inotify, char* pathname, char **filenames, int *fds) {
    int ret;
    char buf[128];

    for (int i = 0; i < 2; i++) {
        memset(buf, '\0', 128);
        strcat(buf, pathname);
        strcat(buf, filenames[i]);
        ret = inotify_add_watch(fd_inotify, buf, IN_MODIFY);
        if (ret == -1) {
            printf("%s\n",strerror(errno));
            perror("Failed in inotify_add_watch()");
            return EXIT_FAILURE;
        }
        fds[i] = ret;
        printf("inotify_add_watch() on %s is successful with fd %d.\n", buf, ret);
    }

    return EXIT_SUCCESS;

}

static void usage(char *pname)
{
    fprintf(stderr, "Usage: %s [option] arg1 [option] arg2 ./test_program \n", pname); 
    fprintf(stderr, "Options should be:\n");
    fprintf(stderr, "    %s", "-m: The maximum amount of memory we want to allocate at the beginning\n");
    fprintf(stderr, "    %s", "-u: The unit of memory amounts we allocated above (KB, MB or GB)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    char buf[128];
    int nfds, poll_num;

    // Sample input: ./resmanager -M xx -U [K,M,G] ./test_program
    // Currently argc = 6
    if (argc == 6) {
        // TODO: Correct number of args. and check is the last argument:./test_program is valid?
    }
    else {
        printf("Incorrect number of arguments are given. Exit.\n");
        usage(argv[0]);
    }

    // TODO: Parsing the inputs
    int max;
    char* unit;
    int opt; 
    int flag = 0;

    // Get the inputs from -m and -o options
    while ((opt = getopt(argc, argv, "+m:u:")) != -1) {
        switch (opt) {
        case 'm': max = atoi(optarg); flag++; break;
        case 'u': 
            unit = optarg;
            // Convert the memory amounts
            if(memcmp(unit, "KB", 2) == 0){
                max = max * 1024;
            }
            else if(memcmp(unit, "MB", 2) == 0){
                max = max * 1024 * 1024;
            }
            else if(memcmp(unit, "GB", 2) == 0){
                max = max * 1024 * 1024 * 1024;
            }
            else{
                printf("The unit of memory isn't correct");
                usage(argv[0]);
            }
            printf("max = %d bytes from the inputs\n", max);
            flag++; 
            break;
        default: usage(argv[0]);
        }
    }

    // Check the accuracy of two options
    if(flag != 2){
        usage(argv[0]);
    }

    // TODO: Setting up the cgroups
    
    // Get the path of our new cgroup
    char dirname[128];
    char temp[128];
    int check;
    int pid = getpid();
    memset(dirname, '\0', 128);
    strcat(dirname, "/sys/fs/cgroup/resmanager_cgroup_");
    sprintf(temp, "%d/", pid);
    strcat(dirname, temp);
    
    // make a new cgroup directory
    check = mkdir(dirname,0777);
    if (!check){
        printf("The directory resmanager_%d created\n", pid);
    }  
    else {
        perror("Unable to create new cgroup directory\n");
        exit(EXIT_FAILURE);
    }

    // TODO: Set resource limits (in memory) what is high and max

    // Open the memory.high file
    int high = max * 0.8;
    char path[128];
    FILE *fp_high;
    memset(path, '\0', 128);
    strcat(path, dirname);
    strcat(path, "memory.high");
    fp_high = fopen(path, "r+");
    fprintf(fp_high, "%d", high);

    // Open the memory.max file
    FILE *fp_max;
    memset(path, '\0', 128);
    strcat(path, dirname);
    strcat(path, "memory.max");
    fp_max = fopen(path, "r+");
    fprintf(fp_max, "%d", max);
    
    printf("Initialize resouce limit successfully\n");

    ///////////////////////////////////////////
    /* Create the wrapper and put it into the procs */
    ///////////////////////////////////////////
    pid_t mid_wrapper_pid;
    struct wrapper_args args;
    args.argv = &argv[optind]; // TODO: chage to the correct argv. @shining
    if (pipe(args.pipe_fd) == -1)
        errExit("pipe");


    /* Create the child in new namespace(s) */

    mid_wrapper_pid = clone(mid_wrapper, wrapper_stack + STACK_SIZE,
                       SIGCHLD, &args);
    if (mid_wrapper_pid == -1)
        errExit("clone");

    printf("%s: PID of wrapper created by clone() is %ld\n", argv[0], (long) mid_wrapper_pid);

    // TODO: Put the pid of the child into the cgroup.procs
    char *cgroup_path = dirname;
    char *cgroup_procs = "cgroup.procs";
    char *cgroup_memory_max = "memory.max";
    char *cgroup_memory_stat = "memory.stat";

    size_t cgroup_path_len = 128;
    char buf_path_cgroup_proc[cgroup_path_len];
    int suppose_write = snprintf(buf_path_cgroup_proc, cgroup_path_len, "%s%s\n", cgroup_path, cgroup_procs);
    if (suppose_write > cgroup_path_len) {
        // TODO: handle if the file path is too long...
    }
    else {
        printf("Write PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);
        sleep(3);
        // Write the PID into the cgroup.procs
        // FILE *fptr = fopen(buf_path_cgroup_proc, "w");
        // fprintf(fptr, "%lu", (long) mid_wrapper_pid);  

        char pid_write[256];
        // sprintf(pid_write, "%lu", mid_wrapper_pid);
        // char* command = "echo";
        // char* argument_list[] = {"echo", pid_write, ">", buf_path_cgroup_proc, NULL};
        // int status_code = execvp(command, argument_list);
 
        // if (status_code == -1) {
        //     printf("Process did not terminate correctly\n");
        //     exit(1);
        // }

        sprintf(pid_write, "echo %lu > %s", mid_wrapper_pid, buf_path_cgroup_proc);
        

        system(pid_write);
        // fclose(fptr);
    }
    printf("Finish writing PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);
    // TODO: initialize the inotify

    // Close the write end of the pipe, to signal to the mid_wrapper
    close(args.pipe_fd[1]);

    printf("Pipe closed\n");


    // TODO: Remove the cgroup directory we created when this program ends or users type ctrl+c
    
    // // Remove it in the end
    // check = rmdir(dirname,0777);
    // if (!check){
    //     printf("The directory cgroup_%d removed\n", pid);cd
    // }  
    // else {
    //     perror("Unable to remove the directory cgroup_%d\n", pid);
    //     exit(EXIT_FAILURE);
    // }

    // // Remove it when typing ctrl+c
    // #include <signal.h>
    // void intHandler(int dummy) {
    //     rmdir(dirname,0777);
    //     signal(SIGINT, SIG_DFL);
    // }
    // signal(SIGINT, intHandler);


    int fd_inotify;
    fd_inotify = inotify_init1(0);
    if (fd_inotify < 0) {
        perror("A watch could not be added for the command line argument that was given");
        exit (EXIT_FAILURE);
    }
    printf("inotify_init1() is initialized successfully with file descriptor value %d.\n", fd_inotify);

    // Add the cgroup files in the folder given in the argument to the inotify.
    char *file_names[2] = {"cgroup.events", "memory.events"};
    int cgroup_file_fds[3];
    int cgroup_file_inotify_fds[3];

    for (int i = 0; i < 2; i++) {
        memset(buf, '\0', 128);
        strcat(buf, argv[1]);
        strcat(buf, file_names[i]);
        printf("Opening file %s\n", buf);
        int fd = open(buf, O_RDONLY); 
        if (fd < 0) {
            perror("Failed to open file");
            exit(EXIT_FAILURE); 
        }
        cgroup_file_fds[i] = fd;

        print_needed_info(i, fd);
    }


    add_to_watch(fd_inotify, argv[1], file_names, cgroup_file_inotify_fds);
    //for (int i = 0; i < 2; i++) {
    //    printf("filename: %s, inotify_fd: %d\n", file_names[i], cgroup_file_inotify_fds[i]);
    //}

    nfds = 1;
    struct pollfd fds[2];

    // // fds[0].fd = STDIN_FILENO;       /* Console input */
    // // fds[0].events = POLLIN;
    fds[0].fd = fd_inotify;                 /* Inotify input */
    fds[0].events = POLLIN;


    // /* TODO: Start to run another wrapper that is the parent of the test program, write its pid to cgroups.proc and run the test program.
    // // RUiqi: I am thinking about a three level structure
    //     resmanager: which is the command line interface (CLI), it should never be killer by the cgroup and show useful information to the usr
    //     |-- second lv wrapper: get pid and work as the parent of the test program
    //             |--- test program as the child of the second level wrapper.

    // */
    while (1) {
    //     // TODO: if the test program is not enabled...
    //     // enable the test program here.

        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0) {
            if (fds[0].revents & POLLIN) {
                inotify_event_handler(fd_inotify, file_names, cgroup_file_fds, cgroup_file_inotify_fds);
            }
        }

    //     // TODO: if the test prog is frozen, we do something...
    // }

    close(fd_inotify);
    exit(EXIT_SUCCESS);
}
