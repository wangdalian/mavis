#include <lib/env.h>
#include <lib/string.h>
#include <lib/stdio.h>
#include <kernel/message.h>

// todo: impl file system

extern char __hello_start[];
extern int __hello_size[];

int main(void) {
    struct message req;
    for(;;) {
        // receive from any server
        ipc_receive(0, &req);

        // todo: use switch-case (cannot use this since runtime has not support branch_table yet)
        if(req.type == SPAWN_TASK_MSG) {
            if(strcmp(req.spawn_task.name, "hello") == 0) {
                
                // create hello world task
                puts("[vm] launching hello...");
                vm_create(__hello_start, __hello_size[0]);

                // todo: return tid
                struct message res = {.type = SPAWN_TASK_REPLY_MSG};

                ipc_send(req.src, &res);
            }
        }

        if(req.type == EXIT_TASK_MSG) {
            printf("[vm] tid %d exited normally\n", req.exit_task.tid);

            // send message to pager task
            // todo: fix this
            struct message res = {.type = DESTROY_TASK_MSG};
            ipc_send(2, &res);
        }
    }
}