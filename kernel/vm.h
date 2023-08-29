#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "module.h"
#include "buffer.h"
#include "list.h"

// support only i32 for now
typedef struct {
    uint8_t ty;
    int32_t val;
} local_variable;

typedef struct {
    globaltype ty;
    int32_t     val;
} global_variable;

typedef struct {
    list_elem_t link;
    functype    *ty;
    bool        imported;
    char        *modName;
    char        *name;
    list_t      *codes;
    local_variable  *locals[];
} wasmfunc;

typedef struct {
    buffer          *stack;
    buffer          *mem;
    instr           *entry;
    list_t          call_stack;
    list_t          blocks;
    global_variable **globals;
    wasmfunc        *funcs[];
} context;

context *create_context(module *m);
void run_vm(context *ctx);