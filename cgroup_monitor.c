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
#define BUF_LEN 4096


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


int main(int argc, char** argv) {
    char buf[128];
    int nfds, poll_num;

    // Sample input     ./resmanager -M xx[K,M,G] ./test_program
    //              or  ./resmanager -M xx[K,M,G] ./test_program
    // Currently argc = 4

    if (argc == 4) {
        // TODO: Correct number of args. and check is the input is valid?
    }
    else {
        printf("Incorrect number of arguments are given. Exit.\n");
        return -1;
    }

    // TODO: Parsing the inputs



    // TODO: Setting up the cgroups



    // TODO: Set resource limits (in memory) what is high and max




    // 

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
        // print_needed_info(i, fd);
    }


    add_to_watch(fd_inotify, argv[1], file_names, cgroup_file_inotify_fds);
    // for (int i = 0; i < 2; i++) {
    //     printf("filename: %s, inotify_fd: %d\n", file_names[i], cgroup_file_inotify_fds[i]);
    // }

    nfds = 1;
    struct pollfd fds[2];

    // fds[0].fd = STDIN_FILENO;       /* Console input */
    // fds[0].events = POLLIN;
    fds[0].fd = fd_inotify;                 /* Inotify input */
    fds[0].events = POLLIN;


    /* TODO: Start to run another wrapper that is the parent of the test program, write its pid to cgroups.proc and run the test program.
    // RUiqi: I am thinking about a three level structure
        resmanager: which is the command line interface (CLI), it should never be killer by the cgroup and show useful information to the usr
        |-- second lv wrapper: get pid and work as the parent of the test program
                |--- test program as the child of the second level wrapper.

    */
    while (1) {
        // TODO: if the test program is not enabled...
        // enable the test program here.

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

        // TODO: if the test prog is frozen, we do something...
    }

    close(fd_inotify);
    exit(EXIT_SUCCESS);
}
