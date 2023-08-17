#pragma once

#include "buffer.h"
#include "list.h"
#include <stdint.h>
#include <assert.h>

#define DEFINE_VECTOR(elem_ty)      \
typedef struct {                    \
    int n;                          \
    elem_ty * (*x)[];               \
} elem_ty##_v;

// num type
#define I32         0x7f
#define I64         0x7e
#define F32         0x7d
#define F64         0x7c
// ref type
#define FuncRef     0x70
#define ExternRef   0x6f

typedef uint8_t ValType;
DEFINE_VECTOR(ValType);

typedef ValType_v ResultType;

typedef struct {
    ResultType  *rt1;
    ResultType  *rt2;
} FuncType;
DEFINE_VECTOR(FuncType);

typedef uint32_t TypeIdx;
DEFINE_VECTOR(TypeIdx);

typedef struct {
    uint32_t val;
    uint8_t ty;
} Locals;
DEFINE_VECTOR(Locals);

typedef enum {
    I32Const    = 0x41,
    End         = 0xb
} Op;

typedef struct {
    uint32_t    n;
}I32ConstInstr;

typedef struct {
    list_elem_t link;
    uint8_t     op;
    union {
        I32ConstInstr i32Const;
    };
} Instr;

typedef struct {
    Locals_v    locals;
    list_t      expr;
} Func;

typedef struct {
    uint32_t    size;
    Func        *func;
} Code;
DEFINE_VECTOR(Code);

#define TYPE_SECTION_ID         1   
#define FUNC_SECTION_ID         3 
#define CODE_SECTION_ID         10
typedef struct {
    list_elem_t link;
    int id;
    union {
        FuncType_v  funcTypes;      // typesec
        TypeIdx_v   typeIdxes;      // funcsec
        Code_v      codes;          // codesec
    };
} Section;

typedef struct {
    uint32_t    magic;
    uint32_t    version;
    list_t      sections;
} Module;

Module * newModule(Buffer *buf);


