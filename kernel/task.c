#include "task.h"

static struct task tasks[NUM_TASK_MAX];
struct task *current_task;
struct task *idle_task;

struct task *create_task(uint32_t ip, uint32_t *arg) {
    struct task *task = NULL;
    int i;
    for (i = 0; i < NUM_TASK_MAX; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            task = &tasks[i];
            break;
        }
    }

    if (!task)
        PANIC("no free process slots");

    arch_task_init(task, ip, arg);

    task->tid = i + 1;
    task->state = TASK_RUNNABLE;
    
    return task;
}

// todo: create shedule function
void yield(void) {
    struct task *next = idle_task;
    for (int i = 0; i < NUM_TASK_MAX; i++) {
        struct task *task = &tasks[(current_task->tid + i) % NUM_TASK_MAX];
        if (task->state == TASK_RUNNABLE && task->tid > 0) {
            next = task;
            break;
        }
    }

    if (next == current_task)
        return;

    struct task *prev = current_task;
    current_task = next;
    arch_task_switch(&prev->sp, &next->sp);
}

__attribute__((noreturn))
void task_exit(int32_t code) {
    printf("task exited normally: tid = %x, code = %x\n", current_task->tid, code);
    current_task->state = TASK_EXITED;
    yield();
    PANIC("unreachable");
    __builtin_unreachable();
}