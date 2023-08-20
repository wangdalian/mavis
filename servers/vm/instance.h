#pragma once

#include "module.h"
#include "buffer.h"
#include "list.h"
#include <string.h>
#include <stdarg.h>

typedef struct {
    uint8_t ty;
    union {
        int32_t i32;
        int64_t i64;
    } val;
} LocalValue;

typedef struct {
    FuncType    *ty;
    list_t      *codes;
    LocalValue  *locals[];
} WasmFunc;

typedef struct Instance{
    Buffer *stack;
    WasmFunc *funcs[];
} Instance;

Instance *instantiate(WasmModule *m);
int32_t call(WasmModule *m, char *name, ...);