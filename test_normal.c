#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    for (int i = 0; i < 30; i++) {
        printf("count: %d\n", i);
        sleep(1);
    }

    printf("Exit the loop. Program exits.\n");

    return 0;
}