#pragma once

#include <stdint.h>
#include <arch_types.h>
#include "memory.h"
#include "buffer.h"
#include "vm.h"

#define NUM_TASK_MAX    8
#define TASK_UNUSED     0
#define TASK_RUNNABLE   1
#define TASK_EXITED     2

typedef int         tid_t;

struct task {
    tid_t               tid;
    int                 state;
    struct arch_task    arch;
    struct malloc_pool  malloc_pool;

    /*
    Used if vm_task. It is initialized by the launch_vm_task function.
    If launch fails, the "vm_task" needs to exit, so it must be initialized after the task has been executed.
    */
    Context             *ctx;
};

struct task *create_task(uint32_t ip, uint32_t *arg);
void launch_vm_task(Buffer *buf);
void yield(void);
void task_exit(int32_t code);