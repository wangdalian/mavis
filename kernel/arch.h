#pragma once

struct task;

void arch_putchar(char ch);
char arch_getchar(void);
void arch_idle(void);
void arch_set_trap_handlers(void);
void arch_task_switch(struct task *prev, struct task *next);