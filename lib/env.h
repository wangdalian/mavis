#include <stdint.h>

__attribute__((
    __import_module__("env"),
    __import_name__("arch_serial_write"),
))
void arch_serial_write(char ch);

__attribute__((
    __import_module__("env"),
    __import_name__("arch_serial_read"),
))
int arch_serial_read(void);

__attribute__((
    __import_module__("env"),
    __import_name__("env_exit"),
))
__attribute__((noreturn))
void env_exit(int32_t);