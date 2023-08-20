#include "module.h"

ValType * parseValType(Buffer *buf) {
    ValType *valTy = malloc(sizeof(ValType));
    *valTy = readByte(buf);
    return valTy;
}

ResultType * parseResultType(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);

    ResultType *retTy = malloc(sizeof(ResultType) + sizeof(ValType *) * n);

    retTy->n = n;

    for(int i = 0; i < retTy->n; i++)
        retTy->x[i] = parseValType(buf);
    return retTy;
}

FuncType * parseFuncType(Buffer *buf) {
    FuncType * funcTy = malloc(sizeof(FuncType));

    assert(readByte(buf) == 0x60);
    
    // rt1
    funcTy->rt1 = parseResultType(buf);
    // rt2
    funcTy->rt2 = parseResultType(buf);

    return funcTy;
}

TypeIdx * parseTypeIdx(Buffer *buf) {
    TypeIdx *typeIdx = malloc(sizeof(TypeIdx));
    *typeIdx = readU32_LEB128(buf);
    return typeIdx;
}

Instr * parseInstr(Buffer *buf) {
    Instr *instr = malloc(sizeof(Instr));

    instr->op = readByte(buf);

    switch(instr->op) {
        case I32Const:
            instr->i32Const = (I32ConstInstr) {
                .n = readU32_LEB128(buf)
            };
            break;
        
        case LocalGet:
            instr->localGet = (LocalGetInstr) {
                .localIdx = readU32_LEB128(buf)
            };
            break;
        
        case LocalSet:
            instr->localSet = (LocalSetInstr) {
                .localIdx = readU32_LEB128(buf)
            };
            break;
        
        case I32Add:
        case I32Eqz:
        case I32Lt_s:
        case I32Ge_s:
        case I32Rem_s:
            break;
        
        case If: {
            instr->If.blockType = readByte(buf);
            LIST_INIT(&instr->If.thenInstrs);
            Instr *i;
            do {
                i = parseInstr(buf);
                list_push_back(&instr->If.thenInstrs, &i->link);
            } while(i->op != End && i->op != Else);

            if(i->op == Else) {
                LIST_INIT(&instr->If.elseInstrs);
                do {
                    i =  parseInstr(buf);
                    list_push_back(&instr->If.elseInstrs, &i->link);
                } while(i->op != End);
            }
            break;
        }

        case Block:
        case Loop: {
            instr->block.blockType = readByte(buf);
            LIST_INIT(&instr->block.instrs);
            Instr *i;
            do {
                i = parseInstr(buf);
                list_push_back(&instr->block.instrs, &i->link);
            } while(i->op != End);
            break;
        }
        
        case Br:
        case BrIf:
            instr->br.labelIdx = readU32_LEB128(buf);
            break;
        
        case Call:
            instr->call.funcIdx = readU32_LEB128(buf);
            break;
        
        case End:
            break;
        
    }

    return instr;
}

Locals * parseLocals(Buffer *buf) {
    Locals * locals = malloc(sizeof(Locals));

    *locals = (Locals) {
        .num    = readU32_LEB128(buf),
        .ty     = readByte(buf)
    };
    return locals;
}

Func * parseFunc(Buffer *buf) {
    uint32_t n =  readU32_LEB128(buf);

    Func *func = malloc(sizeof(Func) + sizeof(Locals *) * n);

    func->locals.n = n;

    for(int i = 0; i < func->locals.n; i++)
        func->locals.x[i] = parseLocals(buf);

    LIST_INIT(&func->expr);

    while(!eof(buf)) {
        list_push_back(
                &func->expr, 
                &parseInstr(buf)->link
            );
    }

    return func;
}

Code * parseCode(Buffer *buf) {
    Code *code = malloc(sizeof(Code));

    code->size = readU32_LEB128(buf);

    Buffer * buffer = readBuffer(buf, code->size);

    code->func = parseFunc(buffer);

    return code;   
}

ExportDesc * parseExportDesc(Buffer *buf) {
    ExportDesc *d = malloc(sizeof(ExportDesc));
    *d = (ExportDesc) {
        .kind   = readByte(buf),
        .idx    = readU32_LEB128(buf)
    };
    return d;
}

Export * parseExport(Buffer *buf) {
    Export *export = malloc(sizeof(Export));

    export->name = readName(buf);
    export->exportDesc = parseExportDesc(buf);

    return export;
}

Section * parseTypeSection(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);

    Section *sec = malloc(sizeof(Section) + sizeof(FuncType *) * n);
    
    sec->id = TYPE_SECTION_ID;
    sec->funcTypes.n = n;

    for(int i = 0; i < sec->funcTypes.n; i++) {
        sec->funcTypes.x[i] = parseFuncType(buf);
    }

    return sec;
}

Section * parseFuncSection(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);

    Section *sec = malloc(sizeof(Section) + sizeof(TypeIdx *) * n);

    sec->id = FUNC_SECTION_ID;
    sec->typeIdxes.n = n;

    for(int i = 0; i < sec->typeIdxes.n; i++) {
        sec->typeIdxes.x[i] = parseTypeIdx(buf);
    }

    return sec;
}

Section * parseCodeSection(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);

    Section *sec = malloc(sizeof(Section) + sizeof(Code *) * n);

    sec->id = CODE_SECTION_ID;
    sec->codes.n = n;

    for(int i = 0; i < sec->typeIdxes.n; i++) {
        sec->codes.x[i] = parseCode(buf);
    }

    return sec;
}

Section * parseExportSection(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);

    Section *sec = malloc(sizeof(Section) + sizeof(Export *) * n);

    sec->id = EXPORT_SECTION_ID;
    sec->exports.n = n;

    for(int i = 0; i < sec->exports.n; i++) {
        sec->exports.x[i] = parseExport(buf);
    }

    return sec;
}

Section * parseSection(Buffer *buf) {
    uint8_t id  = readByte(buf);
    uint32_t size = readU32_LEB128(buf);

    switch(id) {
        case TYPE_SECTION_ID:
            return parseTypeSection(buf);
        
        case FUNC_SECTION_ID:
            return parseFuncSection(buf);
        
        case CODE_SECTION_ID:
            return parseCodeSection(buf);
        
        case EXPORT_SECTION_ID:
            return parseExportSection(buf);
    }

    return NULL;
}

WasmModule * newWasmModule(Buffer *buf) {
    WasmModule *module = malloc(sizeof(WasmModule));
    *module = (WasmModule) {
        .magic = readU32(buf),
        .version = readU32(buf)
    };

    LIST_INIT(&module->sections);

    while(!eof(buf))
        list_push_back(
            &module->sections,
            &parseSection(buf)->link
        );
    
    return module;
}