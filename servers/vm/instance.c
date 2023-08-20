#include "instance.h"

LocalValue *createLocalValue(ValType ty) {
    LocalValue *val = malloc(sizeof(LocalValue));
    *val = (LocalValue) {
        .ty         = ty,
        .val.i32    = 0
    };

    return val;
}

WasmFunc *createWasmFunc(WasmModule *m, int idx) {
    Section *typesec = NULL, *codesec = NULL;

    LIST_FOR_EACH(sec, &m->sections, Section, link) {
        if(sec->id == TYPE_SECTION_ID)
            typesec = sec;
        if(sec->id == CODE_SECTION_ID)
            codesec = sec;
    }

    Func *f = codesec->codes.x[idx]->func;
    FuncType *ty = typesec->funcTypes.x[idx];

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

int32_t invokeF(Instance *instance, WasmFunc *f);

void invokeI(Instance *instance, WasmFunc *func, Instr *instr) {
    switch(instr->op) {
        case I32Const:
            writeI32(instance->stack, instr->i32Const.n);
            break;
        case I32Add: {
            int32_t lhs = readI32(instance->stack);
            int32_t rhs = readI32(instance->stack);
            writeI32(instance->stack, lhs + rhs);
            break;
        }
        case LocalGet: {
            writeI32(
                instance->stack, 
                func->locals[instr->localGet.localIdx]->val.i32
            );
            break;
        }
        case Call: {
            int32_t ret = invokeF(instance, instance->funcs[instr->call.funcIdx]);
            if(ret)
                writeI32(instance->stack, ret);
            break;
        }
        case End:
            break;
    }
}

// todo: fix type
int32_t invokeF(Instance *instance, WasmFunc *f) {
    for(int i = 0; i < f->ty->rt1->n; i++) {
        // todo: validate type
        f->locals[i]->val.i32 = readI32(instance->stack);
    }

    LIST_FOR_EACH(instr, f->codes, Instr, link) {
        invokeI(instance, f, instr);
    }

    int32_t ret = 0;

    if(f->ty->rt2->n)
        ret = readI32(instance->stack);

    return ret;
}

Instance *instantiate(WasmModule *m) {
    Section *funcsec = NULL;

    LIST_FOR_EACH(sec, &m->sections, Section, link) {
        if(sec->id == FUNC_SECTION_ID) {
            funcsec = sec;
            break;
        }
    }
    if(!funcsec)
        return NULL;

    int num_funcs = funcsec->typeIdxes.n;

    Instance *instance = malloc(sizeof(Instance) + sizeof(WasmFunc *) * num_funcs);

    // create stack
    uint8_t *buf = malloc(4096);
    instance->stack = newStack(buf, 4096);

    // create functions
    for(int i = 0; i < num_funcs; i++) {
        instance->funcs[i] = createWasmFunc(m, i);
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
            f = instance->funcs[export->exports.x[i]->exportDesc->idx];
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
                writeI32(instance->stack, va_arg(ap, int32_t));
                break;
            
            // todo: add type
        }
    }

    va_end(ap);
    return invokeF(instance, f); 
}