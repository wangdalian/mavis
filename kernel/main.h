#pragma once

#define PANIC(fmt, ...)                                                         \
    do {                                                                        \
        printf("PANIC %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
        for(;;)                                                                 \
            __asm__ __volatile__("wfi");                                        \
    }while(0)

struct sbiret {
    long error;
    long value;
};
