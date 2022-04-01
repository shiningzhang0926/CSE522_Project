/*
 * test_increase.c
 * ----------------------------
 * A test program that will request memory num_KB for NUM_ALLOC times.
 * 
 * Authors: Ruiqi Wang, Shining Zhang, William Hsaio
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_ALLOC 50

int main()
{
    char *memory_list[NUM_ALLOC];
    int i = 0;
    int num_KB = 40;
    for (i = 0; i < NUM_ALLOC; i++) {
        printf("Allocate #%d %dKB\n", i, num_KB);
        memory_list[i] = (char*) malloc(num_KB*1024); // 4KB
        memset(memory_list[i] , '1', num_KB*1024);
        usleep(0.2*1000*1000);
    }

    printf("[test_increase.c] Allocation completed. Begin to free memory.\n");
    i--;
    for (; i>=0; i--) {
        free(memory_list[i]);
    }

    printf("[test_increase.c] Memory freed. Test Program exits.\n");

    return 0;
}
