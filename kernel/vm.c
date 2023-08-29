#include "vm.h"
#include "memory.h"
#include "common.h"
#include "env.h"

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
    WasmFunc *wasmf = malloc(sizeof(WasmFunc) + sizeof(LocalValue *) * num_params);
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
    //puts("[+] enter block");
    list_push_back(&ctx->blocks, &instr->link_block);
}

void exitBlock(Context *ctx) {
    //puts("[+] exit block");
    list_pop_tail(&ctx->blocks);
}

void enterFrame(Context *ctx, WasmFunc *f) {
    puts("[+] enter frame");
    list_push_back(&ctx->call_stack, &f->link);
}

void exitFrame(Context *ctx) {
    //puts("[+] exit frame");
    list_pop_tail(&ctx->call_stack);
}

Instr * branchIn(Context *ctx, int idx) {
    if(list_tail(&ctx->blocks) == NULL) {
        return NULL;
    }

    // todo: check block`s existence

    // exit blocks
    while(idx--) {
        exitBlock(ctx);
    }

    // taget block
    list_elem_t *block = list_tail(&ctx->blocks);
    Instr *instr = LIST_CONTAINER(block, Instr, link_block);

    switch(instr->op) {
        case Call:
            exitBlock(ctx);
            exitFrame(ctx);
            return LIST_CONTAINER(
                instr->link.next,
                Instr,
                link
            );
        case If:
            exitBlock(ctx);
            return LIST_CONTAINER(
                instr->link.next,
                Instr,
                link
            );
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

        // unexpected block
        default:
            return NULL;
    }
}

static void printInstr(Instr *instr) {
    switch(instr->op) {
        case I32Const:
            printf("i32.const %x\n", instr->i32Const.n);
            break;
        case I32Store:
            printf(
                "i32.store %x %x\n", 
                instr->i32Store.offset, 
                instr->i32Store.align
            );
            break;
        case LocalGet:
            printf("local.get %x\n", instr->localGet.localIdx);
            break;
        case LocalSet:
            printf("local.set %x\n", instr->localSet.localIdx);
            break;
        case GlobalGet:
            printf("global.get %x\n", instr->global_get.idx);
            break;
        case GlobalSet:
            printf("global.set %x\n", instr->global_set.idx);
            break;
        case I32Add:
            puts("i32.add");
            break;
        case I32Sub:
            puts("i32.sub");
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
            printf("if %x\n", instr->If.blockType);
            break;
        case Block:
        case Loop:
            printf("%s\n", instr->op == Block? "block" : "loop");
            break;
        case Br:
        case BrIf:
            printf(
                "%s %x\n", 
                instr->op == Br? "br" : "br_if", 
                instr->br.labelIdx
            );
            break;
        case Return:
            puts("return");
            break;
        case Unreachable:
            puts("unreachable");
            break;
        case Call:
            printf("call %x\n", instr->call.funcIdx);
            break;
        case Drop:
            puts("drop");
            break;
        case End:
            puts("end");
            break;
    }   
}

Instr *invokeF(Context *ctx, WasmFunc *f);

Instr *invokeI(Context *ctx, Instr *ip) {
    Instr *next_ip = LIST_CONTAINER(ip->link.next, Instr , link);
    
    // get current func
    WasmFunc *func = LIST_CONTAINER(list_tail(&ctx->call_stack), WasmFunc, link);
   
    printf("[+] ip = ");
    printInstr(ip);

    switch(ip->op) {
        case I32Const:
            writeI32(ctx->stack, ip->i32Const.n);
            break;
        
        case I32Add: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs + rhs);
            break;
        }

        case I32Sub: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs - rhs);
            break;
        }

        case I32Rem_s: {
            // todo: assert rhs != 0
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs % rhs);
            break;
        }

        case I32Lt_s: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs < rhs);
            break;
        }

        case I32Ge_s: {
            int32_t rhs = readI32(ctx->stack);
            int32_t lhs = readI32(ctx->stack);
            writeI32(ctx->stack, lhs >= rhs);
            break;
        }

        case I32Eqz: {
            int32_t c = readI32(ctx->stack);
            writeI32(ctx->stack, c == 0);
            break;
        }

        case LocalGet: {
            writeI32(
                ctx->stack, 
                func->locals[ip->localGet.localIdx]->val.i32
            );
            break;
        }

        case LocalSet: {
            int32_t val = readI32(ctx->stack);
            func->locals[ip->localSet.localIdx]->val.i32 = val;
            break;
        }

        case GlobalGet: {
            writeI32(
                ctx->stack, 
                ctx->globals[ip->global_get.idx]->val
            );
            break;
        }

        case GlobalSet: {
            int32_t val = readI32(ctx->stack);
            ctx->globals[ip->global_set.idx]->val = val;
            break;
        }

        case Call: {
            enterBlock(ctx, ip);
            next_ip = invokeF(ctx, ctx->funcs[ip->call.funcIdx]);
            break;
        }

        case If: {
            enterBlock(ctx, ip);
            int32_t cond = readI32(ctx->stack);
            if(cond) {
                next_ip = LIST_CONTAINER(
                    list_head(&ip->If.thenInstrs),
                    Instr,
                    link
                );
            } else {
                next_ip = LIST_CONTAINER(
                    list_head(&ip->If.elseInstrs),
                    Instr,
                    link
                );
            }
            break;
        }

        case Block:
        case Loop:
            enterBlock(ctx, ip);
            next_ip = LIST_CONTAINER(
                list_head(&ip->block.instrs),
                Instr,
                link
            );
            break;

        case Br:
            next_ip = branchIn(ctx, ip->br.labelIdx);
            break;
        
        case BrIf: {
            int32_t cond = readI32(ctx->stack);
            if(cond)
                next_ip = branchIn(ctx, ip->br.labelIdx);
            break;
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
            break;
        }

        case Nop:
            break;

        case Drop:
            readI32(ctx->stack);
            break;
        
        case Return:
        case End: 
        case Else:
            next_ip =  branchIn(ctx, 0);
            break;

        case Unreachable:
            // exit current task
            puts("[!] unreachable!");
            env_exit(0);
            break;
        
        default:
            next_ip = NULL;
            break;
    }

    return next_ip;
}

