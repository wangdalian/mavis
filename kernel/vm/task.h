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
    list_elem_t link;
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
    Instr       *ip;
    list_t      call_stack;
    list_t      blocks;
    WasmFunc    *funcs[];
} Context;

#define NUM_TASK_MAX    8
#define TASK_UNUSED     0
#define TASK_RUNNABLE   1
#define TASK_EXITED     2

typedef int task_t;
typedef struct {
    task_t  tid;
    int     state;
    Context *ctx;
} Task;

Task *createTask(WasmModule *m);
void yield(void);