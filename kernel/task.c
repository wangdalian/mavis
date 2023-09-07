#include "task.h"
#include "arch.h"
#include "buffer.h"
#include "common.h"
#include "memory.h"
#include "message.h"
#include "module.h"
#include "vm.h"
#include "list.h"
#include "ipc.h"
#include <stdint.h>

struct task tasks[NUM_TASK_MAX];
static list_t  runqueue = (list_t) {
    .prev   = &runqueue,
    .next   = &runqueue
};

void task_resume(struct task *task) {
    task->state = TASK_RUNNABLE;
    list_push_back(&runqueue, &task->next);
}

void task_block(struct task *task) {
    task->state = TASK_BLOCKED;
}

struct task *current_task;

struct task *schedule(void) {
    struct task *next = LIST_POP_TAIL(&runqueue, struct task, next);

    if(next)
        return next;

    if(current_task->state == TASK_RUNNABLE)
        return current_task;

    __builtin_unreachable();
}

struct task *task_create(uint32_t ip, uint32_t *arg) {
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
    
    // init message_box
    task->message_box.has_message = false;

    // init malloc_pool
    // In the current implementation, the top of the page is used as the header.
    LIST_INIT(&task->malloc_pool.pages);
    struct page *page = pmalloc(1);
    task->page_top = page;
    list_push_back(&task->malloc_pool.pages, &page->link);
    task->malloc_pool.next_ptr = align_up(page->base, 0x10);
   
    // init stack
    arch_task_init(task, ip, arg);

    task->tid = i + 1;

    task_resume(task);

    return task;
}

// todo: impl file system and fix this
// this is entry point of vm_task
void launch_vm_task(struct buffer *buf) {
    module *m = new_module(buf);
    struct context *ctx = create_context(m);
    run_vm(ctx);
}

void vm_create(void *image, int size) {
    struct buffer *buf = newbuffer(image, size);
    task_create((uint32_t)launch_vm_task, (uint32_t *)buf);
}

void task_switch(void) {
    struct task *prev = current_task;
    struct task *next = schedule();

    if(prev == next)
        return;
    
    //todo: push back to runqueue if not idle_task?

    current_task = next;
    arch_task_switch(prev, next);
}

__attribute__((noreturn))
void task_exit(int32_t code) {
    // free memories
    pfree(current_task->page_top);

    // init state
    current_task->state = TASK_UNUSED;

    // send message to vm task
    struct message msg = {.type = EXIT_TASK_MSG, .exit_task = {.tid = current_task->tid}};
    ipc_send(3, &msg);

    // never reach here
    PANIC("unreachable");
    __builtin_unreachable();
}