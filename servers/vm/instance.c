#include "instance.h"
#include "buffer.h"
#include "list.h"
#include "module.h"
#include <stdint.h>

LocalValue *createLocalValue(ValType ty) {
    LocalValue *val = malloc(sizeof(LocalValue));
    *val = (LocalValue) {
        .ty         = ty,
        .val.i32    = 0
    };

    return val;
}

WasmFunc *createWasmFunc(Instance *instance, int idx) {
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
        case End:
            list_pop_tail(&ctx->blocks);
            return LIST_CONTAINER(instr->link.next, Instr , link);
    }

    // unexpected instruction
    return NULL;
}

// todo: fix type
int32_t invokeF(Context *ctx, WasmFunc *f) {
    for(int i = 0; i < f->ty->rt1->n; i++) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(ctx->stack);
    }

    Instr *instr = LIST_CONTAINER(list_head(f->codes), Instr, link);
    while(instr) {
        instr = invokeI(ctx, f, instr);
    }

    int32_t ret = 0;

    if(f->ty->rt2->n)
        ret = readI32(ctx->stack);

    return ret;
}

Instance *instantiate(WasmModule *m) {
    // find sections
    Section *typesec = NULL, *funcsec = NULL, \
            *codesec = NULL, *exportsec = NULL;
    
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
        }
    }

    if(!funcsec)
        return NULL;

    int num_funcs = funcsec->typeIdxes.n;

    Instance *instance = malloc(sizeof(Instance) + sizeof(WasmFunc *) * num_funcs);
    *instance = (Instance) {
        .typesec    = typesec,
        .funcsec    = funcsec,
        .codesec    = codesec,
        .exportsec  = exportsec
    };

    // create context
    uint8_t *buf = malloc(4096);
    instance->ctx.stack = newStack(buf, 4096);
    LIST_INIT(&instance->ctx.blocks);

    // create functions
    for(int i = 0; i < num_funcs; i++) {
        instance->ctx.funcs[i] = createWasmFunc(instance, i);
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