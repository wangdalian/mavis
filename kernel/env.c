#include "env.h"
#include "task.h"
#include "common.h"

__attribute__((noreturn)) void env_exit(int32_t code){
    task_exit(code);
    PANIC("unreachable");

    __builtin_unreachable();
}