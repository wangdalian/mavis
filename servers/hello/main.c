#include <stdint.h>

#include <lib/env.h> 

int main(void) {
    env_puts("Hello World");
    return 0;
}

void _start(void) {
    env_exit(main());
}