#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_ALLOC 30

int main()
{
    int *memory_list[NUM_ALLOC];
    int i = 0;
    for (i = 0; i < NUM_ALLOC; i++) {
        printf("Allocate #%d\n", i);
        memory_list[i] = malloc(4*1024); // 4KB
        sleep(1);
    }

    printf("Allocation completed. Begin to free memory.\n");

    for (; i>=0; i--) {
        free(memory_list[i]);
    }

    printf("Memory freed. Program exits.\n");

    return 0;
}