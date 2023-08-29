#include <stdint.h>

__attribute__((
    __import_module__("env"),
    __import_name__("env_puts"),
))
int32_t env_puts(char *);

__attribute__((
    __import_module__("env"),
    __import_name__("env_exit"),
))
__attribute__((noreturn))
void env_exit(int32_t);
