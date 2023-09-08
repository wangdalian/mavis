#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/env.h>
#include <kernel/message.h>

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
            struct message msg = {
                .type = SPAWN_TASK_MSG,
                .spawn_task = {.name = "hello"}
            };

            // todo: err handling
            // todo: impl call function
            ipc_send(3, &msg);

            ipc_receive(3, &msg);
            
            // wait for end
            for(;;) {
                ipc_receive(3, &msg);
                if(msg.type == DESTROY_TASK_MSG)
                    break;
            }
        }

        else if (strcmp(cmdline, "exit") == 0)
            break;
        else
            printf("unknown command: %s\n", cmdline);
    }

    return 0;
}