void run_vm(Context *ctx) {
    // todo: add entry to task struct
    Instr *ip = ctx->entry;

    while(ip)
        ip = invokeI(ctx, ip);
}

Context *createContext(WasmModule *m) {
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

    // count functions(including imported functions)
    int num_imports = 0;
    if(m->importsec)
        num_imports = m->importsec->imports.n;
    
    int num_defined =m->funcsec->typeIdxes.n;

    int num_funcs =  num_imports + num_defined;
    
    // create context
    Context *ctx = malloc(sizeof(Context) + sizeof(WasmFunc *) * num_funcs);

    // create stack
    uint8_t *buf = pmalloc(1);
    ctx->stack = newStack(buf, 4096);

    // inti call stack
    LIST_INIT(&ctx->call_stack);

    // init block
    LIST_INIT(&ctx->blocks);

    // init global variables if globalsec is defined
    if(m->globalsec) {
        int num_globals = m->globalsec->globals.n;
        global_variable **globals = malloc(sizeof(global_variable *) * num_globals);
        for(int i = 0; i < num_globals; i++) {
            global *g = m->globalsec->globals.x[i];
            global_variable *v = malloc(sizeof(global_variable));
            v->ty = g->ty;
            
            // calculate the initial value(constant expression expected)
            Instr *ip = LIST_CONTAINER(list_head(&g->expr), Instr, link);
            while(ip)
                ip = invokeI(ctx, ip);
                
            v->val = readI32(ctx->stack);
            globals[i] = v;
            printf("[+] globals %d = %x\n", i, v->val);
        }
        ctx->globals = globals;
    }

    // create mem if memsec is defined
    if(m->memsec) {
        // allocate one page for now
        uint8_t *page = pmalloc(1);
        ctx->mem = newBuffer(page, 4096);
        // init mem if datasec is defined
        if(m->datasec) {
            Data *data;
            for(int i = 0; i < m->datasec->datas.n; i++) {
                data = m->datasec->datas.x[i];

                if(data->kind == 0) {
                    // get offs(constant expr expected)
                    Instr *ip = LIST_CONTAINER(list_head(&data->expr), Instr, link);
                    while(ip)
                        ip = invokeI(ctx, ip);

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

    // set entry & ip
    WasmFunc *start = ctx->funcs[export->exportDesc->idx];
    enterFrame(ctx, start);
    ctx->entry = LIST_CONTAINER(list_head(start->codes), Instr, link);

    return ctx;
}

int32_t invokeExternal(Context *ctx, WasmFunc *f) {
    // import from another wasm binary is not supported yet
    if(strcmp(f->modName, "env") == 0) {
        if(strcmp(f->name, "env_exit") == 0) {
            env_exit(f->locals[0]->val.i32);
        }
        if(strcmp(f->name, "env_puts") == 0) {
            env_puts(f->locals[0]->val.i32);
        }
    }

    return 0;
}

Instr *invokeF(Context *ctx, WasmFunc *f) {
    enterFrame(ctx, f);
    
    // set args
    for(int i = f->ty->rt1->n - 1; i >= 0; i--) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    for(int i = 0; i < f->ty->rt1->n; i++) {
        printf("[+] arg %d = %d\n", i, f->locals[i]->val.i32);
    }

    if(f->imported) {
        int32_t ret = invokeExternal(ctx, f);
        if(f->ty->rt2->n)
            writeI32(ctx->stack, ret);
        return branchIn(ctx, 0);
    }
    else {
        return LIST_CONTAINER(list_head(f->codes), Instr, link);
    }
}