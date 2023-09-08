#pragma once

#include <stdint.h>
#include <stdbool.h>

enum message_type {
    SPAWN_TASK_MSG,
    SPAWN_TASK_REPLY_MSG,
    EXIT_TASK_MSG,
    DESTROY_TASK_MSG
};

struct message {
    int     type;
    int     src;
    
    // todo: fix this
    union {
        uint8_t data[256];
        
        struct {
            char    name[16];
            int     tid;
        } spawn_task;

        struct {
            int     tid;
        } exit_task;
    };
};

struct message_box {
    bool            has_message;
    struct message  message;
};