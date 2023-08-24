#include "task.h"
#include "list.h"
#include "module.h"

LocalValue *createLocalValue(ValType ty) {
    LocalValue *val = malloc(sizeof(LocalValue));
    *val = (LocalValue) {
        .ty         = ty,
        .val.i32    = 0
    };

    return val;
}

WasmFunc * createImportedFunc(WasmModule *m, int idx) {
    Import *info = m->importsec->imports.x[idx];
    FuncType *ty = m->typesec->funcTypes.x[info->importDesc->typeIdx];

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

WasmFunc *createDefinedFunc(WasmModule *m, int idx) {
    Func *f = m->codesec->codes.x[idx]->func;
    FuncType *ty = m->typesec->funcTypes.x[idx];

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

void enterBlock(Context *ctx, Instr *instr) {
    list_push_back(&ctx->blocks, &instr->link_block);
}

void exitBlock(Context *ctx) {
    list_pop_tail(&ctx->blocks);
}

Instr * invokeI(Context *ctx, WasmFunc *func, Instr *instr);

Instr * branchIn(Context *ctx, WasmFunc *func, int idx) {
    // todo: check block`s existence
    // todo: support if block?

    // exit blocks
    while(idx--) {
        exitBlock(ctx);
    }

    // taget block
    list_elem_t *block = list_tail(&ctx->blocks);
    Instr *instr = LIST_CONTAINER(block, Instr, link_block);

    switch(instr->op) {
        case Loop:
            return LIST_CONTAINER(
                list_head(&instr->block.instrs),
                Instr,
                link
            );
        case Block: {
            exitBlock(ctx);

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

static void printInstr(Instr *instr) {
    switch(instr->op) {
        case I32Const:
            printf("i32.const %#x\n", instr->i32Const.n);
            break;
        case I32Store:
            printf(
                "i32.store %#x %#x\n", 
                instr->i32Store.offset, 
                instr->i32Store.align
            );
            break;
        case LocalGet:
            printf("local.get %#x\n", instr->localGet.localIdx);
            break;
        case LocalSet:
            printf("local.set %#x\n", instr->localSet.localIdx);
            break;
        case I32Add:
            puts("i32.add");
            break;
        case I32Eqz:
            puts("i32.eqz");
            break;
        case I32Lt_s:
            puts("i32.lt_s");
            break;
        case I32Ge_s:
            puts("i32.ge_s");
            break;
        case I32Rem_s:
            puts("i32.rem_s");
            break;
        case If:
            printf("if %#x\n", instr->If.blockType);
            break;
        case Block:
        case Loop:
            printf("%s\n", instr->op == Block? "block" : "loop");
            break;
        case Br:
        case BrIf:
            printf(
                "%s %#x\n", 
                instr->op == Br? "br" : "br_if", 
                instr->br.labelIdx
            );
            break;
        case Call:
            printf("call %#x\n", instr->call.funcIdx);
            break;
        case Drop:
            puts("drop");
            break;
        case End:
            puts("end");
            break;
    }   
}

int32_t invokeF(Context *ctx, WasmFunc *f);

Instr * invokeI(Context *ctx, WasmFunc *func, Instr *instr) {
    printf("[+] ip = ");
    printInstr(ctx->ip);
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
            enterBlock(ctx, instr);
            int32_t cond = readI32(ctx->stack);
            if(cond) {
                ctx->ip = LIST_CONTAINER(
                    list_head(&instr->If.thenInstrs),
                    Instr,
                    link
                );

                while(ctx->ip->op != Else) {
                    ctx->ip = invokeI(ctx, func, ctx->ip);
                }
            } else {
                ctx->ip = LIST_CONTAINER(
                    list_head(&instr->If.elseInstrs),
                    Instr,
                    link
                );

                while(ctx->ip->op != End) {
                    ctx->ip = invokeI(ctx, func, ctx->ip);
                }
            }
            exitBlock(ctx);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Block:
        case Loop:
            enterBlock(ctx, instr);
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
            /*
            called when the last end instruction of the module is executed.
            The end instruction of a loop block, if block, etc. is not executed; the exitBlock function is used instead.
            */
            return NULL;
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
    ctx->ip = LIST_CONTAINER(list_head(f->codes), Instr, link);
    while(ctx->ip) {
        ctx->ip = invokeI(ctx, f, ctx->ip);
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

Task *createTask(WasmModule *m) {
    if(!m->funcsec)
        return NULL;
    
    Task *task = malloc(sizeof(Task));

    // count functions(including imported functions)
    int num_imports = 0;
    if(m->importsec)
        num_imports = m->importsec->imports.n;
    
    int num_defined =m->funcsec->typeIdxes.n;

    int num_funcs =  num_imports + num_defined;
    
    // create context
    Context *ctx = malloc(sizeof(Context) + sizeof(WasmFunc *) * num_funcs);

    // create stack
    uint8_t *buf = malloc(4096);
    ctx->stack = newStack(buf, 4096);

    // init block
    LIST_INIT(&ctx->blocks);

    // create mem if memsec is defined
    if(m->memsec) {
        // allocate one page for now
        uint8_t *page = calloc(1, 4096);
        ctx->mem = newBuffer(page, 4096);
        // init mem if datasec is defined
        if(m->datasec) {
            Data *data;
            for(int i = 0; i < m->datasec->datas.n; i++) {
                data = m->datasec->datas.x[i];

                if(data->kind == 0) {
                    // get offs(constant expr expected)
                    ctx->ip = LIST_CONTAINER(list_head(&data->expr), Instr, link);
                    do {
                        ctx->ip = invokeI(ctx, NULL, ctx->ip);
                    } while(ctx->ip->op != End);

                    int32_t offs = readI32(ctx->stack);

                    // write data
                    for(uint32_t i = 0; i < data->n; i++) {
                        storeByte(ctx->mem, offs + i, data->data[i]);
                    }
                }
            }
        }
    }

    // create functions(including imported functions)
    int funcIdx = 0;
    for(int i = 0; i < num_imports; i++) {
        ctx->funcs[funcIdx++] = createImportedFunc(m, i);
    }
    for(int i = 0; i < num_defined; i++) {
        ctx->funcs[funcIdx++] = createDefinedFunc(m, i);
    }

    task->ctx = ctx;

    return task;
}

int32_t call(WasmModule *m, char *name, ...) {
    va_list ap;
    va_start(ap, name);

    Section *exportsec = m->exportsec;

    if(!exportsec) {
        puts("export section undefined");
        return 0;   
    }

    Task *task = createTask(m);
    if(!task) {
        puts("failed to instantiate");
        return 0;
    }

    Context *ctx = task->ctx;
    WasmFunc *f = NULL;
    Export *e;
    for(int i = 0; i < exportsec->exports.n; i++) {
        e = exportsec->exports.x[i];
        if(strcmp(e->name, name) == 0) {
            assert(e->exportDesc->kind == 0);
            f = ctx->funcs[exportsec->exports.x[i]->exportDesc->idx];
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
                writeI32(ctx->stack, va_arg(ap, int32_t));
                break;
            
            // todo: add type
        }
    }

    va_end(ap);
    return invokeF(ctx, f); 
}