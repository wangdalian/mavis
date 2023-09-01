#include "kernel.h"

#include <stdint.h>

#include "common.h"
#include "buffer.h"
#include "task.h"

extern char __bss[], __bss_end[], __stack_top[];

extern char __hello_start[], __shell_start[];
extern int __hello_size[], __shell_size[];

void shell(void) {
     while (1) {
prompt:
        printf("> ");
        char cmdline[128];
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0) {
            exec_vm_task(__hello_start, __hello_size[0]);
        }
        else if (strcmp(cmdline, "shell") == 0) {
            exec_vm_task(__shell_start, __shell_size[0]);
        }

        else if (strcmp(cmdline, "exit") == 0)
            task_exit(0);
        else
            printf("unknown command: %s\n", cmdline);
    }
}

extern struct task *current_task, *idle_task;
void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
    
    arch_set_trap_handlers();

    // create idle task(kernel_main itself)
    idle_task = create_task(0, NULL);
    idle_task->tid = -1;
    current_task = idle_task;

    // create shell
    create_task((uint32_t)shell, NULL);
    yield();
    
    PANIC("switched to idle task");
}