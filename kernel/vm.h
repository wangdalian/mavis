#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "module.h"
#include "buffer.h"
#include "list.h"

// support only i32 for now
struct local_variable {
    uint8_t ty;
    int32_t val;
};

struct global_variable {
    globaltype  ty;
    int32_t     val;
};

struct wasm_func{
    list_elem_t             link;
    functype                *ty;
    bool                    imported;
    char                    *modName;
    char                    *name;
    list_t                  *codes;
    struct local_variable   *locals[];
};

struct context {
    struct buffer               *stack;
    struct buffer               *mem;
    instr                       *entry;
    list_t                      call_stack;
    list_t                      blocks;
    struct global_variable      **globals;
    struct wasm_func            *funcs[];
};

struct context *create_context(module *m);
void run_vm(struct context *ctx);