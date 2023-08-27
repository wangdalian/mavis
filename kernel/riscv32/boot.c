#include <stdint.h>

/*
boot.c defines the boot function, which is the entry point of the kernel.
__stack_top is defined in the linker script of kernel. see kernel/kernel.ld
*/

extern uint8_t __stack_top[];

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}