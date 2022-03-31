#define _GNU_SOURCE

#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/syscall.h>    /* Definition of SYS_* constants */
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

#define BUF_LEN 4096
#define STACK_SIZE (1024 * 1024)

static char wrapper_stack[STACK_SIZE];
static char *cgroup_dirname;
static char *human_readable_suffix = "kMG";
int num_exceed = 0;

static void print_needed_info(int index, int fd) {
    printf("index %d, fd: %d\n", index, fd);
    char buf[1024] = {'\0'};
    char info_buf[1024] = {'\0'};
    char *begin, *end;
    if (read(fd, buf, 1024) < 0) {
        perror("Failed to read from file.");
        exit (EXIT_FAILURE);
    }
    lseek(fd, 0, SEEK_SET);
    // printf("buf: %s", buf);
    
    if (index == 0) {
        begin = strchr(buf, '\n') + 1;
        end = strchr(begin, '\n');
        size_t len_info = end - begin + 1;
        strncpy(info_buf, begin, len_info);
        printf("%s", info_buf);
    }
    lseek(fd, 0, SEEK_SET);
    printf("index %d\n", index);
}

// static void inotify_event_handler(int fd, char **filenames, int *fds, int *wds) {
//     char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event)))); 
//     ssize_t len, i = 0;
    

//     /* read BUF_LEN bytes' worth of events */ 
//     len = read (fd, buf, BUF_LEN);

//     /* loop over every read event until none remain */ 
//     while (i < len) {
//         struct inotify_event *event = (struct inotify_event *) &buf[i];
//         printf ("wd=%d mask=0x%x cookie=%d len=%d ", event->wd, event->mask, event->cookie, event->len);

//         /* if there is a name, print it */ 
//         if (event->len) printf ("name=%s\n", event->name);
//         else printf("\n");
        
//         for (int k = 0; k < 2; k ++) {
//             if (event->wd == wds[k]) {
//                 printf("%s\t changed\n", filenames[k]);
//                 print_needed_info(k, fds[k]);
//                 break;
//             }
//         }
            
//         i += sizeof (struct inotify_event) + event->len;

//     }
// }


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
        
        for (int k = 0; k < 1; k ++) {
            if (event->wd == wds[k]) {
                // printf("1\n");
                // char dirname[128];
                // char temp[128];
                // int check;
                // int pid = getpid();
                // memset(dirname, '\0', 128);
                // strcat(dirname, "/sys/fs/cgroup/resmanager_cgroup_");
                // sprintf(temp, "%d/", pid);
                // strcat(dirname, temp);
                // printf("2\n");
                //Open and read from memory.events 
                char path[128];
                FILE *fp_event;
                memset(path, '\0', 128);
                strcat(path, cgroup_dirname);
                strcat(path, "memory.events");
                // printf("3\n");
                fp_event = fopen(path, "r+");
                // printf("4\n");
                printf("%s\n", path);

                //acquire value of high in memory.events
                char buf[1024] = {'\0'};
                char info_buf[1024] = {'\0'};
                if (read(fds[k], buf, 1024) < 0) {
                    perror("Failed to read from file.");
                    exit (EXIT_FAILURE);
                }
                lseek(fds[k], 0, SEEK_SET);
                // printf("%s", buf);
                char *begin, *end;
                begin = strchr(buf, '\n') + 5;
                end = strchr(begin, '\n');
                size_t len_info = end - begin + 1;
                strncpy(info_buf, begin, len_info);
                // printf("info_buf: %s", info_buf);
                printf("5\n");
                //freeze processes
                int high_count = atoi(info_buf);
                printf("high_count[out]: %d, num_exceed: %d\n", high_count, num_exceed);
                if (high_count > num_exceed){
                    printf("high_count[in]: %d\n", high_count);
                    // char state[] = "FROZEN";
                    // FILE *fp_freeze;
                    if (1) { // current > high
                        char freeze_path[128];
                        memset(freeze_path, '\0', 128);
                        // cgroup_dirname
                        strcat(freeze_path, cgroup_dirname);
                        strcat(freeze_path, "cgroup.freeze");

                        char pid_write[256];
                        sprintf(pid_write, "echo %d > %s", 1, freeze_path);
                        system(pid_write);
                    }
                    num_exceed = high_count;
                }

                printf("%s\t changed\n", filenames[k]);
                print_needed_info(k, fds[k]);
                printf("Warning: Exceeded memory.high. Proceed with 1 of the 3 options: \n 1. Give a new memory.high: num [k,M,G]\n 2. Proceed: continue\n 3. Terminate: kill\nPlease note: Proceeding without adding additional memory is not recommended.\n");
                break;
            }
        }
            
        i += sizeof (struct inotify_event) + event->len;

    }
}


