#include "task.h"
#include "arch.h"
#include "common.h"
#include "list.h"

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
    
    // init malloc_pool
    LIST_INIT(&task->malloc_pool.pages);
    struct page *page = pmalloc(1);
    list_push_back(&task->malloc_pool.pages, &page->link);
    task->malloc_pool.next_ptr = align_up(page->base, 0x10);

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
    arch_task_switch(prev, next);
}

__attribute__((noreturn))
void task_exit(int32_t code) {
    for(;;) {
        // LIST_FOR_EACH macro cannot be used because pfree breaks the list structure.
        struct page *page = LIST_CONTAINER(
            list_tail(&current_task->malloc_pool.pages),
            struct page, 
            link
        );
        list_pop_tail(&current_task->malloc_pool.pages);

        if(!page)
            break;
        
        pfree(page);
    }

    LIST_INIT(&current_task->malloc_pool.pages);

    printf("task exited normally: tid = %x, code = %x\n", current_task->tid, code);
    current_task->state = TASK_EXITED;
    yield();
    PANIC("unreachable");
    __builtin_unreachable();
}