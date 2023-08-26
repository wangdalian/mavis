#pragma once

#include <stdint.h>
#include "common.h"

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

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

#define PANIC(fmt, ...)                                                         \
    do {                                                                        \
        printf("PANIC %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
        for(;;)                                                                 \
            __asm__ __volatile__("wfi");                                        \
    }while(0)

#define NUM_TASK_MAX    8
#define TASK_UNUSED     0
#define TASK_RUNNABLE   1
#define TASK_EXITED     2

#define PAGE_SIZE       4096

typedef uintptr_t   vaddr_t;
typedef uintptr_t   paddr_t;
typedef int         tid_t;

typedef struct {
    tid_t   tid;
    int     state;
    vaddr_t sp;
    uint8_t stack[8192];
} Task;

struct sbiret {
    long error;
    long value;
};
