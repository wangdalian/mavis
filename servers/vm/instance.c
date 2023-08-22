#include "instance.h"

LocalValue *createLocalValue(ValType ty) {
    LocalValue *val = malloc(sizeof(LocalValue));
    *val = (LocalValue) {
        .ty         = ty,
        .val.i32    = 0
    };

    return val;
}

WasmFunc * createImportedFunc(Instance *instance, int idx) {
    Import *info = instance->importsec->imports.x[idx];
    FuncType *ty = instance->typesec->funcTypes.x[info->importDesc->typeIdx];

    int num_params = ty->rt1->n;
    WasmFunc *wasmf = malloc(sizeof(WasmFunc) * num_params);

    *wasmf = (WasmFunc) {
        .ty         = ty,
        .imported   = true,
        .modName    = info->modName,
        .name       = info->name
    };

    // create local variables
    for(int i = 0; i < num_params; i++) {
        wasmf->locals[i] = createLocalValue(*(ty->rt1->x[i]));
    }

    return wasmf;
}

WasmFunc *createDefinedFunc(Instance *instance, int idx) {
    Func *f = instance->codesec->codes.x[idx]->func;
    FuncType *ty = instance->typesec->funcTypes.x[idx];

    // count local variables
    int num_params = ty->rt1->n;

    int num_local = num_params;
    for(int i = 0; i < f->locals.n; i++) {
        num_local += f->locals.x[i]->num;
    }

    WasmFunc *wasmf = malloc(sizeof(WasmFunc) + sizeof(LocalValue *) * num_local);
    wasmf->ty = ty;
    wasmf->codes = &f->expr;

    // create local variables
    for(int i = 0; i < num_params; i++) {
        wasmf->locals[i] = createLocalValue(*(ty->rt1->x[i]));
    }

    int localIdx = num_params;
    Locals *locals;
    for(int i = 0; i < f->locals.n; i++) {
        locals = f->locals.x[i];
        for(uint32_t j = 0; j < locals->num; j++) {
            wasmf->locals[localIdx++] = createLocalValue(locals->ty);
        }
    }

    return wasmf;
}

Instr * invokeI(Context *ctx, WasmFunc *func, Instr *instr);

Instr * branchIn(Context *ctx, WasmFunc *func, int idx) {
    // todo: check block`s existence
    // todo: support if block?
    list_elem_t *block = list_tail(&ctx->blocks);
    while(idx) {
        block = block->prev;
        idx--;
    }

    Instr *instr = LIST_CONTAINER(block, Instr, link_block);

    switch(instr->op) {
        case Loop:
            return LIST_CONTAINER(
                list_head(&instr->block.instrs),
                Instr,
                link
            );
        case Block: {
            // end instruction expected
            invokeI(
                ctx, func, 
                LIST_CONTAINER(
                    list_tail(&instr->block.instrs), Instr, link
                )
            );

            return LIST_CONTAINER(
                instr->link.next,
                Instr,
                link
            );
        }
    }

    // unexpected block
    return NULL;
}

int32_t invokeF(Context *instance, WasmFunc *f);

