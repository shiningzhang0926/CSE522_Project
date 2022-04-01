/*
 * test_normal.c
 * ----------------------------
 * A normal test program with no memory allocation. It will count from 0 to 4.
 * 
 * Authors: Ruiqi Wang, Shining Zhang, William Hsaio
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char** argv)
{
    printf("[DEBUG: %s] test pid: %ld\n", argv[0], (long) getpid());
    for (int i = 0; i < 5; i++) {
        printf("count: %d\n", i);
        sleep(1);
    }
    printf("[DEBUG: %s] Child exits.\n", argv[0]);
    return 0;
}