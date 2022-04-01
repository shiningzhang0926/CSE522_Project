/*
 * resmanager.c
 * ----------------------------
 * a command-line utility that  will 
 *  1.  Allow the user to dynamically allocate memory and CPU bandwidth to 
 *      processes.
 *  2.  Monitor and report on memory and CPU bandwidth usage.
 *  3.  Pause the process and warn the user once memory utilization have exceeded
 *      their allotted amounts.
 *  4.  Allow the user to decide between allocating additional resources, 
 *      continue the process without additional resource allocation, 
 *      or terminate the process.
 * 
 * Authors: Ruiqi Wang, Shining Zhang, William Hsaio
 */
/**
 * Additional Comments: The idea of using a pipe to synchronize parent and child,
 * is adopted from userns_child_exec.c from the studio.
 * https://classes.engineering.wustl.edu/cse522/studios/namespace-programs/userns_child_exec.c
*/
#define _GNU_SOURCE

#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <sys/wait.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "mid_wrapper.h"

#define BUF_LEN 4096                        // Buffer size for the inotify events.
#define STACK_SIZE (1024 * 1024)            // stack size of the child/wrapper.
static char wrapper_stack[STACK_SIZE];      // the stack of the child/wrapper.

static char *human_readable_suffix = "kMG"; // Units of memory sizes..
int num_exceed = 0;                         // The last count of high in memory.events.
static char *cgroup_dirname;                // The path to the cgroup that ResManager created.


/*
 * Function: print_needed_info
 * ----------------------------
 * A function for debug purpose. Given the fd of a memory.events file, it will 
 * print the number of memory events in that file.
 * Note: Works only in debug mode.
 * 
 * index: index of the info we need, currently should only be 0.
 * fd: file descriptor of the file
 *
 * returns: None
 */
static void print_needed_info(int index, int fd) {
    #ifdef __DEBUG__
    debug_printf("print_needed_info: index %d, fd: %d\n", index, fd);
    char buf[1024] = {'\0'};
    char info_buf[1024] = {'\0'};
    char *begin, *end;
    if (read(fd, buf, 1024) < 0) {
        perror("Failed to read from file.");
        exit (EXIT_FAILURE);
    }
    lseek(fd, 0, SEEK_SET);
    
    if (index == 0) {
        begin = strchr(buf, '\n') + 1;
        end = strchr(begin, '\n');
        size_t len_info = end - begin + 1;
        strncpy(info_buf, begin, len_info);
        debug_printf("%s", info_buf);
    }
    lseek(fd, 0, SEEK_SET);
    #endif
}

/*
 * Function: inotify_event_handler
 * ----------------------------
 * The inotify event handler called when there is a change in the memory.stat
 * This function will check the content in the memory.events and see if there
 * is a increase in the "high" field. If so, the function will use a syscall
 * to freeze the processes in the corresponding cgroup and print usage info
 * to the user about how to continues with different options.
 * 
 * Params:
 * fd: The fd of the inotify instance.
 * filenames: list of filenames that are monitored by the inotify.
 * fds: list of fds of the files in the filenames.
 * wds: list of the corresponding inotify watch descriptors.
 * 
 * returns: None
 */
