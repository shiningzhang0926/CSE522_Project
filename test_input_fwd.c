/*
 * test_input_fwd.c
 * ----------------------------
 * A simple test program that ask the user to input something and print the 
 * exact same content on the screen.
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
    int testInteger;
    printf("[DEBUG: %s] Enter an string: \n", argv[0]);
    char *line =NULL;

    size_t len = 0;

    ssize_t lineSize = 0;

    lineSize = getline(&line, &len, stdin);
    printf("[DEBUG: %s] You entered: %s\t\t\t\twhich has %d chars (including \\n).\n", argv[0], line, lineSize);
    printf("[DEBUG: %s] Child exits.\n", argv[0], argv[0]);
    return 0;
}
