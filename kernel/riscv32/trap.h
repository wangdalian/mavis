#pragma once


#include <stdint.h>
#include <kernel/common.h>

#define READ_CSR(reg)                                                           \
    ({                                                                          \
        unsigned long __tmp;                                                    \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                   \
        __tmp;                                                                  \
    })

#define WRITE_CSR(reg, value)                                                   \
    ({                                                                          \
        uint32_t __tmp = (value);                                               \
        __asm__ __volatile__("csrw " #reg ", %0" :: "r"(__tmp));                \
    })

void arch_set_trap_handlers(void);