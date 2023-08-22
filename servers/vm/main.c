#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "buffer.h"
#include "list.h"
#include "module.h"
#include "instance.h"

static void fatal(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void printResultType(ResultType *v) {
    for(int i = 0; i < v->n; i++)
        printf("%#x ", *v->x[i]);

    if(v->n)
        putchar('\n');
}

static void printFuncType(FuncType *funcType) {
    puts("param type");
    printResultType(funcType->rt1);
    puts("ret type");
    printResultType(funcType->rt2);
}

static void printTypeSection(Section *sec) {
    puts("[Type Section]");
    for(int i = 0; i < sec->funcTypes.n; i++) {
        printFuncType(sec->funcTypes.x[i]);
    }
}

static void printFuncSection(Section *sec) {
    puts("[Func Section]");

    for(int i = 0; i < sec->typeIdxes.n; i++) {
        printf("%#x ", *sec->typeIdxes.x[i]);
    }

    putchar('\n');
}

static void printLocal(Locals *local) {
    printf(
        "num = %#x, type = %#x\n",
        local->num,
        local->ty
    );
}

void printInstr(Instr *instr) {
    switch(instr->op) {
        case I32Const:
            printf("i32.const %#x\n", instr->i32Const.n);
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
            LIST_FOR_EACH(i, &instr->If.thenInstrs, Instr, link) {
                printInstr(i);
            }
            puts("else");
            LIST_FOR_EACH(i, &instr->If.elseInstrs, Instr, link) {
                printInstr(i);
            }
            break;
        case Block:
        case Loop:
            printf("%s\n", instr->op == Block? "block" : "loop");
            LIST_FOR_EACH(i, &instr->block.instrs, Instr, link) {
                printInstr(i);
            }
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

static void printFunc(Func *func) {
    for(int i = 0; i < func->locals.n; i++) {
        printLocal(func->locals.x[i]);
    }

    LIST_FOR_EACH(instr, &func->expr, Instr, link) {
        printInstr(instr);
    }
}

static void printCode(Code *code) {
    printf("size = %#x\n", code->size);
    printFunc(code->func);
}

static void printCodeSection(Section *sec) {
    puts("[Code Section]");

    for(int i = 0; i < sec->codes.n; i++) {
        printCode(sec->codes.x[i]);
    }
}

static void printExport(Export *export) {
    printf(
        "name: %s, exportDesc: kind: %#x, idx: %#x\n",
        export->name, 
        export->exportDesc->kind,
        export->exportDesc->idx
    );
}

static void printExportSection(Section *sec) {
    puts("[Export Section]");
    for(int i = 0; i < sec->exports.n; i++) {
        printExport(sec->exports.x[i]);
    }
}

static void printImport(Import *import) {
    printf(
        "from %s import %s, kind = %#x, idx = %#x\n",
        import->modName,
        import->name,
        import->importDesc->kind,
        import->importDesc->typeIdx
    );
}

static void printImportSection(Section *sec) {
    puts("[Import Section]");
    for(int i = 0; i < sec->imports.n; i++) {
        printImport(sec->imports.x[i]);
    }
}

static void printMem(Mem *m) {
    switch(m->mt.kind) {
        case 0:
            printf("min = %#x\n", m->mt.min);
            break;
        case 1:
            printf("min = %#x, max = %#x\n", m->mt.min, m->mt.max);
            break;
    }
}

static void printMemSection(Section *sec) {
    puts("[Mem Section]");
    for(int i = 0; i < sec->mems.n; i++) {
        printMem(sec->mems.x[i]);
    }
}

static void printData(Data *data) {
    switch(data->kind) {
        case 0:
            puts("exprt:");
            LIST_FOR_EACH(instr, &data->expr, Instr, link) {
                printInstr(instr);
            }
            puts("data:");
            for(uint32_t i = 0; i < data->n; i++) {
                printf("%x", data->data[i]);
            }
            putchar('\n');
            break;
        case 1:
        case 2:
            break;
    }
}

static void printDataSection(Section *sec) {
    puts("[Data Section]");
    for(int i = 0; i < sec->datas.n; i++) {
        printData(sec->datas.x[i]);
    }
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        puts("Usage: ./a.out <*.wasm>");
    }

    int fd = open(argv[1], O_RDWR);
    if(fd == -1) fatal("open");

    struct stat s;
    if(fstat(fd, &s) == -1) fatal("fstat");

    uint8_t *head = mmap(
        NULL,
        s.st_size,
        PROT_READ,
        MAP_PRIVATE,
        fd, 
        0
    );
    if(head == MAP_FAILED) fatal("mmap");

    Buffer *buf = newBuffer(head, s.st_size);
    WasmModule *module = newWasmModule(buf);
    
    printf(
        "magic = %#x, version = %#x\n", 
        module->magic,
        module->version
    );

    LIST_FOR_EACH(sec, &module->sections, Section, link) {
        switch(sec->id) {
            case TYPE_SECTION_ID:
                printTypeSection(sec);
                break;
            case CODE_SECTION_ID:
                printCodeSection(sec);
                break;
            case FUNC_SECTION_ID:
                printFuncSection(sec);
                break;
            case EXPORT_SECTION_ID:
                printExportSection(sec);
                break;
            case IMPORT_SECTION_ID:
                printImportSection(sec);
                break;
            case MEM_SECTION_ID:
                printMemSection(sec);
                break;
            case DATA_SECTION_ID:
                printDataSection(sec);
                break;
        }
    }
    //int32_t ret = call(module, "main");
    //printf("ret = %d\n", ret);

    munmap(head, s.st_size);
    close(fd);

    return 0;
}