Instr * invokeI(Context *ctx, WasmFunc *func, Instr *instr) {
    switch(instr->op) {
        case I32Const:
            writeI32(ctx->stack, instr->i32Const.n);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        
        case I32Add: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs + rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Rem_s: {
            // todo: assert rhs != 0
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs % rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Lt_s: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs < rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Ge_s: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs >= rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Eqz: {
            int32_t c = readI32(ctx->stack);
            writeI32(ctx->stack, c == 0);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case LocalGet: {
            writeI32(
                ctx->stack, 
                func->locals[instr->localGet.localIdx]->val.i32
            );
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case LocalSet: {
            int32_t val = readI32(ctx->stack);
            func->locals[instr->localSet.localIdx]->val.i32 = val;
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Call: {
            int32_t ret = invokeF(ctx, ctx->funcs[instr->call.funcIdx]);
            if(ret)
                writeI32(ctx->stack, ret);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case If: {
            int32_t cond = readI32(ctx->stack);
            if(cond) {
                Instr *i = LIST_CONTAINER(
                    list_head(&instr->If.thenInstrs),
                    Instr,
                    link
                );

                while(i) {
                    i = invokeI(ctx, func, i);
                }
            } else {
                Instr *i = LIST_CONTAINER(
                    list_head(&instr->If.elseInstrs),
                    Instr,
                    link
                );

                while(i) {
                    i = invokeI(ctx, func, i);
                }
            }
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Block:
        case Loop:
            list_push_back(&ctx->blocks, &instr->link_block);
            return LIST_CONTAINER(
                list_head(&instr->block.instrs),
                Instr,
                link
            );
        
        case Br:
            return branchIn(ctx, func, instr->br.labelIdx);
        
        case BrIf: {
            int32_t cond = readI32(ctx->stack);
            if(cond)
                return branchIn(ctx, func, instr->br.labelIdx);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Store: {
            /*
            memarg.offset of the t.store instruction is an exponent to the power of two: 2 for i32.store and 3 for i64.store.
            In the specification, the offset is added to the value popped from the stack at runtime, 
            However, in the current implementation, the offset is not used because the storeI32 function in buffer.c does the same thing.
            */
            int32_t val = readI32(ctx->stack);
            int32_t offs = readI32(ctx->stack);
            storeI32(ctx->mem, offs, val);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Drop:
            readI32(ctx->stack);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        
        case End:
            list_pop_tail(&ctx->blocks);
            return LIST_CONTAINER(instr->link.next, Instr , link);
    }

    // unexpected instruction
    return NULL;
}

typedef struct {
    uint32_t    base;
    uint32_t    len;
} Iovec;

int32_t fd_write(
    Context *ctx, 
    int32_t fd, 
    int32_t iovs, 
    int32_t iovs_len, 
    int32_t nwitten
) { 
    Iovec iov;
    int n;
    for(int i = 0; i < iovs_len; i++) {
        iov = *(Iovec *)&ctx->mem->p[iovs];

        // this is bad implemention
        // you better make sure format string is null terminated
        n = dprintf(fd, &ctx->mem->p[iov.base]);

        // write n
        storeI32(ctx->mem, nwitten, n);
        iovs += sizeof(Iovec);
    }
    return n;
}

int32_t invokeExternal(Context *ctx, WasmFunc *f) {
    // set args
     for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    // import from another wasm binary is not supported yet
    // imported functions take no arguments for now
    if((strcmp(f->modName, "wasi_unstable") == 0) && \
       (strcmp(f->name, "fd_write") == 0)) {
        return fd_write(
            ctx, 
            f->locals[0]->val.i32, 
            f->locals[1]->val.i32,
            f->locals[2]->val.i32, 
            f->locals[3]->val.i32
        );
    }

    return 0;
}

int32_t invokeInterrnal(Context *ctx, WasmFunc *f) {
    // set args
    for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    // exec
    Instr *instr = LIST_CONTAINER(list_head(f->codes), Instr, link);
    while(instr) {
        instr = invokeI(ctx, f, instr);
    }

    int32_t ret = 0;

    if(f->ty->rt2->n)
        ret = readI32(ctx->stack);

    return ret;
}

// todo: fix return type
int32_t invokeF(Context *ctx, WasmFunc *f) {
    if(f->imported)
        return invokeExternal(ctx, f);
    else
        return invokeInterrnal(ctx, f);
}

Instance *instantiate(WasmModule *m) {
    // find sections
    Section *typesec = NULL, *funcsec = NULL, \
            *codesec = NULL, *exportsec = NULL, \
            *importsec = NULL, *memsec = NULL, \
            *datasec = NULL;
    
    LIST_FOR_EACH(sec, &m->sections, Section, link) {
        switch(sec->id) {
            case TYPE_SECTION_ID:
                typesec = sec;
                break;
            case FUNC_SECTION_ID:
                funcsec = sec;
                break;
            case CODE_SECTION_ID:
                codesec = sec;
                break;
            case EXPORT_SECTION_ID:
                exportsec = sec;
                break;
            case IMPORT_SECTION_ID:
                importsec = sec;
                break;
            case MEM_SECTION_ID:
                memsec = sec;
                break;
            case DATA_SECTION_ID:
                datasec = sec;
                break;
        }
    }

    if(!funcsec)
        return NULL;

    // count functions(including imported functions)
    int num_imports = 0;
    if(importsec)
        num_imports = importsec->imports.n;
    
    int num_defined = funcsec->typeIdxes.n;

    int num_funcs =  num_imports + num_defined;
    Instance *instance = malloc(sizeof(Instance) + sizeof(WasmFunc *) * num_funcs);
    *instance = (Instance) {
        .typesec    = typesec,
        .funcsec    = funcsec,
        .codesec    = codesec,
        .exportsec  = exportsec,
        .importsec  = importsec,
        .memsec     = memsec,
        .datasec    = datasec
    };

    // create context
    // create stack
    uint8_t *buf = malloc(4096);
    instance->ctx.stack = newStack(buf, 4096);

    // init block
    LIST_INIT(&instance->ctx.blocks);

    // create mem if memsec is defined
    if(memsec) {
        // allocate one page for now
        uint8_t *page = calloc(1, 4096);
        instance->ctx.mem = newBuffer(page, 4096);
        // init mem if datasec is defined
        if(datasec) {
            Data *data;
            for(int i = 0; i < datasec->datas.n; i++) {
                data = datasec->datas.x[i];

                if(data->kind == 0) {
                    // get offs(constant expr expected)
                    LIST_FOR_EACH(instr, &data->expr, Instr, link) {
                        invokeI(&instance->ctx, NULL, instr);
                    }
                    int32_t offs = readI32(instance->ctx.stack);

                    // write data
                    for(uint32_t i = 0; i < data->n; i++) {
                        storeByte(instance->ctx.mem, offs + i, data->data[i]);
                    }
                }
            }
        }
    }

    // create functions(including imported functions)
    int funcIdx = 0;
    for(int i = 0; i < num_imports; i++) {
        instance->ctx.funcs[funcIdx++] = createImportedFunc(instance, i);
    }
    for(int i = 0; i < num_defined; i++) {
        instance->ctx.funcs[funcIdx++] = createDefinedFunc(instance, i);
    }

    return instance;
}

int32_t call(WasmModule *m, char *name, ...) {
    va_list ap;
    va_start(ap, name);

    Section *export = NULL;

    LIST_FOR_EACH(sec, &m->sections, Section, link) {
        if(sec->id == EXPORT_SECTION_ID) {
            export = sec;
            break;
        }
    }

    if(!export) {
        puts("export section undefined");
        return 0;   
    }

    Instance *instance = instantiate(m);
    if(!instance) {
        puts("failed to instantiate");
        return 0;
    }

    WasmFunc *f = NULL;
    Export *e;
    for(int i = 0; i < export->exports.n; i++) {
        e = export->exports.x[i];
        if(strcmp(e->name, name) == 0) {
            assert(e->exportDesc->kind == 0);
            f = instance->ctx.funcs[export->exports.x[i]->exportDesc->idx];
            break;
        }
    }

    if(!f) {
        printf("could not find %s\n", name);
        return 0;
    }

    // prepare params
    for(int i = 0; i < f->ty->rt1->n; i++) {
        switch(*f->ty->rt1->x[i]) {
            case I32:
                writeI32(instance->ctx.stack, va_arg(ap, int32_t));
                break;
            
            // todo: add type
        }
    }

    va_end(ap);
    return invokeF(&instance->ctx, f); 
}