#include "task.h"

static Task tasks[NUM_TASK_MAX];
Task *current_task;
Task *idle_task;

__attribute__((naked)) 
static void switch_context(uint32_t *prev_sp, uint32_t *next_sp) {
     __asm__ __volatile__(
        "addi sp, sp, -13 * 4\n"
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"
        "sw sp, (a0)\n"
        "lw sp, (a1)\n"
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"
        "ret\n"
    );
}

__attribute__((naked)) 
static void task_entry(void) {
    __asm__ __volatile__(
        "lw a0, 0 * 4(sp)\n"  // a0
        "lw a1, 1 * 4(sp)\n"  // ip
        "add sp, sp, 2 * 4\n"
        "jalr a1\n"

        "1:\n"
        "j 1b\n"
    );
}

Task *create_vm_task(Context *ctx) {
    Task *task = NULL;
    int i;
    for (i = 0; i < NUM_TASK_MAX; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            task = &tasks[i];
            break;
        }
    }

    if (!task)
        PANIC("no free process slots");

    uint32_t *sp = (uint32_t *) &task->stack[sizeof(task->stack)];
    *--sp = (uint32_t)run_vm;
    *--sp = (uint32_t)ctx;

    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) task_entry;  // ra

    task->tid = i + 1;
    task->state = TASK_RUNNABLE;
    task->sp = (uint32_t)sp;
    return task;
}

Task *create_task(uint32_t pc) {
   Task *task = NULL;
    int i;
    for (i = 0; i < NUM_TASK_MAX; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            task = &tasks[i];
            break;
        }
    }

    if (!task)
        PANIC("no free process slots");

    uint32_t *sp = (uint32_t *) &task->stack[sizeof(task->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = pc;                     // ra

    task->tid = i + 1;
    task->state = TASK_RUNNABLE;
    task->sp = (uint32_t)sp;
    return task;
}

void yield(void) {
    Task *next = idle_task;
    for (int i = 0; i < NUM_TASK_MAX; i++) {
        Task *task = &tasks[(current_task->tid + i) % NUM_TASK_MAX];
        if (task->state == TASK_RUNNABLE && task->tid > 0) {
            next = task;
            break;
        }
    }

    if (next == current_task)
        return;
    
    __asm__ __volatile__(
        "csrw sscratch, %[sscratch]\n"
        :
        : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    Task *prev = current_task;
    current_task = next;
    switch_context(&prev->sp, &next->sp);
}

void exit(int32_t code) {
    printf("task exited normally: tid = %x, code = %x\n", current_task->tid, code);
    current_task->state = TASK_EXITED;
    yield();
    PANIC("unreachable");
}