static void inotify_event_handler(int fd, char **filenames, int *fds, int *wds) {
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event)))); 
    ssize_t len, i = 0;

    /* read BUF_LEN bytes' worth of events */ 
    len = read (fd, buf, BUF_LEN);

    /* loop over every read event until none remain */ 
    while (i < len) {
        struct inotify_event *event = (struct inotify_event *) &buf[i];
        debug_printf("wd=%d mask=0x%x cookie=%d len=%d ", event->wd, event->mask, event->cookie, event->len);

        /* if there is a name, print it */ 
        if (event->len) {debug_printf("name=%s\n", event->name);}
        else debug_printf("\n", 1);
        
        for (int k = 0; k < 1; k ++) {
            if (event->wd == wds[k]) {
                //Open and read from memory.events 
                char path[128];
                FILE *fp_event;
                memset(path, '\0', 128);
                strcat(path, cgroup_dirname);
                strcat(path, "memory.events");
                fp_event = fopen(path, "r+");
                debug_printf("%s\n", path);

                //acquire the current value of high in memory.events
                char buf[1024] = {'\0'};
                char info_buf[1024] = {'\0'};
                if (read(fds[k], buf, 1024) < 0) {
                    perror("Failed to read from file.");
                    exit (EXIT_FAILURE);
                }
                lseek(fds[k], 0, SEEK_SET);
                char *begin, *end;
                begin = strchr(buf, '\n') + 5;
                end = strchr(begin, '\n');
                size_t len_info = end - begin + 1;
                strncpy(info_buf, begin, len_info);
                int high_count = atoi(info_buf);
                debug_printf("high_count[out]: %d, num_exceed: %d\n", high_count, num_exceed);
                if (high_count > num_exceed) {
                    // Freeze the cgroup if there is a change in the high field in memory.events.
                    debug_printf("high_count[in]: %d\n", high_count);
                    char freeze_path[128];
                    memset(freeze_path, '\0', 128);
                    strcat(freeze_path, cgroup_dirname);
                    strcat(freeze_path, "cgroup.freeze");

                    char pid_write[256];
                    sprintf(pid_write, "echo %d > %s", 1, freeze_path);
                    system(pid_write);
                    num_exceed = high_count;

                    debug_printf("%s\t changed\n", filenames[k]);
                    print_needed_info(k, fds[k]);
                    printf( "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
                    printf( "|Warning: Exceeded memory.high. Proceed with 1 of the 3 options:                  |\n"
                            "|1. Give a new memory.max: num[k,M,G], e.g., 20k, 30M                             |\n"
                            "|2. Proceed:   Type \"continue\"                                                    |\n"
                            "|3. Terminate: Type \"kill\"                                                        |\n"
                            "|Please note: Proceeding without adding additional memory is not recommended.     |\n");
                    printf( "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
                }
                break;
            }
        }
        i += sizeof (struct inotify_event) + event->len;
    }
}

/*
 * Function: add_to_watch
 * ----------------------------
 * It is a function that can add the files in the "filenames" in the folder 
 * "pathname" to the inotify instance with fd "fd_inotify" and put the newly 
 * generated watch descriptors in the list "wds[]".
 * 
 * Params:
 * fd_inotify: The fd of the inotify instance.
 * pathname: the path to the folder that contains the files that we are 
 *          interested in.
 * filenames: list of filenames that we want to be monitored by the inotify.
 * wds: list of the corresponding inotify watch descriptors.
 * 
 * returns: EXIT_SUCCESS if success, EXIT_FAILURE on failure.
 */
static int add_to_watch(int fd_inotify, char* pathname, char **filenames, int *wds) {
    int ret;
    char buf[128];
    for (int i = 0; i < 1; i++) {
        memset(buf, '\0', 128);
        strcat(buf, pathname);
        strcat(buf, filenames[i]);
        ret = inotify_add_watch(fd_inotify, buf, IN_MODIFY);
        if (ret == -1) {
            printf("%s\n",strerror(errno));
            perror("Failed in inotify_add_watch()");
            return EXIT_FAILURE;
        }
        wds[i] = ret;
        debug_printf("inotify_add_watch() on %s is successful with fd %d.\n", buf, ret);
    }
    return EXIT_SUCCESS;

}

/*
 * Function: usage
 * ----------------------------
 * This is a error handler called when the user uses improper arguments to start
 * the ResManager. This function will prompt the correct usage information and
 * exit the program.
 * 
 * Params:
 * program_name: name of the program, i.e., resmanager.
 * 
 * returns: exit the program with EXIT_FAILURE
 */
static void usage(char *program_name) {
    fprintf(stderr, "Usage: %s [option] arg1 [option] arg2 ./test_program \n", program_name); 
    fprintf(stderr, "Options should be:\n");
    fprintf(stderr, "    %s", "-m: The maximum amount of memory we want to allocate at the beginning\n");
    fprintf(stderr, "    %s", "-u: The unit of memory amounts we allocated above (KB, MB or GB)\n");
    exit(EXIT_FAILURE);
}


/*
 * Function: remove_cgroup_folder
 * ----------------------------
 * This function is called at the end of ResManager when trying to clear up the
 * cgroup. It will call rmdir to delete the folder of the cgroup. The path to
 * the cgroup is stored in "cgroup_dirname" as a global variable.
 * 
 * Params: None
 * 
 * returns: None
 */
static void remove_cgroup_folder() {
    struct stat s;
    while (stat(cgroup_dirname, &s) == 0) {
        usleep(1*1000*1000);
        #ifdef __DEBUG__
        debug_printf("[DEBUG: remove_cgroup_folder] Trying to remove cgroup dir: %s\n", cgroup_dirname);
        #endif
        rmdir(cgroup_dirname);
    }
}

/*
 * Function: user_interrupt_handler
 * ----------------------------
 * The SIGINT handler when the user press Ctrl-C during the execution of the 
 * ResManager. It will remove the cgroup and notify the user that Ctrl-C is 
 * detected.
 * 
 * Params: 
 * dummy: place holder.
 * 
 * returns: exit the program with EXIT_SUCCESS.
 */
static void user_interrupt_handler(int dummy) {
    remove_cgroup_folder();
    printf("\n[ResManager] Ctrl+C input from user\n");
    printf("[ResManager] ResManager exits.\n");
    exit(EXIT_SUCCESS);
}

/*
 * Function: parse_human_readable
 * ----------------------------
 * Parse memory size string to actual number in bytes with error checking.
 * 
 * Params: 
 * input: the string that contains memory size info. (e.g., 30k, 40M)
 * target: The pointer that will store the parsed size in bytes.
 * 
 * returns: target
 */
/* 
 * Reference: https://gist.github.com/maxux/786a9b8bf55fb0696f7e31b8fa3f6b9d
 */

size_t *parse_human_readable(char *input, size_t *target) {
    char *endp = input;
    char *match = NULL;
    size_t shift = 0;
    errno = 0;

    long double value = strtold(input, &endp);
    if(errno || endp == input || value < 0)
        return NULL;

    if(!(match = strchr(human_readable_suffix, *endp)))
        return NULL;

    if(*match)
        shift = (match - human_readable_suffix + 1) * 10;

    *target = value * (1LU << shift);

    debug_printf("In parse: target = %d\n", *target);

    return target;
}


int main(int argc, char** argv) {
    char buf[128];
    int nfds, poll_num;

    // Sample input: ./resmanager -M xx -U [K,M,G] ./test_program
    if (argc == 6) {
        // TODO: Correct number of args. and check is the last argument:./test_program is valid?
    }
    else {
        printf("Incorrect number of arguments are given. Exit.\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    
    int max;
    char* unit;
    int opt; 
    int flag = 0;

    // Get the inputs from -m and -u options
    while ((opt = getopt(argc, argv, "+m:u:")) != -1) {
        switch (opt) {
            case 'm': max = atoi(optarg); flag++; break;
            case 'u': 
                unit = optarg;
                // Convert the memory amounts
                if(memcmp(unit, "KB", 2) == 0) {
                    max = max * 1024;
                }
                else if(memcmp(unit, "MB", 2) == 0) {
                    max = max * 1024 * 1024;
                }
                else if(memcmp(unit, "GB", 2) == 0) {
                    max = max * 1024 * 1024 * 1024;
                }
                else{
                    printf("The unit of memory isn't correct");
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                printf("User allocated max = %d bytes from the inputs\n", max);
                flag++; 
                break;
            default: usage(argv[0]);
        }
    }

    // Check the accuracy of two options
    if(flag != 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /////////////////////////////////////////////
    // Set the path of the new cgroup
    /////////////////////////////////////////////
    char dirname[128];
    char temp[128];
    char cgroup_memory_high_path[128];
    char cgroup_memory_max_path[128];
    int check;
    int pid = getpid();
    memset(dirname, '\0', 128);
    strcat(dirname, "/sys/fs/cgroup/resmanager_cgroup_");
    sprintf(temp, "%d/", pid);
    strcat(dirname, temp);

    cgroup_dirname = dirname;
    char freeze_path[128], procs_path[128];
    memset(freeze_path, '\0', 128);
    strcat(freeze_path, cgroup_dirname);
    strcat(freeze_path, "cgroup.freeze");

    memset(procs_path, '\0', 128);
    strcat(procs_path, cgroup_dirname);
    strcat(procs_path, "cgroup.procs");

    memset(cgroup_memory_high_path, '\0', 128);
    strcat(cgroup_memory_high_path, cgroup_dirname);
    strcat(cgroup_memory_high_path, "memory.high");

    memset(cgroup_memory_max_path, '\0', 128);
    strcat(cgroup_memory_max_path, cgroup_dirname);
    strcat(cgroup_memory_max_path, "memory.max");

    /////////////////////////////////////////////
    // create a new cgroup directory
    /////////////////////////////////////////////
    signal(SIGINT, user_interrupt_handler);
    check = mkdir(cgroup_dirname,0777);
    if (!check) {
        debug_printf("The directory resmanager_%d created\n", pid);
    }  
    else {
        perror("Unable to create new cgroup directory\n");
        exit(EXIT_FAILURE);
    }

    /////////////////////////////////////////////////////
    // Set resource limits (in memory) what is high and max
    /////////////////////////////////////////////////////

    // Open the memory.high file
    int high = (max - 4096) * 0.8;;
    
    char mem_high_write[256];
    sprintf(mem_high_write, "echo %d > %s", high, cgroup_memory_high_path);
    system(mem_high_write);

    char mem_max_write[256];
    sprintf(mem_max_write, "echo %d > %s", max, cgroup_memory_max_path);
    system(mem_max_write);
    
    
    printf("Initialize resouce limit successfully\n");

    /////////////////////////////////////////////////////
    /* Create the wrapper and put it into the procs */
    /////////////////////////////////////////////////////
    pid_t mid_wrapper_pid;
    struct wrapper_args args;
    args.argv = &argv[optind];
    if (pipe(args.pipe_fd) == -1) {
        remove_cgroup_folder();
        errExit("pipe");
    }

    /////////////////////////////////////////////////////
    /* Create the child and put pid into the cgroup
     *(TODO: consider in new namespace(s)) 
     */
    /////////////////////////////////////////////////////

    mid_wrapper_pid = clone(mid_wrapper, wrapper_stack + STACK_SIZE,
                       SIGCHLD, &args);
    if (mid_wrapper_pid == -1) {
        remove_cgroup_folder();
        errExit("clone");
    }

    debug_printf("%s: PID of wrapper created by clone() is %ld\n", argv[0], (long) mid_wrapper_pid);

    // Put the pid of the child into the cgroup.procs
    
    char *cgroup_procs = "cgroup.procs";
    char *cgroup_memory_max = "memory.max";
    char *cgroup_memory_stat = "memory.stat";

    size_t cgroup_path_len = 128;
    char buf_path_cgroup_proc[cgroup_path_len];
    int suppose_write = snprintf(buf_path_cgroup_proc, cgroup_path_len, "%s%s\n", cgroup_dirname, cgroup_procs);
    if (suppose_write > cgroup_path_len) {
        // TODO: handle if the file path is too long...
    }
    else {
        debug_printf("Write PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);
        sleep(2);
        char pid_write[256];
        sprintf(pid_write, "echo %lu > %s", mid_wrapper_pid, buf_path_cgroup_proc);
        system(pid_write);
    }
    debug_printf("Finish writing PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);

    /////////////////////////////////////////////////////////////////////////////
    // Initialize the inotify monitor
    /////////////////////////////////////////////////////////////////////////////

    int fd_inotify;
    fd_inotify = inotify_init1(0);
    if (fd_inotify < 0) {
        perror("A watch could not be added for the command line argument that was given");
        exit (EXIT_FAILURE);
    }
    debug_printf("inotify_init1() is initialized successfully with file descriptor value %d.\n", fd_inotify);

    // Add the cgroup files in the folder given in the argument to the inotify.
    char *file_names[1] = {"memory.events"};
    int cgroup_file_fds[1];
    int cgroup_file_inotify_fds[1];

    for (int i = 0; i < 1; i++) {
        memset(buf, '\0', 128);
        strcat(buf, cgroup_dirname);
        strcat(buf, file_names[i]);
        debug_printf("Opening file %s\n", buf);
        int fd = open(buf, O_RDONLY); 
        if (fd < 0) {
            perror("Failed to open file");
            exit(EXIT_FAILURE); 
        }
        cgroup_file_fds[i] = fd;
        print_needed_info(1, fd);
    }

    add_to_watch(fd_inotify, cgroup_dirname, file_names, cgroup_file_inotify_fds);

    nfds = 2;
    struct pollfd fds[20];
    /* Inotify events */
    fds[0].fd = fd_inotify;                 
    fds[0].events = POLLIN;
    /* Console input */
    fds[1].fd = STDIN_FILENO;               
    fds[1].events = POLLIN;


    /////////////////////////////////////////////////////////////////////////////
    // Close the write end of the pipe so that the mid_wrapper can start the
    // the program.
    /////////////////////////////////////////////////////////////////////////////
    printf("Program starts ... \n\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
    close(args.pipe_fd[1]);


    /////////////////////////////////////////////////////////////////////////////
    // Check whether the child has returned with waitpid() and monitor user input and 
    // cgroup events with inotify
    /////////////////////////////////////////////////////////////////////////////

    int waitpid_status, waitpid_status_temp;
    pid_t w, w_temp;
    char *buff;
    int is_continue_no_freeze = 0;
    while (1) {
        w_temp = waitpid(-1, &waitpid_status_temp, WNOHANG); // -1 
        if (w_temp == -1) {
            // waitpid returns -1 which means that the child is terminated.
            debug_printf("pid %d\n", w_temp);
            break;
        }
        w = w_temp;
        waitpid_status = waitpid_status_temp;

        poll_num = poll(fds, nfds, 1); // poll with 1s timeout.
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }
        
        if (poll_num > 0) {
            if (fds[0].revents & POLLIN & is_continue_no_freeze == 0) {
                debug_printf("%s", "poll 0\n");
                inotify_event_handler(fd_inotify, file_names, cgroup_file_fds, cgroup_file_inotify_fds);
            }
            else if (fds[1].revents & POLLIN) {
                debug_printf("%s", "poll 1\n");
                int num_read = read(STDIN_FILENO, buff, 1024);
                if (num_read == 1024) {
                    // TODO: The user input is too long. We need to have some error handling to warn the user...
                }
                buff[strcspn(buff,"\n")] = '\0';
                printf("User input: %s\n",buff);

                // TODO: Check if we need to double check to make sure the cgroup is actually frozen.
                size_t new_max;

                if (strcmp(buff, "kill") == 0) { // When the user type "kill"
                    debug_printf("%s", "User choose to kill\n");
                    char freeze_write[256];
                    sprintf(freeze_write, "echo %d > %s", 1, freeze_path);
                    usleep(0.05*1000*1000); // wait slightly to make sure freeze is set.

                    FILE* file = fopen (procs_path, "r");
                    int i = 0;

                    fscanf (file, "%d", &i);    
                    while (!feof (file)) {  
                        debug_printf("Kill: %d \n", i);
                        char pid_kill_write[256];
                        sprintf(pid_kill_write, "kill %d", i);
                        system(pid_kill_write);
                        fscanf (file, "%d", &i);      
                    }
                    fclose (file);  
                    break;
                }
                else if (strcmp(buff, "continue") == 0) { // When the user type "continue"
                    debug_printf("%s", "User choose to continue\n");
                    is_continue_no_freeze = 1;

                    char pid_write[256];
                    sprintf(pid_write, "echo %d > %s", 0, freeze_path);
                    system(pid_write);
                    continue;
                }
                else if(parse_human_readable(buff, &new_max)) { // When the user type new memory size.
                    debug_printf("%s", "User choose to allocate\n");
                    // TODO: Check if inputted value is greater than initial memory.max
                    int curr_cgroup_memory_max_val = -1;
                    FILE *cgroup_memory_max_path_fptr = fopen(cgroup_memory_max_path, "r");
                    if(cgroup_memory_max_path_fptr == NULL) {
                        printf("Error! Could not open file\n");
                    }
                    fscanf(cgroup_memory_max_path_fptr, "%d", &curr_cgroup_memory_max_val);
                    printf("New Max: %d, Orignal Max: %d\n", new_max, curr_cgroup_memory_max_val);
                    if(new_max > curr_cgroup_memory_max_val) {
                        max = new_max;
                        high = (max-4096)*0.8;
                        printf("New memory constraints: Max:%d, High:%d\n", max, high);
                        sprintf(mem_high_write, "echo %d > %s", high, cgroup_memory_high_path);
                        system(mem_high_write);
                        sprintf(mem_max_write, "echo %d > %s", max, cgroup_memory_max_path);
                        system(mem_max_write);

                        char unfreeze_write[256];
                        sprintf(unfreeze_write, "echo %d > %s", 0, freeze_path);
                        system(unfreeze_write);
                        continue;
                    }
                    else{
                        printf("Inputted memory size smaller than original "
                        "memory given. Please input a larger value of allocated "
                        "memory with the structure: num [k,M,G].\n");
                    }
                }
                else { // When the user type something else.
                    printf("Please input a valid command. Proceed with 1 of "
                    "the 3 options: \n 1. Give a new memory.high: num [k,M,G]\n "
                    "2. Proceed: continue\n 3. Terminate: kill\nPlease note: "
                    "Proceeding without adding additional memory is not "
                    "recommended.\n\n");
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // Wait for the child process to finish and report the return value or 
    // errors from the child.
    /////////////////////////////////////////////////////////////////////////////
    #ifdef __DEBUG__
    debug_printf("%s", "out of while\n");
    #endif
    do {
            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
           if (WIFEXITED(waitpid_status)) {
                printf("[ResManager] ");
                printf("Program (%s) exited, status=%d\n", args.argv[0], WEXITSTATUS(waitpid_status));
            } else if (WIFSIGNALED(waitpid_status)) {
                printf("[ResManager] ");
                printf("Program (%s) killed by signal %d\n", args.argv[0], WTERMSIG(waitpid_status));
            } else if (WIFSTOPPED(waitpid_status)) {
                printf("[ResManager] ");
                printf("Program (%s) stopped by signal %d\n", args.argv[0], WSTOPSIG(waitpid_status));
            } else if (WIFCONTINUED(waitpid_status)) {
                printf("[ResManager] ");
                printf("Program (%s) continued\n", args.argv[0]);
            }
    } while (!WIFEXITED(waitpid_status) && !WIFSIGNALED(waitpid_status));

    /////////////////////////////////////////////////////////////////////////////
    // Remove the cgroup directory we created when this program endsRemove it 
    // in the end
    /////////////////////////////////////////////////////////////////////////////
    remove_cgroup_folder();
    // #ifdef __DEBUG__
    debug_printf("The directory %s removed\n", cgroup_dirname);
    // #endif
    printf("[ResManager] ResManager exits.\n");
    exit(EXIT_SUCCESS);
}
