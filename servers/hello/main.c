#include <lib/common.h>

int main(void) {
    printf("Hello World!\n");
    return 0;
}

void _start(void) {
    exit(main());
}