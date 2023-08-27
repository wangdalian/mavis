#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "module.h"
#include "buffer.h"
#include "list.h"

typedef struct {
    uint8_t ty;
    union {
        int32_t i32;
        int64_t i64;
    } val;
} LocalValue;

typedef struct {
    list_elem_t link;
    FuncType    *ty;
    bool        imported;
    char        *modName;
    char        *name;
    list_t      *codes;
    LocalValue  *locals[];
} WasmFunc;

typedef struct {
    Buffer          *stack;
    Buffer          *mem;
    Instr           *entry;
    list_t          call_stack;
    list_t          blocks;
    WasmFunc        *funcs[];
} Context;

Context *createContext(WasmModule *m);
void run_vm(Context *ctx);