static int add_to_watch(int fd_inotify, char* pathname, char **filenames, int *fds) {
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
        fds[i] = ret;
        printf("inotify_add_watch() on %s is successful with fd %d.\n", buf, ret);
    }

    return EXIT_SUCCESS;

}

static void usage(char *pname) {
    fprintf(stderr, "Usage: %s [option] arg1 [option] arg2 ./test_program \n", pname); 
    fprintf(stderr, "Options should be:\n");
    fprintf(stderr, "    %s", "-m: The maximum amount of memory we want to allocate at the beginning\n");
    fprintf(stderr, "    %s", "-u: The unit of memory amounts we allocated above (KB, MB or GB)\n");
    exit(EXIT_FAILURE);
}



static void remove_cgroup_folder() {
    struct stat s;
    while (stat(cgroup_dirname, &s) == 0) {
        usleep(1*1000*1000);
        #ifdef __DEBUG__
        printf("[DEBUG: remove_cgroup_folder] Trying to remove cgroup dir: %s\n", cgroup_dirname);
        #endif
        rmdir(cgroup_dirname);
    }
}

static void intHandler(int dummy) {
    remove_cgroup_folder();
    printf("Ctrl+C input from user, exit.\n");
    exit(EXIT_SUCCESS);
}

//Check proper input for memory
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

    return target;
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
        exit(EXIT_FAILURE);
    }

    // TODO: Better Parsing the inputs
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
            printf("max = %d bytes from the inputs\n", max);
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
    
    // Set the path of our new cgroup
    char dirname[128];
    char temp[128];
    int check;
    int pid = getpid();
    memset(dirname, '\0', 128);
    strcat(dirname, "/sys/fs/cgroup/resmanager_cgroup_");
    sprintf(temp, "%d/", pid);
    strcat(dirname, temp);
    signal(SIGINT, intHandler);


    /////////////////////////////////////////////
    // make a new cgroup directory
    /////////////////////////////////////////////
    check = mkdir(dirname,0777);
    if (!check) {
        printf("The directory resmanager_%d created\n", pid);
    }  
    else {
        perror("Unable to create new cgroup directory\n");
        exit(EXIT_FAILURE);
    }

    // Set resource limits (in memory) what is high and max

    // Open the memory.high file
    int high = max - 4096;
    char cgroup_memory_high_path[128];
    char cgroup_memory_max_path[128];
    FILE *fp_high;
    memset(cgroup_memory_high_path, '\0', 128);
    strcat(cgroup_memory_high_path, dirname);
    strcat(cgroup_memory_high_path, "memory.high");
    // fp_high = fopen(path, "w");
    // fprintf(fp_high, "%d", high);
    char mem_high_write[256];
    sprintf(mem_high_write, "echo %d > %s", high, cgroup_memory_high_path);
    system(mem_high_write);

    // Open the memory.max file
    // FILE *fp_max;
    memset(cgroup_memory_max_path, '\0', 128);
    strcat(cgroup_memory_max_path, dirname);
    strcat(cgroup_memory_max_path, "memory.max");
    // fp_max = fopen(path, "w");
    // fprintf(fp_max, "%d", max);
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


    /* Create the child in new namespace(s) */

    mid_wrapper_pid = clone(mid_wrapper, wrapper_stack + STACK_SIZE,
                       SIGCHLD, &args);
    if (mid_wrapper_pid == -1) {
        remove_cgroup_folder();
        errExit("clone");
    }

    printf("%s: PID of wrapper created by clone() is %ld\n", argv[0], (long) mid_wrapper_pid);

    // Put the pid of the child into the cgroup.procs
    cgroup_dirname = dirname;
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
        printf("Write PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);
        sleep(2);
        char pid_write[256];
        sprintf(pid_write, "echo %lu > %s", mid_wrapper_pid, buf_path_cgroup_proc);
        system(pid_write);
    }
    printf("Finish writing PID %lu of wrapper into %s\n", (long) mid_wrapper_pid, buf_path_cgroup_proc);

    /////////////////////////////////////////////////////////////////////////////
    // TODO: initialize the inotify
    /////////////////////////////////////////////////////////////////////////////

    int fd_inotify;
    fd_inotify = inotify_init1(0);
    if (fd_inotify < 0) {
        perror("A watch could not be added for the command line argument that was given");
        exit (EXIT_FAILURE);
    }
    printf("inotify_init1() is initialized successfully with file descriptor value %d.\n", fd_inotify);

    // Add the cgroup files in the folder given in the argument to the inotify.
    char *file_names[1] = {"memory.events"};
    int cgroup_file_fds[1];
    int cgroup_file_inotify_fds[1];

    for (int i = 0; i < 1; i++) {
        memset(buf, '\0', 128);
        strcat(buf, dirname);
        strcat(buf, file_names[i]);
        printf("Opening file %s\n", buf);
        int fd = open(buf, O_RDONLY); 
        if (fd < 0) {
            perror("Failed to open file");
            exit(EXIT_FAILURE); 
        }
        cgroup_file_fds[i] = fd;

        print_needed_info(1, fd);
    }


    add_to_watch(fd_inotify, cgroup_dirname, file_names, cgroup_file_inotify_fds);
    //for (int i = 0; i < 2; i++) {
    //    printf("filename: %s, inotify_fd: %d\n", file_names[i], cgroup_file_inotify_fds[i]);
    //}

    nfds = 2;
    struct pollfd fds[20];

    fds[0].fd = fd_inotify;                 /* Inotify input */
    fds[0].events = POLLIN;

    fds[1].fd = STDIN_FILENO;               /* Console input */
    fds[1].events = POLLIN;


    /////////////////////////////////////////////////////////////////////////////
    // Close the write end of the pipe so that the mid_wrapper can start the
    // the program.
    /////////////////////////////////////////////////////////////////////////////
    printf("Program starts ... \n");
    close(args.pipe_fd[1]);


    int waitpid_status, waitpid_status_temp;
    pid_t w, w_temp;
    char *buff;
    int isContinue = 0;
    while (1) {
        w_temp = waitpid(-1, &waitpid_status_temp, WNOHANG);
        if (w_temp == -1) {
            printf("pid %d\n", w_temp);
            break;
        }
        w = w_temp;
        waitpid_status = waitpid_status_temp;

        poll_num = poll(fds, nfds, 1);   
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }
        
        if (poll_num > 0) {
            // printf("poll_num > 0\n");
            if (fds[0].revents & POLLIN & isContinue == 0) {
                printf("poll 0\n");
                inotify_event_handler(fd_inotify, file_names, cgroup_file_fds, cgroup_file_inotify_fds);
            }
            if (fds[1].revents & POLLIN){
                printf("poll 1\n");
                int num_read = read(STDIN_FILENO, buff, 1024);
                if (num_read == 1024) {
                // something to warn the user...
                }
                buff[strcspn(buff,"\n")] = '\0';
                printf("User input: %s\n",buff);
                
                // cont --> ;
                // allo --> handler
                //kill --> kill

                

                // TODO: make sure actually frozen.
                size_t result;

                if (!strcmp(buff, "kill")){ //strcmp
                    char pid_write[256];
                    char kill_path[128];
                    memset(kill_path, '\0', 128);
                    // cgroup_dirname
                    strcat(kill_path, cgroup_dirname);
                    strcat(kill_path, "cgroup.freeze");
                    sprintf(pid_write, "echo %d > %s", 1, kill_path);
                    sleep(1);

                    memset(kill_path, '\0', 128);
                    strcat(kill_path, cgroup_dirname);
                    strcat(kill_path, "cgroup.procs");

                    FILE* file = fopen (kill_path, "r");
                    int i = 0;

                    fscanf (file, "%d", &i);    
                    while (!feof (file))
                    {  
                        printf ("Kill: %d \n", i);
                        char pid_write[256];
                        sprintf(pid_write, "kill %d", i);
                        system(pid_write);
                        fscanf (file, "%d", &i);      
                    }
                    fclose (file);  
                    system(pid_write);
                    break;
                }
                else if (!strcmp(buff, "continue")){
                    //printf("Continuing process");
                    //unfreeze cgroup and proceed as is
                    isContinue = 1;
                    char freeze_path[128];
                    memset(freeze_path, '\0', 128);
                    // cgroup_dirname
                    strcat(freeze_path, cgroup_dirname);
                    strcat(freeze_path, "cgroup.freeze");

                    char pid_write[256];
                    sprintf(pid_write, "echo %d > %s", 0, freeze_path);
                    system(pid_write);
                    continue;
                }
                
                else if(parse_human_readable(buff, &result)){
                    //Check if inputted value is greater than initial memory.max
                    char info_buf[1024];
                    FILE *mem_max = fopen(cgroup_memory_max_path, "r");
                    if(mem_max == NULL){
                        printf("Error! Could not open file\n");
                    }
                    fscanf(mem_max, "%d", info_buf);
                    int new_max = atoi(info_buf);
                    if(new_max > result){
                        printf("Entered");
                        max = result;
                        high = max - 4096;
                        
                        printf("Max:%d High:%d Result:%d", max, high, result);
                        sprintf(mem_high_write, "echo %d > %s", high, cgroup_memory_high_path);
                        system(mem_high_write);
                        sprintf(mem_max_write, "echo %d > %s", max, cgroup_memory_max_path);
                        system(mem_max_write);

                        char freeze_path[128];
                        memset(freeze_path, '\0', 128);
                        // cgroup_dirname
                        strcat(freeze_path, cgroup_dirname);
                        strcat(freeze_path, "cgroup.freeze");

                        char pid_write[256];
                        sprintf(pid_write, "echo %d > %s", 0, freeze_path);
                        system(pid_write);
                        continue;
                    }
                    else{
                        printf("Inputted memory size smaller than original memory given. Please input a larger value of allocated memory with the structure: num [k,M,G].\n");
                    }
                }
                else{
                    printf("Please input a valid command. Proceed with 1 of the 3 options: \n 1. Give a new memory.high: num [k,M,G]\n 2. Proceed: continue\n 3. Terminate: kill\nPlease note: Proceeding without adding additional memory is not recommended.\n\n");
                }
            }
        }
         // TODO: if the test prog is frozen, we do something...
    }

    /////////////////////////////////////////////////////////////////////////////
    // Wait for the child process to finish and report the return value or 
    // errors from the child.
    /////////////////////////////////////////////////////////////////////////////
    printf("out of while\n");
    sleep(0.1);
    do {
            // pid_t w = waitpid(mid_wrapper_pid, &waitpid_status, WUNTRACED | WCONTINUED);
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
    printf("The directory %s removed\n", cgroup_dirname);
    exit(EXIT_SUCCESS);

}
