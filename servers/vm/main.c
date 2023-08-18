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
#include "node.h"

static void fatal(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void printResultType(ResultType *v) {
    for(int i = 0; i < v->n; i++)
        printf("%#x ", *(*v->x)[i]);

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
        printFuncType((*sec->funcTypes.x)[i]);
    }
}

static void printFuncSection(Section *sec) {
    puts("[Func Section]");

    for(int i = 0; i < sec->typeIdxes.n; i++) {
        printf("%#x ", *(*sec->typeIdxes.x)[i]);
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

static void printInstr(Instr *instr) {
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
    }   
}

static void printFunc(Func *func) {
    for(int i = 0; i < func->locals.n; i++) {
        printLocal((*func->locals.x)[i]);
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
        printCode((*sec->codes.x)[i]);
    }
}


static void printExport(Export *export) {
    printf(
        "name: %s, exportDesc: %#x\n",
        export->name, 
        export->exportDesc
    );
}

static void printExportSection(Section *sec) {
    puts("[Export Section]");
    for(int i = 0; i < sec->exports.n; i++) {
        printExport((*sec->exports.x)[i]);
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
    Module *module = newModule(buf);
    
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
        }
    }

    munmap(head, s.st_size);
    close(fd);

    return 0;
}