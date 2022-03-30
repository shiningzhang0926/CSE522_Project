#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void sig_handler_child(int signum){
  
}

int main(int argc, char** argv)
{
    printf("[DEBUG: %s] test pid: %ld\n", argv[0], (long) getpid());
    for (int i = 0; i < 5; i++) {
        printf("count: %d\n", i);
        sleep(0.5);
    }
    // raise(SIGSEGV);
    // int a = 4/0;
    // i[4] = 10;
    // kill(getppid(),SIGUSR1);
    // printf("Child: sending siganl to parent\n");

    printf("[DEBUG: %s] Child exits.\n", argv[0]);

    return 0;
}