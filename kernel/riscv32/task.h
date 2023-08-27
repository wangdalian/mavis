#pragma once

#include <stdint.h>
#include <kernel/task.h>

void arch_task_switch(uint32_t *prev_sp, uint32_t *next_sp);
void arch_task_init(struct task *task, uint32_t ip, uint32_t *arg);