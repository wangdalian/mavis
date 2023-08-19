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
    uint32_t num;
    uint8_t ty;
} Locals;
DEFINE_VECTOR(Locals);

typedef enum {
    I32Const    = 0x41,
    LocalGet    = 0x20,
    LocalSet    = 0x21,
    I32Add      = 0x6a,
    I32Eqz      = 0x45,
    I32Lt_s     = 0x48,
    I32Ge_s     = 0x4e,
    I32Rem_s    = 0x6f,
    If          = 0x4,
    Else        = 0x5,
    Block       = 0x2,
    Loop        = 0x3,
    Br          = 0xc,
    BrIf        = 0xd,
    Call        = 0x10,
    End         = 0xb
} Op;

typedef struct {
    uint32_t    n;
}I32ConstInstr;

typedef struct {
    uint32_t    localIdx;
} LocalGetInstr;

typedef struct {
    uint32_t    localIdx;
} LocalSetInstr;

typedef struct {
    uint8_t     blockType;
    list_t      thenInstrs;
    list_t      elseInstrs;
} IfInstr;

typedef struct {
    uint8_t     blockType;
    list_t      instrs;
} BlockInstr;

typedef struct {
    uint32_t    labelIdx;
} BrInstr;

typedef struct {
    uint32_t    funcIdx;
} CallInstr;

typedef struct {
    list_elem_t link;
    uint8_t     op;
    union {
        I32ConstInstr   i32Const;
        LocalGetInstr   localGet;
        LocalSetInstr   localSet;
        IfInstr         If;
        BlockInstr      block;
        BrInstr         br;
        CallInstr       call;
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

typedef struct {
    uint8_t     kind;
    uint32_t    idx;   
} ExportDesc;

typedef struct {
    char *name;
    ExportDesc * exportDesc; 
} Export;
DEFINE_VECTOR(Export);

#define TYPE_SECTION_ID         1   
#define FUNC_SECTION_ID         3 
#define CODE_SECTION_ID         10
#define EXPORT_SECTION_ID       7

typedef struct {
    list_elem_t link;
    int id;
    union {
        FuncType_v  funcTypes;      // typesec
        TypeIdx_v   typeIdxes;      // funcsec
        Code_v      codes;          // codesec
        Export_v    exports;        // exportsec
    };
} Section;

typedef struct {
    uint32_t    magic;
    uint32_t    version;
    list_t      sections;
} WasmModule;

WasmModule * newWasmModule(Buffer *buf);


