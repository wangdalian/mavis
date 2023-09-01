#include <stdio.h>
#include <stdlib.h>

extern char * __heap_base[] , __heap_end[];

int main(void) {
    printf("Hello World!\n");
    malloc(0x10);
    malloc(0x20);
    return 0;
}