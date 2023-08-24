#pragma once

#include "module.h"
#include "buffer.h"
#include "list.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct {
    uint8_t ty;
    union {
        int32_t i32;
        int64_t i64;
    } val;
} LocalValue;

typedef struct {
    FuncType    *ty;
    bool        imported;
    char        *modName;
    char        *name;
    list_t      *codes;
    LocalValue  *locals[];
} WasmFunc;

typedef struct {
    Buffer      *stack;
    Buffer      *mem;
    list_t      blocks;
    WasmFunc    *funcs[];
} Context;

typedef struct Instance {
    Context     ctx;
} Instance;

Instance *instantiate(WasmModule *m);
int32_t call(WasmModule *m, char *name, ...);