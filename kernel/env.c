#include "env.h"
#include "task.h"
#include "common.h"
#include "vm.h"

__attribute__((noreturn)) void env_exit(int32_t code){
    task_exit(code);
    PANIC("unreachable");

    __builtin_unreachable();
}

extern struct task *current_task;

// todo: return number of bytes written
void env_puts(int32_t addr) {
    // get current context
    struct context *ctx = current_task->ctx;

    // todo: fix this. this implemention is dangerous.
    puts((char *)&ctx->mem->p[addr]);
}