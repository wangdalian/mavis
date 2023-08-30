#include <lib/common.h> 
#include <lib/env.h>

int main(void) {
    puts("Hello World");
    return 0;
}

void _start(void) {
    env_exit(main());
}