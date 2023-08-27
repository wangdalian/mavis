#pragma once

#include <stdint.h>
#include "vm.h"

#define NUM_TASK_MAX    8
#define TASK_UNUSED     0
#define TASK_RUNNABLE   1
#define TASK_EXITED     2

typedef int         tid_t;

typedef struct {
    tid_t       tid;
    int         state;
    uint32_t    sp;
    uint8_t     stack[8192];

    // used if vm task
    Context     *ctx;
} Task;

Task *create_vm_task(Context *ctx);
Task *create_task(uint32_t pc);
void yield(void);
void exit(int32_t code);