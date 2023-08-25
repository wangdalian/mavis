#pragma once

#include "buffer.h"
#include "list.h"
#include <stdint.h>
#include <assert.h>

#define DEFINE_VECTOR(elem_ty)      \
typedef struct {                    \
    int n;                          \
    elem_ty * x[];                  \
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
    Nop         = 0x1,
    I32Store    = 0x36,
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
    Drop        = 0x1a,
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
    uint32_t    offset;
    uint32_t    align;
} I32StoreInstr;

typedef struct {
    list_elem_t link;
    list_elem_t link_block; //used to link block instructions
    uint8_t     op;
    union {
        I32ConstInstr   i32Const;
        I32StoreInstr   i32Store;
        LocalGetInstr   localGet;
        LocalSetInstr   localSet;
        IfInstr         If;
        BlockInstr      block;
        BrInstr         br;
        CallInstr       call;
    };
} Instr;

typedef struct {
    list_t      expr;
    Locals_v    locals;
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

typedef struct {
    uint8_t     kind;
    union {
        // todo: add types
        TypeIdx typeIdx;
    };
} ImportDesc;

typedef struct {
    char        *modName;
    char        *name;
    ImportDesc  *importDesc;
} Import;
DEFINE_VECTOR(Import);

typedef struct {
    uint8_t     kind;
    uint32_t    min;
    uint32_t    max;
} Limits;

typedef Limits MemType;
typedef struct {
    MemType     mt;
} Mem;
DEFINE_VECTOR(Mem);

typedef uint8_t Byte;
typedef uint32_t MemIdx;

typedef struct {
    uint32_t    kind;
    MemIdx      x;
    list_t      expr;
    // todo: fix this. add readByteVec to "buffer.c"?
    uint32_t    n;
    Byte        *data;
} Data;
DEFINE_VECTOR(Data);

#define TYPE_SECTION_ID         1
#define IMPORT_SECTION_ID       2
#define FUNC_SECTION_ID         3
#define MEM_SECTION_ID          5
#define EXPORT_SECTION_ID       7
#define CODE_SECTION_ID         10
#define DATA_SECTION_ID         11

typedef struct {
    list_elem_t link;
    int id;
    union {
        FuncType_v  funcTypes;      // typesec
        Import_v    imports;        // importsec
        TypeIdx_v   typeIdxes;      // funcsec
        Mem_v       mems;           // memsec
        Export_v    exports;        // exportsec
        Code_v      codes;          // codesec
        Data_v      datas;          // datasec
    };
} Section;

typedef struct {
    uint32_t    magic;
    uint32_t    version;

    // known sections
    Section     *typesec;
    Section     *importsec;
    Section     *funcsec;
    Section     *memsec;
    Section     *exportsec;
    Section     *codesec;
    Section     *datasec;

    // todo: support custom sections?
} WasmModule;

WasmModule * newWasmModule(Buffer *buf);


