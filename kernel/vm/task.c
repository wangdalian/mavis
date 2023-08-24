#include "task.h"

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

Instr * invokeI(Task *t, WasmFunc *func, Instr *instr);

Instr * branchIn(Task *t, WasmFunc *func, int idx) {
    // todo: check block`s existence
    // todo: support if block?
    list_elem_t *block = list_tail(&t->blocks);
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
                t, func, 
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

int32_t invokeF(Task *instance, WasmFunc *f);

Instr * invokeI(Task *t, WasmFunc *func, Instr *instr) {
    switch(instr->op) {
        case I32Const:
            writeI32(t->stack, instr->i32Const.n);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        
        case I32Add: {
            int32_t rhs = readI32(t->stack);
            int32_t lhs = readI32(t->stack);
            writeI32(t->stack, lhs + rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Rem_s: {
            // todo: assert rhs != 0
            int32_t rhs = readI32(t->stack);
            int32_t lhs = readI32(t->stack);
            writeI32(t->stack, lhs % rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Lt_s: {
            int32_t rhs = readI32(t->stack);
            int32_t lhs = readI32(t->stack);
            writeI32(t->stack, lhs < rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Ge_s: {
            int32_t rhs = readI32(t->stack);
            int32_t lhs = readI32(t->stack);
            writeI32(t->stack, lhs >= rhs);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Eqz: {
            int32_t c = readI32(t->stack);
            writeI32(t->stack, c == 0);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case LocalGet: {
            writeI32(
                t->stack, 
                func->locals[instr->localGet.localIdx]->val.i32
            );
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case LocalSet: {
            int32_t val = readI32(t->stack);
            func->locals[instr->localSet.localIdx]->val.i32 = val;
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Call: {
            int32_t ret = invokeF(t, t->funcs[instr->call.funcIdx]);
            if(ret)
                writeI32(t->stack, ret);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case If: {
            int32_t cond = readI32(t->stack);
            if(cond) {
                Instr *i = LIST_CONTAINER(
                    list_head(&instr->If.thenInstrs),
                    Instr,
                    link
                );

                while(i) {
                    i = invokeI(t, func, i);
                }
            } else {
                Instr *i = LIST_CONTAINER(
                    list_head(&instr->If.elseInstrs),
                    Instr,
                    link
                );

                while(i) {
                    i = invokeI(t, func, i);
                }
            }
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Block:
        case Loop:
            list_push_back(&t->blocks, &instr->link_block);
            return LIST_CONTAINER(
                list_head(&instr->block.instrs),
                Instr,
                link
            );
        
        case Br:
            return branchIn(t, func, instr->br.labelIdx);
        
        case BrIf: {
            int32_t cond = readI32(t->stack);
            if(cond)
                return branchIn(t, func, instr->br.labelIdx);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case I32Store: {
            /*
            memarg.offset of the t.store instruction is an exponent to the power of two: 2 for i32.store and 3 for i64.store.
            In the specification, the offset is added to the value popped from the stack at runtime, 
            However, in the current implementation, the offset is not used because the storeI32 function in buffer.c does the same thing.
            */
            int32_t val = readI32(t->stack);
            int32_t offs = readI32(t->stack);
            storeI32(t->mem, offs, val);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        }

        case Drop:
            readI32(t->stack);
            return LIST_CONTAINER(instr->link.next, Instr , link);
        
        case End:
            list_pop_tail(&t->blocks);
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
    Task *t, 
    int32_t fd, 
    int32_t iovs, 
    int32_t iovs_len, 
    int32_t nwitten
) { 
    Iovec iov;
    int n;
    for(int i = 0; i < iovs_len; i++) {
        iov = *(Iovec *)&t->mem->p[iovs];

        // this is bad implemention
        // you better make sure format string is null terminated
        n = dprintf(fd, &t->mem->p[iov.base]);

        // write n
        storeI32(t->mem, nwitten, n);
        iovs += sizeof(Iovec);
    }
    return n;
}

int32_t invokeExternal(Task *t, WasmFunc *f) {
    // set args
     for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(t->stack);
    }

    // import from another wasm binary is not supported yet
    // imported functions take no arguments for now
    if((strcmp(f->modName, "wasi_unstable") == 0) && \
       (strcmp(f->name, "fd_write") == 0)) {
        return fd_write(
            t, 
            f->locals[0]->val.i32, 
            f->locals[1]->val.i32,
            f->locals[2]->val.i32, 
            f->locals[3]->val.i32
        );
    }

    return 0;
}

int32_t invokeInterrnal(Task *t, WasmFunc *f) {
    // set args
    for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(t->stack);
    }

    // exec
    Instr *instr = LIST_CONTAINER(list_head(f->codes), Instr, link);
    while(instr) {
        instr = invokeI(t, f, instr);
    }

    int32_t ret = 0;

    if(f->ty->rt2->n)
        ret = readI32(t->stack);

    return ret;
}

// todo: fix return type
int32_t invokeF(Task *t, WasmFunc *f) {
    if(f->imported)
        return invokeExternal(t, f);
    else
        return invokeInterrnal(t, f);
}

Task *createTask(WasmModule *m) {
    if(!m->funcsec)
        return NULL;

    // count functions(including imported functions)
    int num_imports = 0;
    if(m->importsec)
        num_imports = m->importsec->imports.n;
    
    int num_defined =m->funcsec->typeIdxes.n;

    int num_funcs =  num_imports + num_defined;
    Task *task = malloc(sizeof(Task) + sizeof(WasmFunc *) * num_funcs);

    // create context
    // create stack
    uint8_t *buf = malloc(4096);
    task->stack = newStack(buf, 4096);

    // init block
    LIST_INIT(&task->blocks);

    // create mem if memsec is defined
    if(m->memsec) {
        // allocate one page for now
        uint8_t *page = calloc(1, 4096);
        task->mem = newBuffer(page, 4096);
        // init mem if datasec is defined
        if(m->datasec) {
            Data *data;
            for(int i = 0; i < m->datasec->datas.n; i++) {
                data = m->datasec->datas.x[i];

                if(data->kind == 0) {
                    // get offs(constant expr expected)
                    LIST_FOR_EACH(instr, &data->expr, Instr, link) {
                        invokeI(task, NULL, instr);
                    }
                    int32_t offs = readI32(task->stack);

                    // write data
                    for(uint32_t i = 0; i < data->n; i++) {
                        storeByte(task->mem, offs + i, data->data[i]);
                    }
                }
            }
        }
    }

    // create functions(including imported functions)
    int funcIdx = 0;
    for(int i = 0; i < num_imports; i++) {
        task->funcs[funcIdx++] = createImportedFunc(m, i);
    }
    for(int i = 0; i < num_defined; i++) {
        task->funcs[funcIdx++] = createDefinedFunc(m, i);
    }

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

    WasmFunc *f = NULL;
    Export *e;
    for(int i = 0; i < exportsec->exports.n; i++) {
        e = exportsec->exports.x[i];
        if(strcmp(e->name, name) == 0) {
            assert(e->exportDesc->kind == 0);
            f = task->funcs[exportsec->exports.x[i]->exportDesc->idx];
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
                writeI32(task->stack, va_arg(ap, int32_t));
                break;
            
            // todo: add type
        }
    }

    va_end(ap);
    return invokeF(task, f); 
}