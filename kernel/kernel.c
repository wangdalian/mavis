#include "kernel.h"

#include <stdint.h>

#include "common.h"
#include "buffer.h"
#include "task.h"

extern char __bss[], __bss_end[];
extern char __shell_start[];
extern int __shell_size[];

extern struct task *current_task, *idle_task;
void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
    
    arch_set_trap_handlers();

    // create idle task(kernel_main itself)
    // this is the only task that is not WASM binary
    idle_task = create_task(0, NULL);
    idle_task->tid = -1;
    current_task = idle_task;

    // exec WASM shell
    exec_vm_task(__shell_start, __shell_size[0]);
    
    PANIC("switched to idle task");
}