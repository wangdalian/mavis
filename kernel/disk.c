#include "buffer.h"
#include <stdint.h>

extern uint8_t __hello_start[], __shell_start[];

struct buffer disk[2] = {
    // todo: fix this
    [0] = {
        .cursor = 0,
        .len    = 1932,
        .p      = __hello_start
    },

    [1] = {
        .cursor = 0,
        .len    = 3311,
        .p      = __shell_start
    }
};