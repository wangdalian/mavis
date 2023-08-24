#include "task.h"
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
    int typeIdx = *m->funcsec->typeIdxes.x[idx];

    Func *f = m->codesec->codes.x[idx]->func;
    FuncType *ty = m->typesec->funcTypes.x[typeIdx];
 
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
    puts("[+] enter block");
    list_push_back(&ctx->blocks, &instr->link_block);
}

void exitBlock(Context *ctx) {
    puts("[+] exit block");
    list_pop_tail(&ctx->blocks);
}

Instr * branchIn(Context *ctx, int idx) {
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

void enterFrame(Context *ctx, WasmFunc *f) {
    puts("[+] enter frame");
    list_push_back(&ctx->call_stack, &f->link);
}

void exitFrame(Context *ctx) {
    puts("[+] exit frame");
    list_pop_tail(&ctx->call_stack);
}

int32_t invokeF(Context *ctx, WasmFunc *f);

Instr * invokeI(Context *ctx, Instr *instr) {
    // get current func
    WasmFunc *func = LIST_CONTAINER(list_tail(&ctx->call_stack), WasmFunc, link);

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
                    ctx->ip = invokeI(ctx, ctx->ip);
                }
            } else {
                ctx->ip = LIST_CONTAINER(
                    list_head(&instr->If.elseInstrs),
                    Instr,
                    link
                );

                while(ctx->ip->op != End) {
                    ctx->ip = invokeI(ctx, ctx->ip);
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
            return branchIn(ctx, instr->br.labelIdx);
        
        case BrIf: {
            int32_t cond = readI32(ctx->stack);
            if(cond)
                return branchIn(ctx, instr->br.labelIdx);
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
        
        case End: {
            return NULL;
        }
    }

    // unexpected instruction
    return NULL;
}

Task tasks[NUM_TASK_MAX];

Task *createTask(WasmModule *m) {
    // funsec required
    if(!m->funcsec) {
        puts("[-] no funcsec");
        return NULL;
    }

    // _start function required
    Section *exportsec = m->exportsec;
    Export *export = NULL;
    
    for(int i = 0; i < exportsec->exports.n; i++) {
        Export *e = exportsec->exports.x[i];
        if(e->exportDesc->kind == 0 && strcmp(e->name, "_start") == 0) {
            export = e;
            break;
        }
    }
    if(!export) {
        puts("[-] no _start function");
        return NULL;
    }
    
    // find available task slot
    Task *task = NULL;
    int i;
    for(i = 0; i < NUM_TASK_MAX; i++) {
        if(tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_EXITED) {
            task = &tasks[i];
            break;
        }
    }

    if(!task) {
        puts("[-] no free task slots");
        return NULL;
    }

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

    // inti call stack
    LIST_INIT(&ctx->call_stack);

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
                        ctx->ip = invokeI(ctx, ctx->ip);
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

    // set tid
    task->tid = i;

    // set entry & ip
    WasmFunc *start = ctx->funcs[export->exportDesc->idx];
    ctx->ip = LIST_CONTAINER(list_head(start->codes), Instr, link);

    // set context
    task->ctx = ctx;

    // set state
    task->state = TASK_RUNNABLE;

    printf("[+] created task: tid =  %d\n", task->tid);

    return task;
}

Task *current_task; // current task

// idle task
void idle_task(void) {
    puts("[+] switched to idle task");
    exit(0);

    // todo: wait for new task
}

void switch_context(Context *ctx) {
    // todo: fix this
    while(ctx->ip->op != End) {
        ctx->ip = invokeI(ctx, ctx->ip);
    }
}

void yield(void) {
    Task *next = NULL;
    for(int i = 0; i < NUM_TASK_MAX; i++) {
        Task *task = &tasks[i];
        if(task->state == TASK_RUNNABLE) {
            next = task;
            break;
        }
    }

    if(!next)
        idle_task();

    current_task = next;
    switch_context(current_task->ctx);
}

// WASI
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

// builtin functions
void exitTask(int code) {
    current_task->state = TASK_EXITED;
    printf("[+] task exited normally: tid =  %d, exit_code = %d\n", current_task->tid, code);
    yield();
    puts("[+] unreachable!");
    exit(0);
}

int32_t invokeExternal(Context *ctx, WasmFunc *f) {
    // set args
     for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    // import from another wasm binary is not supported yet
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

    if(strcmp(f->modName, "env") == 0) {
        if(strcmp(f->name, "exit") == 0) {
            exitTask(f->locals[0]->val.i32);
        }
    }

    return 0;
}

int32_t invokeInternal(Context *ctx, WasmFunc *f) {
    enterFrame(ctx, f);
    // set args
    for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    // exec
    ctx->ip = LIST_CONTAINER(list_head(f->codes), Instr, link);
    while(ctx->ip->op != End) {
        ctx->ip = invokeI(ctx, ctx->ip);
    }

    int32_t ret = 0;

    if(f->ty->rt2->n)
        ret = readI32(ctx->stack);

    exitFrame(ctx);
    return ret;
}

// todo: fix return type
int32_t invokeF(Context *ctx, WasmFunc *f) {
    if(f->imported)
        return invokeExternal(ctx, f);
    else
        return invokeInternal(ctx, f);
}