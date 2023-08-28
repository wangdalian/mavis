#pragma once

#include <stdint.h>

struct arch_task{
    uint32_t stack_bottom;
    uint32_t sp;
    uint32_t stack_top;
};