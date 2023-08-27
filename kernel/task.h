#pragma once

#include <stdint.h>

#define NUM_TASK_MAX    8
#define TASK_UNUSED     0
#define TASK_RUNNABLE   1
#define TASK_EXITED     2

typedef int         tid_t;

struct task {
    tid_t       tid;
    int         state;
    uint32_t    sp;
    uint8_t     stack[8192];
};

struct task *create_task(uint32_t ip, uint32_t *arg);
void yield(void);
void task_exit(int32_t code);