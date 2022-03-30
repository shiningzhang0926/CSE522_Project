#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    while(1) {
        printf("I am fork bomb\n");
        int *some_memory = malloc(4*1024); // 4KB
        
        sleep(1);
        fork();   
        
    }

    return 0;
}