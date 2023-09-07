#pragma once

#include <stdint.h>
#include <arch_types.h>
#include "list.h"
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
    
    void                *page_top;

    list_elem_t         next;
};

struct task *task_create(uint32_t ip, uint32_t *arg);
void exec_vm_task(void *image, int size);
void task_switch(void);
void task_exit(int32_t code);