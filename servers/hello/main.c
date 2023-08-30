#include <lib/common.h> 
#include <lib/env.h>

int main(void) {
    printf("Hello World!\n");
    return 0;
}

void _start(void) {
    env_exit(main());
}