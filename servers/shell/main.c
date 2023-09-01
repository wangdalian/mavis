#include <stdio.h>
#include <string.h>

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
            puts("hello");
        }
        else if (strcmp(cmdline, "exit") == 0)
            break;
        else
            printf("unknown command: %s\n", cmdline);
    }

    return 0;
}