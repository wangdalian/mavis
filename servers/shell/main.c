#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/env.h>

extern char __hello_start[];
extern int __hello_size[];

int main(void) {
     while (1) {
        printf("shell> ");
        char cmdline[128];
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                continue;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }
        
        if (strcmp(cmdline, "hello") == 0) {
            exec_vm_task(__hello_start, __hello_size[0]);
        }
        else if (strcmp(cmdline, "exit") == 0)
            break;
        else
            printf("unknown command: %s\n", cmdline);
    }

    return 0;
}